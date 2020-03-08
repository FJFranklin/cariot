/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#include "config.hh"
#include "Commander.hh"

Commander::Commander(Responder * R) :
  m_Responder(R),
  m_length(0),
  m_bUI(false),
  m_bSOL(true)
{
  // ...
}

Commander::~Commander() {
  // ...
}

const char * Commander::name() const {
  const char * unknown = "(unknown)";
  return unknown;
}

void Commander::update() {
  // ...
}

void Commander::command_send(char code, unsigned long value) {
  if (m_bUI) {
    ui(); // line-break for readability
  }

  char buf[16];
  snprintf(buf, 16, "%c%lu,", code, value);
  m_fifo.write(buf, strlen(buf));

  m_bSOL = false;
}

void Commander::command_print(const char * str) {
  if (str) {
    const char * ptr = str;
    while (*ptr) {
      command_send('p', (unsigned long) (*ptr++));
    }
  }
  command_send('p');
}

const char * Commander::eol() {
  static const char * str_eol = "\n";
  return str_eol;
}

void Commander::ui(char c) {
  bool bPrintable = isprint(c);

  if (!m_bSOL && ((!c && m_bUI) || (bPrintable && !m_bUI))) { // line-break for readability
    const char * str = eol();
    int len = strlen(str);
    if (m_fifo.availableToWrite() >= len) { // don't add the end-of-line unless you can add the whole string
      m_fifo.write(str, len);
    }
    m_bUI = false;
    m_bSOL = true;
  }
  if (bPrintable) { // append
    m_fifo.push(c);
    m_bUI = true;
    m_bSOL = false;
  }
}

void Commander::notify(const char * str) {
  if (m_Responder) {
    m_Responder->notify(this, str);
  }
}

void Commander::command(char code, unsigned long value) {
  if (m_Responder) {
    m_Responder->command(this, code, value);
  }
}

void Commander::push(char next) {
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
      command(m_buffer[0], strtoul(m_buffer+1, 0, 10));
    } else if (m_length == 1) {
      command(m_buffer[0], 0);
    }
    m_length = 0;
  } else {
    m_length = 0;
  }
}
