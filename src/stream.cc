#include <pthread.h>
#include <iomanip>
#include <sstream>

#include "stream.h"
#include "packet.h";
#include "openrequest.h"
#include "streamdata.h"
#include "streamsignal.h"
#include "streammode.h"

#include "error.h"
#include "ioerror.h"
#include "rangeerror.h"
#include "streamerror.h"

namespace hydna {
    
    using namespace std;

    Stream::Stream() : m_host(""), m_port(7010), m_ch(0), m_socket(NULL), m_connected(false), m_closing(false), m_pendingClose(NULL),
                       m_readable(false), m_writable(false), m_emitable(false), m_error("", 0x0), m_openRequest(NULL)
    {
        pthread_mutex_init(&m_dataMutex, NULL);
        pthread_mutex_init(&m_signalMutex, NULL);
        pthread_mutex_init(&m_connectMutex, NULL);
    }

    Stream::~Stream() {
        pthread_mutex_destroy(&m_dataMutex);
        pthread_mutex_destroy(&m_signalMutex);
        pthread_mutex_destroy(&m_connectMutex);
    }
    
    bool Stream::isConnected() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Stream::isClosing() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_closing;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Stream::isReadable() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected && m_readable;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Stream::isWritable() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected && m_writable;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Stream::hasSignalSupport() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected && m_emitable;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    unsigned int Stream::getChannel() const {
        pthread_mutex_lock(&m_connectMutex);
        unsigned int result = m_ch;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }
    
    void Stream::connect(string const &expr,
                 unsigned int mode,
                 const char* token,
                 unsigned int tokenOffset,
                 unsigned int tokenLength)
    {
        Packet* packet;
        OpenRequest* request;
      
        pthread_mutex_lock(&m_connectMutex);
        if (m_socket) {
            pthread_mutex_unlock(&m_connectMutex);
            throw Error("Already connected");
        }
        pthread_mutex_unlock(&m_connectMutex);

        if (mode == 0x04 ||
                mode < StreamMode::READ || 
                mode > StreamMode::READWRITEEMIT) {
            throw Error("Invalid stream mode");
        }
      
        m_mode = mode;
      
        m_readable = ((m_mode & StreamMode::READ) == StreamMode::READ);
        m_writable = ((m_mode & StreamMode::WRITE) == StreamMode::WRITE);
        m_emitable = ((m_mode & StreamMode::EMIT) == StreamMode::EMIT);

        string host = expr;
        unsigned short port = 7010;
        unsigned int ch = 1;
        string tokens = "";
        size_t pos;

        pos = host.find_last_of("?");
        if (pos != string::npos) {
            tokens = host.substr(pos + 1);
            host = host.substr(0, pos);
        }

        pos = host.find("/x");
        if (pos != string::npos) {
            istringstream iss(host.substr(pos + 2));
            host = host.substr(0, pos);

            if ((iss >> setbase(16) >> ch).fail()) {
               throw Error("Could not read the channel \"" + host.substr(pos + 2) + "\""); 
            }
        } else {
            pos = host.find_last_of("/");
            if (pos != string::npos) {
                istringstream iss(host.substr(pos + 1));
                host = host.substr(0, pos);

                if ((iss >> ch).fail()) {
                   throw Error("Could not read the channel \"" + host.substr(pos + 1) + "\""); 
                }
            }
        }

        pos = host.find_last_of(":");
        if (pos != string::npos) {
            istringstream iss(host.substr(pos + 1));
            host = host.substr(0, pos);

            if ((iss >> port).fail()) {
               throw Error("Could not read the port \"" + host.substr(pos + 1) + "\""); 
            }
        }
        
        m_host = host;
        m_port = port;
        m_ch = ch;

        m_socket = ExtSocket::getSocket(m_host, m_port);
      
        // Ref count
        m_socket->allocStream();

        if (token || tokens == "") {
            packet = new Packet(m_ch, Packet::OPEN, mode,
                                token, tokenOffset, tokenLength);
        } else {
            packet = new Packet(m_ch, Packet::OPEN, mode,
                                tokens.c_str(), 0, tokens.size());
        }
      
        request = new OpenRequest(this, m_ch, packet);

        m_error = StreamError("", 0x0);
      
        if (!m_socket->requestOpen(request)) {
            checkForStreamError();
            throw Error("Stream already open");
        }

        m_openRequest = request;
    }
    
    void Stream::writeBytes(const char* data,
                            unsigned int offset,
                            unsigned int length,
                            unsigned int priority)
    {
        bool result;

        pthread_mutex_lock(&m_connectMutex);
        if (!m_connected || !m_socket) {
            pthread_mutex_unlock(&m_connectMutex);
            checkForStreamError();
            throw IOError("Stream is not connected");
        }
        pthread_mutex_unlock(&m_connectMutex);

        if (!m_writable) {
            throw Error("Stream is not writable");
        }
      
        if (priority > 3 || priority == 0) {
            throw RangeError("Priority must be between 1 - 3");
        }

        Packet packet(m_ch, Packet::DATA, priority,
                                data, offset, length);
      
        pthread_mutex_lock(&m_connectMutex);
        ExtSocket* socket = m_socket;
        pthread_mutex_unlock(&m_connectMutex);
        result = socket->writeBytes(packet);

        if (!result)
            checkForStreamError();
    }

    void Stream::writeString(string const &value) {
        writeBytes(value.data(), 0, value.length());
    }
    
    void Stream::emitBytes(const char* data,
                            unsigned int offset,
                            unsigned int length,
                            unsigned int type)
    {
        bool result;

        pthread_mutex_lock(&m_connectMutex);
        if (!m_connected || !m_socket) {
            pthread_mutex_unlock(&m_connectMutex);
            checkForStreamError();
            throw IOError("Stream is not connected.");
        }
        pthread_mutex_unlock(&m_connectMutex);

        if (!m_emitable) {
            throw Error("You do not have permission to send signals");
        }

        Packet packet(m_ch, Packet::SIGNAL, type,
                            data, offset, length);

        pthread_mutex_lock(&m_connectMutex);
        ExtSocket* socket = m_socket;
        pthread_mutex_unlock(&m_connectMutex);
        result = socket->writeBytes(packet);

        if (!result)
            checkForStreamError();
    }

    void Stream::emitString(string const &value, int type) {
        emitBytes(value.data(), 0, value.length(), type);
    }

    void Stream::close() {
        Packet* packet;

        pthread_mutex_lock(&m_connectMutex);
        if (!m_socket || m_closing) {
            pthread_mutex_unlock(&m_connectMutex);
            return;
        }

        m_closing = true;
        m_readable = false;
        m_writable = false;
        m_emitable = false;

        if (m_openRequest && m_socket->cancelOpen(m_openRequest)) {
            // Open request hasn't been posted yet, which means that it's
            // safe to destroy stream immediately.

            m_openRequest = NULL;
            pthread_mutex_unlock(&m_connectMutex);
            
            StreamError error("", 0x0);
            destroy(error);
            return;
        }

        packet = new Packet(m_ch, Packet::SIGNAL, Packet::SIG_END);
      
        if (m_openRequest) {
            // Open request is not responded to yet. Wait to send ENDSIG until
            // we get an OPENRESP.
            
            m_pendingClose = packet;
            pthread_mutex_unlock(&m_connectMutex);
        } else {
            pthread_mutex_unlock(&m_connectMutex);

            try {
#ifdef HYDNADEBUG
                cout << "Stream: Sending close signal" << endl;
#endif
                pthread_mutex_lock(&m_connectMutex);
                ExtSocket* socket = m_socket;
                pthread_mutex_unlock(&m_connectMutex);
                socket->writeBytes(*packet);
                delete packet;
            } catch (StreamError& e) {
                pthread_mutex_unlock(&m_connectMutex);
                delete packet;
                destroy(e);
            }
        }
    }
    
    void Stream::openSuccess(unsigned int respch) {
        pthread_mutex_lock(&m_connectMutex);
        unsigned int origch = m_ch;
        Packet* packet;

        m_openRequest = NULL;
        m_ch = respch;
        m_connected = true;
      
        if (m_pendingClose) {
            packet = m_pendingClose;
            m_pendingClose = NULL;
            pthread_mutex_unlock(&m_connectMutex);

            if (origch != respch) {
                // channel is changed. We need to change the channel of the
                //packet before sending to server.
                
                packet->setChannel(respch);
            }

            try {
#ifdef HYDNADEBUG
                cout << "Stream: Sending close signal" << endl;
#endif
                pthread_mutex_lock(&m_connectMutex);
                ExtSocket* socket = m_socket;
                pthread_mutex_unlock(&m_connectMutex);
                socket->writeBytes(*packet);
                delete packet;
            } catch (StreamError& e) {
                // Something wen't terrible wrong. Queue packet and wait
                // for a reconnect.
                delete packet;
                pthread_mutex_unlock(&m_connectMutex);
                destroy(e);
            }
        } else {
            pthread_mutex_unlock(&m_connectMutex);
        }
    }

    void Stream::checkForStreamError() {
        pthread_mutex_lock(&m_connectMutex);
        if (m_error.getCode() != 0x0) {
            pthread_mutex_unlock(&m_connectMutex);
            throw m_error;
        } else {
            pthread_mutex_unlock(&m_connectMutex);
        }
    }

    void Stream::destroy(StreamError error) {
        pthread_mutex_lock(&m_connectMutex);
        ExtSocket* socket = m_socket;
        bool connected = m_connected;
        unsigned int ch = m_ch;

        m_ch = 0;
        m_connected = false;
        m_writable = false;
        m_readable = false;
        m_pendingClose = NULL;
        m_closing = false;
        m_openRequest = NULL;
        m_socket = NULL;
      
        if (socket) {
            socket->deallocStream(connected ? ch : 0);
        }

        m_error = error;

        pthread_mutex_unlock(&m_connectMutex);
    }
    
    void Stream::addData(StreamData* data) {
        pthread_mutex_lock(&m_dataMutex);

        m_dataQueue.push(data);

        pthread_mutex_unlock(&m_dataMutex);
    }

    StreamData* Stream::popData() {
        pthread_mutex_lock(&m_dataMutex);

        StreamData* data = m_dataQueue.front();
        m_dataQueue.pop();

        pthread_mutex_unlock(&m_dataMutex);
        
        return data;
    }

    bool Stream::isDataEmpty() {
        pthread_mutex_lock(&m_dataMutex);
        bool result = m_dataQueue.empty();
        pthread_mutex_unlock(&m_dataMutex);
        return result;
    }

    void Stream::addSignal(StreamSignal* signal) {
        pthread_mutex_lock(&m_signalMutex);

        m_signalQueue.push(signal);
        
        pthread_mutex_unlock(&m_signalMutex);
    }

    StreamSignal* Stream::popSignal() {
        pthread_mutex_lock(&m_signalMutex);
        
        StreamSignal* data = m_signalQueue.front();
        m_signalQueue.pop();

        pthread_mutex_unlock(&m_signalMutex);
        
        return data;
    }

    bool Stream::isSignalEmpty() {
        pthread_mutex_lock(&m_signalMutex);
        bool result =  m_signalQueue.empty();
        pthread_mutex_unlock(&m_signalMutex);
        return result;
    }
}

