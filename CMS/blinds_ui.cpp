#include "blinds_ui.h"
#include "vector_class.h"

//========================================================================================
//		Constructor and destructor
//========================================================================================

BlindsInterface::BlindsInterface()
{
	control_coid_= -1;
}

BlindsInterface::~BlindsInterface()
{

}

//========================================================================================
//		Initializers
//========================================================================================

void BlindsInterface::BlindsInterfaceInit(PageSharedPtr blinds_page)
{
	//	Copy local pointers
	blinds_page_ = blinds_page;
	scroll_view_ = blinds_page_->lookupNode<ScrollView2D>("ScrollView");

	//	Get custom properties
	blindssetwidget_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("BlindsSetWidgetIndex"));
	blindsgroupwidget_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("BlindsGroupWidgetIndex"));
	blindsset_labeltext_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("BlindsSetLabel"));
	blindsgroup_labeltext_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("BlindsGroupLabel"));

	PopulateBlindsWidgets();
	InitMessageHandlers();
}


void BlindsInterface::InitMessageHandlers()
{
	//	Get custom messages pointers
	PressedCustomMessageSharedPtr openbutton_pressed = PressedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::PressedMessageArguments>("OpenBlindsMessage"));
	PressedCustomMessageSharedPtr closebutton_pressed = PressedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::PressedMessageArguments>("CloseBlindsMessage"));
	CanceledCustomMessageSharedPtr openbutton_released = CanceledCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::CanceledMessageArguments>("OpenBlindsStopMessage"));
	CanceledCustomMessageSharedPtr closebutton_released = CanceledCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::CanceledMessageArguments>("CloseBlindsStopMessage"));

	//Blinds::BlindsGroup current_blindsgroup;
	//	Set message handlers
	for (unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.blinds_.blindsgroups_.size(); group_it++)
	{
		//	Iterates through blinds
		for (unsigned int blinds_it = 0; blinds_it < CmsData::Instance()->cms_self_.blinds_.blindsgroups_.at(group_it).blinds.size(); blinds_it++)
		{
			blindsgroup_widgets_.at(group_it).at(blinds_it)->addMessageHandler(*openbutton_pressed, bind(&BlindsInterface::OpenBlinds, this, placeholders::_1));
			blindsgroup_widgets_.at(group_it).at(blinds_it)->addMessageHandler(*openbutton_released, bind(&BlindsInterface::OpenBlindsStop, this, placeholders::_1));
			blindsgroup_widgets_.at(group_it).at(blinds_it)->addMessageHandler(*closebutton_pressed, bind(&BlindsInterface::CloseBlinds, this, placeholders::_1));
			blindsgroup_widgets_.at(group_it).at(blinds_it)->addMessageHandler(*closebutton_released, bind(&BlindsInterface::CloseBlindsStop, this, placeholders::_1));
		}
	}

	//scroll_view_->addMessageFilter(ScrollView2D::ScrolledMessage, bind(&BlindsInterface::ScrollViewScrolled, this, placeholders::_1));
}


int BlindsInterface::PopulateBlindsWidgets()
{
	//	Temp vector for storing blinds
	vector_class<Node2DSharedPtr> blindsset_widgets;

	//	Empty the widgets vector
	blindsgroup_widgets_.clear();
	//Blinds::BlindsGroup current_blindsgroup;

	StackLayout2DSharedPtr blindswidgets_stack = blinds_page_->lookupNode<StackLayout2D>("#blindswidgets_stack");

	//	Get custom prefabs
	PrefabTemplateSharedPtr blindsset_widgetprefab = blinds_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://CMS/Prefabs/BlindsSetWidget");
	PrefabTemplateSharedPtr blindsgroup_widgetprefab = blinds_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://CMS/Prefabs/BlindsGroupWidget");

	//	Populate the widgets vector with Blinds information
	for (unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.blinds_.blindsgroups_.size(); group_it++)
	{
			//	Get nodes
			Node2DSharedPtr blindsgroup_widget = blindsgroup_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.blinds_.blindsgroups_.at(group_it).name);
			StackLayout2DSharedPtr blinds_stack = blindsgroup_widget->lookupNode<StackLayout2D>("BlindsStack");

			//	Set group properties
			blindsgroup_widget->setProperty(*blindsgroup_labeltext_, CmsData::Instance()->cms_self_.blinds_.blindsgroups_.at(group_it).name);

			//	Clear the temp vector
			blindsset_widgets.clear();
			for (unsigned int blinds_it = 0; blinds_it < CmsData::Instance()->cms_self_.blinds_.blindsgroups_.at(group_it).blinds.size(); blinds_it++)
			{
				//	Instantiate widget
				Node2DSharedPtr blindsset_widget = blindsset_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).name);
				//	Copy properties
				blindsset_widget->setProperty(*blindsset_labeltext_, CmsData::Instance()->cms_self_.blinds_.blindsgroups_.at(group_it).blinds.at(blinds_it).name);
				blindsset_widget->setProperty(*blindssetwidget_index_, blinds_it);
				blindsset_widget->setProperty(*blindsgroupwidget_index_, group_it);


				//	Push into screen stack
				blinds_stack->addChild(blindsset_widget);

				//	Push into temp vector
				blindsset_widgets.push_back(blindsset_widget);

				//	Increase widget height
				blindsgroup_widget->setHeight(blindsgroup_widget->getHeight() + blindsset_widget->getHeight() + 10);
			}

			//	After group is populated
			//	Push into screen
			blindswidgets_stack->addChild(blindsgroup_widget);
			//	Increases height to accomodate group widget
			blindswidgets_stack->setHeight(blindswidgets_stack->getHeight() + blindsgroup_widget->getHeight());

			//	Push into groups vector
			blindsgroup_widgets_.push_back(blindsset_widgets);
	}
	//	Fill screen if it was not already
	if (blindswidgets_stack->getHeight() < 480.0)
		blindswidgets_stack->setHeight(480.0);

	//	Set scroll boundaries
	scroll_view_->setScrollBoundsY(0.0, blindswidgets_stack->getHeight() - 480.0);

	scroll_view_->setSlidingAccelerationCoefficient(SCROLL_SLIDACC);
	scroll_view_->setSlidingDragCoefficient(SCROLL_SLIDDRAG);
	scroll_view_->setDraggingAccelerationCoefficient(SCROLL_DRAGACC);
	scroll_view_->setDraggingDragCoefficient(SCROLL_DRAGDRAG);
	scroll_view_->setRecognitionThreshold(SCROLL_THRESHOLD);
	scroll_view_->setDraggingImpulseFactor(SCROLL_DRAGIMP);

	return 0;
}

int BlindsInterface::CommConnect()
{
	control_coid_ = name_open(CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);
	return control_coid_;
}


//========================================================================================
//		Trigger manipulation functions
//========================================================================================

void BlindsInterface::OpenBlinds(ButtonConcept::PressedMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char blinds_index = argument.getSource()->getProperty(*blindssetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*blindsgroupwidget_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = BLINDS;						//	What
	msg[3] = (unsigned char)group_index;					//	Group Index
	msg[4] = (unsigned char)blinds_index;					//	Blinds Index
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x01;							//	Open
	msg[7] = (unsigned char)0x01;							//	Start

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));

}

void BlindsInterface::OpenBlindsStop(ButtonConcept::CanceledMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char blinds_index = argument.getSource()->getProperty(*blindssetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*blindsgroupwidget_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = BLINDS;						//	What
	msg[3] = (unsigned char)group_index;					//	Group Index
	msg[4] = (unsigned char)blinds_index;					//	Blinds Index
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x01;							//	Open
	msg[7] = (unsigned char)0x00;							//	Stop

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void BlindsInterface::CloseBlinds(ButtonConcept::PressedMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char blinds_index = argument.getSource()->getProperty(*blindssetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*blindsgroupwidget_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = BLINDS;						//	What
	msg[3] = (unsigned char)group_index;					//	Group Index
	msg[4] = (unsigned char)blinds_index;					//	Blinds Index
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x00;							//	Close
	msg[7] = (unsigned char)0x01;							//	Start

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void BlindsInterface::CloseBlindsStop(ButtonConcept::CanceledMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char blinds_index = argument.getSource()->getProperty(*blindssetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*blindsgroupwidget_index_);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = BLINDS;						//	What
	msg[3] = (unsigned char)group_index;					//	Group Index
	msg[4] = (unsigned char)blinds_index;					//	Blinds Index
	msg[5] = (unsigned char)0xFF;							//	N/A
	msg[6] = (unsigned char)0x00;							//	Close
	msg[7] = (unsigned char)0x00;							//	Stop

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void BlindsInterface::ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument)
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
