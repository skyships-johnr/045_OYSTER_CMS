#include "fans_ui.h"
#include "vector_class.h"

//========================================================================================
//		Constructor and destructor
//========================================================================================

FansInterface::FansInterface()
{

}

FansInterface::~FansInterface()
{

}

//========================================================================================
//		Initializers
//========================================================================================

void FansInterface::FansInterfaceInit(PageSharedPtr fans_page)
{
	//	Copy local pointers
	fans_page_ = fans_page;

	scroll_view_ = fans_page_->lookupNode<ScrollView2D>("ScrollView");

	//	Get custom properties
	fansetwidget_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("WidgetIndex"));
	fangroupwidget_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("GroupIndex"));

	percentagebuttons_text_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("PlusMinusPercentageButtonsText"));

	PopulateFansWidgets();
	InitMessageHandlers();
}

void FansInterface::InitMessageHandlers()
{
	//	Get custom messages pointers
	ClickedCustomMessageSharedPtr minusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("MinusButtonPressed"));
	ClickedCustomMessageSharedPtr plusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("PlusButtonPressed"));

	//	Create handlers for LightSetWidgets
	//	Iterates groups
	for (unsigned int group_it = 0; group_it < fansgroup_widgets_.size(); group_it++)
	{
		//	Iterates lights
		for (unsigned int fan_it = 0; fan_it < fansgroup_widgets_.at(group_it).size(); fan_it++)
		{
			//	Add handlers
			fansgroup_widgets_.at(group_it).at(fan_it)->addMessageHandler(*minusbutton_pressed, bind(&FansInterface::DecreaseFan, this, placeholders::_1));
			fansgroup_widgets_.at(group_it).at(fan_it)->addMessageHandler(*plusbutton_pressed, bind(&FansInterface::IncreaseFan, this, placeholders::_1));
		}
	}

	//scroll_view_->addMessageFilter(ScrollView2D::ScrolledMessage, bind(&FansInterface::ScrollViewScrolled, this, placeholders::_1));
}

int FansInterface::PopulateFansWidgets()
{
	vector_class<Node2DSharedPtr> fansset_widgets;

	//	Empty the group widgets vector
	fansgroup_widgets_.clear();

	//	Pointer to set the scrolling boundaries
	StackLayout2DSharedPtr fanswidgets_stack = fans_page_->lookupNode<StackLayout2D>("FansWidgetsStack");

	//	Get prefabs
	PrefabTemplateSharedPtr fansgroup_widgetprefab = fans_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/FansGroupWidget");
	PrefabTemplateSharedPtr fanset_widgetprefab = fans_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/FanSetWidget");

	//	Get custom properties
	StringDynamicPropertyTypeSharedPtr fansgroup_labeltext = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("FansGroupWidgetName"));
	StringDynamicPropertyTypeSharedPtr fanset_labeltext = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("FanSetWidgetName"));

	fansgroup_widgets_.clear();
	//	Populate the widgets vector with Fans information
	//	Iterates groups
	for (unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.fans_.fansgroups_.size(); group_it++)
	{
		//	Get nodes
		Node2DSharedPtr fangroup_widget = fansgroup_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.fans_.fansgroups_.at(group_it).name);
		StackLayout2DSharedPtr fans_stack = fangroup_widget->lookupNode<StackLayout2D>("FansStack");

		//	Set group properties
		fangroup_widget->setProperty(*fansgroup_labeltext, CmsData::Instance()->cms_self_.fans_.fansgroups_.at(group_it).name);

		fansset_widgets.clear();

		//	Iterates Fans inside group
		for (unsigned int fan_it = 0; fan_it < CmsData::Instance()->cms_self_.fans_.fansgroups_.at(group_it).fans.size(); fan_it++)
		{
			//	Instantiate fan widget with proper values
			Node2DSharedPtr fanset_widget = fanset_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.fans_.fansgroups_.at(group_it).fans.at(fan_it).name);
			fanset_widget->setProperty(*fanset_labeltext, CmsData::Instance()->cms_self_.fans_.fansgroups_.at(group_it).fans.at(fan_it).name);
			fanset_widget->setProperty(*fansetwidget_index_, fan_it);
			fanset_widget->setProperty(*fangroupwidget_index_, group_it);
			string value = to_string((int)CmsData::Instance()->cms_self_.fans_.fansgroups_.at(group_it).fans.at(fan_it).level) + "%";
			fanset_widget->setProperty(*percentagebuttons_text_, value);

			//	Stack on group
			fans_stack->addChild(fanset_widget);

			//	Increases height to accomodate light widget
			fangroup_widget->setHeight(fangroup_widget->getHeight() + fanset_widget->getHeight() + 10.0);

			//	Push widget into vector
			fansset_widgets.push_back(fanset_widget);

		}

		//	Draw group on screen
		fanswidgets_stack->addChild(fangroup_widget);
		//	Increases height to accomodate group widget
		fanswidgets_stack->setHeight(fanswidgets_stack->getHeight() + fangroup_widget->getHeight()+10.0);
		//	Push group vector into vector
		fansgroup_widgets_.push_back(fansset_widgets);
	}

	//	Set scroll boundaries
	if (fanswidgets_stack->getHeight() > 480.0)
		scroll_view_->setScrollBoundsY(0.0, fanswidgets_stack->getHeight() - 480.0);
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

int FansInterface::CommConnect()
{
	control_coid_ = name_open(CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);
	return control_coid_;
}

//========================================================================================
//		Screen setters
//========================================================================================

void FansInterface::SetValueOnScreen(const unsigned char group_index, const unsigned char light_index, const unsigned char new_value)
{
	//	Cast integer to not print ASCII
	string text_value = to_string((int)new_value) + "%";
	//	Write on screen
	fansgroup_widgets_.at(group_index).at(light_index)->setProperty(*percentagebuttons_text_, text_value);
}

void FansInterface::UpdateFanValue(const unsigned char group_index, const unsigned char fan_index)
{
	SetValueOnScreen(group_index, fan_index, CmsData::Instance()->GetFansValue(group_index, fan_index));
}

//========================================================================================
//		Value manipulation functions
//========================================================================================

void FansInterface::DecreaseFan(ButtonConcept::ClickedMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char fan_index = argument.getSource()->getProperty(*fansetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*fangroupwidget_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = FANS;							//	What
	msg[3] = (unsigned char)group_index;				//	Group Index
	msg[4] = (unsigned char)fan_index;				//	Fan Index
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x00;							//	N/A
	msg[7] = (unsigned char)0x00;							//	Decrease

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void FansInterface::IncreaseFan(ButtonConcept::ClickedMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char fan_index = argument.getSource()->getProperty(*fansetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*fangroupwidget_index_);


	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = FANS;							//	What
	msg[3] = (unsigned char)group_index;				//	Group Index
	msg[4] = (unsigned char)fan_index;				//	Fan Index
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x00;							//	N/A
	msg[7] = (unsigned char)0x01;							//	Increase

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void FansInterface::ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument)
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
