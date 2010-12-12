#ifndef HYDNA_STREAMDATA_H
#define HYDNA_STREAMDATA_H

#include <iostream>
#include <queue>

namespace hydna {
  
  class StreamData {
  public:
    StreamData(int priority, const char* content, int size);
    
    /**
     *  Returns the data associated with this StreamDataEvent instance.
     */
    const char* getContent() const;
    
    int getPriority() const;

    int getSize() const;

  private:
    int m_priority;
    const char* m_content;
    int m_size;

  };

  typedef std::queue<StreamData*> StreamDataQueue;
}

#endif

