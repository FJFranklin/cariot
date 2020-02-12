/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_SerialCommander_hh
#define cariot_SerialCommander_hh

#include "Commander.hh"

class SerialCommander : public Commander {
private:
  Stream * m_serial;

  char m_id[3];

public:
  SerialCommander(Stream & HS, char identifier, Commander::Responder * R);

  virtual ~SerialCommander();

  virtual const char * name() const;
  virtual void update();
};

#endif /* !cariot_SerialCommander_hh */
