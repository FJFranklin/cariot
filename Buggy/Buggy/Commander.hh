/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_Commander_hh
#define cariot_Commander_hh

#include "FIFO.hh"

class Commander {
public:
  static float unpack754_32(uint32_t i);
  static uint32_t pack754_32(float f);

  class Responder {
  public:
    virtual void notify(Commander * C, const char * str) = 0;
    virtual void command(Commander * C, char code, unsigned long value) = 0;

    virtual ~Responder() { }
  };

protected:
  Responder * m_Responder;
  FIFO<char> m_fifo;

private:
  int m_length;
  char m_buffer[16]; // command receive buffer
  bool m_bUI;        // UI text mode
  bool m_bSOL;       // Start of line

public:
  Commander(Responder * R);
  virtual ~Commander();
  virtual const char * name() const; // override this method to provide ID
  virtual void update();             // override this method to manage IO streams

  virtual void command_send(char code, unsigned long value = 0);
  virtual void command_print(const char * str);
  virtual void ui(char c = 0);
  void ui_print(const char * str) {
    if (str)
      while(*str) {
        ui(*str++);
      }
  }

  virtual const char * eol();

protected:
  void notify(const char * str);
  void command(char code, unsigned long value);
  void push(char c);
};

#endif /* !cariot_Commander_hh */
