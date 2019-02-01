#ifndef CMS_CLIMATE_H_
#define CMS_CLIMATE_H_

#include <kanzi/kanzi.hpp>
#include "vector_class.h"

#define MIN_TEMPERATURE 10
#define MAX_TEMPERATURE 30

using namespace kanzi;

class Climate
{
//========================================================================================
//		Data structure
//========================================================================================
private:

public:
	typedef struct PowerSwitch
	{
		//	TRUE = ON / FALSE = OFF
		bool switch_value;

		//	Name on screen
		string name;

		//	Control information
		unsigned char group;
		unsigned char id;

		//	Message information
		unsigned char message_instance;
		unsigned char frame_position;
	}PowerSwitch;

	typedef enum Mode
	{
		OFF = 1,
		DEHUMIDIFY = 2,
		AUTO = 3,
		HEATINGONLY = 4,
		COOLONLY = 5,
		AUTOWAUXHEAT = 6,
		AUXHEAT = 7
	} Mode;

	typedef struct Aircon
	{
		string name;
		Mode mode;
		Mode lastactive_mode;
		PowerSwitch power_switch;
		unsigned char temperature;
		unsigned char temperature_defaultvalue;
		unsigned char fan_speed;
		unsigned char ambient_temperature;
		bool showing_setpoint_;

		//	Message information
		unsigned int ambient_temperature_in_hash;
		unsigned int temperature_in_hash;
		unsigned int temperature_out_hash;
		unsigned int fan_speed_in_hash;
		unsigned int fan_speed_out_hash;
		unsigned int mode_in_hash;
		unsigned int mode_out_hash;

		struct itimerspec showsetpoint_timer;
		timer_t showsetpoint_timerid;            // timer ID for timer
		struct sigevent showsetpoint_event;      // event to deliver
	}Aircon;

	typedef struct Heating
	{
		string name;
		unsigned char id;
		unsigned char temperature;
		unsigned char temperature_defaultvalue;
		vector_class<PowerSwitch> power_switches;

		//	Setpoint message information
		unsigned char message_instance;
		unsigned char frame_position;
	}Heating;

//========================================================================================
//		Variables
//========================================================================================
public:
	Aircon aircon_;
	vector_class<Heating> heating_;
	bool master_heatingenable_;
	bool has_master_switch_;

//========================================================================================
//		Methods
//========================================================================================
private:


public:
	//Constructors and destructor
	Climate();
	~Climate();

	//	Initializers
	void InitValues();

	//Getters and setters
	void SetAirconTemperature(const unsigned char new_value);
	unsigned char GetAirconTemperature();
	void SetAirconAmbientTemperature(const unsigned char new_value);
	unsigned char GetAirconAmbientTemperature();
	void SetAirconPower(const bool new_value);
	bool GetAirconPower();
	void SetAirconFanSpeed(const unsigned char new_value);
	unsigned char GetAirconFanSpeed();
	void SetAirconMode(const Climate::Mode new_value);
	Climate::Mode GetAirconMode();

	void SetHeatingTemperature(const unsigned char group_index, const unsigned char new_value);
	unsigned char GetHeatingTemperature(const unsigned char group_index);
	void SetHeatingPower(const unsigned char group_index, const unsigned char switch_index, const bool new_value);
	bool GetHeatingPower(const unsigned char group_index, const unsigned char switch_index);

	//Value manipulation functions
	unsigned char IncreaseAirconTemperature();
	unsigned char DecreaseAirconTemperature();
	unsigned char ChangeAirconFanSpeed();
	bool SwitchAirconPower();
	Climate::Mode ChangeAirconMode();

	unsigned char IncreaseHeatingTemperature(const unsigned char group_index);
	unsigned char DecreaseHeatingTemperature(const unsigned char group_index);
	bool SwitchHeatingPower(const unsigned char group_index, const unsigned char switch_index);

	void ResetDefaults();
	void TurnOffHeating();
};

#endif	//CMS_CLIMATE_H_
