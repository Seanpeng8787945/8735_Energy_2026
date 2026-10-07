# 8735 智慧節能專案｜AI 交接紀錄

更新日期：2026-10-07（Asia/Taipei）

## 1. 專案位置與 GitHub

- 本機專案目錄：`D:\智慧節能專案資料夾\115智慧節能`
- GitHub：`https://github.com/Seanpeng8787945/8735_Energy_2026.git`
- 目前分支：`main`
- 目前遠端同步狀態：`main` 與 `origin/main` 同步
- 編譯產生的 `**/build/` 已由 `.gitignore` 排除，不應提交到 GitHub。

## 2. Arduino CLI 與 HUB-8735 工具路徑

### Arduino CLI

```text
C:\Users\彭暐翔\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe
```

### 已安裝核心

```text
ideasHatch:AmebaPro2 4.0.15-Release
```

### 板卡 FQBN

```text
ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra
```

### 專用燒錄工具

```text
C:\Users\彭暐翔\AppData\Local\Arduino15\packages\ideasHatch\tools\ameba_pro2_tools\1.4.1\image_windows.exe
```

工具資料夾：

```text
C:\Users\彭暐翔\AppData\Local\Arduino15\packages\ideasHatch\tools\ameba_pro2_tools\1.4.1
```

## 3. 目前硬體與連接埠

- 開發板：HUB-8735 Ultra
- USB 裝置：USB-SERIAL CH340
- 目前連接埠：`COM5`
- DHT11 DATA：`IO20`
- OLED：SSD1306 128×64，I2C 位址 `0x3C`
- OLED 第一組 I2C：SDA=`IO3`、SCL/SCK=`IO4`
- 三色 LED：綠=`IO24`、黃=`IO23`、紅=`IO22`
- 無源蜂鳴器目前使用：`IO11`

## 4. 最新已燒錄程式

最新整合程式為：

```text
12_ThingSpeak\12_ThingSpeak.ino
```

版本：`v1.0.0`

上一版整合程式仍保留於：

```text
09_OLED_DHT_led_beep\09_OLED_DHT_led_beep.ino
```

`12_ThingSpeak` 功能：

- 連線 Wi-Fi 後顯示 `wifi connecting...`、`wifi connected`。
- 每 1 分鐘向環境部 AQX_P_432 API 取得資料。
- 使用 ArduinoJson 篩選桃園市中壢測站，顯示 AQI 與 PM2.5。
- 每次取得資料時顯示 `data updating...`。
- 保留 DHT11、雙頁 OLED、三色 LED 與蜂鳴器警報功能。
- 每 30 秒將溫度、濕度、AQI、PM2.5 上傳至 ThingSpeak 的 `field1`～`field4`。
- 上傳時顯示 `data uploading...`。
- 已完成 HUB-8735 Ultra 編譯、燒錄，結果為 `upload success`。

v0.9.0 修正：環境部 AQI API 使用 HTTP chunked response，且回應最外層為 JSON 陣列；`10_OLED_AQI_PM` 已依驗證專案的處理方式更新，並已成功燒錄。

`09_OLED_DHT_led_beep` 功能：

- 溫度與濕度顯示於 OLED。
- 溫度 `> 25°C`：紅燈亮，模擬冷氣，啟動蜂鳴器。
- 濕度 `> 80%`：黃燈亮，模擬除濕機，啟動蜂鳴器。
- 溫度與濕度皆正常：綠燈亮，蜂鳴器關閉。
- 溫度與濕度同時超標：紅、黃燈同時亮，蜂鳴器高低頻交替警報。
- 警報音使用 IO11，650 Hz 與 1250 Hz 每 500 ms 交替。

最近一次成功燒錄輸出：

```text
upload success
```

## 5. 編譯指令

在 PowerShell 中執行。設定 `LC_ALL` 與 `LANG` 是必要的，否則 Arduino CLI 可能出現 `locale::facet::_S_create_c_locale name not valid`。

```powershell
$env:LC_ALL = 'C'
$env:LANG = 'C'
$cli = 'C:\Users\彭暐翔\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
$sketch = 'D:\智慧節能專案資料夾\115智慧節能\09_OLED_DHT_led_beep'
$build = 'D:\智慧節能專案資料夾\115智慧節能\09_OLED_DHT_led_beep\build'

& $cli compile `
  --fqbn 'ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra' `
  --build-path $build `
  $sketch
```

成功時應產生：

```text
$build\application.ntz
$build\flash_ntz.bin
```

## 6. HUB-8735 專用燒錄指令

此核心的通用 `arduino-cli upload` 對 `application.ntz`/`flash_ntz.bin` 支援不完整，請使用以下專用工具。`Enable` 自動模式曾成功，`Disable` 曾發生 `Uart boot fail`。

```powershell
$env:LC_ALL = 'C'
$env:LANG = 'C'
$toolDir = 'C:\Users\彭暐翔\AppData\Local\Arduino15\packages\ideasHatch\tools\ameba_pro2_tools\1.4.1'

& (Join-Path $toolDir 'image_windows.exe') `
  $toolDir `
  'COM5' `
  'AMEBA' `
  'Enable' `
  'Disable' `
  '2000000' `
  'uartfwburn.exe' `
  'Auto_Flash_Pro2_V3.3_win.exe' `
  '0x60000' `
  '0x460000' `
  '0x530000'
```

成功訊息：

```text
upload success
```

## 7. 燒錄失敗處理

若出現以下訊息：

```text
ping fail
NOR flashloader loading fail
Uart boot fail
```

依序處理：

1. 關閉 Arduino IDE 的 Serial Monitor 與其他可能占用 COM5 的程式。
2. 確認 Windows 裝置管理員仍顯示 `USB-SERIAL CH340 (COM5)`。
3. USB 拔除後重新插入。
4. RESET 不要持續按住；必要時在燒錄工具開始後按一下再放開。
5. DHT11 避免接 IO16 或 IO17；這兩腳是 `LOG_RX`/`LOG_TX`，實測會造成無法進入燒錄模式。DHT11 使用 IO20 可成功燒錄。

## 8. 重要腳位摘要

- IO0：ADC2、I2C1_SDA
- IO1：ADC1、I2C1_SCL
- IO3：第一組 I2C SDA（依目前核心/實測使用）
- IO4：第一組 I2C SCL
- IO11：PWM，現用於無源蜂鳴器
- IO16：LOG_RX，避免接 DHT11
- IO17：LOG_TX，避免接 DHT11
- IO18/19/20/21：PWM；IO20 現用於 DHT11
- IO22/23/24：PWM，現用於紅/黃/綠 LED
- IO25：板載綠色 LED
- IO26：板載藍色 LED
- IO27：ADC4、I2C2_SCL
- IO28：ADC5、I2C2_SDA

完整圖片：`腳位表.png`

## 9. 程式與文件版本

- `01_led`：IO26 藍色 LED 閃爍測試
- `02_RGled`：IO24/23/22 紅綠燈循環
- `03_OLED`：SSD1306 基礎 Hello 測試
- `03-1_OLED`：U8G2 SSD1306 Hello 測試
- `04_OLED light`：光敏電阻 IO0 與 OLED 顯示、三色 LED 亮度控制
- `05_OLED_DHT11`：DHT11 IO20 與 OLED 溫濕度顯示
- `06_OLED_DHT_Icon`：溫度計/水滴動態圖示
- `07_OLED_DHT_led`：溫濕度門檻控制三色 LED
- `08_beep`：IO11 無源蜂鳴器消防車警笛
- `09_OLED_DHT_led_beep`：目前最新整合版本

成果資料：

```text
成果照片\腳位表.png
成果照片\06成果.jpg
成果照片\06成果影片.mp4
成果照片\07成果影片.mp4
成果照片\09成果影片.mp4
```

## 10. 後續 AI 操作原則

1. 先讀取本文件、`CHANGELOG.md`、`HARDWARE_TEST_LOG.md`。
2. 修改程式後使用新的版本號記錄變更。
3. 編譯時使用上述 Arduino CLI 完整路徑與 `--build-path`。
4. 燒錄時使用 `image_windows.exe` 專用指令，不使用一般 upload 流程。
5. 確認 `upload success` 後再向使用者回報。
6. 程式與文件變更完成後提交並推送到 GitHub `main`。
