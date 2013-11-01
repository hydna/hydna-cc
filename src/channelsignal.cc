#include <sstream>

#include "channelsignal.h"
#include "contenttype.h"

namespace hydna {
    using namespace std;
   
    ChannelSignal::ChannelSignal(int type, const char* content, int size, int ctype) : m_type(type), m_content(content), m_size(size), m_ctype(ctype) {
        m_binary = (ctype == (int)ContentType::BINARY) ? false : true;
    }

    int ChannelSignal::getType() const {
        return m_type;
    }
    
    bool ChannelSignal::isUtf8Content() const {
        return !m_binary;
    }
    
    bool ChannelSignal::isBinaryContent() const {
        return m_binary;
    }

    const char* ChannelSignal::getContent() const {
        return m_content;
    }

    int ChannelSignal::getSize() const {
        return m_size;
    }
}

