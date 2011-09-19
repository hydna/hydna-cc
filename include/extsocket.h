#ifndef HYDNA_EXTSOCKET_H
#define HYDNA_EXTSOCKET_H

#include <iostream>
#include <streambuf>
#include <map>

#include "openrequest.h"
#include "channelerror.h"

namespace hydna {
    class Packet;
    class Channel;

    typedef std::map<unsigned int, Channel*> ChannelMap;


    /**
     *  This class is used internally by the Channel class.
     *  A user of the library should not create an instance of this class.
     */
    class ExtSocket {

        typedef std::map<std::string, ExtSocket*> SocketMap;

    public:
        /**
         *  Return an available socket or create a new one.
         *
         *  @param host The host associated with the socket.
         *  @param port The port associated with the socket.
         *  @return The socket.
         */
        static ExtSocket* getSocket(std::string const &host, unsigned short port);

        /**
         *  Initializes a new Channel instance.
         *
         *  @param host The host the socket should connect to.
         *  @param port The port the socket should connect to.
         */
        ExtSocket(std::string const &host, unsigned short port);

        ~ExtSocket();
        
        /**
         *  Returns the handshake status of the socket.
         *
         *  @return True if the socket has handshaked.
         */
        bool hasHandshaked() const;
        
        /**
         * Method to keep track of the number of channels that is associated 
         * with this socket instance.
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
         *  Writes a packet to the socket.
         *
         *  @param packet The packet to be sent.
         *  @return True if the packet was sent.
         */
        bool writeBytes(Packet& packet);
        
    private:
        /**
         *  Check if there are any more references to the socket.
         */
        void checkRefCount();

        /**
         *  Connect the socket.
         *
         *  @param host The host to connect to.
         *  @param port The port to connect to.
         */
        void connectSocket(std::string &host, int port);
        
        /**
         *  Send a handshake packet.
         */
        void connectHandler();

        /**
         *  Handle the Handshake response packet.
         */
        void handshakeHandler();

        /**
         *  Handles all incomming data.
         */
        void receiveHandler();

        /**
         *  Process an open packet.
         *
         *  @param ch The channel that should receive the open packet.
         *  @param errcode The error code of the open packet.
         *  @param payload The content of the open packet.
         *  @param size The size of the content.
         */
        void processOpenPacket(unsigned int ch,
                                int errcode,
                                const char* payload,
                                int size);

        /**
         *  Process a data packet.
         *
         *  @param ch The channel that should receive the data.
         *  @param priority The priority of the data.
         *  @param payload The content of the data.
         *  @param size The size of the content.
         */
        void processDataPacket(unsigned int ch,
                            int priority,
                            const char* payload,
                            int size);

        /**
         *  Process a signal packet.
         *
         *  @param channel The channel that should receive the signal.
         *  @param flag The flag of the signal.
         *  @param payload The content of the signal.
         *  @param size The size of the content.
         *  @return False is something went wrong.
         */
        bool processSignalPacket(Channel* channel,
                            int flag,
                            const char* payload,
                            int size);

        /**
         *  Process a signal packet.
         *
         *  @param ch The channel that should receive the signal.
         *  @param flag The flag of the signal.
         *  @param payload The content of the signal.
         *  @param size The size of the content.
         */
        void processSignalPacket(unsigned int ch,
                            int flag,
                            const char* payload,
                            int size);

        /**
         *  Destroy the socket.
         *
         *  @error The cause of the destroy.
         */
        void destroy(ChannelError error);


        static const int HANDSHAKE_SIZE = 9;
        static const int HANDSHAKE_RESP_SIZE = 5;

        static SocketMap m_availableSockets;
        static pthread_mutex_t m_socketMutex;

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
        int m_socketFDS;

        OpenRequestMap m_pendingOpenRequests;
        ChannelMap m_openChannels;
        OpenRequestQueueMap m_openWaitQueue;
        
        int m_channelRefCount;
        
        pthread_t listeningThread;

        /**
         * The method that is called in the new thread.
         * Listens for incoming packets.
         *
         * @param ptr A pointer to a ListenArgs struct.
         * @return NULL
         */
        static void* listen(void *ptr);
    };

    typedef std::map<std::string, ExtSocket*> SocketMap;

    /**
     * A struct with args that 
     * the new thread is using.
     */
    struct ListenArgs {
        ExtSocket* extSocket;
    };
}

#endif

