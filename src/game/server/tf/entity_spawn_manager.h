//=============================================================================//
//
// Purpose: CTF Spawn Manager.
//
//=============================================================================//
#ifndef ENTITY_SPAWN_MANAGER_H
#define ENTITY_SPAWN_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"

class CEntitySpawnPoint;

//=============================================================================
//
// CTF Spawn Manager class.
//

class CEntitySpawnManager : public CServerOnlyPointEntity
{
public:
	DECLARE_CLASS( CEntitySpawnManager, CServerOnlyPointEntity );

	void	Spawn( void );
	void	RegisterSpawnPoint( CEntitySpawnPoint *pSpawnPoint );
	void	Activate( void );

	void	SpawnAllEntities( void );
	bool	SpawnEntityAt( int iSpawnPoint );

	int		GetRandomUnusedIndex( void );

	int		GetRespawnTime( void ) { return m_iRespawnTime; }

private:
	DECLARE_DATADESC();

	string_t		m_iszEntityName;
	int				m_iEntityCount;
	int				m_iRespawnTime;
	bool			m_bDropToGround;
	bool			m_bRandomRotation;

	CUtlVector<CEntitySpawnPoint *> m_hSpawnPoints;
};

class CEntitySpawnPoint : public CLogicalEntity, public IEntityListener
{
public:
	DECLARE_CLASS(  CEntitySpawnPoint, CLogicalEntity );

	void	Spawn( void );
	void	RespawnNotifyThink( void );
	void	UpdateOnRemove( void );

	// IEntityListener
	void	OnEntityDeleted( CBaseEntity *pOther );

	// m_hEntity
	void			SetSpawnEntity( CBaseEntity *pOther ) { m_hEntity = pOther; } 
	CBaseEntity*	GetSpawnEntity( void ) { return m_hEntity.Get(); }

private:
	DECLARE_DATADESC();

	string_t						m_iszSpawnManagerName;
	float							m_flUnknown;

	CHandle<CBaseEntity>			m_hEntity;
	CHandle<CEntitySpawnManager>	m_hManager;

};

#endif // ENTITY_SPAWN_MANAGER_H


