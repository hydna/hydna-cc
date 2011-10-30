#include <channel.h>
#include <channelmode.h>
#include <channeldata.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Multiple channels example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    Channel channel;
    channel.connect("localhost:7010/x11221133", ChannelMode::READWRITE);

    Channel channel2;
    channel2.connect("localhost:7010/x3333", ChannelMode::READWRITE);

    while(!channel.isConnected()) {
        channel.checkForChannelError();
        sleep(1);
    }

    while(!channel2.isConnected()) {
        channel2.checkForChannelError();
        sleep(1);
    }

    channel.writeString("Hello");
    channel2.writeString("World");

    for (;;) {
        if (!channel.isDataEmpty()) {
            ChannelData* data = channel.popData();
            const char* payload = data->getContent();

            for (int i=0; i < data->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
            break;
        } else {
            channel.checkForChannelError();
        }
    }
    for (;;) {
        if (!channel2.isDataEmpty()) {
            ChannelData* data = channel2.popData();
            const char* payload = data->getContent();

            for (int i=0; i < data->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
            break;
        } else {
            channel2.checkForChannelError();
        }
    }
    channel.close();
    channel2.close();
}

