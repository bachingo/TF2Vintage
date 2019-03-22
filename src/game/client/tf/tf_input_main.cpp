//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: TF2 specific input handling
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "kbutton.h"
#include "input.h"
#include "cam_thirdperson.h"
#include "c_tf_player.h"


extern ConVar cl_yawspeed;
extern ConVar thirdperson_platformer;
extern ConVar cam_idealyaw;

//-----------------------------------------------------------------------------
// Purpose: TF Input interface
//-----------------------------------------------------------------------------
class CTFInput : public CInput
{
public:
	virtual	float		CAM_CapYaw( float fVal ) const;
	virtual void		AdjustYaw( float speed, QAngle& viewangles );
};

static CTFInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;


float CTFInput::CAM_CapYaw( float fVal ) const
{
	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if (pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ))
	{
		float flCap = 0.45f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flCap, charge_turn_control );
		
		return Clamp( fVal, -flCap, flCap );
	}

	return fVal;
}

void CTFInput::AdjustYaw( float speed, QAngle& viewangles )
{
	extern kbutton_t in_left;
	extern kbutton_t in_right;

	if (!( in_strafe.state & 1 ))
	{
		float flLeft = speed*cl_yawspeed.GetFloat() * KeyState( &in_right );
		float flRight = speed*cl_yawspeed.GetFloat() * KeyState( &in_left );

		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if (pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ))
		{
			float flCap = 0.45f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flCap, charge_turn_control );
			if (flRight > flCap || -flRight < -flCap)
			{
				flRight = -flCap;
			}
			if (flLeft > flCap || -flLeft < -flCap)
			{
				flLeft = flCap;
			}
		}

		viewangles[YAW] -= flLeft;
		viewangles[YAW] += flRight;
	}

	// thirdperson platformer mode
	// use movement keys to aim the player relative to the thirdperson camera
	if (CAM_IsThirdPerson() && thirdperson_platformer.GetInt())
	{
		float side = KeyState( &in_moveleft ) - KeyState( &in_moveright );
		float forward = KeyState( &in_forward ) - KeyState( &in_back );

		if (side || forward)
		{
			viewangles[YAW] = RAD2DEG( atan2( side, forward ) ) + g_ThirdPersonManager.GetCameraOffsetAngles()[YAW];
		}
		if (side || forward || KeyState( &in_right ) || KeyState( &in_left ))
		{
			cam_idealyaw.SetValue( g_ThirdPersonManager.GetCameraOffsetAngles()[YAW] - viewangles[YAW] );
		}
	}
}
