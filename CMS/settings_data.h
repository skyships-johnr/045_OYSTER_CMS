#ifndef CMS_SETTINGS_H_
#define CMS_SETTINGS_H_

#include <kanzi/kanzi.hpp>
#include "vector_class.h"

#define MIN_SCREENTIMEOUT 2
#define MAX_SCREENTIMEOUT 30

using namespace kanzi;

class Settings
{
public:
	typedef enum ResetType
	{
		GUESTS = 0,
		ALL = 1,
		CAN = 2
	} ResetType;

	typedef struct ResetData
	{
		//	Id
		unsigned char id;

		//	Name on screen
		string name;

		//	GroupId
		unsigned char group;


		ResetType reset_type;

		//	Message information
		unsigned char message_instance;
		unsigned char frame_position;

	}ResetData;

	typedef struct ResetGroup
	{
		//	Id
		unsigned char id;
		//	Name on screen
		string name;
		//	Vector of blinds
		vector_class<ResetData> resets;

	}ResetGroup;

	typedef struct TimeSettings
	{
		//Time in minutes with an offset of minus 4 hours (ex: 4am = 0, 10am = (10-4)*60 = 360, 1:15 am = ((1,25-4)+24)*60 = 1275
		int dawn_start_time;
		int dawn_end_time;
		int dusk_start_time;
		int dusk_end_time;
		int sunrise_time;
		int sunset_time;

		int dawn_start_time_defaultvalue;
		int dawn_end_time_defaultvalue;
		int dusk_start_time_defaultvalue;
		int dusk_end_time_defaultvalue;
	}TimeSettings;

	typedef struct ScreenSettings
	{
		//	Intensity in percentage
		int screen_brightness_day;
		int screen_brightness_eve;
		int screen_brightness_night;
		int screen_brightness_theatre;
		int screen_brightness_off;
		int screen_brightness_manual;

		int screen_brightness_day_defaultvalue;
		int screen_brightness_eve_defaultvalue;
		int screen_brightness_night_defaultvalue;
		int screen_brightness_theatre_defaultvalue;

		//	Time in minutes
		int screen_timeout;
		int screen_timeout_defaultvalue;
	}ScreenSettings;


public:
	//	Data
	vector_class<ResetGroup> reset_groups_;
	bool has_master_heating_;
	TimeSettings timesettings_;
	ScreenSettings screensettings_;

	//	Constructors and destructor
	Settings();
	~Settings();

	//	Initializers
	void InitValues();

	//	Getters and setters
	void SetDawnStartTime(const int new_value);
	int GetDawnStartTime();
	int GetDawnStartNormalizedTime();
	void SetDawnEndTime(const int new_value);
	int GetDawnEndTime();
	int GetDawnEndNormalizedTime();
	void SetDuskStartTime(const int new_value);
	int GetDuskStartTime();
	int GetDuskStartNormalizedTime();
	void SetDuskEndTime(const int new_value);
	int GetDuskEndTime();
	int GetDuskEndNormalizedTime();

	void SetSunriseTime(const int new_value);
	int GetSunriseTime();
	int GetSunriseNormalizedTime();
	void SetSunsetTime(const int new_value);
	int GetSunsetTime();
	int GetSunsetNormalizedTime();



	//Value manipulation functions
	int IncreaseScreenBrightness(const int profile);
	int DecreaseScreenBrightness(const int profile);
	int GetScreenBrightness(const int profile);
	int IncreaseScreenTimeout();
	int DecreaseScreenTimeout();
	int IncreaseDawnStartTime();
	int DecreaseDawnStartTime();
	int IncreaseDawnEndTime();
	int DecreaseDawnEndTime();
	int IncreaseDuskStartTime();
	int DecreaseDuskStartTime();
	int IncreaseDuskEndTime();
	int DecreaseDuskEndTime();

	void ResetDefaults();
};

#endif	//CMS_SETTINGS_H_
