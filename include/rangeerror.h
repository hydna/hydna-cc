#ifndef HYDNA_RANGEERROR_H
#define HYDNA_RANGEERROR_H

#include "error.h"

namespace hydna {

    class RangeError : public Error {
    public:
        RangeError(std::string const &what, std::string const &name="RangeError") : Error(what, name) {}

        virtual ~RangeError() throw() {}
    };
}

#endif

