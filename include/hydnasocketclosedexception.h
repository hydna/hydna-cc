/**
 * hydnasocketclosedexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNASOCKETCLOSEDEXCEPTION_H
#define HYDNASOCKETCLOSEDEXCEPTION_H

#include <stdexcept>
 
class HydnaSocketClosedException : public std::runtime_error {
public:
    HydnaSocketClosedException() : std::runtime_error("HydnaSocketClosedException") {}

    virtual const char* what() const throw() {
        return "The socket has been closed by the portal";
    }
    
};

#endif

