#ifndef CMS_INTERFACEINIT_H_
#define CMS_INTERFACEINIT_H_

#include <kanzi/kanzi.hpp>
#include <time.h>
#include <sys/neutrino.h>
#include <pthread.h>

#include "settings_ui.h"
#include "climate_ui.h"
#include "lighting_ui.h"
#include "blinds_ui.h"
#include "fans_ui.h"
#include "common.h"
#include "cms_data.h"
#include "vector_class.h"

using namespace kanzi;

class CmsUI
{
private:

	//========================================================================================
	//		Variables
	//========================================================================================

	//	Custom properties
	IntDynamicPropertyTypeSharedPtr menubutton_pageindex_;

	PageSharedPtr lighting_page_;
	PageSharedPtr blinds_page_;
	PageSharedPtr climate_page_;
	PageSharedPtr fans_page_;
	PageSharedPtr settings_page_;
	PageSharedPtr menu_page_;
	Button2DSharedPtr screen_cover_;
	StackLayout2DSharedPtr button_stack_;
	TextBlock2DSharedPtr clocktext_widget_;
	vector_class<ToggleButton2DSharedPtr> menu_buttons_;

	LightingInterface lighting_ui_;
	BlindsInterface blinds_ui_;
	FansInterface fans_ui_;
	ClimateInterface climate_ui_;
	SettingsInterface settings_ui_;

	int control_coid_;
	bool is_sleeping_;



	//========================================================================================
	//		Methods
	//========================================================================================

	//	Event response methods
	void MenuButtonPressed(ButtonConcept::ClickedMessageArguments& argument);
	void EntrypageBoxPressed(ButtonConcept::ClickedMessageArguments& argument);
	void ScreenCoverPressed(ButtonConcept::ClickedMessageArguments& argument);


private:
	static CmsUI *instance;

	pthread_mutex_t lighting_ui_mutex_;
	pthread_mutex_t clock_mutex_;
	pthread_mutex_t climate_ui_mutex_;
	pthread_mutex_t fans_ui_mutex_;
	pthread_mutex_t settings_ui_mutex_;

public:
	static CmsUI * Instance();

	//========================================================================================
	//		Variables
	//========================================================================================

	//	Pointer to pages
	vector_class<int> menu_pages_;


	//========================================================================================
	//		Methods
	//========================================================================================

	CmsUI();
	~CmsUI();

	//	Initializers
	void InitInterface(ScreenSharedPtr screen);
	int CommConnect();
	void UpdateClock_Request(time_t gps_time);
	void UpdateClock_Check();
	void WakeUpScreen_Request();
	void WakeUpScreen_Check();
	void ShutOffScreen_Request();
	void ShutOffScreen_Check();

	bool GetSleepStatus();


	//	Screen Update requests
	void UpdateAll_Request();
	void UpdateAll_Check();

	//	Lighting
	void UpdateProfile_Request(const unsigned char cms_id, const unsigned char group_index);
	void UpdateProfile_Check();
	void UpdateLightValue_Request(const unsigned char cms_index, const unsigned char group_index, const unsigned char light_index);
	void UpdateLightValue_Check();

	//	Climate
	void UpdateAirconPower_Request();
	void UpdateAirconPower_Check();
	void UpdateAirconTemperature_Request();
	void UpdateAirconTemperature_Check();
	void UpdateAirconAmbientTemperature_Request();
	void UpdateAirconAmbientTemperature_Check();
	void UpdateAirconSetPointTemperature_Request();
	void UpdateAirconSetPointTemperature_Check();
	void UpdateAirconMode_Request();
	void UpdateAirconMode_Check();
	void UpdateAirconFanSpeed_Request();
	void UpdateAirconFanSpeed_Check();
	void UpdateHeatingPower_Request(const unsigned char group_index, const unsigned char switch_index);
	void UpdateHeatingPower_Check();
	void UpdateAllHeatingPower_Request();
	void UpdateAllHeatingPower_Check();
	void UpdateHeatingTemperature_Request(const unsigned char group_index);
	void UpdateHeatingTemperature_Check();

	//	Fans
	void UpdateFanValue_Request(const unsigned char group_index, const unsigned char fan_index);
	void UpdateFanValue_Check();

	//	Settings
	void UpdateScreenBrightness_Request();
	void UpdateScreenBrightness_Check();
	void UpdateScreenTimeout_Request();
	void UpdateScreenTimeout_Check();
	void UpdateDawnStartTime_Request();
	void UpdateDawnStartTime_Check();
	void UpdateDawnEndTime_Request();
	void UpdateDawnEndTime_Check();
	void UpdateDuskStartTime_Request();
	void UpdateDuskStartTime_Check();
	void UpdateDuskEndTime_Request();
	void UpdateDuskEndTime_Check();
	void UpdateSunriseTime_Request();
	void UpdateSunriseTime_Check();
	void UpdateSunsetTime_Request();
	void UpdateSunsetTime_Check();
	void UpdateHeatingEnable_Request();
	void UpdateHeatingEnable_Check();

private:
	// NB. These XXX_Action methods must only be called on kanzi thread
	void WakeUpScreen_Action();
	void ShutOffScreen_Action();
	void UpdateProfile_Action(const unsigned char cms_id, const unsigned char group_index);
	void UpdateAirconTemperature_Action();
	void UpdateLightValue_Action(const unsigned char cms_index, const unsigned char group_index, const unsigned char light_index);
	void UpdateScreenBrightness_Action();
	void UpdateHeatingTemperature_Action(const unsigned char group_index);
	void UpdateHeatingPower_Action(const unsigned char group_index, const unsigned char switch_index);
	void UpdateAllHeatingPower_Action();
	void UpdateFanValue_Action(const unsigned char group_index, const unsigned char fan_index);
	void UpdateScreenTimeout_Action();
	void UpdateDawnStartTime_Action();
	void UpdateDawnEndTime_Action();
	void UpdateDuskStartTime_Action();
	void UpdateDuskEndTime_Action();
	void UpdateSunriseTime_Action();
	void UpdateSunsetTime_Action();
	void UpdateHeatingEnable_Action();
	void UpdateAirconSetPointTemperature_Action();
	void UpdateAirconPower_Action();
	void UpdateAirconMode_Action();
	void UpdateAirconFanSpeed_Action();
	void UpdateAirconAmbientTemperature_Action();
	void UpdateAll_Action();
	void UpdateClock_Action(time_t gps_time);
};


#endif
