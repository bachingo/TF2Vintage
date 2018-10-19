//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BAT_WOOD_H
#define TF_WEAPON_BAT_WOOD_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBat_Wood C_TFBat_Wood
#endif

class CTFBat_Wood : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBat_Wood, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBat_Wood();

	virtual void		Precache( void );
	virtual int			GetWeaponID( void ) const						{ return TF_WEAPON_BAT_WOOD; }

	virtual bool		Deploy( void );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void		WeaponReset( void );
	virtual void		PrimaryAttack( void );
	virtual void		SecondaryAttack( void );

	virtual bool		HasChargeBar( void )							{ return true; }
	virtual const char* GetEffectLabelText( void )						{ return "#TF_Ball"; }
	virtual float		InternalGetEffectBarRechargeTime( void );

	virtual void		UpdateOnRemove( void );

	virtual bool        SendWeaponAnim( int iActivity );

	virtual bool		CanCreateBall( CTFPlayer *pPlayer );
	virtual bool	    PickedUpBall( CTFPlayer *pPlayer );

	CBaseEntity			*LaunchBall( CTFPlayer *pPlayer );
	virtual void		LaunchBallThink( void );

#ifdef CLIENT_DLL
	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual void		CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles );

	void				UpdateViewmodelBall( C_TFPlayer *pOwner, bool bHolster = false );
#endif

private:
	CTFBat_Wood( const CTFBat_Wood & ) {}

	// prediction
	CNetworkVar( bool, m_bFiring );
	CNetworkVar( float, m_flNextFireTime );
};
#endif // TF_WEAPON_BAT_WOOD_H