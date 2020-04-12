#include "Adafruit_EPD.h"

#define COLOR1 EPD_BLACK
#define COLOR2 EPD_RED

class EInk {
private:
  Adafruit_IL0373 * m_disp;

public:
  const int display_width  = 212;
  const int display_height = 104;

  uint16_t black() const {
    return EPD_BLACK;
  }
  uint16_t red() const {
    return EPD_RED;
  }

  EInk() :
    m_disp(new Adafruit_IL0373(display_width, display_height, 9 /* DC */, 5 /* RST */, 10 /* CS */, 11 /* SRCS */, 7 /* BUSY */))
  {
    m_disp->begin();
    m_disp->setTextWrap(false);
  }

  ~EInk() {
    delete m_disp;
  }

  void clear() {
    m_disp->clearBuffer();
  }
  void text(int16_t x, int16_t y, uint16_t color, const char * str) {
    m_disp->setCursor(x, y);
    m_disp->setTextColor(color);
    m_disp->print(str);
    m_disp->display();
  }
  void line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    m_disp->drawLine(x1, y1, x2, y2, color);
    m_disp->display();
  }
  void test() {
    clear();
    for (int i = 0; i < 0; i++) {
      text(i,   i, black(), String(i).c_str());
      text(i+3, i, red(),   "The quick brown fox jumps over");
    }
  }
};
