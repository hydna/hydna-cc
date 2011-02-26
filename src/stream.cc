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

    Stream::Stream() : m_host(""), m_port(7010), m_addr(1), m_socket(NULL), m_connected(false), m_pendingClose(false),
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
        bool result = m_connected && m_emitable && !m_pendingClose;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    unsigned int Stream::getAddr() const {
        pthread_mutex_lock(&m_connectMutex);
        unsigned int result = m_addr;
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
                mode > StreamMode::READWRITE_EMIT) {
            throw Error("Invalid stream mode");
        }
      
        m_mode = mode;
      
        m_readable = ((m_mode & StreamMode::READ) == StreamMode::READ);
        m_writable = ((m_mode & StreamMode::WRITE) == StreamMode::WRITE);
        m_emitable = ((m_mode & StreamMode::EMIT) == StreamMode::EMIT);

        string host = expr;
        unsigned short port = 7010;
        unsigned int addr = 1;
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

            if ((iss >> setbase(16) >> addr).fail()) {
               throw Error("Could not read the address \"" + host.substr(pos + 2) + "\""); 
            }
        } else {
            pos = host.find_last_of("/");
            if (pos != string::npos) {
                istringstream iss(host.substr(pos + 1));
                host = host.substr(0, pos);

                if ((iss >> addr).fail()) {
                   throw Error("Could not read the address \"" + host.substr(pos + 1) + "\""); 
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
        m_addr = addr;

        m_socket = ExtSocket::getSocket(m_host, m_port);
      
        // Ref count
        m_socket->allocStream();

        if (token || tokens == "") {
            packet = new Packet(m_addr, Packet::OPEN, mode,
                                token, tokenOffset, tokenLength);
        } else {
            packet = new Packet(m_addr, Packet::OPEN, mode,
                                tokens.c_str(), 0, tokens.size());
        }
      
        request = new OpenRequest(this, m_addr, packet);

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

        if ((m_mode & StreamMode::WRITE) != StreamMode::WRITE) {
            throw Error("Stream is not writable");
        }
      
        if (priority > 3 || priority == 0) {
            throw RangeError("Priority must be between 1 - 3");
        }

        Packet packet(m_addr, Packet::DATA, priority,
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

        if ((m_mode & StreamMode::EMIT) != StreamMode::EMIT) {
            throw Error("You do not have permission to send signals");
        }

        Packet packet(m_addr, Packet::SIGNAL, type,
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
        pthread_mutex_lock(&m_connectMutex);
        if (!m_socket || m_pendingClose) {
            pthread_mutex_unlock(&m_connectMutex);
            return;
        }
      
        if (m_openRequest) {
            if (m_socket->cancelOpen(m_openRequest)) {
                m_openRequest = NULL;
                pthread_mutex_unlock(&m_connectMutex);
                
                StreamError error("", 0x0);
                destroy(error);
            } else {
                m_pendingClose = true;
                pthread_mutex_unlock(&m_connectMutex);
            }
        } else {
            pthread_mutex_unlock(&m_connectMutex);
            internalClose();
        }
    }
    
    void Stream::openSuccess(unsigned int respaddr) {
        pthread_mutex_lock(&m_connectMutex);
        m_addr = respaddr;
        m_connected = true;
        m_openRequest = NULL;
      
        if (m_pendingClose) {
            pthread_mutex_unlock(&m_connectMutex);
            internalClose();
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

        m_pendingClose = false;
        m_writable = false;
        m_readable = false;
      
        if (m_socket) {
            m_socket->deallocStream(m_connected ? m_addr : 0);
        }

        m_connected = false;
        m_addr = 1;
        m_openRequest = NULL;
        m_socket = NULL;
        m_error = error;

        pthread_mutex_unlock(&m_connectMutex);
    }
    
    void Stream::internalClose() {
        pthread_mutex_lock(&m_connectMutex);
        if (m_socket && m_connected) {
            pthread_mutex_unlock(&m_connectMutex);
#ifdef HYDNADEBUG
            cout << "Stream: Sending close signal" << endl;
#endif
            Packet packet(m_addr, Packet::SIGNAL, Packet::SIG_END, NULL, 0, 0);

            pthread_mutex_lock(&m_connectMutex);
            ExtSocket* socket = m_socket;
            pthread_mutex_unlock(&m_connectMutex);
            socket->writeBytes(packet);
        } else {
            pthread_mutex_unlock(&m_connectMutex);
        }

        StreamError error("", 0x0);
        destroy(error);
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

