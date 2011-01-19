#include <pthread.h>
#include <iomanip>
#include <sstream>

#include "stream.h"
#include "message.h";
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
                       m_readable(false), m_writable(false), m_signalSupport(false), m_error("", 0x0), m_openRequest(NULL)
    {
        pthread_mutex_init(&dataMutex, NULL);
        pthread_mutex_init(&signalMutex, NULL);
    }

    Stream::~Stream() {
        pthread_mutex_destroy(&dataMutex);
        pthread_mutex_destroy(&signalMutex);
    }
    
    bool Stream::isConnected() const {
        return m_connected;
    }

    bool Stream::isReadable() const {
        return m_connected && m_readable;
    }

    bool Stream::isWritable() const {
        return m_connected && m_writable;
    }

    bool Stream::hasSignalSupport() const {
        return !m_pendingClose && m_connected && m_writable;
    }
    
    void Stream::connect(string const &expr,
                 unsigned int mode,
                 const char* token,
                 unsigned int tokenOffset,
                 unsigned int tokenLength)
    {
        Message* message;
        OpenRequest* request;
      
        if (m_socket) {
            throw Error("Already connected");
        }

        if (mode == 0x04 ||
                mode < StreamMode::READ || 
                mode > StreamMode::READWRITE_SIG) {
            throw Error("Invalid stream mode");
        }
      
        m_mode = mode;
      
        m_readable = ((m_mode & StreamMode::READ) == StreamMode::READ);
        m_writable = ((m_mode & StreamMode::WRITE) == StreamMode::WRITE);
        m_signalSupport = ((m_mode & 0x04) == 0x04);

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
            message = new Message(m_addr, Message::OPEN, mode,
                                token, tokenOffset, tokenLength);
        } else {
            message = new Message(m_addr, Message::OPEN, mode,
                                tokens.c_str(), 0, tokens.size());
        }
      
        request = new OpenRequest(this, m_addr, message);

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
        if (!m_connected || !m_socket) {
            checkForStreamError();
            throw IOError("Stream is not connected");
        }

        if (m_mode == StreamMode::READ) {
            throw Error("Stream is not writable");
        }
      
        if (priority > 3 || priority == 0) {
            throw RangeError("Priority must be between 1 - 3");
        }

        Message message(m_addr, Message::DATA, priority,
                                data, offset, length);
      
        result = m_socket->writeBytes(message);

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
        if (!m_connected || !m_socket) {
            checkForStreamError();
            throw IOError("Stream is not connected.");
        }

        if ((m_mode & 0x4) == 0x4) {
            throw Error("You do not have permission to send signals");
        }

        Message message(m_addr, Message::SIGNAL, type,
                            data, offset, length);

        result = m_socket->writeBytes(message);

        if (!result)
            checkForStreamError();
    }

    void Stream::emitString(string const &value, int type) {
        emitBytes(value.data(), 0, value.length(), type);
    }

    void Stream::close() {
        if (!m_socket || m_pendingClose)
            return;
      
        if (m_openRequest) {
            if (m_socket->cancelOpen(m_openRequest)) {
                m_openRequest = NULL;
                StreamError error("", 0x0);
                destroy(error);
            } else {
                m_pendingClose = true;
            }
        } else {
            internalClose();
        }
    }
    
    void Stream::openSuccess(unsigned int respaddr) {
        m_addr = respaddr;
        m_connected = true;
      
        if (m_pendingClose) {
            internalClose();
        }
    }

    void Stream::checkForStreamError() {
        if (m_error.getCode() != 0x0)
            throw m_error;
    }

    void Stream::destroy(StreamError error) {
        //if (pthread_mutex_lock(&dataMutex) != 0)
        //    cerr << "Unable to lock mutex" << endl;


        m_connected = false;
        m_pendingClose = false;
        m_writable = false;
        m_readable = false;
      
        if (m_socket) {
            m_socket->deallocStream(m_connected ? m_addr : 0);
        }

        //m_addr = NULL;
        m_socket = NULL;
        m_error = error;
        
        //if (!event) {
        //    dispatchEvent(event);
        //}
        //if (pthread_mutex_unlock(&dataMutex) != 0)
        //    cerr << "Unable to unlock mutex" << endl;
    }
    
    void Stream::internalClose() {
        if (m_socket && m_connected) {
            emitBytes(NULL, 0, 0, Message::END);
        }

        StreamError error("", 0x0);
        destroy(error);
    }

    void Stream::addData(StreamData* data) {
        pthread_mutex_lock(&dataMutex);

        m_dataQueue.push(data);

        pthread_mutex_unlock(&dataMutex);
    }

    StreamData* Stream::popData() {
        pthread_mutex_lock(&dataMutex);

        StreamData* data = m_dataQueue.front();
        m_dataQueue.pop();

        pthread_mutex_unlock(&dataMutex);
        
        return data;
    }

    bool Stream::isDataEmpty() {
        return m_dataQueue.empty();
    }

    void Stream::addSignal(StreamSignal* signal) {
        pthread_mutex_lock(&signalMutex);

        m_signalQueue.push(signal);
        
        pthread_mutex_unlock(&signalMutex);
    }

    StreamSignal* Stream::popSignal() {
        pthread_mutex_lock(&signalMutex);
        
        StreamSignal* data = m_signalQueue.front();
        m_signalQueue.pop();

        pthread_mutex_unlock(&signalMutex);
        
        return data;
    }

    bool Stream::isSignalEmpty() {
        return m_signalQueue.empty();
    }
}

