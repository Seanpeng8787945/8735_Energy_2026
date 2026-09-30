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

// 繪製會依溫度變化的溫度計圖示。
// 以 0~50°C 作為顯示範圍，溫度越高，內部液柱越滿。
void drawThermometerIcon(int x, int y, float temperature) {
  u8g2.drawFrame(x + 5, y, 5, 21);
  u8g2.drawDisc(x + 7, y + 25, 6);
  u8g2.drawLine(x + 11, y + 4, x + 14, y + 4);
  u8g2.drawLine(x + 11, y + 10, x + 14, y + 10);
  u8g2.drawLine(x + 11, y + 16, x + 14, y + 16);

  // 溫度計底部球體固定填滿，內部液柱依溫度增加。
  int temperatureLevel = constrain((int)temperature, 0, 50);
  int fillHeight = map(temperatureLevel, 0, 50, 0, 18);
  if (fillHeight > 0) {
    u8g2.drawBox(x + 6, y + 26 - fillHeight, 3, fillHeight);
  }
}

// 繪製會依濕度變化的水滴圖示。
// 水滴內部以水平水位表示 0~100% 濕度。
void drawDropletIcon(int x, int y, float humidity) {
  u8g2.drawTriangle(x + 8, y, x + 1, y + 14, x + 8, y + 25);
  u8g2.drawTriangle(x + 8, y, x + 15, y + 14, x + 8, y + 25);
  u8g2.drawDisc(x + 8, y + 18, 7);

  // 依濕度繪製由下往上的水位，水位越高代表濕度越高。
  int humidityLevel = constrain((int)humidity, 0, 100);
  int waterHeight = map(humidityLevel, 0, 100, 0, 17);
  int centerX = x + 8;

  for (int row = 0; row < waterHeight; row++) {
    int rowY = y + 24 - row;
    int halfWidth = min(7, 2 + row / 3);
    u8g2.drawHLine(centerX - halfWidth, rowY, halfWidth * 2 + 1);
  }
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

  // 左側：溫度計圖示與稍微縮小的溫度數值
  drawThermometerIcon(6, 14, temperature);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(27, 17, "TEMP");
  u8g2.setFont(u8g2_font_9x15_tf);
  u8g2.setCursor(27, 46);
  u8g2.print(temperature, 0);
  u8g2.print("C");

  // 右側：水滴圖示上移，避免與下方濕度數值重疊
  drawDropletIcon(74, 5, humidity);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(96, 17, "HUMI");
  u8g2.setFont(u8g2_font_9x15_tf);
  u8g2.setCursor(82, 46);
  u8g2.print(humidity, 0);
  u8g2.print("%");

  // 將畫面更新到 OLED
  u8g2.sendBuffer();
}
