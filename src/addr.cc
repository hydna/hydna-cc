#include <sstream>

#include "addr.h"
#include "error.h"

namespace hydna {
    using namespace std;
   
    Addr::Addr(unsigned int zonecomp,
            unsigned int streamcomp,
            string const &host,
            int port) : m_port(80) {
        m_zone = zonecomp;
        m_stream = streamcomp;
        m_port = port;
      
        if (host == "") {
            m_host = hexifyComponent(zonecomp) + "." + DEFAULT_HOST;
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
        cerr << "Addr::fromExpr: NOT IMPLEMENTED YET" << endl;

        return Addr(0, 0);
        /*
        var m:*;
        unsigned int zonecomp;
        unsigned int streamcomp = 0;
      
        cout << "parse addr expr" << endl;
      
        if (!expr) {
            throw Error("Expected String");
        }
      
        m = expr.match(ADDR_EXPR_RE);
      
        if (!m) {
            throw Error("Bad expression");
        }
      
        if (m[1]) {
            zonecomp = parseInt(m[1], 16);
        } else {
            zonecomp = parseInt(m[2], 16);
            streamcomp = parseInt(m[3], 16);
        }
      
        return new Addr(zonecomp, streamcomp);
        */
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

