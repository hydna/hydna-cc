#ifndef HYDNA_OPENREQUEST_H
#define HYDNA_OPENREQUEST_H

#include <iostream>
#include <map>
#include <queue>

namespace hydna {
    class Packet;
    class Stream;

    class OpenRequest {
    public:
        OpenRequest(Stream* stream,
                unsigned int addr,
                Packet* packet);

        ~OpenRequest();

        Stream* getStream();

        unsigned int getAddr();

        Packet& getPacket();

        bool isSent() const;

        void setSent(bool value);
        
    private:
        Stream* m_stream;
        unsigned int m_addr;
        Packet* m_packet;
        bool m_sent;

    };

    typedef std::map<unsigned int, OpenRequest*> OpenRequestMap;
    typedef std::queue<OpenRequest*> OpenRequestQueue;
    typedef std::map<unsigned int, OpenRequestQueue* > OpenRequestQueueMap;
}

#endif

