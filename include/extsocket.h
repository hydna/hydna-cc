#ifndef HYDNA_EXTSOCKET_H
#define HYDNA_EXTSOCKET_H

#include <iostream>
#include <streambuf>
#include <map>

#include "openrequest.h"
#include "streamerror.h"

namespace hydna {
    class Message;
    class Stream;

    typedef std::map<unsigned int, Stream*> StreamMap;


    class ExtSocket {

        typedef std::map<std::string, ExtSocket*> SocketMap;

    public:
        // Return an available socket or create a new one.
        static ExtSocket* getSocket(std::string const &host, unsigned short port);

        /**
         *  Initializes a new Stream instance
         */
        ExtSocket(std::string const &host, unsigned short port);

        ~ExtSocket();
        
        bool hasHandshaked() const;
        
        // Internal method to keep track of no of streams that is associated 
        // with this connection instance.
        void allocStream();
        
        // Decrease the reference count
        void deallocStream(unsigned int addr);

        // Request to open a stream.
        // Return true if request went well, else false.
        bool requestOpen(OpenRequest* request);
        
        // Try to cancel an open request. Returns true on success else
        // false.
        bool cancelOpen(OpenRequest* request);
        
        bool writeBytes(Message& message);
        void writeMultiByte(std::string& value);
        void writeUnsignedInt(unsigned int value);
        
    private:
        void checkRefCount();

        void connectSocket(std::string &host, int port);
        
        //  Send a handshake packet.
        void connectHandler();

        // Handle the Handshake response packet.
        void handshakeHandler();

        // Handles all incomming data.
        void receiveHandler();

        // Process a open response message.
        void processOpenMessage(unsigned int addr,
                                int errcode,
                                const char* payload,
                                int size);

        // Process a data message.
        void processDataMessage(unsigned int addr,
                            int priority,
                            const char* payload,
                            int size);

        bool processSignalMessage(Stream* stream,
                            int type,
                            const char* payload,
                            int size);

        // Process a signal message.
        void processSignalMessage(unsigned int addr,
                            int type,
                            const char* payload,
                            int size);

        // Finalize the Socket.
        void destroy(StreamError error);


        static const int HANDSHAKE_SIZE = 9;
        static const int HANDSHAKE_RESP_SIZE = 5;

        static const int SUCCESS = 0;
        static const int REDIRECT = 1;
        static const int CUSTOM_ERR_CODE = 0xf;
        
        static SocketMap m_availableSockets;
        static pthread_mutex_t m_socketMutex;

        pthread_mutex_t m_streamRefMutex;
        pthread_mutex_t m_destroyingMutex;
        pthread_mutex_t m_closingMutex;
        pthread_mutex_t m_openStreamsMutex;
        pthread_mutex_t m_openWaitMutex;
        pthread_mutex_t m_pendingMutex;

        bool m_connecting;
        bool m_connected;
        bool m_handshaked;
        bool m_destroying;
        bool m_closing;

        std::string m_host;
        unsigned short m_port;
        int m_socketFDS;

        OpenRequestMap m_pendingOpenRequests;
        StreamMap m_openStreams;
        OpenRequestQueueMap m_openWaitQueue;
        
        int m_streamRefCount;
        
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

