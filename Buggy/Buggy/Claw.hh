/* Copyright 2020-21 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_claw_hh
#define cariot_claw_hh

int MSpeed = 0; // Target setting for (both) motors; range is -127..127

int M1_actual = 0;
int M2_actual = 0;

struct mc_params {
  float P;
  float I;
  float D;
} MC_Params = { 1.0, 0.0, 0.0 };

struct tb_params {
  float slip;
  float target; // target Buggy speed in km/h
  float actual; // actual Buggy speed in km/h, estimated from encoders

  float actual_FL; // front left  (non-powered)
  float actual_BL; // back  left  (powered)
  float actual_FR; // front right (powered)
  float actual_BR; // back  right (non-powered)

  float P; // Use Q/J/E commands to set these values
  float I;
  float D;
} TB_Params = { 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 };

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

static void s_roboclaw_set_M1(int M1) {
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
#endif
  M1_actual = M1;
}

static void s_roboclaw_set_M2(int M2) {
  const uint8_t address = 0x80;
#ifdef ENABLE_ROBOCLAW
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
  M2_actual = M2;
}

class MC {
private:
  void (*m_set)(int M);

  float kmh_error_old;
  float kmh_error_integral;

public:
  MC(void (*set)(int M)) :
    m_set(set),
    kmh_error_old(0),
    kmh_error_integral(0)
  {
    // ...
  }

  ~MC() {
    // ...
  }

  /* Working in km/h, and time interval in milliseconds
   */
  void update(float kmh_target, float kmh_actual, float dt_ms) {
    float kmh_error = kmh_target - kmh_actual;
    float error_difference = (kmh_error - kmh_error_old) / dt_ms;

    kmh_error_integral += ((kmh_error + kmh_error_old) / 2) * dt_ms;
    kmh_error_old = kmh_error;

    float pwm_estimate = MC_Params.P * kmh_error
                       + MC_Params.I * kmh_error_integral
                       + MC_Params.D * error_difference;

    (*m_set) ((int) pwm_estimate);
  }
};

MC MC_Left(&s_roboclaw_set_M1);  // Check: Is M1 on the left?
MC MC_Right(&s_roboclaw_set_M2); // Check: Is M2 on the right?

void s_buggy_update(float dt_ms) {
  /* We need two PID control systems: this one for the Track Buggy speed, and a separate one for each motor speed
   */
  static float kmh_error_old = 0;
  static float kmh_error_integral = 0;

  float kmh_error = TB_Params.target - TB_Params.actual;
  float error_difference = (kmh_error - kmh_error_old) / dt_ms;

  kmh_error_integral += ((kmh_error + kmh_error_old) / 2) * dt_ms;
  kmh_error_old = kmh_error;

  float pwm_estimate = TB_Params.P * kmh_error
                     + TB_Params.I * kmh_error_integral
                     + TB_Params.D * error_difference;

  /* Some slip between the powered wheels and the rails is inevitable; need to control it, however
   */
  if (pwm_estimate > TB_Params.actual + TB_Params.slip) {
    pwm_estimate = TB_Params.actual + TB_Params.slip;
  } else if (pwm_estimate < TB_Params.actual - TB_Params.slip) {
    pwm_estimate = TB_Params.actual - TB_Params.slip;
  }

  /* If the non-powered wheels are kept stationary, then TB_Params.actual will be zero.
   * If, also, TB_Params.P/I/D = 1/0/0, and TB_Params.slip > TB_Params.target, then 
   * pwm_estimate = TB_Params.target and the pure motor response can be recorded.
   */
  MC_Left.update(pwm_estimate, TB_Params.actual_BL, dt_ms);
  MC_Right.update(pwm_estimate, TB_Params.actual_FR, dt_ms);
}

#endif /* !cariot_claw_hh */
