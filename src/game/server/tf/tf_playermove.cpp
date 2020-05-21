//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player_command.h"
#include "igamemovement.h"
#include "in_buttons.h"
#include "ipredictionsystem.h"
#include "tf_player.h"
#include "iservervehicle.h"


static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

ConVar tf_demoman_charge_frametime_scaling( "tf_demoman_charge_frametime_scaling", "1", FCVAR_CHEAT, "When enabled, scale yaw limiting based on client performance (frametime)" );

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;


//-----------------------------------------------------------------------------
// Sets up the move data for TF2
//-----------------------------------------------------------------------------
class CTFPlayerMove : public CPlayerMove
{
DECLARE_CLASS( CTFPlayerMove, CPlayerMove );

public:
	virtual void	StartCommand( CBasePlayer *player, CUserCmd *cmd );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
};

// PlayerMove Interface
static CTFPlayerMove g_PlayerMove;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}

//-----------------------------------------------------------------------------
// Main setup, finish
//-----------------------------------------------------------------------------

void CTFPlayerMove::StartCommand( CBasePlayer *player, CUserCmd *cmd )
{
	BaseClass::StartCommand( player, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: This is called pre player movement and copies all the data necessary
//          from the player for movement. (Server-side, the client-side version
//          of this code can be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( player );
	if ( pTFPlayer )
	{
		// Check to see if we are a crouched, heavy, firing his weapons and zero out movement.
		if ( pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			if ( ( pTFPlayer->GetFlags() & FL_DUCKING ) && ( pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) ) )
			{
				ucmd->forwardmove = 0.0f;
				ucmd->sidemove = 0.0f;
			}
		}

		if ( pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING ) || pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
		{
			ucmd->forwardmove = 0;
			ucmd->upmove = 0;
			ucmd->sidemove = 0;
			ucmd->viewangles = pTFPlayer->pl.v_angle;
		}

		if ( pTFPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			float flTurnRate = 0.45f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFPlayer, flTurnRate, charge_turn_control );

			if ( tf_demoman_charge_frametime_scaling.GetBool() )
			{
				flTurnRate *= RemapValClamped( gpGlobals->frametime, TICKS_TO_TIME( 0.2 ), TICKS_TO_TIME( 2.0 ), 0.25, 2.0 );
			}

			flTurnRate *= 2.5f;
			if ( abs( pTFPlayer->m_angPrevEyeAngles.y ) - abs( ucmd->viewangles.y ) > flTurnRate )
			{
				if ( ucmd->viewangles.y < pTFPlayer->m_angPrevEyeAngles.y )
					ucmd->viewangles.y = pTFPlayer->m_angPrevEyeAngles.y - flTurnRate;
				else
					ucmd->viewangles.y = flTurnRate + pTFPlayer->m_angPrevEyeAngles.y;

				pTFPlayer->SnapEyeAngles( ucmd->viewangles );
				pTFPlayer->m_angPrevEyeAngles = ucmd->viewangles;
			}
		}
		else
		{
			pTFPlayer->m_angPrevEyeAngles = pTFPlayer->EyeAngles();
		}
	}

	BaseClass::SetupMove( player, ucmd, pHelper, move );

	IServerVehicle *pVehicle = player->GetVehicle();
	if ( pVehicle && gpGlobals->frametime != 0 )
	{
		pVehicle->SetupMove( player, ucmd, pHelper, move );
	}
}


//-----------------------------------------------------------------------------
// Purpose: This is called post player movement to copy back all data that
//          movement could have modified and that is necessary for future
//          movement. (Server-side, the client-side version of this code can 
//          be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );

	IServerVehicle *pVehicle = player->GetVehicle();
	if ( pVehicle && gpGlobals->frametime != 0 )
	{
		pVehicle->FinishMove( player, ucmd, move );
	}
}
