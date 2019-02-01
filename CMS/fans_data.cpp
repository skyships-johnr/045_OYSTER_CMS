#include "fans_data.h"


Fans::Fans()
{

}

Fans::~Fans()
{

}


int Fans::InitValues()
{
	return 1;
}


int Fans::SetFanValue(unsigned char group_index, unsigned char fan_index, unsigned char new_level)
{
	fansgroups_.at(group_index).fans.at(fan_index).level = new_level;

	return 1;
}

unsigned char Fans::GetFanValue(unsigned char group_index, unsigned char fan_index)
{
	return fansgroups_.at(group_index).fans.at(fan_index).level;
}


int Fans::IncreaseFanLevel(unsigned char group_index, unsigned char fan_index)
{
	if (fansgroups_.at(group_index).fans.at(fan_index).level < MAX_FAN_INTENSITY)
	{
		fansgroups_.at(group_index).fans.at(fan_index).level += 5;
	}

	return 1;
}

int Fans::DecreaseFanLevel(unsigned char group_index, unsigned char fan_index)
{
	if (fansgroups_.at(group_index).fans.at(fan_index).level > MIN_FAN_INTENSITY)
	{
		fansgroups_.at(group_index).fans.at(fan_index).level -= 5;
	}

	return 1;
}

void Fans::ResetDefaults()
{
	for(unsigned int group_it = 0; group_it < fansgroups_.size(); group_it++)
	{
		for(unsigned int fans_it = 0; fans_it < fansgroups_.at(group_it).fans.size(); fans_it++)
		{
			fansgroups_.at(group_it).fans.at(fans_it).level = fansgroups_.at(group_it).fans.at(fans_it).default_level;
		}
	}
}
