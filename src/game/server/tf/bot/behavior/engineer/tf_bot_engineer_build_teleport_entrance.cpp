#include "cbase.h"
#include "../../tf_bot.h"
#include "team_control_point.h"
#include "func_capture_zone.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_weapon_builder.h"
#include "tf_bot_engineer_build_teleport_entrance.h"
#include "tf_bot_engineer_move_to_build.h"
#include "../tf_bot_get_ammo.h"


ConVar tf_bot_max_teleport_entrance_travel( "tf_bot_max_teleport_entrance_travel", "1500", FCVAR_CHEAT, "Don't plant teleport entrances farther than this travel distance from our spawn room" );
ConVar tf_bot_teleport_build_surface_normal_limit( "tf_bot_teleport_build_surface_normal_limit", "0.99", FCVAR_CHEAT, "If the ground normal Z component is less that this value, Engineer bots won't place their entrance teleporter" );


CTFBotEngineerBuildTeleportEntrance::CTFBotEngineerBuildTeleportEntrance()
{
}

CTFBotEngineerBuildTeleportEntrance::~CTFBotEngineerBuildTeleportEntrance()
{
}


const char *CTFBotEngineerBuildTeleportEntrance::GetName() const
{
	return "EngineerBuildTeleportEntrance";
}


ActionResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::Update( CTFBot *me, float dt )
{
	CBaseEntity *pPoint = me->GetMyControlPoint();
	if ( !pPoint )
	{
		pPoint = me->GetFlagCaptureZone();
		if( !pPoint )
			return Action<CTFBot>::Continue();
	}

	CTFNavArea *pArea = me->GetLastKnownArea();
	if ( !pArea )
		return Action<CTFBot>::Done( "No nav mesh!" );

	if ( pArea->GetIncursionDistance( me->GetTeamNumber() ) > tf_bot_max_teleport_entrance_travel.GetFloat() )
		return Action<CTFBot>::ChangeTo( new CTFBotEngineerMoveToBuild, "Too far from our spawn room to build teleporter entrance" );

	if ( me->IsAmmoFull() || !CTFBotGetAmmo::IsPossible( me ) )
	{
		if ( me->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE ) )
			return Action<CTFBot>::ChangeTo( new CTFBotEngineerMoveToBuild, "Teleport entrance built" );

		if ( !m_PathFollower.IsValid() )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_PathFollower.Compute( me, pPoint->GetAbsOrigin(), cost );
		}

		m_PathFollower.Update( me );

		CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>( me->GetActiveTFWeapon() );
		if ( pBuilder )
		{
			Vector vecStart, vecEnd, vecFwd;
			me->EyeVectors( &vecFwd );

			vecStart = me->WorldSpaceCenter();
			vecEnd = me->WorldSpaceCenter() - Vector( 0, 0, 200.0f );

			vecFwd *= 30.0f;

			trace_t trace;
			UTIL_TraceLine( vecStart + vecFwd, vecEnd + vecFwd, MASK_PLAYERSOLID, (IHandleEntity *)me, COLLISION_GROUP_NONE, &trace );

			if ( pBuilder->IsValidPlacement() && trace.DidHit() &&
				 trace.plane.normal.z > tf_bot_teleport_build_surface_normal_limit.GetFloat() )
			{
				me->PressFireButton();
			}
		}
		else
		{
			me->StartBuildingObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE );
		}
	}
	else
	{
		return Action<CTFBot>::SuspendFor( new CTFBotGetAmmo, "Refilling ammo" );
	}

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::OnStuck( CTFBot *me )
{
	m_PathFollower.Invalidate();

	return Action<CTFBot>::TryContinue();
}
