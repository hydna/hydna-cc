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
        channel.connect("public.hydna.net/hello", ChannelMode::READWRITE);
    }catch (std::exception& e) {
        cout << "could not connect: "<< e.what() << endl;
    }

    Channel channel2;
    
    try{
        channel2.connect("public.hydna.net/hello2", ChannelMode::READWRITE);
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

    channel.writeString("Hello world from c++ channel /hello");
    channel2.writeString("Hello world from c++ channel /hello2");
    
    try{
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
    }catch (std::exception& e) {
        cout << "could not send to multiple channels: "<< e.what() << endl;
    }
    
    channel.close();
    channel2.close();
}

