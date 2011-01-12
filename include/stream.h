#ifndef HYDNA_STREAM_H
#define HYDNA_STREAM_H

#include <iostream>
#include <streambuf>
#include <pthread.h>

#include "addr.h"
#include "extsocket.h"
#include "openrequest.h"
#include "streamdata.h"
#include "streamsignal.h"
#include "streammode.h"
#include "streamerror.h"

namespace hydna {

    class Stream {
    public:
        /**
         *  Initializes a new Stream instance
         */
        Stream();

        ~Stream();
        
        /**
         *  Return the connected state for this Stream instance.
         */
        bool isConnected() const;

        /**
         *  Return true if stream is readable
         */
        bool isReadable() const;

        /**
         *  Return true if stream is writable
         */
        bool isWritable() const;

        /**
         *  Return true if stream has signal support.
         */
        bool hasSignalSupport() const;
        
        /**
         *  Returns the HydnaAddr that this instance listen to.
         *
         *  @return {HydnaAddr} the specified HydnaAddr instance.
         */
        Addr getAddr() const;
        
        /**
         *  Resets the error.
         *  
         *  Connects the stream to the specified addr. If the connection fails 
         *  immediately, either an event is dispatched or an exception is thrown: 
         *  an error event is dispatched if a host was specified, and an exception
         *  is thrown if no host was specified. Otherwise, the status of the 
         *  connection is reported by an event. If the socket is already 
         *  connected, the existing connection is closed first.
         *
         *  By default, the value you pass for host must be in the same domain 
         *  and the value you pass for port must be 1024 or higher. For example, 
         *  a SWF file at adobe.com can connect only to a server daemon running 
         *  at adobe.com. If you want to connect to a socket on a different host 
         *  than the one from which the connecting SWF file was served, or if you 
         *  want to connect to a port lower than 1024 on any host, you must 
         *  obtain an xmlsocket: policy file from the host to which you are 
         *  connecting. Howver, these restrictions do not exist for AIR content 
         *  in the application security sandbox. For more information, see the 
         *  "Flash Player Security" chapter in Programming ActionScript 3.0.
         */
        void connect(Addr addr,
                     unsigned int mode=StreamMode::READ,
                     const char* token=NULL,
                     unsigned int tokenOffset=0,
                     unsigned int tokenLength=0);
        
        /**
         *  Writes a sequence of bytes from the specified byte array. The write 
         *  operation starts at the <code>position</code> specified by offset.
         *  
         *  <p>If you omit the length parameter the default length of 0 causes 
         *  the method to write the entire buffer starting at offset.</p>
         *
         *  <p>If you also omit the <code>offset</code> parameter, the entire 
         *  buffer is written.</p>
         *
         *  <p>If offset or length is out of range, they are adjusted to match 
         *  the beginning and end of the bytes array.</p>
         */
        void writeBytes(const char* data,
                                unsigned int offset,
                                unsigned int length,
                                unsigned int prority=1);

        void writeString(std::string const &value);
        
        /**
         *  Sends a signal to the stream.
         *
         *  <p>Note: Signal write access is permitted in order to send via
         *     network.</p>
         *
         *  @param value The string to write to the stream.
         */
        void sendSignal(const char* data,
                                unsigned int offset=0,
                                unsigned int length=0,
                                unsigned int type=0);

        void sendStringSignal(std::string const &value, int type=0);

        /**
         *  Closes the Stream instance.
         */
        void close();

        // Internal callback for open success
        void openSuccess(Addr respaddr);

        void checkForStreamError();

        // Internally destroy socket.
        void destroy(StreamError error);

        void addData(StreamData* data);

        StreamData* popData();

        bool isDataEmpty();

        void addSignal(StreamSignal* signal);

        StreamSignal* popSignal();

        bool isSignalEmpty();
        
    private:
        
        // Internally close stream
        void internalClose();

        Addr m_addr;
        
        ExtSocket* m_socket;
        bool m_connected;
        bool m_pendingClose;
        
        bool m_readable;
        bool m_writable;
        bool m_signalSupport;

        StreamError m_error;

        unsigned int m_mode;

        OpenRequest* m_openRequest;

        StreamDataQueue m_dataQueue;
        StreamSignalQueue m_signalQueue;

        pthread_mutex_t dataMutex;
        pthread_mutex_t signalMutex;
    };

    typedef std::map<unsigned int, Stream*> StreamMap;
}

#endif

