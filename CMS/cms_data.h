#ifndef CMS_CMSDATA_H_
#define CMS_CMSDATA_H_

#include <netinet/in.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/siginfo.h>
#include <pthread.h>

#include "lighting_data.h"
#include "blinds_data.h"
#include "climate_data.h"
#include "settings_data.h"
#include "fans_data.h"
#include "common.h"
#include "vector_class.h"

// Keep same as CMS v1.0
#define EMPIRBUS_ADDRESS 0x0F
#define EMPIRBUS_MASK    0x1CFF0000
#define EMPIRBUS_ID      0x1CFF0000

/*!
*  \brief		CmsData class
*  \details
	The CmsData class is responsible for maintaining the application data, and
	changing it upon request.
	It holds the data that is displayed in the UI through instances of more specialised
	data classes (for Lighting, Blinds, Climate, Fans and Settings) as well as
	other necessary underlying information, like hostnames, IDs, master/slave informaton.

	It can receive two kinds of requests, being both made via method call:

	1 - State change request (from CmsControl) - After receiving a state change
	request, it calls methods from the specialized class for which the change was
	requested (increasing a light value, switching on the aircon, etc) where
	plausibility and range checks are performed, and then it generates a new
	request via Qnet message for the CmsControl class to forward the new data
	to the correct destination (CAN controller, ethernet controller, another CMS).

	2 - Data query (from CmsUI) - In order to display the data, the CmsUI makes
	query requests to CmsData (which are then, again, forwarded to the specialized
	class) for retrieving it's current state.

	It is valid to emphasise that state change methods will always and only be
	called by CmsControl, while data query methods will always and only be called
	by CmsUI. It plays the "Model" role in the MVC Design Pattern.

	As in CmsControl and CmsUI, it uses the concept of Singleton, declared
	dynamically. Which means it persists in the heap during the whole application
	lifecycle.
*  \author		John Reis
*  \version   	1.0
*  \date      	Feb-2016
*  \pre       	Has to be the first to be initialised (altough it's CommConnect
				method must me called just after CmsControl is initialised).
*  \warning   	Improper use can crash your application
*  \copyright 	Skyships Automotive Ltd.
*/
class CmsData
{
	//! Constructor
    /*! Constructor for CmsData class.*/
	CmsData();

	//! Destructor
    /*! Destructor for CmsData class.*/
	~CmsData();

public:
	//! Configuration data structure for each CMS
    /*! This structure holds an instance of each specialized data class (Lighting,
	Blinds, Climate, Fans and Settings), as well as information needed for
	communication. All the information is initially retrieve from the configuration
	file (cms_config.xml).*/
	typedef struct CmsConfig
	{
		//! CMS ID
        /*! The ID retrieved from the config file, is unique for each CMS. */
		unsigned char id;

		//! CMS Label
        /*! The name that will be displayed on the UI. */
		string label;

		//! Master flag
        /*! True if this CMS is the master. */
		bool is_master;

		string ip_addr;

		int udp_port;

		//! Qnet hostname
        /*! This will be the published name of the CmsControl service inside the
		Qnet environment, it is through this name that connections to other blocks
 		will be established (CmsUI, CmsData or another CMS CmsControl service). */
		string hostname;

		//	Name for calculating the MMS commands hash (compatibility with CMS1)
		string mmshash_name;

		int control_coid;

		pthread_mutex_t control_mutex;

		//! Lighting data
		/*! This instance of the Lighting class will hold data and give access to
		the	state change / query methos related to Lighting, for the CMS with the
		given ID above. */
		Lighting lighting_;

		//! Blinds data
		/*! This instance of the Blinds class will hold data and give access to
		the	query methos related to Blinds, for the CMS with the given ID above.
		This class don't have state changing methods, as the only data it holds
		is related to UI names and communication.*/
		Blinds blinds_;

		//! Climate data
		/*! This instance of the Climate class will hold data and give access to
		the	state change / query methos related to Aircon and Heating, for the CMS
		with the given ID above. */
		Climate climate_;

		//! Fans data
		/*! This instance of the Fans class will hold data and give access to
		the	state change / query methos related to Fans, for the CMS with the
		given ID above. */
		Fans fans_;

		//! Settings data
		/*! This instance of the Settings class will hold data and give access to
		the	state change / query methos related to Settings, for the CMS with the
		given ID above. */
		Settings settings_;

	}CmsConfig;

	typedef struct CanMessage
	{
		unsigned int msg_id;
		char msg_data[8];
	}CanMessage;

private:
	//! Singleton instance
    /*! This class uses the concept of Singleton. By having a pointer to itself
    and being accessed only by the Instance() method, it guarantees the existence
    of only one instance, that can be accessed "globally" (if the CmsData.h is
    included). */
	static CmsData *instance;

	struct itimerspec screentimeout_timer;
	timer_t screentimeout_timerid;            // timer ID for timer
	struct sigevent screentimeout_event;      // event to deliver

public:
	//! Connection flag
    /*! True in case the connection to the CmsControl class is alive. */
	bool is_connected_;

	//! Connection ID
    /*! Unique ID for the connection to the CmsControl class service. */
	int control_coid_;
	int can_coid_;

	//! It's own CMS ID
    /*! This ID will be used to recognise wether a received message is intended
	to change the state of this CMS, as well as writing the "sender" field of
	messages. */
	unsigned char self_id_;

	//! The master's CMS ID
    /*! This ID will be used to recognise wether a received message is coming from
	the master, as well as writing the "recipient" field of messages, whenever
	the message is intended to reach the master CMS.  */
	unsigned char master_id_;

	//! The CAN controller ID
    /*! This ID will be used to recognise wether a received message is coming from
	the CAN controller, as well as writing the "recipient" field of messages, whenever
	the message is intended to reach the CAN controller.  */
	unsigned char can_id_;

	//! The MMS ID
    /*! This ID will be used to recognise wether a received message is coming from
	the ethrenet controller (MMS), as well as writing the "recipient" field of messages, whenever
	the message is intended to reach the ethernet controller (and ultimatelythe mms).  */
	unsigned char mms_id_;

	string mms_ipaddr_;

	int mms_port_;

	string self_ipaddr_;

	int self_port_;

	bool all_on_;
	bool all_off_;

	//! CAN controller Qnet hostname
	/*! This hostname will be the name the CAN controller service will publish
	to Qnet, it will be used by CmsControl service to send messages to the CAN.  */
	string can_hostname_;

	//! Master CMS CmsControl service hostname
	/*! In case this is a slave CMS, this will be the hostname used to send messages
	to the master CMS.  */
	string master_hostname_;

	//! This CMS data
	/*! A CmsConfig data structure referred to the configuration of this CMS.  */
	CmsConfig cms_self_;

	//! The slaves CMS data
	/*! In case this is a master CMS, it populates this array of all the configuration
	data structure of it's slaves. */
	vector_class<CmsConfig> cms_slaves_;

	//! Array of menu pages
	/*! Used to avoid unnecessary data initialisation, being filled up before the
	specialised data classes are called. */
	vector_class<int> menu_pages_;

	vector_class<CanMessage> can_messages_;

	bool change_flag_;

	bool is_night_;

	int minutes_of_day_;

	struct itimerspec saveparams_timer_;
	timer_t saveparams_timerid_;            // timer ID for timer
	struct sigevent saveparams_event_;      // event to deliver

//========================================================================================
//		Control methods
//========================================================================================
	//! Singleton instance retriever
    /*! Calling this method returns either a new instance of the CmsData class
    (if none exists), or the existing one (if it does). It guarantees that the
    whole application will access always the same instance of the CmsControl class.

    \return A new instance to CmsControl class allocated dynamically (if none exists),
    		or the existing one (if it does)*/
	static CmsData * Instance();

	//! Connection to the CmsControl service
	/*! This method is responsible for establishing a connection to the CmsControl
	service over Qnet. It invokes the name_open method, passing the hostname as a
	paramenter. The Qnx Neutrino GNS server will be called to resolve the location
	of the hostname, and then an unique connection ID will be retrieved.

	\return An unique connection ID in case the connection succeeds, -1 otherwise.*/
	int CommConnect();

	//! Sends a Qnet message to the CmsControl service
	/*! This method makes use of the QNX Neutrino MsgSend function to send the
	char array received as parameter to the CmsControl service, through the
	connection ID retrieved from CommConnect.

	\param msg A pointer to an 8 byte char array, containing the message to be sent.
	\return EOK if the message was received and replied successfully by CmsControl,
			-1 otherwise.*/
	int SendControlMessage(const unsigned char *msg);
	int SendCanMessage(const unsigned char *msg);

	//! Reads data from the configuration file
	/*! This method makes calls to XmlData methods in order to read data from
	the configuration file (cms_config.xml) and populate all the specialised
	data classes with the application initial state and default values.

	\return 0 in case the file was valid and the values were read correctly,
			-1 otherwise.*/
	int InitValues();

	int LoadCanMessages();
	int LoadMmsCommands();

	int InitScreenTimer();
	int UpdateScreenTimer();
	int RestartScreenTimer();
	int InitControlTimers();

//========================================================================================
//		State change methods
//========================================================================================
	//! Increases light intensity value and send the new value to the CAN controller
	/*! This method forwards the request to the Lighting class, specifying which
	light is being requested to be increased. The Lighting class will perform
	validity and range check and change the value if possible. The method will then
	mount and send two messages containing the new value to the CmsControl, one
	to be forwarded to the CAN controller (with the CAN message instance and frame
	position information gathered from the Lighting class, for the specified light),
	and the second to be forwarded to the master CMS (in case this is a slave) or
	to the slave (in case this is a master, and the light changed belongs to a slave).

	\param 	cms_id To which CMS belongs the light that is to be changed (will always
			have the same value of self_id_, in case this is a slave).
	\param	group_index To which group belongs the light that is to be changed
	\param 	light_index Which light is to be changed
	\return 0 in case the light was changed and the messages were sent successfully,
			-1 otherwise.*/
	int IncreaseLight(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index, const bool send_messages);

	//! Decreases light intensity value and send the new value to the CAN controller
	/*! This method forwards the request to the Lighting class, specifying which
	light is being requested to be decreased. The Lighting class will perform
	validity and range check and change the value if possible. The method will then
	mount and send two messages containing the new value to the CmsControl, one
	to be forwarded to the CAN controller (with the CAN message instance and frame
	position information gathered from the Lighting class, for the specified light),
	and the second to be forwarded to the master CMS (in case this is a slave) or
	to the slave (in case this is a master,and the light changed belongs to a slave).

	\param 	cms_id To which CMS belongs the light that is to be changed (will always
			have the same value of self_id_, in case this is a slave).
	\param	group_index To which group belongs the light that is to be changed
	\param 	light_index Which light is to be changed
	\return 0 in case the light was changed and the messages were sent successfully,
			-1 otherwise.*/
	int DecreaseLight(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index, const bool send_messages);

	//! Changes the light profile and send the new values to the CAN controller
	/*! This method forwards the request to the Lighting class, specifying for which
	group and to which profile the change is to be made. The Lighting class will
	perform validity check and change the value if possible. The method will then
	mount and send one message (for each light of the group) containing the new
	value to the CmsControl, one to be forwarded to the CAN controller (with the
	CAN message instance and frame position information gathered from the Lighting
	class, for each individual light), and then finally sends a message to be
	forwarded to the master CMS (in case this is a slave) or to the slave (in case
	this is a master,and the group changed belongs to a slave), informing that
	the profiule was changed.

	It is valid to emphasise that for each light belonging to the group, a separate
	message will be sent to the CAN controller (and therefore to EmpirBus).

	\param 	cms_id To which CMS belongs the group that is to be changed (will always
			have the same value of self_id_, in case this is a slave).
	\param	group_index Which group is to have it's profile changed
	\param 	new_value To which profile the change is to be done
	\return 0 in case the profile was changed and the messages were sent successfully,
			-1 otherwise.*/
	int SetProfile(const unsigned char cms_id, const unsigned char group_index, const Lighting::Profile new_value, const bool send_messages);

	//! Increases the light intensity value of a whole group and send the new values to the CAN controller
	/*! This method forwards the request to the Lighting class, specifying which
	group of lights is being requested to be increased. The Lighting class will
	perform validity and range check, then change the values if possible. The method
	will then mount and send two messages (for each light of the group) containing
	the new value to the CmsControl, one to be forwarded to the CAN controller,
	and the second to be forwarded to the master CMS (in case this is a slave) or
	to the slave (in case this is a master, and the lights changed belong to a
	slave).
	This method will never be requested by the UI controller, it's invoking can
	only come from the CAN controller, meaning a physical switch was activated.

	It is valid to emphasise that for each light belonging to the group, a separate
	message will be sent to the CAN controller (and therefore to EmpirBus).

	It is also valid to highlight that, the Lighting class method for increasing a
	whole group has a special range check, in order to maintain the same offsets
	between lights whenever one or more lights of the group reaches top or bottom
	scale.

	\param 	cms_id To which CMS belongs the lights that are to be changed (will
			always have the same value of self_id_, in case this is a slave).
	\param	group_index Which group is to have it's lights changed
	\return 0 in case the lights were changed and the messages were sent successfully,
			-1 otherwise.*/
	int IncreaseGroupLight(const unsigned char cms_id, const unsigned char group_index, const bool send_messages);

	//! Decreases the light intensity value of a whole group and send the new values to the CAN controller
	/*! This method forwards the request to the Lighting class, specifying which
	group of lights is being requested to be decreased. The Lighting class will
	perform validity and range check, then change the values if possible. The method
	will then mount and send two messages (for each light of the group) containing
	the new value to the CmsControl, one to be forwarded to the CAN controller,
	and the second to be forwarded to the master CMS (in case this is a slave) or
	to the slave (in case this is a master, and the lights changed belong to a
	slave).
	This method will never be requested by the UI controller, it's invoking can
	only come from the CAN controller, meaning a physical switch was activated.

	It is valid to emphasise that for each light belonging to the group, a separate
	message will be sent to the CAN controller (and therefore to EmpirBus).

	It is also valid to highlight that, the Lighting class method for decreasing a
	whole group has a special range check, in order to maintain the same offsets
	between lights whenever one or more lights of the group reaches top or bottom
	scale.

	\param 	cms_id To which CMS belongs the lights that are to be changed (will
			always have the same value of self_id_, in case this is a slave).
	\param	group_index Which group is to have it's lights changed
	\return 0 in case the lights were changed and the messages were sent successfully,
			-1 otherwise.*/
	int DecreaseGroupLight(const unsigned char cms_id, const unsigned char group_index, const bool send_messages);

	int StartIncreaseGroupTimer(const unsigned char cms_id, const unsigned char group_index);
	int StartDecreaseGroupTimer(const unsigned char cms_id, const unsigned char group_index);
	int StopGroupTimer(const unsigned char cms_id, const unsigned char group_index);

	//! Send a message to the CAN controller requesting to start opening the specified blinds
	/*! This method mounts the message (with the CAN message instance and frame
	position information gathered from the Blinds class, for the specified blinds),
	and send it to CmsControl to be forwarded to the CAN controller.

	\param 	group_index To which group belongs the blinds to start opening
	\param	blinds_index Which blinds is to to start opening
	\return 0 in case the message was sent successfully, -1 otherwise.*/
	int OpenBlinds(const unsigned char group_index, const unsigned char blinds_index);

	//! Send a message to the CAN controller requesting to stop opening the specified blinds
	/*! This method mounts the message (with the CAN message instance and frame
	position information gathered from the Blinds class, for the specified blinds),
	and send it to CmsControl to be forwarded to the CAN controller.

	\param 	group_index To which group belongs the blinds to stop opening
	\param	blinds_index Which blinds is to to stop opening
	\return 0 in case the message was sent successfully, -1 otherwise.*/
	int OpenBlindsStop(const unsigned char group_index, const unsigned char blinds_index);

	//! Send a message to the CAN controller requesting to start closing the specified blinds
	/*! This method mounts the message (with the CAN message instance and frame
	position information gathered from the Blinds class, for the specified blinds),
	and send it to CmsControl to be forwarded to the CAN controller.

	\param 	group_index To which group belongs the blinds to start closing
	\param	blinds_index Which blinds is to to start closing
	\return 0 in case the message was sent successfully, -1 otherwise.*/
	int CloseBlinds(const unsigned char group_index, const unsigned char blinds_index);

	//! Send a message to the CAN controller requesting to stop closing the specified blinds
	/*! This method mounts the message (with the CAN message instance and frame
	position information gathered from the Blinds class, for the specified blinds),
	and send it to CmsControl to be forwarded to the CAN controller.

	\param 	group_index To which group belongs the blinds to stop closing
	\param	blinds_index Which blinds is to to stop closing
	\return 0 in case the message was sent successfully, -1 otherwise.*/
	int CloseBlindsStop(const unsigned char group_index, const unsigned char blinds_index);

	//! Increases the aircon temperature and forward the message to the ethernet controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity and range check, then change the temperature value if possible.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the ethernet controller (and ultimately to the
	MMS).

	\return 0 in case the temperature was changed and the message was sent successfully,
			-1 otherwise.*/
	int IncreaseAirconTemperature();

	//! Decreases the aircon temperature and forward the message to the ethernet controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity and range check, then change the temperature value if possible.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the ethernet controller (and ultimately to the
	MMS).

	\return 0 in case the temperature was changed and the message was sent successfully,
			-1 otherwise.*/
	int DecreaseAirconTemperature();

	//! Changes the aircon fan speed and forward the message to the ethernet controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity check, then change the fan speed value, in case the aircon
	is in MANUAL mode.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the ethernet controller (and ultimately to the
	MMS).

	\return 0 in case the fan speed was changed and the message was sent successfully,
			-1 otherwise.*/
	int ChangeAirconFanSpeed();

	//! Switches the aircon power value and forward the message to the ethernet controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity check, then change the switch value.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the ethernet controller (and ultimately to the
	MMS).

	\return 0 in case the power switch was changed and the message was sent successfully,
			-1 otherwise.*/
	int SwitchAirconPower();

	//! Changes the aircon mode and forward the message to the ethernet controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity check, then change the aircon mode value.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the ethernet controller (and ultimately to the
	MMS).

	\return 0 in case the mode was changed and the message was sent successfully,
			-1 otherwise.*/
	int ChangeAirconMode();

	//! Increases the heating temperature and forward the message to the CAN controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity and range check, then change the temperature value if possible.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the CAN controller (and ultimately to EmpirBus).

	\return 0 in case the temperature was changed and the message was sent successfully,
			-1 otherwise.*/
	int IncreaseHeatingTemperature(const unsigned char group_index);

	//! Decreases the heating temperature and forward the message to the CAN controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity and range check, then change the temperature value if possible.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the CAN controller (and ultimately to EmpirBus).

	\return 0 in case the temperature was changed and the message was sent successfully,
			-1 otherwise.*/
	int DecreaseHeatingTemperature(const unsigned char group_index);

	//! Switches the specified heating power value and forward the message to the CAN controller
	/*! This method forwards the request to the Climate class, which will then
	perform validity check, then change the switch value.
	The method then will mount and send a message containing the new value to the
	CmsControl, to be forwarded to the CAN controller (and ultimately to EmpirBus).

	\param switch_index Which heating unit is to be switched on/off
	\return 0 in case the power switch was changed and the message was sent successfully,
			-1 otherwise.*/
	int SwitchHeatingPower(const unsigned char group_index, const unsigned char switch_index);

	//! Increases fan intensity value and send the new value to the CAN controller
	/*! This method forwards the request to the Fans class, specifying which
	fan is being requested to be increased. The Fans class will perform
	validity and range check and change the value if possible. The method will then
	mount and send the message containing the new value to the CmsControl,to be
	then forwarded to the CAN controller (with the CAN message instance and frame
	position information gathered from the Fans class, for the specified fan).

	\param	group_index To which group belongs the fan that is to be changed
	\param 	light_index Which fan is to be changed
	\return 0 in case the fan was changed and the messages were sent successfully,
			-1 otherwise.*/
	int IncreaseFanLevel(const unsigned char group_index, const unsigned char fan_index);

	//! Decreases fan intensity value and send the new value to the CAN controller
	/*! This method forwards the request to the Fans class, specifying which
	fan is being requested to be increased. The Fans class will perform
	validity and range check and change the value if possible. The method will then
	mount and send the message containing the new value to the CmsControl,to be
	then forwarded to the CAN controller (with the CAN message instance and frame
	position information gathered from the Fans class, for the specified fan).

	\param	group_index To which group belongs the fan that is to be changed
	\param 	light_index Which fan is to be changed
	\return 0 in case the fan was changed and the messages were sent successfully,
			-1 otherwise.*/
	int DecreaseFanLevel(const unsigned char group_index, const unsigned char fan_index);

	int BoostFanLevel(const unsigned char group_id, Fans::FanType type);
	int StartBoostFanTimer(const unsigned char group_id);
	int UnboostFanLevel(const unsigned char group_index);

	//! Reset all CMS values to the defaults
	/*! This method forwards the call for each specialised data, that will, in their
	turn, replace their current values for the default ones read from the config
	file. After the values are replaced, the method will then iterate mounting the
	messages to be sent to CmsControl. For each component that had it's value reset,
	a separate message will be sent and then forwarded to the CAN controller
	to be then sent to EmpirBus. After completion, another message will be sent,
	this time to be forwarded to each slave, informing that the values were reset
	so they can have their UI updated.

	This method has an option for updating all the CMSs, or just the slaves.

	\param	reset_all Set to TRUE if the reset is to be made in all CMSs (including
			the master).
	\return 0 in case the resets were performed and the messages successfully sent
			-1 otherwise.*/
	int ResetDefaults(const bool reset_all);
	int SendFuseReset();
	int SendMasterSwitch(const bool all_on);
	int CheckTime();
	//int SendNightBit(bool is_night);
	int SendNightBit(const unsigned char message_instance, const unsigned char frame_position, const bool is_night);
	int BroadcastOff();
	int TurnOffHeating();
	int RequestTurnOffHeating();
	int SendAllHeating();

	//! Increase the screen brightness
	/*! This method forwards the call for each specialised data, that will, in their
	turn, replace their current values for the default ones read from the config
	file. After the values are replaced, the method will then iterate mounting the
	messages to be sent to CmsControl. For each component that had it's value reset,
	a separate message will be sent and then forwarded to the CAN controller
	to be then sent to EmpirBus. After completion, another message will be sent,
	this time to be forwarded to each slave, informing that the values were reset
	so they can have their UI updated.

	This method has an option for updating all the CMSs, or just the slaves.

	\param	reset_all Set to TRUE if the reset is to be made in all CMSs (including
			the master).
	\return 0 in case the resets were performed and the messages successfully sent
			-1 otherwise.*/
	int IncreaseScreenBrightness();
	int DecreaseScreenBrightness();
	int SetScreenBrightness();
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



	int SendAllValues();
	int SaveAllValues();
	int SendLightSyncValues(const unsigned char cms_index);

	void SetAirconTemperature(const unsigned char new_temperature);
	void SetAirconAmbientTemperature(const unsigned char new_temperature);
	void SetAirconMode(const int new_mode);
	void SetAirconFanSpeed(const unsigned char new_fanspeed);
	void SetAirconPower(const bool new_power);
//========================================================================================
//		Query methods
//========================================================================================
	//	Lights methods
	Lighting::Profile GetLightProfile(const unsigned char cms_id, const unsigned char group_index);
	int GetLightValue(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index);
	int SyncLightValue(const unsigned char group_index, const unsigned char light_index, const unsigned char profile, const unsigned char value);

	//	Climate methods
	bool GetAirconPower();
	int GetAirconTemperature();
	int GetAirconAmbientTemperature();
	Climate::Mode GetAirconMode();
	unsigned char GetAirconFanSpeed();
	bool GetHeatingPower(const unsigned char group_index, const unsigned char switch_index);
	int GetHeatingTemperature(const unsigned char group_index);



	//	Fans methods
	int GetFansValue(const unsigned char group_index, const unsigned char fans_index);

	//	Settings methods
	int GetScreenBrightness();
	int GetScreenTimeout();
	int GetDawnStartTime();
	int GetDawnEndTime();
	int GetDuskStartTime();
	int GetDuskEndTime();
	int GetSunriseTime();
	int GetSunsetTime();
	int GetDawnStartNormalizedTime();
	int GetDawnEndNormalizedTime();
	int GetDuskStartNormalizedTime();
	int GetDuskEndNormalizedTime();
	int GetSunriseNormalizedTime();
	int GetSunsetNormalizedTime();
	int SwitchHeatingEnable(const bool enable);

	unsigned int CalculateMmsHash(string str);

};

#endif	//CMS_CMSDATA_H_
