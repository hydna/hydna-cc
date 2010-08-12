/**
 * hydnanotconnectedexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNANOTCONNECTEDEXCEPTION_H
#define HYDNANOTCONNECTEDEXCEPTION_H

#include <stdexcept>
 
class HydnaNotConnectedException : public std::runtime_error {
public:
    HydnaNotConnectedException() : std::runtime_error("HydnaNotConnectedException") {}

    virtual const char* what() const throw() {
        return "Hydna is not connect to any host";
    }
    
};

#endif


