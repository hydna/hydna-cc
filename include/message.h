#ifndef HYDNA_MESSAGE_H
#define HYDNA_MESSAGE_H

#include <iostream>
#include <vector>

typedef std::vector<char> ByteArray;

namespace hydna {
    class Message { //: public ByteArray {
    public:
        static const int HEADER_SIZE = 0x08;
    
        static const int OPEN   = 0x01;
        static const int DATA   = 0x02;
        static const int SIGNAL = 0x03;

        static const int END = 0x01;
        
        Message(unsigned int addr,
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
        
    private:
        ByteArray bytes;

    };
}

#endif


