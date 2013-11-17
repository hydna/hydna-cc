#include "frame.h";
#include "channel.h";

namespace hydna {
    
    OpenRequest::OpenRequest(Channel* channel,
                            unsigned int ch,
                            const char* path,
                            int path_size,
                            const char* token,
                            int token_size,
                            Frame* frame) :
                            m_channel(channel),
                            m_ch(ch),
                            m_path(path),
                            m_path_size(path_size),
                            m_token(token),
                            m_token_size(token_size),
                            m_frame(frame){
        
        //std::string m_path_str(path, path_size);
        
        //strcpy(m_path, path);
        
        //strcpy(m_token, token);
        
        m_sent = false;
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
    
    const char* OpenRequest::getToken() {
        return m_token;
    }
    
    int OpenRequest::getTokenSize() {
        return m_token_size;
    }
    
    const char* OpenRequest::getPath() {
        return m_path;
    }
    
    int OpenRequest::getPathSize() {
        return m_path_size;
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

