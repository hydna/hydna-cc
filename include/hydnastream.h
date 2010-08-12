/**
 * hydnastream.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNASTREAM_H
#define HYDNASTREAM_H

#include <iostream>
#include <streambuf>

#include "hydnaaddr.h"
#include "hydnastreambuf.h"

class Hydna;

/**
 * An iostream wrapper around streambuf.
 */
class HydnaStream : public std::iostream {
public:
    HydnaStream(Hydna* hydna, HydnaAddr addr, unsigned int bufSize);

    ~HydnaStream();
    
private:
    HydnaStreamBuf buf;

};

#endif


