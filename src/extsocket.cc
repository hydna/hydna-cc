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
#include "channel.h";
#include "channeldata.h";
#include "channelerror.h"
#include "channelsignal.h";

#ifdef HYDNADEBUG
#include "debughelper.h"
#endif

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
                                                m_channelRefCount(0)
    {
        pthread_mutex_init(&m_socketMutex, NULL);
        pthread_mutex_init(&m_channelRefMutex, NULL);
        pthread_mutex_init(&m_destroyingMutex, NULL);
        pthread_mutex_init(&m_closingMutex, NULL);
        pthread_mutex_init(&m_openChannelsMutex, NULL);
        pthread_mutex_init(&m_openWaitMutex, NULL);
        pthread_mutex_init(&m_pendingMutex, NULL);
        pthread_mutex_init(&m_listeningMutex, NULL);
    }

    ExtSocket::~ExtSocket() {
        pthread_mutex_destroy(&m_socketMutex);
        pthread_mutex_destroy(&m_channelRefMutex);
        pthread_mutex_destroy(&m_destroyingMutex);
        pthread_mutex_destroy(&m_closingMutex);
        pthread_mutex_destroy(&m_openChannelsMutex);
        pthread_mutex_destroy(&m_openWaitMutex);
        pthread_mutex_destroy(&m_pendingMutex);
        pthread_mutex_destroy(&m_listeningMutex);
    }
    
    bool ExtSocket::hasHandshaked() const {
        return m_handshaked;
    }
    
    void ExtSocket::allocChannel() {
        pthread_mutex_lock(&m_channelRefMutex);
        m_channelRefCount++;
        pthread_mutex_unlock(&m_channelRefMutex);
#ifdef HYDNADEBUG
        ostringstream oss;
        oss << m_channelRefCount;
    
        debugPrint("ExtSocket", 0, "Allocating a new channel, channel ref count is " + oss.str());
#endif
    }
    
    void ExtSocket::deallocChannel(unsigned int ch) {  
#ifdef HYDNADEBUG
        debugPrint("ExtSocket", ch, "Deallocating a channel");
#endif
        pthread_mutex_lock(&m_destroyingMutex);
        pthread_mutex_lock(&m_closingMutex);
        if (!m_destroying && !m_closing) {
            pthread_mutex_unlock(&m_closingMutex);
            pthread_mutex_unlock(&m_destroyingMutex);

            pthread_mutex_lock(&m_openChannelsMutex);
            m_openChannels.erase(ch);
#ifdef HYDNADEBUG
            ostringstream oss;
            oss << m_openChannels.size();

            debugPrint("ExtSocket", ch, "Size of openSteams is now " + oss.str());
#endif
            pthread_mutex_unlock(&m_openChannelsMutex);
        } else  {
            pthread_mutex_unlock(&m_closingMutex);
            pthread_mutex_unlock(&m_destroyingMutex);
        }
      
        pthread_mutex_lock(&m_channelRefMutex);
        --m_channelRefCount;
        pthread_mutex_unlock(&m_channelRefMutex);

        checkRefCount();
    }

    void ExtSocket::checkRefCount() {
        pthread_mutex_lock(&m_channelRefMutex);
        if (m_channelRefCount == 0) {
            pthread_mutex_unlock(&m_channelRefMutex);
#ifdef HYDNADEBUG
            debugPrint("ExtSocket", 0, "No more refs, destroy socket");
#endif
            pthread_mutex_lock(&m_destroyingMutex);
            pthread_mutex_lock(&m_closingMutex);
            if (!m_destroying && !m_closing) {
                pthread_mutex_unlock(&m_closingMutex);
                pthread_mutex_unlock(&m_destroyingMutex);
                destroy(ChannelError("", 0x0));
            } else {
                pthread_mutex_unlock(&m_closingMutex);
                pthread_mutex_unlock(&m_destroyingMutex);
            }
        } else {
            pthread_mutex_unlock(&m_channelRefMutex);
        }
    }

    bool ExtSocket::requestOpen(OpenRequest* request) {
        unsigned int chcomp = request->getChannelId();
        OpenRequestQueue* queue;

#ifdef HYDNADEBUG
        debugPrint("ExtSocket", chcomp, "A channel is trying to send a new open request");
#endif

        pthread_mutex_lock(&m_openChannelsMutex);
        if (m_openChannels.count(chcomp) > 0) {
            pthread_mutex_unlock(&m_openChannelsMutex);
#ifdef HYDNADEBUG
            debugPrint("ExtSocket", chcomp, "The channel was already open, cancel the open request");
#endif
            delete request;
            return false;
        }
        pthread_mutex_unlock(&m_openChannelsMutex);

        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(chcomp) > 0) {
            pthread_mutex_unlock(&m_pendingMutex);

#ifdef HYDNADEBUG
            debugPrint("ExtSocket", chcomp, "A open request is waiting to be sent, queue up the new open request");
#endif
            
            pthread_mutex_lock(&m_openWaitMutex);
            queue = m_openWaitQueue[chcomp];
        
            if (!queue) {
                m_openWaitQueue[chcomp] = queue = new OpenRequestQueue();
            } 
        
            queue->push(request);
            pthread_mutex_unlock(&m_openWaitMutex);
        } else if (!m_handshaked) {
#ifdef HYDNADEBUG
            debugPrint("ExtSocket", chcomp, "The socket was not connected, queue up the new open request");
#endif
            m_pendingOpenRequests[chcomp] = request;
            pthread_mutex_unlock(&m_pendingMutex);
            
            if (!m_connecting) {
                m_connecting = true;
                connectSocket(m_host, m_port);
            }
        } else {
            m_pendingOpenRequests[chcomp] = request;
            pthread_mutex_unlock(&m_pendingMutex);

#ifdef HYDNADEBUG
            debugPrint("ExtSocket", chcomp, "The socket was already connected, sending the new open request");
#endif

            writeBytes(request->getPacket());
            request->setSent(true);
        }
      
        return m_connected;
    }
    
    bool ExtSocket::cancelOpen(OpenRequest* request) {
        unsigned int channelcomp = request->getChannelId();
        OpenRequestQueue* queue = NULL;
        OpenRequestQueue  tmp;
        bool found = false;
      
        if (request->isSent()) {
            return false;
        }
      
        pthread_mutex_lock(&m_openWaitMutex);
        if (m_openWaitQueue.count(channelcomp) > 0)
            queue = m_openWaitQueue[channelcomp];
      
        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(channelcomp) > 0) {
            delete m_pendingOpenRequests[channelcomp];
            m_pendingOpenRequests.erase(channelcomp);
        
            if (queue && queue->size() > 0)  {
                m_pendingOpenRequests[channelcomp] = queue->front();
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
        debugPrint("ExtSocket", 0, "Connecting socket");
#endif

        if ((m_socketFDS = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            destroy(ChannelError("Socket could not be created"));
        } else {
            m_connected = true;

            if ((he = gethostbyname(host.c_str())) == NULL) {
                destroy(ChannelError("The host \"" + host + "\" could not be resolved"));
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
                    destroy(ChannelError("Could not connect to the host \"" + host + "\""));
                } else {
                    connectHandler();
                }
            }
        }
    }
    
    void ExtSocket::connectHandler() {
#ifdef HYDNADEBUG
        debugPrint("ExtSocket", 0, "Socket connected, sending handshake");
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
            destroy(ChannelError("Could not send handshake"));
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
        debugPrint("ExtSocket", 0, "Incoming handshake response on socket");
#endif

        while(offset < HANDSHAKE_RESP_SIZE && n != 0) {
            n = read(m_socketFDS, data + offset, HANDSHAKE_RESP_SIZE - offset);
            offset += n;
        }

        if (offset != HANDSHAKE_RESP_SIZE) {
            destroy(ChannelError("Server responded with bad handshake"));
            return;
        }

        responseCode = data[HANDSHAKE_RESP_SIZE - 1];
        data[HANDSHAKE_RESP_SIZE - 1] = '\0';

        if (prefix.compare(data) != 0) {
            destroy(ChannelError("Server responded with bad handshake"));
            return;
        }

        if (responseCode > 0) {
            destroy(ChannelError::fromHandshakeError(responseCode));
            return;
        }

        m_handshaked = true;
        m_connecting = false;

#ifdef HYDNADEBUG
        debugPrint("ExtSocket", 0, "Handshake done on socket");
#endif

        OpenRequestMap::iterator it;
        OpenRequest* request;
        for (it = m_pendingOpenRequests.begin(); it != m_pendingOpenRequests.end(); it++) {
            request = it->second;
            writeBytes(request->getPacket());

            if (m_connected) {
                request->setSent(true);
#ifdef HYDNADEBUG
                debugPrint("ExtSocket", request->getChannelId(), "Open request sent");
#endif
            } else {
                return;
            }
        }

        ListenArgs* args = new ListenArgs();
        args->extSocket = this;

#ifdef HYDNADEBUG
        debugPrint("ExtSocket", 0, "Creating a new thread for packet listening");
#endif

        if (pthread_create(&listeningThread, NULL, listen, (void*) args) != 0) {
            destroy(ChannelError("Could not create a new thread for packet listening"));
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
        unsigned int ch;
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
                    destroy(ChannelError("Could not read from the socket"));
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
                    destroy(ChannelError("Could not read from the socket"));
                } else {
                    pthread_mutex_unlock(&m_listeningMutex);
                }
                break;
            }

            // header[2]; Reserved
            ch = ntohl(*(unsigned int*)&header[3]);
            op   = header[7] >> 4;
            flag = header[7] & 0xf;

            switch (op) {

                case Packet::OPEN:
#ifdef HYDNADEBUG
                    debugPrint("ExtSocket", ch, "Received open response");
#endif
                    processOpenPacket(ch, flag, payload, size - headerSize);
                    break;

                case Packet::DATA:
#ifdef HYDNADEBUG
                    debugPrint("ExtSocket", ch, "Received data");
#endif
                    processDataPacket(ch, flag, payload, size - headerSize);
                    break;

                case Packet::SIGNAL:
#ifdef HYDNADEBUG
                    debugPrint("ExtSocket", ch, "Received signal");
#endif
                    processSignalPacket(ch, flag, payload, size - headerSize);
                    break;
            }

            offset = 0;
            n = 1;
        }
#ifdef HYDNADEBUG
        debugPrint("ExtSocket", 0, "Listening thread exited");
#endif
    }

    void ExtSocket::processOpenPacket(unsigned int ch,
                                       int errcode,
                                       const char* payload,
                                       int size) {
        OpenRequest* request = NULL;
        Channel* channel;
        unsigned int respch = 0;
        
        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(ch) > 0)
            request = m_pendingOpenRequests[ch];
        pthread_mutex_unlock(&m_pendingMutex);

        if (!request) {
            destroy(ChannelError("The server sent an invalid open packet"));
            return;
        }

        channel = request->getChannel();

        if (errcode == Packet::OPEN_SUCCESS) {
            respch = ch;
        } else if (errcode == Packet::OPEN_REDIRECT) {
            if (!payload || size < 4) {
                destroy(ChannelError("Expected redirect channel from the server"));
                return;
            }

            respch = ntohl(*(unsigned int*)&payload[0]);

#ifdef HYDNADEBUG
            ostringstream oss;
            oss << ch;

            ostringstream oss2;
            oss2 << respch;

            debugPrint("ExtSocket",     ch, "Redirected from " + oss.str());
            debugPrint("ExtSocket", respch, "             to " + oss2.str());
#endif
        } else {
            pthread_mutex_lock(&m_pendingMutex);
            delete m_pendingOpenRequests[ch];
            m_pendingOpenRequests.erase(ch);
            pthread_mutex_unlock(&m_pendingMutex);

            string m = "";
            if (payload && size > 0) {
                m = string(payload, size);
            }

#ifdef HYDNADEBUG
            ostringstream oss;
            oss << errcode;
            debugPrint("ExtSocket", ch, "The server rejected the open request, errorcode " + oss.str());
#endif

            ChannelError error = ChannelError::fromOpenError(errcode, m);
            channel->destroy(error);
            return;
        }


        pthread_mutex_lock(&m_openChannelsMutex);
        if (m_openChannels.count(respch) > 0) {
            pthread_mutex_unlock(&m_openChannelsMutex);
            destroy(ChannelError("Server redirected to open channel"));
            return;
        }

        m_openChannels[respch] = channel;
#ifdef HYDNADEBUG
        ostringstream oss;
        oss << m_openChannels.size();

        debugPrint("ExtSocket", respch, "A new channel was added");
        debugPrint("ExtSocket", respch, "The size of openChannels is now " + oss.str());
#endif
        pthread_mutex_unlock(&m_openChannelsMutex);

        channel->openSuccess(respch);

        pthread_mutex_lock(&m_openWaitMutex);
        pthread_mutex_lock(&m_pendingMutex);
        if (m_openWaitQueue.count(ch) > 0) {
            OpenRequestQueue* queue = m_openWaitQueue[ch];
            
            if (queue)
            {
                // Destroy all pending request IF response wasn't a 
                // redirected channel.
                if (respch == ch) {
                    delete m_pendingOpenRequests[ch];
                    m_pendingOpenRequests.erase(ch);

                    ChannelError error("Channel already open");

                    while (!queue->empty()) {
                        request = queue->front();
                        queue->pop();
                        request->getChannel()->destroy(error);
                    }

                    return;
                }

                request = queue->front();
                queue->pop();
                m_pendingOpenRequests[ch] = request;

                if (queue->empty()) {
                    delete m_openWaitQueue[ch];
                    m_openWaitQueue.erase(ch);
                }

                writeBytes(request->getPacket());
                request->setSent(true);
            }
        } else {
            delete m_pendingOpenRequests[ch];
            m_pendingOpenRequests.erase(ch);
        }
        pthread_mutex_unlock(&m_pendingMutex);
        pthread_mutex_unlock(&m_openWaitMutex);
    }

    void ExtSocket::processDataPacket(unsigned int ch,
                                int priority,
                                const char* payload,
                                int size) {
        Channel* channel = NULL;
        ChannelData* data;
        
        pthread_mutex_lock(&m_openChannelsMutex);
        if (m_openChannels.count(ch) > 0)
            channel = m_openChannels[ch];
        pthread_mutex_unlock(&m_openChannelsMutex);

        if (!channel) {
            destroy(ChannelError("No channel was available to take care of the data received"));
            return;
        }

        if (!payload || size == 0) {
            destroy(ChannelError("Zero data packet received"));
            return;
        }

        data = new ChannelData(priority, payload, size);
        channel->addData(data);
    }


    bool ExtSocket::processSignalPacket(Channel* channel,
                                    int flag,
                                    const char* payload,
                                    int size)
    {
        ChannelSignal* signal;

        if (flag > 0x0) {
            string m = "";
            if (payload && size > 0) {
                m = string(payload, size);
            }
            ChannelError error("", 0x0);
            
            if (flag != Packet::SIG_END) {
                error = ChannelError::fromSigError(flag, m);
            }

            channel->destroy(error);
            return false;
        }

        if (!channel)
            return false;

        signal = new ChannelSignal(flag, payload, size);
        channel->addSignal(signal);
        return true;
    }

    void ExtSocket::processSignalPacket(unsigned int ch,
                                    int flag,
                                    const char* payload,
                                    int size)
    {
        if (ch == 0) {
            pthread_mutex_lock(&m_openChannelsMutex);
            ChannelMap::iterator it = m_openChannels.begin();
            bool destroying = false;

            if (flag > 0x0 || !payload || size == 0) {
                destroying = true;

                pthread_mutex_lock(&m_closingMutex);
                m_closing = true;
                pthread_mutex_unlock(&m_closingMutex);
            }

            while (it != m_openChannels.end()) {
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
                    m_openChannels.erase(it++);
                }
            }

            pthread_mutex_unlock(&m_openChannelsMutex);

            if (destroying) {
                pthread_mutex_lock(&m_closingMutex);
                m_closing = false;
                pthread_mutex_unlock(&m_closingMutex);

                checkRefCount();
            }
        } else {
            pthread_mutex_lock(&m_openChannelsMutex);
            Channel* channel = NULL;

            if (m_openChannels.count(ch) > 0)
                channel = m_openChannels[ch];

            if (!channel) {
                pthread_mutex_unlock(&m_openChannelsMutex);
                destroy(ChannelError("Received unknown channel"));
                return;
            }

            if (flag > 0x0 && !channel->isClosing()) {
                pthread_mutex_unlock(&m_openChannelsMutex);
                
                Packet packet(ch, Packet::SIGNAL, Packet::SIG_END, payload);
                try {
                    writeBytes(packet);
                } catch (ChannelError& e) {
                    delete payload;
                    destroy(e);
                }
                
                return;
            }

            processSignalPacket(channel, flag, payload, size);
            pthread_mutex_unlock(&m_openChannelsMutex);
        }
    }

    void ExtSocket::destroy(ChannelError error) {
        pthread_mutex_lock(&m_destroyingMutex);
        m_destroying = true;
        pthread_mutex_unlock(&m_destroyingMutex);

        OpenRequestMap::iterator pending;
        OpenRequestQueueMap::iterator waitqueue;
        ChannelMap::iterator openchannels;

#ifdef HYDNADEBUG
        debugPrint("ExtSocket", 0, "Destroying socket because: " + string(error.what()));
#endif

        pthread_mutex_lock(&m_pendingMutex);
#ifdef HYDNADEBUG
        ostringstream oss;
        oss << m_pendingOpenRequests.size();
        debugPrint("ExtSocket", 0, "Destroying pendingOpenRequests of size " + oss.str());
#endif
        pending = m_pendingOpenRequests.begin();
        for (; pending != m_pendingOpenRequests.end(); pending++) {
#ifdef HYDNADEBUG
        debugPrint("ExtSocket", pending->first, "Destroying channel");
#endif
            pending->second->getChannel()->destroy(error);
        }
        m_pendingOpenRequests.clear();
        pthread_mutex_unlock(&m_pendingMutex);

        pthread_mutex_lock(&m_openWaitMutex);
#ifdef HYDNADEBUG
        ostringstream oss2;
        oss2 << m_openWaitQueue.size();
        debugPrint("ExtSocket", 0, "Destroying waitQueue of size " + oss2.str());
#endif
        waitqueue = m_openWaitQueue.begin();
        for (; waitqueue != m_openWaitQueue.end(); waitqueue++) {
            OpenRequestQueue* queue = waitqueue->second;

            while(!queue->empty()) {
                queue->front()->getChannel()->destroy(error);
                queue->pop();
            }
        }
        m_openWaitQueue.clear();
        pthread_mutex_unlock(&m_openWaitMutex);
        
        pthread_mutex_lock(&m_openChannelsMutex);
#ifdef HYDNADEBUG
        ostringstream oss3;
        oss3 << m_openChannels.size();
        debugPrint("ExtSocket", 0, "Destroying openChannels of size " + oss3.str());
#endif
        openchannels = m_openChannels.begin();
        for (; openchannels != m_openChannels.end(); openchannels++) {
#ifdef HYDNADEBUG
            debugPrint("ExtSocket", openchannels->first, "Destroying channel");
#endif
            openchannels->second->destroy(error);
        }				
        m_openChannels.clear();
        pthread_mutex_unlock(&m_openChannelsMutex);

        if (m_connected) {
#ifdef HYDNADEBUG
            debugPrint("ExtSocket", 0, "Closing socket");
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
        debugPrint("ExtSocket", 0, "Destroying socket done");
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
                destroy(ChannelError("Could not write to the socket"));
                return false;
            }
            return true;
        }
        return false;
    }

    SocketMap ExtSocket::m_availableSockets = SocketMap();
    pthread_mutex_t ExtSocket::m_socketMutex;
}

