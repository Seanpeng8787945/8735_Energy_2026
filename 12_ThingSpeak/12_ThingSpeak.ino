/*
  12_ThingSpeak
  整合 09_OLED_DHT_led_beep、10_OLED_AQI_PM 與 ThingSpeak：
  - 第 1 頁：桃園市中壢 AQI、PM2.5
  - 第 2 頁：DHT11 溫度、濕度
  - OLED 每 5 秒切換頁面
  - AQI 每 1 分鐘透過 Wi-Fi 更新
  - ThingSpeak 每 30 秒上傳溫度、濕度、AQI、PM2.5
  - 保留溫濕度門檻 LED 與蜂鳴器警報
*/

#include <WiFi.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <U8g2lib.h>

char WIFI_SSID[] = "Seanpeng8787";
const char WIFI_PASSWORD[] = "0968709788";
const char API_HOST[] = "data.moenv.gov.tw";
const char API_PATH[] = "/api/v2/AQX_P_432?api_key=6b143ef9-f251-43e4-add6-a5557c7ffa1c";

const int DHT_PIN = 20;
const int GREEN_LED_PIN = 24;
const int YELLOW_LED_PIN = 23;
const int RED_LED_PIN = 22;
const int BUZZER_PIN = 11;
#define DHT_TYPE DHT11

const unsigned long SENSOR_INTERVAL_MS = 2000UL;
const unsigned long AQI_INTERVAL_MS = 60000UL;
const unsigned long THINGSPEAK_INTERVAL_MS = 30000UL;
const unsigned long PAGE_INTERVAL_MS = 5000UL;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000UL;

DHT dht(DHT_PIN, DHT_TYPE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiSSLClient httpsClient;
WiFiClient thingspeakClient;
const char THINGSPEAK_HOST[] = "api.thingspeak.com";
const char THINGSPEAK_API_KEY[] = "D6H5XMAOAOQRGWX0";

float currentTemperature = 0.0;
float currentHumidity = 0.0;
bool sensorReady = false;
bool hasAirData = false;
String latestAqi = "--";
String latestPm25 = "--";
unsigned long lastSensorMs = 0;
unsigned long lastAqiMs = 0;
unsigned long lastThingSpeakMs = 0;
unsigned long lastPageMs = 0;
unsigned long lastWiFiRetryMs = 0;
int displayPage = 0;
bool alarmActive = false;
bool alarmHighTone = false;
unsigned long lastAlarmMs = 0;

void drawMessage(const char* line1, const char* line2 = nullptr) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 25, line1);
  if (line2 != nullptr) u8g2.drawStr(0, 43, line2);
  u8g2.sendBuffer();
}

void drawAirPage() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Zhongli, Taoyuan");
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(0, 34);
  u8g2.print("AQI: ");
  u8g2.print(latestAqi);
  u8g2.setCursor(0, 55);
  u8g2.print("PM2.5: ");
  u8g2.print(latestPm25);
  u8g2.print(" ug");
  u8g2.sendBuffer();
}

void drawDhtPage() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Temperature / Humidity");
  if (!sensorReady) {
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.drawStr(0, 38, "DHT11 ERROR");
    u8g2.sendBuffer();
    return;
  }
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(0, 34);
  u8g2.print("T: ");
  u8g2.print(currentTemperature, 0);
  u8g2.print(" C");
  u8g2.setCursor(0, 55);
  u8g2.print("H: ");
  u8g2.print(currentHumidity, 0);
  u8g2.print(" %");
  u8g2.sendBuffer();
}

void drawCurrentPage() {
  if (displayPage == 0) drawAirPage();
  else drawDhtPage();
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  drawMessage("wifi connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  drawMessage("wifi connected");
  delay(1500);
}

bool skipHttpHeaders(WiFiSSLClient& client, bool& isChunked) {
  String line;
  unsigned long startMs = millis();
  isChunked = false;
  while (millis() - startMs < 15000UL) {
    while (client.available()) {
      char c = static_cast<char>(client.read());
      if (c == '\n') {
        if (line.length() == 0 || line == "\r") return true;
        if (line.indexOf("Transfer-Encoding: chunked") >= 0 ||
            line.indexOf("transfer-encoding: chunked") >= 0) isChunked = true;
        line = "";
      } else if (c != '\r') line += c;
    }
    if (!client.connected() && !client.available()) break;
    delay(1);
  }
  return false;
}

bool readHttpLine(WiFiSSLClient& client, String& line) {
  line = "";
  unsigned long startMs = millis();
  while (millis() - startMs < 15000UL) {
    while (client.available()) {
      char c = static_cast<char>(client.read());
      if (c == '\n') return true;
      if (c != '\r') line += c;
    }
    if (!client.connected() && !client.available()) return line.length() > 0;
    delay(1);
  }
  return false;
}

bool readResponseBody(WiFiSSLClient& client, bool isChunked, String& body) {
  body = "";
  if (!isChunked) {
    unsigned long startMs = millis();
    while (millis() - startMs < 20000UL) {
      while (client.available()) {
        body += static_cast<char>(client.read());
        startMs = millis();
      }
      if (!client.connected() && !client.available()) return true;
      delay(1);
    }
    return false;
  }
  String sizeLine;
  while (true) {
    if (!readHttpLine(client, sizeLine)) return false;
    int separator = sizeLine.indexOf(';');
    if (separator >= 0) sizeLine = sizeLine.substring(0, separator);
    unsigned long chunkSize = strtoul(sizeLine.c_str(), nullptr, 16);
    if (chunkSize == 0) {
      do { if (!readHttpLine(client, sizeLine)) return false; }
      while (sizeLine.length() > 0);
      return true;
    }
    unsigned long remaining = chunkSize;
    while (remaining > 0) {
      while (!client.available()) {
        if (!client.connected()) return false;
        delay(1);
      }
      size_t toRead = remaining > 256UL ? 256U : static_cast<size_t>(remaining);
      uint8_t buffer[256];
      int count = client.read(buffer, toRead);
      if (count <= 0) return false;
      for (int i = 0; i < count; ++i) body += static_cast<char>(buffer[i]);
      remaining -= static_cast<unsigned long>(count);
    }
    if (!readHttpLine(client, sizeLine) || sizeLine.length() != 0) return false;
  }
}

bool fetchAirQuality() {
  httpsClient.stop();
  httpsClient.setRecvTimeout(15000);
  if (!httpsClient.connect(API_HOST, 443)) return false;
  httpsClient.print("GET ");
  httpsClient.print(API_PATH);
  httpsClient.println(" HTTP/1.1");
  httpsClient.print("Host: ");
  httpsClient.println(API_HOST);
  httpsClient.println("Connection: close");
  httpsClient.println();

  bool isChunked = false;
  if (!skipHttpHeaders(httpsClient, isChunked)) {
    httpsClient.stop();
    return false;
  }
  String responseBody;
  if (!readResponseBody(httpsClient, isChunked, responseBody)) {
    httpsClient.stop();
    return false;
  }

  JsonDocument filter;
  filter[0]["county"] = true;
  filter[0]["sitename"] = true;
  filter[0]["aqi"] = true;
  filter[0]["pm2.5"] = true;
  filter[0]["pm2.5_avg"] = true;
  JsonDocument document;
  DeserializationError error = deserializeJson(
    document, responseBody, DeserializationOption::Filter(filter));
  httpsClient.stop();
  if (error) return false;

  for (JsonObject record : document.as<JsonArray>()) {
    const char* county = record["county"] | "";
    const char* siteName = record["sitename"] | "";
    if (strcmp(county, "桃園市") != 0 || strcmp(siteName, "中壢") != 0) continue;
    const char* aqi = record["aqi"] | "";
    const char* pm25 = record["pm2.5"] | "";
    const char* pm25Average = record["pm2.5_avg"] | "";
    if (aqi[0] == '\0') return false;
    latestAqi = aqi;
    latestPm25 = pm25[0] != '\0' ? pm25 : pm25Average;
    return latestPm25.length() > 0;
  }
  return false;
}

void updateLeds() {
  bool highTemp = sensorReady && currentTemperature > 25.0;
  bool highHumidity = sensorReady && currentHumidity > 80.0;
  digitalWrite(RED_LED_PIN, highTemp ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, highHumidity ? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, sensorReady && !highTemp && !highHumidity ? HIGH : LOW);
}

void updateAlarm() {
  bool shouldAlarm = sensorReady &&
    (currentTemperature > 25.0 || currentHumidity > 80.0);
  if (!shouldAlarm) {
    if (alarmActive) noTone(BUZZER_PIN);
    alarmActive = false;
    return;
  }
  if (!alarmActive || millis() - lastAlarmMs >= 500UL) {
    alarmActive = true;
    alarmHighTone = !alarmHighTone;
    tone(BUZZER_PIN, alarmHighTone ? 1250 : 650);
    lastAlarmMs = millis();
  }
}

void updateSensor() {
  if (millis() - lastSensorMs < SENSOR_INTERVAL_MS && sensorReady) return;
  lastSensorMs = millis();
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    sensorReady = false;
    updateLeds();
    return;
  }
  currentHumidity = humidity;
  currentTemperature = temperature;
  sensorReady = true;
  updateLeds();
}

void updateAirPage() {
  drawMessage("data updating...");
  if (!fetchAirQuality() && !hasAirData) {
    drawCurrentPage();
    return;
  }
  if (latestAqi.length() > 0 && latestPm25.length() > 0) hasAirData = true;
  drawCurrentPage();
}

bool uploadThingSpeak() {
  if (!sensorReady || !hasAirData || latestAqi == "--" || latestPm25 == "--") {
    return false;
  }

  drawMessage("data uploading...");

  String requestPath = "/update?api_key=";
  requestPath += THINGSPEAK_API_KEY;
  requestPath += "&field1=";
  requestPath += String(currentTemperature, 1);
  requestPath += "&field2=";
  requestPath += String(currentHumidity, 1);
  requestPath += "&field3=";
  requestPath += latestAqi;
  requestPath += "&field4=";
  requestPath += latestPm25;

  thingspeakClient.stop();
  if (!thingspeakClient.connect(THINGSPEAK_HOST, 80)) {
    drawCurrentPage();
    return false;
  }

  thingspeakClient.print("GET ");
  thingspeakClient.print(requestPath);
  thingspeakClient.println(" HTTP/1.1");
  thingspeakClient.print("Host: ");
  thingspeakClient.println(THINGSPEAK_HOST);
  thingspeakClient.println("Connection: close");
  thingspeakClient.println();

  unsigned long startMs = millis();
  while (thingspeakClient.connected() && millis() - startMs < 5000UL) {
    while (thingspeakClient.available()) thingspeakClient.read();
    delay(1);
  }
  thingspeakClient.stop();
  drawCurrentPage();
  return true;
}

void setup() {
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();
  dht.begin();
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  updateSensor();
  connectWiFi();
  updateAirPage();
  lastAqiMs = millis();
  lastThingSpeakMs = millis();
  lastPageMs = millis();
}

void loop() {
  unsigned long now = millis();
  updateSensor();
  updateAlarm();

  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWiFiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
      lastWiFiRetryMs = now;
      connectWiFi();
    }
  } else if (now - lastAqiMs >= AQI_INTERVAL_MS) {
    lastAqiMs = now;
    updateAirPage();
  }

  if (now - lastThingSpeakMs >= THINGSPEAK_INTERVAL_MS) {
    lastThingSpeakMs = now;
    uploadThingSpeak();
  }

  if (now - lastPageMs >= PAGE_INTERVAL_MS) {
    lastPageMs = now;
    displayPage = displayPage == 0 ? 1 : 0;
    drawCurrentPage();
  }
  delay(10);
}
