
#include "lighting_ui.h"
#include "vector_class.h"

//========================================================================================
//		Constructor and destructor
//========================================================================================

LightingInterface::LightingInterface()
{
	control_coid_ = -1;
}

LightingInterface::~LightingInterface()
{

}

//========================================================================================
//		Initializers
//========================================================================================

void LightingInterface::LightingInterfaceInit(PageSharedPtr lighting_page)
{
	//	Obtain local pointers
	lighting_page_ = lighting_page;

	//	Get page nodes
	entrypage_lightingmode_text_ = lighting_page_->lookupNode<TextBlock2D>("#entrypage_lightingmode_text");
	entrypage_lightingmode_text_->setText("Night"); // ##INITIAL DAY NIGHT MODE
	scroll_view_ = lighting_page_->lookupNode<ScrollView2D>("ScrollView");

	//Vector2 margin(-30.0, -30.0);
	//scroll_view_->setHeight(300.0);
	//Matrix3x3 mat = scroll_view_->getLayoutTransformation();
	//mat.setTranslationY(200.0);
	//scroll_view_->setLayoutTransformation(mat);

	lightwidgets_stack_ = lighting_page_->lookupNode<StackLayout2D>("LightWidgetsStack");

	//	Get custom properties
	//	Lightset
	lightsetwidget_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("LightSetWidgetIndex"));
	lightgroupwidget_index_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("LightGroupWidgetIndex"));
	lightwidget_cmsindex_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("LightWidgetCmsIndex"));
	lightwidget_cmsid_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("LightWidgetCmsID"));
	lightset_labeltext_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("LightSetWidgetName"));
	percentagebuttons_text_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("PlusMinusPercentageButtonsText"));

	//	Lightgroup
	lightgroup_labeltext_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("LightGroupWidgetName"));

	//	Foldable Group
	lightgroupfoldable_isfolded_ = IntDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzInt>("IsGroupFolded"));;
	lightgroupfoldable_height_ = FloatDynamicPropertyTypeSharedPtr(new DynamicPropertyType<kzFloat>("LightGroupFoldableWidgetHeight"));;
	lightgroupfoldable_labeltext_ = StringDynamicPropertyTypeSharedPtr(new DynamicPropertyType<string>("LightGroupFoldableWidgetName"));


	PopulateLightWidgets();
	InitMessageHandlers();
	//UpdateProfiles();
}

void LightingInterface::InitMessageHandlers()
{
	//	Get custom messages pointers
	ClickedCustomMessageSharedPtr minusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("MinusButtonPressed"));
	ClickedCustomMessageSharedPtr plusbutton_pressed = ClickedCustomMessageSharedPtr(new DynamicMessageType<ButtonConcept::ClickedMessageArguments>("PlusButtonPressed"));

	//	Create handlers for LightSetWidgets
	//	Iterates cms'
	for (unsigned int cms_it = 0; cms_it < cmsgroup_widgets_.size(); cms_it++)
	{
		for (unsigned int group_it = 0; group_it < cmsgroup_widgets_.at(cms_it).size(); group_it++)
		{
			//	Iterates lights
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_it).at(group_it).size(); light_it++)
			{
				//	Add handlers
				cmsgroup_widgets_.at(cms_it).at(group_it).at(light_it)->addMessageHandler(*minusbutton_pressed, bind(&LightingInterface::DecreaseLight, this, placeholders::_1));
				cmsgroup_widgets_.at(cms_it).at(group_it).at(light_it)->addMessageHandler(*plusbutton_pressed, bind(&LightingInterface::IncreaseLight, this, placeholders::_1));
			}
		}
	}

	//scroll_view_->addMessageFilter(ScrollView2D::ScrolledMessage, bind(&LightingInterface::ScrollViewScrolled, this, placeholders::_1));

}

int LightingInterface::PopulateLightWidgets()
{
	vector_class<Node2DSharedPtr> lightset_widgets;
	vector_class<vector_class<Node2DSharedPtr>> lightgroup_widgets;

	//	Empty the group widgets vector
	cmsgroup_widgets_.clear();
	lightgroup_widgets.clear();
	lightset_widgets.clear();

	//	Get custom prefabs
	PrefabTemplateSharedPtr lightset_widgetprefab = lighting_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://CMS/Prefabs/LightSetWidget");
	PrefabTemplateSharedPtr lightgroup_widgetprefab = lighting_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://CMS/Prefabs/LightGroupWidget");

	//	Populate the widgets vector with Lights information

	lightgroup_widgets.clear();
	//	Iterates groups
	for (unsigned int group_it = 0; group_it < CmsData::Instance()->cms_self_.lighting_.lightgroups_.size(); group_it++)
	{
			//	Get nodes
			Node2DSharedPtr lightgroup_widget = lightgroup_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).name);
			StackLayout2DSharedPtr lightgroup_stack = lightgroup_widget->lookupNode<StackLayout2D>("LightGroupStack");
			ToggleButtonGroup2DSharedPtr lightprofile_buttongroup = lightgroup_stack->lookupNode<ToggleButtonGroup2D>("LightingProfileButtonGroup");

			//	Set group properties
			lightgroup_widget->setProperty(*lightgroup_labeltext_, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).name);
			lightgroup_widget->setProperty(*lightgroupwidget_index_, group_it);
			lightgroup_widget->setProperty(*lightwidget_cmsid_, CmsData::Instance()->cms_self_.id);


			//	Makes THEATRE or SHOWER button available
			lightprofile_buttongroup->getChild(3)->setVisible(CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).has_theatre);
			lightprofile_buttongroup->getChild(3)->setHitTestable(CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).has_theatre);
			lightprofile_buttongroup->getChild(4)->setVisible(CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).has_shower);
			lightprofile_buttongroup->getChild(4)->setHitTestable(CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).has_shower);

			//	Toggle NIGHT on (initial state) - child0 for day, child2 for night ##INITIAL DAY NIGHT MODE
			lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 1);

			//	Set toggle buttons message handlers
			lightprofile_buttongroup->getChild(0)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
			lightprofile_buttongroup->getChild(1)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
			lightprofile_buttongroup->getChild(2)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
			lightprofile_buttongroup->getChild(3)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
			lightprofile_buttongroup->getChild(4)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));

			//	Clear the lightset_widgets_ vector;
			lightset_widgets.clear();

			//	Iterates Lights inside group
			for (unsigned int light_it = 0; light_it < CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
			{
				//	Instantiate light widget with proper values
				Node2DSharedPtr lightset_widget = lightset_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).name);
				lightset_widget->setProperty(*lightset_labeltext_, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).name);
				lightset_widget->setProperty(*lightsetwidget_index_, light_it);
				lightset_widget->setProperty(*lightgroupwidget_index_, group_it);
				lightset_widget->setProperty(*lightwidget_cmsid_, CmsData::Instance()->cms_self_.id);
				string value = to_string((kzInt)CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_it).lights.at(light_it).night_value) + "%"; // ##INITIAL DAY NIGHT MODE
				lightset_widget->setProperty(*percentagebuttons_text_, value);

				//	Stack on group
				lightgroup_stack->addChild(lightset_widget);

				//	Increases height to accomodate light widget
				lightgroup_widget->setHeight(lightgroup_widget->getHeight() + lightset_widget->getHeight() + 10.0);

				//	Push widget into vector
				lightset_widgets.push_back(lightset_widget);

			}

			//	Draw group on screen
			lightwidgets_stack_->addChild(lightgroup_widget);
			//	Increases height to accomodate group widget
			lightwidgets_stack_->setHeight(lightwidgets_stack_->getHeight() + lightgroup_widget->getHeight());
			//	Push group vector into vector
			lightgroup_widgets.push_back(lightset_widgets);
	}
	cmsgroup_widgets_.push_back(lightgroup_widgets);

	if (CmsData::Instance()->cms_self_.lighting_.master_switch_.has_master)
	{
		PrefabTemplateSharedPtr masterswitch_widgetprefab = lighting_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/MasterSwitchGroupWidget");
		//	Instantiate master switch and set up triggers
		Node2DSharedPtr master_switch = masterswitch_widgetprefab->instantiate<Node2D>("MasterSwitch");
		master_switch->lookupNode<Button2D>("#onbutton")->addMessageHandler(Button2D::ClickedMessage, bind(&LightingInterface::MasterSwitchOn, this, placeholders::_1));
		master_switch->lookupNode<Button2D>("#offbutton")->addMessageHandler(Button2D::ClickedMessage, bind(&LightingInterface::MasterSwitchOff, this, placeholders::_1));
		master_switch->lookupNode<TextBlock2D>("#buttonstitle")->setText("All Lights");

		lightwidgets_stack_->addChild(master_switch);
		lightwidgets_stack_->setHeight(lightwidgets_stack_->getHeight() + master_switch->getHeight());
	}

	//	If it's master, populate the foldable groups
	if (CmsData::Instance()->cms_self_.is_master)
	{
		PrefabTemplateSharedPtr lightgroupfoldable_widgetprefab = lighting_page_->getResourceManager()->acquireResource<PrefabTemplate>("kzb://cms/Prefabs/LightGroupFoldableWidget");
		for (unsigned int cms_it = 0; cms_it < CmsData::Instance()->cms_slaves_.size(); cms_it++)
		{
			Node2DSharedPtr lightgroupfoldable_widget = lightgroupfoldable_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_slaves_.at(cms_it).hostname);
			lightgroupfoldable_widget->setProperty(*lightgroupfoldable_labeltext_, CmsData::Instance()->cms_slaves_.at(cms_it).label);
			StackLayout2DSharedPtr lightgroupwidget_stack = lightgroupfoldable_widget->lookupNode<StackLayout2D>("LightGroupWidgetStack");

			lightgroup_widgets.clear();
			for (unsigned int group_it = 0; group_it < CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.size(); group_it++)
			{
				//	Get nodes
				Node2DSharedPtr lightgroup_widget = lightgroup_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).name);
				StackLayout2DSharedPtr lightgroup_stack = lightgroup_widget->lookupNode<StackLayout2D>("LightGroupStack");
				ToggleButtonGroup2DSharedPtr lightprofile_buttongroup = lightgroup_stack->lookupNode<ToggleButtonGroup2D>("LightingProfileButtonGroup");

				//	Set group properties
				lightgroup_widget->setProperty(*lightgroup_labeltext_, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).name);
				lightgroup_widget->setProperty(*lightgroupwidget_index_, group_it);
				lightgroup_widget->setProperty(*lightwidget_cmsid_, CmsData::Instance()->cms_slaves_.at(cms_it).id);

				//	Makes THEATRE or SHOWER button available
				lightprofile_buttongroup->getChild(3)->setVisible(CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).has_theatre);
				lightprofile_buttongroup->getChild(3)->setHitTestable(CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).has_theatre);
				lightprofile_buttongroup->getChild(4)->setVisible(CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).has_shower);
				lightprofile_buttongroup->getChild(4)->setHitTestable(CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).has_shower);

				//	Toggle NIGHT on (initial state) - child0 for day, child2 for night ##INITIAL DAY NIGHT MODE
				lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 1);

				//	Set toggle buttons message handlers
				lightprofile_buttongroup->getChild(0)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
				lightprofile_buttongroup->getChild(1)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
				lightprofile_buttongroup->getChild(2)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
				lightprofile_buttongroup->getChild(3)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));
				lightprofile_buttongroup->getChild(4)->addMessageHandler(ToggleButton2D::ClickedMessage, bind(&LightingInterface::ChangeProfile, this, placeholders::_1));

				//	Clear the lightset_widgets_ vector;
				lightset_widgets.clear();

				//	Iterates Lights inside group
				for (unsigned int light_it = 0; light_it < CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.size(); light_it++)
				{
					//	Instantiate light widget with proper values
					Node2DSharedPtr lightset_widget = lightset_widgetprefab->instantiate<Node2D>(CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).name);
					lightset_widget->setProperty(*lightset_labeltext_, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).name);
					lightset_widget->setProperty(*lightsetwidget_index_, light_it);
					lightset_widget->setProperty(*lightgroupwidget_index_, group_it);
					lightset_widget->setProperty(*lightwidget_cmsid_, CmsData::Instance()->cms_slaves_.at(cms_it).id);
					string value = to_string((kzInt)CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_it).lights.at(light_it).night_value) + "%"; // ##INITIAL DAY NIGHT MODE
					lightset_widget->setProperty(*percentagebuttons_text_, value);

					//	Stack on group
					lightgroup_stack->addChild(lightset_widget);

					//	Increases height to accomodate light widget
					lightgroup_widget->setHeight(lightgroup_widget->getHeight() + lightset_widget->getHeight() +10.0);

					//	Push widget into vector
					lightset_widgets.push_back(lightset_widget);

				}

				//	Draw group on screen
				lightgroupwidget_stack->addChild(lightgroup_widget);
				//	Increases height to accomodate group widget

				//lightgroupfoldable_widget->setHeight(lightgroupfoldable_widget->getHeight() + lightgroup_widget->getHeight());
				lightgroupfoldable_widget->setProperty(*lightgroupfoldable_height_, (lightgroupfoldable_widget->getProperty(*lightgroupfoldable_height_) + lightgroup_widget->getHeight()));
				//	Push group vector into vector
				lightgroup_widgets.push_back(lightset_widgets);
			}
			//	Set event trigger for folding froup
			lightgroupfoldable_widget->getChild(0)->lookupNode<Button2D>("TitleButton")->addMessageHandler(Button2D::ClickedMessage, bind(&LightingInterface::FoldLightGroup, this, placeholders::_1));
			//	Draw group on screen
			lightwidgets_stack_->addChild(lightgroupfoldable_widget);
			//	Increases height to accomodate group widget
			lightwidgets_stack_->setHeight(lightwidgets_stack_->getHeight() + lightgroupfoldable_widget->getHeight());

			cmsgroup_widgets_.push_back(lightgroup_widgets);
		}

	}

	//	Set scroll boundaries
	if (lightwidgets_stack_->getHeight() > 480.0)
	{
		Vector2 scrollbounds(0.0, lightwidgets_stack_->getHeight() - 480.0);
		scroll_view_->setScrollBoundsMaximum(scrollbounds);
	}
	else
	{
		Vector2 scrollbounds(0.0, 0.0);
		scroll_view_->setScrollBoundsMaximum(scrollbounds);
	}

	scroll_view_->setSlidingAccelerationCoefficient(SCROLL_SLIDACC);
	scroll_view_->setSlidingDragCoefficient(SCROLL_SLIDDRAG);
	scroll_view_->setDraggingImpulseFactor(SCROLL_DRAGIMP);
	scroll_view_->setDraggingAccelerationCoefficient(SCROLL_DRAGACC);
	scroll_view_->setDraggingDragCoefficient(SCROLL_DRAGDRAG);
	scroll_view_->setRecognitionThreshold(SCROLL_THRESHOLD);
	scroll_view_->setLoopingY(false);

	return 0;
}

int LightingInterface::CommConnect()
{
	control_coid_ = name_open(CmsData::Instance()->cms_self_.hostname.c_str(), NAME_FLAG_ATTACH_GLOBAL);
	return control_coid_;
}

//========================================================================================
//		Screen setters
//========================================================================================

void LightingInterface::UpdateProfile(const unsigned char cms_id, const unsigned char group_index)
{
	unsigned int cms_index;
	//	If it's changing itself
	if (cms_id == CmsData::Instance()->self_id_)
	{
		cms_index = 0;
	}
	//	If it's a master changing a slave, find the index for the given id
	else
	{
		for (cms_index = 1; cms_index < cmsgroup_widgets_.size(); cms_index ++)
		{
			if(group_index >= cmsgroup_widgets_.at(cms_index).size())
				continue;
			if(cmsgroup_widgets_.at(cms_index).at(group_index).at(0)->getProperty(*lightwidget_cmsid_) == cms_id)
				break;
		}
	}

	if (cms_id == CmsData::Instance()->self_id_)
	{
		ToggleButtonGroup2DSharedPtr lightprofile_buttongroup = cmsgroup_widgets_.at(0).at(group_index).at(0)->getParent()->lookupNode<ToggleButtonGroup2D>("LightingProfileButtonGroup");

		switch (CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_index).profile)
		{
		case Lighting::DAY:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).day_value);
			}
			lightprofile_buttongroup->getChild(0)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Day");
			break;

		case Lighting::EVE:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).eve_value);
			}
			lightprofile_buttongroup->getChild(1)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Eve");
			break;

		case Lighting::NIGHT:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).night_value);
			}
			lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Night");
			break;

		case Lighting::THEATRE:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).theatre_value);
			}
			lightprofile_buttongroup->getChild(3)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Theatre");
			break;

		case Lighting::SHOWER:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).shower_value);
			}
			lightprofile_buttongroup->getChild(4)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Shower");
			break;

		case Lighting::MANUAL:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_self_.lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value);
			}
			//	Deselect all profile buttons
			lightprofile_buttongroup->getChild(0)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(1)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(3)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(4)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Manual");
			break;

		case Lighting::OFF:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetDashOnScreen(cms_id, group_index, light_it);
			}
			//	Deselect all profile buttons
			lightprofile_buttongroup->getChild(0)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(1)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(3)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(4)->setProperty(ToggleButton2D::ToggleStateProperty, 0);

			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Off");
			break;
		}
	}
	else
	{
		//	Find the vector index for the given id
		unsigned int cms_it = 0;
		while (CmsData::Instance()->cms_slaves_.at(cms_it).id != cms_id)
		{
			//	If it ran the whole vector and did not find
			if (++cms_it >= CmsData::Instance()->cms_slaves_.size())
			{
				cms_it = 0;
				break;
			}
		}

		for (cms_index = 1; cms_index < cmsgroup_widgets_.size(); cms_index ++)
		{
			if(group_index >= cmsgroup_widgets_.at(cms_index).size())
				continue;
			if(cmsgroup_widgets_.at(cms_index).at(group_index).at(0)->getProperty(*lightwidget_cmsid_) == cms_id)
				break;
		}
		ToggleButtonGroup2DSharedPtr lightprofile_buttongroup = cmsgroup_widgets_.at(cms_index).at(group_index).at(0)->getParent()->lookupNode<ToggleButtonGroup2D>("LightingProfileButtonGroup");

		switch (CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_index).profile)
		{
		case Lighting::DAY:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_index).lights.at(light_it).day_value);
			}
			lightprofile_buttongroup->getChild(0)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			break;

		case Lighting::EVE:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_index).lights.at(light_it).eve_value);
			}
			lightprofile_buttongroup->getChild(1)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			break;

		case Lighting::NIGHT:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_index).lights.at(light_it).night_value);
			}
			lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			break;

		case Lighting::THEATRE:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_index).lights.at(light_it).theatre_value);
			}
			lightprofile_buttongroup->getChild(3)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			break;

		case Lighting::SHOWER:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_index).lights.at(light_it).shower_value);
			}
			lightprofile_buttongroup->getChild(4)->setProperty(ToggleButton2D::ToggleStateProperty, 1);
			break;

		case Lighting::MANUAL:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetValueOnScreen(cms_id, group_index, light_it, CmsData::Instance()->cms_slaves_.at(cms_it).lighting_.lightgroups_.at(group_index).lights.at(light_it).manual_value);
			}
			//	Deselect all profile buttons
			lightprofile_buttongroup->getChild(0)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(1)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(3)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(4)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Manual");
			break;

		case Lighting::OFF:
			//	Iterates through lights and change for the correct profile value
			for (unsigned int light_it = 0; light_it < cmsgroup_widgets_.at(cms_index).at(group_index).size(); light_it++)
			{
				SetDashOnScreen(cms_id, group_index, light_it);
			}
			//	Deselect all profile buttons
			lightprofile_buttongroup->getChild(0)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(1)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(2)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(3)->setProperty(ToggleButton2D::ToggleStateProperty, 0);
			lightprofile_buttongroup->getChild(4)->setProperty(ToggleButton2D::ToggleStateProperty, 0);

			//	Set entry screen with main group value
			if (group_index == 0)
				entrypage_lightingmode_text_->setText("Off");
			break;
		}
	}
}

void LightingInterface::UpdateAllProfiles()
{

}

void LightingInterface::SetDashOnScreen(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index)
{
	unsigned int cms_index;
	//	If it's changing itself
	if (cms_id == CmsData::Instance()->self_id_)
	{
		cms_index = 0;
	}
	//	If it's a master changing a slave, find the index for the given id
	else
	{
		for (cms_index = 1; cms_index < cmsgroup_widgets_.size(); cms_index ++)
		{
			if(group_index >= cmsgroup_widgets_.at(cms_index).size())
				continue;
			if(cmsgroup_widgets_.at(cms_index).at(group_index).at(0)->getProperty(*lightwidget_cmsid_) == cms_id)
				break;
		}
	}
	cmsgroup_widgets_.at(cms_index).at(group_index).at(light_index)->setProperty(*percentagebuttons_text_, "- %");
}

void LightingInterface::SetValueOnScreen(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index, int new_value)
{
	if(new_value > 100)
		new_value = 100;
	else if(new_value < 0)
		new_value = 0;


	unsigned int cms_index;
	//	If it's changing itself
	if (cms_id == CmsData::Instance()->self_id_)
	{
		cms_index = 0;
	}
	//	If it's a master changing a slave, find the index for the given id
	else
	{
		for (cms_index = 1; cms_index < cmsgroup_widgets_.size(); cms_index ++)
		{
			if(group_index >= cmsgroup_widgets_.at(cms_index).size())
				continue;
			if(cmsgroup_widgets_.at(cms_index).at(group_index).at(0)->getProperty(*lightwidget_cmsid_) == cms_id)
				break;
		}
	}

	//	Cast integer to not print ASCII
	string text_value = to_string(new_value) + "%";
	//	Write on screen
	cmsgroup_widgets_.at(cms_index).at(group_index).at(light_index)->setProperty(*percentagebuttons_text_, text_value);
}

void LightingInterface::UpdateLightValue(const unsigned char cms_id, const unsigned char group_index, const unsigned char light_index)
{
	SetValueOnScreen(cms_id, group_index, light_index, CmsData::Instance()->GetLightValue(cms_id, group_index, light_index));
}
//========================================================================================
//		Value manipulation functions
//========================================================================================

void LightingInterface::DecreaseLight(ButtonConcept::ClickedMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char light_index = argument.getSource()->getProperty(*lightsetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*lightgroupwidget_index_);
	unsigned char cms_id = argument.getSource()->getProperty(*lightwidget_cmsid_);

	if(CmsData::Instance()->GetLightProfile(cms_id, group_index) != Lighting::OFF)
	{
		unsigned char msg[8];
		unsigned char reply_msg[8];

		msg[0] = SCREEN;						//	From
		msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
		msg[2] = LIGHTING;						//	What
		msg[3] = (unsigned char)group_index;				//	Group Index
		msg[4] = (unsigned char)light_index;				//	Light Index
		msg[5] = (unsigned char)cms_id;					//	Cms ID
		msg[6] = (unsigned char)0x00;							//	N/A
		msg[7] = (unsigned char)0x00;							//	Down

		MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
	}
}

void LightingInterface::IncreaseLight(ButtonConcept::ClickedMessageArguments& argument)
{
	//	Get the index of who triggered the event
	unsigned char light_index = argument.getSource()->getProperty(*lightsetwidget_index_);
	unsigned char group_index = argument.getSource()->getProperty(*lightgroupwidget_index_);
	unsigned char cms_id = argument.getSource()->getProperty(*lightwidget_cmsid_);

	if(CmsData::Instance()->GetLightProfile(cms_id, group_index) != Lighting::OFF)
	{
		unsigned char msg[8];
		unsigned char reply_msg[8];

		msg[0] = SCREEN;						//	From
		msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
		msg[2] = LIGHTING;						//	What
		msg[3] = (unsigned char)group_index;					//	Group Index
		msg[4] = (unsigned char)light_index;					//	Light Index
		msg[5] = (unsigned char)cms_id;						//	Cms ID
		msg[6] = (unsigned char)0x00;							//	N/A
		msg[7] = (unsigned char)0x01;							//	Up

		MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
	}
}

void LightingInterface::ChangeProfile(ToggleButton2D::ClickedMessageArguments& argument)
{

	unsigned char group_index = argument.getSource()->getParent()->getParent()->getParent()->getProperty(*lightgroupwidget_index_);
	unsigned char cms_id = argument.getSource()->getParent()->getParent()->getParent()->getProperty(*lightwidget_cmsid_);
	unsigned char profile_index = argument.getSource()->getProperty(ToggleButton2D::IndexInGroupProperty);

	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = LIGHTING_PROFILE;				//	What
	msg[3] = (unsigned char)group_index;					//	Group Index
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)cms_id;						//	Cms ID
	msg[6] = (unsigned char)0xFF;							//	N/A
	msg[7] = (unsigned char)profile_index;				//	profile

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));

}

void LightingInterface::MasterSwitchOn(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = LIGHTING;				//	What
	msg[3] = (unsigned char)0xFF;					//	Group Index
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;						//	Cms ID
	msg[6] = (unsigned char)0x01;							//	N/A
	msg[7] = (unsigned char)0x01;				//	profile

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void LightingInterface::MasterSwitchOff(ButtonConcept::ClickedMessageArguments& argument)
{
	unsigned char msg[8];
	unsigned char reply_msg[8];

	msg[0] = SCREEN;						//	From
	msg[1] = (unsigned char)CmsData::Instance()->self_id_;	//	To
	msg[2] = LIGHTING;				//	What
	msg[3] = (unsigned char)0xFF;					//	Group Index
	msg[4] = (unsigned char)0xFF;							//	N/A
	msg[5] = (unsigned char)0xFF;						//	Cms ID
	msg[6] = (unsigned char)0x01;							//	N/A
	msg[7] = (unsigned char)0x00;				//	profile

	MsgSend(control_coid_, msg, 8, reply_msg, sizeof(reply_msg));
}

void LightingInterface::FoldLightGroup(ButtonConcept::ClickedMessageArguments& argument)
{
	argument.setHandled(false);
	if (argument.getSource()->getParent()->getParent()->getProperty(*lightgroupfoldable_isfolded_) == 1)
	{
		Vector2 scrollbounds(0.0, scroll_view_->getScrollBoundsMaximum().getY() + argument.getSource()->getParent()->getParent()->getProperty(*lightgroupfoldable_height_) - 70.0);
		scroll_view_->setScrollBoundsMaximum(scrollbounds);
	}
	else
	{
		Vector2 scrollbounds(0.0, scroll_view_->getScrollBoundsMaximum().getY() - argument.getSource()->getParent()->getParent()->getProperty(*lightgroupfoldable_height_) + 70.0);
		scroll_view_->setScrollBoundsMaximum(scrollbounds);
	}
}

void LightingInterface::ScrollViewScrolled(ScrollView2D::ScrollMessageArguments& argument)
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
