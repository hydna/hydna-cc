#include "frame.h";
#include "channel.h";

namespace hydna {

    OpenRequest::OpenRequest(Channel* channel,
                            unsigned int ch,
                            Frame* frame) :
                            m_channel(channel),
                            m_ch(ch),
                            m_frame(frame),
                            m_sent(false) {
    }

    OpenRequest::~OpenRequest() {
        delete m_frame;
    }

    Channel* OpenRequest::getChannel() {
        return m_channel;
    }

    unsigned int OpenRequest::getChannelId() {
        return m_ch;
    }

    Frame& OpenRequest::getFrame() {
        return *m_frame;
    }

    bool OpenRequest::isSent() const {
        return m_sent;
    }

    void OpenRequest::setSent(bool value) {
        m_sent = value;
    }
}

