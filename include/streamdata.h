#ifndef HYDNA_STREAMDATA_H
#define HYDNA_STREAMDATA_H

#include <iostream>
#include <queue>

namespace hydna {
  
  class StreamData {
  public:
    StreamData(int priority, const char* content, int size);
    
    /**
     *  Returns the data associated with this StreamData instance.
     *
     *  @return The content.
     */
    const char* getContent() const;
    
    /**
     *  Returns the priority of the content.
     *
     *  @return The priority of the content.
     */
    int getPriority() const;

    /**
     *  Returns the size of the content.
     *
     *  @return The size of the content.
     */
    int getSize() const;

  private:
    int m_priority;
    const char* m_content;
    int m_size;

  };

  typedef std::queue<StreamData*> StreamDataQueue;
}

#endif

