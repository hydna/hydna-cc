#include <stream.h>
#include <streammode.h>
#include <streamdata.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Listener example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    Stream stream;
    stream.connect("localhost/x11221133", StreamMode::READWRITE);

    while(!stream.isConnected()) {
        stream.checkForStreamError();
        sleep(1);
    }

    for (;;) {
        if (!stream.isDataEmpty()) {
            StreamData* data = stream.popData();
            const char* payload = data->getContent();

            for (int i=0; i < data->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
        } else {
            stream.checkForStreamError();
        }
    }
}

