#ifndef HYDNA_OPENREQUEST_H
#define HYDNA_OPENREQUEST_H

#include <iostream>
#include <map>
#include <queue>

namespace hydna {
    class Message;
    class Stream;

    class OpenRequest {
    public:
        OpenRequest(Stream* stream,
                unsigned int addr,
                Message* message);

        ~OpenRequest();

        Stream* getStream();

        unsigned int getAddr();

        Message& getMessage();

        bool isSent() const;

        void setSent(bool value);
        
    private:
        Stream* m_stream;
        unsigned int m_addr;
        Message* m_message;
        bool m_sent;

    };

    typedef std::map<unsigned int, OpenRequest*> OpenRequestMap;
    typedef std::queue<OpenRequest*> OpenRequestQueue;
    typedef std::map<unsigned int, OpenRequestQueue* > OpenRequestQueueMap;
}

#endif

