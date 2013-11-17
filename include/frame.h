#ifndef HYDNA_FRAME_H
#define HYDNA_FRAME_H

#include <iostream>
#include <vector>

typedef std::vector<char> ByteArray;

namespace hydna {
    class Frame {
    public:

        static const int HEADER_SIZE = 0x07;
        static const unsigned int RESOLVE_CHANNEL = 0x00;
    
        // Opcodes
        static const int KEEPALIVE = 0x00;
        static const int NOOP   = 0x00;
        static const int OPEN   = 0x01;
        static const int DATA   = 0x02;
        static const int SIGNAL = 0x03;
        static const int RESOLVE = 0x04;

        // Open Flags
        static const int OPEN_ALLOW = 0x00;
        static const int OPEN_REDIRECT = 0x01;
        static const int OPEN_DENY = 0x07;

        // Signal Flags
        static const int SIG_EMIT = 0x00;
        static const int SIG_END = 0x01;
        static const int SIG_ERROR = 0x07;
        
        // Bit masks
        static const int FLAG_BITMASK = 0x07;

        static const int OP_BITPOS = 3;
        static const int OP_BITMASK = (0x07 << OP_BITPOS);

        static const int CTYPE_BITPOS = 6;
        static const int CTYPE_BITMASK = (0x01 << CTYPE_BITPOS);       
        
        // Upper payload limit (10kb)
        //static const unsigned int PAYLOAD_MAX_LIMIT = 10 * 1024;
        static const unsigned int PAYLOAD_MAX_LIMIT = 0xFFFFFF - HEADER_SIZE;

        Frame(unsigned int ch,
                unsigned int ctype=0,
                unsigned int op=0,
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
        
        char* m_token;

    };
}

#endif


