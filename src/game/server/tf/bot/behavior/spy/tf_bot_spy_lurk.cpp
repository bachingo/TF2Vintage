#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_obj.h"
#include "tf_bot_spy_lurk.h"
#include "tf_bot_spy_attack.h"
#include "tf_bot_spy_sap.h"


CTFBotSpyLurk::CTFBotSpyLurk()
{
}

CTFBotSpyLurk::~CTFBotSpyLurk()
{
}


const char *CTFBotSpyLurk::GetName() const
{
	return "SpyLurk";
}


ActionResult<CTFBot> CTFBotSpyLurk::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	if ( !me->m_Shared.IsStealthed() )
		me->PressAltFireButton();

	me->DisguiseAsEnemy();

	m_patienceDuration.Start( RandomFloat( 3.0f, 5.0f ) );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyLurk::Update( CTFBot *actor, float dt )
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->GetEntity() )
	{
		CBaseObject *obj = dynamic_cast<CBaseObject *>( threat->GetEntity() );
		if ( obj && !obj->HasSapper() && actor->IsEnemy( obj ) )
			return Action<CTFBot>::SuspendFor( new CTFBotSpySap( obj ), "Sapping an enemy object" );
	}

	if ( actor->GetTargetSentry() != nullptr && !actor->GetTargetSentry()->HasSapper() )
		return Action<CTFBot>::SuspendFor( new CTFBotSpySap( actor->GetTargetSentry() ), "Sapping a Sentry" );

	if ( m_patienceDuration.IsElapsed() )
		return Action<CTFBot>::Done( "Lost patience with my hiding spot" );

	if ( actor->GetLastKnownArea() && threat && threat->GetTimeSinceLastKnown() < 3.0f )
	{
		CTFPlayer *victim = ToTFPlayer( threat->GetEntity() );
		if ( victim && !victim->IsLookingTowards( actor, 0.9f ) )
			return Action<CTFBot>::ChangeTo( new CTFBotSpyAttack( victim ), "Going after a backstab victim" );
	}

	return Action<CTFBot>::Continue();
}


QueryResultType CTFBotSpyLurk::ShouldAttack( const INextBot *nextbot, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}
