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
    try{
        channel.connect("public.hydna.net/1", ChannelMode::READWRITE);
    } catch (std::exception& e) {
        cout << "could not connect: "<< e.what() << endl;
    }

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

