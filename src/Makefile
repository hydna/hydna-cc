# Makefile
#
# @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com

SRCS = hydna.cc hydnaaddr.cc hydnapacket.cc hydnapacketbuffer.cc hydnastreambuf.cc hydnastream.cc
HDRS = ../include/hydna.h ../include/hydnaaddr.h ../include/hydnapacket.h ../include/hydnapacketbuffer.h ../include/hydnastreambuf.h ../include/hydnastream.h ../include/hydnaaddrexception.h
OBJS = $(SRCS:.cc=.o)
DOBJS= $(SRCS:.cc=.o.DEBUG)

CXX = g++
CXXFLAGS = -fPIC -W -Wall -ansi -O3 -I../include/
DEBUGFLAGS = -DHYDNADEBUG
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
