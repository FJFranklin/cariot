#include "Command.hh"
#include "Encoder.hh"

unsigned long previous_time = 0;

int E1_ChA = 4; // yellow
int E1_ChB = 3; // pink
int E1_ppr = 1024;

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  E1.init(E1_ChA, E1_ChB, E1_ppr, false); // set up encoder 1

  previous_time = millis();
}

void every_milli() { // runs once a millisecond, on average
  char code;
  unsigned long value;

  while (command.have(code, value)) {
    // process the command-value pair
  }
}

void every_10ms() { // runs once every 10ms, on average
  E1.sync();
}

void every_tenth(int tenth) { // runs once every tenth of a second, where tenth = 0..9
  digitalWrite(LED_BUILTIN, tenth == 0 || tenth == 8); // double blink per second

  float revs_per_sec = E1.latest();
  if (revs_per_sec) {
    Serial.println(revs_per_sec);
  }
}

void every_second() { // runs once every second
}

void loop() {
  // add code here that can't wait



  // our little internal real-time clock:
  static int count_ms = 0;
  static int count_10ms = 0;
  static int count_tenths = 0;

  unsigned long current_time = millis();

  if (current_time != previous_time) {
    ++previous_time;
    every_milli();

    if (++count_ms == 10) {
      count_ms = 0;
      every_10ms();

      if (++count_10ms == 10) {
        count_10ms = 0;
        every_tenth(count_tenths);

        if (++count_tenths == 10) {
          count_tenths = 0;
          every_second();
        }
      }
    }
  }
}
