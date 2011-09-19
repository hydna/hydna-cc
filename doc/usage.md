# Usage

In the following example we open a read/write channel, send a "Hello world!"
when the connection has been established and print received messages to
stdout.

    :::cpp
    #include <channel.h>
    #include <channelmode.h>
    #include <channeldata.h>
     
    #include <stdexcept>
    #include <exception>

    #include <iostream>

    using namespace hydna;
    using namespace std;

    int main(int argc, const char* argv[]) {
        // create a new channel (not a socket)
        Channel channel;

        // open a channel to channel 12345 on the domain demo.hydna.net in
        // read/write mode.
        channel.connect("demo.hydna.net/12345", ChannelMode::READWRITE);

        // wait for connect.
        while(!channel.isConnected()) {
            channel.checkForChannelError();
            sleep(1);
        }

        // write "Hello world!" to the channel.
        channel.writeString("Hello world!");

        // main receive-loop: listen for incoming messages and write them
        // to stdout.
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
    }
