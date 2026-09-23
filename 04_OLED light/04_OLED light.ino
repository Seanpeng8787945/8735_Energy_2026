/*
  專案名稱：04_OLED light
  功能：讀取 IO0 上的光敏電阻，將亮度百分比顯示在 OLED 上，
        並依照亮度百分比控制綠、黃、紅三個 LED

  OLED 設定：
  - 控制器：SSD1306
  - 解析度：128 x 64
  - I2C 位址：0x3C
  - 使用第一組 I2C：Wire

  HUB-8735 Ultra 接線：

  光敏電阻模組：
  - AO / AOUT -> IO0
  - VCC       -> 3.3V
  - GND       -> GND

  OLED：
  - SDA       -> IO3
  - SCL/SCK   -> IO4
  - VCC       -> 3.3V
  - GND       -> GND

  LED：
  - 綠燈      -> IO24
  - 黃燈      -> IO23
  - 紅燈      -> IO22

  注意：
  1. 光敏電阻必須形成分壓電路，才能由 IO0 讀取類比電壓。
  2. IO0 是類比輸入腳位，不是 OLED 的 I2C 腳位。
  3. 目前校正範圍為 ADC 0~1024。
  4. 目前光敏電阻讀值方向為：0 = 最亮，1024 = 最暗。
  5. 顯示結果會換算為：100% = 最亮，0% = 最暗。
  6. 亮度 0%~33%：只亮綠燈。
  7. 亮度 34%~66%：綠燈與黃燈都亮。
  8. 亮度 67%~100%：綠、黃、紅三燈都亮。
*/

#include <Wire.h>
#include <U8g2lib.h>

// 光敏電阻的類比輸入腳位
const int LIGHT_SENSOR_PIN = 0;  // IO0

// 三色 LED 的輸出腳位
const int GREEN_LED_PIN = 24;   // 綠燈
const int YELLOW_LED_PIN = 23;  // 黃燈
const int RED_LED_PIN = 22;     // 紅燈

// SSD1306 128x64，硬體 I2C，使用第一組 I2C：Wire。
// U8X8_PIN_NONE 表示 OLED 沒有使用 RESET 腳位。
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

void setup() {
  // 設定 OLED 的 I2C 位址。
  // U8G2 的 setI2CAddress() 使用 8-bit 位址，
  // 因此 7-bit 位址 0x3C 需要左移一位。
  u8g2.setI2CAddress(0x3C << 1);

  // 初始化 OLED，使用第一組 I2C：Wire。
  u8g2.begin();

  // 設定光敏電阻腳位為類比輸入。
  pinMode(LIGHT_SENSOR_PIN, INPUT);

  // 設定三色 LED 為輸出腳位，並先全部關閉。
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  // 設定顯示字型。
  u8g2.setFont(u8g2_font_6x12_tf);
}

void loop() {
  // 讀取 IO0 的 ADC 原始數值。
  int lightRawValue = analogRead(LIGHT_SENSOR_PIN);

  // 將 ADC 原始值反向換算為亮度百分比：
  // ADC 0    -> 100%（最亮）
  // ADC 1024 -> 0%（最暗）
  int brightnessPercent = map(lightRawValue, 0, 1024, 100, 0);

  // 避免實際讀值略微超出設定範圍，造成百分比超過 0~100%。
  brightnessPercent = constrain(brightnessPercent, 0, 100);

  // 依照亮度百分比控制 LED。
  // 0%~33%：只亮綠燈
  // 34%~66%：亮綠燈與黃燈
  // 67%~100%：綠、黃、紅三燈都亮
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(YELLOW_LED_PIN, brightnessPercent >= 34 ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, brightnessPercent >= 67 ? HIGH : LOW);

  // 清除畫面緩衝區。
  u8g2.clearBuffer();

  // 顯示標題。
  u8g2.drawStr(0, 14, "Light Sensor");

  // 顯示亮度百分比。
  u8g2.drawStr(0, 32, "Brightness:");
  u8g2.setCursor(72, 32);
  u8g2.print(brightnessPercent);
  u8g2.print("%");

  // 顯示目前的 ADC 原始值，方便日後重新校正。
  u8g2.drawStr(0, 52, "Raw:");
  u8g2.setCursor(32, 52);
  u8g2.print(lightRawValue);

  // 將畫面緩衝區送到 OLED。
  u8g2.sendBuffer();

  // 每 200 毫秒更新一次，避免畫面更新過快。
  delay(200);
}
