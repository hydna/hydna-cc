#include <sstream>

#include "channeldata.h"
#include "contenttype.h"

namespace hydna {
    using namespace std;
   
    ChannelData::ChannelData(int priority, const char* content, int size, int ctype) : m_priority(priority), m_content(content), m_size(size), m_ctype(ctype) {
        
        m_binary = (ctype == (int)ContentType::BINARY) ? false : true;
    }

    int ChannelData::getPriority() const {
        return m_priority;
    }
    
    bool ChannelData::isUtf8Content() const {
        return !m_binary;
    }
    
    bool ChannelData::isBinaryContent() const {
        return m_binary;
    }

    const char* ChannelData::getContent() const {
        return m_content;
    }

    int ChannelData::getSize() const {
        return m_size;
    }
}

