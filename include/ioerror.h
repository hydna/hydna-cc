#ifndef HYDNA_IOERROR_H
#define HYDNA_IOERROR_H

#include "error.h"

namespace hydna {

    class IOError : public Error {
    public:
        IOError(std::string const &what, std::string const &name="IOError") : Error(what, name) {}

        virtual ~IOError() throw() {}
    };
}

#endif

