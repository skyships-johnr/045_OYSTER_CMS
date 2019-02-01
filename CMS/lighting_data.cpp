#include "lighting_data.h"

//========================================================================================
//		Constructor and destructor
//========================================================================================

Lighting::Lighting()
{
	nightbit_message_instance_ = 0;
	nightbit_frame_position_ = 0;
}

Lighting::~Lighting()
{

}


//========================================================================================
//		Initializer
//========================================================================================

void Lighting::InitValues()
{

}


//========================================================================================
//		Getters and setters
//========================================================================================

void Lighting::SetMasterSwitch(const bool newValue)
{
	master_switch_.value = newValue;
}

bool Lighting::GetMasterSwitch()
{
	return master_switch_.value;
}

void Lighting::SetProfile(const unsigned char group_index, const Lighting::Profile new_value)
{
	//	When changing to manual, it should keep the values of the last profile
	if(lightgroups_.at(group_index).profile != MANUAL && new_value == MANUAL)
	{
		for (unsigned int light_it = 0; light_it < lightgroups_.at(group_index).lights.size(); light_it++)
		{
			lightgroups_.at(group_index).lights.at(light_it).manual_value =
			GetLightValue(group_index, light_it);
		}
	}

	lightgroups_.at(group_index).profile = new_value;
}

Lighting::Profile Lighting::GetProfile(const unsigned char group_index)
{
	return lightgroups_.at(group_index).profile;
}

int Lighting::GetLightValue(const unsigned char group_index, const unsigned char light_index)
{
	switch (lightgroups_.at(group_index).profile)
	{
	case DAY:
		return lightgroups_.at(group_index).lights.at(light_index).day_value;
		break;
	case EVE:
		return lightgroups_.at(group_index).lights.at(light_index).eve_value;
		break;
	case NIGHT:
		return lightgroups_.at(group_index).lights.at(light_index).night_value;
		break;
	case THEATRE:
		return lightgroups_.at(group_index).lights.at(light_index).theatre_value;
		break;
	case SHOWER:
		return lightgroups_.at(group_index).lights.at(light_index).shower_value;
		break;
	case MANUAL:
		return lightgroups_.at(group_index).lights.at(light_index).manual_value;
		break;
	case OFF:
		return 0;
		break;
	}
	//	Default
	return lightgroups_.at(group_index).lights.at(light_index).day_value;
}

//========================================================================================
//		Value manipulation functions
//========================================================================================

//	Light intensity manipulators
int Lighting::IncreaseLight(const unsigned char group_index, const unsigned char light_index)
{
	if (group_index < lightgroups_.size())
	{
		if (light_index < lightgroups_.at(group_index).lights.size())
		{
			//	Get pointer to the selected light
			Lighting::LightData *selected_light = &lightgroups_.at(group_index).lights.at(light_index);

			//	Get pointer to the selected property of the selected light
			int *selected_property = 0;
			switch (lightgroups_.at(group_index).profile)
			{
			case DAY:
				selected_property = &selected_light->day_value;
				break;
			case EVE:
				selected_property = &selected_light->eve_value;
				break;
			case NIGHT:
				selected_property = &selected_light->night_value;
				break;
			case THEATRE:
				selected_property = &selected_light->theatre_value;
				break;
			case SHOWER:
				selected_property = &selected_light->shower_value;
				break;
			case MANUAL:
				selected_property = &selected_light->manual_value;
				break;
			default:
				selected_property = &selected_light->day_value;
				break;
			}

			//	Adjust level in case it was messed up in group
			if (*selected_property < MIN_LIGHT_INTENSITY)
			{
				*selected_property = MIN_LIGHT_INTENSITY;
			}
			if (*selected_property > MAX_LIGHT_INTENSITY)
			{
				*selected_property = MAX_LIGHT_INTENSITY;
			}

			if (*selected_property < MAX_LIGHT_INTENSITY)
			{
				*selected_property += 5;
			}

			return *selected_property;
		}
		else return -1;
	}
	else return -1;
}

int Lighting::DecreaseLight(const unsigned char group_index, const unsigned char light_index)
{
	if (group_index < lightgroups_.size())
	{
		if (light_index < lightgroups_.at(group_index).lights.size())
		{
			//	Get pointer to the selected light
			Lighting::LightData *selected_light = &lightgroups_.at(group_index).lights.at(light_index);

			//	Get pointer to the selected property of the selected light
			int *selected_property = 0;
			switch (lightgroups_.at(group_index).profile)
			{
			case DAY:
				selected_property = &selected_light->day_value;
				break;
			case EVE:
				selected_property = &selected_light->eve_value;
				break;
			case NIGHT:
				selected_property = &selected_light->night_value;
				break;
			case THEATRE:
				selected_property = &selected_light->theatre_value;
				break;
			case SHOWER:
				selected_property = &selected_light->shower_value;
				break;
			case MANUAL:
				selected_property = &selected_light->manual_value;
				break;
			default:
				selected_property = &selected_light->day_value;
				break;
			}

			//	Adjust level in case it was messed up in group
			if (*selected_property < MIN_LIGHT_INTENSITY)
			{
				*selected_property = MIN_LIGHT_INTENSITY;
			}
			if (*selected_property > MAX_LIGHT_INTENSITY)
			{
				*selected_property = MAX_LIGHT_INTENSITY;
			}

			if (*selected_property > MIN_LIGHT_INTENSITY)
			{
				*selected_property -= 5;
			}

			return *selected_property;
		}
		else return -1;
	}
	else return -1;
}

int Lighting::IncreaseGroupLight(const unsigned char group_index)
{
	if (group_index < lightgroups_.size())
	{
		if(lightgroups_.at(group_index).profile == MANUAL)
		{
			int lowest_value = MAX_LIGHT_INTENSITY;

			//	Check through every light the conditions to raise or not
			for (unsigned int light_it = 0; light_it < lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	If any is equal to maximum, return without changes
				if (lightgroups_.at(group_index).lights.at(light_it).manual_value == MAX_GROUP_INTENSITY)
					return 0;

				if (lightgroups_.at(group_index).lights.at(light_it).manual_value  < lowest_value)
					lowest_value = lightgroups_.at(group_index).lights.at(light_it).manual_value;

			}
			//	If all are over or equal to 100%, stop increasing
			if (lowest_value==MAX_LIGHT_INTENSITY)
				return 0;

			//	If it reaches this point is because the Maximum has not yet been reached

			//	Iterates again through every light in the selected group
			for (unsigned int light_it = 0; light_it < lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Increase each one of them
				lightgroups_.at(group_index).lights.at(light_it).manual_value  += 5;
			}
		}
		return 0;
	}
	else return -1;
}

int Lighting::DecreaseGroupLight(const unsigned char group_index)
{
	if (group_index < lightgroups_.size())
	{
		if(lightgroups_.at(group_index).profile == MANUAL)
		{
			int highest_value = MIN_LIGHT_INTENSITY;

			//	Iterates through every light in the selected group
			for (unsigned int light_it = 0; light_it < lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	If any is equal to minimum, return without changes
				if (lightgroups_.at(group_index).lights.at(light_it).manual_value  == MIN_GROUP_INTENSITY)
					return 0;

				if (lightgroups_.at(group_index).lights.at(light_it).manual_value > highest_value)
					highest_value = lightgroups_.at(group_index).lights.at(light_it).manual_value;
			}
			//	If all are under or equal to 0%, stop decreasing
			if (highest_value == MIN_LIGHT_INTENSITY)
				return 0;
			//	If it reaches this point is because the Maximum has not yet been reached

			//	Iterates again through every light in the selected group
			for (unsigned int light_it = 0; light_it < lightgroups_.at(group_index).lights.size(); light_it++)
			{
				//	Increase each one of them
				lightgroups_.at(group_index).lights.at(light_it).manual_value -= 5;
			}
		}
		return 0;
	}
	else return -1;
}

void Lighting::ResetDefaults()
{
	for(unsigned int group_it = 0; group_it < lightgroups_.size(); group_it++)
	{
		lightgroups_.at(group_it).profile = DAY;
		for(unsigned int light_it = 0; light_it < lightgroups_.at(group_it).lights.size(); light_it++)
		{
			lightgroups_.at(group_it).lights.at(light_it).day_value = lightgroups_.at(group_it).lights.at(light_it).day_defaultvalue;
			lightgroups_.at(group_it).lights.at(light_it).eve_value = lightgroups_.at(group_it).lights.at(light_it).eve_defaultvalue;
			lightgroups_.at(group_it).lights.at(light_it).night_value = lightgroups_.at(group_it).lights.at(light_it).night_defaultvalue;
			lightgroups_.at(group_it).lights.at(light_it).theatre_value = lightgroups_.at(group_it).lights.at(light_it).theatre_defaultvalue;
			lightgroups_.at(group_it).lights.at(light_it).shower_value = lightgroups_.at(group_it).lights.at(light_it).shower_defaultvalue;
		}
	}
}
