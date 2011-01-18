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
    // skapa en ny ström (ej socket, hydna stream)
    Stream stream;

    // Öppna en ström, men given adress, första delen är "zonen", egentligen
    // 00112233.tcp.hydna.net. Andra biten är ström-id. Öppnas med read write
    stream.connect("localhost/x00112233", StreamMode::READWRITE);

    //stream.addEventListener(StreamDateEvent.DATA, function
    //(event:StreamDateEvent)) {
           // inkommande data
    //       event.data;
    //}

    while(!stream.isConnected()) {
        sleep(1);
    }

    // Skriv till strömmen.
    stream.sendStringSignal("ping");

    for (;;) {
        if (!stream.isSignalEmpty()) {
            StreamSignal* signal = stream.popSignal();
            const char* payload = signal->getContent();

            for (int i=0; i < signal->getSize(); i++) {
                cout << payload[i];
            }

            cout << endl;
            break;
        }
    }
}

