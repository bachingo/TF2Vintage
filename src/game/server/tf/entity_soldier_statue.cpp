#include "cbase.h"
#include "tf_player.h"
#include "entity_soldier_statue.h"


LINK_ENTITY_TO_CLASS( entity_soldier_statue, CEntitySoldierStatue );

void CEntitySoldierStatue::Precache()
{
	bool allowPrecache = IsPrecacheAllowed();
	SetAllowPrecache( true );

	PrecacheModel( "models/soldier_statue/soldier_statue.mdl" );
	PrecacheScriptSound( "Soldier.Statue" );

	SetAllowPrecache( allowPrecache );
}

void CEntitySoldierStatue::Spawn()
{
	Precache();
	BaseClass::Spawn();

	SetModel( "models/soldier_statue/soldier_statue.mdl" );
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_BBOX );

	SetContextThink( &CEntitySoldierStatue::StatueThink, gpGlobals->curtime + 1.f, NULL );
}

void CEntitySoldierStatue::StatueThink( void )
{
	if( m_voiceLineTimer.IsElapsed() )
	{
		CUtlVector<CTFPlayer *> players;
		CollectPlayers( &players, TF_TEAM_RED, true );
		CollectPlayers( &players, TF_TEAM_BLUE, true, true );
		FOR_EACH_VEC( players, i )
		{
			CTFPlayer *pPlayer = players[i];
			if ( !pPlayer->GetAbsVelocity().IsZero() )
				continue;

			if ( !( GetAbsOrigin() - pPlayer->GetAbsOrigin() ).IsLengthLessThan( 500.0 ) )
				continue;

			if ( !pPlayer->IsLineOfSightClear( this ) )
				continue;

			CSoundParameters parms;
			if ( GetParametersForSound( "Soldier.Statue", parms, NULL ) )
			{
				CPASAttenuationFilter filter( GetAbsOrigin() );
				EmitSound_t emitSound( parms );
				EmitSound( filter, entindex(), emitSound );

				m_voiceLineTimer.Start( RandomFloat( 12.0, 17.0 ) );

				break;
			}
		}
	}

	SetContextThink( &CEntitySoldierStatue::StatueThink, gpGlobals->curtime, NULL );
}
