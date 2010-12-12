#ifndef HYDNA_STREAMSIGNAL_H
#define HYDNA_STREAMSIGNAL_H

#include <iostream>
#include <queue>

namespace hydna {
  
  class StreamSignal {
  public:
    StreamSignal(int type, const char* content, int size);
    
    /**
     *  Returns the data associated with this StreamDataEvent instance.
     */
    const char* getContent() const;
    
    int getType() const;

    int getSize() const;

  private:
    int m_type;
    const char* m_content;
    int m_size;

  };

  typedef std::queue<StreamSignal*> StreamSignalQueue;
}

#endif

