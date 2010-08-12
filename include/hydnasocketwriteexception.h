/**
 * hydnasocketwriteexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNASOCKETWRITEEXCEPTION_H
#define HYDNASOCKETWRITEEXCEPTION_H

#include <stdexcept>
 
class HydnaSocketWriteException : public std::runtime_error {
public:
    HydnaSocketWriteException() : std::runtime_error("HydnaSocketWriteException") {}

    virtual const char* what() const throw() {
        return "Could not write to the socket";
    }
    
};

#endif

