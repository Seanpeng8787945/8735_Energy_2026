/*
  專案名稱：08_beep
  功能：使用接在 IO18 的無源蜂鳴器，模擬消防車警笛音效

  接線：
  - 無源蜂鳴器正極 -> IO18
  - 無源蜂鳴器負極 -> GND

  IO18 支援 PWM，適合使用 tone() 改變蜂鳴器音調。
  本程式以低頻到高頻，再由高頻回到低頻的方式形成警笛聲。
*/

const int BUZZER_PIN = 18;

// 播放一次由低到高、再由高到低的消防車警笛掃頻。
void playFireTruckSiren() {
  const int lowFrequency = 650;
  const int highFrequency = 1250;
  const int frequencyStep = 25;
  const int stepDuration = 25;

  // 音調由低升高。
  for (int frequency = lowFrequency;
       frequency <= highFrequency;
       frequency += frequencyStep) {
    tone(BUZZER_PIN, frequency, stepDuration);
    delay(stepDuration);
  }

  // 音調由高降低。
  for (int frequency = highFrequency;
       frequency >= lowFrequency;
       frequency -= frequencyStep) {
    tone(BUZZER_PIN, frequency, stepDuration);
    delay(stepDuration);
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // 持續播放消防車警笛音效。
  playFireTruckSiren();
}
