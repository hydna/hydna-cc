#ifndef HYDNA_EXTSOCKET_H
#define HYDNA_EXTSOCKET_H

#include <iostream>
#include <streambuf>
#include <map>

#include "openrequest.h"
#include "channelerror.h"

namespace hydna {
    class Frame;
    class Channel;

    typedef std::map<unsigned int, Channel*> ChannelMap;


    /**
     *  This class is used internally by the Channel class.
     *  A user of the library should not create an instance of this class.
     */
    class Connection {

        typedef std::map<std::string, Connection*> ConnectionMap;

    public:
        /**
         *  Return an available connection or create a new one.
         *
         *  @param host The host associated with the connection.
         *  @param port The port associated with the connection.
         *  @return The connection.
         */
        static Connection* getConnection(std::string const &host, unsigned short port, std::string const &auth);

        /**
         *  Initializes a new Channel instance.
         *
         *  @param host The host the connection should connect to.
         *  @param port The port the connection should connect to.
         */
        Connection(std::string const &host, unsigned short port, std::string const &auth);

        ~Connection();
        
        /**
         *  Returns the handshake status of the connection.
         *
         *  @return True if the connection has handshaked.
         */
        bool hasHandshaked() const;
        
        /**
         * Method to keep track of the number of channels that is associated 
         * with this connection instance.
         */
        void allocChannel();
        
        /**
         *  Decrease the reference count.
         *
         *  @param ch The channel to dealloc.
         */
        void deallocChannel(unsigned int ch);

        /**
         *  Request to open a channel.
         *
         *  @param request The request to open the channel.
         *  @return True if request went well, else false.
         */
        bool requestOpen(OpenRequest* request);
        
        /**
         *  Try to cancel an open request. Returns true on success else
         *  false.
         *
         *  @param request The request to cancel.
         *  @return True if the request was canceled.
         */
        bool cancelOpen(OpenRequest* request);
        
        /**
         *  Writes a frame to the connection.
         *
         *  @param frame The frame to be sent.
         *  @return True if the frame was sent.
         */
        bool writeBytes(Frame& frame);
        
    private:
        /**
         *  Check if there are any more references to the connection.
         */
        void checkRefCount();

        /**
         *  Connect the connection.
         *
         *  @param host The host to connect to.
         *  @param port The port to connect to.
         */
        void connectConnection(std::string const &host, int port, std::string const &auth);

        /**
         *  Send HTTP upgrade request.
         */
        void connectHandler(std::string const &auth);

        /**
         *  Handle the Handshake response frame.
         */
        void handshakeHandler();

        /**
         *  Handles all incomming data.
         */
        void receiveHandler();

        /**
         *  Process an open frame.
         *
         *  @param ch The channel that should receive the open frame.
         *  @param errcode The error code of the open frame.
         *  @param payload The content of the open frame.
         *  @param size The size of the content.
         */
        void processOpenFrame(unsigned int ch,
                                int errcode,
                                const char* payload,
                                int size);

        /**
         *  Process a data frame.
         *
         *  @param ch The channel that should receive the data.
         *  @param priority The priority of the data.
         *  @param payload The content of the data.
         *  @param size The size of the content.
         */
        void processDataFrame(unsigned int ch,
                            int priority,
                            const char* payload,
                            int size);

        /**
         *  Process a signal frame.
         *
         *  @param channel The channel that should receive the signal.
         *  @param flag The flag of the signal.
         *  @param payload The content of the signal.
         *  @param size The size of the content.
         *  @return False is something went wrong.
         */
        bool processSignalFrame(Channel* channel,
                            int flag,
                            const char* payload,
                            int size);

        /**
         *  Process a signal frame.
         *
         *  @param ch The channel that should receive the signal.
         *  @param flag The flag of the signal.
         *  @param payload The content of the signal.
         *  @param size The size of the content.
         */
        void processSignalFrame(unsigned int ch,
                            int flag,
                            const char* payload,
                            int size);

        /**
         *  Destroy the connection.
         *
         *  @error The cause of the destroy.
         */
        void destroy(ChannelError error);


        static const int HANDSHAKE_SIZE = 9;
        static const int HANDSHAKE_RESP_SIZE = 5;

        static ConnectionMap m_availableConnections;
        static pthread_mutex_t m_connectionMutex;

        pthread_mutex_t m_channelRefMutex;
        pthread_mutex_t m_destroyingMutex;
        pthread_mutex_t m_closingMutex;
        pthread_mutex_t m_openChannelsMutex;
        pthread_mutex_t m_openWaitMutex;
        pthread_mutex_t m_pendingMutex;
        pthread_mutex_t m_listeningMutex;

        bool m_connecting;
        bool m_connected;
        bool m_handshaked;
        bool m_destroying;
        bool m_closing;
        bool m_listening;

        std::string m_host;
        unsigned short m_port;
        std::string m_auth;
        int m_connectionFDS;

        OpenRequestMap m_pendingOpenRequests;
        ChannelMap m_openChannels;
        OpenRequestQueueMap m_openWaitQueue;
        
        int m_channelRefCount;
        
        pthread_t listeningThread;

        /**
         * The method that is called in the new thread.
         * Listens for incoming frames.
         *
         * @param ptr A pointer to a ListenArgs struct.
         * @return NULL
         */
        static void* listen(void *ptr);
    };

    typedef std::map<std::string, Connection*> ConnectionMap;

    /**
     * A struct with args that 
     * the new thread is using.
     */
    struct ListenArgs {
        Connection* extConnection;
    };
}

#endif

