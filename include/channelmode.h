#ifndef HYDNA_STREAMMODE_H
#define HYDNA_STREAMMODE_H

namespace hydna {
  
  class ChannelMode {
  public:
    static const unsigned int LISTEN = 0x00;
    static const unsigned int READ = 0x01;
    static const unsigned int WRITE = 0x02;
    static const unsigned int READWRITE = 0x03;
    static const unsigned int EMIT = 0x04;
    static const unsigned int READEMIT = 0x05;
    static const unsigned int WRITEEMIT = 0x06;
    static const unsigned int READWRITEEMIT = 0x07;
    
  };
}

#endif

