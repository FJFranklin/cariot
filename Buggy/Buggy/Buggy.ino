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
#ifdef ENABLE_GPS
#include <Adafruit_GPS.h>
#endif
#ifdef ENABLE_JOYWING
#include "Joy.hh"
#else
class Joy;
#endif

#include "Claw.hh"

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
#ifdef ENABLE_GPS
  Adafruit_GPS *gps;
#endif
  Joy *J;

  elapsedMicros report;
  bool bReporting;

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
#ifdef ENABLE_GPS
    gps(new Adafruit_GPS(&Serial3)),
#endif
    J(0),
    report(0),
    bReporting(false)
  {
#ifdef ENABLE_GPS
    gps->begin(9600);
    gps->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps->sendCommand(PMTK_API_SET_FIX_CTL_5HZ);
    gps->sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
    //gps->sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    //gps->sendCommand(PGCMD_ANTENNA);
    delay(1000);
#endif
#ifdef ENABLE_JOYWING
    J = Joy::joy();
#endif
    //pinMode(2, INPUT);
  }
  virtual ~Buggy() {
    // ...
  }
  bool reporting() {
    return bReporting /* || digitalRead(2) */;
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
    case 'R':
      bReporting = value;
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
    E2.sync();
    E3.sync();
    E4.sync();
#endif
  }

  virtual void every_tenth(int tenth) { // runs once every tenth of a second, where tenth = 0..9
    int third = 6;
#ifdef ENABLE_GPS
    if (gps->available()) {
      third = 4;
    }
#endif
    digitalWrite(LED_BUILTIN, tenth == 0 || tenth == 8 || (reporting() && tenth == third)); // double blink per second, or triple if reporting

#ifdef APP_MOTORCONTROL
    if (tenth == 0 || tenth == 5) { // i.e., every half-second
      const float d_wheel = WHEEL_DIAMETER; // Wheel diameter [m]
      bool moving = false;
#ifdef ENABLE_ENC_CLASS
      float vs1 = E1.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h
      float vs2 = E2.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h
      float vs3 = E3.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h
      float vs4 = E4.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h
      moving = vs1 || vs2 || vs3 || vs4;
#else
      float vehicle_speed = s_encoder_rpm() * PI * d_wheel * 0.06; // Vehicle speed in km/h
      moving = vehicle_speed;
#endif

      if (moving || MSpeed || M1_actual || M2_actual) {
        char str[64];
#ifdef ENABLE_ENC_CLASS
        snprintf(str, 64, "M: %d {%d %d} v: %.2f %.2f %.2f %.2f km/h", MSpeed, M1_actual, M2_actual, vs1, vs2, vs3, vs4);
#else
        if (encoderForwards) {
          // str += "Forwards";
          snprintf(str, 64, "MSpeed: %d {%d,%d}; Speed: %.2f km/h; Dir.: F", MSpeed, M1_actual, M2_actual, vehicle_speed);
        } else {
          // str += "Backwards";
          snprintf(str, 64, "MSpeed: %d {%d,%d}; Speed: %.2f km/h; Dir.: B", MSpeed, M1_actual, M2_actual, vehicle_speed);
        }
#endif
#ifndef ENABLE_GPS
        if (Serial) {
          Serial.println(str);
        }
#endif
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
      lora->command_print(tick.c_str());
    }
#endif
  }

  void generate_report() {
    char buf[48];

#ifdef ENABLE_GPS
    snprintf(buf, 48, "%02d/%02d/20%02d,%02d.%02d,%02d.%04u,",
      (int) gps->day,
      (int) gps->month,
      (int) gps->year,
      (int) gps->hour,
      (int) gps->minute,
      (int) gps->seconds,
      (unsigned int) gps->milliseconds);
    s0.ui_print(buf);

    if (gps->fix) {
      float coord = abs(gps->latitudeDegrees);
      int degrees = (int) coord;
      coord = (coord - (float) degrees) * 60;
      int minutes = (int) coord;
      coord = (coord - (float) minutes) * 60;
            
      snprintf(buf, 48, "%3d^%02d'%.4f\"%c,", degrees, minutes, coord, gps->lat ? gps->lat : ((gps->latitudeDegrees < 0) ? 'S' : 'N'));
      s0.ui_print(buf);
            
      coord = abs(gps->longitudeDegrees);
      degrees = (int) coord;
      coord = (coord - (float) degrees) * 60;
      minutes = (int) coord;
      coord = (coord - (float) minutes) * 60;
            
      snprintf(buf, 48, "%3d^%02d'%.4f\"%c,", degrees, minutes, coord, gps->lon ? gps->lon : ((gps->longitudeDegrees < 0) ? 'W' : 'E'));
      s0.ui_print(buf);

      snprintf(buf, 48, "%.6f,%.6f,", gps->latitudeDegrees, gps->longitudeDegrees);
      s0.ui_print(buf);
    } else {
      s0.ui_print(",,,,");
    }
#endif

    snprintf(buf, 48, "%10lu", (unsigned long) millis());
    s0.ui_print(buf);

#ifdef APP_MOTORCONTROL
    snprintf(buf, 48, ",%3d,%3d,%3d,", MSpeed, M1_actual, M2_actual);
    s0.ui_print(buf);

    const float d_wheel = WHEEL_DIAMETER;

#ifdef ENABLE_ENC_CLASS
    float vs1 = E1.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h
    float vs2 = E2.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h
    float vs3 = E3.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h
    float vs4 = E4.latest() * PI * d_wheel * 3.6; // Vehicle speed in km/h

    snprintf(buf, 48, "%.3f,%.3f,%.3f,%.3f", vs1, vs2, vs3, vs4);
#else
    float vehicle_speed = s_encoder_rpm() * PI * d_wheel * 0.06; // Vehicle speed in km/h

    snprintf(buf, 48, "%.3f,%c", vehicle_speed, encoderForwards ? 'F' : 'B');
#endif
    s0.ui_print(buf);
#endif

    s0.ui();
  }

  virtual void tick() {
    s0.update(); // important: housekeeping
#ifdef APP_FORWARDING
    s1.update(); // important: housekeeping
#endif
#ifdef APP_MOTORCONTROL
    s2.update(); // important: housekeeping
#endif
#ifdef ENABLE_GPS
    if (gps->available()) {
      gps->read();
      if (gps->newNMEAreceived()) {
        if (gps->parse(gps->lastNMEA())) {
          if (reporting() && report > 100000) {
            report = 0;
            generate_report();
          }
        }
      }
    }
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
          lora->command_print(joy.c_str());
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
  E1.init(E1_ChA, E1_ChB, ENCODER_PPR, false); // set up encoder 1
  E2.init(E2_ChA, E2_ChB, ENCODER_PPR, false); // set up encoder 2
  E3.init(E3_ChA, E3_ChB, ENCODER_PPR, false); // set up encoder 3
  E4.init(E4_ChA, E4_ChB, ENCODER_PPR, false); // set up encoder 4
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
