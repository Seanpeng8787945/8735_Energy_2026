/*
  專案名稱：07_OLED_DHT_led
  功能：
  1. 使用 DHT11 讀取溫度與濕度。
  2. 在 SSD1306 OLED 顯示溫濕度與動態圖示。
  3. 依照溫度與濕度控制紅、黃、綠三個 LED。

  DHT11：
  - VCC  -> 3.3V
  - DATA -> IO20
  - GND  -> GND

  OLED SSD1306：
  - 解析度：128 x 64
  - I2C 位址：0x3C
  - 使用第一組 I2C：Wire
  - SDA -> IO3
  - SCL/SCK -> IO4

  LED：
  - 綠燈 -> IO24：正常狀態
  - 黃燈 -> IO23：濕度過高，模擬開啟除濕機
  - 紅燈 -> IO22：溫度過高，模擬開啟冷氣

  控制條件：
  - 濕度 > 80%：黃燈亮
  - 溫度 > 25°C：紅燈亮
  - 溫度與濕度皆正常：只亮綠燈
  - 溫度與濕度同時過高：黃燈與紅燈同時亮
*/

#include "DHT.h"
#include <Wire.h>
#include <U8g2lib.h>

// DHT11 設定
const int DHT_PIN = 20;
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// LED 設定
const int GREEN_LED_PIN = 24;
const int YELLOW_LED_PIN = 23;
const int RED_LED_PIN = 22;

// SSD1306 128x64，使用第一組 I2C：Wire
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// 溫度計圖示：10°C 為最低，50°C 為最高，共 8 格液柱
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

// 水滴圖示：0% 為空心，100% 為實心，中間依水位填充
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

void updateLEDs(float temperature, float humidity) {
  bool highTemperature = temperature > 25.0;
  bool highHumidity = humidity > 80.0;

  // 紅燈模擬冷氣，黃燈模擬除濕機。
  digitalWrite(RED_LED_PIN, highTemperature ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, highHumidity ? HIGH : LOW);

  // 只有溫度與濕度都正常時，才亮綠燈。
  digitalWrite(
    GREEN_LED_PIN,
    (!highTemperature && !highHumidity) ? HIGH : LOW
  );
}

void showSensorError() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(5, 24, "DHT11");
  u8g2.drawStr(5, 50, "ERROR");
  u8g2.sendBuffer();
}

void setup() {
  // 設定 OLED 位址：7-bit 0x3C 左移一位給 U8G2 使用。
  u8g2.setI2CAddress(0x3C << 1);
  u8g2.begin();
  dht.begin();

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
}

void loop() {
  // DHT11 建議至少間隔 2 秒讀取一次。
  delay(2000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    showSensorError();
    return;
  }

  updateLEDs(temperature, humidity);

  u8g2.clearBuffer();
  u8g2.drawVLine(63, 5, 54);

  // 左側顯示溫度與溫度計圖示。
  drawThermometerIcon(6, 14, temperature);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(27, 17, "TEMP");
  u8g2.setFont(u8g2_font_9x15_tf);
  u8g2.setCursor(27, 46);
  u8g2.print(temperature, 0);
  u8g2.print("C");

  // 右側顯示濕度與上移後的水滴圖示。
  drawDropletIcon(74, 5, humidity);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(96, 17, "HUMI");
  u8g2.setFont(u8g2_font_9x15_tf);
  u8g2.setCursor(82, 46);
  u8g2.print(humidity, 0);
  u8g2.print("%");

  u8g2.sendBuffer();
}
