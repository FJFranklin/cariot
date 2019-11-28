#include <Arduino.h>

class Encoder {
private:
  void (*ch_A_interrupt)();
  void (*ch_B_interrupt)();

  float (*synchronise)(unsigned long & last_elapsed, unsigned long & last_count); // returns latest elapsed time and pulse count

  float cpr;   // counts per revolution = 4 * ppr
  float sense; // 1 for clockwise-is-positive; -1 otherwise
  float rev_s;

  elapsedMicros dsync;
public:
  elapsedMicros timer; // for interrupt use only

  int ch_A_pin; // use init() to set
  int ch_B_pin;

  Encoder(void (*ch_A_int)(), void (*ch_B_int)(), float (*sync)(unsigned long & last_elapsed, unsigned long & last_count)) :
    ch_A_interrupt(ch_A_int),
    ch_B_interrupt(ch_B_int),
    synchronise(sync)
  {
    // ...
  }
  ~Encoder() {
    // ...
  }
  void init(int pin_A, int pin_B, unsigned ppr, bool clockwise=true);
  void sync();

  inline float latest() const { return rev_s; }
};

extern Encoder E1;
extern Encoder E2;
extern Encoder E3;
extern Encoder E4;
