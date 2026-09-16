const int GREEN_LED = 24;
const int YELLOW_LED = 23;
const int RED_LED = 22;

void allLightsOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void setup() {
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  allLightsOff();
}

void loop() {
  allLightsOff();
  digitalWrite(GREEN_LED, HIGH);
  delay(3000);

  allLightsOff();
  digitalWrite(YELLOW_LED, HIGH);
  delay(1000);

  allLightsOff();
  digitalWrite(RED_LED, HIGH);
  delay(5000);
}
