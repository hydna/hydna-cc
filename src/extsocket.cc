#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "extsocket.h"
#include "addr.h";
#include "message.h";
#include "openrequest.h";
#include "stream.h";
#include "streamdata.h";
#include "streamerror.h"
#include "streamsignal.h";

namespace hydna {
    using namespace std;

    ExtSocket* ExtSocket::getSocket(Addr addr) {
        ExtSocket* socket;
        unsigned int zone = addr.getZone();
      
        if (availableSockets[zone]) {
            socket = availableSockets[zone];
        } else {
            socket = new ExtSocket(zone);
            availableSockets[zone] = socket;
        }

        return socket;
    }

    ExtSocket::ExtSocket(unsigned int zoneid) : m_connecting(false),
                                                m_connected(false),
                                                m_handshaked(false),
                                                m_destroying(false),
                                                m_zone(zoneid),
                                                m_streamRefCount(0) {}
    
    bool ExtSocket::hasHandshaked() const {
        return m_handshaked;
    }
    
    void ExtSocket::allocStream() {
        m_streamRefCount++;
#ifdef HYDNADEBUG
        cout << "allocStream: --> " << m_streamRefCount << endl;
#endif
    }
    
    void ExtSocket::deallocStream(Addr& addr) {  
#ifdef HYDNADEBUG
        cout << "deallocStream: - > delete stream of addr: " << addr.getStream() << endl;
#endif
        m_openStreams.erase(addr.getStream());
      
        if (--m_streamRefCount == 0) {
#ifdef HYDNADEBUG
            cout << "no more refs, destroy" << endl;
#endif
            if (!m_destroying)
                destroy(StreamError("", 0x0));
        }
    }

    bool ExtSocket::requestOpen(OpenRequest* request) {
        unsigned int streamcomp = request->getAddr().getStream();
        OpenRequestQueue* queue;

        if (m_openStreams.count(streamcomp) > 0) {
            return false;
        }

        if (m_pendingOpenRequests.count(streamcomp) > 0) {
            queue = m_openWaitQueue[streamcomp];
        
            if (!queue) {
                m_openWaitQueue[streamcomp] = queue = new OpenRequestQueue();
            } 
        
            queue->push(request);
        } else if (!m_handshaked) {
            m_pendingOpenRequests[streamcomp] = request;
            
            if (!m_connecting) {
                m_connecting = true;
                string host = request->getAddr().getHost();
                connectSocket(host, request->getAddr().getPort());
            }
        } else {
            writeBytes(request->getMessage());
            request->setSent(true);
        }
      
        return m_connected;
    }
    
    bool ExtSocket::cancelOpen(OpenRequest* request) {
        unsigned int streamcomp = request->getAddr().getStream();
        OpenRequestQueue* queue;
        OpenRequestQueue  tmp;
        bool found = false;
      
        if (request->isSent()) {
            return false;
        }
      
        queue = m_openWaitQueue[streamcomp];
      
        if (m_pendingOpenRequests.count(streamcomp) > 0) {
            delete m_pendingOpenRequests[streamcomp];
            m_pendingOpenRequests.erase(streamcomp);
        
            if (queue && queue->size() > 0)  {
                m_pendingOpenRequests[streamcomp] = queue->front();
                queue->pop();
            }
        
            return true;
        }
      
        // Should not happen...
        if (!queue) {
            return false;
        }
      
        while (!queue->empty() && !found) {
            OpenRequest* r = queue->front();
            queue->pop();
            
            if (r == request) {
                delete r;
                found = true;
            } else {
                tmp.push(r);
            }
        }

        while(!tmp.empty()) {
            OpenRequest* r = tmp.front();
            tmp.pop();
            queue->push(r);
        }
      
        return found;
    }

    void ExtSocket::connectSocket(std::string &host, int port) {
        struct hostent     *he;
        struct sockaddr_in server;

        if ((m_socketFDS = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            destroy(StreamError("Socket cound not be created"));
        } else {
            m_connected = true;

            if ((he = gethostbyname(host.c_str())) == NULL) {
                destroy(StreamError("The host \"" + host + "\" could not be resolved"));
            } else {
                int flag = 1;
                if (setsockopt(m_socketFDS, IPPROTO_TCP,
                                     TCP_NODELAY, (char *) &flag,
                                     sizeof(flag)) < 0) {
                    std::cerr << "WARNING: Could not set TCP_NODELAY" << std::endl;
                }
                            
                memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
                server.sin_family = AF_INET;
                server.sin_port = htons(port);

                if (connect(m_socketFDS, (struct sockaddr *)&server, sizeof(server)) == -1) {
                    destroy(StreamError("Could not connect to the host \"" + host + "\""));
                } else {
                    connectHandler();
                }
            }
        }
    }
    
    void ExtSocket::connectHandler() {
#ifdef HYDNADEBUG
        cout << "in connect" << endl;
#endif

        char data[8] = "DNA1";

        *(unsigned int*)&data[4] = htonl(m_zone);

        int n = write(m_socketFDS, data, 8);

        if (n == 8) {
            handshakeHandler();
        } else {
            destroy(StreamError("Could not send handshake"));
        }
    }

    void ExtSocket::handshakeHandler() {
        int responseCode = 0;
        int offset = 0;
        int n = -1;
        char data[HANDSHAKE_RESP_SIZE];
        string prefix = "DNA1";

#ifdef HYDNADEBUG
        cout << "incoming handshake response" << endl;
#endif

        while(offset < HANDSHAKE_RESP_SIZE && n != 0) {
            n = read(m_socketFDS, data + offset, HANDSHAKE_RESP_SIZE - offset);
            offset += n;
        }

        if (offset != HANDSHAKE_RESP_SIZE) {
            destroy(StreamError::fromErrorCode(0x1001));
            return;
        }

        responseCode = data[HANDSHAKE_RESP_SIZE - 1];
        data[HANDSHAKE_RESP_SIZE - 1] = '\0';

        if (prefix.compare(data) != 0) {
            destroy(StreamError::fromErrorCode(0x1001));
            return;
        }

        if (responseCode > 0) {
            destroy(StreamError::fromErrorCode(0x10 + responseCode));
        }

        m_handshaked = true;
        m_connecting = false;
#ifdef HYDNADEBUG
        cout << "bytesAvailable: " << offset << " " << responseCode << endl;
        cout << "Handshake process is now done!" << endl;
#endif

        OpenRequestMap::iterator it;
        OpenRequest* request;
        for (it = m_pendingOpenRequests.begin(); it != m_pendingOpenRequests.end(); it++) {
            request = it->second;
            writeBytes(request->getMessage());

            if (m_connected) {
                request->setSent(true);
#ifdef HYDNADEBUG
                cout << "send open request" << endl;
#endif
            } else {
                return;
            }
        }

        ListenArgs* args = new ListenArgs();
        args->extSocket = this;

        if (pthread_create(&listeningThread, NULL, listen, (void*) args) != 0) {
            destroy(StreamError("Could not create a new thread for message listening"));
            return;
        }
    }

    void* ExtSocket::listen(void *ptr) {
        ListenArgs* args;
        args = (ListenArgs*) ptr;
        ExtSocket* extSocket = args->extSocket;
        delete args;

        extSocket->receiveHandler();
        pthread_exit(NULL);
    }

    void ExtSocket::receiveHandler() {
        unsigned int size;
        unsigned int headerSize = Message::HEADER_SIZE;
        unsigned int addr;
        int op;
        int flag;

        char header[headerSize];
        char* payload;

        unsigned int offset = 0;
        int n = -1;

        for (;;) {
            while(offset < headerSize && n != 0) {
                n = read(m_socketFDS, header + offset, headerSize - offset);
                offset += n;
            }

            if (n == 0) {
                destroy(StreamError::fromErrorCode(0x100F));
                break;
            }

            size = ntohs(*(unsigned short*)&header[0]);
            payload = new char[size];

            while(offset < size && n != 0) {
                n = read(m_socketFDS, payload + offset - headerSize, size - offset);
                offset += n;
            }

            if (n == 0) {
                destroy(StreamError::fromErrorCode(0x100F));
                break;
            }

            //m_receiveBuffer.readUnsignedByte(); // Reserved
            addr = ntohl(*(unsigned int*)&header[3]);
            op   = header[7] >> 4;
            flag = header[7] & 0xf;

            switch (op) {

                case Message::OPEN:
#ifdef HYDNADEBUG
                    cout << "open response" << endl;
#endif
                    processOpenMessage(addr, flag, payload, size - headerSize);
                    break;

                case Message::DATA:
#ifdef HYDNADEBUG
                    cout << "data" << endl;
#endif
                    processDataMessage(addr, flag, payload, size - headerSize);
                    break;

                case Message::SIGNAL:
#ifdef HYDNADEBUG
                    cout << "signal" << endl;
#endif
                    processSignalMessage(addr, flag, payload, size - headerSize);
                    break;
            }

            offset = 0;
            n = -1;
            delete[] payload;
        }
    }

    void ExtSocket::processOpenMessage(unsigned int addr,
                                       int errcode,
                                       const char* payload,
                                       int size) {
        OpenRequest* request;
        Stream* stream;
        unsigned int respaddr = 0;
        request = m_pendingOpenRequests[addr];

        if (!request) {
            destroy(StreamError::fromErrorCode(0x1003));
            return;
        }

        stream = request->getStream();

        if (errcode == SUCCESS) {
            respaddr = addr;
        } else if (errcode == REDIRECT) {
            if (!payload || size < 4) {
                destroy(StreamError::fromErrorCode(0x1002));
                return;
            }

            respaddr = ntohl(*(unsigned int*)&payload[0]);
        } else {
            StreamError error("", 0x0);
            if (errcode == CUSTOM_ERR_CODE && !payload && size > 0) {
                string m(payload, size);
                error = StreamError::fromErrorCode(0x100 + errcode, m);
            } else {
                error = StreamError::fromErrorCode(0x100 + errcode);
            }

            stream->destroy(error);
        }

#ifdef HYDNADEBUG
        cout << "inaddr: " << addr << endl;
        cout << "respaddr: " << respaddr << endl;
#endif

        if (m_openStreams.count(respaddr) > 0) {
            destroy(StreamError::fromErrorCode(0x1005));
            return;
        }

        m_openStreams[respaddr] = stream;

        stream->openSuccess(Addr(request->getAddr().getZone(), respaddr));

        if (m_openWaitQueue.count(addr) > 0) {
            OpenRequestQueue* queue = m_openWaitQueue[addr];
            
            if (queue)
            {
                // Destroy all pending request IF response wasn't a 
                // redirected stream.
                if (respaddr == addr) {
                    delete m_pendingOpenRequests[addr];
                    m_pendingOpenRequests.erase(addr);

                    while (!queue->empty()) {
                        request = queue->front();
                        queue->pop();
                        StreamError error = StreamError::fromErrorCode(0x1007);
                        request->getStream()->destroy(error);
                    }

                    return;
                }

                request = queue->front();
                queue->pop();
                m_pendingOpenRequests[addr] = request;

                if (queue->empty()) {
                    delete m_openWaitQueue[addr];
                    m_openWaitQueue.erase(addr);
                }

                writeBytes(request->getMessage());
                request->setSent(true);
            }
        } else {
            delete m_pendingOpenRequests[addr];
            m_pendingOpenRequests.erase(addr);
        }
    }

    void ExtSocket::processDataMessage(unsigned int addr,
                                int priority,
                                const char* payload,
                                int size) {
        Stream* stream;
        StreamData* data;

        stream = m_openStreams[addr];

        if (!stream || !payload || size == 0) {
            destroy(StreamError::fromErrorCode(0x1004));
        }

        data = new StreamData(priority, payload, size);
        stream->addData(data);
    }

    void ExtSocket::processSignalMessage(unsigned int addr,
                                    int type,
                                    const char* payload,
                                    int size) {
        Stream* stream;
        StreamSignal* signal;        

        if (addr == 0) {
            StreamMap::iterator it = m_openStreams.begin();

            for (; it != m_openStreams.end(); it++) {
                char* payloadCopy = new char[size];
                memcpy(payloadCopy, payload, size);

                processSignalMessage(it->first, type, payloadCopy, size);
            }
        }

        stream = m_openStreams[addr];

        if (type > 0x0) {
            StreamError error(StreamError::fromErrorCode(0x20 + type));
            stream->destroy(error);
            return;
        }

        if (!stream || !payload || size == 0) {
            StreamError error(StreamError::fromErrorCode(0x1004));
            stream->destroy(error);
            return;
        }

        signal = new StreamSignal(type, payload, size);
        stream->addSignal(signal);
    }

    void ExtSocket::destroy(StreamError error) {
        m_destroying = true;
        OpenRequestMap::iterator pending;
        OpenRequestQueueMap::iterator waitqueue;
        StreamMap::iterator openstreams = m_openStreams.begin();

#ifdef HYDNADEBUG
        cout << "in destroy " << error.what() << endl;
#endif

        //if (!error) {
        //    error = StreamError::fromErrorCode(0x01);
        //}

#ifdef HYDNADEBUG
        cout << "destroy pendingOpenRequests: " << endl;
#endif
        pending = m_pendingOpenRequests.begin();
        for (; pending != m_pendingOpenRequests.end(); pending++) {
            pending->second->getStream()->destroy(error);
        }
#ifdef HYDNADEBUG
            cout << "destroy waitQueue: " << endl;
#endif

        waitqueue = m_openWaitQueue.begin();
        for (; waitqueue != m_openWaitQueue.end(); waitqueue++) {
            OpenRequestQueue* queue = waitqueue->second;

            while(!queue->empty()) {
                queue->front()->getStream()->destroy(error);
                queue->pop();
            }
        }
        
        openstreams = m_openStreams.begin();
        for (; openstreams != m_openStreams.end(); openstreams++) {
#ifdef HYDNADEBUG
            cout << "destroy stream of key: " << openstreams->first << endl;
#endif
            openstreams->second->destroy(error);
        }				

        if (m_connected) {
#ifdef HYDNADEBUG
            cout << "destroy: call close" << endl;
#endif
            close(m_socketFDS);
            m_connected = false;
            m_handshaked = false;
        }

        if (availableSockets[m_zone]) {
            delete availableSockets[m_zone];
            availableSockets.erase(m_zone);
        }

#ifdef HYDNADEBUG
        cout << "destroy: done" << endl;
#endif
        m_destroying = false;
    }

    bool ExtSocket::writeBytes(Message& message) {
        if (m_handshaked) {
            int n = -1;
            int size = message.getSize();
            char* data = message.getData();
            int offset = 0;

            while(offset < size && n != 0) {
                n = write(m_socketFDS, data + offset, size - offset);
                offset += n;
            }

            if (n <= 0) {
                destroy(StreamError("Could not write to socket"));
                return false;
            }
            return true;
        }
        return false;
    }

    SocketMap ExtSocket::availableSockets = SocketMap();
}

