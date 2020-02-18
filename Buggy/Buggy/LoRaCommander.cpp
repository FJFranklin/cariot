/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#include "config.hh"

#ifdef FEATHER_M0_LORA
#include <SPI.h>
#include <RH_RF95.h> // RadioHead library

#define RFM95_CS  8 // pins for Feather M0 RFM9x
#define RFM95_RST 4
#define RFM95_INT 3

#define RF95_FREQ 868.0 // 915.0 // frequency

#endif
#include "LoRaCommander.hh"

LoRaCommander::LoRaCommander(Commander::Responder * R, unsigned char id_self, unsigned char id_partner) :
  Commander(R),
  m_id_self(id_self),
  m_id_partner(id_partner),
#ifdef FEATHER_M0_LORA
  m_rf95(new RH_RF95(RFM95_CS, RFM95_INT)), // Feather M0 LoRa
#else
  m_rf95(0),
#endif
  m_bConnected(false)
{
  // ...
}

bool LoRaCommander::init() {
  bool success = false;
#ifdef FEATHER_M0_LORA
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!m_rf95->init()) {
    notify("Failed to init LoRa!");
  } else {
    /* Defaults after init are: 
     *   434.0MHz, modulation GFSK_Rb250Fd250, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
     *     [= DR5 SF7/125kHz 5470(bits/s) 230(max.payload)]
     *   
     * The default transmitter power is 13dBm, using PA_BOOST. If you are using RFM95/96/97/98 modules which uses the
     * PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm.
     */
    if (!m_rf95->setFrequency(RF95_FREQ)) {
      notify("Failed to set frequency!");
    } else {
      m_rf95->setTxPower(23, false);

      notify("Init LoRa & reset - okay.");
      success = true;
    }
  }
#endif
  return success;
}

bool LoRaCommander::check_connection() {
#if 0 //def FEATHER_M0_LORA
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

int LoRaCommander::available() {
  int count = 0;
#if 0 // def FEATHER_M0_LORA
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

bool LoRaCommander::print(const char * str, bool add_eol) {
#ifdef FEATHER_M0_LORA
  m_rf95->send((uint8_t *) str, strlen(str));
#endif
#if 0
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

static LoRaCommander * s_lora = 0; // single global instance - if any

LoRaCommander * LoRaCommander::commander() {
  return s_lora;
}

LoRaCommander * LoRaCommander::commander(Commander::Responder * R, unsigned char id_self, unsigned char id_partner) {
#ifdef FEATHER_M0_LORA
  static LoRaCommander LoRa(R, id_self, id_partner);

  if (!s_lora) {
    if (LoRa.init()) {
      s_lora = &LoRa;
    }
  }
#endif
  return s_lora;
}

LoRaCommander::~LoRaCommander() {
  // ...
}

const char * LoRaCommander::name() const {
  const char * lora = "lora";
  return lora;
}

void LoRaCommander::update(bool flush_output) {
#ifdef FEATHER_M0_LORA
  static uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  static bool bTryFlush = false;

  if (flush_output) {
    bTryFlush = true;
  }

  if (m_rf95->available()) {
    if (m_rf95->recv(buf, &len)) {
      String message("Received packet: \"");
      for (uint8_t i = 0; i < len; i++) {
        if (isprint(buf[i])) {
          message.concat((char) buf[i]); 
        } else {
          message.concat('?');
        }
      }
      message += String("\"; RSSI=") + String(m_rf95->lastRssi(), DEC);

      notify(message.c_str());
    }
  }
#endif
#if 0
  while (get_bytes()) {
    const char * ptr = m_ble->buffer;

    while (*ptr) {
      push(*ptr++);
    }
  }
#endif
}
