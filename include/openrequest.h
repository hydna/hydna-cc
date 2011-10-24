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
                Frame* frame);

        ~OpenRequest();

        Channel* getChannel();

        unsigned int getChannelId();

        Frame& getFrame();

        bool isSent() const;

        void setSent(bool value);
        
    private:
        Channel* m_channel;
        unsigned int m_ch;
        Frame* m_frame;
        bool m_sent;

    };

    typedef std::map<unsigned int, OpenRequest*> OpenRequestMap;
    typedef std::queue<OpenRequest*> OpenRequestQueue;
    typedef std::map<unsigned int, OpenRequestQueue* > OpenRequestQueueMap;
}

#endif

