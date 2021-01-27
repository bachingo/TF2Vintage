//=============================================================================//
//
// Purpose: CTF Spawn Manager.
//
//=============================================================================//
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "entity_spawn_manager.h"

//=============================================================================
//
// Entity Spawn Manager.
//

BEGIN_DATADESC( CEntitySpawnManager )

// Keyfields.
DEFINE_KEYFIELD( m_iszEntityName, FIELD_STRING, "entity_name" ),
DEFINE_KEYFIELD( m_iEntityCount, FIELD_INTEGER, "entity_count" ),
DEFINE_KEYFIELD( m_iRespawnTime, FIELD_INTEGER, "respawn_time"),
DEFINE_KEYFIELD( m_bDropToGround, FIELD_BOOLEAN, "drop_to_ground"),
DEFINE_KEYFIELD( m_bRandomRotation, FIELD_BOOLEAN, "random_rotation"),

// Functions.

// Inputs.

// Outputs.

END_DATADESC()

LINK_ENTITY_TO_CLASS( entity_spawn_manager, CEntitySpawnManager );


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::Spawn( void )
{
	BaseClass::Spawn();
	SetNextThink( -1.0f );
	ThinkSet( NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::RegisterSpawnPoint( CEntitySpawnPoint *pSpawnPoint )
{
	m_hSpawnPoints.AddToTail( pSpawnPoint );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::Activate( void )
{
	if ( m_hSpawnPoints.Count() && m_iEntityCount )
		SpawnAllEntities();

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnManager::SpawnAllEntities( void )
{
	int i, iSpawnedEntities = 0;

	for ( i = 0; i < m_hSpawnPoints.Count(); i++ )
	{
		if ( m_hSpawnPoints[i] && m_hSpawnPoints[i]->GetSpawnEntity() != NULL )
		{
			iSpawnedEntities = i;
			break;
		}
	}

	iSpawnedEntities = m_iEntityCount - iSpawnedEntities;

	// Spawn any remaining entities
	if ( iSpawnedEntities > 0 )
	{
		for ( i = 0; i < iSpawnedEntities; i++ )
		{
			if ( !SpawnEntityAt( GetRandomUnusedIndex() ) )
			{
				// No more places to spawn entities at
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEntitySpawnManager::SpawnEntityAt( int iSpawnPoint )
{
	if( iSpawnPoint == -1 )
		return false;

	CEntitySpawnPoint *pPoint = m_hSpawnPoints[iSpawnPoint];
	if ( !pPoint )
		return false;

	CBaseEntity *pEntity = CreateEntityByName( STRING( m_iszEntityName ) );
	if ( !pEntity )
		return false;

	Vector vMin, vMax, vAbsOrigin;

	vAbsOrigin = pPoint->GetAbsOrigin();
	pEntity->CollisionProp()->WorldSpaceAABB( &vMin, &vMax );

	if ( UTIL_IsSpaceEmpty( pEntity, vMin + vAbsOrigin, vMax + vAbsOrigin ) )
	{
		pEntity->SetAbsOrigin( vAbsOrigin );

		if ( m_bDropToGround )
		{
			if ( UTIL_DropToFloor( pEntity, MASK_SOLID ) == 0 )
			{
				Error("TF Entity %s fell out of level at %f,%f,%f", STRING( pEntity->GetEntityName() ), pEntity->GetAbsOrigin().x, pEntity->GetAbsOrigin().y, pEntity->GetAbsOrigin().z);
				UTIL_Remove( pEntity );
				return false;
			}
		}

		// If spawn point is clear, spawn the entity
		DispatchSpawn( pEntity );
		
		if ( m_bRandomRotation )
		{
			// Random rotation
			pEntity->SetAbsAngles( QAngle( 0.0f, RandomFloat( 0.0f, 360.0f ), 0.0f ) );
		}
		else
		{
			// Use the angles provided by the spawn point
			pEntity->SetAbsAngles( pPoint->GetAbsAngles() );
		}
		pPoint->SetSpawnEntity( pEntity );

		return true;
	}

	// Something is in the way. Don't spawn
	UTIL_Remove( pEntity );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEntitySpawnManager::GetRandomUnusedIndex( void )
{
	int index = RandomInt( 0, m_hSpawnPoints.Count() - 1 );

	// Is this spawn point already in use?
	if ( m_hSpawnPoints[index] && m_hSpawnPoints[index]->GetSpawnEntity() )
	{
		// Iterate through the spawnpoints until we find one that's not in use
		index = -1;
		for ( int i = 0; i < m_hSpawnPoints.Count(); i++ )
		{
			if ( m_hSpawnPoints[i] && m_hSpawnPoints[i]->GetSpawnEntity() == NULL )
			{
				index = i;
				break;
			}
		}
	}

	return index;
}


//=============================================================================
//
// Entity Spawn Point.
//

BEGIN_DATADESC( CEntitySpawnPoint )

// Keyfields.
DEFINE_KEYFIELD( m_iszSpawnManagerName, FIELD_STRING, "spawn_manager_name" ),

// Functions.

// Inputs.

// Outputs.

END_DATADESC()

LINK_ENTITY_TO_CLASS( entity_spawn_point, CEntitySpawnPoint );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnPoint::Spawn( void )
{
	BaseClass::Spawn();
	SetNextThink( -1.0f );
	ThinkSet( NULL );

	if ( !m_hManager )
	{
		m_hManager = dynamic_cast<CEntitySpawnManager *>( gEntList.FindEntityByName( NULL, STRING( m_iszSpawnManagerName ) ) );
		if ( m_hManager )
		{
			m_hManager->RegisterSpawnPoint( this );
			gEntList.AddListenerEntity( this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnPoint::RespawnNotifyThink( void )
{
	if ( gpGlobals->curtime > m_flUnknown && m_hManager )
	{
		if ( m_hManager->SpawnEntityAt( m_hManager->GetRandomUnusedIndex() ) )
		{
			SetThink( NULL );
			SetNextThink( TICK_NEVER_THINK );
			return;
		}
	}

	SetThink( &CEntitySpawnPoint::RespawnNotifyThink );
	SetNextThink( gpGlobals->curtime + 5.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnPoint::UpdateOnRemove( void )
{
	gEntList.RemoveListenerEntity( this );
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEntitySpawnPoint::OnEntityDeleted( CBaseEntity *pOther )
{
	if ( m_hManager && pOther == m_hEntity )
	{
		// The entity deleted is the one our spawn point is tracking
		m_flUnknown = gpGlobals->curtime + 10.0f;
		m_hEntity = NULL;

		SetThink( &CEntitySpawnPoint::RespawnNotifyThink );
		SetNextThink( gpGlobals->curtime + m_hManager->GetRespawnTime() );
	}
}