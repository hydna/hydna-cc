/**
 * hydnalisteningexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNALISTENINGEXCEPTION_H
#define HYDNALISTENINGEXCEPTION_H

#include <stdexcept>
 
class HydnaListeningException : public std::runtime_error {
public:
    HydnaListeningException() : std::runtime_error("HydnaListeningException") {}

    virtual const char* what() const throw() {
        return "Could not create a new thread for listening";
    }
    
};

#endif



