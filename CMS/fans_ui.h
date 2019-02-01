#ifndef CMS_FANSINTERFACE_H_
#define CMS_FANSINTERFACE_H_

#include <kanzi/kanzi.hpp>
#include "vector_class.h"

#include "common.h"
#include "cms_data.h"

using namespace kanzi;



class FansInterface
{
private:
	//========================================================================================
	//		Variables
	//========================================================================================

	//	Pointer to Fans page
	PageSharedPtr fans_page_;

	ScrollView2DSharedPtr scroll_view_;

	//	Pointer to custom properties
	//	Fansgroup
	StringDynamicPropertyTypeSharedPtr fansgroup_labeltext_;

	//	Fanset
	IntDynamicPropertyTypeSharedPtr fansetwidget_index_;
	IntDynamicPropertyTypeSharedPtr fangroupwidget_index_;
	StringDynamicPropertyTypeSharedPtr fanset_labeltext_;
	StringDynamicPropertyTypeSharedPtr percentagebuttons_text_;


	//	Vector of vector pointers to lights instantiated by code
	//	Two layers to separate group and light
	vector_class<vector_class<Node2DSharedPtr>> fansgroup_widgets_;

	int control_coid_;
	//========================================================================================
	//		Methods
	//========================================================================================

	//	Initializers
	void SetInitValues();
	void InitMessageHandlers();
	int PopulateFansWidgets();

	//	Screen setters
	void SetValueOnScreen(const unsigned char group_index, const unsigned char fan_index, const unsigned char new_value);

	// Event response methods
	void DecreaseFan(ButtonConcept::ClickedMessageArguments& argument);
	void IncreaseFan(ButtonConcept::ClickedMessageArguments& argument);
	void ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument);

public:
	//========================================================================================
	//		Methods
	//========================================================================================
	FansInterface();
	~FansInterface();

	void FansInterfaceInit(PageSharedPtr fans_page);
	int CommConnect();

	void UpdateFanValue(const unsigned char group_index, const unsigned char fan_index);
};

#endif
