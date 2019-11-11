//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "particle_parse.h"
#include "tf_obj.h"
#include "nav_mesh/tf_nav_area.h"
#include "tf_gamerules.h"
#include "tf_bot.h"
#include "tf_bot_components.h"


float CTFBotBody::GetHeadAimTrackingInterval( void ) const
{
	CTFBot *bot = static_cast<CTFBot *>( this->GetBot() );

	switch( bot->m_iSkill )
	{
		case CTFBot::NORMAL:
			return 0.30f;
		case CTFBot::HARD:
			return 0.10f;
		case CTFBot::EXPERT:
			return 0.05f;

		default:
			return 1.00f;
	}
}


void CTFBotLocomotion::Update( void )
{
	BaseClass::Update();

	CTFBot *actor = ToTFBot( this->GetEntity() );
	if ( actor && !IsOnGround() )
		actor->PressCrouchButton( 0.3f );
}

void CTFBotLocomotion::Approach( const Vector &pos, float goalWeight )
{
	if ( !IsOnGround() && !IsClimbingOrJumping() )
		return;

	BaseClass::Approach( pos, goalWeight );
}

void CTFBotLocomotion::Jump( void )
{
	BaseClass::Jump();

	CTFBot *actor = ToTFBot( this->GetEntity() );

	int nCustomJumpParticle = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( actor, nCustomJumpParticle, bot_custom_jump_particle );
	if ( nCustomJumpParticle != 0 )
	{
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, actor, "foot_L" );
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, actor, "foot_R" );
	}
}

float CTFBotLocomotion::GetMaxJumpHeight( void ) const
{
	return 72.0f;
}

float CTFBotLocomotion::GetDeathDropHeight( void ) const
{
	return 1000.0f;
}

float CTFBotLocomotion::GetRunSpeed( void ) const
{
	CTFBot *actor = ToTFBot( this->GetEntity() );
	return actor->GetPlayerClass()->GetMaxSpeed();
}

bool CTFBotLocomotion::IsAreaTraversable( const CNavArea *baseArea ) const
{
	if ( !BaseClass::IsAreaTraversable( baseArea ) )
		return false;

	const CTFNavArea *tfArea = dynamic_cast<const CTFNavArea *>( baseArea );
	if ( tfArea == nullptr )
		return false;

	// unless the round is over and we are the winning team, we can't enter the other teams spawn
	if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
	{
		switch ( this->GetEntity()->GetTeamNumber() )
		{
			case TF_TEAM_RED:
			{
				if ( tfArea->HasTFAttributes( BLUE_SPAWN_ROOM ) )
					return false;

				break;
			}
			case TF_TEAM_BLUE:
			{
				if ( tfArea->HasTFAttributes( RED_SPAWN_ROOM ) )
					return false;

				break;
			}
			default:
				break;
		}
	}
	else
	{
		if ( TFGameRules()->GetWinningTeam() != this->GetEntity()->GetTeamNumber() )
			return false;
	}

	return true;
}

bool CTFBotLocomotion::IsEntityTraversable( CBaseEntity *ent, TraverseWhenType when ) const
{
	if ( ent )
	{
		// always assume we can walk through players, we'll try to avoid them if needed later
		if ( ent->IsPlayer() )
			return true;

		if ( ent->IsBaseObject() )
		{
			CBaseObject *obj = static_cast<CBaseObject *>( ent );
			if ( obj->GetBuilder() == this->GetEntity() ) // we can't walk through our own buildings...
				return false;
		}
	}

	return BaseClass::IsEntityTraversable( ent, when );
}


CTFBotVision::CTFBotVision( INextBot *bot )
	: BaseClass( bot )
{
	m_updateTimer.Start( RandomFloat( 2.0f, 4.0f ) );
}


CTFBotVision::~CTFBotVision()
{
	m_PVNPCs.RemoveAll();
}

void CTFBotVision::Reset( void )
{
	BaseClass::Reset();
	m_PVNPCs.RemoveAll();
}

void CTFBotVision::Update( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	BaseClass::Update();

	CTFBot *actor = ToTFBot( GetBot()->GetEntity() );
	if ( actor == nullptr )
		return;

	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers( &enemies, GetEnemyTeam( actor ), true );

	for ( CTFPlayer *enemy : enemies )
	{
		if ( enemy->IsPlayerClass( TF_CLASS_SPY ) )
		{
			const CKnownEntity *known = GetKnown( enemy );

			if ( known && ( known->IsVisibleRecently() || !enemy->m_Shared.InCond( TF_COND_DISGUISING ) ) )
				actor->ForgetSpy( enemy );
		}
	}
}

void CTFBotVision::CollectPotentiallyVisibleEntities( CUtlVector<CBaseEntity *> *ents )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	ents->RemoveAll();

	for ( int i=0; i < gpGlobals->maxClients; ++i )
	{
		CBasePlayer *client = UTIL_PlayerByIndex( i );
		if ( client && client->IsConnected() && client->IsAlive() )
		{
			ents->AddToTail( client );
		}
	}

	this->UpdatePotentiallyVisibleNPCs();
	for ( int i=0; i < m_PVNPCs.Count(); ++i )
	{
		CBaseEntity *npc = m_PVNPCs[i];
		ents->AddToTail( npc );
	}
}

bool CTFBotVision::IsVisibleEntityNoticed( CBaseEntity *ent ) const
{
	CTFBot *actor = ToTFBot( GetBot()->GetEntity() );

	CTFPlayer *player = ToTFPlayer( ent );
	if ( player == nullptr )
	{
		return true;
	}

	// we should always be aware of our "friends"
	if ( !actor->IsEnemy( player ) )
	{
		return true;
	}


	if ( player->m_Shared.InCond( TF_COND_BURNING ) ||
		 player->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
		 player->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		if ( player->IsPlayerClass( TF_CLASS_SPY ) )
		{
			actor->RealizeSpy( player );
		}

		return true;
	}

	if ( player->m_Shared.IsStealthed() )
	{
		if ( player->m_Shared.GetPercentInvisible() < 0.75f )
		{
			actor->RealizeSpy( player );
			return true;
		}
		else
		{
			actor->ForgetSpy( player );
			return false;
		}
	}

	return true;
}

bool CTFBotVision::IsIgnored( CBaseEntity *ent ) const
{
	CTFBot *actor = ToTFBot( GetBot()->GetEntity() );
	if ( !actor->IsEnemy( ent ) )
		return false;

	if ( ( ent->GetEffects() & EF_NODRAW ) != 0 )
		return true;

	CTFPlayer *player = ToTFPlayer( ent );
	if ( player )
	{
		if ( player->m_Shared.InCond( TF_COND_BURNING )
			 || player->m_Shared.InCond( TF_COND_STEALTHED_BLINK )
			 || player->m_Shared.InCond( TF_COND_BLEEDING ) )
		{
			return false;
		}

		if ( player->m_Shared.IsStealthed() )
		{
			return ( player->m_Shared.GetPercentInvisible() >= 0.75f );
		}

		if ( !player->m_Shared.InCond( TF_COND_DISGUISED ) ||
			 player->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}

		return ( player->m_Shared.GetDisguiseTeam() == actor->GetTeamNumber() );
	}

	if ( ent->IsBaseObject() )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( ent );
		if ( obj->IsPlacing() || obj->IsBeingCarried() )
			return true;
	}

	return false;
}

float CTFBotVision::GetMaxVisionRange() const
{
	return 6000.0f;
}

float CTFBotVision::GetMinRecognizeTime( void ) const
{
	CTFBot *actor = static_cast<CTFBot *>( this->GetBot() );

	switch ( actor->m_iSkill )
	{
		case CTFBot::NORMAL:
			return 0.50f;
		case CTFBot::HARD:
			return 0.30f;
		case CTFBot::EXPERT:
			return 0.15f;

		default:
			return 1.00f;
	}
}

void CTFBotVision::UpdatePotentiallyVisibleNPCs()
{
	if ( !m_updatePVNPCsTimer.IsElapsed() )
		return;

	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_updatePVNPCsTimer.Start( RandomFloat( 2.0f, 4.0f ) );

	m_PVNPCs.RemoveAll();

	for ( int i=0; i < IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );

		if ( obj->GetType() == OBJ_SENTRYGUN || ( obj->GetType() == OBJ_DISPENSER && obj->ClassMatches( "obj_dispenser" ) ) || obj->GetType() == OBJ_TELEPORTER )
		{
			m_PVNPCs.AddToTail( obj );
		}
	}

	CUtlVector<INextBot *> nextbots;
	TheNextBots().CollectAllBots( &nextbots );
	for ( INextBot *nextbot : nextbots )
	{
		CBaseCombatCharacter *ent = nextbot->GetEntity();
		if ( ent && !ent->IsPlayer() )
			m_PVNPCs.AddToTail( ent );
	}
}
