#ifndef HYDNA_STREAMMODE_H
#define HYDNA_STREAMMODE_H

namespace hydna {
  
  class StreamMode {
  public:
    static const unsigned int READ = 0x01;
    static const unsigned int WRITE = 0x02;
    static const unsigned int READWRITE = 0x03;
    static const unsigned int READ_SIG = 0x05;
    static const unsigned int WRITE_SIG = 0x06;
    static const unsigned int READWRITE_SIG = 0x07;
  };
}

#endif

