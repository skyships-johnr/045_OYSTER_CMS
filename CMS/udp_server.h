#ifndef CMS_UDPSERVER_H_
#define CMS_UDPSERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>

#include "cms_data.h"
#include "common.h"
#include "cmsfmt.h"
#include "cms_ui.h"

class UdpServer
{
public:
    UdpServer();
    ~UdpServer();

    static UdpServer * Instance();

    int InitServer();

    static void * ReceiveThread(void *);

private:

    static UdpServer *instance;

    struct addrinfo *server_addrinfo_;

    pthread_attr_t receivethread_attr;

    
};


#endif
