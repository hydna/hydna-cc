#ifndef HYDNA_ADDR_H
#define HYDNA_ADDR_H

#include <iostream>
#include <streambuf>

namespace hydna {

    class Addr {
    public:
        /**
         *  Addr constructor. 
         */
        Addr(unsigned int zonecomp,
                unsigned int streamcomp,
                std::string const &host="",
                int port=DEFAULT_PORT);

        /**
         *  Returns the underlying ByteArray instance for this HydnaAddr
         *
         *  @return {flash.utils.ByteArray} the underlying ByteArray.
         */
        unsigned int getZone() const;

        int getStream() const;
        
        int getPort() const;

        std::string getHost() const;
            
        /**
         *  Compares this HydnaAddr instance with another one.
         *
         *  @return {Boolean} true if the two instances match, else false.
         */
        bool equals(Addr addr) const;
        
        /**
         *  Converts this Addr instance into a hexa-decimal string.
         *
         */
        std::string toHex() const;
        
        /**
         *  Converts the HydnaAddr into a hex-formated string representation.
         *
         */
        std::string toString() const;
        
        /**
         *  Creates a new Addr instance based on specified expression.
         */
        static Addr fromExpr(std::string const &expr);

        /**
          *  Initializes a new Addr instance from a ByteArray
          *
          *  @param {String} buffer The ByteArray that represents the address.
          */
        static Addr fromByteArray(const char* buffer);

        std::string hexifyComponent(unsigned int comp) const;
        
    private:
        static const int ADDR_SIZE = 8;
        static const int COMP_SIZE = 4;
        
        //static var ADDR_EXPR_RE:RegExp = "/^(?:([0-9a-f]{1,8})-|([0-9a-f]{1,8})-([0-9a-f]{1,8}))$/i";
        
        static const char DEFAULT_HOST[];
        static const int DEFAULT_PORT = 7010;
        
        unsigned int m_zone;
        unsigned int m_stream;
        std::string m_host;
        int m_port;

    };
}

#endif

