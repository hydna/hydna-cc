#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "extsocket.h"
#include "packet.h";
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
                                                m_listening(false),
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
        pthread_mutex_init(&m_listeningMutex, NULL);
    }

    ExtSocket::~ExtSocket() {
        pthread_mutex_destroy(&m_socketMutex);
        pthread_mutex_destroy(&m_streamRefMutex);
        pthread_mutex_destroy(&m_destroyingMutex);
        pthread_mutex_destroy(&m_closingMutex);
        pthread_mutex_destroy(&m_openStreamsMutex);
        pthread_mutex_destroy(&m_openWaitMutex);
        pthread_mutex_destroy(&m_pendingMutex);
        pthread_mutex_destroy(&m_listeningMutex);
    }
    
    bool ExtSocket::hasHandshaked() const {
        return m_handshaked;
    }
    
    void ExtSocket::allocStream() {
        pthread_mutex_lock(&m_streamRefMutex);
        m_streamRefCount++;
        pthread_mutex_unlock(&m_streamRefMutex);
#ifdef HYDNADEBUG
        cout << "ExtSocket: Allocating a new stream, stream ref count is " << m_streamRefCount << endl;
#endif
    }
    
    void ExtSocket::deallocStream(unsigned int addr) {  
#ifdef HYDNADEBUG
        cout << "ExtSocket: Deallocating a stream with the addr " << addr << endl;
#endif
        pthread_mutex_lock(&m_destroyingMutex);
        pthread_mutex_lock(&m_closingMutex);
        if (!m_destroying && !m_closing) {
            pthread_mutex_unlock(&m_closingMutex);
            pthread_mutex_unlock(&m_destroyingMutex);

            pthread_mutex_lock(&m_openStreamsMutex);
            m_openStreams.erase(addr);
#ifdef HYDNADEBUG
            cout << "ExtSocket: Size of openSteams is now " << m_openStreams.size() << endl;
#endif
            pthread_mutex_unlock(&m_openStreamsMutex);
        } else  {
            pthread_mutex_unlock(&m_closingMutex);
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
            cout << "ExtSocket: No more refs, destroy socket" << endl;
#endif
            pthread_mutex_lock(&m_destroyingMutex);
            pthread_mutex_lock(&m_closingMutex);
            if (!m_destroying && !m_closing) {
                pthread_mutex_unlock(&m_closingMutex);
                pthread_mutex_unlock(&m_destroyingMutex);
                destroy(StreamError("", 0x0));
            } else {
                pthread_mutex_unlock(&m_closingMutex);
                pthread_mutex_unlock(&m_destroyingMutex);
            }
        } else {
            pthread_mutex_unlock(&m_streamRefMutex);
        }
    }

    bool ExtSocket::requestOpen(OpenRequest* request) {
        unsigned int streamcomp = request->getAddr();
        OpenRequestQueue* queue;

#ifdef HYDNADEBUG
            cout << "ExtSocket: A stream is trying to send a new open request" << endl;
#endif

        pthread_mutex_lock(&m_openStreamsMutex);
        if (m_openStreams.count(streamcomp) > 0) {
            pthread_mutex_unlock(&m_openStreamsMutex);
#ifdef HYDNADEBUG
            cout << "ExtSocket: The stream was already open, cancel the open request" << endl;
#endif
            delete request;
            return false;
        }
        pthread_mutex_unlock(&m_openStreamsMutex);

        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(streamcomp) > 0) {
            pthread_mutex_unlock(&m_pendingMutex);

#ifdef HYDNADEBUG
            cout << "ExtSocket: A open request is waiting to be sent, queue up the new open request" << endl;
#endif
            
            pthread_mutex_lock(&m_openWaitMutex);
            queue = m_openWaitQueue[streamcomp];
        
            if (!queue) {
                m_openWaitQueue[streamcomp] = queue = new OpenRequestQueue();
            } 
        
            queue->push(request);
            pthread_mutex_unlock(&m_openWaitMutex);
        } else if (!m_handshaked) {
#ifdef HYDNADEBUG
            cout << "ExtSocket: The socket was not connected, queue up the new open request" << endl;
#endif
            m_pendingOpenRequests[streamcomp] = request;
            pthread_mutex_unlock(&m_pendingMutex);
            
            if (!m_connecting) {
                m_connecting = true;
                connectSocket(m_host, m_port);
            }
        } else {
            pthread_mutex_unlock(&m_pendingMutex);

#ifdef HYDNADEBUG
            cout << "ExtSocket: The socket was already connected, sending the new open request" << endl;
#endif

            writeBytes(request->getPacket());
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

#ifdef HYDNADEBUG
        cout << "ExtSocket: Connecting socket" << endl;
#endif

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
        cout << "ExtSocket: Socket connected, sending handshake" << endl;
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
        cout << "ExtSocket: Incoming handshake response on socket" << endl;
#endif

        while(offset < HANDSHAKE_RESP_SIZE && n != 0) {
            n = read(m_socketFDS, data + offset, HANDSHAKE_RESP_SIZE - offset);
            offset += n;
        }

        if (offset != HANDSHAKE_RESP_SIZE) {
            destroy(StreamError("Server responded with bad handshake"));
            return;
        }

        responseCode = data[HANDSHAKE_RESP_SIZE - 1];
        data[HANDSHAKE_RESP_SIZE - 1] = '\0';

        if (prefix.compare(data) != 0) {
            destroy(StreamError("Server responded with bad handshake"));
            return;
        }

        if (responseCode > 0) {
            destroy(StreamError::fromHandshakeError(responseCode));
            return;
        }

        m_handshaked = true;
        m_connecting = false;

#ifdef HYDNADEBUG
        cout << "ExtSocket: Handshake done on socket" << endl;
#endif

        OpenRequestMap::iterator it;
        OpenRequest* request;
        for (it = m_pendingOpenRequests.begin(); it != m_pendingOpenRequests.end(); it++) {
            request = it->second;
            writeBytes(request->getPacket());

            if (m_connected) {
                request->setSent(true);
#ifdef HYDNADEBUG
                cout << "ExtSocket: Open request sent" << endl;
#endif
            } else {
                return;
            }
        }

        ListenArgs* args = new ListenArgs();
        args->extSocket = this;

#ifdef HYDNADEBUG
        cout << "ExtSocket: Creating a new thread for packet listening" << endl;
#endif

        if (pthread_create(&listeningThread, NULL, listen, (void*) args) != 0) {
            destroy(StreamError("Could not create a new thread for packet listening"));
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
        unsigned int headerSize = Packet::HEADER_SIZE;
        unsigned int addr;
        int op;
        int flag;

        char header[headerSize];
        char* payload;

        unsigned int offset = 0;
        int n = 1;

        pthread_mutex_lock(&m_listeningMutex);
        m_listening = true;
        pthread_mutex_unlock(&m_listeningMutex);

        for (;;) {
            while(offset < headerSize && n > 0) {
                n = read(m_socketFDS, header + offset, headerSize - offset);
                offset += n;
            }

            if (n <= 0) {
                pthread_mutex_lock(&m_listeningMutex);
                if (m_listening) {
                    pthread_mutex_unlock(&m_listeningMutex);
                    destroy(StreamError("Could not read from the socket"));
                } else {
                    pthread_mutex_unlock(&m_listeningMutex);
                }
                break;
            }

            size = ntohs(*(unsigned short*)&header[0]);
            payload = new char[size - headerSize];

            while(offset < size && n > 0) {
                n = read(m_socketFDS, payload + offset - headerSize, size - offset);
                offset += n;
            }

            if (n <= 0) {
                pthread_mutex_lock(&m_listeningMutex);
                if (m_listening) {
                    pthread_mutex_unlock(&m_listeningMutex);
                    destroy(StreamError("Could not read from the socket"));
                } else {
                    pthread_mutex_unlock(&m_listeningMutex);
                }
                break;
            }

            // header[2]; Reserved
            addr = ntohl(*(unsigned int*)&header[3]);
            op   = header[7] >> 4;
            flag = header[7] & 0xf;

            switch (op) {

                case Packet::OPEN:
#ifdef HYDNADEBUG
                    cout << "ExtSocket: Received open response" << endl;
#endif
                    processOpenPacket(addr, flag, payload, size - headerSize);
                    break;

                case Packet::DATA:
#ifdef HYDNADEBUG
                    cout << "ExtSocket: Received data" << endl;
#endif
                    processDataPacket(addr, flag, payload, size - headerSize);
                    break;

                case Packet::SIGNAL:
#ifdef HYDNADEBUG
                    cout << "ExtSocket: Received signal" << endl;
#endif
                    processSignalPacket(addr, flag, payload, size - headerSize);
                    break;
            }

            offset = 0;
            n = 1;
        }
#ifdef HYDNADEBUG
        cout << "ExtSocket: Listening thread exited" << endl;
#endif
    }

    void ExtSocket::processOpenPacket(unsigned int addr,
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
            destroy(StreamError("The server sent a invalid open packet"));
            return;
        }

        stream = request->getStream();

        if (errcode == Packet::OPEN_SUCCESS) {
            respaddr = addr;
        } else if (errcode == Packet::OPEN_REDIRECT) {
            if (!payload || size < 4) {
                destroy(StreamError("Expected redirect addr from the server"));
                return;
            }

            respaddr = ntohl(*(unsigned int*)&payload[0]);

#ifdef HYDNADEBUG
            cout << "ExtSocket: Redirected from " << addr << endl;
            cout << "ExtSocket:              to " << respaddr << endl;
#endif
        } else {
            pthread_mutex_lock(&m_pendingMutex);
            delete m_pendingOpenRequests[addr];
            m_pendingOpenRequests.erase(addr);
            pthread_mutex_unlock(&m_pendingMutex);

            string m = "";
            if (payload && size > 0) {
                m = string(payload, size);
            }

#ifdef HYDNADEBUG
            cout << "ExtSocket: The server rejected the open request, errorcode " << errcode << endl;
#endif

            StreamError error = StreamError::fromOpenError(errcode, m);
            stream->destroy(error);
            return;
        }


        pthread_mutex_lock(&m_openStreamsMutex);
        if (m_openStreams.count(respaddr) > 0) {
            pthread_mutex_unlock(&m_openStreamsMutex);
            destroy(StreamError("Server redirected to open stream"));
            return;
        }

        m_openStreams[respaddr] = stream;
#ifdef HYDNADEBUG
        cout << "ExtSocket: A new stream was added" << endl;
        cout << "ExtSocket: The size of openStreams is now " << m_openStreams.size() << endl;
#endif
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

                    StreamError error("Stream already open");

                    while (!queue->empty()) {
                        request = queue->front();
                        queue->pop();
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

                writeBytes(request->getPacket());
                request->setSent(true);
            }
        } else {
            delete m_pendingOpenRequests[addr];
            m_pendingOpenRequests.erase(addr);
        }
        pthread_mutex_unlock(&m_pendingMutex);
        pthread_mutex_unlock(&m_openWaitMutex);
    }

    void ExtSocket::processDataPacket(unsigned int addr,
                                int priority,
                                const char* payload,
                                int size) {
        Stream* stream = NULL;
        StreamData* data;
        
        pthread_mutex_lock(&m_openStreamsMutex);
        if (m_openStreams.count(addr) > 0)
            stream = m_openStreams[addr];
        pthread_mutex_unlock(&m_openStreamsMutex);

        if (!stream) {
            destroy(StreamError("No stream was available to take care of the data received"));
            return;
        }

        if (!payload || size == 0) {
            destroy(StreamError("Zero data packet received"));
            return;
        }

        data = new StreamData(priority, payload, size);
        stream->addData(data);
    }


    bool ExtSocket::processSignalPacket(Stream* stream,
                                    int flag,
                                    const char* payload,
                                    int size)
    {
        StreamSignal* signal;

        if (flag > 0x0) {
            string m = "";
            if (payload && size > 0) {
                m = string(payload, size);
            }
            StreamError error("", 0x0);
            
            if (flag != Packet::SIG_END) {
                error = StreamError::fromSigError(flag, m);
            }

            stream->destroy(error);
            return false;
        }

        if (!stream)
            return false;

        signal = new StreamSignal(flag, payload, size);
        stream->addSignal(signal);
        return true;
    }

    void ExtSocket::processSignalPacket(unsigned int addr,
                                    int flag,
                                    const char* payload,
                                    int size)
    {
        if (addr == 0) {
            pthread_mutex_lock(&m_openStreamsMutex);
            StreamMap::iterator it = m_openStreams.begin();
            bool destroying = false;

            if (flag > 0x0 || !payload || size == 0) {
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

                if (processSignalPacket(it->second, flag, payloadCopy, size)) {
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
            Stream* stream = NULL;

            if (m_openStreams.count(addr) > 0)
                stream = m_openStreams[addr];

            if (!stream) {
                pthread_mutex_unlock(&m_openStreamsMutex);
                destroy(StreamError("Packet sent to unknown stream"));
                return;
            }

            processSignalPacket(stream, flag, payload, size);
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
        cout << "ExtSocket: Destroying socket because: " << error.what() << endl;
#endif

        pthread_mutex_lock(&m_pendingMutex);
#ifdef HYDNADEBUG
        cout << "ExtSocket: Destroying pendingOpenRequests of size " << m_pendingOpenRequests.size() << endl;
#endif
        pending = m_pendingOpenRequests.begin();
        for (; pending != m_pendingOpenRequests.end(); pending++) {
            pending->second->getStream()->destroy(error);
        }
        m_pendingOpenRequests.clear();
        pthread_mutex_unlock(&m_pendingMutex);

        pthread_mutex_lock(&m_openWaitMutex);
#ifdef HYDNADEBUG
        cout << "ExtSocket: Destroying waitQueue of size " << m_openWaitQueue.size() << endl;
#endif
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
#ifdef HYDNADEBUG
        cout << "ExtSocket: Destroying openStreams of size " << m_openStreams.size() << endl;
#endif
        openstreams = m_openStreams.begin();
        for (; openstreams != m_openStreams.end(); openstreams++) {
#ifdef HYDNADEBUG
            cout << "ExtSocket: Destroying stream of key " << openstreams->first << endl;
#endif
            openstreams->second->destroy(error);
        }				
        m_openStreams.clear();
        pthread_mutex_unlock(&m_openStreamsMutex);

        if (m_connected) {
#ifdef HYDNADEBUG
            cout << "ExtSocket: Closing socket" << endl;
#endif
            pthread_mutex_lock(&m_listeningMutex);
            m_listening = false;
            pthread_mutex_unlock(&m_listeningMutex);
            close(m_socketFDS);
            m_connected = false;
            m_handshaked = false;
        }
        
        string ports;
        stringstream out;
        out << m_port;
        ports = out.str();
        
        string key = m_host + ports;

        pthread_mutex_lock(&m_socketMutex);
        if (m_availableSockets[key]) {
            delete m_availableSockets[key];
            m_availableSockets.erase(key);
        }
        pthread_mutex_unlock(&m_socketMutex);

#ifdef HYDNADEBUG
        cout << "ExtSocket: Destroying socket done" << endl;
#endif
        
        pthread_mutex_unlock(&m_destroyingMutex);
        m_destroying = false;
        pthread_mutex_unlock(&m_destroyingMutex);
    }

    bool ExtSocket::writeBytes(Packet& packet) {
        if (m_handshaked) {
            int n = -1;
            int size = packet.getSize();
            char* data = packet.getData();
            int offset = 0;

            while(offset < size && n != 0) {
                n = write(m_socketFDS, data + offset, size - offset);
                offset += n;
            }

            if (n <= 0) {
                destroy(StreamError("Could not write to the socket"));
                return false;
            }
            return true;
        }
        return false;
    }

    SocketMap ExtSocket::m_availableSockets = SocketMap();
    pthread_mutex_t ExtSocket::m_socketMutex;
}

