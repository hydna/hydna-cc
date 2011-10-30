#ifndef HYDNA_URLPARSER_H
#define HYDNA_URLPARSER_H

#include <iostream>
#include <streambuf>
#include <pthread.h>

#include "connection.h"
#include "openrequest.h"
#include "channeldata.h"
#include "channelsignal.h"
#include "channelmode.h"
#include "channelerror.h"

namespace hydna {

    /**
     *  This class is used as to parese URLs within the library.
     */
    class URL {
    public:

        ~URL();

        /**
         * Parse the string and return a new URL object.
         *
         * @param expr The String to be parsed.
         * @return The URL object.
         */
        static URL parse(std::string const &expr);

        unsigned short getPort() const;

        std::string getPath() const;
        
        std::string getHost() const;

        std::string getToken() const;
        
        std::string getAuth() const;
        
        std::string getProtocol() const;
        
        std::string getError() const;
        
    private:
        /**
         *  Initializes a new URL instance.
         */
        URL();

        unsigned short m_port;
        std::string m_path;
        std::string m_host;
        std::string m_token;
        std::string m_auth;
        std::string m_protocol;
        std::string m_error;
    };
}

#endif

