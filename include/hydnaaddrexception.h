/**
 * hydnaaddrexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNAADDREXCEPTION_H
#define HYDNAADDREXCEPTION_H

#include <stdexcept>
 
class HydnaAddrException : public std::runtime_error {
public:
    HydnaAddrException() : std::runtime_error("HydnaAddrException") {}

    virtual const char* what() const throw() {
        return "The string could not be coverted to an address";
    }
    
};

#endif
