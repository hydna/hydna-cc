#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "extsocket.h"
#include "message.h";
#include "openrequest.h";
#include "stream.h";
#include "streamdata.h";
#include "streamerror.h"
#include "streamsignal.h";

namespace hydna {
    using namespace std;

    ExtSocket* ExtSocket::getSocket(string const &host, unsigned short port) {
        ExtSocket* socket;
        string ports;
        stringstream out;
        out << port;
        ports = out.str();
        
        string key = host + ports;
      
        pthread_mutex_lock(&m_socketMutex);
        if (m_availableSockets[key]) {
            socket = m_availableSockets[key];
        } else {
            socket = new ExtSocket(host, port);
            m_availableSockets[key] = socket;
        }
        pthread_mutex_unlock(&m_socketMutex);

        return socket;
    }

    ExtSocket::ExtSocket(string const &host, unsigned short port) :
                                                m_connecting(false),
                                                m_connected(false),
                                                m_handshaked(false),
                                                m_destroying(false),
                                                m_closing(false),
                                                m_host(host),
                                                m_port(port),
                                                m_streamRefCount(0)
    {
        pthread_mutex_init(&m_socketMutex, NULL);
        pthread_mutex_init(&m_streamRefMutex, NULL);
        pthread_mutex_init(&m_destroyingMutex, NULL);
        pthread_mutex_init(&m_closingMutex, NULL);
        pthread_mutex_init(&m_openStreamsMutex, NULL);
        pthread_mutex_init(&m_openWaitMutex, NULL);
        pthread_mutex_init(&m_pendingMutex, NULL);
    }

    ExtSocket::~ExtSocket() {
        pthread_mutex_destroy(&m_socketMutex);
        pthread_mutex_destroy(&m_streamRefMutex);
        pthread_mutex_destroy(&m_destroyingMutex);
        pthread_mutex_destroy(&m_closingMutex);
        pthread_mutex_destroy(&m_openStreamsMutex);
        pthread_mutex_destroy(&m_openWaitMutex);
        pthread_mutex_destroy(&m_pendingMutex);
    }
    
    bool ExtSocket::hasHandshaked() const {
        return m_handshaked;
    }
    
    void ExtSocket::allocStream() {
        pthread_mutex_lock(&m_streamRefMutex);
        m_streamRefCount++;
        pthread_mutex_unlock(&m_streamRefMutex);
#ifdef HYDNADEBUG
        cout << "allocStream: --> " << m_streamRefCount << endl;
#endif
    }
    
    void ExtSocket::deallocStream(unsigned int addr) {  
#ifdef HYDNADEBUG
        cout << "deallocStream: - > delete stream of addr: " << addr << endl;
#endif
        pthread_mutex_lock(&m_destroyingMutex);
        pthread_mutex_lock(&m_closingMutex);
        if (!m_destroying && !m_closing) {
            pthread_mutex_lock(&m_closingMutex);
            pthread_mutex_unlock(&m_destroyingMutex);

            pthread_mutex_lock(&m_openStreamsMutex);
            m_openStreams.erase(addr);
            pthread_mutex_unlock(&m_openStreamsMutex);
        } else  {
            pthread_mutex_lock(&m_closingMutex);
            pthread_mutex_unlock(&m_destroyingMutex);
        }
      
        pthread_mutex_lock(&m_streamRefMutex);
        --m_streamRefCount;
        pthread_mutex_unlock(&m_streamRefMutex);

        checkRefCount();
    }

    void ExtSocket::checkRefCount() {
        pthread_mutex_lock(&m_streamRefMutex);
        if (m_streamRefCount == 0) {
            pthread_mutex_unlock(&m_streamRefMutex);
#ifdef HYDNADEBUG
            cout << "no more refs, destroy" << endl;
#endif
            pthread_mutex_lock(&m_destroyingMutex);
            pthread_mutex_lock(&m_closingMutex);
            if (!m_destroying && !m_closing) {
                pthread_mutex_lock(&m_closingMutex);
                pthread_mutex_unlock(&m_destroyingMutex);
                destroy(StreamError("", 0x0));
            } else {
                pthread_mutex_lock(&m_closingMutex);
                pthread_mutex_unlock(&m_destroyingMutex);
            }
        } else {
            pthread_mutex_unlock(&m_streamRefMutex);
        }
    }

    bool ExtSocket::requestOpen(OpenRequest* request) {
        unsigned int streamcomp = request->getAddr();
        OpenRequestQueue* queue;

        pthread_mutex_lock(&m_openStreamsMutex);
        if (m_openStreams.count(streamcomp) > 0) {
            pthread_mutex_unlock(&m_openStreamsMutex);
            delete request;
            return false;
        }
        pthread_mutex_unlock(&m_openStreamsMutex);

        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(streamcomp) > 0) {
            pthread_mutex_unlock(&m_pendingMutex);
            
            pthread_mutex_lock(&m_openWaitMutex);
            queue = m_openWaitQueue[streamcomp];
        
            if (!queue) {
                m_openWaitQueue[streamcomp] = queue = new OpenRequestQueue();
            } 
        
            queue->push(request);
            pthread_mutex_unlock(&m_openWaitMutex);
        } else if (!m_handshaked) {
            m_pendingOpenRequests[streamcomp] = request;
            pthread_mutex_unlock(&m_pendingMutex);
            
            if (!m_connecting) {
                m_connecting = true;
                connectSocket(m_host, m_port);
            }
        } else {
            pthread_mutex_unlock(&m_pendingMutex);

            writeBytes(request->getMessage());
            request->setSent(true);
        }
      
        return m_connected;
    }
    
    bool ExtSocket::cancelOpen(OpenRequest* request) {
        unsigned int streamcomp = request->getAddr();
        OpenRequestQueue* queue;
        OpenRequestQueue  tmp;
        bool found = false;
      
        if (request->isSent()) {
            return false;
        }
      
        pthread_mutex_lock(&m_openWaitMutex);
        queue = m_openWaitQueue[streamcomp];
      
        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(streamcomp) > 0) {
            delete m_pendingOpenRequests[streamcomp];
            m_pendingOpenRequests.erase(streamcomp);
        
            if (queue && queue->size() > 0)  {
                m_pendingOpenRequests[streamcomp] = queue->front();
                queue->pop();
            }

            pthread_mutex_unlock(&m_pendingMutex);
            pthread_mutex_unlock(&m_openWaitMutex);
            return true;
        }
        pthread_mutex_unlock(&m_pendingMutex);
      
        // Should not happen...
        if (!queue) {
            pthread_mutex_unlock(&m_openWaitMutex);
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
        pthread_mutex_unlock(&m_openWaitMutex);
      
        return found;
    }

    void ExtSocket::connectSocket(std::string &host, int port) {
        struct hostent     *he;
        struct sockaddr_in server;

        if ((m_socketFDS = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            destroy(StreamError("Socket could not be created"));
        } else {
            m_connected = true;

            if ((he = gethostbyname(host.c_str())) == NULL) {
                destroy(StreamError("The host \"" + host + "\" could not be resolved"));
            } else {
                int flag = 1;
                if (setsockopt(m_socketFDS, IPPROTO_TCP,
                                     TCP_NODELAY, (char *) &flag,
                                     sizeof(flag)) < 0) {
                    cerr << "WARNING: Could not set TCP_NODELAY" << endl;
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
        
        unsigned int length = m_host.size();
        unsigned int totalLength = 4 + 1 + length;
        int n = -1;
        unsigned int offset = 0;

        if (length < 256) {
            char data[totalLength];
            data[0] = 'D';
            data[1] = 'N';
            data[2] = 'A';
            data[3] = '1';
            data[4] = length;

            for (unsigned int i = 0; i < length; i++) {
                data[5 + i] = m_host[i];
            }

            while(offset < totalLength && n != 0) {
                n = write(m_socketFDS, data + offset, totalLength - offset);
                offset += n;
            }
        }

        if (n <= 0) {
            destroy(StreamError("Could not send handshake"));
        } else {
            handshakeHandler();
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

            // header[2]; Reserved
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
        }
    }

    void ExtSocket::processOpenMessage(unsigned int addr,
                                       int errcode,
                                       const char* payload,
                                       int size) {
        OpenRequest* request;
        Stream* stream;
        unsigned int respaddr = 0;
        
        pthread_mutex_lock(&m_pendingMutex);
        request = m_pendingOpenRequests[addr];
        pthread_mutex_unlock(&m_pendingMutex);

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

        pthread_mutex_lock(&m_openStreamsMutex);
        if (m_openStreams.count(respaddr) > 0) {
            pthread_mutex_unlock(&m_openStreamsMutex);
            destroy(StreamError::fromErrorCode(0x1005));
            return;
        }

        m_openStreams[respaddr] = stream;
        pthread_mutex_unlock(&m_openStreamsMutex);

        stream->openSuccess(respaddr);

        pthread_mutex_lock(&m_openWaitMutex);
        pthread_mutex_lock(&m_pendingMutex);
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
        pthread_mutex_unlock(&m_pendingMutex);
        pthread_mutex_unlock(&m_openWaitMutex);
    }

    void ExtSocket::processDataMessage(unsigned int addr,
                                int priority,
                                const char* payload,
                                int size) {
        Stream* stream;
        StreamData* data;
        
        pthread_mutex_lock(&m_openStreamsMutex);
        stream = m_openStreams[addr];
        pthread_mutex_unlock(&m_openStreamsMutex);

        if (!stream || !payload || size == 0) {
            destroy(StreamError::fromErrorCode(0x1004));
        }

        data = new StreamData(priority, payload, size);
        stream->addData(data);
    }


    bool ExtSocket::processSignalMessage(Stream* stream,
                                    int type,
                                    const char* payload,
                                    int size)
    {
        StreamSignal* signal;

        if (type > 0x0) {
            StreamError error(StreamError::fromErrorCode(0x20 + type));
            stream->destroy(error);
            return false;
        }

        if (!stream || !payload || size == 0) {
            StreamError error(StreamError::fromErrorCode(0x1004));
            stream->destroy(error);
            return false;
        }

        signal = new StreamSignal(type, payload, size);
        stream->addSignal(signal);
        return true;
    }

    void ExtSocket::processSignalMessage(unsigned int addr,
                                    int type,
                                    const char* payload,
                                    int size)
    {
        if (addr == 0) {
            pthread_mutex_lock(&m_openStreamsMutex);
            StreamMap::iterator it = m_openStreams.begin();
            bool destroying = false;

            if (type > 0x0 || !payload || size == 0) {
                destroying = true;

                pthread_mutex_lock(&m_closingMutex);
                m_closing = true;
                pthread_mutex_unlock(&m_closingMutex);
            }

            while (it != m_openStreams.end()) {
                char* payloadCopy = new char[size];
                memcpy(payloadCopy, payload, size);

                if (!destroying && !it->second) {
                    destroying = true;

                    pthread_mutex_lock(&m_closingMutex);
                    m_closing = true;
                    pthread_mutex_unlock(&m_closingMutex);
                }

                if (processSignalMessage(it->second, type, payloadCopy, size)) {
                    ++it;
                } else {
                    m_openStreams.erase(it++);
                }
            }

            pthread_mutex_unlock(&m_openStreamsMutex);

            if (destroying) {
                pthread_mutex_lock(&m_closingMutex);
                m_closing = false;
                pthread_mutex_unlock(&m_closingMutex);

                checkRefCount();
            }
        } else {
            pthread_mutex_lock(&m_openStreamsMutex);
            processSignalMessage(m_openStreams[addr], type, payload, size);
            pthread_mutex_unlock(&m_openStreamsMutex);
        }
    }

    void ExtSocket::destroy(StreamError error) {
        pthread_mutex_lock(&m_destroyingMutex);
        m_destroying = true;
        pthread_mutex_unlock(&m_destroyingMutex);

        OpenRequestMap::iterator pending;
        OpenRequestQueueMap::iterator waitqueue;
        StreamMap::iterator openstreams;

#ifdef HYDNADEBUG
        cout << "in destroy " << error.what() << endl;
#endif

#ifdef HYDNADEBUG
        cout << "destroy pendingOpenRequests: " << endl;
#endif
        pthread_mutex_lock(&m_pendingMutex);
        pending = m_pendingOpenRequests.begin();
        for (; pending != m_pendingOpenRequests.end(); pending++) {
            pending->second->getStream()->destroy(error);
        }
        m_pendingOpenRequests.clear();
        pthread_mutex_unlock(&m_pendingMutex);

#ifdef HYDNADEBUG
            cout << "destroy waitQueue: " << endl;
#endif

        pthread_mutex_lock(&m_openWaitMutex);
        waitqueue = m_openWaitQueue.begin();
        for (; waitqueue != m_openWaitQueue.end(); waitqueue++) {
            OpenRequestQueue* queue = waitqueue->second;

            while(!queue->empty()) {
                queue->front()->getStream()->destroy(error);
                queue->pop();
            }
        }
        m_openWaitQueue.clear();
        pthread_mutex_unlock(&m_openWaitMutex);
        
        pthread_mutex_lock(&m_openStreamsMutex);
        openstreams = m_openStreams.begin();
        for (; openstreams != m_openStreams.end(); openstreams++) {
#ifdef HYDNADEBUG
            cout << "destroy stream of key: " << openstreams->first << endl;
#endif
            openstreams->second->destroy(error);
        }				
        m_openStreams.clear();
        pthread_mutex_unlock(&m_openStreamsMutex);

        if (m_connected) {
#ifdef HYDNADEBUG
            cout << "destroy: call close" << endl;
#endif
            close(m_socketFDS);
            m_connected = false;
            m_handshaked = false;
        }

        pthread_mutex_lock(&m_socketMutex);
        if (m_availableSockets[m_host]) {
            delete m_availableSockets[m_host];
            m_availableSockets.erase(m_host);
        }
        pthread_mutex_unlock(&m_socketMutex);

#ifdef HYDNADEBUG
        cout << "destroy: done" << endl;
#endif
        
        pthread_mutex_unlock(&m_destroyingMutex);
        m_destroying = false;
        pthread_mutex_unlock(&m_destroyingMutex);
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

    SocketMap ExtSocket::m_availableSockets = SocketMap();
    pthread_mutex_t ExtSocket::m_socketMutex;
}

