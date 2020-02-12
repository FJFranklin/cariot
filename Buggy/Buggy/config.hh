/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_config_hh
#define cariot_config_hh

#include <Arduino.h>

#define ENABLE_ROBOCLAW   // motor control using RoboClaw; comment to disable
#define ENABLE_BLUETOOTH  // required for Bluetooth; comment to disable

/* Currently Bluetooth only works for the Feather M0
 */
#if defined(ADAFRUIT_FEATHER_M0) && defined(ENABLE_BLUETOOTH)
#define FEATHER_M0_BTLE
#endif

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

#endif /* !cariot_config_hh */
