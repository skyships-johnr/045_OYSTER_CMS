#ifndef CMS_BLINDSINTERFACE_H_
#define CMS_BLINDSINTERFACE_H_

#include <kanzi/kanzi.hpp>
#include "vector_class.h"

#include "common.h"
#include "cms_data.h"


using namespace kanzi;


class BlindsInterface
{
private:

	//========================================================================================
	//		Variables
	//========================================================================================

	//	Pointer to Blinds page
	PageSharedPtr blinds_page_;

	//	Pointers to nodes
	StackLayout2DSharedPtr blindswidgets_stack_;
	ScrollView2DSharedPtr scroll_view_;

	//	Pointers to custom properties
	IntDynamicPropertyTypeSharedPtr blindssetwidget_index_;
	IntDynamicPropertyTypeSharedPtr blindsgroupwidget_index_;
	StringDynamicPropertyTypeSharedPtr blindsset_labeltext_;
	StringDynamicPropertyTypeSharedPtr blindsgroup_labeltext_;

	//	Pointer to blinds setting prefab
	PrefabTemplateSharedPtr blindsset_widgetprefab_;
	PrefabTemplateSharedPtr blindsgroup_widgetprefab_;

	// Vector of pointers to blinds instantiated by code
	vector_class<vector_class<Node2DSharedPtr>> blindsgroup_widgets_;

	int control_coid_;

	//========================================================================================
	//		Methods
	//========================================================================================

	//	Initializers
	void InitMessageHandlers();
	int PopulateBlindsWidgets();

	//	Event response methods
	void OpenBlinds(ButtonConcept::PressedMessageArguments& argument);
	void OpenBlindsStop(ButtonConcept::CanceledMessageArguments& argument);
	void CloseBlinds(ButtonConcept::PressedMessageArguments& argument);
	void CloseBlindsStop(ButtonConcept::CanceledMessageArguments& argument);
	void ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument);

public:

	//========================================================================================
	//		Methods
	//========================================================================================

	BlindsInterface();
	~BlindsInterface();

	void BlindsInterfaceInit(PageSharedPtr blinds_page);
	int CommConnect();
};


#endif
