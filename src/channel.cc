#include <pthread.h>
#include <iomanip>
#include <sstream>

#include "channel.h"
#include "frame.h";
#include "openrequest.h"
#include "channeldata.h"
#include "channelsignal.h"
#include "channelmode.h"
#include "url.h"

#include "error.h"
#include "ioerror.h"
#include "rangeerror.h"
#include "channelerror.h"

#ifdef HYDNADEBUG
#include "debughelper.h"
#endif

namespace hydna {
    
    using namespace std;

    Channel::Channel() : m_ch(0), m_message(""), m_connection(NULL), m_connected(false), m_closing(false), m_pendingClose(NULL),
                       m_readable(false), m_writable(false), m_emitable(false), m_error("", 0x0), m_openRequest(NULL)
    {
        pthread_mutex_init(&m_dataMutex, NULL);
        pthread_mutex_init(&m_signalMutex, NULL);
        pthread_mutex_init(&m_connectMutex, NULL);
    }

    Channel::~Channel() {
        pthread_mutex_destroy(&m_dataMutex);
        pthread_mutex_destroy(&m_signalMutex);
        pthread_mutex_destroy(&m_connectMutex);
    }

    bool Channel::getFollowRedirects() const
    {
        return Connection::m_followRedirects;
    }

    void Channel::setFollowRedirects(bool value)
    {
        Connection::m_followRedirects = value;
    }
    
    bool Channel::isConnected() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Channel::isClosing() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_closing;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Channel::isReadable() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected && m_readable;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Channel::isWritable() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected && m_writable;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    bool Channel::hasSignalSupport() const {
        pthread_mutex_lock(&m_connectMutex);
        bool result = m_connected && m_emitable;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    unsigned int Channel::getChannel() const {
        pthread_mutex_lock(&m_connectMutex);
        unsigned int result = m_ch;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }

    string Channel::getMessage() const {
        pthread_mutex_lock(&m_connectMutex);
        string result = m_message;
        pthread_mutex_unlock(&m_connectMutex);
        return result;
    }
    
    void Channel::connect(string const &expr,
                 unsigned int mode,
                 const char* token,
                 unsigned int tokenOffset,
                 unsigned int tokenLength)
    {
        Frame* frame;
        OpenRequest* request;
      
        pthread_mutex_lock(&m_connectMutex);
        if (m_connection) {
            pthread_mutex_unlock(&m_connectMutex);
            throw Error("Already connected");
        }
        pthread_mutex_unlock(&m_connectMutex);

        if (mode == 0x04 ||
                mode < ChannelMode::READ || 
                mode > ChannelMode::READWRITEEMIT) {
            throw Error("Invalid channel mode");
        }
      
        m_mode = mode;
      
        m_readable = ((m_mode & ChannelMode::READ) == ChannelMode::READ);
        m_writable = ((m_mode & ChannelMode::WRITE) == ChannelMode::WRITE);
        m_emitable = ((m_mode & ChannelMode::EMIT) == ChannelMode::EMIT);

        URL url = URL::parse(expr);
        string tokens = "";
        string chs = "";
        unsigned int ch;

        if (url.getProtocol() != "http") {
            if (url.getProtocol() == "https") {
                throw Error("The protocol HTTPS is not supported");
            } else {
                throw Error("Unknown protocol, " + url.getProtocol());
            }
        }

        if (url.getError() != "") {
            throw Error(url.getError()); 
        }

        chs = url.getPath();
        
        // Take out the channel
        
        if (chs.length() > 0) {
            istringstream iss(chs);

            if ((iss >> setbase(16) >> ch).fail()) {
                throw Error("Could not read the channel \"" + chs + "\"");
            }
        } else {
            ch = 1;
        }


        tokens = url.getToken();
        m_ch = ch;
        m_connection = Connection::getConnection(url.getHost(), url.getPort(), url.getAuth());
      
        // Ref count
        m_connection->allocChannel();

        if (token || tokens == "") {
            frame = new Frame(m_ch, Frame::OPEN, mode,
                                token, tokenOffset, tokenLength);
        } else {
            frame = new Frame(m_ch, Frame::OPEN, mode,
                                tokens.c_str(), 0, tokens.size());
        }
      
        request = new OpenRequest(this, m_ch, frame);

        m_error = ChannelError("", 0x0);
      
        if (!m_connection->requestOpen(request)) {
            checkForChannelError();
            throw Error("Channel already open");
        }

        m_openRequest = request;
    }
    
    void Channel::writeBytes(const char* data,
                            unsigned int offset,
                            unsigned int length,
                            unsigned int priority)
    {
        bool result;

        pthread_mutex_lock(&m_connectMutex);
        if (!m_connected || !m_connection) {
            pthread_mutex_unlock(&m_connectMutex);
            checkForChannelError();
            throw IOError("Channel is not connected");
        }
        pthread_mutex_unlock(&m_connectMutex);

        if (!m_writable) {
            throw Error("Channel is not writable");
        }
      
        if (priority > 3 || priority == 0) {
            throw RangeError("Priority must be between 1 - 3");
        }

        Frame frame(m_ch, Frame::DATA, priority,
                                data, offset, length);
      
        pthread_mutex_lock(&m_connectMutex);
        Connection* connection = m_connection;
        pthread_mutex_unlock(&m_connectMutex);
        result = connection->writeBytes(frame);

        if (!result)
            checkForChannelError();
    }

    void Channel::writeString(string const &value) {
        writeBytes(value.data(), 0, value.length());
    }
    
    void Channel::emitBytes(const char* data,
                            unsigned int offset,
                            unsigned int length)
    {
        bool result;

        pthread_mutex_lock(&m_connectMutex);
        if (!m_connected || !m_connection) {
            pthread_mutex_unlock(&m_connectMutex);
            checkForChannelError();
            throw IOError("Channel is not connected.");
        }
        pthread_mutex_unlock(&m_connectMutex);

        if (!m_emitable) {
            throw Error("You do not have permission to send signals");
        }

        Frame frame(m_ch, Frame::SIGNAL, Frame::SIG_EMIT,
                            data, offset, length);

        pthread_mutex_lock(&m_connectMutex);
        Connection* connection = m_connection;
        pthread_mutex_unlock(&m_connectMutex);
        result = connection->writeBytes(frame);

        if (!result)
            checkForChannelError();
    }

    void Channel::emitString(string const &value) {
        emitBytes(value.data(), 0, value.length());
    }

    void Channel::close() {
        Frame* frame;

        pthread_mutex_lock(&m_connectMutex);
        if (!m_connection || m_closing) {
            pthread_mutex_unlock(&m_connectMutex);
            return;
        }

        m_closing = true;
        m_readable = false;
        m_writable = false;
        m_emitable = false;

        if (m_openRequest && m_connection->cancelOpen(m_openRequest)) {
            // Open request hasn't been posted yet, which means that it's
            // safe to destroy channel immediately.

            m_openRequest = NULL;
            pthread_mutex_unlock(&m_connectMutex);
            
            ChannelError error("", 0x0);
            destroy(error);
            return;
        }

        frame = new Frame(m_ch, Frame::SIGNAL, Frame::SIG_END);
      
        if (m_openRequest) {
            // Open request is not responded to yet. Wait to send ENDSIG until
            // we get an OPENRESP.
            
            m_pendingClose = frame;
            pthread_mutex_unlock(&m_connectMutex);
        } else {
            pthread_mutex_unlock(&m_connectMutex);

            try {
#ifdef HYDNADEBUG
                debugPrint("Channel", m_ch, "Sending close signal");
#endif
                pthread_mutex_lock(&m_connectMutex);
                Connection* connection = m_connection;
                pthread_mutex_unlock(&m_connectMutex);
                connection->writeBytes(*frame);
                delete frame;
            } catch (ChannelError& e) {
                pthread_mutex_unlock(&m_connectMutex);
                delete frame;
                destroy(e);
            }
        }
    }
    
    void Channel::openSuccess(unsigned int respch, std::string const &message) {
        pthread_mutex_lock(&m_connectMutex);
        unsigned int origch = m_ch;
        Frame* frame;

        m_openRequest = NULL;
        m_ch = respch;
        m_connected = true;
        m_message = message;
      
        if (m_pendingClose) {
            frame = m_pendingClose;
            m_pendingClose = NULL;
            pthread_mutex_unlock(&m_connectMutex);

            if (origch != respch) {
                // channel is changed. We need to change the channel of the
                //frame before sending to server.
                
                frame->setChannel(respch);
            }

            try {
#ifdef HYDNADEBUG
                debugPrint("Channel", m_ch, "Sending close signal");
#endif
                pthread_mutex_lock(&m_connectMutex);
                Connection* connection = m_connection;
                pthread_mutex_unlock(&m_connectMutex);
                connection->writeBytes(*frame);
                delete frame;
            } catch (ChannelError& e) {
                // Something wen't terrible wrong. Queue frame and wait
                // for a reconnect.
                delete frame;
                pthread_mutex_unlock(&m_connectMutex);
                destroy(e);
            }
        } else {
            pthread_mutex_unlock(&m_connectMutex);
        }
    }

    void Channel::checkForChannelError() {
        pthread_mutex_lock(&m_connectMutex);
        if (m_error.getCode() != 0x0) {
            pthread_mutex_unlock(&m_connectMutex);
            throw m_error;
        } else {
            pthread_mutex_unlock(&m_connectMutex);
        }
    }

    void Channel::destroy(ChannelError error) {
        pthread_mutex_lock(&m_connectMutex);
        Connection* connection = m_connection;
        bool connected = m_connected;
        unsigned int ch = m_ch;

        m_ch = 0;
        m_connected = false;
        m_writable = false;
        m_readable = false;
        m_pendingClose = NULL;
        m_closing = false;
        m_openRequest = NULL;
        m_connection = NULL;
      
        if (connection) {
            connection->deallocChannel(connected ? ch : 0);
        }

        m_error = error;

        pthread_mutex_unlock(&m_connectMutex);
    }
    
    void Channel::addData(ChannelData* data) {
        pthread_mutex_lock(&m_dataMutex);

        m_dataQueue.push(data);

        pthread_mutex_unlock(&m_dataMutex);
    }

    ChannelData* Channel::popData() {
        pthread_mutex_lock(&m_dataMutex);

        ChannelData* data = m_dataQueue.front();
        m_dataQueue.pop();

        pthread_mutex_unlock(&m_dataMutex);
        
        return data;
    }

    bool Channel::isDataEmpty() {
        pthread_mutex_lock(&m_dataMutex);
        bool result = m_dataQueue.empty();
        pthread_mutex_unlock(&m_dataMutex);
        return result;
    }

    void Channel::addSignal(ChannelSignal* signal) {
        pthread_mutex_lock(&m_signalMutex);

        m_signalQueue.push(signal);
        
        pthread_mutex_unlock(&m_signalMutex);
    }

    ChannelSignal* Channel::popSignal() {
        pthread_mutex_lock(&m_signalMutex);
        
        ChannelSignal* data = m_signalQueue.front();
        m_signalQueue.pop();

        pthread_mutex_unlock(&m_signalMutex);
        
        return data;
    }

    bool Channel::isSignalEmpty() {
        pthread_mutex_lock(&m_signalMutex);
        bool result =  m_signalQueue.empty();
        pthread_mutex_unlock(&m_signalMutex);
        return result;
    }
}

