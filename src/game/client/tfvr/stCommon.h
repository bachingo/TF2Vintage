#ifndef STCOMMON_H
#define STCOMMON_H

#define VR_NUM_BUFFERS 3
#define VR_BACK_BUFFER_X "_rt_two_eyes_" //in certain cases backBufferNamePerIndex(int i) will have to be changed
char* backBufferNamePerIndex(int i);

namespace sdkTypes {
	enum _
	{
		NA = 0,
		openvr = 1,
		oculus = 2,
	};
};

namespace ButtonsList
{
	enum _
	{
		left_Menu,
		left_Trigger,
		left_Bumper,
		left_ButtonA,
		left_ButtonB,
		left_Pad,
		left_PadXAxis,
		left_PadYAxis,
		left_DPad_Up,
		left_DPad_Down,
		left_DPad_Left,
		left_DPad_Right,
		
		right_Menu,
		right_Trigger,
		right_Bumper,
		right_ButtonA,
		right_ButtonB,
		right_Pad,
		right_PadXAxis,
		right_PadYAxis,
		right_DPad_Up,
		right_DPad_Down,
		right_DPad_Left,
		right_DPad_Right,

		left_GestureIndexPoint,
		left_GestureThumbUp,
		left_GestureFist,

		right_GestureIndexPoint,
		right_GestureThumbUp,
		right_GestureFist
	};
};

namespace ControllerTypes
{
	enum _
	{
		controllerType_NA = 0,
		controllerType_XBox = 1,
		controllerType_Virtual = 2,
		//controllerType_Both = 3
	};
};

namespace ControllerID
{
	enum _
	{
		NA,
		openvrVive,
		openvrTouch,
		oculusTouch,
	};
};

namespace MouseButtons
{
	enum _
	{
		left,
		middle,
		right,
	};
};

namespace hmdTextureTypes
{
	enum _
	{
		tLeftEye = 100,
		tRightEye = 110,
		tBothEyes  = 120,
		tPass = 190,

		iIndex0 = 0,
		iIndex1 = 1,
		iIndex2 = 2,
		iPass = 0,
	};
};

const int ButtonsListCount = 30;
typedef sdkTypes::_ sdkType;
typedef ButtonsList::_ ButtonList;
typedef ControllerTypes::_ ControllerType;
typedef ControllerID::_ ControllerList;
typedef MouseButtons::_ MouseButton;
typedef hmdTextureTypes::_ hmdTextureType;

#endif //STCOMMON_H