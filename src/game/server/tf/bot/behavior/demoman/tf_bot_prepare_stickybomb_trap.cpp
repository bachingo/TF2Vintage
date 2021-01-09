#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_gamerules.h"
#include "tf_weapon_pipebomblauncher.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_prepare_stickybomb_trap.h"


ConVar tf_bot_stickybomb_density( "tf_bot_stickybomb_density", "0.0001", FCVAR_CHEAT, "Number of stickies to place per square inch" );


class PlaceStickyBombReply : public INextBotReply
{
public:
	virtual void OnSuccess( INextBot *nextbot ) OVERRIDE;
	virtual void OnFail( INextBot *nextbot, FailureReason reason ) OVERRIDE
	{
		if ( m_pAimDuration )
			m_pAimDuration->Invalidate();
	}

	void Init( CTFBotPrepareStickybombTrap::BombTargetArea *target, CountdownTimer *timer )
	{
		m_pBombTargetArea = target;
		m_pAimDuration    = timer;
	}
	void Reset()
	{
		m_pBombTargetArea = nullptr;
		m_pAimDuration    = nullptr;
	}

private:
	CTFBotPrepareStickybombTrap::BombTargetArea *m_pBombTargetArea;
	CountdownTimer *m_pAimDuration;
};
static PlaceStickyBombReply bombReply;

void PlaceStickyBombReply::OnSuccess( INextBot *nextbot )
{
	CTFBot *actor = ToTFBot( nextbot->GetEntity() );

	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if ( weapon && weapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER )
	{
		actor->PressFireButton( 0.1f );

		if ( m_pBombTargetArea )
		{
			++m_pBombTargetArea->stickies;
		}

		if ( m_pAimDuration )
		{
			m_pAimDuration->Start( 0.15f );
		}
	}
}


CTFBotPrepareStickybombTrap::CTFBotPrepareStickybombTrap()
{
	m_LastKnownArea = nullptr;
	m_BombTargetAreas.RemoveAll();
	m_aimDuration.Invalidate();
}

CTFBotPrepareStickybombTrap::~CTFBotPrepareStickybombTrap()
{
	bombReply.Reset();
}


const char *CTFBotPrepareStickybombTrap::GetName() const
{
	return "PrepareStickybombTrap";
}


ActionResult<CTFBot> CTFBotPrepareStickybombTrap::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	CTFPipebombLauncher *launcher = dynamic_cast<CTFPipebombLauncher *>( me->Weapon_GetSlot( 1 ) );
	if ( launcher && me->GetAmmoCount( TF_AMMO_SECONDARY ) >= launcher->GetMaxClip1() && launcher->Clip1() < launcher->GetMaxClip1() )
	{
		m_bReload = true;
	}
	else
	{
		m_bReload = false;
	}

	m_LastKnownArea = me->GetLastKnownArea();
	if ( m_LastKnownArea == nullptr )
		return Action<CTFBot>::Done( "No nav mesh" );

	this->InitBombTargetAreas( me );

	me->StopLookingForEnemies();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPrepareStickybombTrap::Update( CTFBot *me, float dt )
{
	if ( !TFGameRules()->InSetup() )
	{
		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat && ( threat->GetLastKnownPosition() - me->GetAbsOrigin() ).LengthSqr() < Square( 500.0f ) )
			return Action<CTFBot>::Done( "Enemy nearby - giving up" );
	}

	if ( me->GetLastKnownArea() && me->GetLastKnownArea() != m_LastKnownArea )
	{
		m_LastKnownArea = me->GetLastKnownArea();
		this->InitBombTargetAreas( me );
	}

	CTFWeaponBase *active = me->m_Shared.GetActiveTFWeapon();
	CTFPipebombLauncher *launcher = dynamic_cast<CTFPipebombLauncher *>( me->Weapon_GetSlot( 1 ) );
	if ( launcher == nullptr || active == nullptr )
	{
		return Action<CTFBot>::Done( "Missing weapon" );
	}

	if ( active->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER )
	{
		me->Weapon_Switch( launcher );
	}

	if ( m_bReload )
	{
		int iMaxClip = Max( me->GetAmmoCount( TF_AMMO_SECONDARY ), launcher->GetMaxClip1() );
		
		if ( launcher->Clip1() >= iMaxClip )
		{
			m_bReload = false;
		}

		me->PressReloadButton();
	}
	else
	{
		if ( launcher->GetPipeBombCount() >= 8 || me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
		{
			return Action<CTFBot>::Done( "Max sticky bombs reached" );
		}
		else
		{
			if ( m_aimDuration.IsElapsed() )
			{
				FOR_EACH_VEC( m_BombTargetAreas, i )
				{
					BombTargetArea *target = &m_BombTargetAreas[i];

					int iWantedStickies = Max( 1, (int)( target->area->GetSizeX() * target->area->GetSizeY() *  tf_bot_stickybomb_density.GetFloat() ) );

					if ( target->stickies < iWantedStickies )
					{
						bombReply.Init( target, &this->m_aimDuration );
						m_aimDuration.Start( 2.0f );

						me->GetBodyInterface()->AimHeadTowards( target->area->GetRandomPoint(), IBody::IMPORTANT, 5.0f, &bombReply, "Aiming a sticky bomb" );

						return Action<CTFBot>::Continue();
					}
				}

				return Action<CTFBot>::Done( "Exhausted bomb target areas" );
			}
		}
	}

	return Action<CTFBot>::Continue();
}

void CTFBotPrepareStickybombTrap::OnEnd( CTFBot *actor, Action<CTFBot> *newAction )
{
	actor->GetBodyInterface()->ClearPendingAimReply();

	actor->WantsToLookForEnemies();
}

ActionResult<CTFBot> CTFBotPrepareStickybombTrap::OnSuspend( CTFBot *actor, Action<CTFBot> *newAction )
{
	return Action<CTFBot>::Done();
}


EventDesiredResult<CTFBot> CTFBotPrepareStickybombTrap::OnInjured( CTFBot *actor, const CTakeDamageInfo &info )
{
	return Action<CTFBot>::TryDone( RESULT_IMPORTANT, "Ouch!" );
}


QueryResultType CTFBotPrepareStickybombTrap::ShouldAttack( const INextBot *nextbot, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}


bool CTFBotPrepareStickybombTrap::IsPossible( CTFBot *actor )
{
	if ( actor->GetTimeSinceLastInjury() < 1.0f )
	{
		return false;
	}

	if ( !actor->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		return false;
	}

	CTFPipebombLauncher *launcher = dynamic_cast<CTFPipebombLauncher *>( actor->Weapon_GetSlot( 1 ) );
	if ( launcher != nullptr/* && !actor->IsWeaponRestricted( launcher ) */)
	{
		if ( launcher->GetPipeBombCount() >= 8 )
		{
			return false;
		}

		if ( actor->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
		{
			return false;
		}
	}

	return true;
}


void CTFBotPrepareStickybombTrap::InitBombTargetAreas( CTFBot *actor )
{
	const CUtlVector<CTFNavArea *> &invasionAreas = m_LastKnownArea->GetInvasionAreasForTeam( actor->GetTeamNumber() );

	/* intentional array copy */
	CUtlVector<CTFNavArea *> areas;
	areas.CopyArray( invasionAreas.Base(), invasionAreas.Count() );
	areas.Shuffle();

	m_BombTargetAreas.RemoveAll();

	for( int i=0; i<areas.Count(); ++i )
	{
		BombTargetArea target;
		target.area = areas[i];
		target.stickies = 0;

		m_BombTargetAreas.AddToTail( target );
	}

	m_aimDuration.Invalidate();

	actor->GetBodyInterface()->ClearPendingAimReply();
}
