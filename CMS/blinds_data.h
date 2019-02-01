

#ifndef CMS_BLINDS_H_
#define CMS_BLINDS_H_

#include <kanzi/kanzi.hpp>
#include "vector_class.h"

using namespace kanzi;

class Blinds
{
public:
	typedef struct BlindsData
	{
		//	Id
		unsigned char id;

		//	Name on screen
		string name;

		//	GroupId
		unsigned char group;

		//	If widget will refer to opening or closing the whole group
		bool is_group;

		//	Message information
		unsigned char open_message_instance;
		unsigned char open_frame_position;
		unsigned char close_message_instance;
		unsigned char close_frame_position;

	}BlindsData;

	typedef struct BlindsGroup
	{
		//	Id
		unsigned char id;
		//	Name on screen
		string name;
		//	Vector of blinds
		vector_class<BlindsData> blinds;

	}BlindsGroup;
private:




public:

	vector_class<BlindsGroup> blindsgroups_;

	//	Constructors and destructor
	Blinds();
	~Blinds();

	//	Initializer
	void InitValues();

	//	Vector manipulation functions
	int GetBlindsGroup(const unsigned char group_index, Blinds::BlindsGroup *blindsgroup);
	unsigned int GetBlindsGroupsCount();

	//	Trigger handling functions
	int OpenBlinds(const unsigned char group_index, const unsigned char blinds_index);
	int OpenBlindsStop(const unsigned char group_index, const unsigned char blinds_index);
	int CloseBlinds(const unsigned char group_index, const unsigned char blinds_index);
	int CloseBlindsStop(const unsigned char group_index, const unsigned char blinds_index);

};

#endif	//CMS_BLINDS_H_
