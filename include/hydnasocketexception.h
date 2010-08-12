/**
 * hydnasocketexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNASOCKETEXCEPTION_H
#define HYDNASOCKETEXCEPTION_H

#include <stdexcept>
 
class HydnaSocketException : public std::runtime_error {
public:
    HydnaSocketException() : std::runtime_error("HydnaSocketException") {}

    virtual const char* what() const throw() {
        return "Could not create the socket";
    }
    
};

#endif

