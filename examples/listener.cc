#include <stream.h>
#include <streammode.h>
#include <streamdata.h>
 
#include <stdexcept>
#include <exception>

#include <iostream>

/**
 *  Hello world example
 */

using namespace hydna;
using namespace std;

int main(int argc, const char* argv[]) {
    // skapa en ny ström (ej socket, hydna stream)
    Stream stream;

    // Öppna en ström, men given adress, första delen är "zonen", egentligen
    // 00112233.tcp.hydna.net. Andra biten är ström-id. Öppnas med read write
    stream.connect("localhost/x11221133", StreamMode::READWRITE);

    //stream.addEventListener(StreamDateEvent.DATA, function
    //(event:StreamDateEvent)) {
           // inkommande data
    //       event.data;
    //}

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

