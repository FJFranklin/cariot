#include "Joy.hh"

Joy * J = 0;

unsigned long previousTime = 0;

void setup() {
  while (!Serial);
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  J = Joy::joy();

  previousTime = millis();
}

void loop() {
  static int tenth = 0;

  unsigned long currentTime = millis();

  if (currentTime - previousTime > 100) {
    previousTime += 100;

    if (++tenth == 10) {
      tenth = 0;
    }
    digitalWrite(LED_BUILTIN, tenth == 0 || tenth == 7); // double blink per second

    J->start(); // start communication sequence
  }

  if (J->tick()) { // returns true if communication sequence complete
    if (J->changes()) {
      Serial.print("x=");
      Serial.print(J->x());
      Serial.print("; y=");
      Serial.print(J->y());
      Serial.print("; b=");
      if (J->up())     Serial.print('u');
      if (J->down())   Serial.print('d');
      if (J->left())   Serial.print('l');
      if (J->right())  Serial.print('r');
      if (J->select()) Serial.print('s');
      Serial.println();
    }
  }
}
