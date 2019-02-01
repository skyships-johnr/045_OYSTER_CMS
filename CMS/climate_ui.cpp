#include "climate_ui.h"
#include "vector_class.h"

//========================================================================================
//		Constructor and destructor
//========================================================================================
ClimateInterface::ClimateInterface()
{
	control_coid_ = -1;
}

ClimateInterface::~ClimateInterface()
{

}


//========================================================================================
//		Initializers
//========================================================================================
void ClimateInterface::ClimateInterfaceInit(PageSharedPtr climate_page)
{
	//	Copy local pointers
	climate_page_ = climate_page;

	scroll_view_ = climate_page_->lookupNode<ScrollView2D>("ScrollView");
	entrypage_mode_text_ = climate_page->lookupNode<TextBlock2D>("#entrypage_mode_text");
	entrypage_temperature_text_ = climate_page->lookupNode<TextBlock2D>("#entrypage_temperature_text");

	//	Get custom properties
	group_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("GroupIndex"));
	switch_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("WidgetIndex"));
	temperaturesetbar_layoutwidth_ = FloatDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzFloat>("TemperatureBarLayoutWidth"));

	temperaturesetbar_text_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("TemperatureBarText"));
	modetoggle_button_text_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("ClimateModeButtonText"));
	fanspeed_box_text_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("ClimateFanSpeedBoxText"));

	aircontempbar_brush_ = climate_page->getResourceManager()->acquireResource<BrushResource>("kzb://cms/Brushes/AirconTemperatureBarBrush");
	heatingtempbar_brush_ = climate_page->getResourceManager()->acquireResource<BrushResource>("kzb://cms/Brushes/HeatingTemperatureBarBrush");


	PopulateClimateWidgets();
	SetInitValues();
	InitMessageHandlers();
}


//========================================================================================
//		Initializers
//========================================================================================
int ClimateInterface::PopulateClimateWidgets()
{
	StackLayout2DSharedPtr climatewidgets_stack = climate_page_->lookupNode<StackLayout2D>("ClimateWidgetsStack");


	ClickedCustomMessageSharedPtr minusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("MinusButtonPressed"));
	ClickedCustomMessageSharedPtr plusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("PlusButtonPressed"));

	//	Get custom prefabs
	PrefabTemplateSharedPtr aircon_widgetprefab = climate_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/AirconGroupWidget");
	PrefabTemplateSharedPtr heating_widgetprefab = climate_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/HeatingGroupWidget");
	PrefabTemplateSharedPtr switch_widgetprefab = climate_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/LabeledOnOff");

	//	Get custom properties
	StringDynamicPropertyTypeSharedPtr switchlabel_text = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("LabeledOnOffText"));
	StringDynamicPropertyTypeSharedPtr airconlabel_text = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("AirconGroupWidgetLabel"));
	StringDynamicPropertyTypeSharedPtr heatinglabel_text = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("HeatingGroupWidgetLabel"));

	//	Get aircon nodes
	aircon_setgroup_ = aircon_widgetprefab->instantiate<Node2D>("AirconWidget");
	aircon_setpointbar_ = aircon_setgroup_->lookupNode<Node2D>("#setpointbar");
	aircon_setpointbar_->setVisible(false);
	aircon_modetoggle_button_ = aircon_setgroup_->getChild(0)->getChild(2)->lookupNode<ToggleButton2D>("ModeToggleButton");
	aircon_fanspeed_box_ = aircon_setgroup_->getChild(0)->getChild(2)->lookupNode<Button2D>("ClimateFanSpeedBox");
	aircon_switch_button_ = aircon_setgroup_->getChild(0)->getChild(1)->lookupNode<ToggleButton2D>("OnOffButton");
	aircon_plusminus_ = aircon_setgroup_->getChild(0)->getChild(1)->getChild(2)->lookupNode<Node2D>("PlusMinusButtons");
	aircon_tempsetgroup_ = aircon_setgroup_->getChild(0)->getChild(1)->getChild(2);

	aircon_fanspeedbox_bar1_ = aircon_fanspeed_box_->getChild(0)->getChild(0);
	aircon_fanspeedbox_bar2_ = aircon_fanspeed_box_->getChild(0)->getChild(1);
	aircon_fanspeedbox_bar3_ = aircon_fanspeed_box_->getChild(0)->getChild(2);
	aircon_fanspeedbox_bar4_ = aircon_fanspeed_box_->getChild(0)->getChild(3);
	aircon_fanspeedbox_bar5_ = aircon_fanspeed_box_->getChild(0)->getChild(4);
	aircon_setgroup_->setProperty(*airconlabel_text, "Air Conditioning");

	aircon_plusminus_->addMessageHandler(*minusbutton_pressed, bind(&ClimateInterface::DecreaseAirconTemperature, this, placeholders::_1));
	aircon_plusminus_->addMessageHandler(*plusbutton_pressed, bind(&ClimateInterface::IncreaseAirconTemperature, this, placeholders::_1));

	aircon_modetoggle_button_->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&ClimateInterface::SwitchAirconMode, this, placeholders::_1));
	aircon_fanspeed_box_->addMessageHandler(Button2D::ClickedMessage, bind(&ClimateInterface::ChangeAirconFanSpeed, this, placeholders::_1));
	aircon_switch_button_->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&ClimateInterface::SwitchAirconPower, this, placeholders::_1));

	climatewidgets_stack->addChild(aircon_setgroup_);
	climatewidgets_stack->setHeight(aircon_setgroup_->getHeight());

	for(unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.climate_.heating_.size(); group_it++)
	{
		//	Get heating nodes
		Node2DSharedPtr heating_setgroup = heating_widgetprefab->instantiate<Node2D>("HeatingWidget");
		StackLayout2DSharedPtr heating_switchstack = heating_setgroup->getChild(0)->getChild(1)->lookupNode<StackLayout2D>("SwitchStack");
		Node2DSharedPtr heating_tempbar = heating_setgroup->getChild(0)->getChild(1)->getChild(1)->getChild(2)->lookupNode<Node2D>("Bar");
		string group_label = CmsData::Instance()->cms_self_.climate_.heating_.at(group_it).name + " Heating";
		heating_setgroup->setProperty(*heatinglabel_text, group_label);
		heating_setgroup->setProperty(*group_index_, group_it);

		vector_class<ToggleButton2DSharedPtr> switches;
		for (unsigned int switch_it = 0; switch_it < CmsData::Instance()->cms_self_.climate_.heating_.at(group_it).power_switches.size(); switch_it++)
		{
			Node2DSharedPtr heating_switch = switch_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).name);
			heating_switch->setProperty(*switchlabel_text, CmsData::Instance()->cms_self_.climate_.heating_.at(group_it).power_switches.at(switch_it).name);
			heating_switch->setProperty(*group_index_, group_it);
			heating_switch->setProperty(*switch_index_, switch_it);

			heating_switchstack->addChild(heating_switch);
			switches.push_back(heating_switch->getChild(0)->lookupNode<ToggleButton2D>("OnOffButton"));
			if (switch_it >= 1)
			{
				heating_setgroup->setHeight(heating_setgroup->getHeight() + heating_switch->getHeight() + 10.0);
			}
		}

		heating_tempbars_.push_back(heating_tempbar);
		heating_switch_buttons_.push_back(switches);
		heating_setgroups_.push_back(heating_setgroup);

		climatewidgets_stack->addChild(heating_setgroup);
		climatewidgets_stack->setHeight(climatewidgets_stack->getHeight() + heating_setgroup->getHeight() + 20.0);
	}

	if(CmsData::Instance()->cms_self_.climate_.has_master_switch_)
	{
		PrefabTemplateSharedPtr masterswitch_widgetprefab = climate_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/MasterSwitchGroupWidget");
		//	Instantiate master switch and set up triggers
		Node2DSharedPtr master_switch = masterswitch_widgetprefab->instantiate<Node2D>("MasterSwitch");
		master_switch->lookupNode<Button2D>("#onbutton")->setVisible(false);
		master_switch->lookupNode<Button2D>("#onbutton")->setHitTestable(false);//addMessageHandler(Button2D::ClickedMessage, bind(&ClimateInterface::MasterSwitchOn, this, placeholders::_1));
		master_switch->lookupNode<Button2D>("#offbutton")->addMessageHandler(Button2D::ClickedMessage, bind(&ClimateInterface::MasterSwitchOff, this, placeholders::_1));
		master_switch->lookupNode<TextBlock2D>("#buttonstitle")->setText("All Heaters");

		climatewidgets_stack->addChild(master_switch);
		climatewidgets_stack->setHeight(climatewidgets_stack->getHeight() + master_switch->getHeight());
	}

	//	Set scroll boundaries
	if (climatewidgets_stack->getHeight() > 480.0)
		scroll_view_->setScrollBoundsY(0.0, climatewidgets_stack->getHeight() - 480.0);
	else
		scroll_view_->setScrollBoundsY(0.0, 0.0);

	scroll_view_->setSlidingAccelerationCoefficient(SCROLL_SLIDACC);
	scroll_view_->setSlidingDragCoefficient(SCROLL_SLIDDRAG);
	scroll_view_->setDraggingAccelerationCoefficient(SCROLL_DRAGACC);
	scroll_view_->setDraggingDragCoefficient(SCROLL_DRAGDRAG);
	scroll_view_->setRecognitionThreshold(SCROLL_THRESHOLD);
	scroll_view_->setDraggingImpulseFactor(SCROLL_DRAGIMP);


	return 0;
}

void ClimateInterface::InitMessageHandlers()
{
	//	Get custom messages pointers
	ClickedCustomMessageSharedPtr minusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("MinusButtonPressed"));
	ClickedCustomMessageSharedPtr plusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("PlusButtonPressed"));

	//	Set message handlers
	aircon_plusminus_->addMessageHandler(*minusbutton_pressed, bind(&ClimateInterface::DecreaseAirconTemperature, this, placeholders::_1));
	aircon_plusminus_->addMessageHandler(*plusbutton_pressed, bind(&ClimateInterface::IncreaseAirconTemperature, this, placeholders::_1));

	aircon_modetoggle_button_->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&ClimateInterface::SwitchAirconMode, this, placeholders::_1));
	aircon_fanspeed_box_->addMessageHandler(Button2D::ClickedMessage, bind(&ClimateInterface::ChangeAirconFanSpeed, this, placeholders::_1));
	aircon_switch_button_->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&ClimateInterface::SwitchAirconPower, this, placeholders::_1));

	for(unsigned int group_it = 0; group_it < heating_setgroups_.size(); group_it++)
	{
		heating_setgroups_.at(group_it)->getChild(0)->getChild(1)->getChild(1)->lookupNode<Node2D>("PlusMinusButtons")
		->addMessageHandler(*minusbutton_pressed, bind(&ClimateInterface::DecreaseHeatingTemperature, this, placeholders::_1));
		heating_setgroups_.at(group_it)->getChild(0)->getChild(1)->getChild(1)->lookupNode<Node2D>("PlusMinusButtons")
		->addMessageHandler(*plusbutton_pressed, bind(&ClimateInterface::IncreaseHeatingTemperature, this, placeholders::_1));

		for(unsigned int switch_it = 0; switch_it < heating_switch_buttons_.at(group_it).size(); switch_it ++)
		{
			heating_switch_buttons_.at(group_it).at(switch_it)->addMessageHandler(Button2D::ClickedMessage, bind(&ClimateInterface::SwitchHeatingPower, this, placeholders::_1));
		}
	}

	//scroll_view_->addMessageFilter(ScrollView2D::ScrolledMessage, bind(&ClimateInterface::ScrollViewScrolled, this, placeholders::_1));
}

void ClimateInterface::SetInitValues()
{
	SetAirconTemperature(CmsData::Instance()->GetAirconTemperature());
	SetAirconFanSpeed(CmsData::Instance()->GetAirconFanSpeed());
	SetAirconPower(CmsData::Instance()->GetAirconPower());
	SetAirconMode(CmsData::Instance()->GetAirconMode());

	for(unsigned int group_it = 0; group_it < heating_setgroups_.size(); group_it ++)
	{
		int temp = CmsData::Instance()->GetHeatingTemperature(group_it);
		SetHeatingTemperature(group_it, CmsData::Instance()->GetHeatingTemperature(group_it));
		for (unsigned int switch_it = 0; switch_it < heating_switch_buttons_.at(group_it).size(); switch_it++)
		{
			heating_switch_buttons_.at(group_it).at(switch_it)->setToggleState(CmsData::Instance()->GetHeatingPower(group_it, switch_it));
		}
	}
}

int ClimateInterface::CommConnect()
{
	control_coid_ = name_open(CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);
	return control_coid_;
}

//========================================================================================
//		Screen setters
//========================================================================================

void ClimateInterface::SetAirconPower(const bool power)
{
	aircon_switch_button_->setToggleState(power);
	UpdateEntrypageBox();
}

void ClimateInterface::SetAirconTemperature(const int temperature)
{
	//	Normalize values
	float bar_width = (float)(temperature - 10) * 203.0 / 20.0;
	string bar_text = to_string(temperature);
	float blue = (1.0 - ((temperature - 10) / 25.0)) * 0.7;
	float red = ((temperature - 10) / 25.0) * 0.7;
	ColorRGBA color(red, 0.0, blue);

	aircontempbar_brush_->brush->setProperty(ColorBrush::ColorProperty, color);
	aircon_tempsetgroup_->setProperty(*temperaturesetbar_layoutwidth_, bar_width);
	aircon_tempsetgroup_->setProperty(*temperaturesetbar_text_, bar_text);
}

void ClimateInterface::SetAirconSetpointBarTemperature(const unsigned char temperature)
{
	//	Normalize values
	Matrix3x3 render_transformation = aircon_setpointbar_->getRenderTransformation();
	render_transformation.setTranslationX((float)(temperature - 10) * 203.0 / 20.0);

	//	Set on screen
	aircon_setpointbar_->setRenderTransformation(render_transformation);
}

void ClimateInterface::SetAirconMode(const Climate::Mode mode)
{
	switch (mode)
	{
	case Climate::OFF:
		aircon_modetoggle_button_->setProperty(*modetoggle_button_text_, "Off");
		break;
	case Climate::DEHUMIDIFY:
		aircon_modetoggle_button_->setProperty(*modetoggle_button_text_, "Dehumidify");
		break;
	case Climate::AUTO:
		aircon_modetoggle_button_->setProperty(*modetoggle_button_text_, "Auto");
		break;
	case Climate::HEATINGONLY:
		aircon_modetoggle_button_->setProperty(*modetoggle_button_text_, "Heat Only");
		break;
	case Climate::COOLONLY:
		aircon_modetoggle_button_->setProperty(*modetoggle_button_text_, "Cool Only");
		break;
	case Climate::AUTOWAUXHEAT:
		aircon_modetoggle_button_->setProperty(*modetoggle_button_text_, "Auto w/ Aux Heat");
		break;
	case Climate::AUXHEAT:
		aircon_modetoggle_button_->setProperty(*modetoggle_button_text_, "Aux Heat");
		break;
	}

}

void ClimateInterface::SetAirconFanSpeed(const unsigned char fan_speed)
{
	switch (fan_speed)
	{
	case 0:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Off");
		aircon_fanspeedbox_bar1_->setVisible(false);
		aircon_fanspeedbox_bar2_->setVisible(false);
		aircon_fanspeedbox_bar3_->setVisible(false);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 1:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Manual");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(false);
		aircon_fanspeedbox_bar3_->setVisible(false);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;

	case 2:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Manual");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(false);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 3:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Manual");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(true);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 4:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Manual");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(true);
		aircon_fanspeedbox_bar4_->setVisible(true);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 5:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Manual");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(true);
		aircon_fanspeedbox_bar4_->setVisible(true);
		aircon_fanspeedbox_bar5_->setVisible(true);
		break;
	case 6:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Off");
		aircon_fanspeedbox_bar1_->setVisible(false);
		aircon_fanspeedbox_bar2_->setVisible(false);
		aircon_fanspeedbox_bar3_->setVisible(false);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 7:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Auto");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(false);
		aircon_fanspeedbox_bar3_->setVisible(false);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;

	case 8:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Auto");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(false);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 9:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Auto");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(true);
		aircon_fanspeedbox_bar4_->setVisible(false);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 10:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Auto");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(true);
		aircon_fanspeedbox_bar4_->setVisible(true);
		aircon_fanspeedbox_bar5_->setVisible(false);
		break;
	case 11:
		aircon_fanspeed_box_->setProperty(*fanspeed_box_text_, "Auto");
		aircon_fanspeedbox_bar1_->setVisible(true);
		aircon_fanspeedbox_bar2_->setVisible(true);
		aircon_fanspeedbox_bar3_->setVisible(true);
		aircon_fanspeedbox_bar4_->setVisible(true);
		aircon_fanspeedbox_bar5_->setVisible(true);
		break;
	}
}

void ClimateInterface::SetHeatingPower(const unsigned char group_index, const unsigned char switch_index, const bool power)
{
	heating_switch_buttons_.at(group_index).at(switch_index)->setToggleState(power);
	UpdateEntrypageBox();
}

void ClimateInterface::SetHeatingTemperature(const unsigned char group_index, const unsigned char temperature)
{
	//	Normalize values
	kzFloat bar_width = (kzFloat)(temperature - 10) * 203.0 / 20.0;

//    printf(">>> array size = %d, index = %d\n", (int)heating_tempbars_.size(), (int)group_index);
//    printf(">>> node = %X\n", (unsigned int)(heating_tempbars_.at(group_index).get()));

	heating_tempbars_.at(group_index)->setWidth(bar_width);

	kzFloat blue = (1.0 - ((temperature - 10) / 25.0)) * 0.7;
	kzFloat red = ((temperature - 10) / 25.0) * 0.7;
	ColorRGBA color(red, 0.0, blue);

	heating_tempbars_.at(group_index)->setProperty(ColorBrush::ColorProperty, ColorRGBA(red, 0.0, blue, 1));

	UpdateEntrypageBox();
}

void ClimateInterface::UpdateEntrypageBox()
{
	if (CmsData::Instance()->GetAirconPower())
	{
		entrypage_mode_text_->setText("Aircon");
		//entrypage_temperature_text_->setText(to_string(CmsData::Instance()->GetAirconTemperature()));
	}
	else
	{
		if (CmsData::Instance()->GetHeatingPower(0, 0))
		{
			entrypage_mode_text_->setText("Heating");
			//entrypage_temperature_text_->setText(to_string(CmsData::Instance()->GetHeatingTemperature(0)));
		}
		else
		{
			entrypage_mode_text_->setText("Off");
			//entrypage_temperature_text_->setText(" - ");
		}
	}
}


void ClimateInterface::UpdateAirconPower()
{
	SetAirconPower(CmsData::Instance()->GetAirconPower());
}

void ClimateInterface::UpdateAirconTemperature()
{
	//bool showset = CmsData::Instance()->cms_self_.climate_.aircon_.showing_setpoint_;
	//if(showset)
		SetAirconTemperature(CmsData::Instance()->GetAirconTemperature());
	//else
	//	SetAirconTemperature(CmsData::Instance()->GetAirconAmbientTemperature());
}

void ClimateInterface::UpdateAirconSetPointTemperature()
{
	SetAirconSetpointBarTemperature(CmsData::Instance()->GetAirconTemperature());
}

void ClimateInterface::UpdateAirconAmbientTemperature()
{
	entrypage_temperature_text_->setText(to_string(CmsData::Instance()->GetAirconAmbientTemperature()));
}

void ClimateInterface::UpdateAirconMode()
{
	SetAirconMode(CmsData::Instance()->GetAirconMode());
}

void ClimateInterface::UpdateAirconFanSpeed()
{
	SetAirconFanSpeed(CmsData::Instance()->CmsData::Instance()->GetAirconFanSpeed());
}

void ClimateInterface::UpdateHeatingPower(const unsigned char group_index, const unsigned char switch_index)
{
	SetHeatingPower(group_index, switch_index, CmsData::Instance()->GetHeatingPower(group_index, switch_index));
}

void ClimateInterface::UpdateAllHeatingPower()
{
	for(unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.climate_.heating_.size(); group_it++)
	{
		for(unsigned int switch_it = 0; switch_it < CmsData::Instance()->cms_self_.climate_.heating_.at(group_it).power_switches.size(); switch_it++)
		{
			SetHeatingPower(group_it, switch_it, CmsData::Instance()->GetHeatingPower(group_it, switch_it));
		}
	}
}

void ClimateInterface::UpdateHeatingTemperature(const unsigned char group_index)
{
	SetHeatingTemperature(group_index, CmsData::Instance()->GetHeatingTemperature(group_index));
}

//========================================================================================
//		Event response methods
//========================================================================================

void ClimateInterface::DecreaseAirconTemperature(ButtonConcept::ClickedMessageArguments&)
{
	if(CmsData::Instance()->GetAirconPower())
	{
		unsigned char msg[8];
		unsigned char reply_msg[8];

		msg[0] = SCREEN;						//	From
		msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
		msg[2] = AIRCON;						//	What
		msg[3] = (unsigned char)0xFF;							//	N/A
		msg[4] = (unsigned char)0xFF;							//	N/A
		msg[5] = (unsigned char)0xFF;							//	N/A
		msg[6] = (unsigned char)0x00;							//	Temperature
		msg[7] = (unsigned char)0x00;							//	Decrease

		MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
	}
}

void ClimateInterface::IncreaseAirconTemperature(ButtonConcept::ClickedMessageArguments&)
{
	if(CmsData::Instance()->GetAirconPower())
	{
		unsigned char msg[8];
		unsigned char reply_msg[8];

		msg[0] = SCREEN;						//	From
		msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
		msg[2] = AIRCON;						//	What
		msg[3] = (unsigned char)0xFF;							//	N/A
		msg[4] = (unsigned char)0xFF;							//	N/A
		msg[5] = (unsigned char)0xFF;							//	N/A
		msg[6] = (unsigned char)0x00;							//	Temperature
		msg[7] = (unsigned char)0x01;							//	Increase

		MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
	}
}

void ClimateInterface::SwitchAirconMode(ButtonConcept::ClickedMessageArguments&)
{
	if(CmsData::Instance()->GetAirconPower())
	{
		unsigned char msg[8];
		unsigned char reply_msg[8];

		msg[0] = SCREEN;						//	From
		msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
		msg[2] = AIRCON;						//	What
		msg[3] = (unsigned char)0xFF;							//	N/A
		msg[4] = (unsigned char)0xFF;							//	N/A
		msg[5] = (unsigned char)0xFF;							//	N/A
		msg[6] = (unsigned char)0x02;							//	Mode
		msg[7] = (unsigned char)0x01;							//	Change

		MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
	}
}

void ClimateInterface::ChangeAirconFanSpeed(ButtonConcept::ClickedMessageArguments&)
{
	if(CmsData::Instance()->GetAirconPower())
	{
		unsigned char msg[8];
		unsigned char reply_msg[8];

		msg[0] = SCREEN;						//	From
		msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
		msg[2] = AIRCON;						//	What
		msg[3] = (unsigned char)0xFF;							//	N/A
		msg[4] = (unsigned char)0xFF;							//	N/A
		msg[5] = (unsigned char)0xFF;							//	N/A
		msg[6] = (unsigned char)0x03;							//	Fan speed
		msg[7] = (unsigned char)0x01;							//	Change

		MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
	}
}

void ClimateInterface::SwitchAirconPower(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = AIRCON;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x01;							//	Power
	msg[7] = (unsigned char)0x01;							//	Switch

	MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
}


void ClimateInterface::DecreaseHeatingTemperature(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char group_index = argument.getSource()->getParent()->getParent()->getParent()->getParent()->getProperty(*group_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = HEATING;						//	What
	msg[3] = (unsigned char)group_index;							//	N/A
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x00;							//	Tempreature
	msg[7] = (unsigned char)0x00;							//	Decrease

	MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
}

void ClimateInterface::IncreaseHeatingTemperature(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char group_index = argument.getSource()->getParent()->getParent()->getParent()->getParent()->getProperty(*group_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = HEATING;						//	What
	msg[3] = (unsigned char)group_index;							//	N/A
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x00;							//	Tempreature
	msg[7] = (unsigned char)0x01;							//	Increase

	MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
}

void ClimateInterface::SwitchHeatingPower(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char group_index = argument.getSource()->getParent()->getParent()->getProperty(*group_index_);
	unsigned char switch_index = argument.getSource()->getParent()->getParent()->getProperty(*switch_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = HEATING;						//	What
	msg[3] = (unsigned char)group_index;							//	N/A
	msg[4] = (unsigned char)switch_index;			//	Switch index
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x01;							//	Power
	msg[7] = (unsigned char)0x01;							//	Switch

	MsgSend(control_coid_, msg, sizeof(msg), reply_msg, sizeof(reply_msg));
}

void ClimateInterface::MasterSwitchOn(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = HEATING;				//	What
	msg[3] = (unsigned char)0xFF;					//	Group Index
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;						//	Cms ID
	msg[6] = (unsigned char)0x02;							//	N/A
	msg[7] = (unsigned char)0x01;				//	profile

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void ClimateInterface::MasterSwitchOff(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = HEATING;				//	What
	msg[3] = (unsigned char)0xFF;					//	Group Index
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;						//	Cms ID
	msg[6] = (unsigned char)0x02;							//	N/A
	msg[7] = (unsigned char)0x00;				//	profile

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void ClimateInterface::ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument)
{
	static bool scroll_forced = false;
	static float lastvalid_Ytrans = 0.0;
	static float scroll_pos = 0.0;

	scroll_pos = argument.getScrollPositionY();

	//	If it is within boundaries, keep valid position
	if(scroll_pos >= 0 && scroll_pos <= scroll_view_->getScrollBoundsMaximum().getY())
		lastvalid_Ytrans = scroll_pos;

	//	If it went bonkers, bring it back to last valid position
	//	do it just once and quickly
	if(scroll_pos < -480.0)
	{
		if(!scroll_forced)
		{
			scroll_view_->setSlidingAccelerationCoefficient(SCROLL_SLIDACC * 10);
			scroll_view_->setSlidingDragCoefficient(SCROLL_SLIDDRAG / 10);
			scroll_view_->setDraggingAccelerationCoefficient(SCROLL_DRAGACC * 10);
			scroll_view_->setDraggingDragCoefficient(SCROLL_DRAGDRAG / 10);
			Vector2 pos(0.0, lastvalid_Ytrans);
			scroll_view_->scrollToPosition(pos);
			scroll_forced = true;
		}
	}
	//	If it's back to normal, clear flags and set back speeds
	else
	{
		scroll_forced = false;
		scroll_view_->setSlidingAccelerationCoefficient(SCROLL_SLIDACC);
		scroll_view_->setSlidingDragCoefficient(SCROLL_SLIDDRAG);
		scroll_view_->setDraggingAccelerationCoefficient(SCROLL_DRAGACC);
		scroll_view_->setDraggingDragCoefficient(SCROLL_DRAGDRAG);
	}
}
