#ifndef HYDNA_STREAMMODE_H
#define HYDNA_STREAMMODE_H

namespace hydna {
  
  class StreamMode {
  public:
    static const unsigned int LISTEN = 0x00;
    static const unsigned int READ = 0x01;
    static const unsigned int WRITE = 0x02;
    static const unsigned int READWRITE = 0x03;
    static const unsigned int EMIT = 0x04;
    static const unsigned int READ_EMIT = 0x05;
    static const unsigned int WRITE_EMIT = 0x06;
    static const unsigned int READWRITE_EMIT = 0x07;
    
  };
}

#endif

