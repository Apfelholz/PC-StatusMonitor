#include <Arduino.h>

const int BOOL_LED_PIN = 6;

void setup() {
  pinMode(BOOL_LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  digitalWrite(BOOL_LED_PIN, HIGH);
  Serial.println("LED an");
  delay(1000);
  digitalWrite(BOOL_LED_PIN, LOW);
  Serial.println("LED aus");
  delay(1000);
}