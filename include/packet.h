#ifndef HYDNA_PACKET_H
#define HYDNA_PACKET_H

#include <iostream>
#include <vector>

typedef std::vector<char> ByteArray;

namespace hydna {
    class Packet {
    public:

        static const int HEADER_SIZE = 0x07;
    
        // Opcodes
        static const int NOOP   = 0x00;
        static const int OPEN   = 0x01;
        static const int DATA   = 0x02;
        static const int SIGNAL = 0x03;

        // Open Flags
        static const int OPEN_ALLOW = 0x0;
        static const int OPEN_REDIRECT = 0x1;
        static const int OPEN_DENY = 0x7;

        // Signal Flags
        static const int SIG_EMIT = 0x0;
        static const int SIG_END = 0x1;
        static const int SIG_ERROR = 0x7;
        
        // Upper payload limit (10kb)
        static const unsigned int PAYLOAD_MAX_LIMIT = 10 * 1024;

        Packet(unsigned int ch,
                unsigned int op,
                unsigned int flag=0,
                const char* payload=NULL,
                unsigned int offset=0,
                unsigned int length=0);
        
        void writeByte(char value);
        void writeBytes(const char* value, int offset, int length);
        void writeShort(short value);
        void writeUnsignedInt(unsigned int value);

        int getSize();
        char* getData();

        void setChannel(unsigned int value);
        
    private:
        ByteArray bytes;

    };
}

#endif


