class Joy {
private:
  elapsedMicros m_ticker;

  enum JoyMode {
    jm_Idle = 0,
    jm_x_send,
    jm_x_sending,
    jm_x_request,
    jm_x_requesting,
    jm_x_read,
    jm_y_send,
    jm_y_sending,
    jm_y_request,
    jm_y_requesting,
    jm_y_read,
    jm_s_send,
    jm_s_sending,
    jm_s_request,
    jm_s_requesting,
    jm_s_read
  } m_mode;

  float m_x;
  float m_y;

  unsigned m_state;
  unsigned m_changes;

  static void s_transmit();
  static void s_request();
  static void s_error();

  Joy() :
    m_ticker(0),
    m_mode(jm_Idle),
    m_x(0),
    m_y(0),
    m_state(0),
    m_changes(0)
  {
    // ...
  }
public:
  static Joy * joy();

  ~Joy() {
    // ...
  }
  bool tick();

  inline void start() {
    if (!m_mode) {
      m_mode = jm_x_send;
    }
  }
  inline bool idle() const { return m_mode == jm_Idle; }

  inline float x() const { return m_x; }
  inline float y() const { return m_y; }

  inline bool up() const     { return m_state & 0x0400; }
  inline bool down() const   { return m_state & 0x0080; }
  inline bool left() const   { return m_state & 0x0200; }
  inline bool right() const  { return m_state & 0x0040; }
  inline bool select() const { return m_state & 0x4000; }

  inline bool neutral() const { return !m_x && !m_y && !m_state; }
  inline unsigned changes() const { return m_changes; }
};
