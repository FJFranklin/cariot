#include "Command.hh"

Command command;

void Command::send(char code, unsigned long value) const {
  Serial.print(code);
  if (value) {
    Serial.print(value);
  }
  Serial.print(',');
}

bool Command::have(char & code, unsigned long & value) {
  if (++m_count == 1000) { // silence or junk on the input line! (emergency stop support feature)
    m_count = 0; // reset counter - this triggers once per second if no commands are input

    if (m_bEnableSafetyStop) {
      code = 'Q';
      value = 0;
      return true;
    }
    return false;
  }

  while (Serial.available()) {
    char next = (char) Serial.read();

    if ((next >= 'A' && next <= 'Z') || (next >= 'a' && next <= 'z')) {
      m_buffer[0] = next;
      m_length = 1;
    } else if (next >= '0' && next <= '9') {
      if (m_length > 0 && m_length < 11) {
        m_buffer[m_length++] = next;
      } else {
        m_length = 0;
      }
    } else if (next == ',') {
      if (m_length > 1) {
        m_buffer[m_length] = 0;
        code = m_buffer[0];
        value = strtoul(m_buffer+1, 0, 10);
        m_count = 0; // reset the timeout
        break;
      } else if (m_length == 1) {
        code = m_buffer[0];
        value = 0;
        m_count = 0; // reset the timeout
        break;
      }
      m_length = 0;
    } else {
      m_length = 0;
    }
  }
  return m_count == 0;
}
