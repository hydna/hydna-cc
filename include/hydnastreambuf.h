/**
 * hydnastreambuf.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNASTREAMBUF_H
#define HYDNASTREAMBUF_H

#include <iostream>
#include <streambuf>

#include "hydnaaddr.h"
#include "hydnapacket.h"

class Hydna;

/**
 * A streambuf that can be used like regular streams in c++
 * but this stream sends an emit packet with the data as content.
 *
 * It blocks on read if no data is available.
 */
class HydnaStreamBuf : public std::streambuf {
public:
    HydnaStreamBuf(Hydna *hydna, HydnaAddr addr, unsigned int bufSize);

    ~HydnaStreamBuf();

    /**
     * This is called when the local buffer is full
     * and a packet should be sent with Hydna.
     * It can also be called by a flush.
     */
    int sync();

    /**
     * This is called when trying to read an empty local buffer.
     * It blocks until data is available.
     */
    int underflow();

    /**
     * This is called when the local buffer is one char from full.
     * It calls sync().
     */
    int overflow(int c = EOF);
    
private:
    Hydna* hydna;
    HydnaAddr addr;
    unsigned char** in;
    char* out;
    HydnaPacket* currentPacket;
    unsigned int bufSize;

};

#endif



