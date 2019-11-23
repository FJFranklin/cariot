#include "Encoder.hh"

static void s_enc_1_ch_A();
static void s_enc_1_ch_B();
static void s_enc_2_ch_A();
static void s_enc_2_ch_B();

static float s_enc_1_sync(unsigned long & last_elapsed, unsigned long & last_count);
static float s_enc_2_sync(unsigned long & last_elapsed, unsigned long & last_count);

Encoder E1(s_enc_1_ch_A, s_enc_1_ch_B, s_enc_1_sync);
Encoder E2(s_enc_2_ch_A, s_enc_2_ch_B, s_enc_2_sync);

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

volatile static unsigned long s_enc_1_count = 0;
volatile static unsigned long s_enc_1_delta = 0;
volatile static float         s_enc_1_sense = 1;

static void s_enc_1_ch_A() {
  s_enc_1_delta = E1.timer;
  E1.timer = 0;

  s_enc_1_sense = ((digitalRead(E1.ch_A_pin) == digitalRead(E1.ch_B_pin)) ? 1 : -1);

  ++s_enc_1_count;
}

static void s_enc_1_ch_B() {
  s_enc_1_delta = E1.timer;
  E1.timer = 0;

  s_enc_1_sense = ((digitalRead(E1.ch_A_pin) != digitalRead(E1.ch_B_pin)) ? 1 : -1);

  ++s_enc_1_count;
}

static float s_enc_1_sync(unsigned long & last_delta, unsigned long & last_count) {
  last_count = s_enc_1_count;
  last_delta = s_enc_1_delta;

  s_enc_1_count = 0;

  return s_enc_1_sense;
}

volatile static unsigned long s_enc_2_count = 0;
volatile static unsigned long s_enc_2_delta = 0;
volatile static float         s_enc_2_sense = 1;

static void s_enc_2_ch_A() {
  s_enc_2_delta = E2.timer;
  E2.timer = 0;

  s_enc_2_sense = ((digitalRead(E2.ch_A_pin) == digitalRead(E2.ch_B_pin)) ? 1 : -1);

  ++s_enc_2_count;
}

static void s_enc_2_ch_B() {
  s_enc_2_delta = E2.timer;
  E2.timer = 0;

  s_enc_2_sense = ((digitalRead(E2.ch_A_pin) != digitalRead(E2.ch_B_pin)) ? 1 : -1);

  ++s_enc_2_count;
}

static float s_enc_2_sync(unsigned long & last_delta, unsigned long & last_count) {
  last_count = s_enc_2_count;
  last_delta = s_enc_2_delta;

  s_enc_2_count = 0;

  return s_enc_2_sense;
}
