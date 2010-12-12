#ifndef HYDNA_STREAMERROR_H
#define HYDNA_STREAMERROR_H

#include "ioerror.h"

namespace hydna {

    class StreamError : public IOError {
    public:
        StreamError(std::string const &what,
                unsigned int code=0xFFFF,
                std::string const &name="StreamError") : IOError(what, name), m_code(code) {}

        static StreamError fromErrorCode(unsigned int code, std::string const &errorMessage="") {
            std::string message = "";

            switch (code) {
                // Socket related error codes (also related to handshakes)
                case 0x02: message = "Bad message format"; break;
                case 0x03: message = "Multiple ACK request to same addr"; break;
                case 0x04: message = "Invalid operator"; break;
                case 0x05: message = "Invalid operator flag"; break;
                case 0x06: message = "Stream is already open"; break;
                case 0x07: message = "Stream is not writable"; break;
                case 0x08: message = "Stream is not available"; break;
                case 0x09: message = "Server is busy"; break;
                case 0x0A: message = "Bad handshake packet"; break;
                case 0x0F: message = "Invalid domain addr"; break;
                
                // Handshake related error codes
                case 0x12: message = "Server is busy"; break;
                case 0x13: message = "Bad format"; break;
                case 0x14: message = "Invalid Zone"; break;
                
                // OPENRESP releated error codes
                case 0x108: message = "Stream is not available"; break;
                case 0x109: message = "Mode not allowed"; break;
                case 0x10a: message = "Protocol not allowed"; break;
                case 0x10b: message = "Hostname not allowed"; break;
                case 0x10c: message = "Authentication failure"; break;
                case 0x10d: message = "Service not available"; break;
                case 0x10e: message = "Service error"; break;
                case 0x10f: message = "Other error"; break;


                // SIGNAL related error codes
                case 0x201: message = "Close stream"; break;
                case 0x20a: message = "Client sent ill-formatted data"; break;
                case 0x20b: message = "Operation Error"; break;
                case 0x20c: message = "Limit reached (messages/second, message size etc)"; break;
                case 0x20d: message = "Internal server error"; break;
                case 0x20e: message = "A violation has occurred"; break;
                case 0x20f: message = "Other error"; break;

                // Stream releated error end codes
                case 0x111: message = "End of transmission"; break;
                case 0x11F: message = ""; break; // User-defined.
                            
                
                // Library specific error codes
                case 0x1001: message = "Server responed with bad handshake"; break;
                case 0x1002: message = "Server sent malformed packet"; break;
                case 0x1003: message = "Server sent invalid open response"; break;
                case 0x1004: message = "Server sent to non-open stream"; break;
                case 0x1005: message = "Server redirected to open stream"; break;
                case 0x1006: message = "Security error"; break;
                case 0x1007: message = "Stream already open."; break;
                case 0x100F: message = "Disconnected from server"; break;
                default: message = "Unknown Error"; break;
            }

            if (errorMessage != "")
                return StreamError(errorMessage);
            else
                return StreamError(message);
        }

        virtual ~StreamError() throw() {}

        unsigned int getCode() {
            return m_code;
        }

    private:
        unsigned int m_code;
    };
}

#endif

