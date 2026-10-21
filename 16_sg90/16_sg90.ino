/*
  16_sg90
  HUB-8735 Ultra + SG90 伺服馬達

  SG90 訊號線接 IO21。
  每秒依序移動：0 -> 90 -> 180 -> 0，
  回到 0 度後停留 5 秒，再重複循環。
*/

#include <AmebaServo.h>

const int SERVO_PIN = 21;  // HUB-8735 Ultra IO21 / PF8
const int SERVO_ANGLES[] = {0, 90, 180, 0};
const int SERVO_ANGLE_COUNT = sizeof(SERVO_ANGLES) / sizeof(SERVO_ANGLES[0]);

AmebaServo sg90;

void setup() {
  sg90.attach(SERVO_PIN);
  sg90.write(0);
}

void loop() {
  for (int i = 0; i < SERVO_ANGLE_COUNT; i++) {
    sg90.write(SERVO_ANGLES[i]);
    delay(1000);
  }

  delay(5000);
}
