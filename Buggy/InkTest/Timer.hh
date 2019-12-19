/* Copyright 2019 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_Timer_hh
#define cariot_Timer_hh

class Timer {
private:
  unsigned long m_previous;

  int m_count_ms;
  int m_count_10ms;
  int m_count_tenths;

  bool m_stop;

public:
  Timer() :
    m_previous(0),
    m_count_ms(0),
    m_count_10ms(0),
    m_count_tenths(0),
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
    m_stop = false;

    while (!m_stop) {
      loop();

      // our little internal real-time clock:
      unsigned long current_time = millis();

      if (current_time != m_previous) {
        ++m_previous;
        every_milli();

        if (++m_count_ms == 10) {
          m_count_ms = 0;
          every_10ms();

          if (++m_count_10ms == 10) {
            m_count_10ms = 0;
            every_tenth(m_count_tenths);

            if (++m_count_tenths == 10) {
              m_count_tenths = 0;
              every_second();
            }
          }
        }
      }
    }
  }
};

#endif /* !cariot_Timer_hh */
