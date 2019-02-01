#ifndef CMS_SETTINGSINTERFACE_H_
#define CMS_SETTINGSINTERFACE_H_

#include <kanzi/kanzi.hpp>

#include "common.h"
#include "cms_data.h"
#include "vector_class.h"

using namespace kanzi;

class SettingsInterface
{
private:
	//	Pointer to Settings page
	PageSharedPtr settings_page_;

	ScrollView2DSharedPtr scroll_view_;

	//	ScreenSettingsWidget nodes
	Node2DSharedPtr brightness_setbuttons_;
	Node2DSharedPtr brightness_setbar_;
	Node2DSharedPtr screentimeout_setbuttons_;
	Node2DSharedPtr screentimeout_setbar_;

	//	TimeRangeSetWidget nodes
	Node2DSharedPtr dawnstart_time_set_;
	Node2DSharedPtr dawnend_time_set_;
	Node2DSharedPtr duskstart_time_set_;
	Node2DSharedPtr duskend_time_set_;
	Node2DSharedPtr timerange_bar_;
	Node2DSharedPtr sunrise_widget_;
	Node2DSharedPtr sunset_widget_;

	Button2DSharedPtr masterheating_button_;
	TextBlock2DSharedPtr masterheating_buttontext_;

	TimerTriggerSharedPtr reset_timer_;
	SetPropertyActionSharedPtr reset_action_;

	//	Pointers to custom properties
	IntDynamicPropertyTypeSharedPtr widget_index_;
	IntDynamicPropertyTypeSharedPtr group_index_;

	FloatDynamicPropertyTypeSharedPtr labelledsetbar_layoutwidth_;
	FloatDynamicPropertyTypeSharedPtr dawndusk_rangebar_layoutwidth_;
	FloatDynamicPropertyTypeSharedPtr day_rangebar_layoutwidth_;

	Matrix3x3DynamicPropertyTypeSharedPtr dawndusk_rangebar_rendertransformation_;
	Matrix3x3DynamicPropertyTypeSharedPtr day_rangebar_rendertransformation_;

	StringDynamicPropertyTypeSharedPtr labelledsetbar_text_;
	StringDynamicPropertyTypeSharedPtr clocksetgroup_text_;
	StringDynamicPropertyTypeSharedPtr suntimewidget_clocktext_;

	vector_class<Button2DSharedPtr> resetbutton_widgets_;

	int control_coid_;

	float sunwidgets_startpoint_;
	Matrix3x3 sunrise_rendertransformation_;
	Matrix3x3 sunset_rendertransformation_;

	//	Initializers
	void SetInitValues();
	void InitMessageHandlers();
	int PopulateSettingsWidgets();

	//	Screen setting methods
	void SetScreenBrightness(const int brightness);
	void SetScreenTimeout(const int timeout);
	void SetDawnStartTime(const int dawnstart_time, const int dawnstart_normalized_time);
	void SetDawnEndTime(const int dawnend_time, const int dawnend_normalized_time);
	void SetDuskStartTime(const int duskstart_time, const int duskstart_normalized_time);
	void SetDuskEndTime(const int duskend_time, const int duskend_normalized_time);
	void SetSunriseTime(const int sunrise_time, const int sunrise_normalized_time);
	void SetSunsetTime(const int sunset_time, const int sunset_normalized_time);

	//	Event response methods
	void DecreaseScreenBrightness(ButtonConcept::ClickedMessageArguments&);
	void IncreaseScreenBrightness(ButtonConcept::ClickedMessageArguments&);
	void DecreaseScreenTimeout(ButtonConcept::ClickedMessageArguments&);
	void IncreaseScreenTimeout(ButtonConcept::ClickedMessageArguments&);
	void DecreaseDawnStartTime(ButtonConcept::ClickedMessageArguments&);
	void IncreaseDawnStartTime(ButtonConcept::ClickedMessageArguments&);
	void DecreaseDawnEndTime(ButtonConcept::ClickedMessageArguments&);
	void IncreaseDawnEndTime(ButtonConcept::ClickedMessageArguments&);
	void DecreaseDuskStartTime(ButtonConcept::ClickedMessageArguments&);
	void IncreaseDuskStartTime(ButtonConcept::ClickedMessageArguments&);
	void DecreaseDuskEndTime(ButtonConcept::ClickedMessageArguments&);
	void IncreaseDuskEndTime(ButtonConcept::ClickedMessageArguments&);
	void GenerateReset(ButtonConcept::LongPressMessageArguments& argument);
	void SwitchHeatingEnable(ButtonConcept::ClickedMessageArguments& argument);
	void ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument);


public:
	//	Constructor and destructor
	SettingsInterface();
	~SettingsInterface();

	//	Initializers
	void SettingsInterfaceInit(PageSharedPtr settings_page);
	int CommConnect();

	//	Screen update request methods
	void UpdateScreenBrightness();
	void UpdateScreenTimeout();
	void UpdateDawnStartTime();
	void UpdateDawnEndTime();
	void UpdateDuskStartTime();
	void UpdateDuskEndTime();
	void UpdateSunriseTime();
	void UpdateSunsetTime();
	void UpdateHeatingEnable();

};

#endif
