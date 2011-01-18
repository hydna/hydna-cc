# Makefile
#
# @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com

SRCS = extsocket.cc message.cc openrequest.cc stream.cc streamdata.cc streamsignal.cc
HDRS = ../include/extsocket.h ../include/message.h ../include/openrequest.h ../include/stream.h ../include/streamdata.h ../include/streamsignal.h ../include/streammode.h ../include/error.h ../include/ioerror.h ../include/streamerror.h
OBJS = $(SRCS:.cc=.o)
DOBJS= $(SRCS:.cc=.o.DEBUG)

CXX = g++
CXXFLAGS = -fPIC -W -Wall -ansi -Os -I../include/
DEBUGFLAGS = -g -DHYDNADEBUG -O0
LDFLAGS = -shared
SOFILE = ../lib/libhydna.so
TARGET = target
DEBUGTARGET = debugtarget

all: $(TARGET)

debug: $(DEBUGTARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $(SOFILE) $(OBJS) $(LDFLAGS)

$(DEBUGTARGET): $(DOBJS)
	$(CXX) -o $(SOFILE) $(DOBJS) $(LDFLAGS)

$(OBJS): $(HDRS) Makefile

$(DOBJS): $(HDRS) Makefile

.SUFFIXES: .cc .o.DEBUG
.cc.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<
.cc.o.DEBUG:
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(DEBUGTARGET) *.o *.o.DEBUG *~ core
