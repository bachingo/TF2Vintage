#ifndef TF_WEARABLE_DEMOSHIELD_H
#define TF_WEARABLE_DEMOSHIELD_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_wearable.h"

#if defined CLIENT_DLL
#define CTFWearableDemoShield C_TFWearableDemoShield
#endif

class CTFWearableDemoShield : public CTFWearable
{
	DECLARE_CLASS( CTFWearableDemoShield, CTFWearable )
public:
	DECLARE_NETWORKCLASS()

	CTFWearableDemoShield();
	virtual ~CTFWearableDemoShield();

	virtual void	Precache( void );

#ifdef GAME_DLL
	virtual void	Equip( CBasePlayer *pPlayer );
	virtual void	UnEquip( CBasePlayer *pPlayer );

	virtual float	GetDamage( void );
	Vector			GetDamageForce( void );
	void			ShieldBash( CTFPlayer *pAttacker );
	bool			DoSpecialAction( CTFPlayer *pUser );
	void			EndSpecialAction( CTFPlayer *pUser );
#endif

private:
	bool	m_bBashed;

	CTFWearableDemoShield( const CTFWearableDemoShield& ) {}
};


extern CTFWearableDemoShield *GetEquippedDemoShield( CTFPlayer *pPlayer );

#endif
