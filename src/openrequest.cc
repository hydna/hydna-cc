#include "packet.h";
#include "stream.h";

namespace hydna {

    OpenRequest::OpenRequest(Stream* stream,
                            unsigned int ch,
                            Packet* packet) :
                            m_stream(stream),
                            m_ch(ch),
                            m_packet(packet),
                            m_sent(false) {
    }

    OpenRequest::~OpenRequest() {
        delete m_packet;
    }

    Stream* OpenRequest::getStream() {
        return m_stream;
    }

    unsigned int OpenRequest::getChannel() {
        return m_ch;
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

