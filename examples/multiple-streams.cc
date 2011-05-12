#include <stream.h>
#include <streammode.h>
#include <streamdata.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Multiple streams example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    Stream stream;
    stream.connect("localhost/x11221133", StreamMode::READWRITE);

    Stream stream2;
    stream2.connect("localhost/x3333", StreamMode::READWRITE);

    while(!stream.isConnected()) {
        stream.checkForStreamError();
        sleep(1);
    }

    while(!stream2.isConnected()) {
        stream2.checkForStreamError();
        sleep(1);
    }

    stream.writeString("Hello");
    stream2.writeString("World");

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
    for (;;) {
        if (!stream2.isDataEmpty()) {
            StreamData* data = stream2.popData();
            const char* payload = data->getContent();

            for (int i=0; i < data->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
            break;
        } else {
            stream2.checkForStreamError();
        }
    }
    stream.close();
    stream2.close();
}

