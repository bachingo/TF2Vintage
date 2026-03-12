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

#define TF_STUNBALL_VIEWMODEL "models/weapons/v_models/v_baseball.mdl"
#define TF_BAUBLE_VIEWMODEL "models/weapons/c_models/c_xms_festive_ornament.mdl"

#ifdef CLIENT_DLL
#define CTFBat_Wood C_TFBat_Wood
#define CTFBat_Giftwrap C_TFBat_Giftwrap
#endif

class CTFBat_Wood : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBat_Wood, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFBat_Wood();

	virtual void		Precache( void );
	virtual int			GetWeaponID( void ) const					{ return TF_WEAPON_BAT_WOOD; }

	virtual bool		Deploy( void );
	virtual void		PrimaryAttack( void );
	virtual void		SecondaryAttack( void );
	virtual void		ItemPostFrame( void );

	virtual bool		HasChargeBar( void )						{ return true; }
	virtual const char* GetEffectLabelText( void )					{ return "#TF_Ball"; }
	virtual float		InternalGetEffectBarRechargeTime( void );

	virtual bool       	SendWeaponAnim( int iActivity );

	virtual bool		CanCreateBall( CTFPlayer *pPlayer );
	virtual bool	    PickedUpBall( CTFPlayer *pPlayer );

	CBaseEntity			*LaunchBall( CTFPlayer *pPlayer );
	virtual void		LaunchBallThink( void );

#ifdef CLIENT_DLL
	virtual void		CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles );

	virtual const char	*GetStunballViewmodel( void )				{ return ( m_bHasBall ? TF_STUNBALL_VIEWMODEL : NULL_STRING ); }

protected:
	bool m_bHasBall;
#endif

protected:
	CTFBat_Wood( const CTFBat_Wood & ) {}

	// prediction
	CNetworkVar( float, m_flNextFireTime );
	CNetworkVar( bool, m_bFiring );
};

class CTFBat_Giftwrap : public CTFBat_Wood
{
public:

	DECLARE_CLASS( CTFBat_Giftwrap, CTFBat_Wood );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual void		Precache( void );
	virtual int			GetWeaponID( void ) const					{ return TF_WEAPON_BAT_GIFTWRAP; }

	CBaseEntity			*LaunchBall( CTFPlayer *pPlayer );
	virtual void		LaunchBallThink( void );

#ifdef CLIENT_DLL

	virtual const char	*GetStunballViewmodel( void )				{ return ( m_bHasBall ? TF_BAUBLE_VIEWMODEL : NULL_STRING ); }

#endif
};
#endif // TF_WEAPON_BAT_WOOD_H