# Usage

In the following example we open a read/write stream, send a "Hello world!"
when the connection has been established and print received messages to
stdout.

    :::cpp
    #include <stream.h>
    #include <streammode.h>
    #include <streamdata.h>
     
    #include <stdexcept>
    #include <exception>

    #include <iostream>

    using namespace hydna;
    using namespace std;

    int main(int argc, const char* argv[]) {
        // create a new stream (not a socket)
        Stream stream;

        // open a stream to channel 12345 on the domain demo.hydna.net in
        // read/write mode.
        stream.connect("demo.hydna.net/12345", StreamMode::READWRITE);

        // wait for connect.
        while(!stream.isConnected()) {
            stream.checkForStreamError();
            sleep(1);
        }

        // write "Hello world!" to the stream.
        stream.writeString("Hello world!");

        // main receive-loop: listen for incoming messages and write them
        // to stdout.
        for (;;) {
            if (!stream.isDataEmpty()) {
                StreamData* data = stream.popData();
                const char* payload = data->getContent();

                for (int i=0; i < data->getSize(); i++) {
                    cout << payload[i];
                }

                cout << endl;
                break;
            } else {
                stream.checkForStreamError();
            }
        }
    }
