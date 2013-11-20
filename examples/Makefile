# Makefile
#
# @author Emanuel Dahlberg, http://github.com/EmanueI

SRCS = hello-world.cc hello-world-binary.cc signals.cc listener.cc speed-test.cc multiple-channels.cc
HDRS = 
OBJS = $(SRCS:.cc=.o)
DOBJS= $(SRCS:.cc=DEBUG.o)

CXX = g++
CXXFLAGS = -g -Wall -ansi -I../include/
LDFLAGS = -L../lib -lhydna -lpthread
DEBUGLDFLAGS = -L../lib -lhydnad -lpthread
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
