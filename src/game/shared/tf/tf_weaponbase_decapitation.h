//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Decapitation Weapon Base Melee 
//
//=============================================================================

#ifndef TF_WEAPONBASE_DECAPITATION_H
#define TF_WEAPONBASE_DECAPITATION_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef GAME_DLL
#include "GameEventListener.h"
#endif


class CTFDecapitationMeleeWeaponBase : public CTFWeaponBaseMelee
#ifdef GAME_DLL
	, public CGameEventListener
#endif
{
	DECLARE_CLASS( CTFDecapitationMeleeWeaponBase, CTFWeaponBaseMelee )
public:

	CTFDecapitationMeleeWeaponBase();
	virtual ~CTFDecapitationMeleeWeaponBase();

	virtual int		TranslateViewmodelHandActivity( int iActivity );

	virtual bool	CanDecapitate( void );
	virtual float	GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage );
	virtual int		GetWeaponID( void ) const						{ return TF_WEAPON_SWORD; }
	virtual int		GetCustomDamageType( void ) const				{ return TF_DMG_CUSTOM_DECAPITATION; }

	virtual void	OnDecapitation( CTFPlayer *pVictim ) { }

#ifdef GAME_DLL
	virtual bool	Holster( CBaseCombatWeapon *pSwitchTo = nullptr );

	virtual void	SetupGameEventListeners( void );
	virtual void	FireGameEvent( IGameEvent *event );
#else
	virtual void	UpdateViewModel( void ) override;
#endif

private:
	CTFDecapitationMeleeWeaponBase( const CTFDecapitationMeleeWeaponBase& ) { }

	EHANDLE m_hEquippedShield;
};

#endif