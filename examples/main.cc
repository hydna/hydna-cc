#include <stdexcept>
#include <exception>

#include <iostream>
#include <iomanip>

#include <sys/time.h>
#include <time.h>

#include <hydna.h>
#include <hydnaaddr.h>
#include <hydnaaddrexception.h>
#include <hydnastream.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>


static const unsigned int NO_BROADCASTS = 100000;
static const std::string CONTENT = "fjhksdffkhjfhjsdkahjkfsadjhksfjhfsdjhlasfhjlksadfhjldaljhksfadjhsfdahjsljhdfjlhksfadlfsjhadljhkfsadjlhkajhlksdfjhlljhsa";

int getmicrosec() {
    int result = 0;
    struct timeval tv;
    gettimeofday(&tv, (void *)NULL);

    result += (tv.tv_sec - 0) * 1000000;
    result += (tv.tv_usec - 0);

    return result;
}


int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " {receive|send}" << std::endl;
        return -1;
    }

    unsigned int i = 0;
    
    try { 
        std::string arg = std::string(argv[1]);

        HydnaAddr addr(0, 0xAABBCCDD, 0, 0);
        Hydna client("localhost", 7120);

        HydnaStream* s = client.openAddr(addr);
        
        int time = 0;

        if (arg.compare("receive") == 0) {
            std::string cs;
            std::cout << "Receiving from addr " << addr << std::endl;

            for(;;) {
                std::getline(*s, cs);

                if (i == 0) {
                    time = getmicrosec();
                }
            
                ++i;

                if (i == NO_BROADCASTS) {
                    time = getmicrosec() - time;
                    std::cout << std::endl << "Received " << NO_BROADCASTS << " packets" << std::endl;
                    std::cout << "Time: " << time/1000 << "ms" << std::endl;
                    i = 0;
                }
            }
        } else if (arg.compare("send") == 0) {
            std::cout << "Sending " << NO_BROADCASTS << " packets to addr " << addr << std::endl;

            time = getmicrosec();

            for (i = 0; i < NO_BROADCASTS; i++) {
                *s << CONTENT << std::endl;
            }

            *s << std::flush;
            
            time = getmicrosec() - time;

            std::cout << "Time: " << time/1000 << "ms" << std::endl;
        } else {
            std::cerr << "Usage: " << argv[0] << " {receive|send}" << std::endl;
            return -1;
        }

        sleep(10);
    } catch (std::exception& e) {
        std::cout << "Caught exception (i=" << i << "): " << e.what() <<  std::endl;
    }
}

