/**
 * hydnapacketbuffer.cc
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <algorithm>
#include <pthread.h>
#include <exception>

#include "hydnapacketbuffer.h"
#include "hydnapacket.h"

HydnaPacketBuffer::HydnaPacketBuffer() : headerSet(false),
                                         headerOffset(0),
                                         packetOffset(0),
                                         connected(true)
{
    pthread_mutex_init(&dataMutex, NULL);
    pthread_mutex_init(&controlMutex, NULL);
    pthread_mutex_init(&openStatMutex, NULL);
    pthread_mutex_init(&connectMutex, NULL);
}

HydnaPacketBuffer::~HydnaPacketBuffer() {
    pthread_mutex_destroy(&dataMutex);
    pthread_mutex_destroy(&controlMutex);
    pthread_mutex_destroy(&openStatMutex);
    pthread_mutex_destroy(&connectMutex);

    for (AddrMutexMap::iterator it=addrMutex.begin(); it != addrMutex.end(); it++) {
        pthread_mutex_destroy(it->second);
        delete it->second;
    }
}

void HydnaPacketBuffer::pushData(unsigned char* data, unsigned int numbytes) {
    unsigned int offset = 0;
    
    while (offset < numbytes) {
        if (!headerSet) {
            while (offset < numbytes && headerOffset < HydnaPacket::HEADER_SIZE) {
                header[headerOffset] = data[offset];
                ++headerOffset;
                ++offset;
            }

            if (headerOffset == HydnaPacket::HEADER_SIZE) {
                headerSet = true;
                size = ntohs(*(unsigned short*)&header[0]);

                packet = new unsigned char[size];

                memcpy(packet, header, HydnaPacket::HEADER_SIZE);
                packetOffset += HydnaPacket::HEADER_SIZE;
            }
        }
        
        if (offset < numbytes && headerSet) {
            if (packetOffset < size) {
                int length = std::min(size - packetOffset, numbytes - offset);
                
                memcpy(packet + packetOffset, data + offset, length);
                
                packetOffset += length;
                offset += length;
            }

            if (packetOffset == size) {
                HydnaPacket* result = new HydnaPacket(packet, size);

#ifdef HYDNADEBUG
                std::cout << "Hydna(DEBUG): Received a packet of " << size << " bytes." << std::endl;
                std::cout << "Hydna(DEBUG): " << std::endl;
                std::cout << *result;
                std::cout << "Hydna(DEBUG): " << std::endl;
#endif
                
                HydnaAddr addr = result->getAddr();
                unsigned char flag = result->getFlag();

                if (flag == HydnaPacket::FLAG_DATA) {
                    pthread_mutex_lock(&dataMutex);
                    std::queue<HydnaPacket*>* queue = getDataQueue(addr);
                    queue->push(result);
                    
                    if (queue->size() == 1) {
                        pthread_mutex_t* mutex = getAddrMutex(addr);
                        pthread_mutex_unlock(mutex);
                    }
                    pthread_mutex_unlock(&dataMutex);
                } else if (flag == HydnaPacket::FLAG_OPEN_STATUS) {
                    pthread_mutex_lock(&openStatMutex);
                    openStatQueue.push(result);

                    if (openStatQueue.size() == 1) {
                        pthread_mutex_t* mutex = getAddrMutex(addr);
                        pthread_mutex_unlock(mutex);
                    }
                    pthread_mutex_unlock(&openStatMutex); 
                } else {
                    pthread_mutex_lock(&controlMutex);
                    std::queue<HydnaPacket*>* queue = getControlQueue(addr);
                    queue->push(result);
                    pthread_mutex_unlock(&controlMutex);
                }

                headerOffset = 0;
                packet = NULL;
                packetOffset = 0;

                headerSet = false;
            }    
        }
    }
}

void HydnaPacketBuffer::disconnect() {
    pthread_mutex_lock(&connectMutex);
    connected = false;

    for (AddrMutexMap::iterator it = addrMutex.begin(); it != addrMutex.end(); it++) {
        pthread_mutex_unlock(it->second);
    }
    pthread_mutex_unlock(&connectMutex);
}

HydnaPacket* HydnaPacketBuffer::getDataPacket(HydnaAddr addr) {
    HydnaPacket* result;

    pthread_mutex_t* mutex = NULL;

    pthread_mutex_lock(&connectMutex);
    if (connected) {
        mutex = getAddrMutex(addr);
        pthread_mutex_unlock(&connectMutex);
        
        pthread_mutex_lock(mutex);
    } else {
        pthread_mutex_unlock(&connectMutex);
    }
    
    pthread_mutex_lock(&dataMutex);
    
    std::queue<HydnaPacket*>* queue = getDataQueue(addr);

    if (queue->size() > 0) {
        result = queue->front();
        queue->pop();
    } else {
        result = NULL;
    }

    if (queue->size() > 0) {
        if (mutex) {
            pthread_mutex_unlock(mutex);
        }

    }
    
    pthread_mutex_unlock(&dataMutex);

    return result;
}

HydnaPacket* HydnaPacketBuffer::popDataPacket(HydnaAddr addr) {
    HydnaPacket* result;

    pthread_mutex_lock(&dataMutex);
    
    std::queue<HydnaPacket*>* queue = getDataQueue(addr);

    if (queue->size() > 0) {
        result = queue->front();
        queue->pop();
    } else {
        result = NULL;
    }
    
    pthread_mutex_unlock(&dataMutex);

    return result;
}

HydnaPacket* HydnaPacketBuffer::popControlPacket(HydnaAddr addr) {
    HydnaPacket* result;

    pthread_mutex_lock(&controlMutex);
    
    std::queue<HydnaPacket*>* queue = getControlQueue(addr);

    if (queue->size() > 0) {
        result = queue->front();
        queue->pop();
    } else {
        result = NULL;
    }
    
    pthread_mutex_unlock(&controlMutex);

    return result;
}

HydnaPacket* HydnaPacketBuffer::getOpenStatPacket(HydnaAddr addr) {
    HydnaPacket* result;

    pthread_mutex_t* mutex = NULL;

    pthread_mutex_lock(&connectMutex);
    if (connected) {
        mutex = getAddrMutex(addr);
        pthread_mutex_unlock(&connectMutex);
        
        pthread_mutex_lock(mutex);
    } else {
        pthread_mutex_unlock(&connectMutex);
    }
    
    pthread_mutex_lock(&openStatMutex);
    
    std::queue<HydnaPacket*>* queue = getDataQueue(addr);

    if (openStatQueue.size() > 0) {
        result = openStatQueue.front();
        openStatQueue.pop();
    } else {
        result = NULL;
    }

    if (queue->size() > 0) {
        if (mutex) {
            pthread_mutex_unlock(mutex);
        }
    }
    
    pthread_mutex_unlock(&openStatMutex);

    return result;
}

std::queue<HydnaPacket*>* HydnaPacketBuffer::getControlQueue(HydnaAddr addr) {
    std::queue<HydnaPacket*>* result = controlPackets[addr];

    if (!result) {
        result = new std::queue<HydnaPacket*>();
        controlPackets[addr] = result;
    }

    return result;
}

std::queue<HydnaPacket*>* HydnaPacketBuffer::getDataQueue(HydnaAddr addr) {
    std::queue<HydnaPacket*>* result = dataPackets[addr];

    if (!result) {
        result = new std::queue<HydnaPacket*>();
        dataPackets[addr] = result;
    }

    return result;
}

pthread_mutex_t* HydnaPacketBuffer::getAddrMutex(HydnaAddr addr) {
    pthread_mutex_t* result = addrMutex[addr];
    
    if (!result) {
        result = new pthread_mutex_t;

        pthread_mutex_init(result, NULL);
        pthread_mutex_lock(result);
        addrMutex[addr] = result;
    }

    return result;
}

