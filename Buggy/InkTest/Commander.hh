/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_Commander_hh
#define cariot_Commander_hh

class Commander {
public:
  class Responder {
  public:
    virtual void notify(Commander * C, const char * str) = 0;
    virtual void command(Commander * C, char code, unsigned long value) = 0;

    virtual ~Responder() { }
  };

protected:
  Responder * m_Responder;

private:
  int m_length;
  char m_buffer[16];

public:
  Commander(Responder * R);
  virtual ~Commander();
  virtual const char * name() const; // override this method to provide ID
  virtual void update();             // override this method to manage IO streams

protected:
  void notify(const char * str);
  void command(char code, unsigned long value);
  void push(char c);
};

#endif /* !cariot_Commander_hh */
