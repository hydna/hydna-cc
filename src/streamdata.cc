#include <sstream>

#include "streamdata.h"

namespace hydna {
    using namespace std;
   
    StreamData::StreamData(int priority,
            const char* content, int size) : m_priority(priority), m_content(content), m_size(size) {}

    int StreamData::getPriority() const {
        return m_priority;
    }

    const char* StreamData::getContent() const {
        return m_content;
    }

    int StreamData::getSize() const {
        return m_size;
    }
}

