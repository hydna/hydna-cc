# Makefile
#
# @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com

SRCS = hello-world.cc signals.cc listener.cc
HDRS = 
OBJS = $(SRCS:.cc=.o)

CXX = g++
CXXFLAGS = -g -Wall -ansi -I../include/
LDFLAGS = -L../lib -lhydna
TARGET = $(OBJS:.o=)

all: $(TARGET)

$(TARGET): $(OBJS)

$(OBJS): $(HDRS) Makefile

.SUFFIXES: .o
.cc.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<
.o:
	$(CXX) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o *~ core
