#include "udp_server.h"


int g_server_socket;

UdpServer *UdpServer::instance = 0;
//========================================================================================
//		Constructor and destructor
//========================================================================================
UdpServer::UdpServer()
{
	server_addrinfo_ = 0;
}

UdpServer::~UdpServer()
{
	if (instance != 0)
		delete instance;
}

UdpServer * UdpServer::Instance()
{
	if (instance == 0)
        instance = new UdpServer;

    return instance;
}

//========================================================================================
//		Initializers
//========================================================================================
int UdpServer::InitServer()
{
    //  Set up address variables
    char decimal_port[16];
    snprintf(decimal_port, sizeof(decimal_port), "%d", CmsData::Instance()->self_port_);
    decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    getaddrinfo(CmsData::Instance()->self_ipaddr_.c_str(), decimal_port, &hints, &server_addrinfo_);

    //  Create socket
    g_server_socket = socket(server_addrinfo_->ai_family, SOCK_DGRAM, 0);
    if(g_server_socket == -1)
    {
        freeaddrinfo(server_addrinfo_);
		return -1;
    }
    //  Bind it to address
    if(bind(g_server_socket, server_addrinfo_->ai_addr, server_addrinfo_->ai_addrlen) != 0)
    {
        freeaddrinfo(server_addrinfo_);
        close(g_server_socket);
		return -1;
    }

    //  Define the receive thread

    // initialize the attribute structure
    pthread_attr_init (&receivethread_attr);
    // set the detach state to "detached"
    pthread_attr_setdetachstate (&receivethread_attr, PTHREAD_CREATE_DETACHED);
    // override the default of INHERIT_SCHED
    pthread_attr_setinheritsched (&receivethread_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy (&receivethread_attr, SCHED_RR);
    // finally, create the thread
    pthread_create (NULL, &receivethread_attr, ReceiveThread, NULL);

    return 0;
}

void * UdpServer::ReceiveThread(void *)
{
	//	Acknolodgement setup
	int send_socket;
	send_socket = socket(AF_INET, SOCK_DGRAM, 0);

	struct addrinfo *send_addrinfo;
	char decimal_port[16];
	snprintf(decimal_port, sizeof(decimal_port), "%d", CmsData::Instance()->mms_port_);
	decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	getaddrinfo(CmsData::Instance()->mms_ipaddr_.c_str(), decimal_port, &hints, &(send_addrinfo));

	CMS_MSG_ACK ack;
	CMS_MSG_GENERIC *msg = new CMS_MSG_GENERIC;
	memset(msg, 0xFF, sizeof(*msg));
	memset(&ack, 0xFF, sizeof(ack));

	int socketmsg_len = 0;
    unsigned char control_msg[8];
	unsigned char reply_msg[8];
	struct timespec mms_time;

	//	Set up the fixed part of the Qnet frame
	control_msg[0] = CmsData::Instance()->mms_id_;		//	From
	control_msg[1] = CmsData::Instance()->self_id_;		//	To

	//	Set up connection to CmsControl
	int control_coid = name_open(CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);


    while(1)
    {
        socketmsg_len = recv(g_server_socket, msg, sizeof(*msg), 0);

		int test;
		if (socketmsg_len > 0)
		{
			//printf("\nCMS: received msg %d, len %d\n", msg->header.id, socketmsg_len);

			switch (msg->header.id)
			{
				case CMS_MSG_ID_ACK:
					//	Handle ack
					test = 0;
					break;

				// DCM control
				case CMS_MSG_ID_DCM_CAN:

					test = 0;
					break;

				// Param update
				case CMS_MSG_ID_PARAM_UPDATE:

					//	Acknoledge the message
					ack.header.seq = msg->header.seq;
					ack.header.src = msg->header.dest;
					ack.header.dest = msg->header.src;
					ack.header.id = CMS_MSG_ID_ACK;
					ack.header.size = 0;
					sendto(send_socket, &ack, sizeof(ack), 0, send_addrinfo->ai_addr, send_addrinfo->ai_addrlen);

					control_msg[2] = AIRCON;

					//	Cast the right kind of message
					CMS_MSG_PARAM_UPDATE *update_msg;
					update_msg = ((CMS_MSG_PARAM_UPDATE *)msg);

				//	printf("CMS: received CMS_MSG_ID_PARAM_UPDATE, %d params \n", update_msg->update.numparams);

					for(unsigned int param_it = 0; param_it < update_msg->update.numparams; param_it++)
					{
				//		printf("CMS: param %X = %f\n", update_msg->update.params[param_it].hash, update_msg->update.params[param_it].value);

						if(update_msg->update.params[param_it].hash == CmsData::Instance()->cms_self_.climate_.aircon_.temperature_in_hash)
						{
							control_msg[6] = 0x00;
							control_msg[7] = (unsigned char)update_msg->update.params[param_it].value;
							MsgSend(control_coid, control_msg, sizeof(control_msg), reply_msg, sizeof(reply_msg));
						}
						else if(update_msg->update.params[param_it].hash == CmsData::Instance()->cms_self_.climate_.aircon_.fan_speed_in_hash)
						{
							control_msg[6] = 0x03;
							control_msg[7] = (unsigned char)update_msg->update.params[param_it].value;
							MsgSend(control_coid, control_msg, sizeof(control_msg), reply_msg, sizeof(reply_msg));
						}
						else if(update_msg->update.params[param_it].hash == CmsData::Instance()->cms_self_.climate_.aircon_.mode_in_hash)
						{
							control_msg[6] = 0x02;
							control_msg[7] = (unsigned char)update_msg->update.params[param_it].value;
							MsgSend(control_coid, control_msg, sizeof(control_msg), reply_msg, sizeof(reply_msg));
						}
						else if(update_msg->update.params[param_it].hash == CmsData::Instance()->cms_self_.climate_.aircon_.ambient_temperature_in_hash)
						{
							control_msg[6] = 0x04;
							control_msg[7] = (unsigned char)update_msg->update.params[param_it].value;
							MsgSend(control_coid, control_msg, sizeof(control_msg), reply_msg, sizeof(reply_msg));
						}
					}


					break;

				// GPS info
				case CMS_MSG_ID_TIME_INFO:

					//	Acknoledge the message
					ack.header.seq = msg->header.seq;
					ack.header.src = msg->header.dest;
					ack.header.dest = msg->header.src;
					ack.header.id = CMS_MSG_ID_ACK;
					ack.header.size = 0;
					sendto(send_socket, &ack, sizeof(ack), 0, send_addrinfo->ai_addr, send_addrinfo->ai_addrlen);

					//	Cast the right kind of message
					CMS_MSG_TIME_INFO *gps_msg;
					gps_msg = (CMS_MSG_TIME_INFO *)msg;

					//	Handle the message
					time_t mmsgps_time;
					mmsgps_time = (gps_msg->info.timeLocal)/1000;
					CmsUI::Instance()->UpdateClock_Request(mmsgps_time);


					/*if(CmsData::Instance()->cms_self_.is_master)
					{
						CmsData::Instance()->cms_self_.settings_.SetSunsetTime(gps_msg->info.timeSunset);
						CmsData::Instance()->cms_self_.settings_.SetSunriseTime(gps_msg->info.timeSunrise);
						CmsUI::Instance()->UpdateSunsetTime();
						CmsUI::Instance()->UpdateSunriseTime();
					}*/

					break;

			}
			//	Clear message
			memset(msg, 0xFF, sizeof(msg));
		}

    }
	delete msg;
    return 0;
}
