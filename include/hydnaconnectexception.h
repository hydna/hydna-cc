/**
 * hydnaconnectexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNACONNECTEXCEPTION_H
#define HYDNACONNECTEXCEPTION_H

#include <stdexcept>
 
class HydnaConnectException : public std::runtime_error {
public:
    HydnaConnectException() : std::runtime_error("HydnaConnectException") {}

    virtual const char* what() const throw() {
        return "Could not connect to the host";
    }
    
};

#endif


