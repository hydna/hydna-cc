#include "packet.h";
#include "channel.h";

namespace hydna {

    OpenRequest::OpenRequest(Channel* channel,
                            unsigned int ch,
                            Packet* packet) :
                            m_channel(channel),
                            m_ch(ch),
                            m_packet(packet),
                            m_sent(false) {
    }

    OpenRequest::~OpenRequest() {
        delete m_packet;
    }

    Channel* OpenRequest::getChannel() {
        return m_channel;
    }

    unsigned int OpenRequest::getChannelId() {
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

