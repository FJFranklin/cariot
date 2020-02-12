/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_encoders_hh
#define cariot_encoders_hh

/* Rotary Encoder Setup:
          Yellow > 5V
          Red    > GND
          Brown  > 5
          Orange > 6
          Black is the index pulse and is not used in this code
 */
    
#define ENC_COUNT_REV 256  // Rotary encoder pulses per rotation (change as required, this is in the spec of the encoder)

// Encoder output to Arduino Interrupt pin
#define ENC_A 5  //Channel A (Brown)  set to pin 5
#define ENC_B 6  //Channel B (Orange) set to pin 6

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

#endif /* !cariot_encoders_hh */
