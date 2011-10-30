#include <pthread.h>
#include <iomanip>
#include <sstream>

#include "urlparser.h"

#ifdef HYDNADEBUG
#include "debughelper.h"
#endif

namespace hydna {
    using namespace std;

    URL::URL() : m_port(80), m_path(""), m_host(""), m_token(""), m_auth(""), m_protocol("http"), m_error("")
    {
    }

    URL::~URL() {
    }

    unsigned short URL::getPort() const
    {
        return m_port;
    }

    string URL::getPath() const
    {
        return m_path;
    }
    
    string URL::getHost() const
    {
        return m_host;
    }

    string URL::getToken() const
    {
        return m_token;
    }
    
    string URL::getAuth() const
    {
        return m_auth;
    }
    
    string URL::getProtocol() const
    {
        return m_protocol;
    }
    
    string URL::getError() const
    {
        return m_error;
    }
    
    URL URL::parse(string const &expr) {
        URL url;
        string host = expr;
        unsigned short port = 80;
        string path = "";
        string tokens = "";
        string auth = "";
        string protocol = "http";
        string error = "";
        size_t pos;

        // Expr can be on the form "http://auth@localhost:80/x00112233?token"

        // Take out the protocol
        pos = host.find("://");
        if (pos != string::npos) {
            protocol = host.substr(0, pos);
            std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
            host = host.substr(pos + 3);
        }

        // Take out the auth
        pos = host.find("@");
        if (pos != string::npos) {
            auth = host.substr(0, pos);
            host = host.substr(pos + 1);
        }

        // Take out the token
        pos = host.find_last_of("?");
        if (pos != string::npos) {
            tokens = host.substr(pos + 1);
            host = host.substr(0, pos);
        }

        // Take out the path
        pos = host.find("/");
        if (pos != string::npos) {
            path = host.substr(pos + 1);
            host = host.substr(0, pos);
        }

        // Take out the port
        pos = host.find_last_of(":");
        if (pos != string::npos) {
            istringstream iss(host.substr(pos + 1));

            if ((iss >> port).fail()) {
               error = "Could not read the port \"" + host.substr(pos + 1) + "\"";
            }

            host = host.substr(0, pos);
        }
        
        url.m_path = path;
        url.m_host = host;
        url.m_port = port;
        url.m_auth = auth;
        url.m_protocol = protocol;
        url.m_error = error;

        return url;
    }
}

