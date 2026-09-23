/*
  專案名稱：03-1_OLED
  功能：使用 U8G2 函式庫，在 SSD1306 128x64 OLED 顯示器上顯示 Hello !

  OLED 設定：
  - 控制器：SSD1306
  - 解析度：128 x 64
  - 介面：I2C
  - I2C 位址：0x3C
  - 使用第一組 I2C：Wire

  HUB-8735 Ultra 第一組 I2C 接線：
  - OLED SDA -> IO3
  - OLED SCL/SCK -> IO4
  - OLED VCC -> 3.3V
  - OLED GND -> GND

  注意：
  HUB-8735 Ultra 的 Wire 第一組 I2C 對應 IO3/IO4。
  如果 OLED 接在老師前一個指定的 IO0/IO1，請改用 Wire1
  版本，而不是本程式。
*/

#include <Wire.h>
#include <U8g2lib.h>

// SSD1306 128x64，硬體 I2C，使用 Wire 第一組 I2C。
// U8X8_PIN_NONE 表示 OLED 不使用 RESET 腳位。
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

void setup() {
  // 設定 OLED 的 I2C 位址。
  // U8G2 的 setI2CAddress() 需要 8-bit 位址，
  // 因此 7-bit 位址 0x3C 要左移一位。
  u8g2.setI2CAddress(0x3C << 1);

  // 初始化 U8G2；此建構子會使用第一組 I2C：Wire。
  u8g2.begin();

  // 設定字型。
  u8g2.setFont(u8g2_font_ncenB14_tr);

  // 清除內部畫面緩衝區，準備繪圖。
  u8g2.clearBuffer();

  // 顯示 Hello !
  // x=25、y=38，讓文字大致位於 128x64 螢幕中央。
  u8g2.drawStr(25, 38, "Hello !");

  // 將緩衝區內容實際送到 OLED 顯示。
  u8g2.sendBuffer();
}

void loop() {
  // 測試文字固定顯示，不需要重複更新。
}
