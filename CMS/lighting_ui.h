#ifndef CMS_LIGHTINGINTERFACE_H_
#define CMS_LIGHTINGINTERFACE_H_

#include <vector>

#include <kanzi/kanzi.hpp>

#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>

#include "common.h"
#include "cms_data.h"
#include "vector_class.h"

using namespace kanzi;



class LightingInterface
{
private:
	//========================================================================================
	//		Variables
	//========================================================================================

	//	Pointer to Lighting page
	PageSharedPtr lighting_page_;

	int control_coid_;

	//	Pointers to fixed nodes
	TextBlock2DSharedPtr entrypage_lightingmode_text_;
	ScrollView2DSharedPtr scroll_view_;
	StackLayout2DSharedPtr lightwidgets_stack_;

	//	Pointer to custom properties
	//	Lightset
	IntDynamicPropertyTypeSharedPtr lightsetwidget_index_;
	IntDynamicPropertyTypeSharedPtr lightgroupwidget_index_;
	IntDynamicPropertyTypeSharedPtr lightwidget_cmsindex_;
	IntDynamicPropertyTypeSharedPtr lightwidget_cmsid_;
	StringDynamicPropertyTypeSharedPtr lightset_labeltext_;
	StringDynamicPropertyTypeSharedPtr percentagebuttons_text_;

	//	Lightgroup
	StringDynamicPropertyTypeSharedPtr lightgroup_labeltext_;

	//	Foldable Group
	IntDynamicPropertyTypeSharedPtr lightgroupfoldable_isfolded_;
	FloatDynamicPropertyTypeSharedPtr lightgroupfoldable_height_;
	StringDynamicPropertyTypeSharedPtr lightgroupfoldable_labeltext_;


	//	Vector of vector pointers to lights instantiated by code
	//	Two layers to separate group and light
	vector_class<vector_class<vector_class<Node2DSharedPtr>>> cmsgroup_widgets_;


	//========================================================================================
	//		Methods
	//========================================================================================

	//	Initializers
	void InitMessageHandlers();
	int PopulateLightWidgets();

	// Event response functions
	void DecreaseLight(ButtonConcept::ClickedMessageArguments& argument);
	void IncreaseLight(ButtonConcept::ClickedMessageArguments& argument);
	void ChangeProfile(ToggleButton2D::ClickedMessageArguments& argument);
	void FoldLightGroup(ButtonConcept::ClickedMessageArguments& argument);
	void MasterSwitchOn(ButtonConcept::ClickedMessageArguments& argument);
	void MasterSwitchOff(ButtonConcept::ClickedMessageArguments& argument);
	void ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument);


public:
	//========================================================================================
	//		Methods
	//========================================================================================
	LightingInterface();
	~LightingInterface();

	void LightingInterfaceInit(PageSharedPtr lighting_page);
	int CommConnect();

	//	Screen setters
	void UpdateProfile(const unsigned char cms_id, const unsigned char group_index);
	void UpdateAllProfiles();
	void SetMasterSwitch(const bool power);
	void SetValueOnScreen(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index, const int new_value);
	void SetDashOnScreen(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index);
	void UpdateLightValue(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index);
};

#endif
