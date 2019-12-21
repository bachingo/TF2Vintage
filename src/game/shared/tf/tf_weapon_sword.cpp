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
CREATE_SIMPLE_WEAPON_TABLE( TFSword, tf_weapon_sword )

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
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKatana::Deploy( void )
{
	bool orgResult = BaseClass::Deploy();
	if ( CanDecapitate() && orgResult )
	{
#ifdef GAME_DLL
		SetupGameEventListeners();
#endif
		m_bHonorbound = false;	// Reset our honor on deploying.
#ifdef CLIENT_DLL
		GetSkinOverride();
#endif //CLIENT_DLL
		return true;
	}
	else
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if (pOwner/* && DWORD( pOwner + 1917 )*/)
		{

		}
		if (orgResult)
		{
			m_bHonorbound = false;	// Reset our honor on deploying.
#ifdef CLIENT_DLL
			GetSkinOverride();
#endif //CLIENT_DLL
		}
		return orgResult;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFKatana::GetSwingRange( void ) const
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return 72;

	int iBaseRange = BaseClass::GetSwingRange();

	return pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) ? iBaseRange + 96 : iBaseRange + 24;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKatana::FireGameEvent( IGameEvent *event )
{
	if (FStrEq( event->GetName(), "player_death" ))
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if (pOwner && engine->GetPlayerUserId( pOwner->edict() ) == event->GetInt( "attacker" ) && 
			 (event->GetInt( "weaponid" ) == TF_WEAPON_KATANA || event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING) )
		{
			UpdateHonor(event);
		}
	}
}
#endif
//-----------------------------------------------------------------------------
// Purpose: Do extra damage to other katana wielders.
//-----------------------------------------------------------------------------
float CTFKatana::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{
	float flBaseDamage = CTFDecapitationMeleeWeaponBase::GetMeleeDamage( pTarget, iCustomDamage );

	CTFPlayer *pOwner = GetTFPlayerOwner();

	// Check to see if the current target also has a katana equipped.
	if ( pOwner && pTarget->IsPlayer() && pTarget )	
	{
		// Check to see if they're also holding a Katana.
		if (ToTFPlayer(pTarget)->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_KATANA )
		{
			// Do twice the target's health so that it's a guaranteed kill.
			flBaseDamage = pTarget->GetHealth() * 2;
		}
	}

	return flBaseDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKatana::UpdateHonor( IGameEvent *event )
{
	m_bHonorbound = true;
#ifdef CLIENT_DLL
	GetSkinOverride();
#endif //CLIENT_DLL
}

IMPLEMENT_NETWORKCLASS_ALIASED( TFKatana, DT_TFKatana )
BEGIN_NETWORK_TABLE( CTFKatana, DT_TFKatana )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nSkin ) ),
#else
	SendPropInt( SENDINFO( m_nSkin ), 2, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Give us a visual indicator of fulfilling honorbound.
//-----------------------------------------------------------------------------
int CTFKatana::GetSkinOverride()
{

	int iCurrentSkin = CTFWeaponBase::GetSkin();
	if (m_bHonorbound)
	{
		iCurrentSkin = CTFWeaponBase::GetSkin();
		iCurrentSkin += 2;
		m_nSkin = iCurrentSkin;
		return iCurrentSkin;
	}
	
	return BaseClass::GetSkinOverride();
	
	
}
#endif //CLIENT_DLL

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFKatana::CanHolster( void ) const
{
	float flHonorPenalty = 50;
	bool bCanHolster = CTFWeaponBaseMelee::CanHolster();
	// Honorbound affects holster.
	if (!m_bHonorbound)	// We have no honor.
	{
		// Under 50HP, can't holster at all. Very dishonorable.
		if (GetTFPlayerOwner()->GetHealth() <= flHonorPenalty)
			bCanHolster = false;
		else
		{
			// Check to see if we could normally holster.
			if (bCanHolster)
			{
				// We can holster, but do 50HP of damage for being dishonorable.
#ifdef GAME_DLL
				GetTFPlayerOwner()->TakeHealth((flHonorPenalty * -1), DMG_GENERIC);
#endif
			}
		}
	}
	
	return bCanHolster;
	
}