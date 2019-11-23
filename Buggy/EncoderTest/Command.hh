#include <Arduino.h>

#define RangeCheck(x,L) (((x) > (L)) ? (L) : (((x) < -(L)) ? -(L) : (x)))

static inline float byte_to_norm(unsigned long value) { // convert integer in range 0..254 to floating point number in range -1..1
  float f = 0;

  if (value < 255) {
    f = (-127.0 +(float) value) / 127.0;
  }
  return f;
}

static inline unsigned long norm_to_byte(float value) { // convert floating point number in range -1..1 to integer in range 0..254
  int ival = (int) round(127 * value);

  return (unsigned long) (127 + RangeCheck(ival, 127));
}

class Command {
  bool m_bEnableSafetyStop;

  int m_count;
  int m_length;

  char m_buffer[16];

public:
  Command() :
    m_bEnableSafetyStop(true),
    m_count(0),
    m_length(0)
  {
    //
  }
  ~Command() {
    //
  }

  void send(char code, unsigned long value = 0) const;
  bool have(char & code, unsigned long & value);
};

extern Command command;
