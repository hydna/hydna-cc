#include <sstream>

#include "channelsignal.h"

namespace hydna {
    using namespace std;
   
    ChannelSignal::ChannelSignal(int type,
            const char* content, int size) : m_type(type), m_content(content), m_size(size) {}

    int ChannelSignal::getType() const {
        return m_type;
    }

    const char* ChannelSignal::getContent() const {
        return m_content;
    }

    int ChannelSignal::getSize() const {
        return m_size;
    }
}

