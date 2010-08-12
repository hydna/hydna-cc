/**
 * hydnapacket.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNAPACKET_H
#define HYDNAPACKET_H

#include <iostream>
#include <vector>

#include "hydnaaddr.h";

/**
 * A class to hold a complete packet.
 */
class HydnaPacket {
public:
    static const unsigned int    TSIZE_OFFSET = 0;
    static const unsigned int     FLAG_OFFSET = 2;
    static const unsigned int RESERVED_OFFSET = 3;
    static const unsigned int     ADDR_OFFSET = 4;
    static const unsigned int  CONTENT_OFFSET = 20;
    static const unsigned int     MODE_OFFSET = 20;
    static const unsigned int    TOKEN_OFFSET = 21;
    static const unsigned int     CODE_OFFSET = 20;
    static const unsigned int    RADDR_OFFSET = 21;

    static const unsigned int      TSIZE_SIZE = 2;
    static const unsigned int       FLAG_SIZE = 1;
    static const unsigned int   RESERVED_SIZE = 1;
    static const unsigned int       ADDR_SIZE = 16;
    static const unsigned int     HEADER_SIZE = 20;

    static const char               FLAG_OPEN = 0x2;
    static const char               FLAG_EMIT = 0x4;
    static const char              FLAG_CLOSE = 0x5;
    static const char        FLAG_OPEN_STATUS = 0x6;
    static const char               FLAG_DATA = 0x8;

    static const char               MODE_READ = 0x1;
    static const char              MODE_WRITE = 0x2;
    static const char          MODE_READWRITE = 0x4;

    static const int CONTENT_MAX_SIZE = 10 * 1024;

    /**
     * A constructor that makes a packet of a buffer.
     *
     * @param data The data of the packet
     * @param size The size of the data
     */
    HydnaPacket(unsigned char* data, unsigned short size);

    /**
     * A constructor that makes a packet of some datatypes.
     * The total size will be calculated.
     *
     * @param flag The flag of the packet
     * @param addr The address of the packet
     * @param contentSize The size of the content
     */
    HydnaPacket(unsigned char flag,
                HydnaAddr addr,
                unsigned short contentSize = 0);

    ~HydnaPacket();
    
    /**
     * A method for creating an open packet.
     *
     * @param addr The address
     */
    static HydnaPacket open(HydnaAddr addr);

    /**
     * A method for creating an close packet.
     *
     * @param addr The address
     */
    static HydnaPacket close(HydnaAddr addr);

    /**
     * A method for creating an emit packet.
     *
     * @param addr The address
     * @param content The content to be emitted
     * @param contentSize The size of the content
     */
    static HydnaPacket emit(HydnaAddr addr, unsigned char *content, unsigned short contentSize);

    /**
     * A method to get the packet for sending over the network.
     * Network order.
     *
     * @return A pointer to the data
     */
    unsigned char* getData() const;

    /**
     * A method to get the size of the packet for sending over the network.
     *
     * @return The size
     */
    unsigned short getSize() const;

    /**
     * Returns the flag of the packet.
     *
     * @return The flag
     */
    unsigned char getFlag() const;

    /**
     * Returns the address of the packet.
     *
     * @return The address
     */
    HydnaAddr getAddr() const;

    /**
     * Returns the mode of an open packet.
     *
     * @return The mode
     */
    unsigned char getMode() const;

    /**
     * Returns the token of an open packet.
     *
     * @return The token
     */
    unsigned char getToken() const;

    /**
     * Returns the code of an open status packet.
     *
     * @return The code
     */
    unsigned char getCode() const;

    /**
     * Returns the return address of a open status packet.
     *
     * @return The return address
     */
    HydnaAddr getRAddr() const;

private:
    unsigned char *data;
    unsigned int size;

    /**
     * A 'toString' method that prints the info of the packet.
     */
    friend std::ostream& operator<< (std::ostream& os, const HydnaPacket& p) {
        unsigned short flag = p.getFlag();

        os << "Hydna(DEBUG):  Total size: " << p.getSize() << std::endl;
        os << "Hydna(DEBUG):        Flag: 0x";
        os.width(2);
        os.fill('0');
        os << std::hex << flag << std::dec;

        switch (flag) {
            case FLAG_OPEN:
                os << " (Open)";
                break;
            case FLAG_EMIT:
                os << " (Emit)";
                break;
            case FLAG_CLOSE:
                os << " (Close)";
                break;
            case FLAG_OPEN_STATUS:
                os << " (Open Status)";
                break;
            case FLAG_DATA:
                os << " (Data)";
                break;
        }

        os << std::endl;
        os << "Hydna(DEBUG):   HydnaAddr: " << p.getAddr() << std::endl;

        switch (flag) {
            case FLAG_OPEN: {
                unsigned short mode = p.getMode();
                os << "Hydna(DEBUG):        Mode: 0x";
                os.width(2);
                os.fill('0');
                os << std::hex << mode << std::dec;

                switch (mode) {
                    case MODE_READ:
                        os << " (Read)";
                        break;
                    case MODE_WRITE:
                        os << " (Write)";
                        break;
                    case MODE_READWRITE:
                        os << " (Read/Write)";
                        break;
                }

                os << std::endl;

                os << "Hydna(DEBUG):       Token: " << static_cast<unsigned int>(p.getToken()) << std::endl;

                break;
            }
            case FLAG_EMIT:
                os << "Hydna(DEBUG):        Data: ";
                
                for (unsigned int i = HEADER_SIZE; i < p.getSize(); i++) {
                    os << (unsigned short)(p.getData()[i]) << " ";
                }

                os << std::endl;

                break;
            case FLAG_OPEN_STATUS:
                os << "Hydna(DEBUG):        Code: 0x";
                os.width(2);
                os.fill('0');
                os << std::hex << static_cast<unsigned int>(p.getCode()) << std::dec << std::endl;

                os << "Hydna(DEBUG): Return addr: " << p.getRAddr() << std::endl;

                break;
            case FLAG_DATA:
                os << "Hydna(DEBUG):        Data: ";
                
                for (unsigned int i = HEADER_SIZE; i < p.getSize(); i++) {
                    os << (unsigned short)(p.getData()[i]) << " ";
                }
                
                os << std::endl;
                
                break;
        }

        return os;
    }
};

#endif

