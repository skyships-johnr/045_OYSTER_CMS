#ifndef CMS_COMMON_H_
#define CMS_COMMON_H_

#include <kanzi/kanzi.hpp>

using namespace kanzi;

//	Typedefs for custom property types
typedef shared_ptr<DynamicPropertyType<kzBool>> BoolDynamicPropertyTypeSharedPtr;
typedef shared_ptr<DynamicPropertyType<kzInt>> IntDynamicPropertyTypeSharedPtr;
typedef shared_ptr<DynamicPropertyType<kzUint>> UintDynamicPropertyTypeSharedPtr;
typedef shared_ptr<DynamicPropertyType<kzFloat>> FloatDynamicPropertyTypeSharedPtr;
typedef shared_ptr<DynamicPropertyType<string>> StringDynamicPropertyTypeSharedPtr;
typedef shared_ptr<DynamicPropertyType<Matrix3x3>> Matrix3x3DynamicPropertyTypeSharedPtr;
typedef shared_ptr<DynamicPropertyType<ColorBrush>> ColorBrushDynamicPropertyTypeSharedPtr;

//MessageType;
//Message message_<kzFloat>;
typedef shared_ptr<DynamicMessageType<ButtonConcept::ClickedMessageArguments>> ClickedCustomMessageSharedPtr;
typedef shared_ptr<DynamicMessageType<ButtonConcept::PressedMessageArguments>> PressedCustomMessageSharedPtr;
typedef shared_ptr<DynamicMessageType<ButtonConcept::CanceledMessageArguments>> CanceledCustomMessageSharedPtr;
typedef shared_ptr<DynamicMessageType<MessageDispatcherProperties::TimerMessageArguments>> ClockCustomMessageSharedPtr;

//  MACRO DEFINITION FOR PAGE NAVIGATION
#define LIGHTING_PAGE (0)
#define BLINDS_PAGE (1)
#define CLIMATE_PAGE (2)
#define FANS_PAGE (3)
#define SETTINGS_PAGE (4)

//  MACRO DEFINITION FOR COMMUNICATION
#define LIGHTING            (unsigned char)(0x01)
#define LIGHTING_PROFILE    (unsigned char)(0xA1)
#define BLINDS              (unsigned char)(0x02)
#define AIRCON              (unsigned char)(0x03)
#define HEATING             (unsigned char)(0x04)
#define FANS                (unsigned char)(0x05)
#define SETTINGS            (unsigned char)(0x06)
#define SCREEN              (unsigned char)(0xB1)
#define LIGHT_SYNC          (unsigned char)(0xE1)
#define SEND_QUEUE          (unsigned char)(0xF1)

#define SCROLL_DRAGACC       (float)(200.0)
#define SCROLL_DRAGDRAG      (float)(50.0)
#define SCROLL_DRAGIMP       (float)(50.0)
#define SCROLL_SLIDACC       (float)(10.0)
#define SCROLL_SLIDDRAG      (float)(40.0)
#define SCROLL_THRESHOLD     (float)(15.0)

typedef struct
{
    unsigned char from;
    unsigned char to;
    unsigned char sub_system;
    unsigned char group_id;
    unsigned char component_id;
    unsigned char param1;
    unsigned char param2;
    unsigned char value;

}ControlMsg_t;

#endif
