//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_player.h"
#include "tf_zombie.h"
#include "zombie_attack.h"
#include "zombie_special_attack.h"

char const *CZombieAttack::GetName( void ) const
{
	return "Attack";
}

ActionResult<CZombie> CZombieAttack::OnStart( CZombie *me, Action<CZombie> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( 100.0f );

	m_specialAttackTimer.Start( RandomFloat( 5.0f, 10.0f ) );

	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );

	return BaseClass::Continue();
}

ActionResult<CZombie> CZombieAttack::Update( CZombie *me, float dt )
{
	if ( !me->IsAlive() )
		return BaseClass::Done();

	if ( !unk4.IsElapsed() )
		return BaseClass::Continue();

	SelectVictim( me );
	if ( !m_hTarget || !m_hTarget->IsAlive() )
		return BaseClass::Continue();

	if ( m_PathFollower.GetAge() > 0.5f )
	{
		CZombiePathCost func( me );
		m_PathFollower.Compute( me, m_hTarget, func );

		return BaseClass::Continue();
	}

	if ( me->IsRangeGreaterThan( m_hTarget, 50.0f ) || !me->IsLineOfSightClear( m_hTarget ) )
		m_PathFollower.Update( me );

	if ( !me->IsRangeLessThan( m_hTarget, 150.0f ) )
		return BaseClass::Continue();

	me->GetLocomotionInterface()->FaceTowards( m_hTarget->WorldSpaceCenter() );

	if ( !me->IsPlayingGesture( ACT_MP_ATTACK_STAND_MELEE ) )
		me->AddGesture( ACT_MP_ATTACK_STAND_MELEE );

	if ( me->IsRangeLessThan( m_hTarget, me->m_flAttRange ) )
	{
		if ( me->m_nType == CZombie::KING )
		{
			if ( m_specialAttackTimer.IsElapsed() )
			{
				m_specialAttackTimer.Start( RandomFloat( 5.0f, 10.0f ) );
				return BaseClass::SuspendFor( new CZombieSpecialAttack, "Do my special move!" );
			}
		}

		if ( m_attackTimer.IsElapsed() )
		{
			m_attackTimer.Start( RandomFloat( 0.8f, 1.2f ) );

			Vector vecDir = m_hTarget->WorldSpaceCenter() - me->WorldSpaceCenter();
			vecDir.NormalizeInPlace();

			CBaseEntity *pAttacker = me;
			if ( me->GetOwnerEntity() )
				pAttacker = me->GetOwnerEntity();

			CTakeDamageInfo info( me, pAttacker, me->m_flAttDamage, DMG_SLASH );
			CalculateMeleeDamageForce( &info, vecDir, me->WorldSpaceCenter(), 5.0f );
			m_hTarget->TakeDamage( info );
		}
	}

	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );

	return BaseClass::Continue();
}

EventDesiredResult<CZombie> CZombieAttack::OnContact( CZombie *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other->IsPlayer() && me->GetTeamNumber() != TF_TEAM_NPC )
	{
		CTFPlayer *pPlayer = ToTFPlayer( other );
		if ( pPlayer->IsAlive() && pPlayer->GetTeamNumber() != me->GetTeamNumber() )
		{
			m_hTarget = pPlayer;
			m_keepTargetDuration.Start( 3.0f );
		}
	}

	return BaseClass::TryContinue();
}

EventDesiredResult<CZombie> CZombieAttack::OnStuck( CZombie *me )
{
	CTakeDamageInfo info( me, me, 99999.9f, DMG_SLASH );
	me->TakeDamage( info );

	return BaseClass::TryContinue();
}

EventDesiredResult<CZombie> CZombieAttack::OnOtherKilled( CZombie *me, CBaseCombatCharacter *victim, CTakeDamageInfo const &info )
{
	return BaseClass::TryContinue();
}

bool CZombieAttack::IsPotentiallyChaseable( CZombie *actor, CBaseCombatCharacter *other )
{
	if ( !other || !other->IsAlive() )
		return false;

	CTFNavArea *pArea = (CTFNavArea *)other->GetLastKnownArea();
	if ( pArea == nullptr )
		return false;

	if ( pArea->HasTFAttributes( RED_SPAWN_ROOM | BLUE_SPAWN_ROOM ) )
		return false;

	if ( other->GetGroundEntity() == nullptr )
		return false;

	Vector vecPoint;
	pArea->GetClosestPointOnArea( actor->GetAbsOrigin(), &vecPoint );

	return ( actor->GetAbsOrigin() - vecPoint ).Length() <= 2500.0f;
}

void CZombieAttack::SelectVictim( CZombie *actor )
{
	if ( IsPotentiallyChaseable( actor, m_hTarget ) && !m_keepTargetDuration.IsElapsed() )
		return;
	
	CUtlVector<CTFPlayer *> players;
	switch ( actor->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			CollectPlayers( &players, TF_TEAM_BLUE, true );
			break;
		case TF_TEAM_BLUE:
			CollectPlayers( &players, TF_TEAM_RED, true );
			break;
		default:
			CollectPlayers( &players, TF_TEAM_RED, true );
			CollectPlayers( &players, TF_TEAM_RED, true, true );
			break;
	}

	CBaseCombatCharacter *pClosest = nullptr;
	float flMinDist = FLT_MAX;

	FOR_EACH_VEC( players, i )
	{
		CTFPlayer *pPlayer = players[i];
		if ( !IsPotentiallyChaseable( actor, pPlayer ) )
			continue;

		if ( pPlayer->m_Shared.IsStealthed() && !pPlayer->m_Shared.InCond( TF_COND_BURNING ) && !pPlayer->m_Shared.InCond( TF_COND_URINE ) && !pPlayer->m_Shared.InCond( TF_COND_MAD_MILK ) && !pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
			continue;

		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == actor->GetTeamNumber() )
			continue;

		float flDistance = actor->GetRangeSquaredTo( pPlayer );
		if ( flDistance < flMinDist )
		{
			flMinDist = flDistance;
			pClosest = pPlayer;
		}
	}

	FOR_EACH_VEC( IZombieAutoList::AutoList(), i )
	{
		CZombie *pZombie = (CZombie *)IZombieAutoList::AutoList()[i];
		if ( pZombie->GetTeamNumber() == actor->GetTeamNumber() )
			continue;

		float flDistance = actor->GetRangeSquaredTo( pZombie );
		if ( flDistance < flMinDist )
		{
			flMinDist = flDistance;
			pClosest = pZombie;
		}
	}

	m_hTarget = pClosest;
	m_keepTargetDuration.Start( 3.0f );
}
