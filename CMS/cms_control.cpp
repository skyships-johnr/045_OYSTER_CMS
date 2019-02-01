#include "cms_control.h"

//****************************************************************************
//  Global variables
//****************************************************************************
int g_control_chid;
int g_udp_client_socket;
int g_can_coid = -1;
int g_master_coid = -1;
struct addrinfo *g_udp_addrinfo;

pthread_mutex_t g_cms_data_mutex;
pthread_mutex_t g_cms_master_mutex;
pthread_mutex_t g_cms_can_mutex;
pthread_mutex_t g_cms_ui_mutex;
pthread_mutex_t g_udp_client_mutex;
pthread_mutex_t g_mmsaircon_mutex;


uint32_t g_udp_msgseq;

CmsControl *CmsControl::instance = 0;
//****************************************************************************
//  Constructor and destructor
//****************************************************************************
CmsControl::CmsControl()
{
	tpp = NULL;
}

CmsControl::~CmsControl()
{
	if (instance != 0)
		delete instance;
}

CmsControl * CmsControl::Instance()
{
	if (instance == 0)
        instance = new CmsControl;

    return instance;
}

//****************************************************************************
//  Thread pool lifecycle functions
//****************************************************************************
//  Context manipulation functions when thread is created and killed
THREAD_POOL_PARAM_T * CmsControl::ContextAlloc(THREAD_POOL_HANDLE_T *handle)
{
	resmgr_context_t *ctp = resmgr_context_alloc(handle);

	string shell_command;
	int counter;

    ControlServiceStruct *control_struct = new ControlServiceStruct;
	memset(control_struct->msg, 0xFF, sizeof(control_struct->msg));
	memset(control_struct->reply_msg, 0xFF, sizeof(control_struct->reply_msg));


    //  Initialize values;
    control_struct->comm_rcvid = -1;

    SETIOV(ctp->iov, control_struct, sizeof(*control_struct));
    return ctp;
}

//  Function to block waiting for a message from client
THREAD_POOL_PARAM_T * CmsControl::WaitForMessage(THREAD_POOL_PARAM_T *ctp)
{
    //  Cast pointer
    ControlServiceStruct *control_struct = (ControlServiceStruct *)ctp->iov->iov_base;
    //  Blocks waiting for message
    control_struct->comm_rcvid = MsgReceive(g_control_chid, control_struct->msg, sizeof(control_struct->msg), NULL);

	if(control_struct->comm_rcvid != 0)
	{
		delay(5); // kludge: without a delay here the app locks up :(
		int i = MsgReply(control_struct->comm_rcvid, EOK, control_struct->reply_msg, sizeof(control_struct->reply_msg));
	}

	//memset(control_struct->msg, 0xFF, sizeof(control_struct->msg));

    SETIOV(ctp->iov, control_struct, sizeof(*control_struct));
    return ctp;
}

//  Message parsing function when unblocks with received message
int CmsControl::ParseMessage(THREAD_POOL_PARAM_T *ctp)
{
    //  Cast pointer
    ControlServiceStruct *control_struct = (ControlServiceStruct *)ctp->iov->iov_base;

	if(control_struct->comm_rcvid == 0)
	{
		//	If it gets here, one of the timers sent a pulse
		if (control_struct->msg[4] == SETTINGS)
		{
			if(control_struct->msg[8] == (unsigned char)0x01)
			{
				//	If it enters here, the screen timeout was reached
				CmsUI::Instance()->ShutOffScreen_Request();
			}
			else if(control_struct->msg[8] == (unsigned char)0x02)
			{
				//	Means a save parameters timer was reached
				CmsData::Instance()->SaveAllValues();
			}
		}
		else if(control_struct->msg[4] == LIGHTING)
		{
			//	A light dimmer timer was triggered
			if(control_struct->msg[8] == (unsigned char)0x01)
			{
				CmsData::Instance()->IncreaseGroupLight(control_struct->msg[10], control_struct->msg[9], true);
				CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[10], control_struct->msg[9]);
			}
			else if(control_struct->msg[8] == (unsigned char)0x00)
			{
				CmsData::Instance()->DecreaseGroupLight(control_struct->msg[10], control_struct->msg[9], true);
				CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[10], control_struct->msg[9]);
			}
		}
		else if(control_struct->msg[4] == FANS)
		{
			//	The boost timeout was reached
			CmsData::Instance()->UnboostFanLevel(control_struct->msg[8]);
		}
		else if(control_struct->msg[4] == AIRCON)
		{
			//	The setpoint timeout was reached
			CmsData::Instance()->cms_self_.climate_.aircon_.showing_setpoint_ = false;
			CmsUI::Instance()->UpdateAirconTemperature_Request();
		}
	}
	else
	{
	    if ((char)control_struct->msg[40] == 'm')
	    {
			//	Means it's a connection from one of my own
	    	if((char)control_struct->msg[8] == 'x')
			{
				memset(control_struct->msg, 0xFF, sizeof(control_struct->msg));
				return 1;
			}

			if(CmsData::Instance()->cms_self_.is_master)
			{
				char sub_message[7];
				char right_message[7] = "master";
				strncpy(sub_message, (char *)(&(control_struct->msg[40])), 6);
				if(strcmp(sub_message,right_message))
				{
					//	Means it's a connection from a slave
					delay(500);
					//	If it enters here, it means a connection or disconnection happened in the network
					//	Update connection to all slaves
					for(unsigned int cms_it = 0; cms_it < CmsData::Instance()->cms_slaves_.size(); cms_it++)
					{
						pthread_mutex_lock(&CmsData::Instance()->cms_slaves_.at(cms_it).control_mutex);
						if(CmsData::Instance()->cms_slaves_.at(cms_it).control_coid == -1)
							name_close(CmsData::Instance()->cms_slaves_.at(cms_it).control_coid);
						delay(100);
						CmsData::Instance()->cms_slaves_.at(cms_it).control_coid =
						name_open(CmsData::Instance()->cms_slaves_.at(cms_it).hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);
						delay(500);

						if(CmsData::Instance()->cms_slaves_.at(cms_it).control_coid != -1)
							CmsData::Instance()->SendLightSyncValues(cms_it);
						pthread_mutex_unlock(&CmsData::Instance()->cms_slaves_.at(cms_it).control_mutex);
					}
				}
				memset(control_struct->msg, 0xFF, sizeof(control_struct->msg));
				return 1;
			}
	    }
		else
		{
			//	If the message is coming from the UI
			if (control_struct->msg[0] == SCREEN)
			{
				//	First light up screen (in case is off) and reset timeout timer
				CmsUI::Instance()->WakeUpScreen_Request();

				//	Check if it's coming from my own screen
				if(control_struct->msg[1] == CmsData::Instance()->self_id_)
				{
					//	LIGHTS
					if(control_struct->msg[2] == LIGHTING)
					{
						//	Intensity change
						if(control_struct->msg[6] == (unsigned char)0x00)
						{
							//	Increase
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->IncreaseLight(control_struct->msg[5], control_struct->msg[3], control_struct->msg[4], true);
								CmsUI::Instance()->UpdateLightValue_Request(control_struct->msg[5], control_struct->msg[3], control_struct->msg[4]);
							}
							//	Decrease
							else if (control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->DecreaseLight(control_struct->msg[5], control_struct->msg[3], control_struct->msg[4], true);
								CmsUI::Instance()->UpdateLightValue_Request(control_struct->msg[5], control_struct->msg[3], control_struct->msg[4]);
							}
						}
						//	Master Switch
						else if(control_struct->msg[6] == (unsigned char)0x01)
						{
							CmsData::Instance()->SendMasterSwitch((bool)control_struct->msg[7]);

							if(CmsData::Instance()->cms_self_.is_master)
							{
								bool all_off = CmsData::Instance()->all_off_;
								if(all_off)
								{
									//CmsData::Instance()->BroadcastOff();
									CmsUI::Instance()->UpdateAll_Request();
								}
							}
						}
					}
					else if(control_struct->msg[2] == LIGHTING_PROFILE)
					{
						if(control_struct->msg[7] == (unsigned char)0x00)
							CmsData::Instance()->SetProfile(control_struct->msg[5], control_struct->msg[3], Lighting::DAY, true);
						else if(control_struct->msg[7] == (unsigned char)0x01)
							CmsData::Instance()->SetProfile(control_struct->msg[5], control_struct->msg[3], Lighting::EVE, true);
						else if(control_struct->msg[7] == (unsigned char)0x02)
							CmsData::Instance()->SetProfile(control_struct->msg[5], control_struct->msg[3], Lighting::NIGHT, true);
						else if(control_struct->msg[7] == (unsigned char)0x03)
							CmsData::Instance()->SetProfile(control_struct->msg[5], control_struct->msg[3], Lighting::THEATRE, true);
						else if(control_struct->msg[7] == (unsigned char)0x04)
							CmsData::Instance()->SetProfile(control_struct->msg[5], control_struct->msg[3], Lighting::SHOWER, true);

						CmsData::Instance()->SetScreenBrightness();
						CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[5], control_struct->msg[3]);
						CmsUI::Instance()->UpdateScreenBrightness_Request();
					}

					//	BLINDS
					else if(control_struct->msg[2] == BLINDS)
					{
						//	Open
						if(control_struct->msg[6] == (unsigned char)0x01)
						{
							//	Start
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->OpenBlinds(control_struct->msg[3], control_struct->msg[4]);
							}
							//	Stop
							else if(control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->OpenBlindsStop(control_struct->msg[3], control_struct->msg[4]);
							}
						}
						//	Close
						else if (control_struct->msg[6] == (unsigned char)0x00)
						{
							//	Start
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->CloseBlinds(control_struct->msg[3], control_struct->msg[4]);
							}
							//	Stop
							else if(control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->CloseBlindsStop(control_struct->msg[3], control_struct->msg[4]);
							}
						}
					}

					//	CLIMATE
					else if(control_struct->msg[2] == AIRCON)
					{
						//	Temperature
						if(control_struct->msg[6] == (unsigned char)0x00)
						{
							//	Increase
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->IncreaseAirconTemperature();
								//CmsUI::Instance()->UpdateAirconTemperature_Request();
							}
							//	Decrease
							else if(control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->DecreaseAirconTemperature();
								//CmsUI::Instance()->UpdateAirconTemperature_Request();
							}
						}
						//	Power
						else if(control_struct->msg[6] == (unsigned char)0x01)
						{
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->SwitchAirconPower();
								//CmsUI::Instance()->UpdateAirconPower_Request();
								//CmsUI::Instance()->UpdateAirconMode_Request();
							}
						}
						//	Mode
						else if(control_struct->msg[6] == (unsigned char)0x02)
						{
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->ChangeAirconMode();
								//CmsUI::Instance()->UpdateAirconMode_Request();
							}
						}
						//	Fan speed
						else if(control_struct->msg[6] == (unsigned char)0x03)
						{
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->ChangeAirconFanSpeed();
								//CmsUI::Instance()->UpdateAirconFanSpeed_Request();
							}
						}
					}
					else if(control_struct->msg[2] == HEATING)
					{
						//	Temperature
						if(control_struct->msg[6] == (unsigned char)0x00)
						{
							//	Increase
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->IncreaseHeatingTemperature(control_struct->msg[3]);
								CmsUI::Instance()->UpdateHeatingTemperature_Request(control_struct->msg[3]);
							}
							//	Decrease
							else if(control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->DecreaseHeatingTemperature(control_struct->msg[3]);
								CmsUI::Instance()->UpdateHeatingTemperature_Request(control_struct->msg[3]);
							}
						}
						//	Power
						else if(control_struct->msg[6] == (unsigned char)0x01)
						{
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->SwitchHeatingPower(control_struct->msg[3], control_struct->msg[4]);
								CmsUI::Instance()->UpdateHeatingPower_Request(control_struct->msg[3], control_struct->msg[4]);
								//CmsUI::Instance()->UpdateAirconMode_Request();
							}
						}
						//	Master Switch
						else if(control_struct->msg[6] == (unsigned char)0x02)
						{
							if(control_struct->msg[7] == (unsigned char)0x00)
							{
								if(CmsData::Instance()->cms_self_.is_master)
								{
									CmsData::Instance()->TurnOffHeating();
									CmsUI::Instance()->UpdateAllHeatingPower_Request();
								}
								else
								{
									CmsData::Instance()->RequestTurnOffHeating();
								}
							}
						}
					}

					//	FANS
					else if(control_struct->msg[2] == FANS)
					{
						//	If it's a value change
						if(control_struct->msg[6] == (unsigned char)0x00)
						{
							//	Increase
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->IncreaseFanLevel(control_struct->msg[3], control_struct->msg[4]);
								CmsUI::Instance()->UpdateFanValue_Request(control_struct->msg[3], control_struct->msg[4]);
							}
							//	Decrease
							else if (control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->DecreaseFanLevel(control_struct->msg[3], control_struct->msg[4]);
								CmsUI::Instance()->UpdateFanValue_Request(control_struct->msg[3], control_struct->msg[4]);
							}
						}
					}

					//	SETTINGS
					else if(control_struct->msg[2] == SETTINGS)
					{
						//	Screen Brightness
						if(control_struct->msg[6] == (unsigned char)0x00)
						{
							//	Increase
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->IncreaseScreenBrightness();
								CmsUI::Instance()->UpdateScreenBrightness_Request();
							}
							//	Decrease
							else if (control_struct->msg[7] == (char)0x00)
							{
								CmsData::Instance()->DecreaseScreenBrightness();
								CmsUI::Instance()->UpdateScreenBrightness_Request();
							}
						}
						//	Screen Timeout
						else if(control_struct->msg[6] == (unsigned char)0x01)
						{
							//	Increase
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->IncreaseScreenTimeout();
								CmsData::Instance()->UpdateScreenTimer();
								CmsUI::Instance()->UpdateScreenTimeout_Request();
							}
							//	Decrease
							else if (control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->DecreaseScreenTimeout();
								CmsData::Instance()->UpdateScreenTimer();
								CmsUI::Instance()->UpdateScreenTimeout_Request();
							}
						}
						//	Time settings
						else if(control_struct->msg[6] == (unsigned char)0x02)
						{
							//	Dawn start
							if(control_struct->msg[4] == (unsigned char)0x00)
							{
								//	Increase
								if(control_struct->msg[7] == (unsigned char)0x01)
								{
									CmsData::Instance()->IncreaseDawnStartTime();
									CmsUI::Instance()->UpdateDawnStartTime_Request();
								}
								//	Decrease
								else if (control_struct->msg[7] == (unsigned char)0x00)
								{
									CmsData::Instance()->DecreaseDawnStartTime();
									CmsUI::Instance()->UpdateDawnStartTime_Request();
								}
							}
							//	Dawn end
							else if(control_struct->msg[4] == (unsigned char)0x01)
							{
								//	Increase
								if(control_struct->msg[7] == (unsigned char)0x01)
								{
									CmsData::Instance()->IncreaseDawnEndTime();
									CmsUI::Instance()->UpdateDawnEndTime_Request();
								}
								//	Decrease
								else if (control_struct->msg[7] == (unsigned char)0x00)
								{
									CmsData::Instance()->DecreaseDawnEndTime();
									CmsUI::Instance()->UpdateDawnEndTime_Request();
								}
							}
							//	Dusk start
							else if(control_struct->msg[4] == (unsigned char)0x02)
							{
								//	Increase
								if(control_struct->msg[7] == (unsigned char)0x01)
								{
									CmsData::Instance()->IncreaseDuskStartTime();
									CmsUI::Instance()->UpdateDuskStartTime_Request();
								}
								//	Decrease
								else if (control_struct->msg[7] == (unsigned char)0x00)
								{
									CmsData::Instance()->DecreaseDuskStartTime();
									CmsUI::Instance()->UpdateDuskStartTime_Request();
								}
							}
							//	Dusk end
							else if(control_struct->msg[4] == (unsigned char)0x03)
							{
								//	Increase
								if(control_struct->msg[7] == (unsigned char)0x01)
								{
									CmsData::Instance()->IncreaseDuskEndTime();
									CmsUI::Instance()->UpdateDuskEndTime_Request();
								}
								//	Decrease
								else if (control_struct->msg[7] == (unsigned char)0x00)
								{
									CmsData::Instance()->DecreaseDuskEndTime();
									CmsUI::Instance()->UpdateDuskEndTime_Request();
								}
							}
						}
						//	Reset
						else if(control_struct->msg[6] == (unsigned char)0x03)
						{
							//	quick solution, needs to be generalised
							if(control_struct->msg[3] == (unsigned char)0x00)
							{
								//	Reset guests
								if(control_struct->msg[4] == (unsigned char)0x00)
									CmsData::Instance()->ResetDefaults(false);
								//	Reset all
								else if (control_struct->msg[4] == (unsigned char)0x01)
									CmsData::Instance()->ResetDefaults(true);

								CmsUI::Instance()->UpdateAll_Request();
							}
							else if(control_struct->msg[3] == (unsigned char)0x01)
							{
								if(control_struct->msg[4] == (unsigned char)0x00)
								{
									CmsData::Instance()->SendFuseReset();
								}
							}
						}
						else if(control_struct->msg[6] == (unsigned char)0x05)
						{
							bool is_enabled = CmsData::Instance()->cms_self_.climate_.master_heatingenable_;
							if(is_enabled)
								CmsData::Instance()->TurnOffHeating();

							CmsData::Instance()->SwitchHeatingEnable(!is_enabled);
							CmsUI::Instance()->UpdateHeatingEnable_Request();

						}
					}
				}
			}

		    //  If it's coming from itself, it's and outgoing message
			else if (control_struct->msg[0] == CmsData::Instance()->self_id_)
		    {
		        //  Check who's going to
		        if(control_struct->msg[1] == CmsData::Instance()->master_id_)
		        {
		            //  Pass the message over to master control service
		        	if(g_master_coid != -1)
		        	{
		        		MsgSend(g_master_coid, control_struct->msg, sizeof(control_struct->msg),
		            		control_struct->reply_msg, sizeof(control_struct->reply_msg));
		        	}
		        }
		        else if (control_struct->msg[1] == CmsData::Instance()->can_id_)
		        {
		            //  Pass the message over to CAN server
		            MsgSend(g_can_coid, control_struct->msg, sizeof(control_struct->msg),
		            		control_struct->reply_msg, sizeof(control_struct->reply_msg));

		        }
		        else if(control_struct->msg[1] == CmsData::Instance()->mms_id_)
		        {
					if(control_struct->msg[2] == AIRCON)
					{
			            //  If it's sending to MMS, means is aircon control or heating status
						CMS_MSG_GENERIC mms_msg;
						CMS_PARAM_UPDATE cms_param;

						//	Set the fixed part
						mms_msg.header.seq = ++g_udp_msgseq;
						mms_msg.header.src = (uint8_t)CmsData::Instance()->self_id_;
				        mms_msg.header.dest = CmsData::Instance()->mms_id_;
						mms_msg.header.size = sizeof(cms_param);
				        mms_msg.header.id = CMS_MSG_ID_PARAM_UPDATE; 		// CMS_MSG_ID_PARAM_UPDATE from CMS1
						cms_param.numparams = 2;
						//	Set command hash and value
						if(control_struct->msg[4] == (unsigned char)0x00)
						{
							//	Temperature
							cms_param.params[0].hash = CmsData::Instance()->cms_self_.climate_.aircon_.mode_out_hash;
							cms_param.params[0].value = (int)CmsData::Instance()->GetAirconMode();

							cms_param.params[1].hash = CmsData::Instance()->cms_self_.climate_.aircon_.temperature_out_hash;
							cms_param.params[1].value = (int)CmsData::Instance()->GetAirconTemperature();
						}
						else if(control_struct->msg[4] == (unsigned char)0x01)
						{
							//	Fan speed
							cms_param.params[0].hash = CmsData::Instance()->cms_self_.climate_.aircon_.fan_speed_out_hash;
							cms_param.params[0].value = (int)CmsData::Instance()->GetAirconFanSpeed() + 1;

							cms_param.params[1].hash = CmsData::Instance()->cms_self_.climate_.aircon_.temperature_out_hash;
							cms_param.params[1].value = (int)CmsData::Instance()->GetAirconTemperature();
						}
						else if(control_struct->msg[4] == (unsigned char)0x02)
						{
							//	Power
							cms_param.params[0].hash = CmsData::Instance()->cms_self_.climate_.aircon_.mode_out_hash;
							if(control_struct->msg[7] == 1)
								cms_param.params[0].value = (int)Climate::AUTO;
							else
								cms_param.params[0].value = 1;
							cms_param.params[1].hash = CmsData::Instance()->cms_self_.climate_.aircon_.temperature_out_hash;
							cms_param.params[1].value = (int)CmsData::Instance()->GetAirconTemperature();
						}
						else if(control_struct->msg[4] == (unsigned char)0x03)
						{
							//	Mode
							cms_param.params[0].hash = CmsData::Instance()->cms_self_.climate_.aircon_.mode_out_hash;
							cms_param.params[0].value = (int)CmsData::Instance()->GetAirconMode();

							cms_param.params[1].hash = CmsData::Instance()->cms_self_.climate_.aircon_.temperature_out_hash;
							cms_param.params[1].value = (int)CmsData::Instance()->GetAirconTemperature();
						}

						memcpy(mms_msg.data, &cms_param, sizeof(cms_param));

						sendto(g_udp_client_socket, &mms_msg, sizeof(mms_msg), 0, g_udp_addrinfo->ai_addr, g_udp_addrinfo->ai_addrlen);

					}

		        }
		        else if (CmsData::Instance()->cms_self_.is_master)
		        {
		            //  If it enters here it means I am the master, and I am messaging a client

		            //  Find the slave we want to message
		            unsigned int slave_index = 0;
		            for(slave_index = 0; slave_index < CmsData::Instance()->cms_slaves_.size(); slave_index++)
		            {
		            	if(CmsData::Instance()->cms_slaves_.at(slave_index).id == control_struct->msg[1])
		            		break;
		            }
		            //	Slave not found
		            if(slave_index == CmsData::Instance()->cms_slaves_.size())
		            	return -1;

		            if(CmsData::Instance()->cms_slaves_.at(slave_index).control_coid != -1)
					{
						//  If it got here, slave is active and has a valid connection ID

						MsgSend(CmsData::Instance()->cms_slaves_.at(slave_index).control_coid, control_struct->msg, sizeof(control_struct->msg),
			            		control_struct->reply_msg, sizeof(control_struct->reply_msg));

					}

		        }
		    }
		    //  If it's coming from master, might be light changing requests or resets
		    else if (control_struct->msg[0] == CmsData::Instance()->master_id_)
		    {
				//	Check if it's for me
				if(control_struct->msg[1] == CmsData::Instance()->self_id_)
				{
					if(control_struct->msg[2] == LIGHTING)
					{
						//	Increase
						if(control_struct->msg[7] == (unsigned char)0x01)
						{
							CmsData::Instance()->IncreaseLight(CmsData::Instance()->self_id_, control_struct->msg[3], control_struct->msg[4], false);
							CmsUI::Instance()->UpdateLightValue_Request(CmsData::Instance()->self_id_, control_struct->msg[3], control_struct->msg[4]);
						}
						//	Decrease
						else if (control_struct->msg[7] == (unsigned char)0x00)
						{
							CmsData::Instance()->DecreaseLight(CmsData::Instance()->self_id_, control_struct->msg[3], control_struct->msg[4], false);
							CmsUI::Instance()->UpdateLightValue_Request(CmsData::Instance()->self_id_, control_struct->msg[3], control_struct->msg[4]);
						}
						//	Increase group
						else if(control_struct->msg[7] == (unsigned char)0x11)
						{
							CmsData::Instance()->IncreaseGroupLight(CmsData::Instance()->self_id_, control_struct->msg[3], false);
							CmsUI::Instance()->UpdateProfile_Request(CmsData::Instance()->self_id_, control_struct->msg[3]);
						}
						//	Decrease group
						else if (control_struct->msg[7] == (unsigned char)0x10)
						{
							CmsData::Instance()->DecreaseGroupLight(CmsData::Instance()->self_id_, control_struct->msg[3], false);
							CmsUI::Instance()->UpdateProfile_Request(CmsData::Instance()->self_id_, control_struct->msg[3]);
						}
					}
					else if(control_struct->msg[2] == LIGHTING_PROFILE)
					{
						if(control_struct->msg[7] == (unsigned char)0x00)
							CmsData::Instance()->SetProfile(CmsData::Instance()->self_id_, control_struct->msg[3], Lighting::DAY, false);
						else if(control_struct->msg[7] == (unsigned char)0x01)
							CmsData::Instance()->SetProfile(CmsData::Instance()->self_id_, control_struct->msg[3], Lighting::EVE, false);
						else if(control_struct->msg[7] == (unsigned char)0x02)
							CmsData::Instance()->SetProfile(CmsData::Instance()->self_id_, control_struct->msg[3], Lighting::NIGHT, false);
						else if(control_struct->msg[7] == (unsigned char)0x03)
							CmsData::Instance()->SetProfile(CmsData::Instance()->self_id_, control_struct->msg[3], Lighting::THEATRE, false);
						else if(control_struct->msg[7] == (unsigned char)0x04)
							CmsData::Instance()->SetProfile(CmsData::Instance()->self_id_, control_struct->msg[3], Lighting::SHOWER, false);
						else if(control_struct->msg[7] == (unsigned char)0x06)
							CmsData::Instance()->SetProfile(CmsData::Instance()->self_id_, control_struct->msg[3], Lighting::OFF, false);

						bool is_sleeping = CmsUI::Instance()->GetSleepStatus();

						if(!is_sleeping)
							CmsData::Instance()->SetScreenBrightness();

						CmsUI::Instance()->UpdateProfile_Request(CmsData::Instance()->self_id_, control_struct->msg[3]);
						CmsUI::Instance()->UpdateScreenBrightness_Request();
					}
					else if(control_struct->msg[2] == HEATING)
					{
						//	Master Switch
						if(control_struct->msg[6] == (unsigned char)0x02)
						{
							if(control_struct->msg[7] == (unsigned char)0x00)
							{
								CmsData::Instance()->TurnOffHeating();
								CmsUI::Instance()->UpdateAllHeatingPower_Request();
							}
						}
					}
					else if(control_struct->msg[2] == SETTINGS)
					{
						if(control_struct->msg[6] == (unsigned char)0x03)
						{
							CmsData::Instance()->ResetDefaults(false);
							CmsUI::Instance()->UpdateAll_Request();
						}
						else if(control_struct->msg[6] == (unsigned char)0x05)
						{
							CmsData::Instance()->SwitchHeatingEnable((bool)control_struct->msg[7]);
							CmsData::Instance()->TurnOffHeating();
							CmsUI::Instance()->UpdateAllHeatingPower_Request();
							//CmsUI::Instance()->UpdateHeatingEnable_Request();
						}
					}
					else if(control_struct->msg[2] == LIGHT_SYNC)
					{
						CmsData::Instance()->SyncLightValue((int)control_struct->msg[3], (int)control_struct->msg[4], (int)control_struct->msg[6], (int)control_struct->msg[7]);
						CmsUI::Instance()->UpdateProfile_Request(CmsData::Instance()->self_id_, control_struct->msg[3]);
					}
				}

		    }

		    //  If it's coming from CAN, might be a request to change light profile or group intensity
		    else if (control_struct->msg[0] == CmsData::Instance()->can_id_)
		    {
				if(control_struct->msg[2] == LIGHTING)
				{
					//	Increase group start
					if(control_struct->msg[7] == (unsigned char)0x11)
					{
						CmsData::Instance()->StartIncreaseGroupTimer(control_struct->msg[1], control_struct->msg[3]);
						CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[1], control_struct->msg[3]);
					}
					//	Decrease group start
					else if (control_struct->msg[7] == (unsigned char)0x10)
					{
						CmsData::Instance()->StartDecreaseGroupTimer(control_struct->msg[1], control_struct->msg[3]);
						CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[1], control_struct->msg[3]);
					}
					else if(control_struct->msg[7] == (unsigned char)0xA0)
					{
						CmsData::Instance()->StopGroupTimer(control_struct->msg[1], control_struct->msg[3]);
						CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[1], control_struct->msg[3]);
					}
				}
				else if (control_struct->msg[2] == LIGHTING_PROFILE)
				{
					if(control_struct->msg[7] == (unsigned char)0x00)
						CmsData::Instance()->SetProfile(control_struct->msg[1], control_struct->msg[3], Lighting::DAY, true);
					else if(control_struct->msg[7] == (unsigned char)0x01)
						CmsData::Instance()->SetProfile(control_struct->msg[1], control_struct->msg[3], Lighting::EVE, true);
					else if(control_struct->msg[7] == (unsigned char)0x02)
						CmsData::Instance()->SetProfile(control_struct->msg[1], control_struct->msg[3], Lighting::NIGHT, true);
					else if(control_struct->msg[7] == (unsigned char)0x03)
						CmsData::Instance()->SetProfile(control_struct->msg[1], control_struct->msg[3], Lighting::THEATRE, true);
					else if(control_struct->msg[7] == (unsigned char)0x04)
						CmsData::Instance()->SetProfile(control_struct->msg[1], control_struct->msg[3], Lighting::SHOWER, true);
					else if(control_struct->msg[7] == (unsigned char)0x06)
						CmsData::Instance()->SetProfile(control_struct->msg[1], control_struct->msg[3], Lighting::OFF, true);

					bool is_sleeping = CmsUI::Instance()->GetSleepStatus();

					if(!is_sleeping)
						CmsData::Instance()->SetScreenBrightness();

					CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[1], control_struct->msg[3]);
					CmsUI::Instance()->UpdateScreenBrightness_Request();
				}
		    }

		    //  If it's coming from MMS, might be aircon, dawn/dusk time adjustments
		    else if (control_struct->msg[0] == CmsData::Instance()->mms_id_)
		    {
				if(control_struct->msg[1] == CmsData::Instance()->self_id_)
				{
					if(control_struct->msg[2] == AIRCON)
					{
						//	Temperature
						if(control_struct->msg[6] == (unsigned char)0x00)
						{
							pthread_mutex_lock(&g_mmsaircon_mutex);
							CmsData::Instance()->SetAirconTemperature((int)control_struct->msg[7]);
							CmsUI::Instance()->UpdateAirconSetPointTemperature_Request();
							CmsUI::Instance()->UpdateAirconTemperature_Request();
							pthread_mutex_unlock(&g_mmsaircon_mutex);
						}
						//	Power
						else if(control_struct->msg[6] == (unsigned char)0x01)
						{
							if(control_struct->msg[7] == (unsigned char)0x01)
							{
								CmsData::Instance()->SwitchAirconPower();
								CmsUI::Instance()->UpdateAirconPower_Request();
							}
						}
						//	Mode
						else if(control_struct->msg[6] == (unsigned char)0x02)
						{
							pthread_mutex_lock(&g_mmsaircon_mutex);
							CmsData::Instance()->SetAirconMode(control_struct->msg[7]);
							CmsUI::Instance()->UpdateAirconMode_Request();
							if(control_struct->msg[7] == 1)
							{
								CmsData::Instance()->SetAirconPower(false);
								CmsUI::Instance()->UpdateAirconPower_Request();
							}
							else
							{
								CmsData::Instance()->SetAirconPower(true);
								CmsUI::Instance()->UpdateAirconPower_Request();
							}
							pthread_mutex_unlock(&g_mmsaircon_mutex);
						}
						//	Fan speed
						else if(control_struct->msg[6] == (unsigned char)0x03)
						{
							pthread_mutex_lock(&g_mmsaircon_mutex);
							CmsData::Instance()->SetAirconFanSpeed(control_struct->msg[7]);
							CmsUI::Instance()->UpdateAirconFanSpeed_Request();
							pthread_mutex_unlock(&g_mmsaircon_mutex);
						}
						//	Ambient Temperature
						else if(control_struct->msg[6] == (unsigned char)0x04)
						{
							pthread_mutex_lock(&g_mmsaircon_mutex);
							CmsData::Instance()->SetAirconAmbientTemperature((int)control_struct->msg[7]);
							CmsUI::Instance()->UpdateAirconAmbientTemperature_Request();
							CmsUI::Instance()->UpdateAirconTemperature_Request();
							pthread_mutex_unlock(&g_mmsaircon_mutex);
						}
					}
					else if(control_struct->msg[2] == SETTINGS)
					{
						if(control_struct->msg[3] == (unsigned char)0x00 && control_struct->msg[7] == (unsigned char)0x00)
						{
							CmsUI::Instance()->UpdateSunriseTime_Request();
							CmsUI::Instance()->UpdateSunsetTime_Request();
						}
					}
				}
		    }
		 	 else if (CmsData::Instance()->cms_self_.is_master && control_struct->msg[1] == CmsData::Instance()->master_id_)
		    {
		        //  If it enters here it means I am the master, and a client messaged me
				if(control_struct->msg[2] == LIGHTING)
				{
					if(control_struct->msg[6] == (unsigned char)0x00)
					{
						//	Increase
						if(control_struct->msg[7] == (unsigned char)0x01)
						{
							CmsData::Instance()->IncreaseLight(control_struct->msg[0], control_struct->msg[3], control_struct->msg[4], false);
							CmsUI::Instance()->UpdateLightValue_Request(control_struct->msg[0], control_struct->msg[3], control_struct->msg[4]);
						}
						//	Decrease
						else if (control_struct->msg[7] == (unsigned char)0x00)
						{
							CmsData::Instance()->DecreaseLight(control_struct->msg[0], control_struct->msg[3], control_struct->msg[4], false);
							CmsUI::Instance()->UpdateLightValue_Request(control_struct->msg[0], control_struct->msg[3], control_struct->msg[4]);
						}
					}
					else if(control_struct->msg[6] == (unsigned char)0x01)
					{
						//	A request to bradcast an OFF message was received
						CmsData::Instance()->SendMasterSwitch((bool)control_struct->msg[7]);

						bool all_off = CmsData::Instance()->all_off_;
						if(all_off)
						{
							//CmsData::Instance()->BroadcastOff();
							CmsUI::Instance()->UpdateAll_Request();
						}
					}
				}
				else if(control_struct->msg[2] == LIGHTING_PROFILE)
				{
					if(control_struct->msg[7] == (unsigned char)0x00)
						CmsData::Instance()->SetProfile(control_struct->msg[0], control_struct->msg[3], Lighting::DAY, false);
					else if(control_struct->msg[7] == (unsigned char)0x01)
						CmsData::Instance()->SetProfile(control_struct->msg[0], control_struct->msg[3], Lighting::EVE, false);
					else if(control_struct->msg[7] == (unsigned char)0x02)
						CmsData::Instance()->SetProfile(control_struct->msg[0], control_struct->msg[3], Lighting::NIGHT, false);
					else if(control_struct->msg[7] == (unsigned char)0x03)
						CmsData::Instance()->SetProfile(control_struct->msg[0], control_struct->msg[3], Lighting::THEATRE, false);
					else if(control_struct->msg[7] == (unsigned char)0x04)
						CmsData::Instance()->SetProfile(control_struct->msg[0], control_struct->msg[3], Lighting::SHOWER, false);

					CmsUI::Instance()->UpdateProfile_Request(control_struct->msg[0], control_struct->msg[3]);
					CmsUI::Instance()->UpdateScreenBrightness_Request();
				}
				else if(control_struct->msg[2] == HEATING)
				{
					if(control_struct->msg[6] == (unsigned char)0x02)
					{
						if(control_struct->msg[7] == (unsigned char)0x00)
						{
							CmsData::Instance()->TurnOffHeating();
							CmsUI::Instance()->UpdateAllHeatingPower_Request();
						}
					}
				}
		    }
		}
	}
	//	Clean message
	memset(control_struct->msg, 0xFF, sizeof(control_struct->msg));
    return 1;
}

void CmsControl::ContextFree(THREAD_POOL_PARAM_T *ctp)
{
    //  Cast pointer
    ControlServiceStruct *control_struct = (ControlServiceStruct *)ctp->iov->iov_base;

    //  Clean up
    delete control_struct;
    resmgr_context_free(ctp);
}


//****************************************************************************
//  Communications set up and start
//****************************************************************************
int CmsControl::InitController()
{
    //  Set up the the server path and attach to channel
    name_attach_t *name_att;
    if((name_att = name_attach(NULL, CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL)) == NULL)
    {
        //  Throw error
        return -1;
    }
    g_control_chid = name_att->chid;

	//	Set UDP socket
	g_udp_client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	//	Set up UDP client address info
	char decimal_port[16];
	snprintf(decimal_port, sizeof(decimal_port), "%d", CmsData::Instance()->mms_port_);
	decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	getaddrinfo(CmsData::Instance()->mms_ipaddr_.c_str(), decimal_port, &hints, &(g_udp_addrinfo));

	//	Initialise udp counter
	g_udp_msgseq = 0;

	//	Initialize mutexes
	g_cms_data_mutex = PTHREAD_MUTEX_INITIALIZER;
	g_cms_ui_mutex = PTHREAD_MUTEX_INITIALIZER;
	g_cms_master_mutex = PTHREAD_MUTEX_INITIALIZER;
	g_cms_can_mutex = PTHREAD_MUTEX_INITIALIZER;
	g_udp_client_mutex = PTHREAD_MUTEX_INITIALIZER;

	if(CmsData::Instance()->cms_self_.is_master)
	{
		g_can_coid = name_open("can", NAME_FLAG_ATTACH_GLOBAL);
	}
	else
	{
		g_master_coid = name_open("master", NAME_FLAG_ATTACH_GLOBAL);
		g_can_coid = name_open("can", NAME_FLAG_ATTACH_GLOBAL);
	}

	g_mmsaircon_mutex = PTHREAD_MUTEX_INITIALIZER;
    //  Set up the thread parameters for the threads that will populate the pool
    pthread_attr_init (&t_attr);

    //  Set up the thread pool parameters
    memset(&tp_attr, 0, sizeof(tp_attr));
    tp_attr.handle = name_att->dpp;
    tp_attr.block_func = WaitForMessage;
    tp_attr.context_alloc = ContextAlloc;
    tp_attr.handler_func = ParseMessage;
    tp_attr.context_free = ContextFree;
    tp_attr.lo_water = 2;
    tp_attr.hi_water = 4;
    tp_attr.increment = 1;
    tp_attr.maximum = 7;
    tp_attr.attr = &t_attr;

    //  Create a thread pool that returns
    tpp = thread_pool_create(&tp_attr, 0);
    if (tpp == NULL)
    {
        //  Could not create thread pool
        name_detach(name_att, 0);
        return -1;
    }

    if (thread_pool_start(tpp) == -1)
    {
        //  Could not start thread pool
        name_detach(name_att, 0);
        return -1;
    }

    //  If it gets here, thread pool is up and running
    return 0;
}
