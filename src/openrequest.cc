#include "addr.h";
#include "message.h";
#include "stream.h";

namespace hydna {

    OpenRequest::OpenRequest(Stream* stream,
                            Addr addr,
                            Message* message) :
                            m_stream(stream),
                            m_addr(addr),
                            m_message(message) {
    }

    Stream* OpenRequest::getStream() {
        return m_stream;
    }

    Addr OpenRequest::getAddr() {
        return m_addr;
    }

    Message& OpenRequest::getMessage() {
        return *m_message;
    }

    bool OpenRequest::isSent() const {
        return m_sent;
    }

    void OpenRequest::setSent(bool value) {
        m_sent = value;
    }
}

