#ifndef HYDNA_OPENREQUEST_H
#define HYDNA_OPENREQUEST_H

#include <iostream>
#include <map>
#include <queue>

namespace hydna {
    class Packet;
    class Stream;
    
    /**
     *  This class is used internally by both the Stream and the ExtSocket class.
     *  A user of the library should not create an instance of this class.
     */
    class OpenRequest {
    public:
        OpenRequest(Stream* stream,
                unsigned int ch,
                Packet* packet);

        ~OpenRequest();

        Stream* getStream();

        unsigned int getChannel();

        Packet& getPacket();

        bool isSent() const;

        void setSent(bool value);
        
    private:
        Stream* m_stream;
        unsigned int m_ch;
        Packet* m_packet;
        bool m_sent;

    };

    typedef std::map<unsigned int, OpenRequest*> OpenRequestMap;
    typedef std::queue<OpenRequest*> OpenRequestQueue;
    typedef std::map<unsigned int, OpenRequestQueue* > OpenRequestQueueMap;
}

#endif

