#define BuildEncoder(EX) \
  static void EX##_ch_A(); \
  static void EX##_ch_B(); \
  \
  static float EX##_sync(unsigned long & last_elapsed, unsigned long & last_count); \
  \
  Encoder EX(EX##_ch_A, EX##_ch_B, EX##_sync); \
  \
  volatile static unsigned long EX##_count = 0; \
  volatile static unsigned long EX##_delta = 0; \
  volatile static float         EX##_sense = 1; \
  \
  static void EX##_ch_A() { \
    EX##_delta = EX.timer; \
    EX.timer = 0; \
  \
    EX##_sense = ((digitalRead(EX.ch_A_pin) == digitalRead(EX.ch_B_pin)) ? 1 : -1); \
  \
    ++EX##_count; \
  } \
  \
  static void EX##_ch_B() { \
    EX##_delta = EX.timer; \
    EX.timer = 0; \
  \
    EX##_sense = ((digitalRead(EX.ch_A_pin) != digitalRead(EX.ch_B_pin)) ? 1 : -1); \
  \
    ++EX##_count; \
  } \
  \
  static float EX##_sync(unsigned long & last_delta, unsigned long & last_count) { \
    last_count = EX##_count; \
    last_delta = EX##_delta; \
  \
    EX##_count = 0; \
  \
    return EX##_sense; \
  }

#include "Encoder.hh"

BuildEncoder(E1)
BuildEncoder(E2)
BuildEncoder(E3)
BuildEncoder(E4)

void Encoder::init(int pin_A, int pin_B, unsigned ppr, bool clockwise) {
  cpr = 4 * (float) ppr;

  sense = clockwise ? 1 : -1;

  ch_A_pin = pin_A;
  ch_B_pin = pin_B;

  pinMode(ch_A_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ch_A_pin), ch_A_interrupt, CHANGE);

  pinMode(ch_B_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ch_B_pin), ch_B_interrupt, CHANGE);

  rev_s = 0;
  dsync = 0;
  timer = 0;
}

void Encoder::sync() {
  unsigned long dt = dsync; // how long since last sync()
  dsync = 0;

  unsigned long count;
  unsigned long delta;
  rev_s = sense * synchronise(delta, count);

  if (count < 2) {
    rev_s = 0; // call it: stopped
  } else {
    if (count > delta) {
      rev_s *= 1E6 * ((float) count) / (cpr * (float) dt);
    } else {
      rev_s *= 1E6 / (cpr * (float) delta);
    }
  }
}
