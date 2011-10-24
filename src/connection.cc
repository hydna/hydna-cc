#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "connection.h"
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

    Connection* Connection::getConnection(string const &host, unsigned short port, string const &auth) {
        Connection* connection;
        string ports;
        stringstream out;
        out << port;
        ports = out.str();
        
        string key = host + ports + auth;
      
        pthread_mutex_lock(&m_connectionMutex);
        if (m_availableConnections[key]) {
            connection = m_availableConnections[key];
        } else {
            connection = new Connection(host, port, auth);
            m_availableConnections[key] = connection;
        }
        pthread_mutex_unlock(&m_connectionMutex);

        return connection;
    }

    Connection::Connection(string const &host, unsigned short port, string const &auth) :
                                                m_connecting(false),
                                                m_connected(false),
                                                m_handshaked(false),
                                                m_destroying(false),
                                                m_closing(false),
                                                m_listening(false),
                                                m_host(host),
                                                m_port(port),
                                                m_auth(auth),
                                                m_channelRefCount(0)
    {
        pthread_mutex_init(&m_connectionMutex, NULL);
        pthread_mutex_init(&m_channelRefMutex, NULL);
        pthread_mutex_init(&m_destroyingMutex, NULL);
        pthread_mutex_init(&m_closingMutex, NULL);
        pthread_mutex_init(&m_openChannelsMutex, NULL);
        pthread_mutex_init(&m_openWaitMutex, NULL);
        pthread_mutex_init(&m_pendingMutex, NULL);
        pthread_mutex_init(&m_listeningMutex, NULL);
    }

    Connection::~Connection() {
        pthread_mutex_destroy(&m_connectionMutex);
        pthread_mutex_destroy(&m_channelRefMutex);
        pthread_mutex_destroy(&m_destroyingMutex);
        pthread_mutex_destroy(&m_closingMutex);
        pthread_mutex_destroy(&m_openChannelsMutex);
        pthread_mutex_destroy(&m_openWaitMutex);
        pthread_mutex_destroy(&m_pendingMutex);
        pthread_mutex_destroy(&m_listeningMutex);
    }
    
    bool Connection::hasHandshaked() const {
        return m_handshaked;
    }
    
    void Connection::allocChannel() {
        pthread_mutex_lock(&m_channelRefMutex);
        m_channelRefCount++;
        pthread_mutex_unlock(&m_channelRefMutex);
#ifdef HYDNADEBUG
        ostringstream oss;
        oss << m_channelRefCount;
    
        debugPrint("Connection", 0, "Allocating a new channel, channel ref count is " + oss.str());
#endif
    }
    
    void Connection::deallocChannel(unsigned int ch) {  
#ifdef HYDNADEBUG
        debugPrint("Connection", ch, "Deallocating a channel");
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

            debugPrint("Connection", ch, "Size of openSteams is now " + oss.str());
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

    void Connection::checkRefCount() {
        pthread_mutex_lock(&m_channelRefMutex);
        if (m_channelRefCount == 0) {
            pthread_mutex_unlock(&m_channelRefMutex);
#ifdef HYDNADEBUG
            debugPrint("Connection", 0, "No more refs, destroy connection");
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

    bool Connection::requestOpen(OpenRequest* request) {
        unsigned int chcomp = request->getChannelId();
        OpenRequestQueue* queue;

#ifdef HYDNADEBUG
        debugPrint("Connection", chcomp, "A channel is trying to send a new open request");
#endif

        pthread_mutex_lock(&m_openChannelsMutex);
        if (m_openChannels.count(chcomp) > 0) {
            pthread_mutex_unlock(&m_openChannelsMutex);
#ifdef HYDNADEBUG
            debugPrint("Connection", chcomp, "The channel was already open, cancel the open request");
#endif
            delete request;
            return false;
        }
        pthread_mutex_unlock(&m_openChannelsMutex);

        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(chcomp) > 0) {
            pthread_mutex_unlock(&m_pendingMutex);

#ifdef HYDNADEBUG
            debugPrint("Connection", chcomp, "A open request is waiting to be sent, queue up the new open request");
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
            debugPrint("Connection", chcomp, "No connection, queue up the new open request");
#endif
            m_pendingOpenRequests[chcomp] = request;
            pthread_mutex_unlock(&m_pendingMutex);
            
            if (!m_connecting) {
                m_connecting = true;
                connectConnection(m_host, m_port, m_auth);
            }
        } else {
            m_pendingOpenRequests[chcomp] = request;
            pthread_mutex_unlock(&m_pendingMutex);

#ifdef HYDNADEBUG
            debugPrint("Connection", chcomp, "Already connected, sending the new open request");
#endif

            writeBytes(request->getPacket());
            request->setSent(true);
        }
      
        return m_connected;
    }
    
    bool Connection::cancelOpen(OpenRequest* request) {
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

    void Connection::connectConnection(std::string const &host, int port, std::string const &auth) {
        struct hostent     *he;
        struct sockaddr_in server;

#ifdef HYDNADEBUG
        debugPrint("Connection", 0, "Connecting...");
#endif

        if ((m_connectionFDS = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            destroy(ChannelError("Connection could not be created"));
        } else {
            m_connected = true;

            if ((he = gethostbyname(host.c_str())) == NULL) {
                destroy(ChannelError("The host \"" + host + "\" could not be resolved"));
            } else {
                int flag = 1;
                if (setsockopt(m_connectionFDS, IPPROTO_TCP,
                                     TCP_NODELAY, (char *) &flag,
                                     sizeof(flag)) < 0) {
                    cerr << "WARNING: Could not set TCP_NODELAY" << endl;
                }
                            
                memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
                server.sin_family = AF_INET;
                server.sin_port = htons(port);

                if (connect(m_connectionFDS, (struct sockaddr *)&server, sizeof(server)) == -1) {
                    destroy(ChannelError("Could not connect to the host \"" + host + "\""));
                } else {
#ifdef HYDNADEBUG
                    debugPrint("Connection", 0, "Connected, sending HTTP upgrade request");
#endif
                    connectHandler(auth);
                }
            }
        }
    }
    
    void Connection::connectHandler(string const &auth) {
        const char *data;
        unsigned int length;
        int n = -1;
        unsigned int offset = 0;

        string request = "GET /" + auth + " HTTP/1.1\n"
                         "Connection: upgrade\n"
                         "Upgrade: winksock/1\n"
                         "Host: " + m_host;

        // Redirects are not supported yet
        if (true) {
            request += "\nX-Follow-Redirects: no";
        }

        // End of upgrade request
        request += "\n\n";

        data = request.data();
        length = request.size();

        while(offset < length && n != 0) {
            n = write(m_connectionFDS, data + offset, length - offset);
            offset += n;
        }

        if (n <= 0) {
            destroy(ChannelError("Could not send upgrade request"));
        } else {
            handshakeHandler();
        }
    }

    void Connection::handshakeHandler() {
#ifdef HYDNADEBUG
        debugPrint("Connection", 0, "Incoming upgrade reponse");
#endif

        char lf = '\n';
        char cr = '\r';
        bool fieldsLeft = true;
        bool gotResponse = false;

        while(fieldsLeft) {
            string line;
            char c = ' ';

            while(c != lf) {
                read(m_connectionFDS, &c, 1);

                if (c != lf && c != cr) {
                    line.append(1, c);
                } else if (line.length() == 0) {
                    fieldsLeft = false;
                }
            }

            if (fieldsLeft) {
                // First line is a response, all others are fields
                if (!gotResponse) {
                    int code = 0;
                    size_t pos1, pos2;

                    // Take the response code from "HTTP/1.1 101 Switching Protocols"
                    pos1 = line.find(" ");
                    if (pos1 != string::npos) {
                        pos2 = line.find(" ", pos1 + 1);

                        if (pos2 != string::npos) {
                            istringstream iss(line.substr(pos1 + 1, pos2 - (pos1 + 1)));

                            if ((iss >> code).fail()) {
                                destroy(ChannelError("Could not read the status from the response \"" + line + "\""));
                            }
                        }
                    }

                    switch (code) {
                        case 101:
                            // Everything is ok, continue.
                            break;
                        case 301:
                        case 302:
                        case 307:
                            destroy(ChannelError("HTTP redirect is not supported yet"));
                            return;
                        default:
                            ostringstream oss;
                            oss << code;

                            destroy(ChannelError("Server responded with bad HTTP response code, " + oss.str()));
                            return;
                    }

                    gotResponse = true;
                } else {
                    std::transform(line.begin(), line.end(), line.begin(), ::tolower);
                    size_t pos;

                    pos = line.find("upgrade: ");
                    if (pos != string::npos) {
                        string header = line.substr(9);
                        if (header != "winksock/1") {
                            destroy(ChannelError("Bad protocol version: " + header));
                            return;
                        }
                    }
                }
            }
        }

        m_handshaked = true;
        m_connecting = false;

#ifdef HYDNADEBUG
        debugPrint("Connection", 0, "Handshake done on connection");
#endif

        OpenRequestMap::iterator it;
        OpenRequest* request;
        for (it = m_pendingOpenRequests.begin(); it != m_pendingOpenRequests.end(); it++) {
            request = it->second;
            writeBytes(request->getPacket());

            if (m_connected) {
                request->setSent(true);
#ifdef HYDNADEBUG
                debugPrint("Connection", request->getChannelId(), "Open request sent");
#endif
            } else {
                return;
            }
        }

        ListenArgs* args = new ListenArgs();
        args->extConnection = this;

#ifdef HYDNADEBUG
        debugPrint("Connection", 0, "Creating a new thread for packet listening");
#endif

        if (pthread_create(&listeningThread, NULL, listen, (void*) args) != 0) {
            destroy(ChannelError("Could not create a new thread for packet listening"));
            return;
        }
    }

    void* Connection::listen(void *ptr) {
        ListenArgs* args;
        args = (ListenArgs*) ptr;
        Connection* extConnection = args->extConnection;
        delete args;

        extConnection->receiveHandler();
        pthread_exit(NULL);
    }

    void Connection::receiveHandler() {
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
                n = read(m_connectionFDS, header + offset, headerSize - offset);
                offset += n;
            }

            if (n <= 0) {
                pthread_mutex_lock(&m_listeningMutex);
                if (m_listening) {
                    pthread_mutex_unlock(&m_listeningMutex);
                    destroy(ChannelError("Could not read from the connection"));
                } else {
                    pthread_mutex_unlock(&m_listeningMutex);
                }
                break;
            }

            size = ntohs(*(unsigned short*)&header[0]);
            payload = new char[size - headerSize];

            while(offset < size && n > 0) {
                n = read(m_connectionFDS, payload + offset - headerSize, size - offset);
                offset += n;
            }

            if (n <= 0) {
                pthread_mutex_lock(&m_listeningMutex);
                if (m_listening) {
                    pthread_mutex_unlock(&m_listeningMutex);
                    destroy(ChannelError("Could not read from the connection"));
                } else {
                    pthread_mutex_unlock(&m_listeningMutex);
                }
                break;
            }

            ch = ntohl(*(unsigned int*)&header[2]);
            op   = header[6] >> 3 & 3;
            flag = header[6] & 7;

            switch (op) {

                case Packet::OPEN:
#ifdef HYDNADEBUG
                    debugPrint("Connection", ch, "Received open response");
#endif
                    processOpenPacket(ch, flag, payload, size - headerSize);
                    break;

                case Packet::DATA:
#ifdef HYDNADEBUG
                    debugPrint("Connection", ch, "Received data");
#endif
                    processDataPacket(ch, flag, payload, size - headerSize);
                    break;

                case Packet::SIGNAL:
#ifdef HYDNADEBUG
                    debugPrint("Connection", ch, "Received signal");
#endif
                    processSignalPacket(ch, flag, payload, size - headerSize);
                    break;
            }

            offset = 0;
            n = 1;
        }
#ifdef HYDNADEBUG
        debugPrint("Connection", 0, "Listening thread exited");
#endif
    }

    void Connection::processOpenPacket(unsigned int ch,
                                       int errcode,
                                       const char* payload,
                                       int size) {
        OpenRequest* request = NULL;
        Channel* channel;
        unsigned int respch = 0;
        string message = "";
        
        pthread_mutex_lock(&m_pendingMutex);
        if (m_pendingOpenRequests.count(ch) > 0)
            request = m_pendingOpenRequests[ch];
        pthread_mutex_unlock(&m_pendingMutex);

        if (!request) {
            destroy(ChannelError("The server sent an invalid open packet"));
            return;
        }

        channel = request->getChannel();

        if (errcode == Packet::OPEN_ALLOW) {
            respch = ch;

            if (payload && size > 0) {
                message = string(payload, size);
            }
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

            debugPrint("Connection",     ch, "Redirected from " + oss.str());
            debugPrint("Connection", respch, "             to " + oss2.str());
#endif

            if (payload && size > 4) {
                message = string(payload + 4, size);
            }
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
            debugPrint("Connection", ch, "The server rejected the open request, errorcode " + oss.str());
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

        debugPrint("Connection", respch, "A new channel was added");
        debugPrint("Connection", respch, "The size of openChannels is now " + oss.str());
#endif
        pthread_mutex_unlock(&m_openChannelsMutex);

        channel->openSuccess(respch, message);

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

    void Connection::processDataPacket(unsigned int ch,
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


    bool Connection::processSignalPacket(Channel* channel,
                                    int flag,
                                    const char* payload,
                                    int size)
    {
        ChannelSignal* signal;

        if (flag != Packet::SIG_EMIT) {
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

    void Connection::processSignalPacket(unsigned int ch,
                                    int flag,
                                    const char* payload,
                                    int size)
    {
        if (ch == 0) {
            pthread_mutex_lock(&m_openChannelsMutex);
            ChannelMap::iterator it = m_openChannels.begin();
            bool destroying = false;

            if (flag != Packet::SIG_EMIT || !payload || size == 0) {
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

            if (flag != Packet::SIG_EMIT && !channel->isClosing()) {
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

    void Connection::destroy(ChannelError error) {
        pthread_mutex_lock(&m_destroyingMutex);
        m_destroying = true;
        pthread_mutex_unlock(&m_destroyingMutex);

        OpenRequestMap::iterator pending;
        OpenRequestQueueMap::iterator waitqueue;
        ChannelMap::iterator openchannels;

#ifdef HYDNADEBUG
        debugPrint("Connection", 0, "Destroying connection because: " + string(error.what()));
#endif

        pthread_mutex_lock(&m_pendingMutex);
#ifdef HYDNADEBUG
        ostringstream oss;
        oss << m_pendingOpenRequests.size();
        debugPrint("Connection", 0, "Destroying pendingOpenRequests of size " + oss.str());
#endif
        pending = m_pendingOpenRequests.begin();
        for (; pending != m_pendingOpenRequests.end(); pending++) {
#ifdef HYDNADEBUG
        debugPrint("Connection", pending->first, "Destroying channel");
#endif
            pending->second->getChannel()->destroy(error);
        }
        m_pendingOpenRequests.clear();
        pthread_mutex_unlock(&m_pendingMutex);

        pthread_mutex_lock(&m_openWaitMutex);
#ifdef HYDNADEBUG
        ostringstream oss2;
        oss2 << m_openWaitQueue.size();
        debugPrint("Connection", 0, "Destroying waitQueue of size " + oss2.str());
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
        debugPrint("Connection", 0, "Destroying openChannels of size " + oss3.str());
#endif
        openchannels = m_openChannels.begin();
        for (; openchannels != m_openChannels.end(); openchannels++) {
#ifdef HYDNADEBUG
            debugPrint("Connection", openchannels->first, "Destroying channel");
#endif
            openchannels->second->destroy(error);
        }				
        m_openChannels.clear();
        pthread_mutex_unlock(&m_openChannelsMutex);

        if (m_connected) {
#ifdef HYDNADEBUG
            debugPrint("Connection", 0, "Closing connection");
#endif
            pthread_mutex_lock(&m_listeningMutex);
            m_listening = false;
            pthread_mutex_unlock(&m_listeningMutex);
            close(m_connectionFDS);
            m_connected = false;
            m_handshaked = false;
        }
        
        string ports;
        stringstream out;
        out << m_port;
        ports = out.str();
        
        string key = m_host + ports + m_auth;

        pthread_mutex_lock(&m_connectionMutex);
        if (m_availableConnections[key]) {
            delete m_availableConnections[key];
            m_availableConnections.erase(key);
        }
        pthread_mutex_unlock(&m_connectionMutex);

#ifdef HYDNADEBUG
        debugPrint("Connection", 0, "Destroying connection done");
#endif
        
        pthread_mutex_unlock(&m_destroyingMutex);
        m_destroying = false;
        pthread_mutex_unlock(&m_destroyingMutex);
    }

    bool Connection::writeBytes(Packet& packet) {
        if (m_handshaked) {
            int n = -1;
            int size = packet.getSize();
            char* data = packet.getData();
            int offset = 0;

            while(offset < size && n != 0) {
                n = write(m_connectionFDS, data + offset, size - offset);
                offset += n;
            }

            if (n <= 0) {
                destroy(ChannelError("Could not write to the connection"));
                return false;
            }
            return true;
        }
        return false;
    }

    ConnectionMap Connection::m_availableConnections = ConnectionMap();
    pthread_mutex_t Connection::m_connectionMutex;
}

