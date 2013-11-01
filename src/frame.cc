#include <iostream>
#include <netinet/in.h>

#include "frame.h"
#include "rangeerror.h"

namespace hydna {
    using namespace std;
    
    Frame::Frame(unsigned int ch,
                        unsigned int ctype,
                        unsigned int op,
                        unsigned int flag,
                        const char* payload,
                        unsigned int offset,
                        unsigned int length)
    {
        if (!payload) {
            length = 0;
        }

        if (length > PAYLOAD_MAX_LIMIT) {
            throw new RangeError("Payload max limit reached.");
        }

        bytes.reserve(length + HEADER_SIZE);
        writeShort(length + HEADER_SIZE);
        writeUnsignedInt(ch);
        
        writeByte(((ctype << Frame::CTYPE_BITPOS) | (op << Frame::OP_BITPOS) | (flag & 7)));

        if (payload) {
            writeBytes(payload, offset, length);
        }
    }

    void Frame::writeByte(char value) {
        bytes.push_back(value);
    }

    void Frame::writeBytes(const char* value, int offset, int length) {
        for(int i = offset; i < offset + length; i++) {
            bytes.push_back(value[i]);
        }
    }
    
    void Frame::writeShort(short value) {
        char result[2];

        *(short*)&result[0] = htons(value);

        bytes.push_back(result[0]);
        bytes.push_back(result[1]);
    }

    void Frame::writeUnsignedInt(unsigned int value) {
        char result[4];

        *(unsigned int*)&result[0] = htonl(value);

        bytes.push_back(result[0]);
        bytes.push_back(result[1]);
        bytes.push_back(result[2]);
        bytes.push_back(result[3]);
    }

    int Frame::getSize() {
        return bytes.size();
    }

    char* Frame::getData() {
        return &bytes[0];
    }

    void Frame::setChannel(unsigned int value) {
        char result[4];

        *(unsigned int*)&result[0] = htonl(value);

        bytes[3] = result[0];
        bytes[4] = result[1];
        bytes[5] = result[2];
        bytes[6] = result[3];
    }    
}

