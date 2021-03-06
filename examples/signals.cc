#include <channel.h>
#include <channelmode.h>
#include <channelsignal.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Signal example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    Channel channel;
    
    try{
        channel.connect("test-beta.hydna.net/ping-back", ChannelMode::READWRITEEMIT);
    }catch (std::exception& e) {
        cout << "could not connect: "<< e.what() << endl;
    }

    while(!channel.isConnected()) {
        channel.checkForChannelError();
        sleep(1);
    }

    string message = channel.getMessage();
    if (message != "") {
        cout << message << endl;
    }

    channel.emitString("ping");
    
    try{
        for (;;) {
            if (!channel.isSignalEmpty()) {
                ChannelSignal* signal = channel.popSignal();
                const char* payload = signal->getContent();

                for (int i=0; i < signal->getSize(); i++) {
                    cout << payload[i];
                }

                cout << endl;
                break;
            } else {
                channel.checkForChannelError();
            }
        }
    }catch (std::exception& e) {
        cout << "could not emit: "<< e.what() << endl;
    }
    
    channel.close();
}

