/**
 * hydnaaddr.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNAADDR_H
#define HYDNAADDR_H

#include <iostream>
#include <vector>

/**
 * A class that holds a key with 1-8 components.
 * The components are saved unsigned inside the class
 * but return signed with the getter.
 */
class HydnaAddr {
public:
    static const unsigned int KEY_INDEX_OUT_OF_RANGE = 0;

    /**
     * The way ints are written on hex format is how
     * they are sent over the network. For example
     * 0x0, 0xAABBCCDD, 0x122BDC 0x1
     *
     * @param compA The first 4 bytes.
     * @param compB The second 4 bytes.
     * @param compC The third 4 bytes.
     * @param compD The fourth 4 bytes.
     */
    HydnaAddr(unsigned int compA, unsigned int compB,
              unsigned int compC, unsigned int compD);

    /**
     * Accepted string formats is
     * aaaa:aaae:fffd:3443:2355:2352:2342:2352 and
     * aaaaaaaefffd34432355235223422352
     */
    HydnaAddr(std::string const &addrString);

    /**
     * Creates an address from a vector.
     * The back of the address will be filled with zeros
     * if the vector is not long enough.
     *
     * @param addr The address vector
     */
    HydnaAddr(std::vector<unsigned char> addr);

    /**
     * Creates an addres from an array.
     *
     * @param addr The address array.
     */
    HydnaAddr(unsigned char addr[]);

    ~HydnaAddr();
    
    bool operator== (const HydnaAddr& h) const;
    bool operator!= (const HydnaAddr& h) const;
    bool operator< (const HydnaAddr& h) const;
    unsigned char& operator[] (unsigned int i);
    
private:
    unsigned char addr[16];

    /**
     * A 'toString' method that separates the address with ':'.
     */
    friend std::ostream& operator<< (std::ostream& os, const HydnaAddr& ha) {
        for (unsigned int i = 0; i < 16; i++) {
            os.width(2);
            os.fill('0');
            os << std::hex << (unsigned short)ha.addr[i];

            if ((i + 1) % 2 == 0 && i != 15) {
                os << ":";
            }
        }

        os << std::dec;

        return os;
    }
};

#endif

