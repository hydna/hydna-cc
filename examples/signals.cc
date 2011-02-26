#include <stream.h>
#include <streammode.h>
#include <streamsignal.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Signal example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    Stream stream;
    stream.connect("localhost/x00112233", StreamMode::READWRITE_EMIT);

    while(!stream.isConnected()) {
        stream.checkForStreamError();
        sleep(1);
    }

    stream.emitString("ping");

    for (;;) {
        if (!stream.isSignalEmpty()) {
            StreamSignal* signal = stream.popSignal();
            const char* payload = signal->getContent();

            for (int i=0; i < signal->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
            break;
        } else {
            stream.checkForStreamError();
        }
    }
    stream.close();
}

