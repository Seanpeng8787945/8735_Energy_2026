/*
  專案名稱：03_OLED
  功能：使用 I2C 介面在 0.96 吋 OLED 顯示器上顯示 Hello

  HUB-8735 Ultra 接線：
  OLED SDA  -> IO0
  OLED SCL  -> IO1
  OLED VCC  -> 3.3V
  OLED GND  -> GND

  本程式使用 SSD1306 常見的 128x64 OLED 位址 0x3C。
  如果螢幕沒有反應，可以將 OLED_ADDRESS 改成 0x3D 試試看。

  本程式直接透過 Wire1 函式庫控制 SSD1306，
  不需要額外安裝 Adafruit_GFX 或 Adafruit_SSD1306 函式庫。
*/

#include <Wire.h>

// OLED 的 I2C 位址；0.96 吋 SSD1306 模組最常見是 0x3C
const uint8_t OLED_ADDRESS = 0x3C;

// OLED 顯示器解析度
const uint8_t OLED_WIDTH = 128;
const uint8_t OLED_HEIGHT = 64;

// SSD1306 的命令模式控制位元
const uint8_t SSD1306_COMMAND = 0x00;
const uint8_t SSD1306_DATA = 0x40;

// 5x7 英文字母字型：只放本次測試需要的 H、e、l、o
const uint8_t FONT_H[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
const uint8_t FONT_e[5] = {0x38, 0x54, 0x54, 0x54, 0x18};
const uint8_t FONT_l[5] = {0x00, 0x41, 0x7F, 0x40, 0x00};
const uint8_t FONT_o[5] = {0x38, 0x44, 0x44, 0x44, 0x38};

// 傳送一個 SSD1306 命令
void sendCommand(uint8_t command) {
  Wire1.beginTransmission(OLED_ADDRESS);
  Wire1.write(SSD1306_COMMAND);
  Wire1.write(command);
  Wire1.endTransmission();
}

// 初始化 OLED 顯示器
void initializeOLED() {
  // 關閉顯示器，開始設定參數
  sendCommand(0xAE);

  // 設定時脈分頻
  sendCommand(0xD5);
  sendCommand(0x80);

  // 設定多工比例：64 行
  sendCommand(0xA8);
  sendCommand(0x3F);

  // 設定顯示偏移
  sendCommand(0xD3);
  sendCommand(0x00);

  // 設定起始顯示行
  sendCommand(0x40);

  // 設定電荷泵
  sendCommand(0x8D);
  sendCommand(0x14);

  // 設定記憶體位址模式為水平模式
  sendCommand(0x20);
  sendCommand(0x00);

  // 設定左右及上下顯示方向
  sendCommand(0xA1);
  sendCommand(0xC8);

  // 設定 COM 腳位硬體組態
  sendCommand(0xDA);
  sendCommand(0x12);

  // 設定對比度
  sendCommand(0x81);
  sendCommand(0x8F);

  // 設定預充電週期及 VCOMH 電壓
  sendCommand(0xD9);
  sendCommand(0xF1);
  sendCommand(0xDB);
  sendCommand(0x40);

  // 使用正常顯示模式，不反白
  sendCommand(0xA4);
  sendCommand(0xA6);

  // 清除畫面後開啟顯示器
  sendCommand(0xAF);
}

// 將整個 OLED 畫面清除為黑色
void clearScreen() {
  // 128x64 顯示器共有 8 個 page，每個 page 高度為 8 像素
  for (uint8_t page = 0; page < 8; page++) {
    sendCommand(0xB0 + page);  // 指定目前 page
    sendCommand(0x00);          // 設定低位欄位起始位置
    sendCommand(0x10);          // 設定高位欄位起始位置

    // 每次最多傳送 16 個資料位元組，避免超過 I2C 緩衝區
    for (uint8_t column = 0; column < OLED_WIDTH; column += 16) {
      Wire1.beginTransmission(OLED_ADDRESS);
      Wire1.write(SSD1306_DATA);

      for (uint8_t i = 0; i < 16; i++) {
        Wire1.write(0x00);
      }

      Wire1.endTransmission();
    }
  }
}

// 在指定 page、column 位置畫出一個 5x7 字元
void drawCharacter(const uint8_t fontData[5], uint8_t column, uint8_t page) {
  // 指定要寫入的 page 及欄位位置
  sendCommand(0xB0 + page);
  sendCommand(0x00 | (column & 0x0F));
  sendCommand(0x10 | ((column >> 4) & 0x0F));

  // 傳送字元的 5 個欄位資料
  Wire1.beginTransmission(OLED_ADDRESS);
  Wire1.write(SSD1306_DATA);

  for (uint8_t i = 0; i < 5; i++) {
    Wire1.write(fontData[i]);
  }

  // 字元右側留一個空白欄位，讓字元彼此分開
  Wire1.write(0x00);
  Wire1.endTransmission();
}

// 顯示 Hello
void displayHello() {
  // 將 Hello 放在畫面中央附近
  const uint8_t startColumn = 46;
  const uint8_t page = 3;

  drawCharacter(FONT_H, startColumn, page);
  drawCharacter(FONT_e, startColumn + 6, page);
  drawCharacter(FONT_l, startColumn + 12, page);
  drawCharacter(FONT_l, startColumn + 18, page);
  drawCharacter(FONT_o, startColumn + 24, page);
}

void setup() {
  // 使用老師指定的 I2C 腳位：
  // SDA = IO0，SCL/SCK = IO1
  // HUB-8735 Ultra 的 Wire1 介面對應到 IO0/IO1，
  // 因此使用 Wire1.begin() 啟動老師指定的 I2C 腳位。
  Wire1.begin();

  // 初始化 OLED、清除畫面並顯示 Hello
  initializeOLED();
  clearScreen();
  displayHello();
}

void loop() {
  // 保持 Hello 顯示，不需要重複更新畫面
}
