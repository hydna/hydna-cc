#ifndef HYDNA_EXTSOCKET_H
#define HYDNA_EXTSOCKET_H

#include <iostream>
#include <streambuf>
#include <map>

#include "openrequest.h"
#include "streamerror.h"

namespace hydna {
    class Addr;
    class Message;
    class Stream;

    typedef std::map<unsigned int, Stream*> StreamMap;


    class ExtSocket { //: public Socket {

        typedef std::map<unsigned int, ExtSocket*> SocketMap;

    public:
        // Return an available socket or create a new one.
        static ExtSocket* getSocket(Addr addr);

        /**
         *  Initializes a new Stream instance
         */
        ExtSocket(unsigned int zoneid);
        
        bool hasHandshaked() const;
        
        // Internal method to keep track of no of streams that is associated 
        // with this connection instance.
        void allocStream();
        
        // Decrease the reference count
        void deallocStream(Addr& addr);

        // Request to open a stream. Return true if request went well, else
        // false.
        bool requestOpen(OpenRequest* request);
        
        // Try to cancel an open request. Returns true on success else
        // false.
        bool cancelOpen(OpenRequest* request);
        
        void connectSocket(std::string &host, int port);

        //  Send a handshake packet and wait for a handshake
        // response packet in return.
        void connectHandler(); //Event event);

        // Handle the Handshake response packet.
        void handshakeHandler(); //ProgressEvent event);

        // Handles all incomming data.
        void receiveHandler(); //ProgressEvent event);

        // process a open response message
        void processOpenMessage(unsigned int addr,
                                int errcode,
                                const char* payload,
                                int size);

        // process a data message
        void processDataMessage(unsigned int addr,
                            int priority,
                            const char* payload,
                            int size);

        // process a signal message
        void processSignalMessage(unsigned int addr,
                            int type,
                            const char* payload,
                            int size);

        // Finalize the Socket
        void destroy(StreamError error);


        bool writeBytes(Message& message);
        void writeMultiByte(std::string& value);
        void writeUnsignedInt(unsigned int value);

        void readByte();
        void readBytes();
        void readMultiBytes();
        
    private:
        static const int HANDSHAKE_SIZE = 9;
        static const int HANDSHAKE_RESP_SIZE = 5;

        static const int SUCCESS = 0;
        static const int REDIRECT = 1;
        static const int CUSTOM_ERR_CODE = 0xf;
        
        static SocketMap availableSockets;

        bool m_connecting;
        bool m_connected;
        bool m_handshaked;
        bool m_destroying;

        unsigned int m_zone;
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

    typedef std::map<unsigned int, ExtSocket*> SocketMap;

    /**
     * A struct with args that 
     * the new thread is using.
     */
    struct ListenArgs {
        ExtSocket* extSocket;
    };
}

#endif

