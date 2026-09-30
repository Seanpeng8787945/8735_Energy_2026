/*
  專案名稱：09_OLED_DHT_led_beep
  功能：
  1. 使用 DHT11 讀取溫度與濕度。
  2. 在 SSD1306 OLED 顯示溫濕度與動態圖示。
  3. 依照溫度與濕度控制紅、黃、綠三個 LED。
  4. 溫度或濕度過高時，使用無源蜂鳴器發出消防車警報聲。

  感測器與輸出接線：
  - DHT11 DATA -> IO20
  - OLED SDA -> IO3，SCL/SCK -> IO4，I2C 位址 0x3C
  - 綠燈 -> IO24
  - 黃燈 -> IO23
  - 紅燈 -> IO22
  - 無源蜂鳴器正極 -> IO11
  - 無源蜂鳴器負極 -> GND

  控制條件：
  - 濕度 > 80%：黃燈亮，模擬除濕機，啟動警報聲。
  - 溫度 > 25°C：紅燈亮，模擬冷氣，啟動警報聲。
  - 溫度與濕度皆正常：只亮綠燈，關閉警報聲。
  - 溫度與濕度同時過高：黃燈與紅燈同時亮，持續警報。
*/

#include "DHT.h"
#include <Wire.h>
#include <U8g2lib.h>

// DHT11 設定
const int DHT_PIN = 20;
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// LED 與蜂鳴器腳位
const int GREEN_LED_PIN = 24;
const int YELLOW_LED_PIN = 23;
const int RED_LED_PIN = 22;
const int BUZZER_PIN = 11;

// SSD1306 128x64，硬體 I2C，使用第一組 I2C：Wire
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// 最近一次成功讀取的感測值
float currentTemperature = 0.0;
float currentHumidity = 0.0;
bool sensorReady = false;
unsigned long lastSensorRead = 0;

// 警報音高低頻切換控制
bool alarmActive = false;
bool alarmHighTone = false;
unsigned long lastAlarmChange = 0;

void drawThermometerIcon(int x, int y, float temperature) {
  u8g2.drawFrame(x + 3, y, 10, 25);
  u8g2.drawCircle(x + 8, y + 28, 6);
  u8g2.drawLine(x + 14, y + 4, x + 17, y + 4);
  u8g2.drawLine(x + 14, y + 12, x + 17, y + 12);
  u8g2.drawLine(x + 14, y + 20, x + 17, y + 20);

  int temperatureLevel = constrain((int)temperature, 10, 50);
  int filledSegments = map(temperatureLevel, 10, 50, 0, 8);

  for (int segment = 0; segment < filledSegments; segment++) {
    int segmentY = y + 21 - (segment * 3);
    u8g2.drawBox(x + 5, segmentY, 6, 2);
  }

  if (filledSegments > 0) {
    u8g2.drawDisc(x + 8, y + 28, 5);
  }
}

void drawDropletIcon(int x, int y, float humidity) {
  u8g2.drawTriangle(x + 8, y, x + 1, y + 14, x + 8, y + 25);
  u8g2.drawTriangle(x + 8, y, x + 15, y + 14, x + 8, y + 25);
  u8g2.drawCircle(x + 8, y + 18, 7);

  int humidityLevel = constrain((int)humidity, 0, 100);
  int waterHeight = map(humidityLevel, 0, 100, 0, 20);
  int centerX = x + 8;

  if (humidityLevel >= 100) {
    u8g2.drawTriangle(x + 8, y, x + 1, y + 14, x + 8, y + 25);
    u8g2.drawTriangle(x + 8, y, x + 15, y + 14, x + 8, y + 25);
    u8g2.drawDisc(x + 8, y + 18, 7);
    u8g2.drawTriangle(x + 8, y, x + 1, y + 14, x + 8, y + 25);
    u8g2.drawTriangle(x + 8, y, x + 15, y + 14, x + 8, y + 25);
  } else {
    for (int row = 0; row < waterHeight; row++) {
      int rowY = y + 24 - row;
      int halfWidth = min(7, 2 + row / 3);
      u8g2.drawHLine(centerX - halfWidth, rowY, halfWidth * 2 + 1);
    }
  }
}

void updateLEDs() {
  bool highTemperature = currentTemperature > 25.0;
  bool highHumidity = currentHumidity > 80.0;

  digitalWrite(RED_LED_PIN, highTemperature ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, highHumidity ? HIGH : LOW);
  digitalWrite(
    GREEN_LED_PIN,
    (!highTemperature && !highHumidity) ? HIGH : LOW
  );
}

void updateAlarm() {
  bool shouldAlarm = currentTemperature > 25.0 || currentHumidity > 80.0;

  if (!shouldAlarm) {
    if (alarmActive) {
      noTone(BUZZER_PIN);
      alarmActive = false;
    }
    return;
  }

  // 超標時每 500 ms 在低頻與高頻之間切換，形成消防車警報聲。
  if (!alarmActive || millis() - lastAlarmChange >= 500) {
    alarmActive = true;
    alarmHighTone = !alarmHighTone;
    tone(BUZZER_PIN, alarmHighTone ? 1250 : 650);
    lastAlarmChange = millis();
  }
}

void showSensorError() {
  noTone(BUZZER_PIN);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(5, 24, "DHT11");
  u8g2.drawStr(5, 50, "ERROR");
  u8g2.sendBuffer();
}

void drawDisplay() {
  u8g2.clearBuffer();
  u8g2.drawVLine(63, 5, 54);

  drawThermometerIcon(6, 14, currentTemperature);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(27, 17, "TEMP");
  u8g2.setFont(u8g2_font_9x15_tf);
  u8g2.setCursor(27, 46);
  u8g2.print(currentTemperature, 0);
  u8g2.print("C");

  drawDropletIcon(74, 5, currentHumidity);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(96, 17, "HUMI");
  u8g2.setFont(u8g2_font_9x15_tf);
  u8g2.setCursor(82, 46);
  u8g2.print(currentHumidity, 0);
  u8g2.print("%");

  u8g2.sendBuffer();
}

void setup() {
  u8g2.setI2CAddress(0x3C << 1);
  u8g2.begin();
  dht.begin();

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
}

void loop() {
  // 每 2 秒讀取一次 DHT11；警報音則由 millis() 持續控制。
  if (millis() - lastSensorRead >= 2000 || !sensorReady) {
    lastSensorRead = millis();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      sensorReady = false;
      showSensorError();
    } else {
      currentHumidity = humidity;
      currentTemperature = temperature;
      sensorReady = true;
      updateLEDs();
      drawDisplay();
    }
  }

  if (sensorReady) {
    updateAlarm();
  }

  delay(10);
}
