/* -*- mode: c++ -*-
 */

#include "config.hh"
#include "Timer.hh"
#include "SerialCommander.hh"

#ifdef ENABLE_BLUETOOTH
#include "BTCommander.hh"
#endif
#ifdef ENABLE_LORA
#include "LoRaCommander.hh"
#endif
#ifdef ENABLE_ENCODERS
#include "Encoders.hh"
#endif
#ifdef ENABLE_JOYWING
#include "Joy.hh"
#else
class Joy;
#endif

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
RoboClaw roboclaw(config_serial(), 10000);
#endif // ENABLE_ROBOCLAW

static void s_roboclaw_init() {
#ifdef ENABLE_ROBOCLAW
  roboclaw.begin(38400);
#endif
}

int M1_actual = 0;
int M2_actual = 0;

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

int MSpeed = 0; // Target setting for (both) motors; range is -127..127

class Buggy : public Timer, public Commander::Responder {
private:
  SerialCommander s0;
#ifdef APP_FORWARDING
  SerialCommander s1;
#endif
#ifdef APP_MOTORCONTROL
  SerialCommander s2;
#endif
#ifdef ENABLE_BLUETOOTH
  BTCommander *bt;
#endif
#ifdef ENABLE_LORA
  LoRaCommander *lora;
#endif
  Joy *J;

public:
  Buggy() :
    s0(Serial, '0', this),
#ifdef APP_FORWARDING
    s1(Serial1, '1', this),
#endif
#ifdef APP_MOTORCONTROL
    s2(Serial2, '2', this),
#endif
#ifdef ENABLE_BLUETOOTH
    bt(BTCommander::commander(this)), // zero if no Bluetooth connection is possible
#endif
#ifdef ENABLE_LORA
    lora(LoRaCommander::commander(this, LORA_ID_SELF, LORA_ID_PARTNER)), // zero if no LoRa connection is possible
#endif
    J(0)
  {
#ifdef ENABLE_JOYWING
    J = Joy::joy();
#endif
    // ...
  }
  virtual ~Buggy() {
    // ...
  }

  virtual void notify(Commander * C, const char * str) {
    if (Serial) {
      Serial.print(C->name());
      Serial.print(": notify: ");
      Serial.println(str);
    }

    if (strcmp(str, "bluetooth: disconnect") == 0) { // oops
      String e_stop("Emergency stop!");
      if (Serial) {
        Serial.println(e_stop);
      }
#ifdef APP_FORWARDING
      s1.command_send('x');
#else
      MSpeed = 0;
#endif
    }
  }
  virtual void command(Commander * C, char code, unsigned long value) {
#ifdef ENABLE_FEEDBACK
    if (Serial) {
      Serial.print(C->name());
      Serial.print(": command: ");
      Serial.print(code);
      Serial.print(": ");
      Serial.println(value);
    }
#endif

#ifdef APP_FORWARDING
    if (code == 'p') {
      s0.ui((char) (value & 0xFF));
#ifdef ENABLE_BLUETOOTH
      if (bt) {
        bt->ui((char) (value & 0xFF));
      }      
#endif
    } else if (strcmp(C->name(), "s1") == 0) { // command received from Serial1; forward to others
      s0.command_send(code, value);
#ifdef ENABLE_BLUETOOTH
      if (bt) {
        bt->command_send(code, value);
      }
#endif
    } else {                                   // command received from others; forward to Serial1
      s1.command_send(code, value);
    }
#endif
#ifdef APP_MOTORCONTROL
    switch(code) {
    case 'f':
      MSpeed = (value > 127 ? 127 : value);
      break;
    case 'b':
      MSpeed = (value > 127 ? -127 : -value);
      break;
    case 'x':
      MSpeed = 0;
      break;
    default:
      break;
    }
#endif
  }

  virtual void every_milli() { // runs once a millisecond, on average
    // ...
  }

  virtual void every_10ms() { // runs once every 10ms, on average
#ifdef APP_MOTORCONTROL
    int M1 = M1_actual;
    int M2 = M2_actual;

    if (M1 < MSpeed) {
      ++M1;
    }
    if (M1 > MSpeed) {
      --M1;
    }

    if (M2 < MSpeed) {
      ++M2;
    }
    if (M2 > MSpeed) {
      --M2;
    }

    if ((M1 != M1_actual) || (M2 != M2_actual)) {
      s_roboclaw_set(M1, M2);
    }
#endif
#ifdef ENABLE_LORA
    if (lora) { // important: housekeeping - check for incoming messages
      lora->update();
    }
#endif
#ifdef ENABLE_ENC_CLASS
    E1.sync();
#endif
  }

  virtual void every_tenth(int tenth) { // runs once every tenth of a second, where tenth = 0..9
    digitalWrite(LED_BUILTIN, tenth == 0 || tenth == 8); // double blink per second

#ifdef APP_MOTORCONTROL
    if (tenth == 0 || tenth == 5) { // i.e., every half-second
#ifdef ENABLE_ENC_CLASS
      float rpm = 60.0 * E1.latest();
#else
      float rpm = s_encoder_rpm(); // Calculate RPM
#endif
      const float d_wheel = 0.16; // Wheel diameter [m]
      float vehicle_speed = rpm * PI * d_wheel * 0.06; // Vehicle speed in km/h

      if (rpm || MSpeed || M1_actual || M2_actual) {
        char str[64];
#ifndef ENABLE_ENC_CLASS
        if (encoderForwards) {
          // str += "Forwards";
          snprintf(str, 64, "MSpeed: %d {%d,%d}; Speed: %.2f km/h; Dir.: F", MSpeed, M1_actual, M2_actual, vehicle_speed);
        } else {
          // str += "Backwards";
          snprintf(str, 64, "MSpeed: %d {%d,%d}; Speed: %.2f km/h; Dir.: B", MSpeed, M1_actual, M2_actual, vehicle_speed);
        }
#endif
        if (Serial) {
          Serial.println(str);
        }
        s2.command_print(str);
      }
    }
#endif
#ifdef ENABLE_BLUETOOTH
    if (bt) { // important: housekeeping
      bt->update();
    }
#endif
#ifdef ENABLE_LORA
    if (lora) { // important: housekeeping
      lora->update(true); // flush output, if any
    }
#endif
#ifdef ENABLE_JOYWING
    if (J) {
      J->start(); // start communication sequence
    }
#endif
  }

  virtual void every_second() { // runs once every second
#ifdef ENABLE_LORA
    static unsigned long count = 0;

    if (lora) { // important: housekeeping
      String tick("Tick #");
      tick += String(++count);
      lora->print(tick.c_str());
    }
#endif
  }

  virtual void loop() {
    s0.update(); // important: housekeeping
#ifdef APP_FORWARDING
    s1.update(); // important: housekeeping
#endif
#ifdef APP_MOTORCONTROL
    s2.update(); // important: housekeeping
#endif
#ifdef ENABLE_JOYWING
    if (J->tick()) { // returns true if Joystick communication sequence complete
      if (J->changes()) {
        String joy("x=");
        joy += String(J->x()) + String("; y=") + String(J->y()) + String("; b=");
        if (J->up())     joy += 'u';
        if (J->down())   joy += 'd';
        if (J->left())   joy += 'l';
        if (J->right())  joy += 'r';
        if (J->select()) joy += 's';
        if (Serial) {
          Serial.println(joy);
        }
        if (lora) {
          lora->print(joy.c_str());
        }
      }
    }
#endif
  }
};

void setup() {
  while (millis() < 500) {
    if (Serial) break;
  }
  Serial.begin(115200);
#ifdef APP_FORWARDING
  while (millis() < 500) {
    if (Serial1) break;
  }
  Serial1.begin(115200);
#endif
#ifdef APP_MOTORCONTROL
  while (millis() < 500) {
    if (Serial2) break;
  }
  Serial2.begin(115200);

  s_roboclaw_init();    // Setup RoboClaw
#ifdef ENABLE_ENC_CLASS
  E1.init(E1_ChA, E1_ChB, E1_ppr, false); // set up encoder 1
#else
  s_encoder_init();     // Setup encoders
#endif
#endif

  pinMode(LED_BUILTIN, OUTPUT);

  Buggy().run();
}

void loop() {
  // ...
}
