#include "can_control.h"

#include <sys/can_dcmd.h>
#include "vector_class.h"

#define CAN_DEVCTL_DEBUG_INFO   __DION(_DCMD_MISC, CAN_CMD_CODE + 101)
#define WAKEUP_MSGS     5

//****************************************************************************
//  Global variables
//****************************************************************************
static int send_channel;
static int receive_channel;
static int g_can_chid;
static vector_class<unsigned char> messages_queue_;

pthread_mutex_t g_cms_candata_mutex;
pthread_mutex_t g_cms_cansend_mutex;

CanControl *CanControl::instance = 0;
//****************************************************************************
//  Constructor and destructor
//****************************************************************************
CanControl::CanControl()
{
	messages_queue_.clear();
	cansend_tpp = NULL;
	canreceive_tpp = NULL;
}

CanControl::~CanControl()
{
	if (instance != 0)
		delete instance;
}

CanControl * CanControl::Instance()
{
	if (instance == 0)
        instance = new CanControl;

    return instance;
}

int CanControl::UpdateByteValue(const unsigned char message_instance, const unsigned char frame_position, const unsigned char new_value)
{
	if(message_instance < CmsData::Instance()->can_messages_.size() && frame_position < 8)
	{
		CmsData::Instance()->can_messages_.at(message_instance).msg_data[frame_position] = new_value;
		return 0;
	}

	return -1;
}

int CanControl::UpdateBitValue(const unsigned char message_instance, const unsigned char frame_position, bool new_value)
{
	if(message_instance < CmsData::Instance()->can_messages_.size() && frame_position < 8)
	{
		if(new_value)
			CmsData::Instance()->can_messages_.at(message_instance).msg_data[7] |= 1 << frame_position;
		else
			CmsData::Instance()->can_messages_.at(message_instance).msg_data[7]  &= ~(1 << frame_position);

		return 0;
	}

	return -1;
}

int CanControl::SendQueuedMessages()
{
	bool all_sent = true;
	unsigned char can_data[8];

	//	Circle through the queued messages and send them one by one
	for(unsigned int queue_it = 0; queue_it < messages_queue_.size(); queue_it++)
	{
		//	Mount messages
		can_data[ DBYTE0 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[0];
		can_data[ DBYTE1 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[1];
		can_data[ DBYTE2 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[2];
		can_data[ DBYTE3 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[3];
		can_data[ DBYTE4 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[4];
		can_data[ DBYTE5 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[5];
		can_data[ DBYTE6 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[6];
		can_data[ DBYTE7 ] = CmsData::Instance()->can_messages_.at(messages_queue_.at(queue_it)).msg_data[7];

		// Send it to the CAN driver
		if (write(send_channel, can_data, sizeof(can_data)) != sizeof(can_data))
		{
			printf("Failed to send CAN.");
			all_sent = false;
		}
	}

	//	If all messages were sent, clear the queue
	if(all_sent)
	{
		messages_queue_.clear();
		return 0;
	}
	else
		return -1;
}

int CanControl::TryQueueMessage(unsigned char instance)
{
	//	See if message is already queued
	for(unsigned int queue_it = 0; queue_it < messages_queue_.size(); queue_it++)
	{
		if(messages_queue_.at(queue_it) == instance)
			return -1;
	}
	//	If it is not, add it!
	messages_queue_.push_back(instance);

	return 0;
}

//****************************************************************************
//  CAN Send thread pool lifecycle functions
//****************************************************************************
THREAD_POOL_PARAM_T * CanControl::CanSendContextAlloc(THREAD_POOL_HANDLE_T *handle)
{
	resmgr_context_t *ctp = resmgr_context_alloc(handle);
	CanSendServiceStruct *cansend_struct = new CanSendServiceStruct;
    //  Initialize values;
    cansend_struct->control_rcvid = -1;

    SETIOV(ctp->iov, cansend_struct, sizeof(*cansend_struct));
    return ctp;
}


THREAD_POOL_PARAM_T * CanControl::CanSendWaitForMessage(THREAD_POOL_PARAM_T *ctp)
{
	CanSendServiceStruct *cansend_struct = (CanSendServiceStruct *)ctp->iov->iov_base;

	cansend_struct->control_rcvid = MsgReceive(g_can_chid, cansend_struct->msg, sizeof(cansend_struct->msg), NULL);

	//  Reply to sender to unblock it immediately
	if(cansend_struct->control_rcvid != 0)
	{
		delay(5); // kludge: without a delay here the app locks up :(
		int i = MsgReply(cansend_struct->control_rcvid, EOK, cansend_struct->reply_msg, sizeof(cansend_struct->reply_msg));
	}

	SETIOV(ctp->iov, cansend_struct, sizeof(*cansend_struct));
	return ctp;
}


int CanControl::CanSendParseMessage(THREAD_POOL_PARAM_T *ctp)
{
	CanSendServiceStruct *cansend_struct = (CanSendServiceStruct *)ctp->iov->iov_base;

	int msg_id;

	if(cansend_struct->msg[1] == CmsData::Instance()->can_id_)
	{
		if(cansend_struct->msg[5] < CmsData::Instance()->can_messages_.size())
		{
			if(cansend_struct->msg[2] == LIGHTING)
			{
				pthread_mutex_lock(&g_cms_candata_mutex);
				UpdateByteValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
				TryQueueMessage(cansend_struct->msg[5]);
				pthread_mutex_unlock(&g_cms_candata_mutex);
			}
			else if(cansend_struct->msg[2] == BLINDS)
			{
				pthread_mutex_lock(&g_cms_candata_mutex);
				UpdateBitValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
				TryQueueMessage(cansend_struct->msg[5]);
				pthread_mutex_unlock(&g_cms_candata_mutex);
			}
			else if(cansend_struct->msg[2] == HEATING)
			{
				if(cansend_struct->msg[4] == 0x00)
				{
					//	If it enters here, it's temperature
					pthread_mutex_lock(&g_cms_candata_mutex);
					UpdateByteValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
					TryQueueMessage(cansend_struct->msg[5]);
					pthread_mutex_unlock(&g_cms_candata_mutex);
				}
				else if(cansend_struct->msg[4] == 0x01)
				{
					//	If it enters here, it's a switch
					pthread_mutex_lock(&g_cms_candata_mutex);
					UpdateBitValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
					TryQueueMessage(cansend_struct->msg[5]);
					pthread_mutex_unlock(&g_cms_candata_mutex);
				}
			}
			else if(cansend_struct->msg[2] == FANS)
			{
				pthread_mutex_lock(&g_cms_candata_mutex);
				UpdateByteValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
				TryQueueMessage(cansend_struct->msg[5]);
				pthread_mutex_unlock(&g_cms_candata_mutex);
			}
			else if(cansend_struct->msg[2] == SETTINGS)
			{
				//	Reset fuses
				if(cansend_struct->msg[3] == 0x01)
				{
					unsigned char can_data[8];
					//	The reset can't be queued because it needs to have it's
					//	value brought back just after sending
					pthread_mutex_lock(&g_cms_candata_mutex);
					UpdateBitValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
					can_data[ DBYTE0 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[0];
					can_data[ DBYTE1 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[1];
					can_data[ DBYTE2 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[2];
					can_data[ DBYTE3 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[3];
					can_data[ DBYTE4 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[4];
					can_data[ DBYTE5 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[5];
					can_data[ DBYTE6 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[6];
					can_data[ DBYTE7 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[7];

					if (write(send_channel, can_data, sizeof(can_data)) != sizeof(can_data))
					{
						printf("Failed to send fuse reset to CAN.");
					}
					//	Bring the reset bit back to zero
					UpdateBitValue(cansend_struct->msg[5], cansend_struct->msg[6], 0);
					pthread_mutex_unlock(&g_cms_candata_mutex);

				}
				//	MASTER SWITCH
				else if(cansend_struct->msg[3] == 0x04)
				{
					unsigned char can_data[8];
					//	The master switch can't be queued because it needs to have it's
					//	value brought back just after sending
					pthread_mutex_lock(&g_cms_candata_mutex);
					UpdateBitValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
					can_data[ DBYTE0 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[0];
					can_data[ DBYTE1 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[1];
					can_data[ DBYTE2 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[2];
					can_data[ DBYTE3 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[3];
					can_data[ DBYTE4 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[4];
					can_data[ DBYTE5 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[5];
					can_data[ DBYTE6 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[6];
					can_data[ DBYTE7 ] = CmsData::Instance()->can_messages_.at(cansend_struct->msg[5]).msg_data[7];

					if (write(send_channel, can_data, sizeof(can_data)) != sizeof(can_data))
					{
						printf("Failed to send fuse reset to CAN.");
					}
					//	Bring the all on/off bit back to zero
					UpdateBitValue(cansend_struct->msg[5], cansend_struct->msg[6], 0);
					pthread_mutex_unlock(&g_cms_candata_mutex);
				}
				//	Night Bit
				else if(cansend_struct->msg[3] == 0x05)
				{
					pthread_mutex_lock(&g_cms_candata_mutex);
					UpdateBitValue(cansend_struct->msg[5], cansend_struct->msg[6], cansend_struct->msg[7]);
					TryQueueMessage(cansend_struct->msg[5]);
					pthread_mutex_unlock(&g_cms_candata_mutex);
				}
			}
			else if(cansend_struct->msg[2] == SEND_QUEUE)
			{
				pthread_mutex_lock(&g_cms_candata_mutex);
				//	Try just one more time in case it fails the first
				if(SendQueuedMessages())
					SendQueuedMessages();
				pthread_mutex_unlock(&g_cms_candata_mutex);
			}
		}
	}

	return 0;
}

void CanControl::CanSendContextFree(THREAD_POOL_PARAM_T *ctp)
{
	CanSendServiceStruct *cansend_struct = (CanSendServiceStruct *)ctp->iov->iov_base;

	delete cansend_struct;
	resmgr_context_free(ctp);
}


//****************************************************************************
//  CAN Receive thread pool lifecycle functions
//****************************************************************************

THREAD_POOL_PARAM_T * CanControl::CanReceiveContextAlloc(THREAD_POOL_HANDLE_T *handle)
{
	resmgr_context_t *ctp = new resmgr_context_t;//resmgr_context_alloc(handle);
	CanReceiveServiceStruct *canreceive_struct = new CanReceiveServiceStruct;


	canreceive_struct->control_coid = name_open("master", NAME_FLAG_ATTACH_GLOBAL);


    //  Initialize values;
	canreceive_struct->control_msg[0] = CmsData::Instance()->can_id_;
	canreceive_struct->control_msg[4] = 0xFF;
	canreceive_struct->control_msg[5] = 0xFF;
	canreceive_struct->control_msg[6] = 0xFF;



    SETIOV(ctp->iov, canreceive_struct, sizeof(*canreceive_struct));
    return ctp;
}

THREAD_POOL_PARAM_T * CanControl::CanReceiveWaitForMessage(THREAD_POOL_PARAM_T *ctp)
{
	CanReceiveServiceStruct *canreceive_struct = (CanReceiveServiceStruct *)ctp->iov->iov_base;
	unsigned char can_data[8];
	memset(can_data, 0xFF, sizeof(can_data));

	if (read(receive_channel, can_data, sizeof(can_data)) != sizeof(can_data))
	{
		//	If it didn't get the whole message, "erase" it
		memset(can_data, 0xFF, sizeof(can_data));
	}
	//	Adjust the byte order
	canreceive_struct->can_msg[0] = can_data[DBYTE0];
	canreceive_struct->can_msg[1] = can_data[DBYTE1];
	canreceive_struct->can_msg[2] = can_data[DBYTE2];
	canreceive_struct->can_msg[3] = can_data[DBYTE3];
	canreceive_struct->can_msg[4] = can_data[DBYTE4];
	canreceive_struct->can_msg[5] = can_data[DBYTE5];
	canreceive_struct->can_msg[6] = can_data[DBYTE6];
	canreceive_struct->can_msg[7] = can_data[DBYTE7];


	SETIOV(ctp->iov, canreceive_struct, sizeof(*canreceive_struct));
    return ctp;
}

int CanControl::CanReceiveParseMessage(THREAD_POOL_PARAM_T *ctp)
{
	CanReceiveServiceStruct *canreceive_struct = (CanReceiveServiceStruct *)ctp->iov->iov_base;
	if(canreceive_struct->can_msg[2] == (unsigned char)40)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Forward cabin OFF
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Forward cabin DAY
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT2)
		{
			//	Forward cabin EVE
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x01;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT3)
		{
			//	Forward cabin NIGHT
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT4)
		{
			//	Forward cabin UP
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x11;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT5)
		{
			//	Forward cabin DOWN
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x10;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		//	If it's not increasing or decreasing
		if(!(canreceive_struct->can_msg[7] & BIT5) && !(canreceive_struct->can_msg[7] & BIT4))
		{
			//	Stop increasing/decreasing
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0xA0;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT6)
		{
			//	Forward heads OFF
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT7)
		{
			//	Forward heads DAY
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	else if(canreceive_struct->can_msg[2] == (unsigned char)41)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Forward heads NIGHT
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Forward heads SHOWER
			canreceive_struct->control_msg[1] = 0x06;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x04;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT2)
		{
			//	Saloon OFF
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT3)
		{
			//	Saloon DAY
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT4)
		{
			//	Saloon EVE
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x01;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT5)
		{
			//	Saloon NIGHT
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT6)
		{
			//	Saloon UP
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x11;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT7)
		{
			//	Saloon DOWN
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x10;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		//	If it's not increasing or decreasing
		if(!(canreceive_struct->can_msg[7] & BIT6) && !(canreceive_struct->can_msg[7] & BIT7))
		{
			//	Stop increasing/decreasing
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0xA0;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	else if(canreceive_struct->can_msg[2] == (unsigned char)42)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Saloon passageway OFF
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Saloon passageway DAY
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT2)
		{
			//	Saloon passageway EVE
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x01;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT3)
		{
			//	Saloon passageway NIGHT
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT4)
		{
			//	Saloon passageway UP
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x11;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT5)
		{
			//	Saloon passageway DOWN
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x10;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		//	If it's not increasing or decreasing
		if(!(canreceive_struct->can_msg[7] & BIT4) && !(canreceive_struct->can_msg[7] & BIT5))
		{
			//	Stop increasing/decreasing
			canreceive_struct->control_msg[1] = 0x04;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0xA0;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT6)
		{
			//	Galley OFF
			canreceive_struct->control_msg[1] = 0x01;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT7)
		{
			//	Galley DAY
			canreceive_struct->control_msg[1] = 0x01;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	else if(canreceive_struct->can_msg[2] == (unsigned char)43)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Galley EVE
			canreceive_struct->control_msg[1] = 0x01;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x01;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Galley NIGHT
			canreceive_struct->control_msg[1] = 0x01;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT2)
		{
			//	Galley UP
			canreceive_struct->control_msg[1] = 0x01;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x11;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT3)
		{
			//	Galley DOWN
			canreceive_struct->control_msg[1] = 0x01;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x10;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(!(canreceive_struct->can_msg[7] & BIT2) && !(canreceive_struct->can_msg[7] & BIT3))
		{
			//	Stop increasing/decreasing
			canreceive_struct->control_msg[1] = 0x01;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0xA0;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT4)
		{
			//	Port cabin OFF
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT5)
		{
			//	Port cabin DAY
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT6)
		{
			//	Port cabin EVE
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x01;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT7)
		{
			//	Port cabin NIGHT
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	else if(canreceive_struct->can_msg[2] == (unsigned char)44)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Port cabin UP
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x11;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Port cabin DOWN
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x10;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(!(canreceive_struct->can_msg[7] & BIT0) && !(canreceive_struct->can_msg[7] & BIT1))
		{
			//	Stop increasing/decreasing
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0xA0;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT2)
		{
			//	Port heads OFF
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT3)
		{
			//	Port heads DAY
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT4)
		{
			//	Port heads NIGHT
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT5)
		{
			//	Port heads SHOWER
			canreceive_struct->control_msg[1] = 0x03;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x04;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT6)
		{
			//	Starboard cabin OFF
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT7)
		{
			//	Starboard cabin DAY
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	else if(canreceive_struct->can_msg[2] == (unsigned char)45)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Starboard cabin EVE
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x01;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Starboard cabin NIGHT
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT2)
		{
			//	Starboard cabin UP
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x11;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT3)
		{
			//	Starboard cabin DOWN
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x10;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(!(canreceive_struct->can_msg[7] & BIT2) && !(canreceive_struct->can_msg[7] & BIT3))
		{
			//	Stop increasing/decreasing
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0xA0;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT4)
		{
			//	Starboard heads OFF
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT5)
		{
			//	Starboard heads DAY
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT6)
		{
			//	Starboard heads NIGHT
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT7)
		{
			//	Starboard heads SHOWER
			canreceive_struct->control_msg[1] = 0x05;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x04;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	else if(canreceive_struct->can_msg[2] == (unsigned char)46)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Owners cabin OFF
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Owners cabin DAY
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT2)
		{
			//	Owners cabin EVE
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x01;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT3)
		{
			//	Owners cabin NIGHT
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT4)
		{
			//	Owners cabin UP
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x11;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT5)
		{
			//	Owners cabin DOWN
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0x10;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(!(canreceive_struct->can_msg[7] & BIT4) && !(canreceive_struct->can_msg[7] & BIT5))
		{
			//	Stop increasing/decreasing
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING;
			canreceive_struct->control_msg[3] = 0x00;
			canreceive_struct->control_msg[7] = 0xA0;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT6)
		{
			//	Owners heads OFF
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x06;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT7)
		{
			//	Owners heads DAY
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x00;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	else if(canreceive_struct->can_msg[2] == (unsigned char)47)
	{
		if(canreceive_struct->can_msg[7] & BIT0)
		{
			//	Owners heads NIGHT
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x02;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
		if(canreceive_struct->can_msg[7] & BIT1)
		{
			//	Owners heads SHOWER
			canreceive_struct->control_msg[1] = 0x02;
			canreceive_struct->control_msg[2] = LIGHTING_PROFILE;
			canreceive_struct->control_msg[3] = 0x01;
			canreceive_struct->control_msg[7] = 0x04;

			MsgSend(canreceive_struct->control_coid, canreceive_struct->control_msg,
				sizeof(canreceive_struct->control_msg), canreceive_struct->control_replymsg,
				sizeof(canreceive_struct->control_replymsg));
		}
	}
	return 0;
}

void CanControl::CanReceiveContextFree(THREAD_POOL_PARAM_T *ctp)
{
	CanReceiveServiceStruct *canreceive_struct = (CanReceiveServiceStruct *)ctp->iov->iov_base;

	name_close(canreceive_struct->control_coid);

	delete canreceive_struct;
	resmgr_context_free(ctp);
}

//****************************************************************************
//  Communications set up and start
//****************************************************************************

int CanControl::InitCan()
{
	int val;

	unsigned char data[8];

    ////////////////////////////////////////////////////////////
    // Open RX slot

    if ((receive_channel = open("/dev/can0/rx0", O_RDWR)) == -1)
    {
        printf("failed to open rx slot!\n");
        exit(EXIT_FAILURE);
    }

    val = 0;
    if (devctl(receive_channel, CAN_DEVCTL_SET_MFILTER, &val, sizeof(val), NULL))
    {
        printf("failed to set rx mfilter!\n");
        exit(EXIT_FAILURE);
    }

    if (devctl(receive_channel, CAN_DEVCTL_SET_MID, &val, sizeof(val), NULL))
    {
        printf("failed to set rx mid!\n");
        exit(EXIT_FAILURE);
    }


    ////////////////////////////////////////////////////////////
    // Open TX slot

    if ((send_channel = open("/dev/can0/tx16", O_RDWR | O_NONBLOCK)) == -1)
    {
        printf("failed to open tx slot!\n");
        exit(EXIT_FAILURE);
    }

    val = EMPIRBUS_ID | EMPIRBUS_ADDRESS;
    if (devctl(send_channel, CAN_DEVCTL_SET_MID, &val, sizeof(val), NULL))
    {
        printf("failed to set tx mid!\n");
        exit(EXIT_FAILURE);
    }

    // WORKAROUND: due to h/w issue, do not start CAN comms until after
    //             we received at least N messages. Any messages will do.
    //             This is to make sure the transceiver is fully awake and
    //             is out of error state before we attempt to talk to it.

    for (val = 0; val < WAKEUP_MSGS; val ++)
    {
        printf("Waiting for CAN ready, %d%% ...\n", (val * 100) / WAKEUP_MSGS);
        if (read(receive_channel, data, sizeof(data)) != sizeof(data))
        {
            printf("failed to receive DCM msg %d!\n", val);
            exit(EXIT_FAILURE);
        }
    }

    //sleep(5);

    val = EMPIRBUS_MASK;
    if (devctl(receive_channel, CAN_DEVCTL_SET_MFILTER, &val, sizeof(val), NULL))
    {
        printf("failed to set rx mfilter!\n");
        exit(EXIT_FAILURE);
    }

    val = EMPIRBUS_ID;
    if (devctl(receive_channel, CAN_DEVCTL_SET_MID, &val, sizeof(val), NULL))
    {
        printf("failed to set rx mid!\n");
        exit(EXIT_FAILURE);
    }

    // CAN is ready at this point, coninue with comms as usual
    printf("CAN ready!\n");



    //	Setup TX thread pool
	//	Create a Qnet service and publish it in the GNS
	name_attach_t *name_att;
    if((name_att = name_attach(NULL, "can", NAME_FLAG_ATTACH_GLOBAL)) == NULL)
    {
        //  Throw error
        return -1;
    }
    g_can_chid = name_att->chid;

    pthread_attr_init(&cansend_t_attr);
	struct sched_param param;

	pthread_attr_getschedparam(&cansend_t_attr, &param);
	pthread_attr_setinheritsched(&cansend_t_attr, PTHREAD_EXPLICIT_SCHED);
	param.sched_priority = 15;
	pthread_attr_setschedparam(&cansend_t_attr, &param);
    //  Set up the thread pool parameters
    memset(&cansend_tp_attr, 0, sizeof(cansend_tp_attr));
    cansend_tp_attr.handle = name_att->dpp;
    cansend_tp_attr.block_func = CanSendWaitForMessage;
    cansend_tp_attr.context_alloc = CanSendContextAlloc;
    cansend_tp_attr.handler_func = CanSendParseMessage;
    cansend_tp_attr.context_free = CanSendContextFree;
    cansend_tp_attr.lo_water = 2;
    cansend_tp_attr.hi_water = 4;
    cansend_tp_attr.increment = 1;
    cansend_tp_attr.maximum = 6;
    cansend_tp_attr.attr = &cansend_t_attr;
	//  Create a thread pool that returns
	cansend_tpp = thread_pool_create (&cansend_tp_attr, 0);
	if (cansend_tpp == NULL)
	{
		//  Could not create thread pool
		close(send_channel);
		return -1;
	}

	//	Start thread pools
	if (thread_pool_start(cansend_tpp) == -1)
	{
		//  Could not create thread pool
		close(send_channel);
		return -1;
	}

	//  Setup RX thread pool
    pthread_attr_init (&canreceive_t_attr);
	dispatch_t  *dpp;
	dpp = dispatch_create();
    //  Set up the thread pool parameters
    memset(&canreceive_tp_attr, 0, sizeof(canreceive_tp_attr));
    canreceive_tp_attr.handle = dpp;
    canreceive_tp_attr.block_func = CanReceiveWaitForMessage;
    canreceive_tp_attr.context_alloc = CanReceiveContextAlloc;
    canreceive_tp_attr.handler_func = CanReceiveParseMessage;
    canreceive_tp_attr.context_free = CanReceiveContextFree;
    canreceive_tp_attr.lo_water = 2;
    canreceive_tp_attr.hi_water = 4;
    canreceive_tp_attr.increment = 1;
    canreceive_tp_attr.maximum = 6;
    canreceive_tp_attr.attr = &canreceive_t_attr;
	//  Create a thread pool that returns
	canreceive_tpp = thread_pool_create (&canreceive_tp_attr, 0);
	if (canreceive_tpp == NULL)
	{
		//  Could not create thread pool
		close(receive_channel);
		return -1;
	}

	//	Initialize mutexes
	g_cms_candata_mutex = PTHREAD_MUTEX_INITIALIZER;
	g_cms_cansend_mutex = PTHREAD_MUTEX_INITIALIZER;

    return 0;
}

int CanControl::SendAllMessages()
{
	unsigned char can_data[8];
	for(unsigned int message_it = 0; message_it < CmsData::Instance()->can_messages_.size(); message_it++)
	{
		pthread_mutex_lock(&g_cms_candata_mutex);
		can_data[ DBYTE0 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[0];
		can_data[ DBYTE1 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[1];
		can_data[ DBYTE2 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[2];
		can_data[ DBYTE3 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[3];
		can_data[ DBYTE4 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[4];
		can_data[ DBYTE5 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[5];
		can_data[ DBYTE6 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[6];
		can_data[ DBYTE7 ] = CmsData::Instance()->can_messages_.at(message_it).msg_data[7];
		pthread_mutex_unlock(&g_cms_candata_mutex);

		pthread_mutex_lock(&g_cms_cansend_mutex);
		if (write(send_channel, can_data, sizeof(can_data)) == sizeof(can_data))
		{
			//MsgReply(cansend_struct->control_rcvid, EOK, cansend_struct->reply_msg, sizeof(cansend_struct->reply_msg));
		}
		pthread_mutex_unlock(&g_cms_cansend_mutex);
	}
	return 0;
}

int CanControl::InitThreadPools()
{
	if (thread_pool_start(canreceive_tpp) == -1)
    {
		//  Could not create thread pool
        close(receive_channel);
        return -1;
    }

	//  If it gets here, thread pool is up and running
    return 0;
}

int CanControl::CloseChannels()
{
	if(send_channel != -1)
		close(send_channel);
	if(receive_channel != -1)
		close(receive_channel);

	return 0;
}
