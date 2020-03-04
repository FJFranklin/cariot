/* Copyright 2020 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_LoRaCommander_hh
#define cariot_LoRaCommander_hh

#include "Commander.hh"

class RH_RF95;
class MiniChain;

class LoRaCommander : public Commander {
private:
  RH_RF95 * m_rf95;
  MiniChain * m_chain;

  unsigned char m_id_self;
  unsigned char m_id_partner;

  bool m_bConnected;

  LoRaCommander(Commander::Responder * R, unsigned char id_self, unsigned char id_partner);
public:
  static LoRaCommander * commander();
  static LoRaCommander * commander(Commander::Responder * R, unsigned char id_self, unsigned char id_partner);

  virtual ~LoRaCommander();
  virtual const char * name() const;

private:
  bool init();
public:
  inline bool connected() const { return m_bConnected; }
  bool check_connection();
private:
  int available();
public:
  virtual void command_send(char code, unsigned long value = 0);
  virtual void command_print(const char * str);
  virtual void ui(char c = 0);
public:
  virtual void update(bool flush_output=false);
};

#endif /* !cariot_LoRaCommander_hh */
