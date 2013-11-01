#ifndef HYDNA_OPENREQUEST_H
#define HYDNA_OPENREQUEST_H

#include <iostream>
#include <map>
#include <queue>

namespace hydna {
    class Frame;
    class Channel;
    
    /**
     *  This class is used internally by both the Channel and the Connection class.
     *  A user of the library should not create an instance of this class.
     */
    class OpenRequest {
    public:
        OpenRequest(Channel* channel,
                unsigned int ch,
                const char* path,
                int path_size,
                const char* token,
                int token_size,
                Frame* frame);

        ~OpenRequest();

        Channel* getChannel();

        unsigned int getChannelId();
        
        int getTokenSize();
        int getPathSize();

        Frame& getFrame();

        bool isSent() const;
        void setSent(bool value);
        
        const char* getPath();
        const char* getToken();
        
        
    private:
        Channel* m_channel;
        
        unsigned int m_ch;
        const char* m_path; 
        int m_path_size;
        const char* m_token;
        int m_token_size;
        Frame* m_frame;
        bool m_sent;
    };
    

    typedef std::map<unsigned int, OpenRequest*> OpenRequestMap;
    // http://stackoverflow.com/questions/4157687/using-char-as-a-key-in-stdmap    
    
    // map openrequest to path
    typedef std::map<std::string, OpenRequest*> OpenRequestPathMap;
    
    typedef std::queue<OpenRequest*> OpenRequestQueue;
    
    typedef std::map<unsigned int, OpenRequestQueue*> OpenRequestQueueMap;
    
    typedef std::map<std::string, OpenRequestQueue*> OpenRequestQueuePathMap;
}

#endif

