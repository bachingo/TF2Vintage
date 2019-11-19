#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_fx.h"
#include "tf_zombie.h"
#include "zombie_special_attack.h"

const char *CZombieSpecialAttack::GetName( void ) const
{
	return "SpecialAttack";
}

ActionResult<CZombie> CZombieSpecialAttack::OnStart( CZombie *me, Action<CZombie> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_SPECIAL_ATTACK1 );

	m_timeUntilAttack.Start( 1.0f );

	return BaseClass::Continue();
}

ActionResult<CZombie> CZombieSpecialAttack::Update( CZombie *me, float dt )
{
	if ( m_timeUntilAttack.HasStarted() && m_timeUntilAttack.IsElapsed() )
	{
		DoSpecialAttack( me );
		m_timeUntilAttack.Invalidate();

		if ( !me->IsActivityFinished() )
			return BaseClass::Continue();

		return BaseClass::Done();
	}

	if ( me->IsActivityFinished() )
		return BaseClass::Done();

	return BaseClass::Continue();
}

void CZombieSpecialAttack::DoSpecialAttack( CZombie *actor )
{
	Vector vecOrigin = actor->GetAbsOrigin();

	// Thanks Pelipoika for figuring this out
	Vector vecFwd, vecRight, vecUp;
	actor->GetVectors( &vecFwd, &vecRight, &vecUp );
	vecOrigin += vecFwd * 55;
	vecOrigin -= vecRight * 35;
	// --------------------------------------

	CPVSFilter filter( actor->GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0, "bomibomicon_ring", vecOrigin, vec3_angle );

	int iTeam = actor->GetTeamNumber() == TF_TEAM_NPC ? TEAM_ANY : GetEnemyTeam( actor );
	CUtlVector<CTFPlayer *> playersPushed;
	TFGameRules()->PushAllPlayersAway( actor->GetAbsOrigin(), 200.0f, 500.0f, iTeam, &playersPushed );

	CBaseEntity *pAttacker = actor;
	if ( actor->GetOwnerEntity() )
		pAttacker = actor->GetOwnerEntity();

	FOR_EACH_VEC( playersPushed, i )
	{
		CTFPlayer *pPlayer = playersPushed[i];

		Vector vecDir = pPlayer->WorldSpaceCenter() - actor->WorldSpaceCenter();
		vecDir.NormalizeInPlace();

		CTakeDamageInfo info( actor, pAttacker, actor->m_flAttDamage, DMG_SLASH, TF_DMG_CUSTOM_SPELL_SKELETON );
		CalculateMeleeDamageForce( &info, vecDir, vecOrigin, 5.0f );
		pPlayer->TakeDamage( info );
	}
}
