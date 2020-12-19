//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Melee 
//
//=============================================================================

#ifndef TF_WEAPONBASE_MELEE_H
#define TF_WEAPONBASE_MELEE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTFWeaponBaseMelee C_TFWeaponBaseMelee
#endif

typedef CUtlVector< CGameTrace > MeleePartitionVector;
//=============================================================================
//
// Weapon Base Melee Class
//
class CTFWeaponBaseMelee : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseMelee, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponBaseMelee();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool	HasPrimaryAmmo()								{ return true; }
	virtual bool	CanBeSelected()									{ return true; }
	virtual void	Precache();
	virtual void	ItemPostFrame();
	virtual void	Spawn();
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual int		GetWeaponID( void ) const						{ return TF_WEAPON_NONE; }
	virtual bool	ShouldDrawCrosshair( void )						{ return true; }
	virtual void	WeaponReset( void );

	virtual bool	CalcIsAttackCriticalHelper( void );

	virtual void	DoViewModelAnimation( void );

	virtual bool	DoSwingTrace( trace_t &tr );
	virtual void	Smack( void );
	virtual void	DoMeleeDamage( CBaseEntity *pTarget, trace_t &trace );

	virtual float	GetMeleeDamage( CBaseEntity *pTarget, int &iDamageType, int &iCustomDamage );
	virtual int		GetCustomDamageType() const						{ return TF_DMG_CUSTOM_NONE; }
	char const*		GetShootSound( int iIndex ) const OVERRIDE;

#ifndef CLIENT_DLL
	virtual float	GetForceScale( void );
#endif

	// Call when we hit an entity. Use for special weapon effects on hit.
	virtual void	OnEntityHit( CBaseEntity *pEntity );
	virtual void	OnSwingHit( trace_t &trace );

	virtual void	SendPlayerAnimEvent( CTFPlayer *pPlayer );

	bool			IsCurrentAttackACritical( void )				{ return m_bCurrentAttackIsCrit; }
	bool			ConnectedHit( void )							{ return m_bConnected; }

	virtual int		GetSwingRange( void ) const;

public:	

	CTFWeaponInfo	*m_pWeaponInfo;

protected:

	virtual void	Swing( CTFPlayer *pPlayer );
	bool			DoSwingTraceInternal( trace_t &tr, bool bCleave, MeleePartitionVector *enumResults );

protected:

	float	m_flSmackTime;
	bool	m_bConnected;

private:

	CTFWeaponBaseMelee( const CTFWeaponBaseMelee & ) {}
};

#endif // TF_WEAPONBASE_MELEE_H
