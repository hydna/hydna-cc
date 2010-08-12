/**
 * hydna.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNA_H
#define HYDNA_H

#include <iostream>
#include <vector>
#include <map>
#include <pthread.h>

#include "hydnaaddr.h"
#include "hydnapacketbuffer.h"
#include "hydnapacket.h"
#include "hydnastream.h"

typedef std::map<HydnaAddr, HydnaAddr> AddrMap;

/**
 * A struct with args that 
 * the new thread is using.
 */
struct ListenArgs {
    int socketFDS;
    HydnaPacketBuffer* buffer;
    unsigned int bufSize;
    bool *connected;
    bool *socketClosed;
    bool *socketReadException;
    pthread_mutex_t *socketStatusMutex;
};

/**
 * A class to both receive and send packets.
 * A new thread is started in the constructor
 * that listens for incoming packets.
 */
class Hydna {
public:
    /**
     * Makes a client connect
     * to a specific host on a specific port.
     *
     * @param host The hostname of the server
     * @param port The port of the server
     * @param bufSize The number of chars to read
     */
    Hydna(const std::string &host, unsigned int port, unsigned int bufSize=1000);

    /**
     * Closes all open addresses.
     */
    ~Hydna();

    
    /**
     * Sends a packet.
     * Blocks until the whole packet is sent.
     *
     * @param packet The packet to be sent
     */
    void sendPacket(HydnaPacket &packet);

    /**
     * Sends a open packet to an address and
     * wraps a iosocket for sending and receiving data.
     *
     * @param addr The address
     * @return The iostream
     */
    HydnaStream* openAddr(HydnaAddr addr);
    
    /**
     * Closes an address.
     *
     * @param addr The address
     */
    void closeAddr(HydnaAddr addr);

    /**
     * Sends data to an address. Blocking.
     *
     * @param addr The address
     * @param data A pointer to the data
     * @param size The size of the data
     */
    void sendData(HydnaAddr addr, unsigned char* data, unsigned short size);
    
    /**
     * Pops a data packet from the buffer. Blocking.
     * Used by the iostream.
     *
     * @param addr The address
     * @return The packet
     */
    HydnaPacket* getDataPacket(HydnaAddr addr);
    
    /**
     * Pops a data packet from the buffer. Non-nlocking.
     *
     * @param addr The address
     * @return The packet
     */
    HydnaPacket* popDataPacket(HydnaAddr addr);

    /**
     * Pops a control packet from the buffer. Non-blocking.
     *
     * @param addr The address
     * @return The packet
     */
    HydnaPacket* popControlPacket(HydnaAddr addr);

private:
    int socketFDS;
    HydnaPacketBuffer* buffer;
    bool *connected;
    unsigned int bufSize;
                
    pthread_t listeningThread;

    AddrMap *addrRedirectMap;

    bool *socketClosed;
    bool *socketReadException;
    pthread_mutex_t* socketStatusMutex;
    bool disconnecting;

    /**
     * The method that is called in the new thread.
     * Listens for incoming packets.
     *
     * @param ptr A pointer to a ListenArgs struct.
     * @return NULL
     */
    static void* listen(void *ptr);

};

#endif

