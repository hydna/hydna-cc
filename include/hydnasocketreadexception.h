/**
 * hydnasocketreadexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNASOCKETREADEXCEPTION_H
#define HYDNASOCKETREADEXCEPTION_H

#include <stdexcept>
 
class HydnaSocketReadException : public std::runtime_error {
public:
    HydnaSocketReadException() : std::runtime_error("HydnaSocketReadException") {}

    virtual const char* what() const throw() {
        return "Could not read from the socket";
    }
    
};

#endif

