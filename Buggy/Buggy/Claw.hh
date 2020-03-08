/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_claw_hh
#define cariot_claw_hh

int MSpeed = 0; // Target setting for (both) motors; range is -127..127

int M1_actual = 0;
int M2_actual = 0;

#ifdef ENABLE_ROBOCLAW
#include <RoboClaw.h>

/* Motor Setup:
   (RoboClaw) S1 > (Uno etc) 11
              S2 >           10
              S1->           GND
 * 
   (RoboClaw) S1 > (other)   1
              S2 >           0
              S1->           GND
 */

/* The RoboClaw expects software serial on AVR systems (Uno, etc.)
 */
#if defined(ADAFRUIT_FEATHER_M0) || defined(TEENSYDUINO)
static HardwareSerial * config_serial() {
  return &Serial1; // RX,TX = 0,1
}
#else
#include <SoftwareSerial.h>
static SoftwareSerial * config_serial() {
  static SoftwareSerial serial(10, 11); // On the Uno, pins 0,1 correspond to the main serial line; use RX,TX = 10,11 instead
  return &serial;
}
#endif // RoboClaw serial setup

RoboClaw roboclaw(config_serial(), 10000);
#endif

static void s_roboclaw_init() {
#ifdef ENABLE_ROBOCLAW
  roboclaw.begin(38400);
#endif
}

static void s_roboclaw_set(int M1, int M2) {
  const uint8_t address = 0x80;
#ifdef ENABLE_ROBOCLAW
  if (M1 < 0) {
    if (M1 < -127) {
      M1 = -127;
    }
    roboclaw.BackwardM1(address, (uint8_t) (-M1));
  } else {
    if (M1 > 127) {
      M1 = 127;
    }
    roboclaw.ForwardM1(address, (uint8_t) M1);
  }

  if (M2 < 0) {
    if (M2 < -127) {
      M2 = -127;
    }
    roboclaw.BackwardM2(address, (uint8_t) (-M2));
  } else {
    if (M2 > 127) {
      M2 = 127;
    }
    roboclaw.ForwardM2(address, (uint8_t) M2);
  }
#endif
  M1_actual = M1;
  M2_actual = M2;
}

#endif /* !cariot_claw_hh */
