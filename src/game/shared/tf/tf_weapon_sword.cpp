//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_sword.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Sword tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSword, DT_TFSword )
BEGIN_NETWORK_TABLE( CTFSword, DT_TFSword )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSword )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sword, CTFSword );
PRECACHE_WEAPON_REGISTER( tf_weapon_sword );

//=============================================================================
//
// Weapon Sword functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFSword::CTFSword()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFSword::~CTFSword()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSword::Deploy( void )
{
	bool orgResult = BaseClass::Deploy();
	if (CanDecapitate() && orgResult)
	{
#ifdef GAME_DLL
		SetupGameEventListeners();
#endif
		return true;
	}
	else
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if (pOwner/* && DWORD( pOwner + 1917 )*/)
		{

		}

		return orgResult;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFSword::GetSwingRange( void ) const
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if (!pOwner)
		return 72;
	return pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) ? 128 : 72;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFSword::GetSwordHealthMod( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if (!pOwner)
		return 0;

	if (CanDecapitate())
		return Min( pOwner->m_Shared.GetDecapitationCount(), 4 ) * 15;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFSword::GetSwordSpeedMod( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if (!pOwner)
		return 1.0f;

	if (CanDecapitate())
		return ( Min( pOwner->m_Shared.GetDecapitationCount(), 4 ) * 0.08 ) + 1.0f;

	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSword::OnDecapitation( CTFPlayer *pVictim )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if (!pOwner)
		return;

	int iHeadCount = pOwner->m_Shared.GetDecapitationCount() + 1;
	if (pVictim)
		iHeadCount += pVictim->m_Shared.GetDecapitationCount();

	pOwner->m_Shared.SetDecapitationCount( iHeadCount );

	pOwner->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	if (pOwner->m_Shared.GetMaxBuffedHealth() > pOwner->GetHealth())
		pOwner->TakeHealth( 15.0f, DMG_IGNORE_MAXHEALTH );

	if (!pOwner->m_Shared.InCond( TF_COND_DEMO_BUFF ))
		pOwner->m_Shared.AddCond( TF_COND_DEMO_BUFF );
#endif
}
