/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

// #define FEATHER_M0_BTLE // comment this line to disable Feather Bluetooth

#ifdef FEATHER_M0_BTLE
#include "Adafruit_BluefruitLE_SPI.h"
#endif
#include "BTCommander.hh"

BTCommander::BTCommander(Commander::Responder * R) :
  Commander(R),
#ifdef FEATHER_M0_BTLE
  m_ble(new Adafruit_BluefruitLE_SPI(8, 7, 4)), // Feather M0 BTLE
#else
  m_ble(0),
#endif
  m_bConnected(false)
{
  // ...
}

bool BTCommander::init() {
  bool success = false;
#ifdef FEATHER_M0_BTLE
  if (!m_ble->begin(false)) {
    notify("Failed to init BTLE!");
  } else if (!m_ble->factoryReset()) {
    notify("Failed to reset BTLE!");
  } else {
    m_ble->echo(false);
    // m_ble->info();
    // m_ble->verbose(false);
    m_ble->sendCommandCheckOK("AT+HWModeLED=MODE");

    notify("Init BTLE & reset - okay.");
    success = true;
  }
#endif
  return success;
}

bool BTCommander::check_connection() {
#ifdef FEATHER_M0_BTLE
  if (m_ble->isConnected()) {
    if (!m_bConnected) {
      m_bConnected = true;
      notify("bluetooth: connect");
      m_ble->sendCommandCheckOK("AT+HWModeLED=DISABLE");
    }
  } else {
    if (m_bConnected) {
      m_bConnected = false;
      notify("bluetooth: disconnect");
      m_ble->sendCommandCheckOK("AT+HWModeLED=MODE");
    }
  }
#endif
  return m_bConnected;
}

int BTCommander::available() {
  int count = 0;
#ifdef FEATHER_M0_BTLE
  if (check_connection()) {
    m_ble->println("AT+BLEUARTFIFO=TX");

    long bytes = m_ble->readline_parseInt();

    if (bytes > 200) // limit single transaction length // AT commands limited to 240 bytes?? - check
      count = 200;
    else if (bytes > 0)
      count = (int) bytes - 1;
  }
#endif
  return count;
}

bool BTCommander::print(const char * str, bool add_eol) {
#ifdef FEATHER_M0_BTLE
  if (available() > strlen(str) + 2) {
    m_ble->print("AT+BLEUARTTX=");
    m_ble->print(str);
    if (add_eol) {
      m_ble->print("\\n");
    }
    m_ble->println();
    if (!m_ble->waitForOK()) {
      notify("print: wait - error!");
    }
    return true;
  }
#endif
  return false;
}

bool BTCommander::get_bytes() {
  bool have_input = false;
#ifdef FEATHER_M0_BTLE
  int count = 0;

  if (check_connection()) {
    m_ble->println("AT+BLEUARTFIFO=RX");

    long bytes = m_ble->readline_parseInt();

    if (bytes < 1024) { // limit is 1024
      count = 1023 - (int) bytes;
    }
    if (!m_ble->waitForOK()) {
      notify("get_bytes: wait - error!");
      count = 0;
    }
  }
  if (count) {
    m_ble->println("AT+BLEUARTRX");

    if (!m_ble->waitForOK()) {
      notify("get_bytes: wait - error!");
    } else {
      have_input = true;
    }
  }
#endif
  return have_input;
}

static BTCommander * s_bt = 0; // single global instance - if any

BTCommander * BTCommander::commander(Commander::Responder * R) {
#ifdef FEATHER_M0_BTLE
  static BTCommander BT(0);

  if (R) {
    BT.m_Responder = R;
  }
  if (!s_bt) {
    if (BT.init()) {
      s_bt = &BT;
    }
  }
#endif
  return s_bt;
}

BTCommander::~BTCommander() {
  // ...
}

const char * BTCommander::name() const {
  const char * bt = "bt";
  return bt;
}

void BTCommander::update() {
#ifdef FEATHER_M0_BTLE
  while (get_bytes()) {
    const char * ptr = m_ble->buffer;

    while (*ptr) {
      push(*ptr++);
    }
  }
#endif
}
