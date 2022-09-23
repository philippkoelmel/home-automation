#define VBATPIN A7

void setup() {
  pinMode(VBATPIN, INPUT);
}

void loop() {
  delay(1000);
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  Serial.print("VBat: " ); Serial.println(measuredvbat * 1000.0);
}
