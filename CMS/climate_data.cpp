#include "climate_data.h"

#define MANUAL_FANSPEED_OFF		0
#define MANUAL_FANSPEED_MAX		5
#define AUTO_FANSPEED_OFF		6
#define AUTO_FANSPEED_MAX		11

//========================================================================================
//		Constructors and destructor
//========================================================================================
Climate::Climate()
{
	master_heatingenable_ = false;
	has_master_switch_ = false;
}

Climate::~Climate()
{

}


//========================================================================================
//		Initializers
//========================================================================================
void Climate::InitValues()
{

}


//========================================================================================
//		Getters and setters
//========================================================================================

void Climate::SetAirconTemperature(const unsigned char new_value)
{
	aircon_.temperature = new_value;
}

unsigned char Climate::GetAirconTemperature()
{
	return aircon_.temperature;
}

void Climate::SetAirconAmbientTemperature(const unsigned char new_value)
{
	aircon_.ambient_temperature = new_value;
}

unsigned char Climate::GetAirconAmbientTemperature()
{
	return aircon_.ambient_temperature;
}

void Climate::SetAirconPower(const bool new_value)
{
	aircon_.power_switch.switch_value = new_value;
}

bool Climate::GetAirconPower()
{
	return aircon_.power_switch.switch_value;
}

void Climate::SetAirconFanSpeed(const unsigned char new_value)
{
	aircon_.fan_speed = new_value;
}

unsigned char Climate::GetAirconFanSpeed()
{
	return aircon_.fan_speed;
}

void Climate::SetAirconMode(const Climate::Mode new_value)
{
	aircon_.mode = new_value;
}

Climate::Mode Climate::GetAirconMode()
{
	return aircon_.mode;
}


void Climate::SetHeatingTemperature(const unsigned char group_index, const unsigned char new_value)
{
	heating_.at(group_index).temperature = new_value;
}

unsigned char Climate::GetHeatingTemperature(const unsigned char group_index)
{
	return heating_.at(group_index).temperature;
}

void Climate::SetHeatingPower(const unsigned char group_index, const unsigned char switch_index, const bool new_value)
{
	heating_.at(group_index).power_switches.at(switch_index).switch_value = new_value;
}

bool Climate::GetHeatingPower(const unsigned char group_index, const unsigned char switch_index)
{
	return heating_.at(group_index).power_switches.at(switch_index).switch_value;
}

//========================================================================================
//		Value manipulation functions
//========================================================================================


unsigned char Climate::IncreaseAirconTemperature()
{
	if(aircon_.power_switch.switch_value)
	{
		if (aircon_.temperature < MAX_TEMPERATURE)
		{
			aircon_.temperature += 1;
		}
	}
	return aircon_.temperature;
}

unsigned char Climate::DecreaseAirconTemperature()
{
	if(aircon_.power_switch.switch_value)
	{
		if (aircon_.temperature > MIN_TEMPERATURE)
		{
			aircon_.temperature -= 1;
		}
	}
	return aircon_.temperature;
}

unsigned char Climate::ChangeAirconFanSpeed()
{
	if(aircon_.power_switch.switch_value)
	{
		//	If it is one of the manuals
		if (aircon_.mode == HEATINGONLY || aircon_.mode == COOLONLY)
		{
			//	If it was in auto range, bring to manual, at speed 1
			if(aircon_.fan_speed >= AUTO_FANSPEED_OFF || aircon_.fan_speed == MANUAL_FANSPEED_OFF)
				aircon_.fan_speed = MANUAL_FANSPEED_OFF + 1;	//	means manual 1
			else
			{
				if(aircon_.fan_speed > MANUAL_FANSPEED_OFF && aircon_.fan_speed < MANUAL_FANSPEED_MAX)
					aircon_.fan_speed++;
				else
				{
					aircon_.fan_speed = MANUAL_FANSPEED_OFF;
				}
			}
		}
	}
	return aircon_.fan_speed;
}

bool Climate::SwitchAirconPower()
{
	aircon_.power_switch.switch_value = !(aircon_.power_switch.switch_value);

	return aircon_.power_switch.switch_value;
}

Climate::Mode Climate::ChangeAirconMode()
{
	if(aircon_.power_switch.switch_value)
	{
		if(aircon_.mode < COOLONLY)
		{
			if(aircon_.mode >= AUTO)
			{
				int modeint = (int)aircon_.mode;
				modeint++;
				aircon_.mode = (Climate::Mode)modeint;
			}
			else
				aircon_.mode = AUTO;
		}
		else
			aircon_.mode = AUTO;
	}

	return aircon_.mode;
}


unsigned char Climate::IncreaseHeatingTemperature(const unsigned char group_index)
{
	if (heating_.at(group_index).temperature < MAX_TEMPERATURE)
	{
		heating_.at(group_index).temperature += 1;
	}

	return heating_.at(group_index).temperature;
}

unsigned char Climate::DecreaseHeatingTemperature(const unsigned char group_index)
{
	if (heating_.at(group_index).temperature > MIN_TEMPERATURE)
	{
		heating_.at(group_index).temperature -= 1;
	}

	return heating_.at(group_index).temperature;
}

bool Climate::SwitchHeatingPower(const unsigned char group_index, const unsigned char switch_index)
{
	heating_.at(group_index).power_switches.at(switch_index).switch_value = !(heating_.at(group_index).power_switches.at(switch_index).switch_value);

	return heating_.at(group_index).power_switches.at(switch_index).switch_value;
}

void Climate::ResetDefaults()
{
	//	Reset Aircon
	aircon_.temperature = aircon_.temperature_defaultvalue;
	aircon_.power_switch.switch_value = false;

	//	Reset Heating
	for(unsigned int group_it = 0; group_it < heating_.size(); group_it ++)
	{
		heating_.at(group_it).temperature = heating_.at(group_it).temperature_defaultvalue;
		for(unsigned int switch_it = 0; switch_it < heating_.at(group_it).power_switches.size(); switch_it++)
		{
			heating_.at(group_it).power_switches.at(switch_it).switch_value = false;
		}
	}
}

void Climate::TurnOffHeating()
{
	for(unsigned int group_it = 0; group_it < heating_.size(); group_it ++)
	{
		for(unsigned int switch_it = 0; switch_it < heating_.at(group_it).power_switches.size(); switch_it++)
		{
			heating_.at(group_it).power_switches.at(switch_it).switch_value = false;
		}
	}
}
