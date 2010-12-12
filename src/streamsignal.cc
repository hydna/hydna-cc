#include <sstream>

#include "streamsignal.h"

namespace hydna {
    using namespace std;
   
    StreamSignal::StreamSignal(int type,
            const char* content, int size) : m_type(type), m_content(content), m_size(size) {}

    int StreamSignal::getType() const {
        return m_type;
    }

    const char* StreamSignal::getContent() const {
        return m_content;
    }

    int StreamSignal::getSize() const {
        return m_size;
    }
}

