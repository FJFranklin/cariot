/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_encoders_hh
#define cariot_encoders_hh

#ifndef ENABLE_ENC_CLASS

/* Rotary Encoder Setup:
          Yellow > 5V
          Red    > GND
          Brown  > 5
          Orange > 6
          Black is the index pulse and is not used in this code
 */
// Rotary encoder pulses per rotation (change as required, this is in the spec of the encoder)
#define ENC_COUNT_REV 1024 // 256 (Low-Res Encoder) or 1024 (High-Res Encoder)

// Encoder output to Arduino Interrupt pin
#define ENC_A 14  //Channel A (Brown)  set to pin 5 (Feather) or 14 (Teensy)
#define ENC_B 15  //Channel B (Orange) set to pin 6 (Feather) or 15 (Teensy)

volatile long encoderValue = 0; // Pulse count from encoder
volatile bool encoderForwards = true;

volatile int A_current = 0;
volatile int B_current = 0;

int A_previous;
int B_previous;

static void s_encoder_interrupt() {
  // Increment value for each pulse from encoder
  encoderValue++;

  A_current = digitalRead(ENC_A);
  B_current = digitalRead(ENC_B);

  encoderForwards = (A_current == B_current);
}

static void s_encoder_init() {
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), s_encoder_interrupt, RISING);
  
  A_previous = digitalRead(ENC_A);
  B_previous = digitalRead(ENC_B);
}

static float s_encoder_rpm() { // Calculate RPM
  static unsigned long time_last = 0;
  unsigned long time_this = micros();
  unsigned long interval;

  if (time_this < time_last) { // overflow
    interval = time_this + (0xFFFFFFFF - time_last) + 1;
  } else {
    interval = time_this - time_last;
  }
  time_last = time_this;

  float revs = ((float) encoderValue) / ENC_COUNT_REV;
  encoderValue = 0;

  float rpm = 0;
  if (interval != 0) { // otherwise too short a time interval!
    rpm = revs * 60E6 / interval;
  }
  return rpm;
}

#else // ENABLE_ENC_CLASS

#define E1_ChA 14 // yellow is /A
#define E1_ChB 15 // pink   is /B
#define E1_ppr 1024
#define E2_ChA 16 // yellow is /A
#define E2_ChB 17 // pink   is /B
#define E2_ppr 1024
#define E3_ChA 18 // yellow is /A
#define E3_ChB 19 // pink   is /B
#define E3_ppr 1024
#define E4_ChA 20 // yellow is /A
#define E4_ChB 21 // pink   is /B
#define E4_ppr 1024

class Encoder {
private:
  void (*ch_A_interrupt)();
  void (*ch_B_interrupt)();

  float (*synchronise)(unsigned long & last_elapsed, unsigned long & last_count); // returns latest elapsed time and pulse count

  float cpr;   // counts per revolution = 4 * ppr
  float sense; // 1 for clockwise-is-positive; -1 otherwise
  float rev_s;

  elapsedMicros dsync;
public:
  elapsedMicros timer; // for interrupt use only

  int ch_A_pin; // use init() to set
  int ch_B_pin;

  Encoder(void (*ch_A_int)(), void (*ch_B_int)(), float (*sync)(unsigned long & last_elapsed, unsigned long & last_count)) :
    ch_A_interrupt(ch_A_int),
    ch_B_interrupt(ch_B_int),
    synchronise(sync)
  {
    // ...
  }
  ~Encoder() {
    // ...
  }
  void init(int pin_A, int pin_B, unsigned ppr, bool clockwise=true);
  void sync();

  inline float latest() const { return rev_s; }
};

extern Encoder E1;
extern Encoder E2;
extern Encoder E3;
extern Encoder E4;

#endif // ENABLE_ENC_CLASS

#endif /* !cariot_encoders_hh */
