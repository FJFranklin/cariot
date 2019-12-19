/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_Timer_hh
#define cariot_Timer_hh

class Timer {
private:
  bool m_stop;

public:
  Timer() :
    m_stop(false)
  {
    // ...
  }

  virtual ~Timer() {
    // ...
  }

  virtual void every_milli() { // runs once a millisecond, on average
    // ...
  }
  virtual void every_10ms() { // runs once every 10ms, on average
    // ...
  }
  virtual void every_tenth(int tenth) { // runs once every tenth of a second, where tenth = 0..9
    // ...
  }
  virtual void every_second() { // runs once every second
    // ...
  }
  virtual void loop() {
    // ...
  }

  inline void stop() {
    m_stop = true;
  }
  void run() {
    int count_ms = 0;
    int count_10ms = 0;
    int count_tenths = 0;

    unsigned long previous_time = millis();

    m_stop = false;

    while (!m_stop) {
      loop();

      // our little internal real-time clock:
      unsigned long current_time = millis();

      if (current_time != previous_time) {
        ++previous_time;
        every_milli();

        if (++count_ms == 10) {
          count_ms = 0;
          every_10ms();

          if (++count_10ms == 10) {
            count_10ms = 0;
            every_tenth(count_tenths);

            if (++count_tenths == 10) {
              count_tenths = 0;
              every_second();
            }
          }
        }
      }
    }
  }
};

#endif /* !cariot_Timer_hh */
