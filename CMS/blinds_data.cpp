#include "blinds_data.h"


//========================================================================================
//		Constructor and destructor
//========================================================================================

Blinds::Blinds()
{

}

Blinds::~Blinds()
{

}

//========================================================================================
//		Initializer
//========================================================================================

void Blinds::InitValues()
{

}

//========================================================================================
//		Vector manipulation
//========================================================================================


int Blinds::GetBlindsGroup(const unsigned char group_index, Blinds::BlindsGroup *blindsgroup)
{
	if (group_index < blindsgroups_.size())
	{
		*blindsgroup = blindsgroups_.at(group_index);
		return 0;
	}
	else return -1;
}


unsigned int Blinds::GetBlindsGroupsCount()
{
	return blindsgroups_.size();
}


//========================================================================================
//		Trigger handling functions
//========================================================================================

int Blinds::OpenBlinds(const unsigned char group_index, const unsigned char blinds_index)
{
	if (group_index < blindsgroups_.size())
	{
		if (blinds_index < blindsgroups_.at(group_index).blinds.size())
		{
			//	TODO: Send message

			return 0;
		}
		else return -1;
	}

	else return -1;
}

int Blinds::OpenBlindsStop(const unsigned char group_index, const unsigned char blinds_index)
{
	if (group_index < blindsgroups_.size())
	{
		if (blinds_index < blindsgroups_.at(group_index).blinds.size())
		{
			//	TODO: Send message

			return 0;
		}
		else return -1;
	}

	else return -1;
}

int Blinds::CloseBlinds(const unsigned char group_index, const unsigned char blinds_index)
{
	if (group_index < blindsgroups_.size())
	{
		if (blinds_index < blindsgroups_.at(group_index).blinds.size())
		{
			//	TODO: Send message

			return 0;
		}
		else return -1;
	}

	else return -1;
}

int Blinds::CloseBlindsStop(const unsigned char group_index, const unsigned char blinds_index)
{
	if (group_index < blindsgroups_.size())
	{
		if (blinds_index < blindsgroups_.at(group_index).blinds.size())
		{
			//	TODO: Send message

			return 0;
		}
		else return -1;
	}

	else return -1;
}
