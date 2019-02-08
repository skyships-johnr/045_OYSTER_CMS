#include "cms_data.h"
#include "xml_data.h"

CmsData *CmsData::instance = 0;

CmsData::CmsData()
{
	is_connected_ = false;
	control_coid_ = -1;
	can_coid_ = -1;
	change_flag_ = false;
	self_id_ = 0;
	master_id_ = 0;
	can_id_ = 0;
	mms_id_ = 0;
	mms_port_ = 0;
	self_port_ = 0;
	all_on_ = false;
	all_off_ = false;
	is_night_ = false;
	minutes_of_day_ = 0;
}

CmsData::~CmsData()
{
	if (instance != 0)
		delete instance;
}

//========================================================================================
//		Control methods
//========================================================================================

CmsData * CmsData::Instance()
{
	if (instance == 0)
        instance = new CmsData;

    return instance;
}

int CmsData::CommConnect()
{
	control_coid_ = name_open(cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);
	can_coid_ = name_open("can", NAME_FLAG_ATTACH_GLOBAL);

	return control_coid_;
}

int CmsData::InitScreenTimer()
{
	//	Set up the timer for screen Timeout
	SIGEV_PULSE_INIT(&screentimeout_event, control_coid_, SIGEV_PULSE_PRIO_INHERIT, SETTINGS, 0x01);

	if(timer_create (CLOCK_REALTIME, &screentimeout_event, &screentimeout_timerid) == -1)
	{
		printf("Could not create timer!");
		return -1;
	}

	screentimeout_timer.it_value.tv_sec = cms_self_.settings_.GetScreenTimeoutTime(cms_self_.settings_.screensettings_.screen_timeout);
	screentimeout_timer.it_value.tv_nsec = 0;
	screentimeout_timer.it_interval.tv_sec = 0;
	screentimeout_timer.it_interval.tv_nsec = 0;

	timer_settime(screentimeout_timerid, 0, &screentimeout_timer, NULL);

	return 0;
}

int CmsData::UpdateScreenTimer()
{
	screentimeout_timer.it_value.tv_sec = cms_self_.settings_.GetScreenTimeoutTime(cms_self_.settings_.screensettings_.screen_timeout);
	timer_settime(screentimeout_timerid, 0, &screentimeout_timer, NULL);

	return 0;
}

int CmsData::RestartScreenTimer()
{
	timer_settime(screentimeout_timerid, 0, &screentimeout_timer, NULL);

	return 0;
}

int CmsData::SendControlMessage(const unsigned char *msg)
{
	unsigned char reply_msg[8];
	int reply_status = MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
	return reply_status;
}

int CmsData::SendCanMessage(const unsigned char *msg)
{
	unsigned char reply_msg[8];
	int reply_status = MsgSend(can_coid_, msg, 8, reply_msg, sizeof(reply_msg));
	return reply_status;
}

int CmsData::InitValues()
{
	XmlData::Instance()->LoadXmlFile();
	XmlData::Instance()->GetCmsInfo();

	for (unsigned int page_it = 0; page_it < menu_pages_.size(); page_it++)
	{
		switch (menu_pages_.at(page_it))
		{
		case LIGHTING_PAGE:
			XmlData::Instance()->GetLightsInfo();
			break;

		case BLINDS_PAGE:
			XmlData::Instance()->GetBlindsInfo();
			break;

		case CLIMATE_PAGE:
			XmlData::Instance()->GetClimateInfo();
			break;

		case FANS_PAGE:
			XmlData::Instance()->GetFansInfo();
			break;

		case SETTINGS_PAGE:
			XmlData::Instance()->GetSettingsInfo();
			break;
		}
	}

	LoadMmsCommands();

	if(cms_self_.is_master)
	{
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it ++)
		{
			cms_slaves_.at(cms_it).control_mutex = PTHREAD_MUTEX_INITIALIZER;
		}
		cms_self_.lighting_.master_switch_.value = true;

		LoadCanMessages();
	}

	all_on_ = false;
	all_off_ = false;
	cms_self_.climate_.master_heatingenable_ = true;
	cms_self_.climate_.aircon_.showing_setpoint_ = false;

    cms_self_.settings_.screensettings_.screen_timeout = DEFAULT_SCREENTIMEOUT;
    cms_self_.settings_.screensettings_.screen_timeout_defaultvalue = DEFAULT_SCREENTIMEOUT;

	return 0;
}

int CmsData::InitControlTimers()
{
	//	Init SaveParameters timer

	SIGEV_PULSE_INIT(&saveparams_event_, control_coid_, SIGEV_PULSE_PRIO_INHERIT, SETTINGS, 0x02);
	if(timer_create (CLOCK_REALTIME, &saveparams_event_, &saveparams_timerid_) == -1)
	{
		printf("Could not create timer!");
		return -1;
	}
	saveparams_timer_.it_value.tv_sec = 20;
	saveparams_timer_.it_value.tv_nsec = 0;
	saveparams_timer_.it_interval.tv_sec = 20;
	saveparams_timer_.it_interval.tv_nsec = 0;
	//timer_settime(saveparams_timerid_, 0, &saveparams_timer_, NULL);

	//	Set timer for aircon setpoint/ambient temperature logic
	SIGEV_PULSE_INIT(&cms_self_.climate_.aircon_.showsetpoint_event, control_coid_,
		SIGEV_PULSE_PRIO_INHERIT, AIRCON, 0X01);
	if(timer_create (CLOCK_REALTIME, &cms_self_.climate_.aircon_.showsetpoint_event,
		&cms_self_.climate_.aircon_.showsetpoint_timerid) == -1)
	{
		printf("Could not create timer!");
		return -1;
	}
	cms_self_.climate_.aircon_.showsetpoint_timer.it_value.tv_sec = 3;
	cms_self_.climate_.aircon_.showsetpoint_timer.it_value.tv_nsec = 0;
	cms_self_.climate_.aircon_.showsetpoint_timer.it_interval.tv_sec = 0;
	cms_self_.climate_.aircon_.showsetpoint_timer.it_interval.tv_nsec = 0;

	if(InitScreenTimer() == -1)
		return -1;

	return 0;
}
int CmsData::LoadCanMessages()
{
	//	Find the number of CAN messages
	int max_instance = 0;

	//	Lights
	for(unsigned int group_it = 0; group_it < cms_self_.lighting_.lightgroups_.size(); group_it++)
	{
		for(unsigned int light_it = 0; light_it < cms_self_.lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
		{
			if(cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance > max_instance)
				max_instance = cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance;
		}
	}
	//	Blinds
	for(unsigned int group_it = 0; group_it < cms_self_.blinds_.blindsgroups_.size(); group_it++)
	{
		for(unsigned int blinds_it = 0; blinds_it < cms_self_.blinds_.blindsgroups_.at(group_it).blinds.size(); blinds_it++)
		{
			if(cms_self_.blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).is_group)
				continue;
			if(cms_self_.blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).open_message_instance > max_instance)
				max_instance = cms_self_.blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).open_message_instance;
			if(cms_self_.blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).close_message_instance > max_instance)
				max_instance = cms_self_.blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).close_message_instance;
		}
	}
	//	Climate
	for(unsigned int group_it = 0; group_it < cms_self_.climate_.heating_.size(); group_it++)
	{
		if(cms_self_.climate_.heating_.at(group_it).message_instance > max_instance)
			max_instance = cms_self_.climate_.heating_.at(group_it).message_instance;
		for(unsigned int switch_it = 0; switch_it < cms_self_.climate_.heating_.at(group_it).power_switches.size(); switch_it++)
		{
			if(cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance > max_instance)
				max_instance = cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance;
		}
	}

	//	Fans
	for(unsigned int group_it = 0; group_it < cms_self_.fans_.fansgroups_.size(); group_it++)
	{
		for(unsigned int fans_it = 0; fans_it < cms_self_.fans_.fansgroups_.at(group_it).fans.size(); fans_it++)
		{
			if(cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).type != Fans::BACKGROUND)
				continue;
			if(cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance > max_instance)
				max_instance = cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance ;
		}
	}
	//	Settings
	for(unsigned int group_it = 0; group_it < cms_self_.settings_.reset_groups_.size(); group_it++)
	{
		for(unsigned int reset_it = 0; reset_it < cms_self_.settings_.reset_groups_.at(group_it).resets.size(); reset_it++)
		{
			if(cms_self_.settings_.reset_groups_.at(group_it).resets.at(reset_it).reset_type != Settings::CAN)
				continue;
			if(cms_self_.settings_.reset_groups_.at(group_it).resets.at(reset_it).message_instance > max_instance)
				max_instance = cms_self_.settings_.reset_groups_.at(group_it).resets.at(reset_it).message_instance;
		}
	}

	//	Now do slaves
	for(unsigned int slave_it = 0; slave_it < cms_slaves_.size(); slave_it++)
	{
		//	Lights
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).lighting_.lightgroups_.size(); group_it++)
		{
			for(unsigned int light_it = 0; light_it < cms_slaves_.at(slave_it).lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
			{
				if(cms_slaves_.at(slave_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance > max_instance)
					max_instance = cms_slaves_.at(slave_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance;
			}
		}
		//	Blinds
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).blinds_.blindsgroups_.size(); group_it++)
		{
			for(unsigned int blinds_it = 0; blinds_it < cms_slaves_.at(slave_it).blinds_.blindsgroups_.at(group_it).blinds.size(); blinds_it++)
			{
				if(cms_slaves_.at(slave_it).blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).is_group)
					continue;
				if(cms_slaves_.at(slave_it).blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).open_message_instance > max_instance)
					max_instance = cms_slaves_.at(slave_it).blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).open_message_instance;
				if(cms_slaves_.at(slave_it).blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).close_message_instance > max_instance)
					max_instance = cms_slaves_.at(slave_it).blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).close_message_instance;
			}
		}
		//	Climate
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).climate_.heating_.size(); group_it++)
		{
			if(cms_slaves_.at(slave_it).climate_.heating_.at(group_it).message_instance > max_instance)
				max_instance = cms_slaves_.at(slave_it).climate_.heating_.at(group_it).message_instance;
			for(unsigned int switch_it = 0; switch_it < cms_slaves_.at(slave_it).climate_.heating_.at(group_it).power_switches.size(); switch_it++)
			{
				if(cms_slaves_.at(slave_it).climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance > max_instance)
					max_instance = cms_slaves_.at(slave_it).climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance;
			}
		}
		//	Fans
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).fans_.fansgroups_.size(); group_it++)
		{
			for(unsigned int fans_it = 0; fans_it < cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.size(); fans_it++)
			{
				if(cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).type != Fans::BACKGROUND)
					continue;
				if(cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance > max_instance)
					max_instance = cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance ;
			}
		}
		//	Settings
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).settings_.reset_groups_.size(); group_it++)
		{
			for(unsigned int reset_it = 0; reset_it < cms_slaves_.at(slave_it).settings_.reset_groups_.at(group_it).resets.size(); reset_it++)
			{
				if(cms_slaves_.at(slave_it).settings_.reset_groups_.at(group_it).resets.at(reset_it).reset_type != Settings::CAN)
					continue;
				if(cms_slaves_.at(slave_it).settings_.reset_groups_.at(group_it).resets.at(reset_it).message_instance > max_instance)
					max_instance = cms_slaves_.at(slave_it).settings_.reset_groups_.at(group_it).resets.at(reset_it).message_instance;
			}
		}
	}

	//	Grow messages vector
	CanMessage can_message;
	for(int message_it = 0; message_it <= max_instance; message_it++)
	{
		can_message.msg_id = (EMPIRBUS_ID | message_it);
		memset(can_message.msg_data, 0, sizeof(can_message.msg_data));
		can_message.msg_data[0] = (unsigned char)0x30;
		can_message.msg_data[1] = (unsigned char)0x99;
		can_message.msg_data[2] = (unsigned char)message_it;
		can_message.msg_data[3] = (unsigned char)0x00;
		can_message.msg_data[4] = (unsigned char)0x00;
		can_message.msg_data[5] = (unsigned char)0x00;
		can_message.msg_data[6] = (unsigned char)0x00;
		can_message.msg_data[7] = (unsigned char)0x00;

		can_messages_.push_back(can_message);
	}

	//	Populate vector with initial values
	//	Lights
	for(unsigned int group_it = 0; group_it < cms_self_.lighting_.lightgroups_.size(); group_it++)
	{
		for(unsigned int light_it = 0; light_it < cms_self_.lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
		{
			can_messages_.at(cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance).
			msg_data[cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).frame_position] =
			(unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).night_value; // ##INITIAL DAY NIGHT MODE
		}
	}
	//	Blinds do not have initial values

	//	Climate
	for(unsigned int group_it = 0; group_it < cms_self_.climate_.heating_.size(); group_it++)
	{
		can_messages_.at(cms_self_.climate_.heating_.at(group_it).message_instance).
		msg_data[cms_self_.climate_.heating_.at(group_it).frame_position]
		= (unsigned char)cms_self_.climate_.heating_.at(group_it).temperature;
	}

	//	Fans
	for(unsigned int group_it = 0; group_it < cms_self_.fans_.fansgroups_.size(); group_it++)
	{
		for(unsigned int fans_it = 0; fans_it < cms_self_.fans_.fansgroups_.at(group_it).fans.size(); fans_it++)
		{
			if(cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).type != Fans::BACKGROUND)
				continue;

			can_messages_.at(cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance).
			msg_data[cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).frame_position] =
			(unsigned char)cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).level;
		}
	}
	//	Settings resets do not have initial values

	//	Now do slaves
	for(unsigned int slave_it = 0; slave_it < cms_slaves_.size(); slave_it++)
	{
		//	Lights
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).lighting_.lightgroups_.size(); group_it++)
		{
			for(unsigned int light_it = 0; light_it < cms_slaves_.at(slave_it).lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
			{
				can_messages_.at(cms_slaves_.at(slave_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance).
				msg_data[cms_slaves_.at(slave_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).frame_position] =
				(unsigned char)cms_slaves_.at(slave_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).night_value; // ##INITIAL DAY NIGHT MODE
			}
		}
		//	Blinds do not have initial values

		//	Climate
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).climate_.heating_.size(); group_it++)
		{
			can_messages_.at(cms_slaves_.at(slave_it).climate_.heating_.at(group_it).message_instance).
			msg_data[cms_slaves_.at(slave_it).climate_.heating_.at(group_it).frame_position]
			= (unsigned char)cms_slaves_.at(slave_it).climate_.heating_.at(group_it).temperature;
		}

		//	Fans
		for(unsigned int group_it = 0; group_it < cms_slaves_.at(slave_it).fans_.fansgroups_.size(); group_it++)
		{
			for(unsigned int fans_it = 0; fans_it < cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.size(); fans_it++)
			{
				if(cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).type != Fans::BACKGROUND)
					continue;
				can_messages_.at(cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance).
				msg_data[cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).frame_position] =
				(unsigned char)cms_slaves_.at(slave_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).level;
			}
		}
		//	Settings resets do not have initial values
	}


	return max_instance;
}

int CmsData::LoadMmsCommands()
{
	char buffer[128];
	int command_len;

	if((command_len = sprintf(buffer, "%s Mode", cms_self_.mmshash_name.c_str())) > 0)
	{
		string temp(buffer, command_len);
		cms_self_.climate_.aircon_.mode_in_hash = CalculateMmsHash(temp);
	}

	if((command_len = sprintf(buffer, "%s Set Mode", cms_self_.mmshash_name.c_str())) > 0)
	{
		string temp(buffer, command_len);
		cms_self_.climate_.aircon_.mode_out_hash = CalculateMmsHash(temp);
	}

	if((command_len = sprintf(buffer, "%s SetPoint Temperature", cms_self_.mmshash_name.c_str())) > 0)
	{
		string temp(buffer, command_len);
		cms_self_.climate_.aircon_.temperature_in_hash = CalculateMmsHash(temp);
	}

	if((command_len = sprintf(buffer, "%s Set Temperature", cms_self_.mmshash_name.c_str())) > 0)
	{
		string temp(buffer, command_len);
		cms_self_.climate_.aircon_.temperature_out_hash = CalculateMmsHash(temp);
	}

	if((command_len = sprintf(buffer, "%s Fan Speed", cms_self_.mmshash_name.c_str())) > 0)
	{
		string temp(buffer, command_len);
		cms_self_.climate_.aircon_.fan_speed_in_hash = CalculateMmsHash(temp);
	}

	if((command_len = sprintf(buffer, "%s Set Fan Speed", cms_self_.mmshash_name.c_str())) > 0)
	{
		string temp(buffer, command_len);
		cms_self_.climate_.aircon_.fan_speed_out_hash = CalculateMmsHash(temp);
	}

	if((command_len = sprintf(buffer, "%s Ambient Temperature", cms_self_.mmshash_name.c_str())) > 0)
	{
		string temp(buffer, command_len);
		cms_self_.climate_.aircon_.ambient_temperature_in_hash = CalculateMmsHash(temp);
	}

	return 0;
}
//========================================================================================
//		State change methods
//========================================================================================
//	Lights functions
int CmsData::IncreaseLight(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index, const bool send_messages)
{
	int light_intensity;
	unsigned char msg[8];

	change_flag_ = true;

	if (cms_id == self_id_)
	{
		if(cms_self_.lighting_.lightgroups_.at(group_index).profile != Lighting::OFF)
		{
			//	Function call to lighting class to increase the value (range checks are done in there)

			if(send_messages)
			{

				//	First send message to the CAN server
				//	Mount the message
				msg[0] = (unsigned char)self_id_;				//	From
				msg[1] = (unsigned char)can_id_;				//	To
				msg[2] = LIGHTING;				//	What: Light
				msg[3] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).id;										//	Group id
				msg[4] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).id;				//	Component id
				msg[5] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).frame_position;	//	CAN frame position
				msg[7] = (unsigned char)cms_self_.lighting_.IncreaseLight(group_index, light_index);		//	Value

				if (SendCanMessage(msg) == -1)
					return -1;

				//	Then we tell the master we've changed our light
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)master_id_;					//	To
				msg[2] = LIGHTING;						//	What
				msg[3] = (unsigned char)group_index;				//	Group Index
				msg[4] = (unsigned char)light_index;				//	Light Index
				msg[5] = (unsigned char)0xFF;							//	Cms ID
				msg[6] = (unsigned char)0x00;							//	N/A
				msg[7] = (unsigned char)0x01;							//	Up

				if (SendControlMessage(msg) == -1)
					return -1;
			}
			else
			{
				cms_self_.lighting_.IncreaseLight(group_index, light_index);
			}
		}
	}
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		//	If it got here, cms_index is the vector index of the slave we want to deal with

		if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::OFF)
		{
			if(send_messages)
			{
				//	First send message to the CAN server
				//	Mount the message
				msg[0] = (unsigned char)self_id_;				//	From
				msg[1] = (unsigned char)can_id_;				//	To
				msg[2] = LIGHTING;				//	What: Light
				msg[3] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).id;										//	Group id
				msg[4] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_index).id;				//	Component id
				msg[5] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_index).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_index).frame_position;	//	CAN frame position
				msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.IncreaseLight(group_index, light_index);		//	Value

				if (SendCanMessage(msg) == -1)
					return -1;

				//	Then we tell the slave we've messed with its light
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)cms_id;					//	To
				msg[2] = LIGHTING;						//	What
				msg[3] = (char)group_index;				//	Group Index
				msg[4] = (char)light_index;				//	Light Index
				msg[5] = (unsigned char)0xFF;							//	Cms ID
				msg[6] = (unsigned char)0x00;							//	N/A
				msg[7] = (unsigned char)0x01;							//	Up

				if (SendControlMessage(msg) == -1)
					return -1;
			}
			else
			{
				//	Function call to lighting class to increase the value (range checks are done in there)
				cms_slaves_.at(cms_index).lighting_.IncreaseLight(group_index, light_index);
			}
		}
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::DecreaseLight(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index, const bool send_messages)
{
	int light_intensity;
	unsigned char msg[8];

	change_flag_ = true;

	if (cms_id == self_id_)
	{
		if(cms_self_.lighting_.lightgroups_.at(group_index).profile != Lighting::OFF)
		{
			if(send_messages)
			{
				//	First send message to the CAN server
				msg[0] = self_id_;				//	From
				msg[1] = can_id_;				//	To
				msg[2] = LIGHTING;				//	What: Light
				msg[3] = cms_self_.lighting_.lightgroups_.at(group_index).id;										//	Group id
				msg[4] = cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).id;				//	Component id
				msg[5] = cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).message_instance;	//	CAN message instance
				msg[6] = cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).frame_position;	//	CAN frame position
				msg[7] = (unsigned char)cms_self_.lighting_.DecreaseLight(group_index, light_index);		//	Value

				if (SendCanMessage(msg) == -1)
					return -1;

				//	Then we tell the master we've changed our light
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)master_id_;					//	To
				msg[2] = LIGHTING;						//	What
				msg[3] = (char)group_index;				//	Group Index
				msg[4] = (char)light_index;				//	Light Index
				msg[5] = (unsigned char)0xFF;							//	Cms ID
				msg[6] = (unsigned char)0x00;							//	N/A
				msg[7] = (unsigned char)0x00;							//	Down

				if (SendControlMessage(msg) == -1)
					return -1;
			}
			else
			{
				cms_self_.lighting_.DecreaseLight(group_index, light_index);
			}
		}
	}
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		//	If it got here, cms_index is the vector index of the slave we want to deal with
		//	Function call to lighting class to increase the value (range checks are done in there)
		if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::OFF)
		{
			if(send_messages)
			{
				//	Mount the message
				msg[0] = (unsigned char)self_id_;				//	From
				msg[1] = (unsigned char)can_id_;				//	To
				msg[2] = LIGHTING;				//	What: Light
				msg[3] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).id;										//	Group id
				msg[4] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_index).id;				//	Component id
				msg[5] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_index).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_index).frame_position;	//	CAN frame position
				msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.DecreaseLight(group_index, light_index);		//	Value

				if (SendCanMessage(msg) == -1)
					return -1;

				//	Then we tell the slave we've changed its light
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)cms_id;					//	To
				msg[2] = LIGHTING;						//	What
				msg[3] = (unsigned char)group_index;				//	Group Index
				msg[4] = (unsigned char)light_index;				//	Light Index
				msg[5] = (unsigned char)0xFF;							//	Cms ID
				msg[6] = (unsigned char)0x00;							//	N/A
				msg[7] = (unsigned char)0x00;							//	Down

				if (SendControlMessage(msg) == -1)
					return -1;
			}
			else
			{
				cms_slaves_.at(cms_index).lighting_.DecreaseLight(group_index, light_index);
			}
		}
	}
	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::SetProfile(const unsigned char cms_id, const unsigned char group_index, const Lighting::Profile new_value, const bool send_messages)
{
	//	Will have to iterate through every light in the group and send it's value
	unsigned char msg[8];
	if (cms_id == self_id_)
	{
		//	If it's changing to manual, maintain the last active screen brightness
		if(group_index == 0)
		{
			cms_self_.settings_.screensettings_.screen_brightness_manual = cms_self_.settings_.GetScreenBrightness(cms_self_.lighting_.lightgroups_.at(group_index).profile);
			cms_self_.settings_.screensettings_.screen_brightness_off = cms_self_.settings_.GetScreenBrightness(cms_self_.lighting_.lightgroups_.at(group_index).profile);
		}

		if(send_messages)
		{
			if(all_off_)
			{
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)can_id_;	//	To
				msg[2] = SETTINGS;						//	What
				msg[3] = (unsigned char)0x04;							//	N/A
				msg[4] = (unsigned char)0x00;							//
				msg[5] = (unsigned char)0x03;							//	N/A
				msg[6] = (unsigned char)0x03;							//	Reset
				msg[7] = (unsigned char)0x00;

				if (SendCanMessage(msg) == -1)
					return -1;

				all_off_ = false;
			}
			//	Mount the fixed part
			msg[0] = (unsigned char)self_id_;				//	From
			msg[1] = (unsigned char)can_id_;				//	To
			msg[2] = (unsigned char)LIGHTING;				//	What: Light

			cms_self_.lighting_.SetProfile(group_index, new_value);
			//	First we send all the CAN messages
			for (unsigned int light_it = 0; light_it < cms_self_.lighting_.lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Mount the rest
				msg[3] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).id;									//	Group id
				msg[4] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).id;				//	Component id
				msg[5] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).frame_position;	//	CAN frame position
				msg[7] = (unsigned char)cms_self_.lighting_.GetLightValue(group_index, light_it);

				if (SendCanMessage(msg) == -1)
					return -1;
			}

			//	Then we send the day/night bit
			SendNightBit(cms_self_.lighting_.nightbit_message_instance_, cms_self_.lighting_.nightbit_frame_position_,
			(cms_self_.lighting_.GetProfile(0) == Lighting::NIGHT));

			if(!cms_self_.is_master)
			{
				//	Then we tell the master we changed our profile
				//	Mount the message
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)master_id_;					//	To
				msg[2] = LIGHTING_PROFILE;				//	What
				msg[3] = (unsigned char)group_index;					//	Group Index
				msg[4] = (unsigned char)0xFF;							//	N/A
				msg[5] = (unsigned char)cms_id;						//	Cms ID
				msg[6] = (unsigned char)0xFF;							//	N/A
				msg[7] = (unsigned char)new_value;				//	profile

				if (SendControlMessage(msg) == -1)
					return -1;
			}
		}
		else
		{
			 cms_self_.lighting_.SetProfile(group_index, new_value);
		}

		//	In case it has a related fan group, activate/deactivate boost ventilation
		if(cms_self_.lighting_.lightgroups_.at(group_index).related_fangroup != 0)
		{
			if(cms_self_.lighting_.lightgroups_.at(group_index).profile == Lighting::OFF)
			{
				StartBoostFanTimer(cms_self_.lighting_.lightgroups_.at(group_index).related_fangroup);
			}
			else if(cms_self_.lighting_.lightgroups_.at(group_index).profile == Lighting::DAY ||
					cms_self_.lighting_.lightgroups_.at(group_index).profile == Lighting::EVE ||
					cms_self_.lighting_.lightgroups_.at(group_index).profile == Lighting::SHOWER)
			{
				BoostFanLevel(cms_self_.lighting_.lightgroups_.at(group_index).related_fangroup, Fans::DAYBOOST);
			}
			else if	(cms_self_.lighting_.lightgroups_.at(group_index).profile == Lighting::NIGHT)
			{
				BoostFanLevel(cms_self_.lighting_.lightgroups_.at(group_index).related_fangroup, Fans::NIGHTBOOST);
			}
		}

	}
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		//	If it got here, cms_index is the vector index of the slave we want to deal with

		if(group_index == 0)
		{
			//	If it's changing to manual, maintain the last active screen brightness
			if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL
				&& new_value == Lighting::MANUAL)
			{
				cms_slaves_.at(cms_index).settings_.screensettings_.screen_brightness_manual = cms_slaves_.at(cms_index).settings_.GetScreenBrightness(cms_self_.lighting_.lightgroups_.at(group_index).profile);
			}
			//	If it's changing to off, maintain the last active screen brightness
			else if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::OFF
				&& new_value == Lighting::OFF)
			{
				cms_slaves_.at(cms_index).settings_.screensettings_.screen_brightness_off = cms_slaves_.at(cms_index).settings_.GetScreenBrightness(cms_self_.lighting_.lightgroups_.at(group_index).profile);
			}
		}

		if(send_messages)
		{
			if(all_off_)
			{
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)can_id_;	//	To
				msg[2] = SETTINGS;						//	What
				msg[3] = (unsigned char)0x04;							//	N/A
				msg[4] = (unsigned char)0x00;							//
				msg[5] = (unsigned char)0x03;							//	N/A
				msg[6] = (unsigned char)0x03;							//	Reset
				msg[7] = (unsigned char)0x00;

				if (SendCanMessage(msg) == -1)
					return -1;

				all_off_ = false;
			}

			//	Mount the fixed part
			msg[0] = (unsigned char)self_id_;				//	From
			msg[1] = (unsigned char)can_id_;				//	To
			msg[2] = LIGHTING;				//	What: Light

			cms_slaves_.at(cms_index).lighting_.SetProfile(group_index, new_value);
			//	First we send all the CAN messages
			for (unsigned int light_it = 0; light_it < cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Mount the rest
				msg[3] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).id;									//	Group id
				msg[4] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).id;				//	Component id
				msg[5] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).frame_position;	//	CAN frame position
				msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.GetLightValue(group_index, light_it);

				if (SendCanMessage(msg) == -1)
					return -1;
			}

			//	Then we send the day/night bit
			SendNightBit(cms_slaves_.at(cms_index).lighting_.nightbit_message_instance_, cms_slaves_.at(cms_index).lighting_.nightbit_frame_position_,
			(cms_slaves_.at(cms_index).lighting_.GetProfile(0) == Lighting::NIGHT));


			//	Then we tell the slave we changed our profile
			//	Mount the message
			msg[0] = (unsigned char)self_id_;						//	From
			msg[1] = (unsigned char)cms_id;					//	To
			msg[2] = LIGHTING_PROFILE;				//	What
			msg[3] = (unsigned char)group_index;					//	Group Index
			msg[4] = (unsigned char)0xFF;							//	N/A
			msg[5] = (unsigned char)cms_id;						//	Cms ID
			msg[6] = (unsigned char)0xFF;							//	N/A
			msg[7] = (unsigned char)new_value;				//	profile

			if (SendControlMessage(msg) == -1)
				return -1;

		}
		else
		{
			cms_slaves_.at(cms_index).lighting_.SetProfile(group_index, new_value);
		}
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::IncreaseGroupLight(const unsigned char cms_id, const unsigned char group_index, const bool send_messages)
{
	//	Will have to iterate through every light in the group and send it's value
	unsigned char msg[8];
	if (cms_id == self_id_)
	{
		if(send_messages)
		{
			//	Mount the fixed part
			msg[0] = (unsigned char)self_id_;				//	From
			msg[1] = (unsigned char)can_id_;				//	To
			msg[2] = LIGHTING;				//	What: Light

			if(cms_self_.lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
			{
				cms_self_.settings_.screensettings_.screen_brightness_manual =
				cms_self_.settings_.GetScreenBrightness(cms_self_.lighting_.lightgroups_.at(group_index).profile);
				cms_self_.lighting_.SetProfile(group_index, Lighting::MANUAL);
			}

			cms_self_.lighting_.IncreaseGroupLight(group_index);
			//	First we send all the CAN messages
			for (unsigned int light_it = 0; light_it < cms_self_.lighting_.lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Mount the rest
				msg[3] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).id;									//	Group id
				msg[4] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).id;				//	Component id
				msg[5] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).frame_position;	//	CAN frame position

				if (cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value > 100)
					msg[7] = (unsigned char)100;
				else if (cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value < 0)
					msg[7] = (unsigned char)0;
				else
					msg[7] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value;


				//	Send CAN
				if (SendCanMessage(msg) == -1)
					return -1;

			}
		}
		else
		{
			if(cms_self_.lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
				cms_self_.lighting_.SetProfile(group_index, Lighting::MANUAL);
			cms_self_.lighting_.IncreaseGroupLight(group_index);
		}

	}
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		//	If it got here, cms_index is the vector index of the slave we want to deal with

		if(send_messages)
		{
			//	Mount fixed part
			msg[0] = (unsigned char)self_id_;				//	From
			msg[1] = (unsigned char)can_id_;				//	To
			msg[2] = LIGHTING;				//	What: Light
			//	If it was not already, change profile to manual
			if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
				cms_slaves_.at(cms_index).lighting_.SetProfile(group_index, Lighting::MANUAL);

			cms_slaves_.at(cms_index).lighting_.IncreaseGroupLight(group_index);
			//	First we send all the CAN messages
			for (unsigned int light_it = 0; light_it < cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Mount the rest
				msg[3] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).id;									//	Group id
				msg[4] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).id;				//	Component id
				msg[5] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).frame_position;	//	CAN frame position

				if (cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value > 100)
					msg[7] = (unsigned char)100;
				else if (cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value < 0)
					msg[7] = (unsigned char)0;
				else
					msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value;

				//	Send CAN
				if (SendCanMessage(msg) == -1)
					return -1;

			}

			//	Then we tell the slave to update values
			//	Mount the message
			msg[0] = (unsigned char)self_id_;						//	From
			msg[1] = (unsigned char)cms_id;					//	To
			msg[2] = LIGHTING;				//	What
			msg[3] = (unsigned char)group_index;					//	Group Index
			msg[4] = (unsigned char)0xFF;							//	N/A
			msg[5] = (unsigned char)0xFF;						//	Cms ID
			msg[6] = (unsigned char)0x00;							//	N/A
			msg[7] = (unsigned char)0x11;				//	profile

			if (SendControlMessage(msg) == -1)
				return -1;
		}
		else
		{
			if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
				cms_slaves_.at(cms_index).lighting_.SetProfile(group_index, Lighting::MANUAL);
			cms_slaves_.at(cms_index).lighting_.IncreaseGroupLight(group_index);
		}
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::DecreaseGroupLight(const unsigned char cms_id, const unsigned char group_index, const bool send_messages)
{
	unsigned char msg[8];
	if (cms_id == self_id_)
	{
		if(send_messages)
		{
			//	Mount the fixed part
			msg[0] = (unsigned char)self_id_;				//	From
			msg[1] = (unsigned char)can_id_;				//	To
			msg[2] = LIGHTING;				//	What: Light

			if(cms_self_.lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
			{
				cms_self_.lighting_.SetProfile(group_index, Lighting::MANUAL);
			}

			cms_self_.lighting_.DecreaseGroupLight(group_index);
			//	First we send all the CAN messages
			for (unsigned int light_it = 0; light_it < cms_self_.lighting_.lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Mount the rest
				msg[3] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).id;									//	Group id
				msg[4] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).id;				//	Component id
				msg[5] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).frame_position;	//	CAN frame position

				if (cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value > 100)
					msg[7] = (unsigned char)100;
				else if (cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value < 0)
					msg[7] = (unsigned char)0;
				else
					msg[7] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value;


				//	Send CAN
				if (SendCanMessage(msg) == -1)
					return -1;

			}
		}
		else
		{
			if(cms_self_.lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
				cms_self_.lighting_.SetProfile(group_index, Lighting::MANUAL);

			cms_self_.lighting_.DecreaseGroupLight(group_index);
		}

	}
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		//	If it got here, cms_index is the vector index of the slave we want to deal with

		if(send_messages)
		{
			//	Mount fixed part
			msg[0] = (unsigned char)self_id_;				//	From
			msg[1] = (unsigned char)can_id_;				//	To
			msg[2] = LIGHTING;				//	What: Light

			if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
				cms_slaves_.at(cms_index).lighting_.SetProfile(group_index, Lighting::MANUAL);

			cms_slaves_.at(cms_index).lighting_.DecreaseGroupLight(group_index);
			//	First we send all the CAN messages
			for (unsigned int light_it = 0; light_it < cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Mount the rest
				msg[3] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).id;									//	Group id
				msg[4] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).id;				//	Component id
				msg[5] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).frame_position;	//	CAN frame position

				if (cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value > 100)
					msg[7] = (unsigned char)100;
				else if (cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value < 0)
					msg[7] = (unsigned char)0;
				else
					msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value;

				//	Send CAN
				if (SendCanMessage(msg) == -1)
					return -1;

			}

			//	Then we tell the slave to update values
			//	Mount the message
			msg[0] = (unsigned char)self_id_;						//	From
			msg[1] = (unsigned char)cms_id;					//	To
			msg[2] = LIGHTING;				//	What
			msg[3] = (unsigned char)group_index;					//	Group Index
			msg[4] = (unsigned char)0xFF;							//	N/A
			msg[5] = (unsigned char)0xFF;						//	Cms ID
			msg[6] = (unsigned char)0x00;							//	N/A
			msg[7] = (unsigned char)0x11;				//	profile

			if (SendControlMessage(msg) == -1)
				return -1;
		}
		else
		{
			if(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile != Lighting::MANUAL)
				cms_slaves_.at(cms_index).lighting_.SetProfile(group_index, Lighting::MANUAL);
			cms_slaves_.at(cms_index).lighting_.DecreaseGroupLight(group_index);
		}
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::StartIncreaseGroupTimer(const unsigned char cms_id, const unsigned char group_index)
{
	//	Set up the timer for screen Timeout
	int pulse_info = ((unsigned char)cms_id << 16) | ((unsigned char)group_index << 8) |  (unsigned char)0x01;

	if (cms_id == self_id_)
	{
		timer_delete(cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timerid);
		SIGEV_PULSE_INIT(&cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_event,
		control_coid_, SIGEV_PULSE_PRIO_INHERIT, LIGHTING, pulse_info);

		if(timer_create (CLOCK_REALTIME, &cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_event,
		&cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timerid) == -1)
		{
			printf("Could not create timer!");
			return -1;
		}
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_sec = 0;
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_nsec = 200000000;
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_sec = 0;
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_nsec = 200000000;

		timer_settime(cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timerid,
		0, &cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer, NULL);
	}
	else
	{
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		timer_delete(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timerid);
		SIGEV_PULSE_INIT(&cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_event,
		control_coid_, SIGEV_PULSE_PRIO_INHERIT, LIGHTING, pulse_info);

		if(timer_create (CLOCK_REALTIME, &cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_event,
		&cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timerid) == -1)
		{
			printf("Could not create timer!");
			return -1;
		}
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_sec = 0;
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_nsec = 200000000;
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_sec = 0;
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_nsec = 200000000;

		timer_settime(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timerid,
		0, &cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer, NULL);
	}

	return 0;
}

int CmsData::StartDecreaseGroupTimer(const unsigned char cms_id, const unsigned char group_index)
{
	//	Set up the timer for screen Timeout
	int pulse_info = ((unsigned char)cms_id << 16) | ((unsigned char)group_index << 8) |  (unsigned char)0x00;

	if (cms_id == self_id_)
	{
		timer_delete(cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timerid);
		SIGEV_PULSE_INIT(&cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_event,
		control_coid_, SIGEV_PULSE_PRIO_INHERIT, LIGHTING, pulse_info);

		if(timer_create (CLOCK_REALTIME, &cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_event,
		&cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timerid) == -1)
		{
			printf("Could not create timer!");
			return -1;
		}
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_sec = 0;
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_nsec = 200000000;
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_sec = 0;
		cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_nsec = 200000000;

		timer_settime(cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timerid,
		0, &cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timer, NULL);
	}
	else
	{
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		timer_delete(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timerid);
		SIGEV_PULSE_INIT(&cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_event,
		control_coid_, SIGEV_PULSE_PRIO_INHERIT, LIGHTING, pulse_info);

		if(timer_create (CLOCK_REALTIME, &cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_event,
		&cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timerid) == -1)
		{
			printf("Could not create timer!");
			return -1;
		}
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_sec = 0;
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_value.tv_nsec = 200000000;
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_sec = 0;
		cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer.it_interval.tv_nsec = 200000000;

		timer_settime(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timerid,
		0, &cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timer, NULL);
	}

	return 0;
}

int CmsData::StopGroupTimer(const unsigned char cms_id, const unsigned char group_index)
{
	if (cms_id == self_id_)
	{
		timer_delete(cms_self_.lighting_.lightgroups_.at(group_index).lightgroup_timerid);
	}
	else
	{
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}
		timer_delete(cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).lightgroup_timerid);
	}

	return 0;
}

//	Blinds functions
int CmsData::OpenBlinds(const unsigned char group_index, const unsigned char blinds_index)
{
	unsigned char msg[8];
	//	If it's a group, iterate through all blinds
	if (cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).is_group)
	{
		//	Mount the fixed part
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds

		for (unsigned int blinds_it = 0; blinds_it < cms_self_.blinds_.blindsgroups_.at(group_index).blinds.size(); blinds_it++)
		{
			//	Mount the rest
			msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;										//	Group id
			msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).id;					//	Component id
			msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).open_message_instance;//	CAN message instance
			msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).open_frame_position;	//	CAN frame position
			msg[7] = (unsigned char)0x01;			//	Value

			//	Send one by one
			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}
	else
	{
		//	Mount the message
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds
		msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;											//	Group id
		msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).id;					//	Component id
		msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).open_message_instance;	//	CAN message instance
		msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).open_frame_position;	//	CAN frame position
		msg[7] = (unsigned char)0x01;			//	Value

		//	Send one by one
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::OpenBlindsStop(const unsigned char group_index, const unsigned char blinds_index)
{
	unsigned char msg[8];
	//	If it's a group, iterate through all blinds
	if (cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).is_group)
	{
		//	Mount the fixed part
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds

		for (unsigned int blinds_it = 0; blinds_it < cms_self_.blinds_.blindsgroups_.at(group_index).blinds.size(); blinds_it++)
		{
			//	Mount the rest
			msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;										//	Group id
			msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).id;					//	Component id
			msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).open_message_instance;//	CAN message instance
			msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).open_frame_position;	//	CAN frame position
			msg[7] = (unsigned char)0x00;			//	Value

			//	Send one by one
			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}
	else
	{
		//	Mount the message
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds
		msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;											//	Group id
		msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).id;					//	Component id
		msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).open_message_instance;	//	CAN message instance
		msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).open_frame_position;	//	CAN frame position
		msg[7] = (unsigned char)0x00;			//	Value

		//	Send one by one
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::CloseBlinds(const unsigned char group_index, const unsigned char blinds_index)
{
	unsigned char msg[8];
	//	If it's a group, iterate through all blinds
	if (cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).is_group)
	{
		//	Mount the fixed part
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds

		for (unsigned int blinds_it = 0; blinds_it < cms_self_.blinds_.blindsgroups_.at(group_index).blinds.size(); blinds_it++)
		{

			//	Mount the rest
			msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;										//	Group id
			msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).id;					//	Component id
			msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).close_message_instance;//	CAN message instance
			msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).close_frame_position;	//	CAN frame position
			msg[7] = (unsigned char)0x01;			//	Value

			//	Send one by one
			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}
	else
	{
		//	Mount the message
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds
		msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;											//	Group id
		msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).id;					//	Component id
		msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).close_message_instance;	//	CAN message instance
		msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).close_frame_position;	//	CAN frame position
		msg[7] = (unsigned char)0x01;			//	Value

		//	Send one by one
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::CloseBlindsStop(const unsigned char group_index, const unsigned char blinds_index)
{
	unsigned char msg[8];
	//	If it's a group, iterate through all blinds
	if (cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).is_group)
	{
		//	Mount the fixed part
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds

		for (unsigned int blinds_it = 0; blinds_it < cms_self_.blinds_.blindsgroups_.at(group_index).blinds.size(); blinds_it++)
		{
			//	Mount the rest
			msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;										//	Group id
			msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).id;					//	Component id
			msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).close_message_instance;//	CAN message instance
			msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_it).close_frame_position;	//	CAN frame position
			msg[7] = (unsigned char)0x00;			//	Value

			//	Send one by one
			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}
	else
	{
		//	Mount the message
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = BLINDS;		//	What: Blinds
		msg[3] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).id;											//	Group id
		msg[4] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).id;					//	Component id
		msg[5] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).close_message_instance;	//	CAN message instance
		msg[6] = (unsigned char)cms_self_.blinds_.blindsgroups_.at(group_index).blinds.at(blinds_index).close_frame_position;	//	CAN frame position
		msg[7] = (unsigned char)0x00;			//	Value

		//	Send one by one
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}


//	Climate functions
int CmsData::IncreaseAirconTemperature()
{
	if(cms_self_.climate_.GetAirconPower())
	{
		change_flag_ = true;

		unsigned char msg[8];

		timer_delete(cms_self_.climate_.aircon_.showsetpoint_timerid);
		timer_settime(cms_self_.climate_.aircon_.showsetpoint_timerid, 0, &cms_self_.climate_.aircon_.showsetpoint_timer, NULL);
		cms_self_.climate_.aircon_.showing_setpoint_ = true;
		unsigned char temperature = cms_self_.climate_.IncreaseAirconTemperature();

		//	Mount the message
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)mms_id_;		//	To
		msg[2] = AIRCON;		//	What: Aircon
		msg[3] = (unsigned char)0xFF;			//	Not applicable
		msg[4] = (unsigned char)0x00;			//	Temperature
		msg[5] = (unsigned char)0xFF;	//	MMS message instance
		msg[6] = (unsigned char)0xFF;		//	MMS frame position
		msg[7] = temperature;	//	Value

		if (SendControlMessage(msg) == -1)
			return -1;
	}

	return 0;
}

int CmsData::DecreaseAirconTemperature()
{
	if(cms_self_.climate_.GetAirconPower())
	{
		change_flag_ = true;

		unsigned char msg[8];

		timer_delete(cms_self_.climate_.aircon_.showsetpoint_timerid);
		timer_settime(cms_self_.climate_.aircon_.showsetpoint_timerid, 0, &cms_self_.climate_.aircon_.showsetpoint_timer, NULL);
		cms_self_.climate_.aircon_.showing_setpoint_ = true;
		unsigned char temperature = cms_self_.climate_.DecreaseAirconTemperature();

		//	Mount the message
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)mms_id_;		//	To
		msg[2] = AIRCON;		//	What: Aircon
		msg[3] = (unsigned char)0xFF;			//	ot applicable
		msg[4] = (unsigned char)0x00;			//	Temperature
		msg[5] = (unsigned char)0xFF;	//	MMS message instance
		msg[6] = (unsigned char)0xFF;		//	MMS frame position
		msg[7] = temperature;	//	Value

		if (SendControlMessage(msg) == -1)
			return -1;
	}

	return 0;
}

int CmsData::ChangeAirconFanSpeed()
{
	if(cms_self_.climate_.GetAirconPower())
	{
		if(cms_self_.climate_.GetAirconMode() == Climate::HEATINGONLY || cms_self_.climate_.GetAirconMode() == Climate::COOLONLY)
		{
			unsigned char msg[8];
			unsigned char fan_speed = cms_self_.climate_.ChangeAirconFanSpeed();

			//	Mount the message
			msg[0] = (unsigned char)self_id_;		//	From
			msg[1] = (unsigned char)mms_id_;		//	To
			msg[2] = AIRCON;		//	What: Aircon
			msg[3] = (unsigned char)0xFF;			//	ot applicable
			msg[4] = (unsigned char)0x01;			//	Fan Speed
			msg[5] = (unsigned char)0xFF;	//	MMS message instance
			msg[6] = (unsigned char)0xFF;		//	MMS frame position
			msg[7] = fan_speed;	//	Value

			if (SendControlMessage(msg) == -1)
				return -1;
		}
	}

	return 0;
}

int CmsData::SwitchAirconPower()
{
	unsigned char msg[8];
	unsigned char power = cms_self_.climate_.SwitchAirconPower();

	//	Mount the message
	msg[0] = (unsigned char)self_id_;		//	From
	msg[1] = (unsigned char)mms_id_;		//	To
	msg[2] = AIRCON;		//	What: Aircon
	msg[3] = (unsigned char)0xFF;			//	ot applicable
	msg[4] = (unsigned char)0x02;			//	Power
	msg[5] = (unsigned char)0xFF;	//	MMS message instance
	msg[6] = (unsigned char)0xFF;		//	MMS frame position
	msg[7] = power;		//	Value

	if (SendControlMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::ChangeAirconMode()
{
	unsigned char msg[8];
	unsigned char mode = cms_self_.climate_.ChangeAirconMode();

	//	Mount the message
	msg[0] = (unsigned char)self_id_;		//	From
	msg[1] = (unsigned char)mms_id_;		//	To
	msg[2] = AIRCON;		//	What: Aircon
	msg[3] = (unsigned char)0xFF;			//	ot applicable
	msg[4] = (unsigned char)0x03;			//	Mode
	msg[5] = (unsigned char)0xFF;	//	MMS message instance
	msg[6] = (unsigned char)0xFF;		//	MMS frame position
	msg[7] = mode;			//	Value

	if (SendControlMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::IncreaseHeatingTemperature(const unsigned char group_index)
{
	change_flag_ = true;

	unsigned char msg[8];
	unsigned char temperature = cms_self_.climate_.IncreaseHeatingTemperature(group_index);

	//	Mount the message
	msg[0] = (unsigned char)self_id_;		//	From
	msg[1] = (unsigned char)can_id_;		//	To
	msg[2] = HEATING;		//	What: Heating
	msg[3] = (unsigned char)0xFF;			//	Not applicable
	msg[4] = (unsigned char)0x00;		//	Temperature
	msg[5] = (unsigned char)cms_self_.climate_.heating_.at(group_index).message_instance;	//	MMS message instance
	msg[6] = (unsigned char)cms_self_.climate_.heating_.at(group_index).frame_position;		//	MMS frame position
	msg[7] = temperature;			//	Value

	if (SendCanMessage(msg) == -1)
		return -1;

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::DecreaseHeatingTemperature(const unsigned char group_index)
{
	change_flag_ = true;

	unsigned char msg[8];
	unsigned char temperature = cms_self_.climate_.DecreaseHeatingTemperature(group_index);

	//	Mount the message
	msg[0] = (unsigned char)self_id_;		//	From
	msg[1] = (unsigned char)can_id_;		//	To
	msg[2] = HEATING;		//	What: Heating
	msg[3] = (unsigned char)0xFF;			//	Not applicable
	msg[4] = (unsigned char)0x00;			//	Not applicable
	msg[5] = (unsigned char)cms_self_.climate_.heating_.at(group_index).message_instance;	//	MMS message instance
	msg[6] = (unsigned char)cms_self_.climate_.heating_.at(group_index).frame_position;		//	MMS frame position
	msg[7] = temperature;			//	Value

	if (SendCanMessage(msg) == -1)
		return -1;

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::SwitchHeatingPower(const unsigned char group_index, const unsigned char switch_index)
{
	unsigned char msg[8];

	if(cms_self_.climate_.master_heatingenable_)
	{
		unsigned char power = cms_self_.climate_.SwitchHeatingPower(group_index, switch_index);

		//	Mount the message
		msg[0] = (unsigned char)self_id_;		//	From
		msg[1] = (unsigned char)can_id_;		//	To
		msg[2] = HEATING;		//	What: Heating
		msg[3] = (unsigned char)cms_self_.climate_.heating_.at(group_index).power_switches.at(switch_index).group;
		msg[4] = 0x01;
		msg[5] = (unsigned char)cms_self_.climate_.heating_.at(group_index).power_switches.at(switch_index).message_instance;		//	MMS message instance
		msg[6] = (unsigned char)cms_self_.climate_.heating_.at(group_index).power_switches.at(switch_index).frame_position;		//	MMS frame position
		msg[7] = power;			//	Value

		if (SendCanMessage(msg) == -1)
			return -1;

		//	Send queued messages
		msg[1] = (unsigned char)can_id_;
		msg[2] = SEND_QUEUE;
		msg[5] = (unsigned char)0x00;
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	return 0;
}


//	Fans functions
int CmsData::IncreaseFanLevel(const unsigned char group_index, const unsigned char fan_index)
{
	change_flag_ = true;

	unsigned char msg[8];

	//	Mount the message
	msg[0] = (unsigned char)self_id_;				//	From
	msg[1] = (unsigned char)can_id_;				//	To
	msg[2] = FANS;					//	What: Fans
	msg[3] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).id;										//	Group id
	msg[4] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).id;						//	Component id
	msg[5] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).message_instance;		//	CAN message instance
	msg[6] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).frame_position;			//	CAN frame position

	//	Function call to fans class to increase the value (range checks are done in there)
	cms_self_.fans_.IncreaseFanLevel(group_index, fan_index);
	msg[7] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).level;					//	Value

	if(cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).type == Fans::BACKGROUND)
	{
		if (SendCanMessage(msg) == -1)
			return -1;

		//	Send queued messages
		msg[1] = (unsigned char)can_id_;
		msg[2] = SEND_QUEUE;
		msg[5] = (unsigned char)0x00;
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	return 0;
}

int CmsData::DecreaseFanLevel(const unsigned char group_index, const unsigned char fan_index)
{
	change_flag_ = true;

	unsigned char msg[8];

	//	Mount the message
	msg[0] = (unsigned char)self_id_;				//	From
	msg[1] = (unsigned char)can_id_;				//	To
	msg[2] = FANS;					//	What: Fans
	msg[3] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).id;										//	Group id
	msg[4] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).id;						//	Component id
	msg[5] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).message_instance;		//	CAN message instance
	msg[6] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).frame_position;			//	CAN frame position

	//	Function call to fans class to increase the value (range checks are done in there)
	cms_self_.fans_.DecreaseFanLevel(group_index, fan_index);
	msg[7] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).level;					//	Value

	if(cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).type == Fans::BACKGROUND)
	{
		if (SendCanMessage(msg) == -1)
			return -1;

		//	Send queued messages
		msg[1] = (unsigned char)can_id_;
		msg[2] = SEND_QUEUE;
		msg[5] = (unsigned char)0x00;
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	return 0;
}

int CmsData::BoostFanLevel(const unsigned char group_id, Fans::FanType type)
{
	unsigned char msg[8];
	unsigned int group_index = 0;
	unsigned int fan_index = 0;

	//	Find the group index in the vector for the given id
	for(group_index = 0; group_index < cms_self_.fans_.fansgroups_.size(); group_index++)
	{
		if(cms_self_.fans_.fansgroups_.at(group_index).id == group_id)
			break;
	}
	//	Find the fan index in the group for the given type
	for(fan_index = 0; fan_index < cms_self_.fans_.fansgroups_.at(group_index).fans.size(); fan_index++)
	{
		if(cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).type == type)
			break;
	}
	//	If it ran through groups and fans but did't find the right one
	if(cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).type != type)
		return -1;

	//	Mount the message
	msg[0] = (unsigned char)self_id_;				//	From
	msg[1] = (unsigned char)can_id_;				//	To
	msg[2] = FANS;					//	What: Fans
	msg[3] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).id;										//	Group id
	msg[4] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).id;						//	Component id
	msg[5] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).message_instance;		//	CAN message instance
	msg[6] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).frame_position;			//	CAN frame position

	//	Function call to fans class to increase the value (range checks are done in there)
	msg[7] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).level;					//	Value

	if (SendCanMessage(msg) == -1)
		return -1;

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::StartBoostFanTimer(const unsigned char group_id)
{
	unsigned int group_index = 0;

	//	Find the group index in the vector for the given id
	for(group_index = 0; group_index < cms_self_.fans_.fansgroups_.size(); group_index++)
	{
		if(cms_self_.fans_.fansgroups_.at(group_index).id == group_id)
			break;
	}

	//	Function call to fans class to increase the value (range checks are done in there)
 	//	(Re)Set the timer for boosting the fan
	timer_delete(cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timerid);
	SIGEV_PULSE_INIT(&cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_event,
	control_coid_, SIGEV_PULSE_PRIO_INHERIT, FANS, group_index);

	if(timer_create (CLOCK_REALTIME, &cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_event,
	&cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timerid) == -1)
	{
		printf("Could not create timer!");
		return -1;
	}
	cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timer.it_value.tv_sec = 600;
	cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timer.it_value.tv_nsec = 0;
	cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timer.it_interval.tv_sec = 0;
	cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timer.it_interval.tv_nsec = 0;

	timer_settime(cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timerid,
	0, &cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timer, NULL);

	return 0;
}

int CmsData::UnboostFanLevel(const unsigned char group_index)
{
	unsigned char msg[8];
	unsigned int fan_index = 0;

	//	Find the fan index in the group for the given type
	for(fan_index = 0; fan_index < cms_self_.fans_.fansgroups_.at(group_index).fans.size(); fan_index++)
	{
		if(cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).type == Fans::BACKGROUND)
			break;
	}
	//	If it ran through groups and fans but did't find the right one
	if(cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).type != Fans::BACKGROUND)
		return -1;

	//	Mount the message
	msg[0] = (unsigned char)self_id_;				//	From
	msg[1] = (unsigned char)can_id_;				//	To
	msg[2] = FANS;					//	What: Fans
	msg[3] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).id;										//	Group id
	msg[4] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).id;						//	Component id
	msg[5] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).message_instance;		//	CAN message instance
	msg[6] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).frame_position;			//	CAN frame position
	msg[7] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_index).fans.at(fan_index).level;					//	Value
	timer_delete(cms_self_.fans_.fansgroups_.at(group_index).fangroupboost_timerid);

	if (SendCanMessage(msg) == -1)
		return -1;

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

//	Settings functions
int CmsData::IncreaseScreenBrightness()
{
	change_flag_ = true;
	int brightness;
	//if(cms_self_.lighting_.GetProfile(0) != Lighting::OFF && cms_self_.lighting_.GetProfile(0) != Lighting::MANUAL)
	{
		cms_self_.settings_.IncreaseScreenBrightness(cms_self_.lighting_.GetProfile(0));
		brightness = (cms_self_.settings_.GetScreenBrightness(cms_self_.lighting_.GetProfile(0)))*0.8;
	}
/*	else
	{
		cms_self_.settings_.IncreaseScreenBrightness(Lighting::DAY);
		brightness = (cms_self_.settings_.GetScreenBrightness(Lighting::DAY))*0.8;
	}*/

	string shell_command = "echo brightness=" + to_string(brightness) + ">/dev/sysctl";
	system(shell_command.c_str());

	return 0;
}

int CmsData::DecreaseScreenBrightness()
{
	change_flag_ = true;

	int brightness;
	//if(cms_self_.lighting_.GetProfile(0) != Lighting::OFF && cms_self_.lighting_.GetProfile(0) != Lighting::MANUAL)
	{
		cms_self_.settings_.DecreaseScreenBrightness(cms_self_.lighting_.GetProfile(0));
		brightness = (cms_self_.settings_.GetScreenBrightness(cms_self_.lighting_.GetProfile(0)))*0.8;
	}
/*	else
	{
		cms_self_.settings_.DecreaseScreenBrightness(Lighting::DAY);
		brightness = (cms_self_.settings_.GetScreenBrightness(Lighting::DAY))*0.8;
	}*/

	string shell_command = "echo brightness=" + to_string(brightness) + ">/dev/sysctl";
	system(shell_command.c_str());

	return 0;
}

int CmsData::SetScreenBrightness()
{
	change_flag_ = true;

	//if(cms_self_.lighting_.GetProfile(0) != Lighting::OFF && cms_self_.lighting_.GetProfile(0) != Lighting::MANUAL)
	{
		int brightness = (cms_self_.settings_.GetScreenBrightness(cms_self_.lighting_.GetProfile(0)))*0.8;

		string shell_command = "echo brightness=" + to_string(brightness) + ">/dev/sysctl";
		system(shell_command.c_str());
	}

	return 0;
}

int CmsData::IncreaseScreenTimeout()
{
	change_flag_ = true;

	int timeout = cms_self_.settings_.IncreaseScreenTimeout();

	return timeout;
}

int CmsData::DecreaseScreenTimeout()
{
	change_flag_ = true;

	int timeout = cms_self_.settings_.DecreaseScreenTimeout();

	return timeout;
}


int CmsData::IncreaseDawnStartTime()
{
	change_flag_ = true;

	return cms_self_.settings_.IncreaseDawnStartTime();
}

int CmsData::DecreaseDawnStartTime()
{
	change_flag_ = true;

	return cms_self_.settings_.DecreaseDawnStartTime();
}

int CmsData::IncreaseDawnEndTime()
{
	change_flag_ = true;

	return cms_self_.settings_.IncreaseDawnEndTime();
}

int CmsData::DecreaseDawnEndTime()
{
	change_flag_ = true;

	return cms_self_.settings_.DecreaseDawnEndTime();
}

int CmsData::IncreaseDuskStartTime()
{
	change_flag_ = true;

	return cms_self_.settings_.IncreaseDuskStartTime();
}

int CmsData::DecreaseDuskStartTime()
{
	change_flag_ = true;

	return cms_self_.settings_.DecreaseDuskStartTime();
}

int CmsData::IncreaseDuskEndTime()
{
	change_flag_ = true;

	return cms_self_.settings_.IncreaseDuskEndTime();
}

int CmsData::DecreaseDuskEndTime()
{
	change_flag_ = true;

	return cms_self_.settings_.DecreaseDuskEndTime();
}

int CmsData::ResetDefaults(bool reset_all)
{
	change_flag_ = true;

	//	Reset values
	if(cms_self_.is_master)
	{
		//	Reset all slaves
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			cms_slaves_.at(cms_it).lighting_.ResetDefaults();
			cms_slaves_.at(cms_it).climate_.ResetDefaults();
			cms_slaves_.at(cms_it).fans_.ResetDefaults();
			cms_slaves_.at(cms_it).settings_.ResetDefaults();
		}
		//	And itself if required
		if(reset_all)
		{
			cms_self_.lighting_.ResetDefaults();
			cms_self_.climate_.ResetDefaults();
			cms_self_.fans_.ResetDefaults();
			cms_self_.settings_.ResetDefaults();
		}

		//	Send all CAN messages
		if(SendAllValues() != 1)
			return -1;

		unsigned char msg[8];

		//	Then tell slaves to reset their values
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			msg[0] = (unsigned char)CmsData::Instance()->self_id_;						//	From
			msg[1] = (unsigned char)cms_slaves_.at(cms_it).id;	//	To
			msg[2] = SETTINGS;						//	What
			msg[3] = (unsigned char)0xFF;							//	N/A
			msg[4] = (unsigned char)0xFF;							//	N/A
			msg[5] = (unsigned char)0xFF;							//	N/A
			msg[6] = (unsigned char)0x03;							//	Reset
			msg[7] = (unsigned char)0x01;							//	Send

			if (SendControlMessage(msg) == -1)
				return -1;
		}

	}
	else
	{
		//	If it's not master, just reset the values
		cms_self_.lighting_.ResetDefaults();
		cms_self_.climate_.ResetDefaults();
		cms_self_.fans_.ResetDefaults();
		cms_self_.settings_.ResetDefaults();
	}

	return 0;
}

int CmsData::TurnOffHeating()
{
	//	Just the master send the messages
	if(cms_self_.is_master)
	{
		//	Reset all slaves
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			cms_slaves_.at(cms_it).climate_.TurnOffHeating();
		}
		cms_self_.climate_.TurnOffHeating();

		//	Send all CAN messages
		if(SendAllHeating() != 1)
			return -1;

		unsigned char msg[8];

		//	Then tell slaves to reset their values
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			msg[0] = (unsigned char)cms_self_.id;						//	From
			msg[1] = (unsigned char)cms_slaves_.at(cms_it).id;	//	To
			msg[2] = HEATING;				//	What
			msg[3] = (unsigned char)0xFF;					//	Group Index
			msg[4] = (unsigned char)0xFF;							//	N/A
			msg[5] = (unsigned char)0xFF;						//	Cms ID
			msg[6] = (unsigned char)0x02;							//	N/A
			msg[7] = (unsigned char)0x00;						//	Send

			if (SendControlMessage(msg) == -1)
				return -1;
		}
	}
	else
	{
		//	If it's not master, just reset the values
		cms_self_.climate_.TurnOffHeating();
	}

	return 0;
}

int CmsData::RequestTurnOffHeating()
{
	unsigned char msg[8];

	msg[0] = (unsigned char)cms_self_.id;						//	From
	msg[1] = (unsigned char)master_id_;	//	To
	msg[2] = HEATING;				//	What
	msg[6] = (unsigned char)0x02;							//	N/A
	msg[7] = (unsigned char)0x00;						//	Send

	if (SendControlMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::SwitchHeatingEnable(bool enable)
{
	if(cms_self_.is_master)
	{
		cms_self_.climate_.master_heatingenable_ = enable;

		unsigned char msg[8];
		//	Then tell slaves to set their values
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			msg[0] = (unsigned char)cms_self_.id;						//	From
			msg[1] = (unsigned char)cms_slaves_.at(cms_it).id;	//	To
			msg[2] = SETTINGS;						//	What
			msg[3] = (unsigned char)0xFF;							//	N/A
			msg[4] = (unsigned char)0xFF;							//	N/A
			msg[5] = (unsigned char)0xFF;							//	N/A
			msg[6] = (unsigned char)0x05;							//	Reset
			msg[7] = (unsigned char)enable;							//	Send

			if (SendControlMessage(msg) == -1)
				return -1;
		}
	}
	else
	{
		cms_self_.climate_.master_heatingenable_ = enable;
	}

	return 0;
}

int CmsData::SendAllValues()
{
	//	Send messages
	unsigned char msg[8];

	msg[1] = can_id_;			//	To

	if(cms_self_.is_master)
	{
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			msg[0] = (unsigned char)cms_slaves_.at(cms_it).id;		//	From
			//	Do Lights
			for(unsigned int group_it = 0; group_it < cms_slaves_.at(cms_it).lighting_.lightgroups_.size(); group_it++)
			{
				for(unsigned int light_it = 0; light_it < cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
				{
					msg[2] = LIGHTING;	//	What
					msg[5] = (unsigned char)cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance;	//	CAN message instance
					msg[6] = (unsigned char)cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).frame_position;	//	CAN frame position
					msg[7] = (unsigned char)cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).night_value; // ##INITIAL DAY NIGHT MODE

					if (SendCanMessage(msg) == -1)
						return -1;
				}
			}

			//	Heating
			msg[2] = HEATING;
			//	Temperature
			for(unsigned int group_it = 0; group_it < cms_slaves_.at(cms_it).climate_.heating_.size(); group_it++)
			{
				msg[4] = (unsigned char)0x00;
				msg[5] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).message_instance;		//	MMS message instance
				msg[6] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).frame_position;		//	MMS frame position
				msg[7] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).temperature;			//	Value
				if (SendCanMessage(msg) == -1)
					return -1;

				//	Switches
				msg[4] = (unsigned char)0x01;
				for(unsigned int switch_it = 0; switch_it < cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.size(); switch_it++)
				{
					msg[5] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance;		//	MMS message instance
					msg[6] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.at(switch_it).frame_position;			//	MMS frame position
					msg[7] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.at(switch_it).switch_value;			//	Value
					if (SendCanMessage(msg) == -1)
						return -1;
				}
			}

			//	Do Fans
			msg[2] = FANS;
			for(unsigned int group_it = 0; group_it < cms_slaves_.at(cms_it).fans_.fansgroups_.size(); group_it++)
			{
				for(unsigned int fans_it = 0; fans_it < cms_slaves_.at(cms_it).fans_.fansgroups_.at(group_it).fans.size(); fans_it++)
				{

					msg[5] = (unsigned char)cms_slaves_.at(cms_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance;	//	CAN message instance
					msg[6] = (unsigned char)cms_slaves_.at(cms_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).frame_position;		//	CAN frame position
					msg[7] = (unsigned char)cms_slaves_.at(cms_it).fans_.fansgroups_.at(group_it).fans.at(fans_it).level;				//	Value

					if (SendCanMessage(msg) == -1)
						return -1;
				}
			}

		}
	}

	msg[0] = (unsigned char)cms_self_.id;		//	From

	//	Do Lights
	for(unsigned int group_it = 0; group_it < cms_self_.lighting_.lightgroups_.size(); group_it++)
	{
		for(unsigned int light_it = 0; light_it < cms_self_.lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
		{
			msg[2] = LIGHTING;	//	What
			msg[5] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance;	//	CAN message instance
			msg[6] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).frame_position;	//	CAN frame position
			msg[7] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).night_value; // ##INITIAL DAY NIGHT MODE

			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}

	//	Heating
	msg[2] = HEATING;		//	What: Heating

	for(unsigned int group_it = 0; group_it < cms_self_.climate_.heating_.size(); group_it++)
	{
		msg[4] = (unsigned char)0x00;
		msg[5] = (unsigned char)cms_self_.climate_.heating_.at(group_it).message_instance;		//	MMS message instance
		msg[6] = (unsigned char)cms_self_.climate_.heating_.at(group_it).frame_position;		//	MMS frame position
		msg[7] = (unsigned char)cms_self_.climate_.heating_.at(group_it).temperature;			//	Value
		if (SendCanMessage(msg) == -1)
			return -1;

		//	Switches
		msg[4] = (unsigned char)0x01;
		for(unsigned int switch_it = 0; switch_it < cms_self_.climate_.heating_.at(group_it).power_switches.size(); switch_it++)
		{
			msg[5] = (unsigned char)cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance;		//	MMS message instance
			msg[6] = (unsigned char)cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).frame_position;			//	MMS frame position
			msg[7] = (unsigned char)cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).switch_value;			//	Value
			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}

	//	Do Fans
	msg[2] = FANS;
	for(unsigned int group_it = 0; group_it < cms_self_.fans_.fansgroups_.size(); group_it++)
	{
		for(unsigned int fans_it = 0; fans_it < cms_self_.fans_.fansgroups_.at(group_it).fans.size(); fans_it++)
		{
			msg[5] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).message_instance;	//	CAN message instance
			msg[6] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).frame_position;		//	CAN frame position
			msg[7] = (unsigned char)cms_self_.fans_.fansgroups_.at(group_it).fans.at(fans_it).level;				//	Value

			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;


	return 0;
}

int CmsData::SendAllHeating()
{
	//	Send messages
	unsigned char msg[8];

	msg[1] = can_id_;			//	To
	msg[2] = HEATING;

	if(cms_self_.is_master)
	{
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			msg[0] = (unsigned char)cms_slaves_.at(cms_it).id;		//	From

			//	Temperature
			for(unsigned int group_it = 0; group_it < cms_slaves_.at(cms_it).climate_.heating_.size(); group_it++)
			{
				msg[4] = (unsigned char)0x00;
				msg[5] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).message_instance;		//	MMS message instance
				msg[6] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).frame_position;		//	MMS frame position
				msg[7] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).temperature;			//	Value
				if (SendCanMessage(msg) == -1)
					return -1;

				//	Switches
				for(unsigned int switch_it = 0; switch_it < cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.size(); switch_it++)
				{
					msg[4] = (unsigned char)0x01;
					msg[5] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance;		//	MMS message instance
					msg[6] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.at(switch_it).frame_position;			//	MMS frame position
					msg[7] = (unsigned char)cms_slaves_.at(cms_it).climate_.heating_.at(group_it).power_switches.at(switch_it).switch_value;			//	Value
					if (SendCanMessage(msg) == -1)
						return -1;
				}
			}

		}
	}

	msg[0] = (unsigned char)cms_self_.id;		//	From

	for(unsigned int group_it = 0; group_it < cms_self_.climate_.heating_.size(); group_it++)
	{
		msg[4] = (unsigned char)0x00;
		msg[5] = (unsigned char)cms_self_.climate_.heating_.at(group_it).message_instance;		//	MMS message instance
		msg[6] = (unsigned char)cms_self_.climate_.heating_.at(group_it).frame_position;		//	MMS frame position
		msg[7] = (unsigned char)cms_self_.climate_.heating_.at(group_it).temperature;			//	Value
		if (SendCanMessage(msg) == -1)
			return -1;

		//	Switches
		for(unsigned int switch_it = 0; switch_it < cms_self_.climate_.heating_.at(group_it).power_switches.size(); switch_it++)
		{
			msg[4] = (unsigned char)0x01;
			msg[5] = (unsigned char)cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).message_instance;		//	MMS message instance
			msg[6] = (unsigned char)cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).frame_position;			//	MMS frame position
			msg[7] = (unsigned char)cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).switch_value;			//	Value
			if (SendCanMessage(msg) == -1)
				return -1;
		}
	}

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::SaveAllValues()
{
	if(change_flag_)
	{
		XmlData::Instance()->SaveAllParameters();
		change_flag_ = false;
	}

	return 0;
}

int CmsData::SendFuseReset()
{
	if(cms_self_.is_master)
	{
		//	Send messages
		unsigned char msg[8];

		msg[0] = (unsigned char)self_id_;						//	From
		msg[1] = (unsigned char)can_id_;	//	To
		msg[2] = SETTINGS;						//	What
		msg[3] = (unsigned char)0x01;							//	N/A
		msg[4] = (unsigned char)0x00;							//	Dusk end
		msg[5] = (unsigned char)cms_self_.settings_.reset_groups_.at(1).resets.at(0).message_instance;							//	N/A
		msg[6] = (unsigned char)cms_self_.settings_.reset_groups_.at(1).resets.at(0).frame_position;							//	Reset
		msg[7] = (unsigned char)0x01;							//	Send

		if (SendCanMessage(msg) == -1)
			return -1;

		//	Send queued messages
		msg[1] = (unsigned char)can_id_;
		msg[2] = SEND_QUEUE;
		msg[5] = (unsigned char)0x00;
		if (SendCanMessage(msg) == -1)
			return -1;

		return 0;
	}
	return -1;
}

int CmsData::SendMasterSwitch(bool all_on)
{
	unsigned char msg[8];
	if(cms_self_.is_master)
	{
		//	Send messages
		msg[0] = (unsigned char)self_id_;
		msg[1] = (unsigned char)can_id_;
		msg[2] = SETTINGS;
		msg[3] = (unsigned char)0x04;
		msg[4] = (unsigned char)0x00;
		if(all_on)
		{
			msg[5] = (unsigned char)0x03;
			msg[6] = (unsigned char)0x02;
			msg[7] = (unsigned char)0x01;

			if (SendCanMessage(msg) == -1)
				return -1;
		}
		else
		{
			//	ALL OFF
			msg[5] = (unsigned char)0x03;
			msg[6] = (unsigned char)0x03;
			msg[7] = (unsigned char)0x01;

			if (SendCanMessage(msg) == -1)
				return -1;

		}
		return 0;
	}
	else
	{
		//	If I'm not the master, send the master a request
		msg[0] = (unsigned char)self_id_;
		msg[1] = (unsigned char)master_id_;
		msg[2] = LIGHTING;
		msg[6] = (unsigned char)0x01;
		msg[7] = (unsigned char)all_on;

		if (SendControlMessage(msg) == -1)
			return -1;
	}
	return 0;
}

int CmsData::CheckTime()
{
	if(cms_self_.is_master)
	{
		int normalized_minutes = minutes_of_day_ - 240;
		if (normalized_minutes < 0)
			normalized_minutes += 1440;

		if(normalized_minutes > cms_self_.settings_.GetDawnStartNormalizedTime() &&
		normalized_minutes < cms_self_.settings_.GetDuskEndNormalizedTime())
		{
			//	Transition from night to day
			if(is_night_)
			{
				is_night_ = false;
			//	SendNightBit(is_night_);
			}
		}
		else
		{
			//	Transition from day to night
			if(!is_night_)
			{
				is_night_ = true;
			//	SendNightBit(is_night_);
			}
		}
	}
	return 0;
}

int CmsData::SendNightBit(unsigned char message_instance, unsigned char frame_position, bool is_night)
{
	//	Send messages
	unsigned char msg[8];

	msg[0] = (unsigned char)self_id_;						//	From
	msg[1] = (unsigned char)can_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0x05;							//	N/A
	msg[4] = (unsigned char)0xFF;							//	Dusk end
	msg[5] = (unsigned char)message_instance;							//	N/A
	msg[6] = (unsigned char)frame_position;							//	Reset
	msg[7] = (unsigned char)is_night;							//	Send

	if (SendCanMessage(msg) == -1)
		return -1;

	//	Send queued messages
	msg[1] = (unsigned char)can_id_;
	msg[2] = SEND_QUEUE;
	msg[5] = (unsigned char)0x00;
	if (SendCanMessage(msg) == -1)
		return -1;

	return 0;
}

int CmsData::BroadcastOff()
{
	if(cms_self_.is_master)
	{
		unsigned char msg[8];
		//	Do self
		for(unsigned int group_it = 0; group_it < cms_self_.lighting_.lightgroups_.size(); group_it++)
		{
			//	Mount the fixed part
			msg[0] = (unsigned char)self_id_;				//	From
			msg[1] = (unsigned char)can_id_;				//	To
			msg[2] = LIGHTING;				//	What: Light

			cms_self_.lighting_.SetProfile(group_it, (Lighting::OFF));
			//	First we send all the CAN messages
			for (unsigned int light_it = 0; light_it < cms_self_.lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
			{
				//	Mount the rest
				msg[3] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).id;									//	Group id
				msg[4] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).id;				//	Component id
				msg[5] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance;	//	CAN message instance
				msg[6] = (unsigned char)cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).frame_position;	//	CAN frame position
				msg[7] = (unsigned char)cms_self_.lighting_.GetLightValue(group_it, light_it);

				if (SendCanMessage(msg) == -1)
					return -1;
			}
		}


		//	Do Slaves
		for(unsigned int cms_it = 0; cms_it < cms_slaves_.size(); cms_it++)
		{
			for(unsigned int group_it = 0; group_it < cms_slaves_.at(cms_it).lighting_.lightgroups_.size(); group_it++)
			{
				//	Mount the fixed part
				msg[0] = (unsigned char)self_id_;				//	From
				msg[1] = (unsigned char)can_id_;				//	To
				msg[2] = LIGHTING;				//	What: Light

				cms_slaves_.at(cms_it).lighting_.SetProfile(group_it, (Lighting::OFF));
				//	First we send all the CAN messages
				for (unsigned int light_it = 0; light_it < cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
				{
					//	Mount the rest
					msg[3] = (unsigned char)cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).id;									//	Group id
					msg[4] = (unsigned char)cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).id;				//	Component id
					msg[5] = (unsigned char)cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).message_instance;	//	CAN message instance
					msg[6] = (unsigned char)cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).frame_position;	//	CAN frame position
					msg[7] = (unsigned char)cms_slaves_.at(cms_it).lighting_.GetLightValue(group_it, light_it);

					if (SendCanMessage(msg) == -1)
						return -1;
				}

				//	Then we tell the slave we changed our profile
				//	Mount the message
				msg[0] = (unsigned char)self_id_;						//	From
				msg[1] = (unsigned char)cms_slaves_.at(cms_it).id;					//	To
				msg[2] = LIGHTING_PROFILE;				//	What
				msg[3] = (unsigned char)group_it;					//	Group Index
				msg[4] = (unsigned char)0xFF;							//	N/A
				msg[5] = (unsigned char)cms_slaves_.at(cms_it).id;						//	Cms ID
				msg[6] = (unsigned char)0xFF;							//	N/A
				msg[7] = (unsigned char)(Lighting::OFF);				//	profile

				if (SendControlMessage(msg) == -1)
					return -1;
			}
		}

		//	Send queued messages
		msg[1] = (unsigned char)can_id_;
msg[2] = SEND_QUEUE;
msg[5] = (unsigned char)0x00;
		if (SendCanMessage(msg) == -1)
			return -1;
	}

	return 0;
}

int CmsData::SendLightSyncValues(const unsigned char cms_index)
{
	unsigned char msg[8];

	msg[0] = (unsigned char)self_id_;		//	From
	msg[1] = (unsigned char)cms_slaves_.at(cms_index).id;			//	To
	msg[2] = LIGHT_SYNC;					//	What

	for(unsigned int group_it = 0; group_it < cms_slaves_.at(cms_index).lighting_.lightgroups_.size(); group_it++)
	{
		for(unsigned int light_it = 0; light_it < cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
		{
			msg[3] = (unsigned char)group_it;
			msg[4] = (unsigned char)light_it;
			msg[5] = (unsigned char)0xFF;

			msg[6] = (unsigned char)Lighting::DAY;
			msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_it).lights.at(light_it).day_value;
			if (SendControlMessage(msg) == -1)
				return -1;

			msg[6] = (unsigned char)Lighting::EVE;
			msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_it).lights.at(light_it).eve_value;
			if (SendControlMessage(msg) == -1)
				return -1;

			msg[6] = (unsigned char)Lighting::NIGHT;
			msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_it).lights.at(light_it).night_value;
			if (SendControlMessage(msg) == -1)
				return -1;

			msg[6] = (unsigned char)Lighting::THEATRE;
			msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_it).lights.at(light_it).theatre_value;
			if (SendControlMessage(msg) == -1)
				return -1;

			msg[6] = (unsigned char)Lighting::SHOWER;
			msg[7] = (unsigned char)cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_it).lights.at(light_it).shower_value;
			if (SendControlMessage(msg) == -1)
				return -1;
		}
	}

	return 0;
}

void CmsData::SetAirconTemperature(const unsigned char new_temperature)
{
	cms_self_.climate_.SetAirconTemperature(new_temperature);
}

void CmsData::SetAirconAmbientTemperature(const unsigned char new_temperature)
{
	cms_self_.climate_.SetAirconAmbientTemperature(new_temperature);
}

void CmsData::SetAirconMode(int new_mode)
{
	cms_self_.climate_.SetAirconMode((Climate::Mode)new_mode);
}

void CmsData::SetAirconFanSpeed(const unsigned char new_fanspeed)
{
	cms_self_.climate_.SetAirconFanSpeed(new_fanspeed);
}

void CmsData::SetAirconPower(bool new_power)
{
	cms_self_.climate_.SetAirconPower(new_power);
}

//========================================================================================
//		Query methods
//========================================================================================

	//	Lights methods
Lighting::Profile CmsData::GetLightProfile(const unsigned char cms_id, const unsigned char group_index)
{
	if(cms_id == self_id_)
		return cms_self_.lighting_.lightgroups_.at(group_index).profile;
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_index = 0;
		for(cms_index = 0; cms_index < cms_slaves_.size(); cms_index++)
		{
			if(cms_slaves_.at(cms_index).id == cms_id)
					break;
		}

		return cms_slaves_.at(cms_index).lighting_.lightgroups_.at(group_index).profile;
	}
}

int CmsData::GetLightValue(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index)
{
	if(cms_id == self_id_)
		return cms_self_.lighting_.GetLightValue(group_index, light_index);
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_index = 0;
		while (cms_slaves_.at(cms_index).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_index >= cms_slaves_.size())
				return -1;
		}

		return cms_slaves_.at(cms_index).lighting_.GetLightValue(group_index, light_index);
	}
}

int CmsData::SyncLightValue(const unsigned char group_index, const unsigned char light_index, const unsigned char profile, const unsigned char value)
{
	switch(profile)
	{
		case Lighting::DAY:
			cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).day_value = value;
		break;
		case Lighting::EVE:
			cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).eve_value = value;
		break;
		case Lighting::NIGHT:
			cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).night_value = value;
		break;
		case Lighting::THEATRE:
			cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).theatre_value = value;
		break;
		case Lighting::SHOWER:
			cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_index).shower_value = value;
		break;
	}

	return 0;
}
	//	Climate methods
bool CmsData::GetAirconPower()
{
	return cms_self_.climate_.aircon_.power_switch.switch_value;
}

int CmsData::GetAirconTemperature()
{
	return cms_self_.climate_.aircon_.temperature;
}

int CmsData::GetAirconAmbientTemperature()
{
	return cms_self_.climate_.aircon_.ambient_temperature;
}

Climate::Mode CmsData::GetAirconMode()
{
	return cms_self_.climate_.aircon_.mode;
}

unsigned char CmsData::GetAirconFanSpeed()
{
	return cms_self_.climate_.aircon_.fan_speed;
}

bool CmsData::GetHeatingPower(const unsigned char group_index, const unsigned char switch_index)
{
	return cms_self_.climate_.heating_.at(group_index).power_switches.at(switch_index).switch_value;
}

int CmsData::GetHeatingTemperature(const unsigned char group_index)
{
	return (int)(cms_self_.climate_.heating_.at(group_index).temperature);
}

	//	Fans methods
int CmsData::GetFansValue(const unsigned char group_index, const unsigned char fans_index)
{
	return (int)(cms_self_.fans_.GetFanValue(group_index, fans_index));
}

	//	Settings methods
int CmsData::GetScreenBrightness()
{
	switch (cms_self_.lighting_.GetProfile(0))
	{
	case Lighting::DAY:
		return cms_self_.settings_.GetScreenBrightness(Lighting::DAY);
		break;

	case Lighting::EVE:
		return cms_self_.settings_.GetScreenBrightness(Lighting::EVE);
		break;

	case Lighting::NIGHT:
		return cms_self_.settings_.GetScreenBrightness(Lighting::NIGHT);
		break;

	case Lighting::THEATRE:
		return cms_self_.settings_.GetScreenBrightness(Lighting::THEATRE);
		break;

	case Lighting::MANUAL:
		return cms_self_.settings_.GetScreenBrightness(Lighting::MANUAL);
		break;

	case Lighting::OFF:
		return cms_self_.settings_.GetScreenBrightness(Lighting::OFF);
		break;

	default:
		return cms_self_.settings_.GetScreenBrightness(Lighting::DAY);
		break;
	}
	return 0;
}

int CmsData::GetScreenTimeout()
{
	return cms_self_.settings_.screensettings_.screen_timeout;
}

int CmsData::GetDawnStartTime()
{
	return cms_self_.settings_.GetDawnStartTime();
}

int CmsData::GetDawnEndTime()
{
	return cms_self_.settings_.GetDawnEndTime();
}

int CmsData::GetDuskStartTime()
{
	return cms_self_.settings_.GetDuskStartTime();
}

int CmsData::GetDuskEndTime()
{
	return cms_self_.settings_.GetDuskEndTime();
}

int CmsData::GetSunriseTime()
{
	return cms_self_.settings_.GetSunriseTime();
}

int CmsData::GetSunsetTime()
{
	return cms_self_.settings_.GetSunsetTime();
}

int CmsData::GetDawnStartNormalizedTime()
{
	return cms_self_.settings_.GetDawnStartNormalizedTime();
}

int CmsData::GetDawnEndNormalizedTime()
{
	return cms_self_.settings_.GetDawnEndNormalizedTime();
}

int CmsData::GetDuskStartNormalizedTime()
{
	return cms_self_.settings_.GetDuskStartNormalizedTime();
}

int CmsData::GetDuskEndNormalizedTime()
{
	return cms_self_.settings_.GetDuskEndNormalizedTime();
}

int CmsData::GetSunriseNormalizedTime()
{
	return cms_self_.settings_.GetSunriseNormalizedTime();
}

int CmsData::GetSunsetNormalizedTime()
{
	return cms_self_.settings_.GetSunsetNormalizedTime();
}

unsigned int CmsData::CalculateMmsHash(string str)
{
    unsigned int h = 0;
    char *c = &(str[0]);
    while (*c)
    {
		h = ((h << 6) + (h << 16) - h) + (*c);
		c ++;
    }

	return h;
}
