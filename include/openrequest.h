#ifndef HYDNA_OPENREQUEST_H
#define HYDNA_OPENREQUEST_H

#include <iostream>
#include <map>
#include <queue>

namespace hydna {
    class Packet;
    class Channel;
    
    /**
     *  This class is used internally by both the Channel and the Connection class.
     *  A user of the library should not create an instance of this class.
     */
    class OpenRequest {
    public:
        OpenRequest(Channel* channel,
                unsigned int ch,
                Packet* packet);

        ~OpenRequest();

        Channel* getChannel();

        unsigned int getChannelId();

        Packet& getPacket();

        bool isSent() const;

        void setSent(bool value);
        
    private:
        Channel* m_channel;
        unsigned int m_ch;
        Packet* m_packet;
        bool m_sent;

    };

    typedef std::map<unsigned int, OpenRequest*> OpenRequestMap;
    typedef std::queue<OpenRequest*> OpenRequestQueue;
    typedef std::map<unsigned int, OpenRequestQueue* > OpenRequestQueueMap;
}

#endif

