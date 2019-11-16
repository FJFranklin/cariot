srcdir = ./src

CPPFLAGS=-I/usr/local/Cellar/mosquitto/1.6.7/include
LDFLAGS=-L/usr/local/Cellar/mosquitto/1.6.7/lib

HEADERS = \
	$(srcdir)/Ticker.hh \
	$(srcdir)/Serial.hh \
	$(srcdir)/Client.hh

SOURCES = \
	$(srcdir)/Ticker.cc \
	$(srcdir)/Serial.cc \
	$(srcdir)/Client.cc \
	$(srcdir)/car.cc

car:	$(HEADERS) $(SOURCES)
	c++ -o car $(SOURCES) $(CPPFLAGS) $(LDFLAGS) -lmosquitto
