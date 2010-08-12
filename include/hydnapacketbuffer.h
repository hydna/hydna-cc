/**
 * hydnapacketbuffer.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNAPACKETBUFFER_H
#define HYDNAPACKETBUFFER_H

#include <map>
#include <queue>
#include <pthread.h>

#include "hydnapacket.h"


typedef std::queue<HydnaPacket*> PacketQueue;
typedef std::map<HydnaAddr, PacketQueue* > AddrPacketMap;
typedef std::map<HydnaAddr, pthread_mutex_t*> AddrMutexMap;

/**
 * A class to buffer the incoming data to the client.
 * This class holds an uncompleted packet.
 */
class HydnaPacketBuffer {
public:
    HydnaPacketBuffer();

    ~HydnaPacketBuffer();
    
    /**
     * Add data to the buffer. The buffer is responsible
     * for where the data should be put.
     * 
     * @param data A pointer to the data
     * @param numbytes The number of bytes
     */
    void pushData(unsigned char *data, unsigned int numbytes);

    /**
     * Disconnect the buffer and release all locks.
     * Called by Hydna.
     */
    void disconnect();

    /**
     * Get the oldest data packet in the buffer and remove it from the buffer.
     * Blocks until a data packet is ready.
     * Returns NULL if the client disconnected.
     *
     * @param addr The address
     * @return The packet
     */
    HydnaPacket* getDataPacket(HydnaAddr addr);

    /**
     * Get the oldest data packet in the buffer and remove it from the buffer.
     * Returns NULL if no data packet is in the buffer.
     *
     * @param addr The address
     * @return The packet
     */
    HydnaPacket* popDataPacket(HydnaAddr addr);
    
    /**
     * Get the oldest control packet in the buffer and remove it from the buffer.
     * Returns NULL if no control packet is in the buffer.
     *
     * @param addr The address
     * @return The packet
     */
    HydnaPacket* popControlPacket(HydnaAddr addr);
    
    /**
     * Get the oldest open stat packet in the buffer and remove it from the buffer.
     * Blocks until an open stat packet is ready.
     * Returns NULL if the client disconnected.
     *
     * @param addr The address
     * @return The packet
     */
    HydnaPacket* getOpenStatPacket(HydnaAddr addr);

private:
    bool headerSet;

    unsigned char header[HydnaPacket::HEADER_SIZE];

    unsigned int headerOffset;
    unsigned int packetOffset;

    bool connected;

    unsigned char* packet;
    unsigned int   size;

    AddrPacketMap dataPackets;
    AddrPacketMap controlPackets;
    PacketQueue openStatQueue;

    pthread_mutex_t dataMutex;
    pthread_mutex_t controlMutex;
    pthread_mutex_t openStatMutex;
    pthread_mutex_t connectMutex;
    AddrMutexMap addrMutex;

    PacketQueue* getControlQueue(HydnaAddr addr);
    PacketQueue* getDataQueue(HydnaAddr addr);
    pthread_mutex_t* getAddrMutex(HydnaAddr addr);
};

#endif

