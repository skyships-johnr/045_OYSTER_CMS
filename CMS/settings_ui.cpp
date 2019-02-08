#include "settings_ui.h"



//========================================================================================
//		Constructor and destructor
//========================================================================================
SettingsInterface::SettingsInterface()
{
	control_coid_ = -1;
	sunwidgets_startpoint_ = 0;
}

SettingsInterface::~SettingsInterface()
{

}

//========================================================================================
//		Initializers
//========================================================================================
void SettingsInterface::SettingsInterfaceInit(PageSharedPtr settings_page)
{
	//	Copy local pointers
	settings_page_ = settings_page;

	scroll_view_ = settings_page_->lookupNode<ScrollView2D>("ScrollView");

	//	Get custom properties pointers
	//	Ints
	widget_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("WidgetIndex"));
	group_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("GroupIndex"));

	//	Floats
	labelledsetbar_layoutwidth_ = FloatDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzFloat>("LabelledSetBarLayoutWidth"));
	dawndusk_rangebar_layoutwidth_ = FloatDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzFloat>("DawnDuskTimeRangeBarLayoutWidth"));
	day_rangebar_layoutwidth_ = FloatDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzFloat>("DayTimeRangeBarLayoutWidth"));

	//	Matrices 3x3
	dawndusk_rangebar_rendertransformation_ = Matrix3x3DynamicPropertyTypeSharedPtr(new DynamicPropertyType<Matrix3x3>("DawnDuskTimeRangeBarRenderTransformation"));
	day_rangebar_rendertransformation_ = Matrix3x3DynamicPropertyTypeSharedPtr(new DynamicPropertyType<Matrix3x3>("DayTimeRangeBarRenderTransformation"));

	//	Strings
	labelledsetbar_text_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("LabelledSetBarText"));
	clocksetgroup_text_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("ClockSetGroupText"));
	suntimewidget_clocktext_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("SunTimeWidgetText"));

	PopulateSettingsWidgets();
	InitMessageHandlers();
	SetInitValues();

}


void SettingsInterface::InitMessageHandlers()
{
	//	Get custom messages pointers
	ClickedCustomMessageSharedPtr minusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("MinusButtonPressed"));
	ClickedCustomMessageSharedPtr plusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("PlusButtonPressed"));

	//	Set message handlers
	brightness_setbuttons_->addMessageHandler(*minusbutton_pressed, bind(&SettingsInterface::DecreaseScreenBrightness, this, placeholders::_1));
	brightness_setbuttons_->addMessageHandler(*plusbutton_pressed, bind(&SettingsInterface::IncreaseScreenBrightness, this, placeholders::_1));
	screentimeout_setbuttons_->addMessageHandler(*minusbutton_pressed, bind(&SettingsInterface::DecreaseScreenTimeout, this, placeholders::_1));
	screentimeout_setbuttons_->addMessageHandler(*plusbutton_pressed, bind(&SettingsInterface::IncreaseScreenTimeout, this, placeholders::_1));

	/*if (CmsData::Instance()->cms_self_.is_master)
	{
		dawnstart_time_set_->addMessageHandler(*minusbutton_pressed, bind(&SettingsInterface::DecreaseDawnStartTime, this, placeholders::_1));
		dawnstart_time_set_->addMessageHandler(*plusbutton_pressed, bind(&SettingsInterface::IncreaseDawnStartTime, this, placeholders::_1));
		dawnend_time_set_->addMessageHandler(*minusbutton_pressed, bind(&SettingsInterface::DecreaseDawnEndTime, this, placeholders::_1));
		dawnend_time_set_->addMessageHandler(*plusbutton_pressed, bind(&SettingsInterface::IncreaseDawnEndTime, this, placeholders::_1));
		duskstart_time_set_->addMessageHandler(*minusbutton_pressed, bind(&SettingsInterface::DecreaseDuskStartTime, this, placeholders::_1));
		duskstart_time_set_->addMessageHandler(*plusbutton_pressed, bind(&SettingsInterface::IncreaseDuskStartTime, this, placeholders::_1));
		duskend_time_set_->addMessageHandler(*minusbutton_pressed, bind(&SettingsInterface::DecreaseDuskEndTime, this, placeholders::_1));
		duskend_time_set_->addMessageHandler(*plusbutton_pressed, bind(&SettingsInterface::IncreaseDuskEndTime, this, placeholders::_1));
	}*/

	for (unsigned int reset_it = 0; reset_it < resetbutton_widgets_.size(); reset_it++)
	{
		resetbutton_widgets_.at(reset_it)->addMessageHandler(ButtonConcept::LongPressMessage, bind(&SettingsInterface::GenerateReset, this, placeholders::_1));

	}

	//scroll_view_->addMessageFilter(ScrollView2D::ScrolledMessage, bind(&SettingsInterface::ScrollViewScrolled, this, placeholders::_1));

}

void SettingsInterface::SetInitValues()
{
	SetScreenBrightness(CmsData::Instance()->GetScreenBrightness());
	SetScreenTimeout(CmsData::Instance()->GetScreenTimeout());
	/*if (CmsData::Instance()->cms_self_.is_master)
	{
		SetDawnStartTime(CmsData::Instance()->cms_self_.settings_.GetDawnStartTime(), CmsData::Instance()->cms_self_.settings_.GetDawnStartNormalizedTime());
		SetDawnEndTime(CmsData::Instance()->cms_self_.settings_.GetDawnEndTime(), CmsData::Instance()->cms_self_.settings_.GetDawnEndNormalizedTime());
		SetDuskStartTime(CmsData::Instance()->cms_self_.settings_.GetDuskStartTime(), CmsData::Instance()->cms_self_.settings_.GetDuskStartNormalizedTime());
		SetDuskEndTime(CmsData::Instance()->cms_self_.settings_.GetDuskEndTime(), CmsData::Instance()->cms_self_.settings_.GetDuskEndNormalizedTime());
		SetSunriseTime(CmsData::Instance()->cms_self_.settings_.GetSunriseTime(), CmsData::Instance()->cms_self_.settings_.GetSunriseNormalizedTime());
		SetSunsetTime(CmsData::Instance()->cms_self_.settings_.GetSunsetTime(), CmsData::Instance()->cms_self_.settings_.GetSunsetNormalizedTime());
	}*/
}

int SettingsInterface::PopulateSettingsWidgets()
{
	StackLayout2DSharedPtr settingswidgets_stack = settings_page_->lookupNode<StackLayout2D>("SettingsWidgetsStack");

	//	Get custom prefabs
	PrefabTemplateSharedPtr screensettings_widgetprefab = settings_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/ScreenSettingsWidget");
	PrefabTemplateSharedPtr timerangeset_widgetprefab = settings_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/TimeRangeSetGroupWidget");
	PrefabTemplateSharedPtr resetgroup_widgetprefab = settings_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/ResetGroupWidget");
	PrefabTemplateSharedPtr resetbutton_widgetprefab = settings_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/ResetButtonWidget");



	//	Do screen settings
	Node2DSharedPtr screensetting_widget = screensettings_widgetprefab->instantiate<Node2D>("ScreenSettingWidget");
	brightness_setbuttons_ = screensetting_widget->lookupNode<Node2D>("#brightness_setbuttons");
	brightness_setbar_ = screensetting_widget->lookupNode<Node2D>("#brightness_setbar");
	screentimeout_setbuttons_ = screensetting_widget->lookupNode<Node2D>("#timeout_setbuttons");
	screentimeout_setbar_ = screensetting_widget->lookupNode<Node2D>("#timeout_setbar");

	settingswidgets_stack->addChild(screensetting_widget);

	settingswidgets_stack->setHeight(screensetting_widget->getHeight() + 10.0);

	//	Do time auto-adjust
	/*
	if (CmsData::Instance()->cms_self_.is_master)
	{
		Node2DSharedPtr timerangeset_widget = timerangeset_widgetprefab->instantiate<Node2D>("TimeRangeSetWidget");
		dawnstart_time_set_ = timerangeset_widget->lookupNode<Node2D>("#dawnstart_time_set");
		dawnend_time_set_ = timerangeset_widget->lookupNode<Node2D>("#dawnend_time_set");
		duskstart_time_set_ = timerangeset_widget->lookupNode<Node2D>("#duskstart_time_set");
		duskend_time_set_ = timerangeset_widget->lookupNode<Node2D>("#duskend_time_set");
		timerange_bar_ = timerangeset_widget->lookupNode<Node2D>("#timerange_bar");
		sunrise_widget_ = timerangeset_widget->lookupNode<Node2D>("#sunrise_widget");
		sunset_widget_ = timerangeset_widget->lookupNode<Node2D>("#sunset_widget");

		settingswidgets_stack->addChild(timerangeset_widget);

		settingswidgets_stack->setHeight(settingswidgets_stack->getHeight() + timerangeset_widget->getHeight() + 20.0);

		sunrise_rendertransformation_ = sunrise_widget_->getProperty(Node2D::RenderTransformationProperty);
		sunset_rendertransformation_ = sunrise_widget_->getProperty(Node2D::RenderTransformationProperty);
		sunwidgets_startpoint_ = sunrise_rendertransformation_.getTranslationX();
	}*/



	//	Do resets
	StringDynamicPropertyTypeSharedPtr resetgroup_labeltext = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("ResetGroupWidgetName"));
	StringDynamicPropertyTypeSharedPtr resetbutton_labeltext = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("ResetButtonWidgetName"));

	resetbutton_widgets_.clear();

	for (unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.settings_.reset_groups_.size(); group_it++)
	{
		//	Set group properties
		Node2DSharedPtr resetgroup_widget = resetgroup_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.settings_.reset_groups_.at(group_it).name);
		resetgroup_widget->setProperty(*resetgroup_labeltext, CmsData::Instance()->cms_self_.settings_.reset_groups_.at(group_it).name);
		resetgroup_widget->setProperty(*widget_index_, group_it);

		StackLayout2DSharedPtr resets_stack = resetgroup_widget->lookupNode<StackLayout2D>("ResetsStack");

		for (unsigned int reset_it = 0; reset_it < CmsData::Instance()->cms_self_.settings_.reset_groups_.at(group_it).resets.size(); reset_it++)
		{
			//	Set button properties
			Node2DSharedPtr resetbutton_widget = resetbutton_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.settings_.reset_groups_.at(group_it).resets.at(reset_it).name);
			resetbutton_widget->setProperty(*resetbutton_labeltext, CmsData::Instance()->cms_self_.settings_.reset_groups_.at(group_it).resets.at(reset_it).name);
			resetbutton_widget->setProperty(*group_index_, group_it);
			resetbutton_widget->setProperty(*widget_index_, reset_it);

			//	Get the button and add to vector
			Button2DSharedPtr reset_button = resetbutton_widget->lookupNode<Button2D>("ResetButton");
			resetbutton_widgets_.push_back(reset_button);

			//	Push into screen and adjust height to accomodate it
			resets_stack->addChild(resetbutton_widget);
			resetgroup_widget->setHeight(resetgroup_widget->getHeight() + resetbutton_widget->getHeight() + 10.0);
		}

		//	Push into screen and adjust height to accomodate it
		settingswidgets_stack->addChild(resetgroup_widget);
		settingswidgets_stack->setHeight(settingswidgets_stack->getHeight() + resetgroup_widget->getHeight() + 10.0);

	}

	if(CmsData::Instance()->cms_self_.settings_.has_master_heating_)
	{
		PrefabTemplateSharedPtr masterheating_widgetprefab = settings_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/MasterHeaterSwitchGroup");
		Node2DSharedPtr masterheating_widget = masterheating_widgetprefab->instantiate<Node2D>("MasterHeatingSwitchGroup");
		masterheating_button_ = masterheating_widget->lookupNode<Button2D>("#button");
		masterheating_button_->addMessageHandler(Button2D::ClickedMessage, bind(&SettingsInterface::SwitchHeatingEnable, this, placeholders::_1));
		masterheating_buttontext_ = masterheating_button_->getChild(0)->lookupNode<TextBlock2D>("ButtonText");

		settingswidgets_stack->addChild(masterheating_widget);
		settingswidgets_stack->setHeight(settingswidgets_stack->getHeight() + masterheating_widget->getHeight() + 20.0);

		UpdateHeatingEnable();
	}


	//	Set scroll boundaries
	if (settingswidgets_stack->getHeight() > 480.0)
		scroll_view_->setScrollBoundsY(0.0, settingswidgets_stack->getHeight() - 480.0);
	else
		scroll_view_->setScrollBoundsY(0.0, 0.0);

	scroll_view_->setSlidingAccelerationCoefficient(SCROLL_SLIDACC);
	scroll_view_->setSlidingDragCoefficient(SCROLL_SLIDDRAG);
	scroll_view_->setDraggingAccelerationCoefficient(SCROLL_DRAGACC);
	scroll_view_->setDraggingDragCoefficient(SCROLL_DRAGDRAG);
	scroll_view_->setRecognitionThreshold(SCROLL_THRESHOLD);
	scroll_view_->setDraggingImpulseFactor(SCROLL_DRAGIMP);

	return 1;
}

int SettingsInterface::CommConnect()
{
	control_coid_ = name_open(CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);
	return control_coid_;
}

//========================================================================================
//		Screen setters
//========================================================================================

void SettingsInterface::SetScreenBrightness(const int brightness)
{
	//	Normalize values
	float bar_width = (brightness)*310.0/100.0;
	string bar_text = to_string(brightness) + "%";

	//	Set on screen
	brightness_setbar_->setProperty(*labelledsetbar_layoutwidth_, bar_width);
	brightness_setbar_->setProperty(*labelledsetbar_text_, bar_text);
}

void SettingsInterface::SetScreenTimeout(const int timeout)
{
	//	Normalize values
	int bar_width = (timeout-2) * 310 / 28;
    string bar_text = CmsData::Instance()->cms_self_.settings_.GetScreenTimeoutText(timeout);

	//	Set on screen
	screentimeout_setbar_->setProperty(*labelledsetbar_layoutwidth_, bar_width);
	screentimeout_setbar_->setProperty(*labelledsetbar_text_, bar_text);
}

void SettingsInterface::SetDawnStartTime(const int dawnstart_time, const int dawnstart_normalized_time)
{
	//	Normalize values
	float bar_translation = (float)dawnstart_normalized_time * (492.0 / 1440.0);
	Matrix3x3 render_transformation;
	render_transformation.setTranslationX(bar_translation);

	string minutes = to_string(dawnstart_time % 60);
	if (minutes.size()< 2)
		minutes = "0" + minutes;
	string hours = to_string(dawnstart_time / 60);
	if (hours.size()<2)
		hours = "0" + hours;
	string clock = hours + ":" + minutes;

	//	Set on screen
	timerange_bar_->setProperty(*dawndusk_rangebar_rendertransformation_, render_transformation);
	dawnstart_time_set_->setProperty(*clocksetgroup_text_, clock);

	//	Adjust bar width
	SetDuskEndTime(CmsData::Instance()->cms_self_.settings_.GetDuskEndTime(), CmsData::Instance()->cms_self_.settings_.GetDuskEndNormalizedTime());

}

void SettingsInterface::SetDawnEndTime(const int dawnend_time, const int dawnend_normalized_time)
{
	//	Normalize values
	float bar_translation = (float)dawnend_normalized_time * (492.0 / 1440.0);
	Matrix3x3 render_transformation;
	render_transformation.setTranslationX(bar_translation);

	string minutes = to_string(dawnend_time % 60);
	if (minutes.size()< 2)
		minutes = "0" + minutes;
	string hours = to_string(dawnend_time / 60);
	if (hours.size()<2)
		hours = "0" + hours;
	string clock = hours + ":" + minutes;

	//	Set on screen
	timerange_bar_->setProperty(*day_rangebar_rendertransformation_, render_transformation);
	dawnend_time_set_->setProperty(*clocksetgroup_text_, clock);

	//	Adjust bar width
	SetDuskStartTime(CmsData::Instance()->cms_self_.settings_.GetDuskStartTime(), CmsData::Instance()->cms_self_.settings_.GetDuskStartNormalizedTime());

}

void SettingsInterface::SetDuskStartTime(const int duskstart_time, const int duskstart_normalized_time)
{
	//	Normalize values
	float bar_width = (float)duskstart_normalized_time * (492.0 / 1440.0);
	Matrix3x3 render_transformation = timerange_bar_->getProperty(*day_rangebar_rendertransformation_);
	bar_width = bar_width - render_transformation.getTranslationX();

	string minutes = to_string(duskstart_time % 60);
	if (minutes.size()< 2)
		minutes = "0" + minutes;
	string hours = to_string(duskstart_time / 60);
	if (hours.size()<2)
		hours = "0" + hours;
	string clock = hours + ":" + minutes;

	//	Set on screen
	timerange_bar_->setProperty(*day_rangebar_layoutwidth_, bar_width);
	duskstart_time_set_->setProperty(*clocksetgroup_text_, clock);

}

void SettingsInterface::SetDuskEndTime(const int duskend_time, const int duskend_normalized_time)
{
	//	Normalize values
	float bar_width = (float)duskend_normalized_time * (492.0 / 1440.0);
	Matrix3x3 render_transformation = timerange_bar_->getProperty(*dawndusk_rangebar_rendertransformation_);
	bar_width = bar_width - render_transformation.getTranslationX();

	string minutes = to_string(duskend_time % 60);
	if (minutes.size()< 2)
		minutes = "0" + minutes;
	string hours = to_string(duskend_time / 60);
	if (hours.size()<2)
		hours = "0" + hours;
	string clock = hours + ":" + minutes;


	//	Set on screen
	timerange_bar_->setProperty(*dawndusk_rangebar_layoutwidth_, bar_width);
	duskend_time_set_->setProperty(*clocksetgroup_text_, clock);

}

void SettingsInterface::SetSunriseTime(const int sunrise_time, const int sunrise_normalized_time)
{
	//	Normalize values
	sunrise_rendertransformation_.setTranslationX(sunwidgets_startpoint_ + ((float)sunrise_normalized_time * (492.0 / 1440.0)));

	string minutes = to_string(sunrise_time % 60);
	if (minutes.size()< 2)
		minutes = "0" + minutes;
	string hours = to_string(sunrise_time / 60);
	if (hours.size()<2)
		hours = "0" + hours;
	string clock = hours + ":" + minutes;

	//	Set on screen
	sunrise_widget_->setRenderTransformation(sunrise_rendertransformation_);
	sunrise_widget_->setProperty(*suntimewidget_clocktext_, clock);
}

void SettingsInterface::SetSunsetTime(const int sunset_time, const int sunset_normalized_time)
{
	//	Normalize values
	sunset_rendertransformation_.setTranslationX(sunwidgets_startpoint_ + ((float)sunset_normalized_time * (492.0 / 1440.0)));

	string minutes = to_string(sunset_time % 60);
	if (minutes.size()< 2)
		minutes = "0" + minutes;
	string hours = to_string(sunset_time / 60);
	if (hours.size()<2)
		hours = "0" + hours;
	string clock = hours + ":" + minutes;

	//	Set on screen
	sunset_widget_->setRenderTransformation(sunset_rendertransformation_);
	sunset_widget_->setProperty(*suntimewidget_clocktext_, clock);
}

void SettingsInterface::UpdateScreenBrightness()
{
	SetScreenBrightness(CmsData::Instance()->GetScreenBrightness());
}

void SettingsInterface::UpdateScreenTimeout()
{
	SetScreenTimeout(CmsData::Instance()->GetScreenTimeout());
}

void SettingsInterface::UpdateDawnStartTime()
{
	SetDawnStartTime(CmsData::Instance()->GetDawnStartTime(), CmsData::Instance()->GetDawnStartNormalizedTime());
}

void SettingsInterface::UpdateDawnEndTime()
{
	SetDawnEndTime(CmsData::Instance()->GetDawnEndTime(), CmsData::Instance()->GetDawnEndNormalizedTime());
}

void SettingsInterface::UpdateDuskStartTime()
{
	SetDuskStartTime(CmsData::Instance()->GetDuskStartTime(), CmsData::Instance()->GetDuskStartNormalizedTime());
}

void SettingsInterface::UpdateDuskEndTime()
{
	SetDuskEndTime(CmsData::Instance()->GetDuskEndTime(), CmsData::Instance()->GetDuskEndNormalizedTime());
}

void SettingsInterface::UpdateSunriseTime()
{
	SetSunriseTime(CmsData::Instance()->GetSunriseTime(), CmsData::Instance()->GetSunriseNormalizedTime());
}

void SettingsInterface::UpdateSunsetTime()
{
	SetSunsetTime(CmsData::Instance()->GetSunsetTime(), CmsData::Instance()->GetSunsetNormalizedTime());
}

void SettingsInterface::UpdateHeatingEnable()
{
	if(CmsData::Instance()->cms_self_.climate_.master_heatingenable_)
	{
		masterheating_buttontext_->setText("All Heaters Enabled");
	}
	else
	{
		masterheating_buttontext_->setText("All Heaters Disabled");
	}
}

//========================================================================================
//		Value manipulation functions
//========================================================================================

	//	Screen brightness
void SettingsInterface::DecreaseScreenBrightness(ButtonConcept::ClickedMessageArguments&)
{

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = 0xFF;							//	N/A
	msg[4] = 0xFF;							//	N/A
	msg[5] = 0xFF;							//	N/A
	msg[6] = 0x00;							//	Brightness
	msg[7] = 0x00;							//	Decrease

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::IncreaseScreenBrightness(ButtonConcept::ClickedMessageArguments&)
{

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = 0xFF;							//	N/A
	msg[4] = 0xFF;							//	N/A
	msg[5] = 0xFF;							//	N/A
	msg[6] = 0x00;							//	Brightness
	msg[7] = 0x01;							//	Increase

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

	// Screen timeout
void SettingsInterface::DecreaseScreenTimeout(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x01;							//	Timeout
	msg[7] = (unsigned char)0x00;							//	Decrease

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::IncreaseScreenTimeout(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x01;							//	Timeout
	msg[7] = (unsigned char)0x01;							//	Increase

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

	// Dawn start time
void SettingsInterface::DecreaseDawnStartTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x00;							//	Dawn start
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x00;							//	Decrease

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::IncreaseDawnStartTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x00;							//	Dawn start
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x01;							//	Increase

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::DecreaseDawnEndTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x01;							//	Dawn end
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x00;							//	Decrease

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::IncreaseDawnEndTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x01;							//	Dawn end
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x01;							//	Increase

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::DecreaseDuskStartTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x02;							//	Dusk start
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x00;							//	Decrease

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::IncreaseDuskStartTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x02;							//	Dusk start
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x01;							//	Increase

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::DecreaseDuskEndTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x03;							//	Dusk end
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x00;							//	Decrease

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::IncreaseDuskEndTime(ButtonConcept::ClickedMessageArguments&)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;							//	N/A
	msg[4] = (unsigned char)0x03;							//	Dusk end
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x02;							//	Time
	msg[7] = (unsigned char)0x01;							//	Increase

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}


void SettingsInterface::GenerateReset(ButtonConcept::LongPressMessageArguments& argument)
{
	unsigned char group_index = argument.getSource()->getParent()->getProperty(*group_index_);
	unsigned char reset_index = argument.getSource()->getParent()->getProperty(*widget_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)group_index;				//	N/A
	msg[4] = (unsigned char)reset_index;				//
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x03;							//	Reset
	msg[7] = (unsigned char)0x01;							//	Send

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::SwitchHeatingEnable(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = SETTINGS;						//	What
	msg[3] = (unsigned char)0xFF;				//	N/A
	msg[4] = (unsigned char)0xFF;				//
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x05;							//	Reset
	msg[7] = (unsigned char)0x01;							//	Send

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void SettingsInterface::ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument)
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
			Vector2 pos(0.0, 0.0);
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
