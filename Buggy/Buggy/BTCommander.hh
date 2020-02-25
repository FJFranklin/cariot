/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_BTCommander_hh
#define cariot_BTCommander_hh

#include "Commander.hh"

class Adafruit_BluefruitLE_SPI;

class BTCommander : public Commander {
private:
  Adafruit_BluefruitLE_SPI * m_ble;

  bool m_bConnected;

  BTCommander(Commander::Responder * R);
public:
  static BTCommander * commander(Commander::Responder * R = 0);

  virtual ~BTCommander();
  virtual const char * name() const;

private:
  bool init();
public:
  inline bool connected() const { return m_bConnected; }
  bool check_connection();
private:
  int available();
public:
  virtual const char * eol();
  bool print(const char * str, bool add_eol=false);
private:
  bool get_bytes();
public:
  virtual void update();
};

#endif /* !cariot_BTCommander_hh */
