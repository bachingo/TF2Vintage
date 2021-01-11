//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//========================================================================//

#ifndef ECON_WEARABLE_H
#define ECON_WEARABLE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "particles_new.h"
#endif

#define MAX_WEARABLES_SENT_FROM_SERVER	7
#define PARTICLE_MODIFY_STRING_SIZE		128

#if defined( CLIENT_DLL )
#define CEconWearable C_EconWearable
#define CEconWearableGib C_EconWearableGib
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconWearable : public CEconEntity
{
	DECLARE_CLASS( CEconWearable, CEconEntity );
	DECLARE_NETWORKCLASS();

public:

	virtual void			Spawn( void );
	virtual bool			IsWearable( void ) { return true; }
	virtual int				GetSkin(void);
	virtual void			SetParticle(const char* name);
	virtual void			UpdateWearableBodyGroups( CBasePlayer *pPlayer );
	virtual void			GiveTo( CBaseEntity *pEntity );
	virtual void			RemoveFrom( CBaseEntity *pEntity );

	virtual bool			IsViewModelWearable( void ) const { return false; }
	
	virtual	int 			GetDropType( void );
	virtual	int 			GetLoadoutSlot(void);

#ifdef GAME_DLL
	virtual void			Equip( CBasePlayer *pPlayer );
	virtual void			UnEquip( CBasePlayer *pPlayer );
#else
	virtual void			OnDataChanged(DataUpdateType_t type);
	virtual	ShadowType_t	ShadowCastType( void );
	virtual bool			ShouldDraw( void );
#endif
	

private:
#ifdef GAME_DLL
	CNetworkString(m_ParticleName, PARTICLE_MODIFY_STRING_SIZE);
#else
	char m_ParticleName[PARTICLE_MODIFY_STRING_SIZE];
	CNewParticleEffect *m_pUnusualParticle;
#endif

};

#if defined( CLIENT_DLL )
class CEconWearableGib : public CEconEntity
{
	DECLARE_CLASS( CEconWearableGib, CEconEntity );
public:
	CEconWearableGib();
	virtual ~CEconWearableGib();

	virtual CollideType_t GetCollideType( void );
	virtual void ImpactTrace( trace_t *pTrace, int dmgCustom, char const *szWeaponName );

	virtual void Spawn( void );
	virtual void SpawnClientEntity( void );
	virtual CStudioHdr *OnNewModel( void );

	virtual void ClientThink( void );
	
	void StartFadeOut( float flTime );
	void FinishModelInitialization( void );
	bool Initialize( bool bAttached );

private:
	bool m_bAttachedModel;
	bool m_bDynamicLoad;
	float m_flFadeTime;
};
#endif

#endif
