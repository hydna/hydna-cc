# Makefile
#
# @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com

SRCS = main.cpp
HDRS = 
OBJS = $(SRCS:.cpp=.o)

CXX = g++
CXXFLAGS = -g -Wall -ansi -I../include/
LDFLAGS = -L../lib -lhydna
TARGET = hydnatestclient

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HDRS) Makefile

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) *.o *~ core
