/*
  專案名稱：05_OLED_DHT11
  功能：讀取接在 IO20 的 DHT11，並將溫度與濕度顯示在 OLED 上

  DHT11 接線：
  - VCC  -> 3.3V
  - DATA -> IO20
  - GND  -> GND
  - 若是裸 DHT11 感測器，DATA 與 VCC 之間需要 10K 上拉電阻

  OLED 設定：
  - 控制器：SSD1306
  - 解析度：128 x 64
  - I2C 位址：0x3C
  - 使用第一組 I2C：Wire
  - SDA -> IO3
  - SCL/SCK -> IO4
  - VCC -> 3.3V
  - GND -> GND

  HUB-8735 Ultra 的 DHT11 官方範例使用 DHT.h，
  本程式沿用相同的函式庫與讀取方式。
*/

#include "DHT.h"
#include <Wire.h>
#include <U8g2lib.h>

// DHT11 的資料腳位與感測器類型
const int DHT_PIN = 20;  // IO20
#define DHT_TYPE DHT11

// 建立 DHT11 感測器物件
DHT dht(DHT_PIN, DHT_TYPE);

// SSD1306 128x64，硬體 I2C，使用第一組 I2C：Wire
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

void setup() {
  // 設定 OLED 的 7-bit I2C 位址 0x3C。
  // U8G2 需要 8-bit 位址，所以要左移一位。
  u8g2.setI2CAddress(0x3C << 1);

  // 初始化 OLED 與 DHT11
  u8g2.begin();
  dht.begin();

  // 設定 OLED 字型
  u8g2.setFont(u8g2_font_6x12_tf);
}

void showSensorError() {
  // 顯示 DHT11 讀取失敗訊息
  u8g2.clearBuffer();
  u8g2.drawStr(0, 20, "DHT11 Sensor");
  u8g2.drawStr(0, 42, "Read Error");
  u8g2.sendBuffer();
}

void loop() {
  // DHT11 建議至少間隔約 2 秒讀取一次
  delay(2000);

  // 讀取溫度（攝氏）與相對濕度
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // 檢查感測器是否讀取成功
  if (isnan(humidity) || isnan(temperature)) {
    showSensorError();
    return;
  }

  // 清除 OLED 畫面
  u8g2.clearBuffer();

  // 顯示溫度，例如：Temp : 25.0 C
  u8g2.drawStr(0, 24, "Temp :");
  u8g2.setCursor(48, 24);
  u8g2.print(temperature, 1);
  u8g2.print(" C");

  // 顯示濕度，例如：HUMI : 58%
  u8g2.drawStr(0, 48, "HUMI :");
  u8g2.setCursor(48, 48);
  u8g2.print(humidity, 0);
  u8g2.print("%");

  // 將畫面更新到 OLED
  u8g2.sendBuffer();
}
