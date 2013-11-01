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
    
    try{
        channel.connect("hydnacc.hydna.net/hello", ChannelMode::READWRITE);
    }catch (std::exception& e) {
        cout << "could not connect: "<< e.what() << endl;
    }
    
    while(!channel.isConnected()) {
        channel.checkForChannelError();
        sleep(1);
    }
    
    cout << "We are connected in hello world..." << endl;

    string message = channel.getMessage();
    if (message != "") {
        cout << message << endl;
    }

    channel.writeString("Hello World from c++ är e inte cool");

    for (;;) {
        if (!channel.isDataEmpty()) {
            ChannelData* data = channel.popData();
            
            const char* payload = data->getContent();

            for (int i=0; i < data->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
            break;
        }else{
            channel.checkForChannelError();
        }
    }
    channel.close();
}

