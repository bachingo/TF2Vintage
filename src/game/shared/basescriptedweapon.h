#ifndef BASESCRIPTEDWEAPON_H
#define BASESCRIPTEDWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon_shared.h"
#include "vscript/ivscript.h"


#if defined( CLIENT_DLL )
#define CBaseScriptedWeapon C_BaseScriptedWeapon
#endif

class CBaseScriptedWeapon : public CBaseCombatWeapon
{
	DECLARE_CLASS( CBaseScriptedWeapon, CBaseCombatWeapon );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ENT_SCRIPTDESC();

	CBaseScriptedWeapon();
	~CBaseScriptedWeapon();

	virtual void		Precache();
	virtual void		Spawn();

#if defined( GAME_DLL )
	void				HandleAnimEvent( animevent_t *pEvent ) OVERRIDE;
#endif

	virtual void		PrimaryAttack();
	virtual void		SecondaryAttack();

	virtual bool		CanHolster( void );
	virtual bool		CanDeploy( void );
	virtual bool		Deploy( void );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void		OnActiveStateChanged( int iOldState );
	virtual void		Detach();

	virtual void		Equip( CBaseCombatCharacter *pOwner );
};

#endif
