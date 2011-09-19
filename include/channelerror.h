#ifndef HYDNA_STREAMERROR_H
#define HYDNA_STREAMERROR_H

#include "ioerror.h"
#include "packet.h"

namespace hydna {

    class ChannelError : public IOError {
    public:
        ChannelError(std::string const &what,
                unsigned int code=0xFFFF,
                std::string const &name="ChannelError") : IOError(what, name), m_code(code) {}

        static ChannelError fromHandshakeError(int flag) {
            int code = 0xFFFF;
            std::string msg;

            switch (flag) {
                case Packet::HANDSHAKE_SERVER_BUSY:
                    msg = "Handshake failed, server is busy";
                    break;
                case Packet::HANDSHAKE_BADFORMAT:
                    msg = "Handshake failed, bad format sent by client";
                    break;
                case Packet::HANDSHAKE_HOSTNAME:
                    msg = "Handshake failed, invalid hostname";
                    break;
                case Packet::HANDSHAKE_PROTOCOL:
                    msg = "Handshake failed, protocol not allowed";
                    break;
                case Packet::HANDSHAKE_SERVER_ERROR:
                    msg = "Handshake failed, server error";
                    break;

                default:
                case Packet::HANDSHAKE_UNKNOWN:
                    code = Packet::HANDSHAKE_UNKNOWN;
                    msg = "Unknown handshake error";
                    break;
            }

            return ChannelError(msg, code);
        }

        static ChannelError fromOpenError(int flag, std::string data) {
            int code = flag;
            std::string msg;

            switch (code) {
                case Packet::OPEN_FAIL_NA:
                    msg = "Failed to open channel, not available";
                    break;
                case Packet::OPEN_FAIL_MODE:
                    msg = "Not allowed to open channel with specified mode";
                    break;
                case Packet::OPEN_FAIL_PROTOCOL:
                    msg = "Not allowed to open channel with specified protocol";
                    break;
                case Packet::OPEN_FAIL_HOST:
                    msg = "Not allowed to open channel from host";
                    break;
                case Packet::OPEN_FAIL_AUTH:
                    msg = "Not allowed to open channel with credentials";
                    break;
                case Packet::OPEN_FAIL_SERVICE_NA:
                    msg = "Failed to open channel, service is not available";
                    break;
                case Packet::OPEN_FAIL_SERVICE_ERR:
                    msg = "Failed to open channel, service error";
                    break;

                default:
                case Packet::OPEN_FAIL_OTHER:
                    code = Packet::OPEN_FAIL_OTHER;
                    msg = "Failed to open channel, unknown error";
                    break;
            }

            if (data != "" || data.length() != 0) {
                msg = data;
            }

            return ChannelError(msg, code);
        }

        static ChannelError fromSigError(int flag, std::string data) {
            int code = flag;
            std::string msg;

            switch (code) {
                case Packet::SIG_ERR_PROTOCOL:
                    msg = "Protocol error";
                    break;
                case Packet::SIG_ERR_OPERATION:
                    msg = "Operational error";
                    break;
                case Packet::SIG_ERR_LIMIT:
                    msg = "Limit error";
                    break;
                case Packet::SIG_ERR_SERVER:
                    msg = "Server error";
                    break;
                case Packet::SIG_ERR_VIOLATION:
                    msg = "Violation error";
                    break;

                default:
                case Packet::SIG_ERR_OTHER:
                    code = Packet::SIG_ERR_OTHER;
                    msg = "Unknown error";
                    break;
            }

            if (data != "" || data.length() != 0) {
                msg = data;
            }

            return ChannelError(msg, code);
        }
        
        virtual ~ChannelError() throw() {}

        unsigned int getCode() {
            return m_code;
        }

    private:
        unsigned int m_code;
    };
}

#endif

