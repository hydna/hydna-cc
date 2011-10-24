# Makefile
#
# @author Emanuel Dahlberg, http://github.com/EmanueI

SRCS = connection.cc frame.cc openrequest.cc channel.cc channeldata.cc channelsignal.cc debughelper.cc
HDRS = ../include/connection.h ../include/frame.h ../include/openrequest.h ../include/channel.h ../include/channeldata.h ../include/channelsignal.h ../include/channelmode.h ../include/error.h ../include/ioerror.h ../include/channelerror.h ../include/debughelper.h
OBJS = $(SRCS:.cc=.o)
DOBJS= $(SRCS:.cc=DEBUG.o)

CXX = g++
CXXFLAGS = -fPIC -W -Wall -ansi -Os -I../include/
DEBUGFLAGS = -g -DHYDNADEBUG -O0
LDFLAGS = -shared
SOFILE = ../lib/libhydna.so
DEBUGSOFILE = ../lib/libhydnad.so
TARGET = target
DEBUGTARGET = debugtarget

all: $(TARGET)

debug: $(DEBUGTARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $(SOFILE) $(OBJS) $(LDFLAGS)

$(DEBUGTARGET): $(DOBJS)
	$(CXX) -o $(DEBUGSOFILE) $(DOBJS) $(LDFLAGS)

$(OBJS): $(HDRS) Makefile

$(DOBJS): $(HDRS) Makefile

.SUFFIXES: .cc DEBUG.o
.cc.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<
.ccDEBUG.o:
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(DEBUGTARGET) *.o *~ core
