/* Copyright (c) 2019 Francis James Franklin
 * 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and
 *    the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Client.hh"
#include "Serial.hh"

#define CARIOT_WEBDIR "/home/pi/cariot/www/"

static void loglist();

class Car : public Client, public Serial::Command {
private:
  Serial m_S;

  float x_actual;
  float y_actual;
  float slip_l;
  float slip_r;

  const char * m_pattern;

  int m_length;

public:
  Car(const char * serial, bool verbose=false) :
    Client("car", verbose),
    m_S(this, serial, true, verbose),
    x_actual(0),
    y_actual(0)
  {
    set_sleeper(&m_S);

    m_pattern = "/cariot/#";
    m_length = strlen(m_pattern) - 1;
  }
  virtual ~Car() {
    // ...
  }
  virtual void serial_connect() {
    fprintf(stdout, "car: connected to Arduino\n");
  }
  virtual void serial_disconnect() {
    fprintf(stdout, "car: disconnected from Arduino\n");
  }
  virtual void serial_command(char command, unsigned long value) {
    switch(command) {
    case 'x':
      {
	x_actual = (-127 + (float) value) / 127;
      }
      break;
    case 'y':
      {
	y_actual = (-127 + (float) value) / 127;
      }
      break;
    case 'l':
      {
	slip_l = (-127 + (float) value) / 127;
      }
      break;
    case 'r':
      {
	slip_r = (-127 + (float) value) / 127;
      }
      break;
    default:
      break;
    }
  }
  virtual void tick() { // every millisecond
    static unsigned count = 0;
    static char buffer[32];

    if (++count == 100) {
      count = 0;
      sprintf(buffer, "%.3f %.3f", x_actual, y_actual);
      publish("/cariot/car/XY", buffer);
      sprintf(buffer, "%.3f %.3f", slip_l, slip_r);
      publish("/cariot/car/slip", buffer);
    }

    Client::tick(); // network update
  }
  virtual void second() { // every second
    if (!m_S.connected()) {
      m_S.connect();
    }
    Client::second();
  }
  virtual void setup() {
    if (!subscribe(m_pattern)) {
      if (verbose())
	fprintf(stdout, "car: Failed to subscribe with pattern %s\n", m_pattern);
    }
  }
  virtual void message(const char * topic, const char * message, int length) {
    static char buf[32];

    if (length > 31) {
      length = 31;
    }
    strncpy(buf, message, length);
    buf[length] = 0;

    topic += m_length;

    if (verbose())
      fprintf(stdout, "car: message=\"%s\" @ %s\n", buf, topic);

    if (strcmp(topic, "system/exit") == 0) {
      if (strcmp(message, "car") == 0) {
	stop();
      } else if (strcmp(message, "controller") == 0) {
	// Do something to shutdown the Raspberry Pi??
      }
    } else if (strcmp(topic, "dash/XY") == 0) {
      float x, y;
      if (sscanf(message, "%f %f", &x, &y) == 2) {
	int ix = (int) ((1 + x) * 127);
	int iy = (int) ((1 + y) * 127);
	ix = (ix < 0) ? 0 : ((ix > 254) ? 254 : ix);
	iy = (iy < 0) ? 0 : ((iy > 254) ? 254 : iy);
	m_S.write('x', (unsigned long) ix);
	m_S.write('y', (unsigned long) iy);
      }
    }
  }
};

class Logger : public Ticker, public Serial::Report {
private:
  Serial m_S;

  int m_fd;

public:
  Logger(const char * serial, bool verbose=false) :
    m_S(this, serial, true, verbose),
    m_fd(-1)
  {
    set_sleeper(&m_S);
  }
  virtual ~Logger() {
    // ...
  }
  virtual void serial_connect() {
    fprintf(stdout, "logger: connected to Arduino\n");

    char logx[] = CARIOT_WEBDIR"logs/log-XXXXXX.csv";

    m_fd = mkstemps(logx, 4);
    if (m_fd != -1) {
      fprintf(stdout, "logger: log file created\n");
    }
  }
  virtual void serial_disconnect() {
    fprintf(stdout, "logger: disconnected from Arduino\n");

    if (m_fd != -1) {
      close(m_fd);
      m_fd = -1;
      fprintf(stdout, "logger: log file closed\n");
    }
  }
  virtual void serial_report(const char * report) {
    if (m_fd != -1) {
      int length = strlen(report);
      write(m_fd, report, length);
    }
  }
  virtual void tick() { // every millisecond
    // ...
  }
  virtual void second() { // every second
    if (!m_S.connected()) {
      m_S.connect();
    }
  }
};

int main(int argc, char ** argv) {
  const char * serial = "/dev/ttyACM0";

  bool verbose = false;
  bool logger  = false;
  
  for (int arg = 1; arg < argc; arg++) {
    if (strcmp(argv[arg], "--help") == 0) {
      fprintf(stderr, "\n%s [--help] [--verbose] [/dev/<ID>]\n\n", argv[0]);
      fprintf(stderr, "  --help     Display this help.\n");
      fprintf(stderr, "  --verbose  Print debugging info.\n");
      fprintf(stderr, "  --logger   Run as a data logger.\n");
      fprintf(stderr, "  /dev/<ID>  Connect to /dev/<ID> instead of default [/dev/ttyACM0].\n\n");
      return 0;
    }
    if (strcmp(argv[arg], "--verbose") == 0) {
      verbose = true;
    } else if (strcmp(argv[arg], "--logger") == 0) {
      logger = true;
    } else if (strncmp(argv[arg], "/dev/", 5) == 0) {
      serial = argv[arg];
    } else {
      fprintf(stderr, "%s [--help] [--verbose] [/dev/ID]\n", argv[0]);
      return -1;
    }
  }

  if (logger) {
    if (!fork()) {
      loglist();
    }
    Logger(serial, verbose).loop();
  } else {
    Car(serial, verbose).loop();
  }
  return 0;
}

static const char * size_string(off_t bytes) {
  static char buffer[16];
  if (bytes < 1024) {
    snprintf(buffer, 16, "%14lu ", (unsigned long) bytes);
  } else {
    float k = (float) bytes / 1024.0;
    snprintf(buffer, 16, "%14.2fk", k);
  }
  return buffer;
}

static const char * time_string(const time_t * t_mod) {
  static char buffer[32];
  struct tm * info = localtime(t_mod);
  strftime(buffer, 31, "%c", info);
  return buffer;
}

void loglist() {
  const char * logdir  = CARIOT_WEBDIR"logs";
  const char * loglist = CARIOT_WEBDIR"logs.html";

  if (chdir(logdir) < 0) return; // oops

  FILE * f = fopen(loglist, "w");
  if (f) {
    fputs("<html>\n <head>\n  <title>Cariot Log File Listing</title>\n  <style type=\"text/css\">\n", f);
    fputs("li {\n\tfont-family: Lucida Console, Courier, monospace;\n\twhite-space: pre;\n}\n", f);
    fputs("  </style>\n </head>\n <body>\n  <ul>\n", f);

    DIR * d = opendir(".");
    if (d) {
      while (true) {
	struct dirent * e = readdir(d);
	if (!e) break;

	if (e->d_type == DT_REG)                                        // it's a regular file
	  if (strcmp(e->d_name + strlen(e->d_name) - 4, ".csv") == 0) { // with a .csv suffix
	    struct stat s;
	    if (!stat(e->d_name, &s)) {
	      const char * file_size = size_string( s.st_size);
	      const char * file_time = time_string(&s.st_mtime);

	      fprintf(f, "   <li><a href=\"logs/%s\">%s %s  %s</a></li>\n", e->d_name, file_time, file_size, e->d_name);
	    }
	  }
      }
      closedir(d);
    }
    fputs("  </ul>\n </body>\n</html>\n", f);
    fclose(f);
  }
}
