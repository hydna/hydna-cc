#ifndef HYDNA_STREAMSIGNAL_H
#define HYDNA_STREAMSIGNAL_H

#include <iostream>
#include <queue>

namespace hydna {
  
  class ChannelSignal {
  public:
    ChannelSignal(int type, const char* content, int size);
    
    /**
     *  Returns the content associated with this ChannelSignal instance.
     *
     *  @return The content.
     */
    const char* getContent() const;
    
    /**
     *  Returns the type of the content.
     *
     *  @return The type of the content.
     */
    int getType() const;

    /**
     *  Returns the size of the content.
     *
     *  @return The size of the content.
     */
    int getSize() const;

  private:
    int m_type;
    const char* m_content;
    int m_size;

  };

  typedef std::queue<ChannelSignal*> ChannelSignalQueue;
}

#endif

