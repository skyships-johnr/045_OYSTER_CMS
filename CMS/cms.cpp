#include <kanzi/kanzi.hpp>

#include <pthread.h>
#include <sched.h>

#include "cms.hpp"
#include "cms_ui.h"
#include "common.h"
#include "xml_data.h"
#include "cms_data.h"
#include "cms_control.h"
#include "udp_server.h"
#include "can_control.h"

using namespace kanzi;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// KANZI APPLICATION FRAMEWORK
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//==================================================================================================================================================================================
// Register
//==================================================================================================================================================================================

void CMS::registerMetadataOverride(ObjectFactory& /*factory*/)
{
	KanziComponentsModule::registerModule(getDomain());
}

//==================================================================================================================================================================================
// Configure application
//==================================================================================================================================================================================

void CMS::onConfigure(ApplicationProperties &configuration)
{
	configuration.binaryName = "CMS.kzb.cfg";
}

//==================================================================================================================================================================================
// Project loading complete - execute main application initialisation
//==================================================================================================================================================================================

void CMS::onProjectLoaded()
{
	ScreenSharedPtr screen = getScreen();

	//	Load data from XML
	CmsData::Instance()->InitValues();
	//	Initialize interfaces
	CmsUI::Instance()->InitInterface(screen);

	system("slay gns");

	if(CmsData::Instance()->cms_self_.is_master)
	{
		system("gns -s");
		system("echo GNS server started");

		//  Initialise CAN sender
		if(!CanControl::Instance()->InitCan())
		{
			//	Initialize controller
			if(!CmsControl::Instance()->InitController())
			{
				CanControl::Instance()->InitThreadPools();
				CmsData::Instance()->CommConnect();
				CmsUI::Instance()->CommConnect();
				UdpServer::Instance()->InitServer();
				CanControl::Instance()->SendAllMessages();
				CmsData::Instance()->InitControlTimers();
			}
		}
	}
	else
	{
		system("gns -c");
		system("echo GNS client started");
		int master_coid;

		//  Wait for master to be alive
		srand((rand()%200)*CmsData::Instance()->self_id_);
		delay(rand()%(20000/(CmsData::Instance()->self_id_)));
		while((master_coid = name_open("master", NAME_FLAG_ATTACH_GLOBAL)) == -1)
		{
			delay(rand()%(15000/CmsData::Instance()->self_id_));
		}
		name_close(master_coid);
		delay(rand()%(35000/(CmsData::Instance()->self_id_)));

		//	Initialize controller
		if(!CmsControl::Instance()->InitController())
		{
			CmsData::Instance()->CommConnect();
			CmsUI::Instance()->CommConnect();
			UdpServer::Instance()->InitServer();
			CmsData::Instance()->InitScreenTimer();
		}
	}

	//  Disable splash screen
	system("echo screen=-1>/dev/sysctl");
}

//==================================================================================================================================================================================
// Callback for application update implementation
//==================================================================================================================================================================================

void CMS::onUpdate(chrono::milliseconds /*deltaTime*/)
{
	// Update CMS UI elements
	CmsUI* pCMS = CmsUI::Instance();
	pCMS->ShutOffScreen_Check();
	pCMS->WakeUpScreen_Check();
	pCMS->UpdateProfile_Check();
	pCMS->UpdateAirconTemperature_Check();
	pCMS->UpdateLightValue_Check();
	pCMS->UpdateScreenBrightness_Check();
	pCMS->UpdateHeatingTemperature_Check();
	pCMS->UpdateHeatingPower_Check();
	pCMS->UpdateAllHeatingPower_Check();
	pCMS->UpdateFanValue_Check();
	pCMS->UpdateScreenTimeout_Check();
	pCMS->UpdateDawnStartTime_Check();
	pCMS->UpdateDawnEndTime_Check();
	pCMS->UpdateDuskStartTime_Check();
	pCMS->UpdateDuskEndTime_Check();
	pCMS->UpdateSunriseTime_Check();
	pCMS->UpdateSunsetTime_Check();
	pCMS->UpdateHeatingEnable_Check();
	pCMS->UpdateAirconSetPointTemperature_Check();
	pCMS->UpdateAirconPower_Check();
	pCMS->UpdateAirconMode_Check();
	pCMS->UpdateAirconFanSpeed_Check();
	pCMS->UpdateAirconAmbientTemperature_Check();
	pCMS->UpdateAll_Check();
	pCMS->UpdateClock_Check();
}

//==================================================================================================================================================================================
// Callback for application shut-down handling
//==================================================================================================================================================================================

void CMS::onShutdown()
{
	CanControl::Instance()->CloseChannels();
}

//==================================================================================================================================================================================
// Create CMS Application
//==================================================================================================================================================================================

Application* createApplication()
{
    return new CMS;
}

