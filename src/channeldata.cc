#include <sstream>

#include "channeldata.h"

namespace hydna {
    using namespace std;
   
    ChannelData::ChannelData(int priority,
            const char* content, int size) : m_priority(priority), m_content(content), m_size(size) {}

    int ChannelData::getPriority() const {
        return m_priority;
    }

    const char* ChannelData::getContent() const {
        return m_content;
    }

    int ChannelData::getSize() const {
        return m_size;
    }
}

