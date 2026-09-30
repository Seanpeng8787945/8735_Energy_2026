# HUB-8735 Ultra 硬體測試紀錄

## DHT11 燒錄相容性測試

測試日期：2026-09-30

| DHT11 DATA 腳位 | 編譯 | UART 燒錄 | 結果 |
|---|---|---|---|
| IO16 | 成功 | 失敗 | 無法進入 UART 燒錄模式 |
| IO17 | 成功 | 失敗 | 無法進入 UART 燒錄模式 |
| IO20 | 成功 | 成功 | `upload success` |

結論：目前專案的 DHT11 DATA 腳位固定使用 IO20。後續若要使用 IO16 或 IO17，需另外確認板卡啟動/燒錄時序與腳位限制。

相關程式：`05_OLED_DHT11/05_OLED_DHT11.ino`

## HUB-8735 Ultra 腳位表摘要

腳位完整圖檔：`腳位表.png`

| IO | 腳位功能 |
|---:|---|
| 0 | ADC2、I2C1_SDA |
| 1 | ADC1、I2C1_SCL |
| 2 | ADC0 |
| 3 | SPI_SS、圖中標示 I2C_SDK |
| 4 | SPI_MOSI、I2C_SCL |
| 5 | SERIAL3_RX、SPI_MISO |
| 6 | SERIAL3_TX、SPI_SCLK |
| 7 | — |
| 8 | SERIAL2_TX |
| 9 | ADC6、SERIAL1_TX |
| 10 | ADC7、SERIAL1_RX |
| 11 | PWM |
| 12 | PWM、Button |
| 13 | PWM、Camera Flash LED |
| 14 | Flash Mode |
| 15 | SERIAL2_RX |
| 16 | LOG_RX |
| 17 | LOG_TX |
| 18 | PWM、SPI1_MISO |
| 19 | PWM、SPI1_SCLK |
| 20 | PWM、SPI1_MOSI |
| 21 | PWM、SPI1_SS |
| 22 | PWM |
| 23 | PWM |
| 24 | PWM |
| 25 | 板載綠色 LED（LED_G） |
| 26 | 板載藍色 LED（LED_B） |
| 27 | ADC4、I2C2_SCL |
| 28 | ADC5、I2C2_SDA |
| 29 | — |

備註：腳位表中的 `I2C_SDK` 依圖片原文保留；實際程式使用時，仍需以目前安裝的 HUB-8735 Ultra 核心腳位定義與實測結果為準。
