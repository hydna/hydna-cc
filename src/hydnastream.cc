/**
 * hydnastream.cc
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#include <iostream>
#include <streambuf>

#include "hydnastream.h"
#include "hydna.h"
#include "hydnaaddr.h"

HydnaStream::HydnaStream(Hydna* hydna, HydnaAddr addr, unsigned int bufSize) : buf(hydna, addr, bufSize) {
    init(&buf);
}

HydnaStream::~HydnaStream() {}

