# 8735 智慧節能專案｜AI 交接紀錄

更新日期：2026-10-21（Asia/Taipei）

## 1. 專案與 GitHub

- 本機目錄：`D:\智慧節能專案資料夾\115智慧節能`
- GitHub：`https://github.com/Seanpeng8787945/8735_Energy_2026.git`
- 分支：`main`
- 遠端：`origin`，目前 `main` 與 `origin/main` 同步。
- 最新提交：`fdcba25 Add MQTT light data result photo`
- 最新已燒錄程式：`13-2_MQTT_light_add\13-2_MQTT_light_add.ino`
- 目前工作區有多個未追蹤的舊編譯資料夾與備份草稿，例如 `10_OLED_AQI_PM\build*`、`10_OLED_AQI_PM2.5`、`10_OLED_AQI_PM2_5`；不要自行加入提交，也不要刪除使用者檔案。

## 2. Arduino CLI 與燒錄工具（重要）

### Arduino CLI 完整路徑

```text
C:\Users\彭暐翔\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe
```

### 已安裝核心與板卡 FQBN

```text
核心：ideasHatch:AmebaPro2 4.0.15-Release
FQBN：ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra
```

### HUB-8735 專用燒錄工具

```text
工具：C:\Users\彭暐翔\AppData\Local\Arduino15\packages\ideasHatch\tools\ameba_pro2_tools\1.4.1\image_windows.exe
工具資料夾：C:\Users\彭暐翔\AppData\Local\Arduino15\packages\ideasHatch\tools\ameba_pro2_tools\1.4.1
```

不要使用一般 `arduino-cli upload` 燒錄 `application.ntz` 或 `flash_ntz.bin`；此板卡使用下方專用命令。

## 3. 編譯標準流程

PowerShell 必須先設定 `LC_ALL` 與 `LANG`，否則 Arduino CLI 可能出現 locale 錯誤。

```powershell
$env:LC_ALL = 'C'
$env:LANG = 'C'
$cli = 'C:\Users\彭暐翔\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
$sketch = 'D:\智慧節能專案資料夾\115智慧節能\13-2_MQTT_light_add'
$build = 'D:\智慧節能專案資料夾\115智慧節能\13-2_MQTT_light_add\build'

& $cli compile `
  --clean `
  --fqbn 'ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra' `
  --build-path $build `
  $sketch
```

成功後應存在：

```text
$build\application.ntz
$build\flash_ntz.bin
```

替換 `$sketch` 與 `$build` 即可編譯其他版本，例如 `13_MQTT`。

## 4. 專用燒錄流程

目前 USB-SERIAL CH340 使用 `COM5`。燒錄前關閉 Arduino IDE Serial Monitor 與其他占用 COM5 的程式。

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

只有看到以下訊息才算成功：

```text
upload success
```

## 5. 燒錄失敗處理

若出現 `ping fail`、`NOR flashloader loading fail` 或 `Uart boot fail`：

1. 關閉 Serial Monitor。
2. 確認裝置管理員仍是 `USB-SERIAL CH340 (COM5)`。
3. 拔除 USB 後重新插入。
4. 燒錄工具開始後按一下 RESET 再放開，不要長按 RESET。
5. 必要時重新執行專用燒錄命令。
6. DHT11 DATA 固定接 IO20；不要接 IO16/IO17，這兩腳是 LOG_RX/LOG_TX，會影響 UART 燒錄。

## 6. 硬體接線

- HUB-8735 Ultra：USB-SERIAL CH340，通常為 `COM5`
- DHT11 DATA：`IO20`
- 光敏電阻 AO/AOUT：`IO0`
- OLED SSD1306 128×64：I2C 位址 `0x3C`
- OLED SDA：`IO3`
- OLED SCL/SCK：`IO4`
- 綠色 LED：`IO24`
- 黃色 LED：`IO23`
- 紅色 LED：`IO22`
- 無源蜂鳴器：`IO11`

光敏電阻的亮度換算沿用 `04_OLED light`：ADC `0` 對應 100%，ADC `1024` 對應 0%，最後限制在 0～100%。

## 7. 最新程式功能

### `13_MQTT`

檔案：`13_MQTT\13_MQTT.ino`

- 保留 DHT11、AQI/PM2.5、雙頁 OLED、三色 LED、蜂鳴器警報與 ThingSpeak。
- MQTT Broker：`mqttgo.io:1883`
- MQTT Topic：`Seanpeng/class105/data`
- MQTT payload：`{"temp":25,"humi":72}`
- 每 15 秒傳送一次。
- OLED 顯示 `wifi connecting...`、`wifi connected`、`mqtt connecting...`、`mqtt connected`、`data transmitting...`。
- 使用內建 `PubSubClient`，並呼叫 `mqttNetworkClient.setNonBlockingMode()`。

### `13-2_MQTT_light_add`（目前最新已燒錄）

檔案：`13-2_MQTT_light_add\13-2_MQTT_light_add.ino`

- 以 `13_MQTT` 為基礎增加 IO0 光敏電阻亮度。
- 第二頁 OLED 顯示溫度、濕度、亮度百分比。
- MQTT payload：`{"temp":25,"humi":72,"light":92}`
- 每 15 秒傳送一次。
- 保留 AQI/PM2.5、ThingSpeak、LED 與蜂鳴器功能。
- 最近一次燒錄結果：`upload success`。

### AQI API

- API Host：`data.moenv.gov.tw`
- API：`/api/v2/AQX_P_432?api_key=6b143ef9-f251-43e4-add6-a5557c7ffa1c`
- 每 60 秒更新。
- 篩選 `county == 桃園市`、`sitename == 中壢`。
- API 回應是最外層 JSON 陣列，不是 `records` 物件。
- API 使用 HTTP chunked response；程式必須先移除 chunk size 再交給 ArduinoJson 解析。

### ThingSpeak

- Host：`api.thingspeak.com`
- API key：`D6H5XMAOAOQRGWX0`
- 每 30 秒上傳。
- `field1` 溫度、`field2` 濕度、`field3` AQI、`field4` PM2.5。
- 上傳時 OLED 顯示 `data uploading...`。

## 8. MQTT 函式庫與官方範例

本機核心已內建：

```text
C:\Users\彭暐翔\AppData\Local\Arduino15\packages\ideasHatch\hardware\AmebaPro2\4.0.15-Release\libraries\MQTTClient
```

主要函式庫：`PubSubClient.h`。內建範例資料夾：

```text
MQTT_Basic
MQTT_Auth
MQTT_TLS
MQTT_Publish_In_Callback
MQTT_Image
MQTT_GenAIVision
```

非阻塞 socket 的使用方式：

```cpp
wifiClient.setNonBlockingMode();
```

本機沒有獨立命名為 `Non-Blocking Mode` 的範例；`MQTT_Basic` 與 `MQTT_Auth` 內有上述呼叫。

## 9. 程式版本索引

- `01_led`：IO26 藍色 LED
- `02_RGled`：IO24/23/22 三色 LED
- `03_OLED`、`03-1_OLED`：OLED 測試
- `04_OLED light`：IO0 光敏電阻與亮度
- `05_OLED_DHT11`：DHT11 溫濕度
- `06_OLED_DHT_Icon`：溫濕度圖示
- `07_OLED_DHT_led`：溫濕度門檻 LED
- `08_beep`：無源蜂鳴器
- `09_OLED_DHT_led_beep`：DHT、OLED、LED、蜂鳴器整合
- `10_OLED_AQI_PM`：Wi-Fi AQI/PM2.5
- `11_wifi_oled_aqi_dht`：AQI、PM2.5 與 DHT 雙頁整合
- `12_ThingSpeak`：ThingSpeak 上傳
- `13_MQTT`：MQTT 溫濕度上傳
- `13-2_MQTT_light_add`：MQTT 溫濕度與亮度上傳，目前最新

## 10. 成果資料

```text
成果照片\腳位表.png
成果照片\06成果.jpg
成果照片\06成果影片.mp4
成果照片\07成果影片.mp4
成果照片\09成果影片.mp4
成果照片\10成果照片.jpg
成果照片\11成果影片.mp4
成果照片\12成果ThingSpeak數據上傳.png
成果照片\13成果照片.jpg
```

## 11. Git 操作原則

1. 修改程式後更新版本紀錄。
2. 編譯一定使用本文件第 2 節的 Arduino CLI 完整路徑。
3. 燒錄一定使用 `image_windows.exe` 專用命令。
4. 只有看到 `upload success` 才回報燒錄成功。
5. Git 提交只加入明確要同步的程式、文件與成果檔案；不要把大型 `build` 資料夾或未確認的備份草稿加入。
6. 推送前確認 `git status` 與 `git log`，推送目標為 GitHub `origin/main`。
