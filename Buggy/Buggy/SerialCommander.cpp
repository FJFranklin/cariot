/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#include "config.hh"
#include "SerialCommander.hh"

SerialCommander::SerialCommander(Stream & HS, char identifier, Commander::Responder * R) :
  Commander(R),
  m_serial(&HS)
{
  m_id[0] = 's';
  m_id[1] = identifier;
  m_id[2] = 0;
}

SerialCommander::~SerialCommander() {
  // ...
}

const char * SerialCommander::name() const {
  return m_id;
}

void SerialCommander::update() {
  while (m_serial->available()) {
    push((char) m_serial->read());
  }
}
