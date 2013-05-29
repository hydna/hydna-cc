#ifndef HYDNA_CHANNELDATA_H
#define HYDNA_CHANNELDATA_H

#include <iostream>
#include <queue>

namespace hydna {
  
  class ChannelData {
  public:
    ChannelData(int priority, const char* content, int size);
    
    /**
     *  Returns the data associated with this ChannelData instance.
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
    
    bool isUtf8Content() const;
    
    bool isBinaryContent() const;

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
    bool m_binary;

  };

  typedef std::queue<ChannelData*> ChannelDataQueue;
}

#endif

