#include <channel.h>
#include <channelmode.h>
#include <channeldata.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Listener example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    Channel channel;
    channel.connect("localhost:7010/x11221133", ChannelMode::READWRITE);

    while(!channel.isConnected()) {
        channel.checkForChannelError();
        sleep(1);
    }

    for (;;) {
        if (!channel.isDataEmpty()) {
            ChannelData* data = channel.popData();
            const char* payload = data->getContent();

            for (int i=0; i < data->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
        } else {
            channel.checkForChannelError();
        }
    }
}

