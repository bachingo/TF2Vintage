#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_bot_medic_retreat.h"
#include "nav_mesh/tf_nav_area.h"


class CUsefulHealTargetFilter : public INextBotEntityFilter
{
public:
	CUsefulHealTargetFilter( int teamNum )
		: m_iTeamNum( teamNum )
	{
	};

	virtual bool IsAllowed( CBaseEntity *ent ) const
	{
		if ( ent == nullptr || !ent->IsPlayer() || ent->GetTeamNumber() != m_iTeamNum )
			return false;

		if ( ToTFPlayer( ent )->IsPlayerClass( TF_CLASS_MEDIC ) ||
			 ToTFPlayer( ent )->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			return false;
		}

		return true;
	}

private:
	int m_iTeamNum;
};


CTFBotMedicRetreat::CTFBotMedicRetreat()
{
}

CTFBotMedicRetreat::~CTFBotMedicRetreat()
{
}


const char *CTFBotMedicRetreat::GetName( void ) const
{
	return "MedicRetreat";
}


ActionResult<CTFBot> CTFBotMedicRetreat::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	if ( me->m_HomeArea == nullptr )
		return Action<CTFBot>::Done( "No home area!" );

	CTFBotPathCost cost( me, FASTEST_ROUTE );
	m_PathFollower.Compute( me, me->m_HomeArea->GetCenter(), cost );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMedicRetreat::Update( CTFBot *me, float dt )
{
	CTFWeaponBase *weapon = me->m_Shared.GetActiveTFWeapon();
	if ( weapon && weapon->GetWeaponID() != TF_WEAPON_SYRINGEGUN_MEDIC )
	{
		CBaseCombatWeapon *primary = me->Weapon_GetSlot( 0 );
		if ( primary )
			me->Weapon_Switch( primary );
	}

	m_PathFollower.Update( me );

	if ( m_lookForPatientsTimer.IsElapsed() )
	{
		m_lookForPatientsTimer.Start( RandomFloat( 0.33f, 1.0f ) );

		Vector fwd;
		AngleVectors( QAngle( 0.0f, RandomFloat( -180.0f, 180.0f ), 0.0f ), &fwd );

		me->GetBodyInterface()->AimHeadTowards( me->EyePosition() + fwd, IBody::IMPORTANT, 0.1f, nullptr, "Looking for someone to heal" );
	}

	CUsefulHealTargetFilter filter( me->GetTeamNumber() );
	if ( me->GetVisionInterface()->GetClosestKnown( filter ) != nullptr )
		return Action<CTFBot>::Done( "I know of a teammate" );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMedicRetreat::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	CTFBotPathCost cost( me, FASTEST_ROUTE );
	m_PathFollower.Compute( me, me->m_HomeArea->GetCenter(), cost );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	CTFBotPathCost cost( me, FASTEST_ROUTE );
	m_PathFollower.Compute( me, me->m_HomeArea->GetCenter(), cost );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnStuck( CTFBot *me )
{
	CTFBotPathCost func( me, FASTEST_ROUTE );
	m_PathFollower.Compute( me, me->m_HomeArea->GetCenter(), func );

	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotMedicRetreat::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_YES;
}
