/*
  專案名稱：08_beep
  功能：使用接在 IO18 的無源蜂鳴器，模擬消防車警笛音效

  接線：
  - 無源蜂鳴器正極 -> IO11
  - 無源蜂鳴器負極 -> GND

  IO11 支援 PWM，適合使用 tone() 改變蜂鳴器音調。
  本程式以低頻到高頻，再由高頻回到低頻的方式形成警笛聲。
*/

const int BUZZER_PIN = 11;

// 播放一次高低交替的消防車警笛音效。
// 使用固定音調持續播放，讓無源蜂鳴器得到穩定的 PWM 訊號。
void playFireTruckSiren() {
  tone(BUZZER_PIN, 650);
  delay(500);

  tone(BUZZER_PIN, 1250);
  delay(500);
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // 持續播放消防車警笛音效。
  playFireTruckSiren();
}
