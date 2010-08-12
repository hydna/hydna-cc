/**
 * hydnaresolveexception.h
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#ifndef HYDNARESOLVEEXCEPTION_H
#define HYDNARESOLVEEXCEPTION_H

#include <stdexcept>
 
class HydnaResolveException : public std::runtime_error {
public:
    HydnaResolveException() : std::runtime_error("HydnaResolveException") {}

    virtual const char* what() const throw() {
        return "Could not find the host";
    }
    
};

#endif

