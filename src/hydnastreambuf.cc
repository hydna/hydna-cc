/**
 * hydnastreambuf.cc
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#include <iostream>
#include <streambuf>

#include "hydnastreambuf.h"
#include "hydna.h"

HydnaStreamBuf::HydnaStreamBuf(Hydna* hydna, HydnaAddr addr, unsigned int bufSize) :
        hydna(hydna), addr(addr), out(new char[bufSize]),
        currentPacket(NULL), bufSize(bufSize)
{
    setg(0, 0, 0);
    setp(out, out + bufSize - 1);
}

HydnaStreamBuf::~HydnaStreamBuf() {
    delete[] out;
}

int HydnaStreamBuf::sync() {
    if (pptr() - pbase() != 0) {
        hydna->sendData(addr, reinterpret_cast<unsigned char*>(out), pptr() - pbase());
    
        setp(out, out + bufSize - 1);
    }

    return 0;
}


int HydnaStreamBuf::underflow() {
    delete currentPacket;
    currentPacket = hydna->getDataPacket(addr);

    if (!currentPacket) {
        std::cout << "NULL packet" << std::endl;
        return traits_type::eof();
    }
    
    char* gPtr = reinterpret_cast<char *>(currentPacket->getData());

    gPtr += HydnaPacket::HEADER_SIZE;

    setg(gPtr, gPtr, gPtr - HydnaPacket::HEADER_SIZE + currentPacket->getSize());
    return traits_type::to_int_type(*gptr());
}

int HydnaStreamBuf::overflow (int c) {
    if (!traits_type::eq_int_type(traits_type::eof(), c)) {
        traits_type::assign(*pptr(), traits_type::to_char_type(c));
        pbump(1);
    }

    return sync() == 0? traits_type::not_eof(c): traits_type::eof();
}


