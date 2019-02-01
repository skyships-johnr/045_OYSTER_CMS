#include "cms_ui.h"
#include "xml_data.h"

CmsUI *CmsUI::instance = 0;
//========================================================================================
//		Constructor and destructor
//========================================================================================

CmsUI::CmsUI()
{
	is_sleeping_= false;
	control_coid_ = -1;
}

CmsUI::~CmsUI()
{
	if (instance != 0)
        delete instance;
}

CmsUI * CmsUI::Instance()
{
	if (instance == 0)
        instance = new CmsUI;

    return instance;
}
//========================================================================================
//		Initializers
//========================================================================================

void CmsUI::InitInterface(ScreenSharedPtr screen)
{
	//	Get screen and pages nodes
	menu_page_ = screen->lookupNode<Page>("#menu_page");
	button_stack_ = menu_page_->lookupNode<StackLayout2D>("#button_stack");

	//	Get custom properties
	StringDynamicPropertyTypeSharedPtr menubutton_text = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("MenuButtonText"));
	menubutton_pageindex_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("MenuButtonPageIndex"));

	//	Get custom prefabs
	PrefabTemplateSharedPtr menubutton_widgetprefab = screen->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/MenuButton");

	PageSharedPtr entry_page = screen->lookupNode<Page>("#entry_page");
	lighting_page_ = screen->lookupNode<Page>("#lighting_page");
	blinds_page_ = screen->lookupNode<Page>("#blinds_page");
	climate_page_ = screen->lookupNode<Page>("#climate_page");
	fans_page_ = screen->lookupNode<Page>("#fans_page");
	settings_page_ = screen->lookupNode<Page>("#settings_page");

	//	Get node and set up clock
	//ClockCustomMessageSharedPtr clock_message = ClockCustomMessageSharedPtr(new DynamicMessageType<MessageDispatcherProperties::TimerMessageArguments>("UpdateClockMessage"));
	clocktext_widget_ = screen->lookupNode<TextBlock2D>("#clock_text");
	//entry_page->addMessageHandler(*clock_message, bind(&CmsUI::ClockTimerHandler, this, placeholders::_1));
	//UpdateClock();

	//	Clear the entry page
	Button2DSharedPtr lighting_box = entry_page->lookupNode<Button2D>("#lighting_box");
	Button2DSharedPtr blinds_box = entry_page->lookupNode<Button2D>("#blinds_box");
	Button2DSharedPtr climate_box = entry_page->lookupNode<Button2D>("#climate_box");
	Button2DSharedPtr fans_box = entry_page->lookupNode<Button2D>("#fans_box");

	lighting_box->setHitTestable(false);
	lighting_box->setVisible(false);
	blinds_box->setHitTestable(false);
	blinds_box->setVisible(false);
	climate_box->setHitTestable(false);
	climate_box->setVisible(false);
	fans_box->setHitTestable(false);
	fans_box->setVisible(false);

	screen_cover_ = screen->lookupNode<Button2D>("#screen_cover");
	screen_cover_->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::ScreenCoverPressed, this, placeholders::_1));
	screen_cover_->setHitTestable(false);
	screen_cover_->setVisible(false);
	is_sleeping_ = false;

	//	Clear the buttons vector
	menu_buttons_.clear();

	//	Populate the menu
	for (unsigned int page_it = 0; page_it < CmsData::Instance()->menu_pages_.size(); page_it++)
	{
		ToggleButton2DSharedPtr menu_button;
		switch (CmsData::Instance()->menu_pages_.at(page_it))
		{
		case LIGHTING_PAGE:
			//	Create menu entry
			menu_button = menubutton_widgetprefab->instantiate<ToggleButton2D>("MenuButtonLighting");
			menu_button->setProperty(*menubutton_text, "Lighting");
			menu_button->addResource(ResourceID("IconTexture"), "kzb://cms/Textures/icon_lightingcolor");
			menu_button->setProperty(*menubutton_pageindex_, LIGHTING_PAGE);
			menu_button->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::MenuButtonPressed, this, placeholders::_1));
			menu_button->setIndexInGroup(LIGHTING_PAGE);

			//	Push button into menu
			button_stack_->addChild(menu_button);

			//	Initialize page
			lighting_ui_.LightingInterfaceInit(lighting_page_);

			//	Sent entry page box visibility and click handle
			lighting_box->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::EntrypageBoxPressed, this, placeholders::_1));
			lighting_box->setHitTestable(true);
			lighting_box->setVisible(true);

			break;

		case BLINDS_PAGE:
			//	Create menu entry
			menu_button = menubutton_widgetprefab->instantiate<ToggleButton2D>("MenuButtonBlinds");
			menu_button->setProperty(*menubutton_text, "Blinds");
			menu_button->addResource(ResourceID("IconTexture"), "kzb://cms/Textures/icon_blindscolor");
			menu_button->setProperty(*menubutton_pageindex_, BLINDS_PAGE);
			menu_button->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::MenuButtonPressed, this, placeholders::_1));
			menu_button->setIndexInGroup(BLINDS_PAGE);

			//	Push button into menu
			button_stack_->addChild(menu_button);

			//	Initialize page
			blinds_ui_.BlindsInterfaceInit(blinds_page_);

			//	Sent entry page box visibility and click handle
			blinds_box->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::EntrypageBoxPressed, this, placeholders::_1));
			blinds_box->setHitTestable(true);
			blinds_box->setVisible(true);

			break;

		case CLIMATE_PAGE:
			//	Create menu entry
			menu_button = menubutton_widgetprefab->instantiate<ToggleButton2D>("MenuButtonClimate");
			menu_button->setProperty(*menubutton_text, "Climate");
			menu_button->addResource(ResourceID("IconTexture"), "kzb://cms/Textures/icon_climatecolor");
			menu_button->setProperty(*menubutton_pageindex_, CLIMATE_PAGE);
			menu_button->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::MenuButtonPressed, this, placeholders::_1));
			menu_button->setIndexInGroup(CLIMATE_PAGE);

			//	Push button into menu
			button_stack_->addChild(menu_button);

			//	Initialize page
			climate_ui_.ClimateInterfaceInit(climate_page_);

			//	Sent entry page box visibility and click handle
			climate_box->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::EntrypageBoxPressed, this, placeholders::_1));
			climate_box->setHitTestable(true);
			climate_box->setVisible(true);

			break;

		case FANS_PAGE:
			//	Create menu entry
			menu_button = menubutton_widgetprefab->instantiate<ToggleButton2D>("MenuButtonFans");
			menu_button->setProperty(*menubutton_text, "Fans");
			menu_button->addResource(ResourceID("IconTexture"), "kzb://cms/Textures/icon_fanscolor");
			menu_button->setProperty(*menubutton_pageindex_, FANS_PAGE);
			menu_button->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::MenuButtonPressed, this, placeholders::_1));
			menu_button->setIndexInGroup(FANS_PAGE);

			//	Push button into menu
			button_stack_->addChild(menu_button);

			//	Initialize page
			fans_ui_.FansInterfaceInit(fans_page_);

			//	Sent entry page box visibility and click handle
			fans_box->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::EntrypageBoxPressed, this, placeholders::_1));
			fans_box->setHitTestable(true);
			fans_box->setVisible(true);

			break;

		case SETTINGS_PAGE:
			//	Create menu entry
			menu_button = menubutton_widgetprefab->instantiate<ToggleButton2D>("MenuButtonSettings");
			menu_button->setProperty(*menubutton_text, "Settings");
			menu_button->addResource(ResourceID("IconTexture"), "kzb://cms/Textures/icon_settingscolor");
			menu_button->setProperty(*menubutton_pageindex_, SETTINGS_PAGE);
			menu_button->addMessageHandler(ButtonConcept::ClickedMessage, bind(&CmsUI::MenuButtonPressed, this, placeholders::_1));
			menu_button->setIndexInGroup(SETTINGS_PAGE);

			//	Push button into menu
			button_stack_->addChild(menu_button);

			//	Initialize page
			settings_ui_.SettingsInterfaceInit(settings_page_);

			break;
		}

	}
	// apply current screen brightness
	WakeUpScreen_Action();

	lighting_ui_mutex_ = PTHREAD_MUTEX_INITIALIZER;
	clock_mutex_ = PTHREAD_MUTEX_INITIALIZER;

}

int CmsUI::CommConnect()
{
	if((control_coid_ = name_open(CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL)) == -1)
		return -1;
	else
	{
		if(lighting_ui_.CommConnect() == -1)
			return -1;
		if(blinds_ui_.CommConnect() == -1)
			return -1;
		if(climate_ui_.CommConnect() == -1)
			return -1;
		if(fans_ui_.CommConnect() == -1)
			return -1;
		if(settings_ui_.CommConnect() == -1)
			return -1;

		return 1;
	}
}

void CmsUI::MenuButtonPressed(ButtonConcept::ClickedMessageArguments& argument)
{
	WakeUpScreen_Action();

	int page_index = argument.getSource()->getProperty(*menubutton_pageindex_);
	ScrollView2DSharedPtr scroll_view;
	switch (page_index)
	{
	case LIGHTING_PAGE:
		scroll_view = lighting_page_->lookupNode<ScrollView2D>("ScrollView");
		scroll_view->jumpToPosition(Vector2(0, 0));
		lighting_page_->navigate(false);
		break;

	case BLINDS_PAGE:
		scroll_view = blinds_page_->lookupNode<ScrollView2D>("ScrollView");
		scroll_view->jumpToPosition(Vector2(0, 0));
		blinds_page_->navigate(false);
		break;

	case CLIMATE_PAGE:
		scroll_view = climate_page_->lookupNode<ScrollView2D>("ScrollView");
		scroll_view->jumpToPosition(Vector2(0, 0));
		climate_page_->navigate(false);
		break;

	case FANS_PAGE:
		scroll_view = fans_page_->lookupNode<ScrollView2D>("ScrollView");
		scroll_view->jumpToPosition(Vector2(0, 0));
		fans_page_->navigate(false);
		break;

	case SETTINGS_PAGE:
		scroll_view = settings_page_->lookupNode<ScrollView2D>("ScrollView");
		scroll_view->jumpToPosition(Vector2(0, 0));
		settings_page_->navigate(false);
		break;
	}

}

void CmsUI::ScreenCoverPressed(ButtonConcept::ClickedMessageArguments& argument)
{
	WakeUpScreen_Action();
}

void CmsUI::EntrypageBoxPressed(ButtonConcept::ClickedMessageArguments& argument)
{
	WakeUpScreen_Action();

	int page_index = argument.getSource()->getProperty(*menubutton_pageindex_);
	ToggleButton2DSharedPtr menu_button;

	switch (page_index)
	{
	case LIGHTING_PAGE:
		menu_button = button_stack_->lookupNode<ToggleButton2D>("MenuButtonLighting");
		menu_button->setToggleState(true);
		break;

	case BLINDS_PAGE:
		menu_button = button_stack_->lookupNode<ToggleButton2D>("MenuButtonBlinds");
		menu_button->setToggleState(true);
		break;

	case CLIMATE_PAGE:
		menu_button = button_stack_->lookupNode<ToggleButton2D>("MenuButtonClimate");
		menu_button->setToggleState(true);
		break;

	case FANS_PAGE:
		menu_button = button_stack_->lookupNode<ToggleButton2D>("MenuButtonFans");
		menu_button->setToggleState(true);
		break;
	}
}

static bool g_bUpdateClock_Request = false;
static struct
{
	time_t gps_time;
} g_UpdateClock_Params;
void CmsUI::UpdateClock_Request(time_t gps_time) // called on non-kanzi thread
{
	char time_string[26];
	ctime_r(&gps_time, &time_string[0]);
	string time_text(time_string);
	pthread_mutex_lock(&clock_mutex_);
	CmsData::Instance()->minutes_of_day_ = 60*(atoi(time_text.substr(11,2).c_str())) + atoi(time_text.substr(14,2).c_str());
	pthread_mutex_unlock(&clock_mutex_);

	if(g_bUpdateClock_Request)
		printf("** UpdateClock_Request(%d) dropped because one for %d in progress\r\n",
			(int)gps_time, (int)g_UpdateClock_Params.gps_time);
	else
	{
		g_UpdateClock_Params.gps_time = gps_time;
		g_bUpdateClock_Request = true;
	}
}
void CmsUI::UpdateClock_Action(time_t gps_time) // called on kanzi thread
{
	char time_string[26];
	ctime_r(&gps_time, &time_string[0]);
	string time_text(time_string);
	string clock_text = time_text.substr(11,5);
	clocktext_widget_->setText(clock_text);
}
void CmsUI::UpdateClock_Check() // called on kanzi thread
{
	if(g_bUpdateClock_Request)
	{
		UpdateClock_Action(g_UpdateClock_Params.gps_time);
		g_bUpdateClock_Request = false;
	}
}

static bool g_bWakeUpScreen_Request = false;
void CmsUI::WakeUpScreen_Request() // called on non-kanzi thread
{
	g_bWakeUpScreen_Request = true;
	is_sleeping_ = false;
	CmsData::Instance()->RestartScreenTimer();
}
void CmsUI::WakeUpScreen_Action() // called on kanzi thread
{
	CmsData::Instance()->SetScreenBrightness();
	screen_cover_->setHitTestable(false);
	screen_cover_->setVisible(false);
}
void CmsUI::WakeUpScreen_Check() // called on kanzi thread
{
	if(g_bWakeUpScreen_Request)
	{
		WakeUpScreen_Action();
		g_bWakeUpScreen_Request = false;
	}
}

static bool g_bShutOffScreen_Request = false;
void CmsUI::ShutOffScreen_Request() // called on non-kanzi thread
{
	g_bShutOffScreen_Request = true;
	is_sleeping_ = true;
}
void CmsUI::ShutOffScreen_Action() // called on kanzi thread
{
	string shell_command = "echo brightness=0>/dev/sysctl";
	system(shell_command.c_str());
	screen_cover_->setHitTestable(true);
	screen_cover_->setVisible(true);
}
void CmsUI::ShutOffScreen_Check() // called on kanzi thread
{
	if(g_bShutOffScreen_Request)
	{
		ShutOffScreen_Action();
		g_bShutOffScreen_Request = false;
	}
}

bool CmsUI::GetSleepStatus()
{
	return is_sleeping_;
}

//========================================================================================
//		Screen Update requests
//========================================================================================
static bool g_bUpdateAll_Request = false;
void CmsUI::UpdateAll_Request() // called on non-kanzi thread
{
	g_bUpdateAll_Request = true;
}
void CmsUI::UpdateAll_Action() // called on kanzi thread
{
	//	Do slaves lights if master
	pthread_mutex_lock(&lighting_ui_mutex_);
	if(CmsData::Instance()->cms_self_.is_master)
	{
		for(unsigned int cms_it = 0; cms_it < CmsData::Instance()->cms_slaves_.size(); cms_it++)
		{
			for(unsigned int group_it = 0; group_it < CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.size(); group_it++)
			{
				UpdateProfile_Action(CmsData::Instance()->cms_slaves_.at(cms_it).id, group_it);
			}
		}
	}
	//	Do Lights
	for(unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.lighting_.lightgroups_.size(); group_it++)
	{
		UpdateProfile_Action(CmsData::Instance()->self_id_, group_it);
	}
	pthread_mutex_unlock(&lighting_ui_mutex_);

	//	Do Climate
	UpdateAirconPower_Action();
	UpdateAirconTemperature_Action();
	UpdateAirconMode_Action();
	UpdateAirconFanSpeed_Action();

	for(unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.climate_.heating_.size(); group_it++)
	{
		UpdateHeatingTemperature_Action(group_it);
		for(unsigned int switch_it = 0; switch_it < CmsData::Instance()->cms_self_.climate_.heating_.at(group_it).power_switches.size(); switch_it++)
		{
			UpdateHeatingPower_Action(group_it, switch_it);
		}
	}

	//	Do Fans
	for(unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.fans_.fansgroups_.size(); group_it++)
	{
		for(unsigned int fans_it = 0; fans_it < CmsData::Instance()->cms_self_.fans_.fansgroups_.at(group_it).fans.size(); fans_it++)
		{
			UpdateFanValue_Action(group_it, fans_it);
		}
	}

	//	Settings
	UpdateScreenBrightness_Action();
	UpdateScreenTimeout_Action();
}
void CmsUI::UpdateAll_Check() // called on kanzi thread
{
	if(g_bUpdateAll_Request)
	{
		UpdateAll_Action();
		g_bUpdateAll_Request = false;
	}
}

	//	Lighting
static bool g_bUpdateProfile_Request[2] = {false, false};
static struct
{
	unsigned char cms_id, group_index;
} g_UpdateProfile_Params[2];
void CmsUI::UpdateProfile_Request(const unsigned char cms_id, const unsigned char group_index) // called on non-kanzi thread
{
	// have we already got this one ?
	for(int i = 0; i < 2; i++)
	{
		if(g_bUpdateProfile_Request[i] && cms_id == g_UpdateProfile_Params[i].cms_id && group_index == g_UpdateProfile_Params[i].group_index)
			// already got this one queued
			return;
	}
	// find a slot for this new one
	for(int i = 0; i < 2; i++)
	{
		if(!g_bUpdateProfile_Request[i])
		{
			// added this new one
			g_UpdateProfile_Params[i].cms_id = cms_id;
			g_UpdateProfile_Params[i].group_index = group_index;
			g_bUpdateProfile_Request[i] = true;
			return;
		}
	}
	// no slot for new one
	printf("** UpdateProfile_Request(%d,%d) dropped because ones for %d,%d & %d,%d in progress\r\n",
			cms_id, group_index, 
			g_UpdateProfile_Params[0].cms_id, g_UpdateProfile_Params[0].group_index,
			g_UpdateProfile_Params[1].cms_id, g_UpdateProfile_Params[1].group_index);
}
void CmsUI::UpdateProfile_Action(const unsigned char cms_id, const unsigned char group_index) // called on kanzi thread
{
	pthread_mutex_lock(&lighting_ui_mutex_);
	lighting_ui_.UpdateProfile(cms_id, group_index);
	pthread_mutex_unlock(&lighting_ui_mutex_);
}
void CmsUI::UpdateProfile_Check() // called on kanzi thread
{
	for(int i = 0; i < 2; i++)
	{
		if(g_bUpdateProfile_Request[i])
		{
			UpdateProfile_Action(g_UpdateProfile_Params[i].cms_id, g_UpdateProfile_Params[i].group_index);
			g_bUpdateProfile_Request[i] = false;
		}
	}
}

static bool g_bUpdateLightValue_Request = false;
static struct
{
	unsigned char cms_id, group_index, light_index;
} g_UpdateLightValue_Params;
void CmsUI::UpdateLightValue_Request(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index) // called on non-kanzi thread
{
	if(g_bUpdateLightValue_Request)
		printf("** UpdateLightValue_Request(%d,%d,%d) dropped because one for %d,%d,%d in progress\r\n",
			cms_id, group_index, light_index, g_UpdateLightValue_Params.cms_id, g_UpdateLightValue_Params.group_index, g_UpdateLightValue_Params.light_index);
	else
	{
		g_UpdateLightValue_Params.cms_id = cms_id;
		g_UpdateLightValue_Params.group_index = group_index;
		g_UpdateLightValue_Params.light_index = light_index;
		g_bUpdateLightValue_Request = true;
	}
}
void CmsUI::UpdateLightValue_Action(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index) // called on kanzi thread
{
	pthread_mutex_lock(&lighting_ui_mutex_);
	lighting_ui_.UpdateLightValue(cms_id, group_index, light_index);
	pthread_mutex_unlock(&lighting_ui_mutex_);
}
void CmsUI::UpdateLightValue_Check() // called on kanzi thread
{
	if(g_bUpdateLightValue_Request)
	{
		UpdateLightValue_Action(g_UpdateLightValue_Params.cms_id, g_UpdateLightValue_Params.group_index, g_UpdateLightValue_Params.light_index);
		g_bUpdateLightValue_Request = false;
	}
}

	//	Climate
static bool g_bUpdateAirconPower_Request = false;
void CmsUI::UpdateAirconPower_Request() // called on non-kanzi thread
{
	g_bUpdateAirconPower_Request = true;
}
void CmsUI::UpdateAirconPower_Action() // called on kanzi thread
{
	climate_ui_.UpdateAirconPower();
}
void CmsUI::UpdateAirconPower_Check() // called on kanzi thread
{
	if(g_bUpdateAirconPower_Request)
	{
		UpdateAirconPower_Action();
		g_bUpdateAirconPower_Request = false;
	}
}

static bool g_bUpdateAirconTemperature_Request = false;
void CmsUI::UpdateAirconTemperature_Request() // called on non-kanzi thread
{
	g_bUpdateAirconTemperature_Request = true;
}
void CmsUI::UpdateAirconTemperature_Action() // called on kanzi thread
{
	climate_ui_.UpdateAirconTemperature();
}
void CmsUI::UpdateAirconTemperature_Check() // called on kanzi thread
{
	if(g_bUpdateAirconTemperature_Request)
	{
		UpdateAirconTemperature_Action();
		g_bUpdateAirconTemperature_Request = false;
	}
}

static bool g_bUpdateAirconAmbientTemperature_Request = false;
void CmsUI::UpdateAirconAmbientTemperature_Request() // called on non-kanzi thread
{
	g_bUpdateAirconAmbientTemperature_Request = true;
}
void CmsUI::UpdateAirconAmbientTemperature_Action() // called on kanzi thread
{
	climate_ui_.UpdateAirconAmbientTemperature();
}
void CmsUI::UpdateAirconAmbientTemperature_Check() // called on kanzi thread
{
	if(g_bUpdateAirconAmbientTemperature_Request)
	{
		UpdateAirconAmbientTemperature_Action();
		g_bUpdateAirconAmbientTemperature_Request = false;
	}
}

static bool g_bUpdateAirconSetPointTemperature_Request = false;
void CmsUI::UpdateAirconSetPointTemperature_Request() // called on non-kanzi thread
{
	g_bUpdateAirconSetPointTemperature_Request = true;
}
void CmsUI::UpdateAirconSetPointTemperature_Action() // called on kanzi thread
{
	climate_ui_.UpdateAirconSetPointTemperature();
}
void CmsUI::UpdateAirconSetPointTemperature_Check() // called on kanzi thread
{
	if(g_bUpdateAirconSetPointTemperature_Request)
	{
		UpdateAirconSetPointTemperature_Action();
		g_bUpdateAirconSetPointTemperature_Request = false;
	}
}

static bool g_bUpdateAirconMode_Request = false;
void CmsUI::UpdateAirconMode_Request() // called on non-kanzi thread
{
	g_bUpdateAirconMode_Request = true;
}
void CmsUI::UpdateAirconMode_Action() // called on kanzi thread
{
	climate_ui_.UpdateAirconMode();
}
void CmsUI::UpdateAirconMode_Check() // called on kanzi thread
{
	if(g_bUpdateAirconMode_Request)
	{
		UpdateAirconMode_Action();
		g_bUpdateAirconMode_Request = false;
	}
}

static bool g_bUpdateAirconFanSpeed_Request = false;
void CmsUI::UpdateAirconFanSpeed_Request() // called on non-kanzi thread
{
	g_bUpdateAirconFanSpeed_Request = true;
}
void CmsUI::UpdateAirconFanSpeed_Action() // called on kanzi thread
{
	climate_ui_.UpdateAirconFanSpeed();
}
void CmsUI::UpdateAirconFanSpeed_Check() // called on kanzi thread
{
	if(g_bUpdateAirconFanSpeed_Request)
	{
		UpdateAirconFanSpeed_Action();
		g_bUpdateAirconFanSpeed_Request = false;
	}
}

static bool g_bUpdateHeatingPower_Request = false;
static struct
{
	unsigned char group_index, switch_index;
} g_UpdateHeatingPower_Params;
void CmsUI::UpdateHeatingPower_Request(const unsigned char group_index, const unsigned char switch_index) // called on non-kanzi thread
{
	if(g_bUpdateHeatingPower_Request)
		printf("** UpdateHeatingPower_Request(%d,%d) dropped because one for %d,%d in progress\r\n",
			group_index, switch_index, g_UpdateHeatingPower_Params.group_index, g_UpdateHeatingPower_Params.switch_index);
	else
	{
		g_UpdateHeatingPower_Params.group_index = group_index;
		g_UpdateHeatingPower_Params.switch_index = switch_index;
		g_bUpdateHeatingPower_Request = true;
	}
}
void CmsUI::UpdateHeatingPower_Action(const unsigned char group_index, const unsigned char switch_index) // called on kanzi thread
{
	climate_ui_.UpdateHeatingPower(group_index, switch_index);
}
void CmsUI::UpdateHeatingPower_Check() // called on kanzi thread
{
	if(g_bUpdateHeatingPower_Request)
	{
		UpdateHeatingPower_Action(g_UpdateHeatingPower_Params.group_index, g_UpdateHeatingPower_Params.switch_index);
		g_bUpdateHeatingPower_Request = false;
	}
}

static bool g_bUpdateAllHeatingPower_Request = false;
void CmsUI::UpdateAllHeatingPower_Request() // called on non-kanzi thread
{
	g_bUpdateAllHeatingPower_Request = true;
}
void CmsUI::UpdateAllHeatingPower_Action() // called on kanzi thread
{
	climate_ui_.UpdateAllHeatingPower();
}
void CmsUI::UpdateAllHeatingPower_Check() // called on kanzi thread
{
	if(g_bUpdateAllHeatingPower_Request)
	{
		UpdateAllHeatingPower_Action();
		g_bUpdateAllHeatingPower_Request = false;
	}
}

static bool g_bUpdateHeatingTemperature_Request = false;
static struct
{
	unsigned char group_index;
} g_UpdateHeatingTemperature_Params;
void CmsUI::UpdateHeatingTemperature_Request(const unsigned char group_index) // called on non-kanzi thread
{
	if(g_bUpdateHeatingTemperature_Request)
		printf("** UpdateHeatingTemperature_Request(%d) dropped because one for %d in progress\r\n",
			group_index, g_UpdateHeatingTemperature_Params.group_index);
	else
	{
		g_UpdateHeatingTemperature_Params.group_index = group_index;
		g_bUpdateHeatingTemperature_Request = true;
	}
}
void CmsUI::UpdateHeatingTemperature_Action(const unsigned char group_index) // called on kanzi thread
{
	climate_ui_.UpdateHeatingTemperature(group_index);
}
void CmsUI::UpdateHeatingTemperature_Check() // called on kanzi thread
{
	if(g_bUpdateHeatingTemperature_Request)
	{
		UpdateHeatingTemperature_Action(g_UpdateHeatingTemperature_Params.group_index);
		g_bUpdateHeatingTemperature_Request = false;
	}
}

	//	Fans
static bool g_bUpdateFanValue_Request = false;
static struct
{
	unsigned char group_index, fan_index;
} g_UpdateFanValue_Params;
void CmsUI::UpdateFanValue_Request(const unsigned char group_index, const unsigned char fan_index) // called on non-kanzi thread
{
	if(g_bUpdateFanValue_Request)
		printf("** UpdateFanValue_Request(%d,%d) dropped because one for %d,%d in progress\r\n",
			group_index, fan_index, g_UpdateFanValue_Params.group_index, g_UpdateFanValue_Params.fan_index);
	else
	{
		g_UpdateFanValue_Params.group_index = group_index;
		g_UpdateFanValue_Params.fan_index = fan_index;
		g_bUpdateFanValue_Request = true;
	}
}
void CmsUI::UpdateFanValue_Action(const unsigned char group_index, const unsigned char fan_index) // called on kanzi thread
{
	fans_ui_.UpdateFanValue(group_index, fan_index);
}
void CmsUI::UpdateFanValue_Check() // called on kanzi thread
{
	if(g_bUpdateFanValue_Request)
	{
		UpdateFanValue_Action(g_UpdateFanValue_Params.group_index, g_UpdateFanValue_Params.fan_index);
		g_bUpdateFanValue_Request = false;
	}
}

	//	Settings
static bool g_bUpdateScreenBrightness_Request = false;
void CmsUI::UpdateScreenBrightness_Request() // called on non-kanzi thread
{
	g_bUpdateScreenBrightness_Request = true;
}
void CmsUI::UpdateScreenBrightness_Action() // called on kanzi thread
{
	settings_ui_.UpdateScreenBrightness();
}
void CmsUI::UpdateScreenBrightness_Check() // called on kanzi thread
{
	if(g_bUpdateScreenBrightness_Request)
	{
		UpdateScreenBrightness_Action();
		g_bUpdateScreenBrightness_Request = false;
	}
}

static bool g_bUpdateScreenTimeout_Request = false;
void CmsUI::UpdateScreenTimeout_Request() // called on non-kanzi thread
{
	g_bUpdateScreenTimeout_Request = true;
}
void CmsUI::UpdateScreenTimeout_Action() // called on kanzi thread
{
	settings_ui_.UpdateScreenTimeout();
}
void CmsUI::UpdateScreenTimeout_Check() // called on kanzi thread
{
	if(g_bUpdateScreenTimeout_Request)
	{
		UpdateScreenTimeout_Action();
		g_bUpdateScreenTimeout_Request = false;
	}
}

static bool g_bUpdateDawnStartTime_Request = false;
void CmsUI::UpdateDawnStartTime_Request() // called on non-kanzi thread
{
	g_bUpdateDawnStartTime_Request = true;
}
void CmsUI::UpdateDawnStartTime_Action() // called on kanzi thread
{
	settings_ui_.UpdateDawnStartTime();
}
void CmsUI::UpdateDawnStartTime_Check() // called on kanzi thread
{
	if(g_bUpdateDawnStartTime_Request)
	{
		UpdateDawnStartTime_Action();
		g_bUpdateDawnStartTime_Request = false;
	}
}

static bool g_bUpdateDawnEndTime_Request = false;
void CmsUI::UpdateDawnEndTime_Request() // called on non-kanzi thread
{
	g_bUpdateDawnEndTime_Request = true;
}
void CmsUI::UpdateDawnEndTime_Action() // called on kanzi thread
{
	settings_ui_.UpdateDawnEndTime();
}
void CmsUI::UpdateDawnEndTime_Check() // called on kanzi thread
{
	if(g_bUpdateDawnEndTime_Request)
	{
		UpdateDawnEndTime_Action();
		g_bUpdateDawnEndTime_Request = false;
	}
}

static bool g_bUpdateDuskStartTime_Request = false;
void CmsUI::UpdateDuskStartTime_Request() // called on non-kanzi thread
{
	g_bUpdateDuskStartTime_Request = true;
}
void CmsUI::UpdateDuskStartTime_Action() // called on kanzi thread
{
	settings_ui_.UpdateDuskStartTime();
}
void CmsUI::UpdateDuskStartTime_Check() // called on kanzi thread
{
	if(g_bUpdateDuskStartTime_Request)
	{
		UpdateDuskStartTime_Action();
		g_bUpdateDuskStartTime_Request = false;
	}
}

static bool g_bUpdateDuskEndTime_Request = false;
void CmsUI::UpdateDuskEndTime_Request() // called on non-kanzi thread
{
	g_bUpdateDuskEndTime_Request = true;
}
void CmsUI::UpdateDuskEndTime_Action() // called on kanzi thread
{
	settings_ui_.UpdateDuskEndTime();
}
void CmsUI::UpdateDuskEndTime_Check() // called on kanzi thread
{
	if(g_bUpdateDuskEndTime_Request)
	{
		UpdateDuskEndTime_Action();
		g_bUpdateDuskEndTime_Request = false;
	}
}

static bool g_bUpdateSunriseTime_Request = false;
void CmsUI::UpdateSunriseTime_Request() // called on non-kanzi thread
{
	g_bUpdateSunriseTime_Request = true;
}
void CmsUI::UpdateSunriseTime_Action() // called on kanzi thread
{
	settings_ui_.UpdateSunriseTime();
}
void CmsUI::UpdateSunriseTime_Check() // called on kanzi thread
{
	if(g_bUpdateSunriseTime_Request)
	{
		UpdateSunriseTime_Action();
		g_bUpdateSunriseTime_Request = false;
	}
}

static bool g_bUpdateSunsetTime_Request = false;
void CmsUI::UpdateSunsetTime_Request() // called on non-kanzi thread
{
	g_bUpdateSunsetTime_Request = true;
}
void CmsUI::UpdateSunsetTime_Action() // called on kanzi thread
{
	settings_ui_.UpdateSunsetTime();
}
void CmsUI::UpdateSunsetTime_Check() // called on kanzi thread
{
	if(g_bUpdateSunsetTime_Request)
	{
		UpdateSunsetTime_Action();
		g_bUpdateSunsetTime_Request = false;
	}
}

static bool g_bUpdateHeatingEnable_Request = false;
void CmsUI::UpdateHeatingEnable_Request() // called on non-kanzi thread
{
	g_bUpdateHeatingEnable_Request = true;
}
void CmsUI::UpdateHeatingEnable_Action() // called on kanzi thread
{
	settings_ui_.UpdateHeatingEnable();
}
void CmsUI::UpdateHeatingEnable_Check() // called on kanzi thread
{
	if(g_bUpdateHeatingEnable_Request)
	{
		UpdateHeatingEnable_Action();
		g_bUpdateHeatingEnable_Request = false;
	}
}
