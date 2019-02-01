#ifndef CMS_CLIMATEINTERFACE_H_
#define CMS_CLIMATEINTERFACE_H_

#include <kanzi/kanzi.hpp>
#include "vector_class.h"

#include "common.h"
#include "cms_data.h"


using namespace kanzi;



class ClimateInterface
{
private:
	//	Pointer to Climate page
	PageSharedPtr climate_page_;

	//	Page nodes
	ScrollView2DSharedPtr scroll_view_;
	Node2DSharedPtr aircon_setgroup_;
	Node2DSharedPtr aircon_tempsetgroup_;
	Node2DSharedPtr aircon_setpointbar_;
	ToggleButton2DSharedPtr aircon_modetoggle_button_;
	Button2DSharedPtr aircon_fanspeed_box_;
	ToggleButton2DSharedPtr  aircon_switch_button_;
	Node2DSharedPtr aircon_plusminus_;
	NodeSharedPtr aircon_fanspeedbox_bar1_;
	NodeSharedPtr aircon_fanspeedbox_bar2_;
	NodeSharedPtr aircon_fanspeedbox_bar3_;
	NodeSharedPtr aircon_fanspeedbox_bar4_;
	NodeSharedPtr aircon_fanspeedbox_bar5_;

	TextBlock2DSharedPtr entrypage_mode_text_;
	TextBlock2DSharedPtr entrypage_temperature_text_;

	BrushResourceSharedPtr aircontempbar_brush_;


	vector_class<Node2DSharedPtr> heating_setgroups_;
	vector_class<Node2DSharedPtr> heating_tempbars_;
	vector_class<BrushResourceSharedPtr> heating_tempbarbrushes_;
	vector_class<vector_class<ToggleButton2DSharedPtr>>  heating_switch_buttons_;
	BrushResourceSharedPtr heatingtempbar_brush_;

	//	Pointers to custom properties
	IntDynamicPropertyTypeSharedPtr group_index_;
	IntDynamicPropertyTypeSharedPtr switch_index_;
	FloatDynamicPropertyTypeSharedPtr temperaturesetbar_layoutwidth_;

	StringDynamicPropertyTypeSharedPtr temperaturesetbar_text_;
	StringDynamicPropertyTypeSharedPtr modetoggle_button_text_;
	StringDynamicPropertyTypeSharedPtr fanspeed_box_text_;

	int control_coid_;

	//	Initializers
	void SetInitValues();
	int PopulateClimateWidgets();
	void InitMessageHandlers();


	//	Screen setters
	void SetAirconPower(const bool power);
	void SetAirconTemperature(const int temperature);
	void SetAirconSetpointBarTemperature(const unsigned char temperature);
	void SetAirconMode(const Climate::Mode mode);
	void SetAirconFanSpeed(const unsigned char fan_speed);
	void SetHeatingPower(const unsigned char group_index, const unsigned char switch_index, const bool power);
	void SetHeatingTemperature(const unsigned char group_index, const unsigned char temperature);
	void SetEntrypageBox();
	void UpdateEntrypageBox();

	//	Event response methods
	void DecreaseAirconTemperature(ButtonConcept::ClickedMessageArguments&);
	void IncreaseAirconTemperature(ButtonConcept::ClickedMessageArguments&);
	void SwitchAirconMode(ButtonConcept::ClickedMessageArguments&);
	void ChangeAirconFanSpeed(ButtonConcept::ClickedMessageArguments&);
	void SwitchAirconPower(ButtonConcept::ClickedMessageArguments&);
	void DecreaseHeatingTemperature(ButtonConcept::ClickedMessageArguments&);
	void IncreaseHeatingTemperature(ButtonConcept::ClickedMessageArguments&);
	void SwitchHeatingPower(ButtonConcept::ClickedMessageArguments& argument);
	void MasterSwitchOn(ButtonConcept::ClickedMessageArguments& argument);
	void MasterSwitchOff(ButtonConcept::ClickedMessageArguments& argument);
	void ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument);

public:
	ClimateInterface();
	~ClimateInterface();

	void ClimateInterfaceInit(PageSharedPtr climate_page);
	int CommConnect();

	void UpdateAirconPower();
	void UpdateAirconTemperature();
	void UpdateAirconAmbientTemperature();
	void UpdateAirconSetPointTemperature();
	void UpdateAirconMode();
	void UpdateAirconFanSpeed();
	void UpdateHeatingPower(const unsigned char group_index, const unsigned char switch_index);
	void UpdateAllHeatingPower();
	void UpdateHeatingTemperature(const unsigned char group_index);
};

#endif
