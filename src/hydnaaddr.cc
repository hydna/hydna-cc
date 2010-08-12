/**
 * hydnaaddr.cc
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

#include "hydnaaddr.h"
#include "hydnaaddrexception.h"

HydnaAddr::HydnaAddr(std::string const &addrString) {
    std::string compString = "";
    unsigned int l = addrString.length();
    unsigned short num = 0;
    unsigned short index = 0;

    if (l == 39 && addrString.find(":") != std::string::npos) {
        for(int i = 0; i < 39; i += 5) {
            std::istringstream iss(addrString.substr(i, 4));
            
            if ((iss >> std::setbase(16) >> num).fail()) {
                break;
            }

            *(unsigned short*)&addr[index] = ntohs(num);

            index += 2;
        }

        compString = addrString;
    } else if (l == 32) {
        for(int i = 0; i < 32; i += 4) {
            std::string numString = addrString.substr(i, 4);
            std::istringstream iss(numString);

            if ((iss >> std::setbase(16) >> num).fail()) {
                break;
            }

            *(unsigned short*)&addr[index] = ntohs(num);

            index += 2;

            compString += numString;

            if (index != 16) {
                compString += ":";
            }
        }
    }

    std::ostringstream oss(std::ostringstream::out);
    oss << *this;
    
    if (oss.str() != compString) {
        throw HydnaAddrException();
    }
}

HydnaAddr::HydnaAddr(unsigned int compA, unsigned int compB,
                     unsigned int compC, unsigned int compD) {
    *(unsigned int*)&addr[0] = ntohl(compA);
    *(unsigned int*)&addr[4] = ntohl(compB);
    *(unsigned int*)&addr[8] = ntohl(compC);
    *(unsigned int*)&addr[12] = ntohl(compD);
}

HydnaAddr::HydnaAddr(std::vector<unsigned char> addrVector) {
    for(unsigned int i = 0; i < 16; i++) {
        if (i < addrVector.size()) {
            addr[i] = addrVector[i];
        } else {
            addr[i] = 0;
        }
    }
}

HydnaAddr::HydnaAddr(unsigned char addrArray[]) {
    for(unsigned int i = 0; i < 16; i++) {
        addr[i] = addrArray[i];
    }
}

HydnaAddr::~HydnaAddr() {}

bool HydnaAddr::operator== (const HydnaAddr& h) const {
    for (unsigned int i = 0; i < 16; i++) {
        if (addr[i] != h.addr[i]) {
            return false;
        }
    }

    return true;
}

bool HydnaAddr::operator!= (const HydnaAddr& h) const {
    for (unsigned int i = 0; i < 16; i++) {
        if (addr[i] != h.addr[i]) {
            return true;
        }
    }

    return false;
}

bool HydnaAddr::operator< (const HydnaAddr& h) const {
    for (unsigned int i = 0; i < 16; i++) {
        if (addr[i] < h.addr[i]) {
            return true;
        }
    }

    return false;
}

unsigned char& HydnaAddr::operator[] (unsigned int i) {
    return addr[i];
}

