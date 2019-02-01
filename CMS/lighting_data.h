#ifndef CMS_LIGHTING_H_
#define CMS_LIGHTING_H_

#define MAX_LIGHT_INTENSITY (100)
#define MIN_LIGHT_INTENSITY (0)

#define MAX_GROUP_INTENSITY (200)
#define MIN_GROUP_INTENSITY (-100)

#include <kanzi/kanzi.hpp>
#include <time.h>
#include <sys/siginfo.h>
#include "vector_class.h"

using namespace kanzi;

class Lighting
{
public:
	typedef enum Profile
	{
		DAY = 0,
		EVE = 1,
		NIGHT = 2,
		THEATRE = 3,
		SHOWER = 4,
		MANUAL = 5,
		OFF = 6
	} Profile;

	typedef struct LightData
	{
		//	Internal ID
		unsigned char id;

		//	Name on screen
		string name;

		//	Control information
		unsigned char group;

		//	Message information
		unsigned char message_instance;
		unsigned char frame_position;

		//	Intensity values
		int day_value;
		int eve_value;
		int night_value;
		int theatre_value;
		int shower_value;
		int manual_value;

		//	Default intensity values
		unsigned char day_defaultvalue;
		unsigned char eve_defaultvalue;
		unsigned char night_defaultvalue;
		unsigned char theatre_defaultvalue;
		unsigned char shower_defaultvalue;

	}LightData;

	typedef struct LightGroup
	{
		//	ID
		unsigned char id;

		//	Name on screen
		string name;

		//	Profiles available
		bool has_theatre;
		bool has_shower;

		//	Current profile
		Profile profile;

		//	Vector of lights
		vector_class<Lighting::LightData> lights;

		unsigned char related_fangroup;

		struct itimerspec lightgroup_timer;
		timer_t lightgroup_timerid;            // timer ID for timer
		struct sigevent lightgroup_event;      // event to deliver

	}LightGroup;

	typedef struct LightSwitch
	{
		//	ID
		unsigned char id;

		bool value;

		bool has_master;

		//	Name on screen
		string name;

		//	Message information
		unsigned char message_instance;
		unsigned char frame_position;

	}LightSwitch;


	LightSwitch master_switch_;

	//	Vector of light groups
	vector_class<LightGroup> lightgroups_;

	//	Night bit information
	unsigned char nightbit_message_instance_;
	unsigned char nightbit_frame_position_;

private:





public:
	//Constructors and destructor
	Lighting();
	~Lighting();

	//	Initializers
	void InitValues();

	//Getters and setters
	void SetMasterSwitch(const bool newValue);
	bool GetMasterSwitch();
	void SetProfile(const unsigned char group_index, const Lighting::Profile new_value);
	Lighting::Profile GetProfile(const unsigned char group_index);
	int GetLightValue(const unsigned char group_index, const unsigned char light_index);

	//	Light intensity manipulators
	int IncreaseLight(const unsigned char group_index, const unsigned char light_index);
	int DecreaseLight(const unsigned char group_index, const unsigned char light_index);
	int IncreaseGroupLight(const unsigned char group_index);
	int DecreaseGroupLight(const unsigned char group_index);

	void ResetDefaults();

};

#endif	//CMS_LIGHTING_H_
