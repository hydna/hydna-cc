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
        channel.connect("public.hydna.net/cc", ChannelMode::READWRITEEMIT);
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
    channel.close();
}

