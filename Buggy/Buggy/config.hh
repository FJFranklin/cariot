/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_config_hh
#define cariot_config_hh

#include <Arduino.h>

//#define ENABLE_ROBOCLAW   // motor control using RoboClaw; comment to disable
//#define ENABLE_PID        // PID motor control - experimental
//#define ENABLE_BLUETOOTH  // required for Bluetooth; comment to disable
//#define ENABLE_LORA       // required for LoRa; comment to disable
//#define ENABLE_GPS        // required for GPS; comment to disable
//#define ENABLE_JOYWING    // Feather JoyWing; comment to disable
//#define ENABLE_ENCODERS   // use encoders
//#define ENABLE_ENC_CLASS  // use Encoder class
//#define ENABLE_FEEDBACK   // echo received commands to Serial, if available // FIXME - collisions!

#define TARGET_TRACKBUGGY   // Control circuit for the Track Buggy
//#define TARGET_PROTOTYPE    // Control circuit for the prototype
//#define TARGET_JOYSTICK     // Feather JoyWing & LoRa transmitter
//#define TARGET_ANTENNA      // On-board LoRa receiver

#if defined(TARGET_TRACKBUGGY) || defined(TARGET_PROTOTYPE)
#if defined(ADAFRUIT_FEATHER_M0)
#define APP_FORWARDING    // Forward commands between Serial1 and Serial/Bluetooth
#endif
#if defined(TEENSYDUINO)
#define APP_MOTORCONTROL  // Command channel on Serial2; RoboClaw on Serial1
#endif
#endif

#ifdef APP_FORWARDING
#define ENABLE_BLUETOOTH
#endif

#ifdef APP_MOTORCONTROL
#define ENABLE_ROBOCLAW
#define ENABLE_GPS
#define ENABLE_ENCODERS
#define ENABLE_ENC_CLASS
#define ENABLE_PID
#endif

#define LORA_ID_NONE_ALL  42
#define LORA_ID_ANTENNA   65
#define LORA_ID_JOYSTICK  74

#if defined(TARGET_TRACKBUGGY)
#define WHEEL_DIAMETER 0.12 // TODO - check!!
#endif
#if defined(TARGET_PROTOTYPE)
#define WHEEL_DIAMETER 0.16
#endif

#if defined(TARGET_JOYSTICK)
#define ENABLE_LORA       // required for LoRa; comment to disable
#define LORA_ID_SELF    LORA_ID_JOYSTICK
#define LORA_ID_PARTNER LORA_ID_ANTENNA
#endif
#if defined(TARGET_ANTENNA)
#define ENABLE_LORA       // required for LoRa; comment to disable
#define LORA_ID_SELF    LORA_ID_ANTENNA
#define LORA_ID_PARTNER LORA_ID_NONE_ALL // send to none, receive from all
#endif

/* Currently Bluetooth & LoRa only work for the Feather M0
 */
#if defined(ADAFRUIT_FEATHER_M0)
#if defined(ENABLE_BLUETOOTH)
#define FEATHER_M0_BTLE
#endif
#if defined(ENABLE_LORA)
#define FEATHER_M0_LORA
#endif
#endif

#if !defined(TEENSYDUINO)
/* elapsedMicros is defined automatically for Teensy; need to define it for Arduino - this is a partial definition only:
 */
class elapsedMicros {
private:
  unsigned long m_micros;

public:
  elapsedMicros(unsigned long us = 0) {
    m_micros = micros() - us;
  }
  ~elapsedMicros() {
    //
  }
  operator unsigned long () const {
    return micros() - m_micros;
  }
  elapsedMicros & operator = (unsigned long us) {
    m_micros = micros() - us;
    return *this;
  }
};

#endif

#define FIFO_BUFSIZE 256

#endif /* !cariot_config_hh */
