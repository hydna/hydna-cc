#include <channel.h>
#include <channelmode.h>
#include <channeldata.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Hello world example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    Channel channel;
    channel.connect("localhost/x11221133", ChannelMode::READWRITE);

    while(!channel.isConnected()) {
        channel.checkForChannelError();
        sleep(1);
    }

    channel.writeString("Hello World");

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
    channel.close();
}

