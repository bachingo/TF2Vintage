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

	CTFSword( const CTFSword& ) {}
};

#endif // TF_WEAPON_SWORD_H
