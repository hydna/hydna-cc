#include <iomanip>
#include <sstream>

#include "addr.h"
#include "error.h"

namespace hydna {
    using namespace std;
   
    Addr::Addr(unsigned int zonecomp,
            unsigned int streamcomp,
            string const &host,
            int port) : m_port(7010) {
        m_zone = zonecomp;
        m_stream = streamcomp;
        m_port = port;
      
        if (host == "") {
            m_host = hexifyComponent(zonecomp) + "." + DEFAULT_HOST;
        } else {
            m_host = host;
        }
    }

    unsigned int Addr::getZone() const {
        return m_zone;
    }

    int Addr::getStream() const {
        return m_stream;
    }
    
    int Addr::getPort() const {
        return m_port;
    }

    string Addr::getHost() const {
        return m_host;
    }
        
    bool Addr::equals(Addr addr) const {
        return m_zone == addr.m_zone && m_stream == addr.m_stream;
    }
    
    string Addr::toHex() const {
        return hexifyComponent(m_zone) + "-" + hexifyComponent(m_stream);
    }
    
    string Addr::toString() const {
        return toHex();
    }
    
    Addr Addr::fromExpr(string const &expr) {
        string host = expr;
        unsigned short port = 7010;
        unsigned int addr = 1;
        string token = "";
        size_t pos;

        pos = host.find_last_of("?");
        if (pos != string::npos) {
            token = host.substr(pos + 1);
            host = host.substr(0, pos);
        }

        pos = host.find("/x");
        if (pos != string::npos) {
            istringstream iss(host.substr(pos + 2));
            host = host.substr(0, pos);

            if ((iss >> setbase(16) >> addr).fail()) {
               throw Error("Could not read the address \"" + host.substr(pos + 2) + "\""); 
            }
        } else {
            pos = host.find_last_of("/");
            if (pos != string::npos) {
                istringstream iss(host.substr(pos + 1));
                host = host.substr(0, pos);

                if ((iss >> addr).fail()) {
                   throw Error("Could not read the address \"" + host.substr(pos + 1) + "\""); 
                }
            }
        }

        pos = host.find_last_of(":");
        if (pos != string::npos) {
            istringstream iss(host.substr(pos + 1));
            host = host.substr(0, pos);

            if ((iss >> port).fail()) {
               throw Error("Could not read the port \"" + host.substr(pos + 1) + "\""); 
            }
        }
        
        return Addr(0, addr, host, port);
    }
    
    Addr Addr::fromByteArray(const char* buffer) {
        cerr << "Addr::fromByteArray: NOT IMPLEMENTED YET" << endl;

        return Addr(0, 0);
        /*
        var index:Number = SIZE;
        var chars:Array = new Array();
        var host:String;
        
        if (!buffer || buffer.length != SIZE) {
          throw new Error("Expected a size " + SIZE + " ByteArray instance");
        }
          
        buffer.position = 0;
        
        while (index--) {
          chars.push(String.fromCharCode(buffer.readUnsignedByte()));
        }
        
        host = convertToHex(bytes, "", 0, PART_SIZE) + "." + DEFAULT_HOST;
        
        return new HydnaAddr(buffer, chars.join(""), host);
        */    
    }

    string Addr::hexifyComponent(unsigned int comp) const {
        ostringstream os(ostringstream::out);
        os.width(8);
        os.fill('0');
        os << hex << comp;
        
        string h = os.str();

        while (h.length() < 8) {
            h = "0" + h;
        }

        return h;
    }

    const char Addr::DEFAULT_HOST[] = "tcp.hydna.net";
}

