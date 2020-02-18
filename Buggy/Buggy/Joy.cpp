/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#include "config.hh"
#include "Joy.hh"

#if defined(TEENSYDUINO)
#include <i2c_t3.h>
#else
#include <Wire.h>
#endif

static Joy *s_J = 0;

static const char     address = 0x49;
static const unsigned pinmask = 0x46C0;

static void s_write_pins(char lo) {
  static char data[6] = { 0x01, 0x00, 0x00, 0x00, 0x46, 0xC0 }; // see pinmask

  data[1] = lo;

  Wire.beginTransmission(address);
  Wire.write(data, 6);
  Wire.endTransmission();

  delayMicroseconds(200);
}

Joy * Joy::joy() {
  static Joy joy;

  if (!s_J) {
#if defined(TEENSYDUINO)
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_33_34, I2C_PULLUP_INT, 100000);
    Wire.setDefaultTimeout(500); // 0.5ms

    Wire.onTransmitDone(Joy::s_transmit);
    Wire.onReqFromDone(Joy::s_request);
    Wire.onError(Joy::s_error);
#else
    Wire.begin();
#endif

    delayMicroseconds(200);
    s_write_pins(0x03);
    s_write_pins(0x0B);
    s_write_pins(0x05);

    s_J = &joy;
  }
  return s_J;
}

static inline float s_map(int reading) {
  float value = 0;

  if ((reading < 503) || (reading > 521)) {
    value = (2 * (float) reading - 1023) / 1023;
  }
  return value;
}

bool Joy::tick() {
  const unsigned long interval = 100;

  bool sequence_complete = false;

  switch(m_mode) {
    case jm_x_send:
    {
      m_changes = 0;

      char data[2];
      data[0] = 0x09;
      data[1] = 0x08;

      // Serial.println("i2c: sending...");
      Wire.beginTransmission(address);
      Wire.write(data, 2);
#if defined(TEENSYDUINO)
      Wire.sendTransmission();
      m_mode = jm_x_sending;
#else
      Wire.endTransmission();
      m_mode = jm_x_request;
      m_ticker = 0;
#endif
      break;
    }
    case jm_x_request:
    {
      if (m_ticker < interval) break;

#if defined(TEENSYDUINO)
      Wire.sendRequest(address, 2);
      m_mode = jm_x_requesting;
#else
      Wire.requestFrom(address, 2);
      m_mode = jm_x_read;
      m_ticker = 0;
#endif
      break;
    }
    case jm_x_read:
    {
      int value = 0;
      while (Wire.available()) {
        char data;
#if defined(TEENSYDUINO)
        Wire.read(&data, 1);
#else
        data = Wire.read();
#endif
        value = (value << 8) | data;
      }
      float new_x = s_map(value);
      if (m_x != new_x) {
        m_x = new_x;
        m_changes |= 0x0001;
      }

      m_mode = jm_y_send;
      break;
    }

    case jm_y_send:
    {
      if (m_ticker < interval) break;

      char data[2];
      data[0] = 0x09;
      data[1] = 0x07;

      // Serial.println("i2c: sending...");
      Wire.beginTransmission(address);
      Wire.write(data, 2);
#if defined(TEENSYDUINO)
      Wire.sendTransmission();
      m_mode = jm_y_sending;
#else
      Wire.endTransmission();
      m_mode = jm_y_request;
      m_ticker = 0;
#endif
      break;
    }
    case jm_y_request:
    {
      if (m_ticker < interval) break;

#if defined(TEENSYDUINO)
      Wire.sendRequest(address, 2);
      m_mode = jm_y_requesting;
#else
      Wire.requestFrom(address, 2);
      m_mode = jm_y_read;
      m_ticker = 0;
#endif
      break;
    }
    case jm_y_read:
    {
      int value = 0;
      while (Wire.available()) {
        char data;
#if defined(TEENSYDUINO)
        Wire.read(&data, 1);
#else
        data = Wire.read();
#endif
        value = (value << 8) | data;
      }
      float new_y = -s_map(value);
      if (m_y != new_y) {
        m_y = new_y;
        m_changes |= 0x0002;
      }

      m_mode = jm_s_send;
      break;
    }

    case jm_s_send:
    {
      if (m_ticker < interval) break;

      char data[2];
      data[0] = 0x01;
      data[1] = 0x04;

      // Serial.println("i2c: sending...");
      Wire.beginTransmission(address);
      Wire.write(data, 2);
#if defined(TEENSYDUINO)
      Wire.sendTransmission();
      m_mode = jm_s_sending;
#else
      Wire.endTransmission();
      m_mode = jm_s_request;
      m_ticker = 0;
#endif
      break;
    }
    case jm_s_request:
    {
      if (m_ticker < interval) break;

#if defined(TEENSYDUINO)
      Wire.sendRequest(address, 4);
      m_mode = jm_s_requesting;
#else
      Wire.requestFrom(address, 4);
      m_mode = jm_s_read;
      m_ticker = 0;
#endif
      break;
    }
    case jm_s_read:
    {
      unsigned long value = 0;
      while (Wire.available()) {
        char data;
#if defined(TEENSYDUINO)
        Wire.read(&data, 1);
#else
        data = Wire.read();
#endif
        value = (value << 8) | data;
      }
      unsigned new_state = ~value & pinmask;
      if (m_state != new_state) {
        m_changes |= m_state ^ new_state;
        m_state = new_state;
      }

      // we've completed the update; return true
      sequence_complete = true;

      m_mode = jm_Idle;
      break;
    }

    default:
    {
      break;
    }
  }
  return sequence_complete;
}

#if defined(TEENSYDUINO)

void Joy::s_transmit() {
  // Serial.print("i2c: event: transmit: ");
  switch(s_J->m_mode) {
    case jm_x_sending: s_J->m_mode = jm_x_request; break;
    case jm_y_sending: s_J->m_mode = jm_y_request; break;
    case jm_s_sending: s_J->m_mode = jm_s_request; break;

    default: break;
  }
  s_J->m_ticker = 0;
}

void Joy::s_request() {
  // Serial.print("i2c: event: request: ");
  switch(s_J->m_mode) {
    case jm_x_requesting: s_J->m_mode = jm_x_read; break;
    case jm_y_requesting: s_J->m_mode = jm_y_read; break;
    case jm_s_requesting: s_J->m_mode = jm_s_read; break;

    default: break;
  }
  s_J->m_ticker = 0;
}

void Joy::s_error() {
  Serial.print("i2c: event: error: ");
  switch (Wire.status()) {
    case I2C_TIMEOUT:  Serial.println("I2C timeout"); Wire.resetBus(); break;
    case I2C_ADDR_NAK: Serial.println("Slave addr not acknowledged"); break;
    case I2C_DATA_NAK: Serial.println("Slave data not acknowledged"); break;
    case I2C_ARB_LOST: Serial.println("Arbitration Lost, possible pullup problem"); Wire.resetBus(); break;
    case I2C_BUF_OVF:  Serial.println("I2C buffer overflow"); break;
    case I2C_NOT_ACQ:  Serial.println("Cannot acquire bus, possible stuck SDA/SCL"); Wire.resetBus(); break;
    case I2C_DMA_ERR:  Serial.println("DMA Error"); break;
    default:           Serial.println("(unknown)"); break;
  }
}

#endif // TEENSYDUINO
