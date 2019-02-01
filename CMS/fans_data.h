#ifndef CMS_FANS_H_
#define CMS_FANS_H_

#define MAX_FAN_INTENSITY (100)
#define MIN_FAN_INTENSITY (0)

#include <kanzi/kanzi.hpp>
#include "vector_class.h"

using namespace kanzi;


class Fans
{
//========================================================================================
//		Data structure
//========================================================================================
public:
	typedef enum FanType
	{
		BACKGROUND = 0,
		DAYBOOST = 1,
		NIGHTBOOST = 2
	} FanType;

	typedef struct FanData
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

		//	Intensities
		unsigned char level;
		unsigned char default_level;

		FanType type;

	}FanData;

	typedef struct FansGroup
	{
		//	ID
		unsigned char id;

		//	Name on screen
		string name;

		//	Vector of fans
		vector_class<Fans::FanData> fans;

		struct itimerspec fangroupboost_timer;
		timer_t fangroupboost_timerid;            // timer ID for timer
		struct sigevent fangroupboost_event;      // event to deliver

	}FansGroup;

//========================================================================================
//		Variables
//========================================================================================
public:
	vector_class<FansGroup> fansgroups_;

//========================================================================================
//		Methods
//========================================================================================

public:

	Fans();
	~Fans();

	int InitValues();

	int SetFanValue(const unsigned char group_index, const unsigned char fan_index, const unsigned char new_level);
	unsigned char GetFanValue(const unsigned char group_index, const unsigned char fan_index);

	int IncreaseFanLevel(const unsigned char group_index, const unsigned char fan_index);
	int DecreaseFanLevel(const unsigned char group_index, const unsigned char fan_index);
	void ResetDefaults();
};



#endif
