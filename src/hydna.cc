/**
 * hydna.cc
 *
 * @author Emanuel Dahlberg, emanuel.dahlberg@gmail.com
 */

#include <iostream>
#include <ios>
#include <vector>
#include <map>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "hydna.h"
#include "hydnapacket.h"
#include "hydnapacketbuffer.h"
#include "hydnaaddr.h"
#include "hydnastream.h"
#include "hydnasocketexception.h"
#include "hydnasocketreadexception.h"
#include "hydnasocketwriteexception.h"
#include "hydnaresolveexception.h"
#include "hydnaconnectexception.h"
#include "hydnanotconnectedexception.h"
#include "hydnalisteningexception.h"
#include "hydnasocketclosedexception.h"

Hydna::Hydna(const std::string &host, unsigned int port, unsigned int bufSize) :
        buffer(new HydnaPacketBuffer()), bufSize(bufSize), addrRedirectMap(new AddrMap()), disconnecting(false)
{
    socketStatusMutex = new pthread_mutex_t();
    pthread_mutex_init(socketStatusMutex, NULL);

    struct hostent     *he;
    struct sockaddr_in server;

    connected = new bool;
    *connected = false;

    socketClosed = new bool;
    *socketClosed = false;

    socketReadException = new bool;
    *socketReadException = false;

    if ((socketFDS = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        throw HydnaSocketException();
    } else {
        if ((he = gethostbyname(host.c_str())) == NULL) {
            close(socketFDS);
            throw HydnaResolveException();
        } else {
            int flag = 1;
            if (setsockopt(socketFDS, IPPROTO_TCP,
                                 TCP_NODELAY, (char *) &flag,
                                 sizeof(flag)) < 0) {
                std::cerr << "Hydna: Could not set TCP_NODELAY" << std::endl;
            }
                        
            memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
            server.sin_family = AF_INET;
            server.sin_port = htons(port);

            if (connect(socketFDS, (struct sockaddr *)&server, sizeof(server)) == -1) {
                close(socketFDS);
                throw HydnaConnectException();
            } else {
                *connected = true;

                ListenArgs* args = new ListenArgs();
                args->socketFDS = socketFDS;
                args->buffer = buffer;
                args->bufSize = bufSize;
                args->connected = connected;
                args->socketClosed = socketClosed;
                args->socketReadException = socketReadException;
                args->socketStatusMutex = socketStatusMutex;
                
                if (pthread_create(&listeningThread, NULL, listen, (void*) args) != 0) {
                    *connected = false;
                    close(socketFDS);
                    throw HydnaListeningException();
                }
            }
        }
    }
}

Hydna::~Hydna() {
    disconnecting = true;

    for (AddrMap::iterator it = addrRedirectMap->begin(); it != addrRedirectMap->end(); it++) {
        closeAddr(it->second);
    }

    // TODO Find out why a sleep is sometimes needed for a clean exit (on portal)
    //sleep(10);

    pthread_mutex_lock(socketStatusMutex);
    if (*connected) {
        *connected = false;
        shutdown(socketFDS, SHUT_WR);
        close(socketFDS);
    }
    pthread_mutex_unlock(socketStatusMutex);
    
    pthread_join(listeningThread, NULL); 

    delete buffer;
    delete connected;
    delete addrRedirectMap;
    delete socketClosed;
    delete socketReadException;
    pthread_mutex_destroy(socketStatusMutex);
    delete socketStatusMutex;
}

void* Hydna::listen(void *ptr) {
    ListenArgs* args;
    args = (ListenArgs*) ptr;
    int socketFDS = args->socketFDS;
    HydnaPacketBuffer* buffer = args->buffer;
    unsigned int bufSize = args->bufSize;
    bool *connected = args->connected;
    bool *socketClosed  = args->socketClosed;
    bool *socketReadException  = args->socketReadException;
    pthread_mutex_t* socketStatusMutex = args->socketStatusMutex;

    delete args;

    int n = -1;
    unsigned char* data = new unsigned char[bufSize];

    while (n != 0) {
        n = read(socketFDS, data, bufSize);

        if (n <= 0) {
            pthread_mutex_lock(socketStatusMutex);
            if (*connected && n != 0) {
                *socketReadException = true;
                *connected = false;
            }
            pthread_mutex_unlock(socketStatusMutex);

            break;
        }

#ifdef HYDNADEBUG
        std::cout << "Hydna(DEBUG): Received " << n << " bytes of data." << std::endl;
#endif

        buffer->pushData(data, n);
    }
    
    delete[] data;

    pthread_mutex_lock(socketStatusMutex);
    if (*connected) {
        *socketClosed = true;
        *connected = false;
    }
    pthread_mutex_unlock(socketStatusMutex);
    
    buffer->disconnect();

    pthread_exit(NULL);
}

void Hydna::sendPacket(HydnaPacket &packet) {
    pthread_mutex_lock(socketStatusMutex);
    if (*socketClosed) {
        throw HydnaSocketClosedException();
    }

    if (!*connected) {
        throw HydnaNotConnectedException();
    }
    pthread_mutex_unlock(socketStatusMutex);

    int n = -1;
    unsigned int offset = 0;
    unsigned int size = packet.getSize();

    while (n != 0 && offset < size) {
        n = write(socketFDS, packet.getData() + offset, size - offset);

        if (n < 0) {
            throw HydnaSocketWriteException();
        } else {
            offset += n;
        }
    }

#ifdef HYDNADEBUG
    std::cout << "Hydna(DEBUG): Sent " << offset << " bytes of data." << std::endl;
#endif
}

HydnaStream* Hydna::openAddr(HydnaAddr addr) {
    AddrMap::iterator it = addrRedirectMap->find(addr);

    if (it == addrRedirectMap->end()) {
        HydnaPacket packet = HydnaPacket::open(addr);

        sendPacket(packet);

        HydnaPacket* statPacket = buffer->getOpenStatPacket(addr);
        HydnaStream* stream;

        if (statPacket) {
            HydnaAddr addr = statPacket->getAddr();

            // TODO Chech for open stat error
            HydnaAddr raddr = statPacket->getRAddr();

#ifdef HYDNADEBUG
            if (addr != raddr) {
                std::cout << "Hydna(DEBUG): Received a redirect." << std::endl; 
                std::cout << "Hydna(DEBUG):    From: " << addr  << std::endl;
                std::cout << "Hydna(DEBUG):      To: " << raddr << std::endl;
            }
#endif
            addrRedirectMap->insert(std::pair<HydnaAddr, HydnaAddr>(addr, raddr));

            stream = new HydnaStream(this, raddr, bufSize);
        } else {
            stream = new HydnaStream(this, addr, bufSize);
        }

        stream->exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
        return stream;
    } else {
        HydnaStream* stream = new HydnaStream(this, it->second, bufSize);

        return stream;
    }
}

void Hydna::closeAddr(HydnaAddr addr) {
    AddrMap::iterator it = addrRedirectMap->find(addr);

    HydnaPacket packet = HydnaPacket::close(addr);
    sendPacket(packet);

    if (!disconnecting) {
        addrRedirectMap->erase(it);
    }
}

void Hydna::sendData(HydnaAddr addr, unsigned char* data, unsigned short size) {
    HydnaPacket packet = HydnaPacket::emit(addr, data, size);
    sendPacket(packet);
}

HydnaPacket* Hydna::getDataPacket(HydnaAddr addr) {
    HydnaPacket *packet = buffer->getDataPacket(addr);

    if (!packet) {
        pthread_mutex_lock(socketStatusMutex);
        if (*socketClosed) {
            throw HydnaSocketClosedException();
        }

        if (*socketReadException) {
            throw HydnaSocketReadException();
        }

        if (!*connected) {
            throw HydnaNotConnectedException();
        }
        pthread_mutex_unlock(socketStatusMutex);
    }

    return packet;
}

HydnaPacket* Hydna::popDataPacket(HydnaAddr addr) {
    HydnaPacket *packet = buffer->popDataPacket(addr);

    if (!packet) {
        pthread_mutex_lock(socketStatusMutex);
        if (*socketClosed) {
            throw HydnaSocketClosedException();
        }

        if (*socketReadException) {
            throw HydnaSocketReadException();
        }

        if (!*connected) {
            throw HydnaNotConnectedException();
        }
        pthread_mutex_unlock(socketStatusMutex);
    }

    return packet;
}

HydnaPacket* Hydna::popControlPacket(HydnaAddr addr) {
    HydnaPacket *packet = buffer->popControlPacket(addr);

    if (!packet) {
        pthread_mutex_lock(socketStatusMutex);
        if (*socketClosed) {
            throw HydnaSocketClosedException();
        }

        if (*socketReadException) {
            throw HydnaSocketReadException();
        }

        if (!*connected) {
            throw HydnaNotConnectedException();
        }
        pthread_mutex_unlock(socketStatusMutex);
    }

    return packet;
}

