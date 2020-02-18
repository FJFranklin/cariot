/* -*- mode: c++ -*-
 */

#include "config.hh"
#include "Timer.hh"
#include "BTCommander.hh"
#include "LoRaCommander.hh"
#include "SerialCommander.hh"
#include "Encoders.hh"

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
  BTCommander *bt;
  LoRaCommander *lora;
  Joy *J;

public:
  Buggy() :
    s0(Serial, '0', this),
    bt(BTCommander::commander(this)), // zero if no Bluetooth connection is possible
    lora(LoRaCommander::commander(this, LORA_ID_SELF, LORA_ID_PARTNER)), // zero if no LoRa connection is possible
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
      MSpeed = 0;
    }
  }
  virtual void command(Commander * C, char code, unsigned long value) {
    if (Serial) {
      Serial.print(C->name());
      Serial.print(": command: ");
      Serial.print(code);
      Serial.print(": ");
      Serial.println(value);
    }

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
  }

  virtual void every_milli() { // runs once a millisecond, on average
    s0.update(); // important: housekeeping
  }

  virtual void every_10ms() { // runs once every 10ms, on average
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

    if (lora) { // important: housekeeping - check for incoming messages
      lora->update();
    }
  }

  virtual void every_tenth(int tenth) { // runs once every tenth of a second, where tenth = 0..9
    digitalWrite(LED_BUILTIN, tenth == 0 || tenth == 8); // double blink per second

    if (tenth == 0 || tenth == 5) { // i.e., every half-second
      float rpm = s_encoder_rpm(); // Calculate RPM

      const float d_wheel = 0.16; // Wheel diameter [m]
      float vehicle_speed = rpm * PI * d_wheel * 0.06; // Vehicle speed in km/h

      if (rpm || MSpeed || M1_actual || M2_actual) {
        String str("MSpeed: ");
        str += String(MSpeed) + String(" {") + String(M1_actual) + String(",") + String(M2_actual) + String("}; Speed: ") + String(vehicle_speed) + String(" km/h; Dir.: ");
        if (encoderForwards) {
          str += "Forwards";
        } else {
          str += "Backwards";
        }
        if (Serial) {
          Serial.println(str);
        }
        if (bt) {
          bt->print(str.c_str(), true);
        }
      }
    } else if (bt) { // important: housekeeping
      bt->update();
    }
    if (lora) { // important: housekeeping
      lora->update(true); // flush output, if any
    }
#ifdef ENABLE_JOYWING
    J->start(); // start communication sequence
#endif
  }

  virtual void every_second() { // runs once every second
    // ...

    static unsigned long count = 0;

    if (lora) { // important: housekeeping
      String tick("Tick #");
      tick += String(++count);
      lora->print(tick.c_str());
    }
  }

  virtual void loop() {
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

  pinMode(LED_BUILTIN, OUTPUT);

  s_roboclaw_init();    // Setup RoboClaw
  s_encoder_init();     // Setup encoders
  
  Buggy().run();
}

void loop() {
  // ...
}
