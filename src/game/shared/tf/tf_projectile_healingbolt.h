//=============================================================================//
//
// Purpose: Arrow used by Huntsman.
//
//=============================================================================//

#ifndef TF_PROJECTILE_HEALINGBOLT_H
#define TF_PROJECTILE_HEALINGBOLT_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_projectile_arrow.h"

#ifdef CLIENT_DLL
#define CTFProjectile_HealingBolt C_TFProjectile_HealingBolt
#endif

class CTFProjectile_HealingBolt : public CTFProjectile_Arrow
{
	DECLARE_CLASS( CTFProjectile_HealingBolt, CTFProjectile_Arrow );
public:
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFProjectile_HealingBolt();
	virtual ~CTFProjectile_HealingBolt();



#ifdef GAME_DLL
	virtual bool		CanHeadshot( void )                        { return false; }
	virtual void		ImpactTeamPlayer( CTFPlayer *pTarget );

	virtual float		GetCollideWithTeammatesDelay( void ) const { return 0.0; }

	virtual float		GetDamage( void ) override;
#else
	virtual void		OnDataChanged( DataUpdateType_t updateType );
#endif

private:
};

#endif // TF_PROJECTILE_ARROW_H
