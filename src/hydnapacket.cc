/**
 * hydnapacket.cc
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <vector>

#include "hydnapacket.h"
#include "hydnaaddr.h"

HydnaPacket::HydnaPacket(unsigned char* data, unsigned short size) {
    HydnaPacket::data = data;
    HydnaPacket::size = size;
}

HydnaPacket::HydnaPacket(unsigned char flag,
                         HydnaAddr addr,
                         unsigned short contentSize)
{
    size = HEADER_SIZE + contentSize;
    data = new unsigned char[size];
    *(unsigned short*)&data[TSIZE_OFFSET] = htons(size);
    data[FLAG_OFFSET]  = flag;
    data[RESERVED_OFFSET] = 0;

    // TODO Maybe a separate content pointer
    for (unsigned int i = 0; i < ADDR_SIZE; i++) {
        data[ADDR_OFFSET + i]  = addr[i];
    }

#ifdef HYDNADEBUG
    std::cout << "Hydna(DEBUG): Created a packet of " << size << " bytes." << std::endl;
#endif
}

HydnaPacket::~HydnaPacket() {
    delete[] data;
}

HydnaPacket HydnaPacket::open(HydnaAddr addr) {
    HydnaPacket p(FLAG_OPEN, addr, 2);

    p.data[CONTENT_OFFSET] = 0x4;
    p.data[CONTENT_OFFSET + 1] = 0x00;

#ifdef HYDNADEBUG
    std::cout << "Hydna(DEBUG): " << std::endl;
    std::cout << p;
    std::cout << "Hydna(DEBUG): " << std::endl;
#endif

    return p;
}

HydnaPacket HydnaPacket::close(HydnaAddr addr) {
    HydnaPacket p(FLAG_CLOSE, addr, 0);

#ifdef HYDNADEBUG
    std::cout << "Hydna(DEBUG): " << std::endl;
    std::cout << p;
    std::cout << "Hydna(DEBUG): " << std::endl;
#endif

    return p;
}

HydnaPacket HydnaPacket::emit(HydnaAddr addr, unsigned char *content , unsigned short contentSize) {
    HydnaPacket p(FLAG_EMIT, addr, contentSize);

    memcpy(p.data + CONTENT_OFFSET, content, contentSize);

#ifdef HYDNADEBUG
    std::cout << "Hydna(DEBUG): " << std::endl;
    std::cout << p;
    std::cout << "Hydna(DEBUG): " << std::endl;
#endif

    return p;
}

unsigned char* HydnaPacket::getData() const {
    return data;
}

unsigned short HydnaPacket::getSize() const {
    return ntohs(*(unsigned short*)&data[TSIZE_OFFSET]);
}

unsigned char HydnaPacket::getFlag() const {
    return data[FLAG_OFFSET];
}

HydnaAddr HydnaPacket::getAddr() const {
    return HydnaAddr(&data[ADDR_OFFSET]);
}

unsigned char HydnaPacket::getMode() const {
    return data[MODE_OFFSET];
}

unsigned char HydnaPacket::getToken() const {
    return data[TOKEN_OFFSET];
}

unsigned char HydnaPacket::getCode() const {
    return data[CODE_OFFSET];
}

HydnaAddr HydnaPacket::getRAddr() const {
    return HydnaAddr(&data[RADDR_OFFSET]);
}

