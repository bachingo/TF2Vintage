//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_POWERUPBOTTLE_H
#define TF_POWERUPBOTTLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_wearable.h"

#if defined( CLIENT_DLL )
#define CTFPowerupBottle C_TFPowerupBottle
#endif

class CTFPowerupBottle : public CTFWearable
#if defined( CLIENT_DLL )
	, public CGameEventListener
#endif
{
	DECLARE_CLASS( CTFPowerupBottle, CTFWearable );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFPowerupBottle();
	virtual ~CTFPowerupBottle() { }

	virtual int		GetSkin( void ) OVERRIDE;
#if defined( CLIENT_DLL )
	virtual int		GetWorldModelIndex( void ) OVERRIDE;
	virtual void	FireGameEvent( IGameEvent *event ) OVERRIDE;
#endif
	virtual void	Precache( void ) OVERRIDE;
	virtual void	ReapplyProvision( void ) OVERRIDE;
#if defined( GAME_DLL )
	virtual void	UnEquip( CBasePlayer *pOwner ) OVERRIDE;
#endif

	char const*		GetEffectLabelText( void );
	char const*		GetEffectIconName( void );
	float			GetEffectBarProgress( void ) { return 0; }

	int				GetNumCharges() const;
	void			SetNumCharges( int nNumCharges );
	int				GetMaxNumCharges() const;

	bool			AllowedToUse( void ) const;
	PowerupBottleType_t GetPowerupType( void ) const;
	void			RemoveEffect( void );
	void			Reset( void );
	void			StatusThink();
	bool			Use( void );

private:
	CNetworkVar( bool, m_bActive );
	CNetworkVar( uint8, m_usNumCharges );
	mutable float m_flLastSpawnTime;
};

#endif
