#include "settings_data.h"

	//Constructors and destructor
Settings::Settings()
{
	has_master_heating_ = false;
}

Settings::~Settings()
{

}


	//	Values initializer;
void Settings::InitValues()
{
	timesettings_.dawn_start_time = 150;		//6:30am
	timesettings_.dawn_end_time = 270;		//8:30am
	timesettings_.dusk_start_time = 780;		//5:00pm
	timesettings_.dusk_end_time = 930;		//7:30pm

	timesettings_.sunrise_time = 120;		//6:00am
	timesettings_.sunset_time = 890;			//6:50pm
}

	//Getters and setters
void Settings::SetDawnStartTime(const int new_value)
{
	int offset_value = new_value - 240;
	if (offset_value < 0)
		offset_value += 1440;

	timesettings_.dawn_start_time = offset_value;
}

int Settings::GetDawnStartTime()
{
	int true_value = timesettings_.dawn_start_time + 240;
	if (true_value > 1440)
		true_value -= 1440;

	return true_value;
}

int Settings::GetDawnStartNormalizedTime()
{
	return timesettings_.dawn_start_time;
}


void Settings::SetDawnEndTime(const int new_value)
{
	int offset_value = new_value - 240;
	if (offset_value < 0)
		offset_value += 1440;

	timesettings_.dawn_end_time = offset_value;
}

int Settings::GetDawnEndTime()
{
	int true_value = timesettings_.dawn_end_time + 240;
	if (true_value > 1440)
		true_value -= 1440;

	return true_value;
}

int Settings::GetDawnEndNormalizedTime()
{
	return timesettings_.dawn_end_time;
}


void Settings::SetDuskStartTime(const int new_value)
{
	int offset_value = new_value - 240;
	if (offset_value < 0)
		offset_value += 1440;

	timesettings_.dusk_start_time = offset_value;
}

int Settings::GetDuskStartTime()
{
	int true_value = timesettings_.dusk_start_time + 240;
	if (true_value > 1440)
		true_value -= 1440;

	return true_value;
}

int Settings::GetDuskStartNormalizedTime()
{
	return timesettings_.dusk_start_time;
}


void Settings::SetDuskEndTime(const int new_value)
{
	int offset_value = new_value - 240;
	if (offset_value < 0)
		offset_value += 1440;

	timesettings_.dusk_end_time = offset_value;
}

int Settings::GetDuskEndTime()
{
	int true_value = timesettings_.dusk_end_time + 240;
	if (true_value > 1440)
		true_value -= 1440;

	return true_value;
}

int Settings::GetDuskEndNormalizedTime()
{
	return timesettings_.dusk_end_time;
}

void Settings::SetSunriseTime(const int new_value)
{
	int offset_value = new_value - 240;
	if (offset_value < 0)
		offset_value += 1440;

	timesettings_.sunrise_time = offset_value;
}

int Settings::GetSunriseTime()
{
	int true_value = timesettings_.sunrise_time + 240;
	if (true_value > 1440)
		true_value -= 1440;

	return true_value;
}

int Settings::GetSunriseNormalizedTime()
{
	return timesettings_.sunrise_time;
}

void Settings::SetSunsetTime(const int new_value)
{
	int offset_value = new_value - 240;
	if (offset_value < 0)
		offset_value += 1440;

	timesettings_.sunset_time = offset_value;
}

int Settings::GetSunsetTime()
{
	int true_value = timesettings_.sunset_time + 240;
	if (true_value > 1440)
		true_value -= 1440;

	return true_value;
}

int Settings::GetSunsetNormalizedTime()
{
	return timesettings_.sunset_time;
}


	//Value manipulators
int Settings::IncreaseScreenBrightness(int profile)
{
	int *selected_property = NULL;
	switch (profile)
	{
		//	DAY
	case 0:
		selected_property = &screensettings_.screen_brightness_day;
		break;

		//	EVE
	case 1:
		selected_property = &screensettings_.screen_brightness_eve;
		break;

		//	NIGHT
	case 2:
		selected_property = &screensettings_.screen_brightness_night;
		break;

		//	THEATRE
	case 3:
		selected_property = &screensettings_.screen_brightness_theatre;
		break;

		//	MANUAL
	case 5:
		selected_property = &screensettings_.screen_brightness_manual;
		break;

		//	OFF
	case 6:
		selected_property = &screensettings_.screen_brightness_off;
		break;
	}
	if (*selected_property < 100)
		*selected_property += 5;

	return *selected_property;
}

int Settings::DecreaseScreenBrightness(int profile)
{
	int *selected_property = NULL;
	switch (profile)
	{
		//	DAY
	case 0:
		selected_property = &screensettings_.screen_brightness_day;
		break;

		//	EVE
	case 1:
		selected_property = &screensettings_.screen_brightness_eve;
		break;

		//	NIGHT
	case 2:
		selected_property = &screensettings_.screen_brightness_night;
		break;

		//	THEATRE
	case 3:
		selected_property = &screensettings_.screen_brightness_theatre;
		break;

		//	MANUAL
	case 5:
		selected_property = &screensettings_.screen_brightness_manual;
		break;

		//	OFF
	case 6:
		selected_property = &screensettings_.screen_brightness_off;
		break;
	}
	if (*selected_property > 5)
		*selected_property -= 5;

	return *selected_property;
}

int Settings::GetScreenBrightness(int profile)
{
	int *selected_property = NULL;
	switch (profile)
	{
		//	DAY
	case 0:
		selected_property = &screensettings_.screen_brightness_day;
		break;

		//	EVE
	case 1:
		selected_property = &screensettings_.screen_brightness_eve;
		break;

		//	NIGHT
	case 2:
		selected_property = &screensettings_.screen_brightness_night;
		break;

		//	THEATRE
	case 3:
		selected_property = &screensettings_.screen_brightness_theatre;
		break;

		//	MANUAL
	case 5:
		selected_property = &screensettings_.screen_brightness_manual;
		break;

		//	OFF
	case 6:
		selected_property = &screensettings_.screen_brightness_off;
		break;
	}

	if(*selected_property > 100)
		return 100;

	if(*selected_property < 5)
		return 5;

	return *selected_property;
}

int Settings::IncreaseScreenTimeout()
{
	if (screensettings_.screen_timeout < MAX_SCREENTIMEOUT)
		screensettings_.screen_timeout += 2;


	return screensettings_.screen_timeout;
}

int Settings::DecreaseScreenTimeout()
{
	if (screensettings_.screen_timeout > MIN_SCREENTIMEOUT)
		screensettings_.screen_timeout -= 2;

	return screensettings_.screen_timeout;
}

	//Dawn starting time has to between 4am and dawn end time
int Settings::IncreaseDawnStartTime()
{
	if (timesettings_.dawn_start_time < timesettings_.dawn_end_time)
		timesettings_.dawn_start_time += 5;

	return GetDawnStartTime();
}

int Settings::DecreaseDawnStartTime()
{
	if (timesettings_.dawn_start_time > 0) //4am
		timesettings_.dawn_start_time -= 5;

	return GetDawnStartTime();
}

	//Dawn end time has to be between dawn start time and dusk start time
int Settings::IncreaseDawnEndTime()
{
	if (timesettings_.dawn_end_time < timesettings_.dusk_start_time)
		timesettings_.dawn_end_time += 5;

	return GetDawnEndTime();
}

int Settings::DecreaseDawnEndTime()
{
	if (timesettings_.dawn_end_time > timesettings_.dawn_start_time)
		timesettings_.dawn_end_time -= 5;

	return GetDawnEndTime();
}

	//Dusk start time has to be between dawn end time and dusk end time
int Settings::IncreaseDuskStartTime()
{
	if (timesettings_.dusk_start_time < timesettings_.dusk_end_time)
		timesettings_.dusk_start_time += 5;

	return GetDuskStartTime();
}

int Settings::DecreaseDuskStartTime()
{
	if (timesettings_.dusk_start_time > timesettings_.dawn_end_time)
		timesettings_.dusk_start_time -= 5;

	return GetDuskStartTime();
}

	//Dusk end time has to be between dawn end time and 3:55am
int Settings::IncreaseDuskEndTime()
{
	if (timesettings_.dusk_end_time < 1435) //3:55am
		timesettings_.dusk_end_time += 5;

	return GetDuskEndTime();
}

int Settings::DecreaseDuskEndTime()
{
	if (timesettings_.dusk_end_time > timesettings_.dusk_start_time)
		timesettings_.dusk_end_time -= 5;

	return GetDuskEndTime();
}

void Settings::ResetDefaults()
{
	screensettings_.screen_timeout = DEFAULT_SCREENTIMEOUT;
	screensettings_.screen_brightness_day = screensettings_.screen_brightness_day_defaultvalue;
	screensettings_.screen_brightness_eve = screensettings_.screen_brightness_eve_defaultvalue;
	screensettings_.screen_brightness_night = screensettings_.screen_brightness_night_defaultvalue;
	screensettings_.screen_brightness_theatre = screensettings_.screen_brightness_theatre_defaultvalue;
}

// Screen timeout workaraound
string Settings::GetScreenTimeoutText(int timeout)
{
    switch(timeout)
    {
        case 2:     return "5s";
        case 4:     return "10s";
        case 6:     return "15s";
        case 8:     return "20s";
        case 10:    return "30s";
        case 12:    return "45s";
        case 14:    return "1m";
        case 16:    return "2m";
        case 18:    return "3m";
        case 20:    return "5m";
        case 22:    return "7m";
        case 24:    return "10m";
        case 26:    return "15m";
        case 28:    return "20m";
        case 30:    return "30m";
        default:    return "1m";
    }
}
int Settings::GetScreenTimeoutTime(int timeout)
{
    switch(timeout)
    {
        case 2:     return 5;
        case 4:     return 10;
        case 6:     return 15;
        case 8:     return 20;
        case 10:    return 30;
        case 12:    return 45;
        case 14:    return 60;
        case 16:    return 120;
        case 18:    return 180;
        case 20:    return 300;
        case 22:    return 420;
        case 24:    return 600;
        case 26:    return 900;
        case 28:    return 1200;
        case 30:    return 1800;
        default:    return 60;
    }
}
