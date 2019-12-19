#include "Timer.hh"
#include "BTCommander.hh"
#include "SerialCommander.hh"

class Inky : public Timer, public Commander::Responder {
private:
  SerialCommander s0;
  BTCommander *bt;

public:
  Inky() :
    s0(Serial, '0', this),
    bt(BTCommander::commander(this)) // zero if no bluetooth connection is possible
  {
    // ...
  }
  virtual ~Inky() {
    // ...
  }

  virtual void notify(Commander * C, const char * str) {
    Serial.print(C->name());
    Serial.print(": notify: ");
    Serial.println(str);
  }
  virtual void command(Commander * C, char code, unsigned long value) {
    Serial.print(C->name());
    Serial.print(": command: ");
    Serial.print(code);
    Serial.print(": ");
    Serial.println(value);
  }

  virtual void every_milli() { // runs once a millisecond, on average
    s0.update();
  }

  virtual void every_tenth(int tenth) { // runs once every tenth of a second, where tenth = 0..9
    digitalWrite(LED_BUILTIN, tenth == 0 || tenth == 8); // double blink per second

    if (bt) {
      bt->update();
      if (tenth == 3) {
        bt->print("tick... ");
      }
    }
  }

  virtual void every_second() { // runs once every second
    if (bt) {
      bt->print("tock.", true);
    }
  }
};

void setup() {
  while (!Serial);
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  Inky().run();
}

void loop() {
  // ...
}
