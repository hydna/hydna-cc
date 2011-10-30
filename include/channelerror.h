#ifndef HYDNA_CHANNELERROR_H
#define HYDNA_CHANNELERROR_H

#include "ioerror.h"

namespace hydna {

    class ChannelError : public IOError {
    public:
        ChannelError(std::string const &what,
                unsigned int code=0xFFFF,
                std::string const &name="ChannelError") : IOError(what, name), m_code(code) {}

        static ChannelError fromOpenError(int flag, std::string data) {
            int code = flag;
            std::string msg;

            if (code < 7)
                msg = "Not allowd to open channel";

            if (data != "" || data.length() != 0) {
                msg = data;
            }

            return ChannelError(msg, code);
        }

        static ChannelError fromSigError(int flag, std::string data) {
            int code = flag;
            std::string msg;

            if (code == 0)
                msg = "Bad signal";

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

