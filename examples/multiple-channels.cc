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
    try{
        channel.connect("public.hydna.net/1", ChannelMode::READWRITE);
    }catch (std::exception& e) {
        cout << "could not connect: "<< e.what() << endl;
    }

    Channel channel2;
    
    try{
        channel2.connect("public.hydna.net/2", ChannelMode::READWRITE);
    }catch (std::exception& e) {
        cout << "could not connect: "<< e.what() << endl;
    }

    while(!channel.isConnected()) {
        channel.checkForChannelError();
        sleep(1);
    }

    while(!channel2.isConnected()) {
        channel2.checkForChannelError();
        sleep(1);
    }

    channel.writeString("Hello world from c++ channel 1");
    channel2.writeString("Hello world from c++ channel 2");

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

