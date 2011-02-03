#include "packet.h";
#include "stream.h";

namespace hydna {

    OpenRequest::OpenRequest(Stream* stream,
                            unsigned int addr,
                            Packet* packet) :
                            m_stream(stream),
                            m_addr(addr),
                            m_packet(packet) {
    }

    OpenRequest::~OpenRequest() {
        delete m_packet;
    }

    Stream* OpenRequest::getStream() {
        return m_stream;
    }

    unsigned int OpenRequest::getAddr() {
        return m_addr;
    }

    Packet& OpenRequest::getPacket() {
        return *m_packet;
    }

    bool OpenRequest::isSent() const {
        return m_sent;
    }

    void OpenRequest::setSent(bool value) {
        m_sent = value;
    }
}

