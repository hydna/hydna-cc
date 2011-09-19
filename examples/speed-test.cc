#include <stdexcept>
#include <exception>

#include <iostream>
#include <iomanip>

#include <sys/time.h>
#include <time.h>

#include <channel.h>
#include <channelmode.h>
#include <channeldata.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace hydna;
using namespace std;

static const unsigned int NO_BROADCASTS = 100000;
static const string CONTENT = "fjhksdffkhjfhjsdkahjkfsadjhksfjhfsdjhlasfhjlksadfhjldaljhksfadjhsfdahjsljhdfjlhksfadlfsjhadljhkfsadjlhkajhlksdfjhlljhsa";

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
        cerr << "Usage: " << argv[0] << " {receive|send}" << endl;
        return -1;
    }

    unsigned int i = 0;
    
    try { 
        string arg = string(argv[1]);

        Channel channel;
        channel.connect("localhost/x11221133", ChannelMode::READWRITE);

        while(!channel.isConnected()) {
            channel.checkForChannelError();
            sleep(1);
        }
        
        int time = 0;

        if (arg.compare("receive") == 0) {
            cout << "Receiving from x11221133" << endl;

            for(;;) {
                if (!channel.isDataEmpty()) {
                    channel.popData();

                    if (i == 0) {
                        time = getmicrosec();
                    }
                
                    ++i;

                    if (i == NO_BROADCASTS) {
                        time = getmicrosec() - time;
                        cout << endl << "Received " << NO_BROADCASTS << " packets" << endl;
                        cout << "Time: " << time/1000 << "ms" << endl;
                        i = 0;
                    }
                } else {
                    channel.checkForChannelError();
                }
            }
        } else if (arg.compare("send") == 0) {
            cout << "Sending " << NO_BROADCASTS << " packets to x11221133" << endl;

            time = getmicrosec();

            for (i = 0; i < NO_BROADCASTS; i++) {
                channel.writeString(CONTENT);
            }

            time = getmicrosec() - time;

            cout << "Time: " << time/1000 << "ms" << endl;

            i = 0;
            while(i < NO_BROADCASTS) {
                if (!channel.isDataEmpty()) {
                    channel.popData();
                    i++;
                } else {
                    channel.checkForChannelError();
                }
            }
        } else {
            cerr << "Usage: " << argv[0] << " {receive|send}" << endl;
            return -1;
        }

        channel.close();
    } catch (Error& e) {
        cout << "Caught exception (i=" << i << "): " << e.what() <<  endl;
    }

    return 0;
}

