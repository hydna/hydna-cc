#ifndef HYDNA_STREAM_H
#define HYDNA_STREAM_H

#include <iostream>
#include <streambuf>
#include <pthread.h>

#include "extsocket.h"
#include "openrequest.h"
#include "channeldata.h"
#include "channelsignal.h"
#include "channelmode.h"
#include "channelerror.h"

namespace hydna {

    /**
     *  This class is used as an interface to the library.
     *  A user of the library should use an instance of this class
     *  to communicate with a server.
     */
    class Channel {
    public:
        /**
         *  Initializes a new Channel instance
         */
        Channel();

        ~Channel();
        
        /**
         *  Checks the connected state for this Channel instance.
         *
         *  @return The connected state.
         */
        bool isConnected() const;

        /**
         *  Checks the closing state for this Channel instance.
         *
         *  @return The closing state.
         */
        bool isClosing() const;

        /**
         *  Checks if the channel is readable.
         *
         *  @return True if channel is readable.
         */
        bool isReadable() const;

        /**
         *  Checks if the channel is writable.
         *
         *  @return True if channel is writable.
         */
        bool isWritable() const;

        /**
         *  Checks if the channel can emit signals.
         *
         *  @return True if channel has signal support.
         */
        bool hasSignalSupport() const;
        
        /**
         *  Returns the channel that this instance listen to.
         *
         *  @return The channel.
         */
        unsigned int getChannel() const;
        
        /**
         *  Resets the error.
         *  
         *  Connects the channel to the specified channel. If the connection fails 
         *  immediately, an exception is thrown.
         *
         *  @param expr The channel to connect to,
         *  @param mode The mode in which to open the channel.
         *  @param token An optional token.
         *  @param tokenOffset Were to start to read the token from.
         *  @param tokenLength The length of the token.
         */
        void connect(std::string const &expr,
                     unsigned int mode=ChannelMode::READ,
                     const char* token=NULL,
                     unsigned int tokenOffset=0,
                     unsigned int tokenLength=0);
        
        /**
         *  Sends data to the channel.
         *
         *  @param data The data to write to the channel.
         *  @param offset Were to read from.
         *  @param length The length to read.
         *  @param priority The priority of the data.
         */
        void writeBytes(const char* data,
                                unsigned int offset,
                                unsigned int length,
                                unsigned int prority=1);

        /**
         *  Sends string data to the channel.
         *
         *  @param value The string to be sent.
         */
        void writeString(std::string const &value);
        
        /**
         *  Sends data signal to the channel.
         *
         *  @param data The data to write to the channel.
         *  @param offset Were to read from.
         *  @param length The length to read.
         *  @param type The type of the signal.
         */
        void emitBytes(const char* data,
                                unsigned int offset=0,
                                unsigned int length=0,
                                unsigned int type=0);

        /**
         *  Sends a string signal to the channel.
         *
         *  @param value The string to be sent.
         *  @param type The type of the signal.
         */
        void emitString(std::string const &value, int type=0);

        /**
         *  Closes the Channel instance.
         */
        void close();


        /**
         *  Checks if some error has occured in the channel
         *  and throws an exception if that is the case.
         */
        void checkForChannelError();

        /**
         *  Pop the next data in the data queue.
         *
         *  @return The data that was removed from the queue,
         *          or NULL if the queue was empty.
         */
        ChannelData* popData();

        /**
         *  Checks if the signal queue is empty.
         *
         *  @return True if the queue is empty.
         */
        bool isDataEmpty();

        /**
         *  Pop the next signal in the signal queue.
         *
         *  @return The signal that was removed from the queue,
         *          or NULL if the queue was empty.
         */
        ChannelSignal* popSignal();

        /**
         *  Checks if the signal queue is empty.
         *
         *  @return True is the queue is empty.
         */
        bool isSignalEmpty();

        friend class ExtSocket;
        
    private:
        /**
         *  Internal callback for open success.
         *  Used by the ExtSocket class.
         *
         *  @param respch The response channel.
         */
        void openSuccess(unsigned int respch);

        /**
         *  Internally destroy socket.
         *
         *  @param error The cause of the destroy.
         */
        void destroy(ChannelError error);

        /**
         *  Add data to the data queue.
         *
         *  @param data The data to add to queue.
         */
        void addData(ChannelData* data);

        /**
         *  Add signals to the signal queue.
         *
         *  @param signal The signal to add to the queue.
         */
        void addSignal(ChannelSignal* signal);
        
        /**
         *  Internally close the channel.
         */
        void internalClose();

        std::string m_host;
        unsigned short m_port;
        unsigned int m_ch;
        
        ExtSocket* m_socket;
        bool m_connected;
        bool m_closing;
        Packet* m_pendingClose;
        
        bool m_readable;
        bool m_writable;
        bool m_emitable;

        ChannelError m_error;

        unsigned int m_mode;

        OpenRequest* m_openRequest;

        ChannelDataQueue m_dataQueue;
        ChannelSignalQueue m_signalQueue;

        mutable pthread_mutex_t m_dataMutex;
        mutable pthread_mutex_t m_signalMutex;
        mutable pthread_mutex_t m_connectMutex;
    };

    typedef std::map<unsigned int, Channel*> ChannelMap;
}

#endif

