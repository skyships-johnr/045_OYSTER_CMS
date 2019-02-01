#ifndef CMS_CMSCONTROL_H_
#define CMS_CMSCONTROL_H_


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/dispatch.h>
#include <sys/stat.h>
#include <sys/neutrino.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "common.h"
#include "cms_data.h"
#include "cms_ui.h"
#include "cmsfmt.h"
#include "vector_class.h"

/*!
*  \brief		CmsControl class
*  \details
    The CmsControl class is the main block of the application, being responsible
    for controlling the behaviour of the system, depending on the received input.
    The inputs are received via Qnet messages, that can come either from the UI,
    other CMSs, CAN controller or the MMS. Although this is it's current status,
    it can actually receive such inputs from any desired external component that
    talks over Qnet, with very little implementation.

    After receiving and interpreting the request, it then decides the purpose of
    the message, that can be one of the following:

    1 - UI event: The UI does not make direct calls to any state changing methods,
    such actions occur via requests to the CmsControl.
    2 - CAN I/O: messaging come from the CAN controller will
    3 - Ethernet I/O: it is responsible for receiving messages from / forwarding
    messages to the Ethernet controller, which ultimately means MMS
    4 - Qnet I/O: It is re
    which oftenly generate calls to CmsData methods to change it's
    state, then call CmsUI methods, telling the UI to be updated with the new
    data.

    It is important to emphasise that the controller get inputs only via Qnet
    messages, and the calls to the data and UI controller are made via methods,
    following the MVC Design pattern.

    In order to guarantee it's robustness, CmsControl makes use of QNX Neutrino
    Thread Pool functionalities, to make sure that there will always be "someone
    listening" to the bus, and letting the QNX kernell make the best use of it's
    scheduling to attend all the requests in the shortest time possible.
*  \author		John Reis
*  \version   	1.0
*  \date      	Feb-2016
*  \pre       	Has to be the first to be initialised (altough it's CommConnect
				method must me called just after CmsControl is initialised).
*  \warning   	Improper use can crash your application
*  \copyright 	Skyships Automotive Ltd.
*/

class CmsControl
{
public:
    //! Constructor
    /*! Constructor for CmsControl class.*/
    CmsControl();

    //! Destructor
    /*! Destructor for CmsControl class.*/
    ~CmsControl();

    //! Control data structure for slaves CMSs
    /*! The structure contains information necessary to maintain communication
    with slave CMSs, it's valid to remember that this will only be used in case
    this CMS is the master. */
    typedef struct SlaveStruct
    {
        //! Slave ID
        /*! The ID retrieved from the config file. */
        unsigned char slave_id;

        string hostname;
        //! Connection ID
        /*! Each slave has an unique connection ID, in order to be messaged
        directly. */
        int slave_coid;
        //! Active flag
         /*! This flag indicates whether a slave is online. */
        bool is_active;
    }SlaveStruct;

    //! Communication server data structure.
    /*! This is the main data structure of CmsControl class, it contains information
    about received messages, necesssary connection IDs and and array of available
    slaves (in case this is the master). */
    typedef struct ControlServiceStruct
    {
        //  How long without CAN reply for killing itself
        int timeout;

        //! Receive ID
        /*! Any received message has an unique receive ID, in that way they can
        be replied directly to who sent it. */
        int comm_rcvid;

        //! Received message
        /*! A data array to store the received message */
        unsigned char msg[64];

        //! Reply message
        /*! A data array to store the reply message */
        unsigned char reply_msg[64];

        //! Array of slaves
        /*! In case this is a master CMS, it needs to have information about it's
        slaves, in order to maintain communication (see contents of SlaveStruct).*/
        vector_class<SlaveStruct> slaves;

    } ControlServiceStruct;
/*
    //  Structs for sending MMS messages
    typedef struct
        {
            uint32_t hash; // Hash
            double value; // Value
        }UdpParam;
    typedef struct
    {
        uint32_t seq; // Sequence
    	uint8_t src;  // Source device
    	uint8_t dest; // Destination device
    	uint8_t id;   // Command id
    	uint8_t size; // Data size
    }UdpMsgHeader;
    typedef struct
    {
    	UdpMsgHeader header; // Header
    	uint8_t data[sizeof(UdpParam)];
    }UdpMsg;
*/




private:


    //! Thread attributes
    /*! This QNX Neutrino structure contains information about the dynamically
    created threads.*/
    pthread_attr_t t_attr;

    //! Thread pool attributes
    /*! This QNX Neutrino structure contains information about how and when to
    create threads dynamically, and also the lifecycle of each individual thread
    (see QNX Neutrino documentation about thread pools).*/
    thread_pool_attr_t tp_attr;

    //! Thread pool pointer
    /*! Pointer to the the thread pool that will be created.*/
    thread_pool_t *tpp;

    //! Singleton instance
    /*! This class uses the concept of Singleton. By having a pointer to itself
    and being accessed only by the Instance() method, it guarantees the existence
    of only one instance, that can be accessed "globally" (if the CmsControl.h is
    included). */
    static CmsControl *instance;

    //! Thread pool: Context allocation method
    /*! This is the first method called whenever the thread pool controller decides
    that a new thread must be created. It is responsible for allocating the needed
    memory, and establishing the connections to the CAN controller, ethernet
    controller, master CMS (in case this is a slave) or slave CMSs (in case this
    is a master).

    \param handle Pointer to the dispatch handler retrieved by the Qnet service publishing
    function.
    \return A pointer for the allocated QNX Neutrino resmgr_context_t, cointaining
    a CommServerStruct inside it's IOV member.*/
    static THREAD_POOL_PARAM_T * ContextAlloc(THREAD_POOL_HANDLE_T *handle);

    //! Thread pool: Context freeing method
    /*! This is the method called by the thread pool controller, whenever it
    decides that a thread must be killed. It closes the established connections
    and free the allocated memory.

    \param param Pointer for the QNX Neutrino resmgr_context_t, allocated by
    ContextAlloc.
    .*/
    static void ContextFree(THREAD_POOL_PARAM_T *param);

    //! Thread pool: Blocking function
    /*! This is the first of the two main methods of the thread pool. It will
    called either just after the thread is created, or whenever the thread pool
    handler function finishes it's execution. It's role is to block in receive
    state, listening to the Qnet chennel for new messages. It is based on how many
    threads are blocked in this state that the thread pool controller decides if
    creating or killing threads is needed.

    \param param Pointer for the QNX Neutrino resmgr_context_t, allocated by
    ContextAlloc (with the CommServerStruct inside it's IOV member).
    \return The same pointer received as parameter (but now with the received
    message copied into it's CommServerStruct IOV member).
    */
    static THREAD_POOL_PARAM_T * WaitForMessage(THREAD_POOL_PARAM_T *ctp);


    //! Thread pool: Handler function
    /*! This is the second of the two main methods of the thread pool.After the
    blocking method unblocks (meaning a message was received), it then calls this
    method to parse the received message and take action depending on the request.
    It is here that the true functionality of CmsControl resides.

    \param param Pointer for the QNX Neutrino resmgr_context_t, allocated by
    ContextAlloc (but now with the received message copied into it's
    CommServerStruct IOV member)
    \return The same pointer received as parameter (but now with the received
    message copied into it's CommServerStruct IOV member).
    */
    static int ParseMessage(THREAD_POOL_PARAM_T *ctp);

public:
    //! Singleton instance retriever
    /*! Calling this method returns either a new instance of the CmsControl class
    (if none exists), or the existing one (if it does). It guarantees that the
    whole application will access always the same instance of the CmsControl class.

    \return A new instance to CmsControl class allocated dynamically (if none exists),
    or the existing one (if it does)*/
    static CmsControl * Instance();

    //! Initialization method
    /*! Method responsible for defining and initializing the class. creating
    the required mutexes, and defining / creating / starting the thread pool
    controller.
    \return 1 in case the service publishing over Qnet and thread pool
    initialization succeeded, -1 otherwise.
    */
    int InitController();
};


#endif
