#ifndef HYDNA_ERROR_H
#define HYDNA_ERROR_H

#include <stdexcept>

namespace hydna {

    class Error : public std::runtime_error {
    public:
        Error(std::string const &what, std::string const &name="Error") : std::runtime_error(name), m_what(what), m_name(name) {}

        virtual ~Error() throw() {}

        virtual const char* what() const throw() {
            return (m_name + ": " + m_what).c_str();
        }
    private:
        std::string m_what;
        std::string m_name;
        
    };
}

#endif

