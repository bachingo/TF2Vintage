//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SWORD_H
#define TF_WEAPON_SWORD_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_decapitation.h"

#ifdef CLIENT_DLL
#define CTFSword C_TFSword
#define CTFKatana C_TFKatana
#endif


class CTFSword : public CTFDecapitationMeleeWeaponBase
{
	DECLARE_CLASS( CTFSword, CTFWeaponBaseMelee )
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFSword();
	virtual ~CTFSword();

	virtual bool	Deploy( void );
	virtual int		GetSwingRange( void ) const;
	virtual int		GetSwordHealthMod( void );
	virtual float	GetSwordSpeedMod( void );

	virtual const char* GetEffectLabelText( void ) { return "#TF_Berzerk"; }

	virtual void	OnDecapitation( CTFPlayer *pVictim );

private:

	CTFSword( const CTFSword& );
};


class CTFKatana : public CTFDecapitationMeleeWeaponBase
{
public:
	DECLARE_CLASS( CTFKatana, CTFDecapitationMeleeWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFKatana();
	virtual ~CTFKatana() {}

	virtual bool	Deploy( void );
	virtual int		GetSkinOverride( void );
	virtual void	OnDecapitation( CTFPlayer *pVictim );
	virtual float	GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage );
		
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_KATANA; }

	virtual int		GetActivityWeaponRole( void );
	
	
private:
#if defined( GAME_DLL )
	CNetworkVar( bool, m_bIsBloody );
#else
	bool m_bIsBloody;
#endif

	CTFKatana( const CTFKatana& );
};

#endif // TF_WEAPON_SWORD_H
