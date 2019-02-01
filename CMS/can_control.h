#ifndef CMS_CANCONTROL_H_
#define CMS_CANCONTROL_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/can_dcmd.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <kanzi/kanzi.hpp>

#include "common.h"
#include "cms_data.h"

// Data bytes appear to be swapped
#define DBYTE0 3
#define DBYTE1 2
#define DBYTE2 1
#define DBYTE3 0
#define DBYTE4 7
#define DBYTE5 6
#define DBYTE6 5
#define DBYTE7 4



#define BIT7 (unsigned char)0b10000000
#define BIT6 (unsigned char)0b01000000
#define BIT5 (unsigned char)0b00100000
#define BIT4 (unsigned char)0b00010000
#define BIT3 (unsigned char)0b00001000
#define BIT2 (unsigned char)0b00000100
#define BIT1 (unsigned char)0b00000010
#define BIT0 (unsigned char)0b00000001

class CanControl
{
public:

    CanControl();

    ~CanControl();

    typedef struct CanSendServiceStruct
    {
        int control_rcvid;

        unsigned char msg[64];
        unsigned char reply_msg[8];

    }CanSendServiceStruct;

    typedef struct CanReceiveServiceStruct
    {
        int control_coid;
        unsigned char can_msg[8];
        unsigned char control_msg[8];
        unsigned char control_replymsg[8];

    }CanReceiveServiceStruct;

private:
    static CanControl *instance;

    //  Thread pool parameters for CAN Sending
    pthread_attr_t cansend_t_attr;
    thread_pool_attr_t cansend_tp_attr;
    thread_pool_t *cansend_tpp;

    static THREAD_POOL_PARAM_T * CanSendContextAlloc(THREAD_POOL_HANDLE_T *handle);
    static void CanSendContextFree(THREAD_POOL_PARAM_T *param);
    static THREAD_POOL_PARAM_T * CanSendWaitForMessage(THREAD_POOL_PARAM_T *ctp);
    static int CanSendParseMessage(THREAD_POOL_PARAM_T *ctp);

    //  Thread pool parameters for CAN receiving
    pthread_attr_t canreceive_t_attr;
    thread_pool_attr_t canreceive_tp_attr;
    thread_pool_t *canreceive_tpp;

    static THREAD_POOL_PARAM_T * CanReceiveContextAlloc(THREAD_POOL_HANDLE_T *handle);
    static void CanReceiveContextFree(THREAD_POOL_PARAM_T *param);
    static THREAD_POOL_PARAM_T * CanReceiveWaitForMessage(THREAD_POOL_PARAM_T *ctp);
    static int CanReceiveParseMessage(THREAD_POOL_PARAM_T *ctp);

    static int UpdateByteValue(const unsigned char message_instance, const unsigned char frame_position, const unsigned char new_value);
    static int UpdateBitValue(const unsigned char message_instance, const unsigned char frame_position, const bool new_value);
    static int SendQueuedMessages();
    static int TryQueueMessage(unsigned char instance);



public:

    static CanControl * Instance();


    int InitThreadPools();
    int InitCan();
    int SendAllMessages();
    int CloseChannels();
};


#endif
