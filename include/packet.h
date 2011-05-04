#ifndef HYDNA_PACKET_H
#define HYDNA_PACKET_H

#include <iostream>
#include <vector>

typedef std::vector<char> ByteArray;

namespace hydna {
    class Packet {
    public:

        static const int HEADER_SIZE = 0x08;
    
        // Opcodes
        static const int OPEN   = 0x01;
        static const int DATA   = 0x02;
        static const int SIGNAL = 0x03;

        // Handshake flags
        static const int HANDSHAKE_UNKNOWN = 0x01;
        static const int HANDSHAKE_SERVER_BUSY = 0x02;
        static const int HANDSHAKE_BADFORMAT = 0x03;
        static const int HANDSHAKE_HOSTNAME = 0x04;
        static const int HANDSHAKE_PROTOCOL = 0x05;
        static const int HANDSHAKE_SERVER_ERROR = 0x06;

        // Open Flags
        static const int OPEN_SUCCESS = 0x0;
        static const int OPEN_REDIRECT = 0x1;
        static const int OPEN_FAIL_NA = 0x8;
        static const int OPEN_FAIL_MODE = 0x9;
        static const int OPEN_FAIL_PROTOCOL = 0xa;
        static const int OPEN_FAIL_HOST = 0xb;
        static const int OPEN_FAIL_AUTH = 0xc;
        static const int OPEN_FAIL_SERVICE_NA = 0xd;
        static const int OPEN_FAIL_SERVICE_ERR = 0xe;
        static const int OPEN_FAIL_OTHER = 0xf;

        // Signal Flags
        static const int SIG_EMIT = 0x0;
        static const int SIG_END = 0x1;
        static const int SIG_ERR_PROTOCOL = 0xa;
        static const int SIG_ERR_OPERATION = 0xb;
        static const int SIG_ERR_LIMIT = 0xc;
        static const int SIG_ERR_SERVER = 0xd;
        static const int SIG_ERR_VIOLATION = 0xe;
        static const int SIG_ERR_OTHER = 0xf;
        
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


