//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

#include "tf_weapon_sword.h"
#include "decals.h"


//=============================================================================
//
// Weapon Sword tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFSword, tf_weapon_sword )


IMPLEMENT_NETWORKCLASS_ALIASED( TFKatana, DT_TFKatana )
BEGIN_NETWORK_TABLE( CTFKatana, DT_TFKatana )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bIsBloody ) ),
#else
SendPropBool( SENDINFO( m_bIsBloody ) ),
#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFKatana )
DEFINE_PRED_FIELD( m_bIsBloody, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_katana, CTFKatana );
PRECACHE_REGISTER( tf_weapon_katana );


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
	if ( CanDecapitate() && orgResult )
	{
#ifdef GAME_DLL
		SetupGameEventListeners();
#endif
	}

	return orgResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFSword::GetSwingRange( void ) const
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return 72;

	int iBaseRange = BaseClass::GetSwingRange();

	return pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) ? iBaseRange + 96 : iBaseRange + 24;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFSword::GetSwordHealthMod( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return 0;

	if ( CanDecapitate() )
		return Min( pOwner->m_Shared.GetDecapitationCount(), 4 ) * 15;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFSword::GetSwordSpeedMod( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return 1.0f;

	if ( CanDecapitate() )
		return ( Min( pOwner->m_Shared.GetDecapitationCount(), 4 ) * 0.08 ) + 1.0f;

	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSword::OnDecapitation( CTFPlayer *pVictim )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return;

	int iHeadCount = pOwner->m_Shared.GetDecapitationCount() + 1;
	if ( pVictim )
		iHeadCount += pVictim->m_Shared.GetDecapitationCount();

	pOwner->m_Shared.SetDecapitationCount( iHeadCount );

	pOwner->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	if ( pOwner->m_Shared.GetMaxBuffedHealth() > pOwner->GetHealth() )
		pOwner->TakeHealth( 15.0f, DMG_IGNORE_MAXHEALTH );

	if ( !pOwner->m_Shared.InCond( TF_COND_DEMO_BUFF ) )
		pOwner->m_Shared.AddCond( TF_COND_DEMO_BUFF );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFKatana::CTFKatana()
{
#if defined( CLIENT_DLL )
	m_bIsBloody = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKatana::Deploy( void )
{
	bool orgResult = BaseClass::Deploy();
	if ( m_bIsBloody )
		m_bIsBloody = false;
	
	if ( CanDecapitate() )
	{
	#if defined( GAME_DLL )
		if( orgResult )
			SetupGameEventListeners();
	#endif
	}

	return orgResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKatana::CanHolster( void )
{
	return m_bIsBloody;
}

//-----------------------------------------------------------------------------
// Purpose: Do extra damage to other katana wielders.
//-----------------------------------------------------------------------------
float CTFKatana::GetMeleeDamage( CBaseEntity *pTarget, int &iDamageType, int &iCustomDamage )
{
	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, iDamageType, iCustomDamage );

	// Check to see if me and the current target have a katana equipped.
	if ( !IsHonorBound() )
		return flBaseDamage;

	CTFPlayer *pTFVictim = ToTFPlayer( pTarget );
	if ( !pTFVictim || !pTFVictim->GetActiveWeapon() )
		return flBaseDamage;

	if( pTFVictim->GetActiveTFWeapon()->IsHonorBound() )
	{
		// Do thrice the target's health so that it's a guaranteed kill.
		flBaseDamage = pTarget->GetHealth() * 3;
	}

	return flBaseDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Give us a visual indicator of fulfilling honorbound.
//-----------------------------------------------------------------------------
int CTFKatana::GetSkinOverride()
{
	if ( !m_bIsBloody )
		return -1;

	if ( UTIL_IsLowViolence() )
		return -1;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner == nullptr )
		return -1;

	switch ( pOwner->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			return 2;
		case TF_TEAM_BLUE:
			return 3;
	}

	return 3;
}

void CTFKatana::OnDecapitation( CTFPlayer *pVictim )
{
#if defined( GAME_DLL )
	m_bIsBloody = true;
#endif
}

int CTFKatana::GetActivityWeaponRole( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		return TF_WPN_TYPE_ITEM1;

	return TF_WPN_TYPE_MELEE;
}
