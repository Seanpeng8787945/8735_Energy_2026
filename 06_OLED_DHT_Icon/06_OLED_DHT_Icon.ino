/*
  專案名稱：06_OLED_DHT_Icon
  功能：讀取 DHT11 溫度與濕度，並以圖示及放大文字顯示在 OLED 上

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

  HUB-8735 Ultra 注意：
  DHT11 使用 IO20；IO16 與 IO17 為 LOG_RX/LOG_TX，避免作為 DHT11 資料腳位。
*/

#include "DHT.h"
#include <Wire.h>
#include <U8g2lib.h>

// DHT11 資料腳位與感測器類型
const int DHT_PIN = 20;
#define DHT_TYPE DHT11

// 建立 DHT11 感測器物件
DHT dht(DHT_PIN, DHT_TYPE);

// SSD1306 128x64，硬體 I2C，使用第一組 I2C：Wire
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// 繪製簡單溫度計圖示，位置固定在左側欄位
void drawThermometerIcon(int x, int y) {
  u8g2.drawFrame(x + 5, y, 6, 25);
  u8g2.drawDisc(x + 8, y + 29, 7);
  u8g2.drawBox(x + 7, y + 10, 3, 19);
  u8g2.drawLine(x + 12, y + 5, x + 16, y + 5);
  u8g2.drawLine(x + 12, y + 12, x + 16, y + 12);
  u8g2.drawLine(x + 12, y + 19, x + 16, y + 19);
}

// 繪製簡單水滴圖示，位置固定在右側欄位
void drawDropletIcon(int x, int y) {
  u8g2.drawTriangle(x + 10, y, x + 1, y + 17, x + 10, y + 30);
  u8g2.drawTriangle(x + 10, y, x + 19, y + 17, x + 10, y + 30);
  u8g2.drawDisc(x + 10, y + 21, 9);
  u8g2.setColorIndex(0);
  u8g2.drawDisc(x + 7, y + 20, 2);
  u8g2.setColorIndex(1);
}

void showSensorError() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(5, 24, "DHT11");
  u8g2.drawStr(5, 50, "ERROR");
  u8g2.sendBuffer();
}

void setup() {
  // 設定 OLED 的 7-bit I2C 位址 0x3C。
  // U8G2 需要 8-bit 位址，因此左移一位。
  u8g2.setI2CAddress(0x3C << 1);

  // 初始化 OLED 與 DHT11
  u8g2.begin();
  dht.begin();
}

void loop() {
  // DHT11 建議至少間隔約 2 秒讀取一次
  delay(2000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // 感測器讀取失敗時顯示錯誤
  if (isnan(humidity) || isnan(temperature)) {
    showSensorError();
    return;
  }

  u8g2.clearBuffer();

  // 中央垂直分隔線，將畫面分成左右兩區
  u8g2.drawVLine(63, 5, 54);

  // 左側：溫度計圖示與放大溫度數值
  drawThermometerIcon(5, 12);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(27, 17, "TEMP");
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.setCursor(24, 48);
  u8g2.print(temperature, 0);
  u8g2.print("C");

  // 右側：水滴圖示與放大濕度數值
  drawDropletIcon(72, 10);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(96, 17, "HUMI");
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.setCursor(80, 48);
  u8g2.print(humidity, 0);
  u8g2.print("%");

  // 將畫面更新到 OLED
  u8g2.sendBuffer();
}
