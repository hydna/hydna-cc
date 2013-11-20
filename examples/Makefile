# Makefile
#
# @author Emanuel Dahlberg, http://github.com/EmanueI

SRCS = hello-world.cc hello-world-binary.cc signals.cc listener.cc speed-test.cc multiple-channels.cc
HDRS = 
OBJS = $(SRCS:.cc=.o)
DOBJS= $(SRCS:.cc=DEBUG.o)
LIBDIR = ../lib

LIBDIRS = -L$(LIBDIR)

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
LIBDIRS += -Wl,-R$(LIBDIR)
endif
                                                                            
CXX = g++
CXXFLAGS = -g -Wall -ansi -I../include/
LDFLAGS = $(LIBDIRS) -lhydna -lpthread
DEBUGLDFLAGS = $(LIBDIRS) -lhydnad -lpthread
TARGET = $(OBJS:.o=)
DEBUGTARGET = $(DOBJS:DEBUG.o=DEBUG)


all: $(TARGET)

debug: $(DEBUGTARGET)

$(TARGET): $(OBJS)

$(DEBUGTARGET): $(DOBJS)

$(OBJS): $(HDRS) Makefile

$(DOBJS): $(HDRS) Makefile

%DEBUG.o : %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o : %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%DEBUG : %DEBUG.o
	$(CXX) -o $@ $< $(DEBUGLDFLAGS)

% : %.o
	$(CXX) -o $@ $< $(LDFLAGS)


clean:
	rm -f $(TARGET) $(DEBUGTARGET) *.o *~ core
