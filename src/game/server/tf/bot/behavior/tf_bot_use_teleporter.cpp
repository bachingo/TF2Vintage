#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_use_teleporter.h"
#include "tf_obj_teleporter.h"


CTFBotUseTeleporter::CTFBotUseTeleporter(CObjectTeleporter *teleporter, UseHowType how)
{
	m_hTele = teleporter;
	
	m_bTeleported = false;
	
	m_iHow = how;
}

CTFBotUseTeleporter::~CTFBotUseTeleporter()
{
}


const char *CTFBotUseTeleporter::GetName( void ) const
{
	return "UseTeleporter";
}


ActionResult<CTFBot> CTFBotUseTeleporter::OnStart(CTFBot *me, Action<CTFBot> *priorAction)
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUseTeleporter::Update(CTFBot *me, float dt)
{
	if (m_hTele == nullptr)
		return Action<CTFBot>::Done("Teleporter is gone");
	
	CObjectTeleporter *exit = m_hTele->GetMatchingTeleporter();
	if (exit == nullptr)
		return Action<CTFBot>::Done("Missing teleporter exit");
	
	if (m_hTele->IsSendingPlayer( me ))
	{
		m_bTeleported = true;
	}
	else if (!m_bTeleported)
	{
		if (!IsTeleporterAvailable() && ( m_iHow == USE_IMMEDIATE ))
			return Action<CTFBot>::Done( "Teleporter is unavailable" );
	}

	if (me->IsRangeLessThan( exit, 2.5f ))
		return Action<CTFBot>::Done( "Successful teleport" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat && threat->IsVisibleRecently())
		me->EquipBestWeaponForThreat( threat );

	if (m_recomputePath.IsElapsed())
	{
		m_recomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost func( me, FASTEST_ROUTE );
		if (!m_PathFollower.Compute( me, m_hTele->GetAbsOrigin(), func ))
			return Action<CTFBot>::Done( "Can't reach teleporter!" );
	}

	if (me->GetLocomotionInterface()->GetGround() != m_hTele)
		m_PathFollower.Update( me );
	
	return Action<CTFBot>::Continue();
}


bool CTFBotUseTeleporter::IsTeleporterAvailable( void ) const
{
	if (m_hTele == nullptr || !m_hTele->IsReady())
		return false;
	
	return (this->m_hTele->GetState() == TELEPORTER_STATE_READY);
}
