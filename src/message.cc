#include <iostream>
#include <netinet/in.h>

#include "message.h"
#include "addr.h"

namespace hydna {
    using namespace std;

    Message::Message(Addr& addr,
                        unsigned int op,
                        unsigned int flag,
                        const char* payload,
                        unsigned int offset,
                        unsigned int length)
    {
        if (!payload) {
            length = 0;
        }

        bytes.reserve(length + HEADER_SIZE);
        writeShort(length + HEADER_SIZE);
        writeByte(0); // Reserved
        writeUnsignedInt(addr.getStream());
        writeByte(op << 4 | flag);

        if (payload) {
            writeBytes(payload, offset, length);
        }
    }

    void Message::writeByte(char value) {
        bytes.push_back(value);
    }

    void Message::writeBytes(const char* value, int offset, int length) {
        for(int i = offset; i < offset + length; i++) {
            bytes.push_back(value[i]);
        }
    }
    
    void Message::writeShort(short value) {
        char result[2];

        *(short*)&result[0] = htons(value);

        bytes.push_back(result[0]);
        bytes.push_back(result[1]);
    }

    void Message::writeUnsignedInt(unsigned int value) {
        char result[4];

        *(unsigned int*)&result[0] = htonl(value);

        bytes.push_back(result[0]);
        bytes.push_back(result[1]);
        bytes.push_back(result[2]);
        bytes.push_back(result[3]);
    }

    int Message::getSize() {
        return bytes.size();
    }

    char* Message::getData() {
        return &bytes[0];
    }
}

