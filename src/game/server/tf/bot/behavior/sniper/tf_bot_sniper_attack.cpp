#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_obj_sentrygun.h"
#include "tf_bot_sniper_attack.h"
#include "../tf_bot_retreat_to_cover.h"


ConVar tf_bot_sniper_flee_range( "tf_bot_sniper_flee_range", "400", FCVAR_CHEAT, "If threat is closer than this, retreat" );
ConVar tf_bot_sniper_melee_range( "tf_bot_sniper_melee_range", "200", FCVAR_CHEAT, "If threat is closer than this, attack with melee weapon" );
ConVar tf_bot_sniper_linger_time( "tf_bot_sniper_linger_time", "5", FCVAR_CHEAT, "How long Sniper will wait around after losing his target before giving up" );


CTFBotSniperAttack::CTFBotSniperAttack()
{
}

CTFBotSniperAttack::~CTFBotSniperAttack()
{
}


const char *CTFBotSniperAttack::GetName() const
{
	return "SniperAttack";
}


ActionResult<CTFBot> CTFBotSniperAttack::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperAttack::Update( CTFBot *me, float dt )
{
	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	if ( pPrimary != nullptr )
	{
		me->Weapon_Switch( pPrimary );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( false );
	if ( threat == nullptr || !threat->GetEntity()->IsAlive() || !threat->IsVisibleInFOVNow() )
	{
		if ( m_lingerDuration.IsElapsed() && !me->m_Shared.InCond( TF_COND_ZOOMED ) )
			return Action<CTFBot>::Done( "No threat for a while" );

		return Action<CTFBot>::Continue();
	}

	me->EquipBestWeaponForThreat( threat );

	if ( ( me->GetAbsOrigin() - threat->GetLastKnownPosition() ).LengthSqr() < Square( tf_bot_sniper_flee_range.GetFloat() ) )
	{
		return Action<CTFBot>::SuspendFor( new CTFBotRetreatToCover, "Retreating from nearby enemy" );
	}

	if ( me->GetTimeSinceLastInjury() < 1.0f )
	{
		return Action<CTFBot>::SuspendFor( new CTFBotRetreatToCover, "Retreating due to injury" );
	}

	m_lingerDuration.Start( RandomFloat( 0.75f, 1.25f ) * tf_bot_sniper_linger_time.GetFloat() );

	if ( !me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}

	return Action<CTFBot>::Continue();
}

void CTFBotSniperAttack::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}
}

ActionResult<CTFBot> CTFBotSniperAttack::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperAttack::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	return Action<CTFBot>::Continue();
}


Vector CTFBotSniperAttack::SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *them ) const
{
	VPROF_BUDGET( "CTFBotSniperAttack::SelectTargetPoint", "NextBot" );

	trace_t trace;
	NextBotTraceFilterIgnoreActors filter( me->GetEntity(), COLLISION_GROUP_NONE );

	Vector vecStart, vecEnd;
	
	vecStart = me->GetBodyInterface()->GetEyePosition();
	vecEnd = them->EyePosition();
	UTIL_TraceLine( vecStart, vecEnd, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &trace );

	if ( trace.DidHit() )
	{
		vecEnd = them->WorldSpaceCenter();
		UTIL_TraceLine( vecStart, vecEnd, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &trace );

		if ( trace.DidHit() )
		{
			vecEnd = them->GetAbsOrigin();
			UTIL_TraceLine( vecStart, vecEnd, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &trace );
		}
	}

	return trace.endpos;
}

const CKnownEntity *CTFBotSniperAttack::SelectMoreDangerousThreat( const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	if ( IsImmediateThreat( them, threat1 ) )
	{
		if ( IsImmediateThreat( them, threat2 ) )
			return nullptr;

		return threat1;
	}
	else
	{
		if ( IsImmediateThreat( them, threat2 ) )
			return threat2;

		return nullptr;
	}
}


bool CTFBotSniperAttack::IsPossible( CTFBot *actor )
{
	if ( actor->IsPlayerClass( TF_CLASS_SNIPER ) && actor->GetVisionInterface()->GetPrimaryKnownThreat() &&
		 actor->GetVisionInterface()->GetPrimaryKnownThreat()->IsVisibleRecently() )
	{
		return true;
	}

	return false;
}


bool CTFBotSniperAttack::IsImmediateThreat( const CBaseCombatCharacter *who, const CKnownEntity *threat ) const
{
	if ( who->InSameTeam( threat->GetEntity() ) )
		return false;

	if ( !threat->GetEntity()->IsAlive() )
		return false;

	if ( !threat->WasEverVisible() || threat->GetTimeSinceLastSeen() > 3.0f )
		return false;

	Vector vecFwd;
	Vector vecToThreat = ( who->GetAbsOrigin() - threat->GetLastKnownPosition() );
	vecToThreat.NormalizeInPlace();

	CTFPlayer *pPlayer = ToTFPlayer( threat->GetEntity() );
	if ( pPlayer )
	{
		// Counter-sniping is primary concern
		if ( pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
			return true;

		pPlayer->EyeVectors( &vecFwd );
		if ( vecToThreat.Dot( vecFwd ) > 0.8f )
			return true;

		// Always go for the healer
		return pPlayer->IsPlayerClass( TF_CLASS_MEDIC );
	}

	CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( threat->GetEntity() );
	if ( pSentry )
	{
		QAngle turretAng = pSentry->GetTurretAngles();
		AngleVectors( turretAng, &vecFwd );

		if ( vecToThreat.Dot( vecFwd ) > 0.8f )
			return true;
	}

	return false;
}
