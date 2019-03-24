#include "cbase.h"
#include "fmtstr.h"
#include "utlbuffer.h"
#include "tier0/vprof.h"
#include "functorutils.h"
#include "triggers.h"
#include "filters.h"

#include "tf_nav_mesh.h"

#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "tf_obj.h"

#include "NextBotUtil.h"

ConVar tf_show_in_combat_areas( "tf_show_in_combat_areas", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );
ConVar tf_show_mesh_decoration( "tf_show_mesh_decoration", "0", FCVAR_CHEAT, "Highlight special areas", true, 0.0f, true, 1.0f );
ConVar tf_show_enemy_invasion_areas( "tf_show_enemy_invasion_areas", "0", FCVAR_CHEAT, "Highlight areas where the enemy team enters the visible environment of the local player", true, 0.0f, true, 1.0f );
ConVar tf_show_blocked_areas( "tf_show_blocked_areas", "0", FCVAR_CHEAT, "Highlight areas that are considered blocked for TF-specific reasons", true, 0.0f, true, 1.0f );
ConVar tf_show_incursion_flow( "tf_show_incursion_flow", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );
ConVar tf_show_incursion_flow_range( "tf_show_incursion_flow_range", "150", FCVAR_CHEAT, "" );
ConVar tf_show_incursion_flow_gradient( "tf_show_incursion_flow_gradient", "0", FCVAR_CHEAT, "1 = red, 2 = blue", true, 0.0f, true, 2.0f );

ConVar tf_bot_min_setup_gate_defend_range( "tf_bot_min_setup_gate_defend_range", "750", FCVAR_CHEAT, "How close from the setup gate(s) defending bots can take up positions. Areas closer than this will be in cover to ambush." );
ConVar tf_bot_max_setup_gate_defend_range( "tf_bot_max_setup_gate_defend_range", "2000", FCVAR_CHEAT, "How far from the setup gate(s) defending bots can take up positions" );

ConVar tf_select_ambush_areas_radius( "tf_select_ambush_areas_radius", "750", FCVAR_CHEAT );
ConVar tf_select_ambush_areas_close_range( "tf_select_ambush_areas_close_range", "300", FCVAR_CHEAT );
ConVar tf_select_ambush_areas_max_enemy_exposure_area( "tf_select_ambush_areas_max_enemy_exposure_area", "500000", FCVAR_CHEAT );

void TestAndBlockOverlappingAreas( CBaseEntity *pBlocker )
{
	/*NextBotTraceFilterIgnoreActors filter( pBlocker, COLLISION_GROUP_NONE );

	Extent blockerExtent;
	blockerExtent.Init( pBlocker );

	CUtlVector<CNavArea *> potentiallyBlockedAreas;
	TheNavMesh->CollectAreasOverlappingExtent( blockerExtent, &potentiallyBlockedAreas );

	for (int i=0; i<potentiallyBlockedAreas.Count(); ++i)
	{
		CNavArea *area = potentiallyBlockedAreas[i];

		// some funky ass SSE intrinsics with the areas
		// m_nwCorner, m_neZ and m_seCorner, m_swZ happens

		// could just be conditional Ray_t.Init

		Ray_t ray;
		//

		trace_t tr;
		enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &tr );

		if (tr.DidHit() && tr.m_pEnt && tr.m_pEnt->ShouldBlockNav())
			area->MarkAsBlocked( TEAM_ANY, pBlocker );
	}*/
}

CTFNavMesh::CTFNavMesh()
{
	ListenForGameEvent( "teamplay_setup_finished" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_point_unlocked" );
	ListenForGameEvent( "player_builtobject" );
	ListenForGameEvent( "player_dropobject" );
	ListenForGameEvent( "player_carryobject" );
	ListenForGameEvent( "object_detonated" );
	ListenForGameEvent( "object_destroyed" );
}

CTFNavMesh::~CTFNavMesh()
{
}

void CTFNavMesh::FireGameEvent( IGameEvent *event )
{
	CUtlString string( event->GetName() );
	CNavMesh::FireGameEvent( event );

	if (string.IsEqual_CaseSensitive( "teamplay_point_captured" ))
	{
		m_pointChangedIdx = event->GetInt( "cp" );
		m_pointState = CP_STATE_OWNERSHIP_CHANGED;
		m_recomputeTimer.Start( 2.0f );
	}

	if (string.IsEqual_CaseSensitive( "teamplay_setup_finished" ))
	{
		m_pointChangedIdx = 0;
		m_pointState = CP_STATE_RESET;
		m_recomputeTimer.Start( 2.0f );
	}

	if (string.IsEqual_CaseSensitive( "teamplay_point_unlocked" ))
	{
		m_pointChangedIdx = event->GetInt( "cp" );
		m_pointState = CP_STATE_AWAITING_CAPTURE;
		m_recomputeTimer.Start( 2.0f );
	}

	if (string.IsEqual_CaseSensitive( "player_builtobject" ) ||
		string.IsEqual_CaseSensitive( "player_carryobject" ) ||
		string.IsEqual_CaseSensitive( "player_dropobject" ) ||
		string.IsEqual_CaseSensitive( "object_detonated" ) ||
		string.IsEqual_CaseSensitive( "object_destroyed" ))
	{
		int iObjectType = event->IsEmpty( "objecttype" ) ? event->GetInt( "object" ) : event->GetInt( "objecttype" );
		if (iObjectType == OBJ_SENTRYGUN)
		{
			if (false)
			{
				DevMsg( "%s: Got sentrygun %s event\n", __FUNCTION__, string.Get() );
			}
			OnObjectChanged();
		}
	}
}

CNavArea *CTFNavMesh::CreateArea() const
{
	return new CTFNavArea;
}

void CTFNavMesh::Update()
{
	CNavMesh::Update();
	if (!TheNavAreas.IsEmpty())
	{
		UpdateDebugDisplay();
		if (TheNextBots().GetNextBotCount() > 0)
		{
			if (!m_lastNPCCount)
			{
				m_recomputeTimer.Start( 2.0f );
			}

			if (m_recomputeTimer.HasStarted() && m_recomputeTimer.IsElapsed())
			{
				m_recomputeTimer.Invalidate();
				RecomputeInternalData();
			}

			if (TFGameRules()->State_Get() == GR_STATE_PREROUND)
			{
				if (unk10.IsElapsed())
					unk10.Start( 3.0f );
			}

			m_lastNPCCount = TheNextBots().GetNextBotCount();
		}
	}
}

bool CTFNavMesh::IsAuthoritative() const
{
	return true;
}

unsigned int CTFNavMesh::GetSubVersionNumber() const
{
	return 2;
}

void CTFNavMesh::SaveCustomData( CUtlBuffer &fileBuffer ) const
{
	
}

void CTFNavMesh::LoadCustomData( CUtlBuffer &fileBuffer, unsigned int subVersion )
{

}

void CTFNavMesh::OnServerActivate()
{
	CNavMesh::OnServerActivate();
	ResetMeshAttributes( true );

	m_sentryAreas.RemoveAll();
	unk5.RemoveAll();
	m_spawnAreasTeam1.RemoveAll();
	m_spawnAreasTeam2.RemoveAll();
	m_spawnExitsTeam1.RemoveAll();
	m_spawnExitsTeam2.RemoveAll();

	for (int i=0; i < MAX_CONTROL_POINTS; ++i)
		m_CPAreas[i].RemoveAll();
}

void CTFNavMesh::OnRoundRestart()
{
	CNavMesh::OnRoundRestart();
	ResetMeshAttributes( true );
	TheNextBots().OnRoundRestart();
	if (TFGameRules()->IsMannVsMachineMode())
		RecomputeInternalData();
}

unsigned int CTFNavMesh::GetGenerationTraceMask() const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}

void CTFNavMesh::PostCustomAnalysis()
{

}

void CTFNavMesh::BeginCustomAnalysis( bool bIncremental )
{

}

void CTFNavMesh::EndCustomAnalysis()
{

}

void CTFNavMesh::CollectBuiltObjects( CUtlVector<CBaseObject *> *objects, int teamNum )
{
	objects->RemoveAll();

	
	for (int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i)
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if (obj && ( teamNum == TEAM_ANY || obj->GetTeamNumber() == teamNum ))
			objects->AddToTail( obj );
	}
}

// this function makes 0 sense
void CTFNavMesh::CollectSpawnRoomThresholdAreas( CUtlVector<CTFNavArea*> *areas, int teamNum ) const
{
	const CUtlVector<CTFNavArea *> *spawnExits = nullptr;
	if (teamNum == TF_TEAM_RED)
	{
		spawnExits = &m_spawnExitsTeam1;
	}
	else
	{
		if (teamNum != TF_TEAM_BLUE)
			return;

		spawnExits = &m_spawnExitsTeam2;
	}

	if (spawnExits)
	{
		for (int i=0; i<spawnExits->Count(); ++i)
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
			float flMaxSize = 0.0f;
			CTFNavArea *candidate = nullptr;
			for (int dir = 0; dir < NUM_DIRECTIONS; ++dir)
			{
				for (int j=0; j<area->GetAdjacentCount( (NavDirType)dir ); ++j)
				{
					CTFNavArea *adj = static_cast<CTFNavArea *>( area->GetAdjacentArea( (NavDirType)dir, j ) );
					if (adj->HasTFAttributes( RED_SPAWN_ROOM|BLUE_SPAWN_ROOM|SPAWN_ROOM_EXIT ))
						continue;

					float flSize = adj->GetSizeY() * adj->GetSizeX();
					if (flSize > flMaxSize)
					{
						candidate = adj;
						flMaxSize = flSize;
						break;
					}
				}
			}

			if (candidate)
				areas->AddToTail( candidate );
		}
	}
}

bool CTFNavMesh::IsSentryGunHere( CTFNavArea *area ) const
{
	for (int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i)
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if (obj == nullptr || obj->GetType() != OBJ_SENTRYGUN)
			continue;

		obj->UpdateLastKnownArea();
		if (obj->GetLastKnownArea() == area)
			return true;
	}

	return false;
}

void CTFNavMesh::OnBlockedAreasChanged()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if (TheNextBots().GetNextBotCount() > 0)
		m_recomputeTimer.Start( 2.0f );
}

void CTFNavMesh::CollectAndMarkSpawnRoomExits( CTFNavArea *area, CUtlVector<CTFNavArea *> *areas )
{
	for (int dir=0; dir < NUM_DIRECTIONS; dir++)
	{
		for (int i=0; i<area->GetAdjacentCount( (NavDirType)dir ); ++i)
		{
			CTFNavArea *other = static_cast<CTFNavArea *>( area->GetAdjacentArea( (NavDirType)dir, i ) );
			if (other->HasTFAttributes( RED_SPAWN_ROOM|BLUE_SPAWN_ROOM) )
				continue;

			area->AddTFAttributes( SPAWN_ROOM_EXIT );
			areas->AddToTail( area );
		}
	}
}

void CTFNavMesh::CollectControlPointAreas()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	for (int i=0; i < MAX_CONTROL_POINTS; ++i)
		m_CPAreas[i].RemoveAll();

	if (!g_hControlPointMasters.IsEmpty())
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters[0].Get();
		if (pMaster)
		{
			for (int i=0; i < ITriggerAreaCaptureAutoList::AutoList().Count(); ++i)
			{
				CTriggerAreaCapture *pCaptureArea = static_cast<CTriggerAreaCapture *>( ITriggerAreaCaptureAutoList::AutoList()[i] );

				CTeamControlPoint *pPoint = pCaptureArea->GetControlPoint();
				if (pPoint)
				{
					Extent captureExtent;
					captureExtent.Init( pCaptureArea );

					captureExtent.lo -= 35.5;
					captureExtent.hi += 35.5;

					int iIndex = pPoint->GetPointIndex();

					CollectAreasOverlappingExtent( captureExtent, &m_CPAreas[iIndex] );

					if (m_CPAreas[iIndex].Count())
					{
						float fDistance = FLT_MAX;
						Vector vOrigin = pCaptureArea->WorldSpaceCenter();
						for (int j=0; j < m_CPAreas[iIndex].Count(); ++j)
						{
							Vector vCenter = m_CPAreas[iIndex][j]->GetCenter();
							if (fDistance > ( vCenter - vOrigin ).AsVector2D().LengthSqr())
							{
								fDistance = ( vCenter - vOrigin ).AsVector2D().LengthSqr();
								m_CPArea[iIndex] = m_CPAreas[iIndex][j];
							}
						}
					}
				}
			}
		}
	}
}

void CTFNavMesh::ComputeBlockedAreas()
{
	for (int i=0; i<TheNavAreas.Count(); ++i)
	{
		CNavArea *area = TheNavAreas[i];
		area->UnblockArea();
	}

	for (CBaseEntity *pBrush = gEntList.FirstEnt(); pBrush; pBrush = gEntList.FindEntityByClassname( pBrush, "func_brush" ))
	{
		if (static_cast<CFuncBrush *>( pBrush ) == nullptr)
			break;

		if (pBrush->IsSolid())
			TestAndBlockOverlappingAreas( pBrush );
	}
	
	for (CBaseEntity *pDoor = gEntList.FirstEnt(); pDoor; pDoor = gEntList.FindEntityByClassname( pDoor, "func_door*" ))
	{
		if (static_cast<CBaseDoor *>( pDoor ) == nullptr)
			break;

		Extent doorExent;
		doorExent.Init( pDoor );
		// this doesn't make sense because 219 should be CBaseToggle::m_toggle_state
		// bool v49 = ( DWORD( pDoor + 219 ) & 0xFFFFFFFD ) == 1; 

		int iBlockedTeam = 0;
		bool bNoFilter = false;

		for (CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity; pEntity = gEntList.FindEntityByClassname( pEntity, "trigger_multiple" ))
		{
			CBaseTrigger *pTrigger = static_cast<CBaseTrigger *>( pEntity );
			if (pTrigger == nullptr)
				break;

			Extent triggerExtent;
			triggerExtent.Init( pTrigger );
			if (doorExent.IsOverlapping( triggerExtent ) && !pTrigger->m_bDisabled)
			{
				CBaseFilter *pFilter = pTrigger->m_hFilter;
				if (pFilter && FClassnameIs( pFilter, "filter_activator_tfteam" ))
					iBlockedTeam = pFilter->GetTeamNumber();
				else
					bNoFilter = true;
			}
		}
		
		CUtlVector<CTFNavArea *> potentiallyBlockedAreas;
		CollectAreasOverlappingExtent( doorExent, &potentiallyBlockedAreas );

		int iNavTeam = TEAM_ANY;
		if (iBlockedTeam > 0)
			iNavTeam = (iBlockedTeam == 2) + 2;

		for (int i=0; i<potentiallyBlockedAreas.Count(); ++i)
		{
			CTFNavArea *area = potentiallyBlockedAreas[i];

			if (!area->HasTFAttributes( DOOR_NEVER_BLOCKS ))
				area->MarkAsBlocked( iNavTeam, pDoor );

			// v49 maybe
			if (( (CBaseToggle *)pDoor )->m_toggle_state == TS_AT_BOTTOM)
				continue;

			if (!area->HasTFAttributes( DOOR_ALWAYS_BLOCKS ) && !bNoFilter && iNavTeam > 0)
				continue;

			area->UnblockArea( iNavTeam );
		}
	}
}

void CTFNavMesh::ComputeIncursionDistances()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	for (int i=0; i<TheNavAreas.Count(); ++i)
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		area->SetIncursionDistance( TF_TEAM_RED, -1.0f );
		area->SetIncursionDistance( TF_TEAM_BLUE, -1.0f );
	}

	bool bFoundRedSpawn = false;
	bool bFoundBluSpawn = false;

	if (IFuncRespawnRoomAutoList::AutoList().Count() && ITFTeamSpawnAutoList::AutoList().Count())
	{
		for (int i=0; i<IFuncRespawnRoomAutoList::AutoList().Count(); ++i)
		{
			CFuncRespawnRoom *respawnRoom = static_cast<CFuncRespawnRoom *>( IFuncRespawnRoomAutoList::AutoList()[i] );
			if (respawnRoom->GetActive() && !respawnRoom->m_bDisabled)
			{
				for (int j=0; j<ITFTeamSpawnAutoList::AutoList().Count(); ++j)
				{
					CTFTeamSpawn *teamSpawn = static_cast<CTFTeamSpawn *>( ITFTeamSpawnAutoList::AutoList()[j] );
					if (/*(teamSpawn + 280)(teamspawn, 0) &&*/!teamSpawn->IsDisabled() && 
						 (teamSpawn->GetTeamNumber() != TF_TEAM_RED || !bFoundRedSpawn) &&
						 (teamSpawn->GetTeamNumber() != TF_TEAM_BLUE || !bFoundBluSpawn))
					{
						if (respawnRoom->PointIsWithin( teamSpawn->GetAbsOrigin() ))
						{
							CTFNavArea *area = static_cast<CTFNavArea *>( GetNearestNavArea( teamSpawn ) );
							if (area)
							{
								//ComputeIncursionDistances( area, teamSpawn->GetTeamNumber() );
								if (teamSpawn->GetTeamNumber() == TF_TEAM_RED)
									bFoundRedSpawn = true;
								else
									bFoundBluSpawn = true;
							}
						}
					}
				}
			}
		}

		if (!bFoundRedSpawn)
			goto WarnAndContinue;

		if (!bFoundBluSpawn)
		{
			Warning(
				"Can't compute incursion distances from the Blue spawn room(s). Bots will perform poorly. This is caused by either "
				"a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
		}
	}
	else
	{
WarnAndContinue:
		Warning(
			"Can't compute incursion distances from the Red spawn room(s). Bots will perform poorly. This is caused by either a"
			" missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}

	// not sure why they do this, let alone for a single team
	if (!TFGameRules()->IsPVEModeActive() && TheNavAreas.Count())
	{
		float flMaxDistance = 0.0f;
		for (int i=0; i<TheNavAreas.Count(); ++i)
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
			flMaxDistance = fmax( flMaxDistance, area->GetIncursionDistance( TF_TEAM_BLUE ) );
		}

		for (int i=0; i<TheNavAreas.Count(); ++i)
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
			float flIncursionDist = area->GetIncursionDistance( TF_TEAM_BLUE );
			if (flIncursionDist >= 0.0f)
				area->SetIncursionDistance( TF_TEAM_RED, ( flIncursionDist  - flMaxDistance ) );
		}
	}
}

void CTFNavMesh::ComputeIncursionDistances( CTFNavArea *startArea, int teamNum )
{
	if (startArea && teamNum <= 3)
	{
		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVectorFixedGrowable<const NavConnect *, 64u> adjCons;

		while (!CNavArea::IsOpenListEmpty())
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( CNavArea::PopOpenList() );

			adjCons.RemoveAll();

			if (!TFGameRules()->IsMannVsMachineMode() && !area->HasTFAttributes( RED_SETUP_GATE|BLUE_SETUP_GATE|SPAWN_ROOM_EXIT ) && area->IsBlocked( teamNum ))
				continue;

			for (int dir = 0; dir < NUM_DIRECTIONS; ++dir)
			{
				if (area->GetAdjacentCount( (NavDirType)dir ) > 0)
				{
					for (int i=0; i<area->GetAdjacentCount( (NavDirType)dir ); ++i)
					{
						const NavConnect *connection = &(*area->GetAdjacentAreas( (NavDirType)dir ))[i];
						adjCons.AddToTail( connection );
					}
				}
			}

			if (adjCons.Count() <= 0)
				continue;

			for (int i=0; i<adjCons.Count(); ++i)
			{
				CTFNavArea *adj = static_cast<CTFNavArea *>( adjCons[i]->area );
				if (area->ComputeAdjacentConnectionHeightChange( adj ) <= 45.0f)
				{
					float flIncursionDist = area->GetIncursionDistance( teamNum ) + adjCons[i]->length;
					if (adj->GetIncursionDistance( teamNum ) < 0.0f || flIncursionDist > adj->GetIncursionDistance( teamNum ))
					{
						adj->SetIncursionDistance( teamNum, flIncursionDist );
						adj->Mark();
						adj->SetParent( area );
						if (!adj->IsOpen())
							adj->AddToOpenList();
					}
				}
			}
		}
	}
}

void CTFNavMesh::ComputeInvasionAreas()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	for (int i=0; i<TheNavAreas.Count(); ++i)
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		if (area) area->ComputeInvasionAreaVectors();
	}
}

void CTFNavMesh::DecorateMesh()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_spawnAreasTeam1.RemoveAll();
	m_spawnAreasTeam2.RemoveAll();
	m_spawnExitsTeam1.RemoveAll();
	m_spawnExitsTeam2.RemoveAll();
	
	for (int i=0; i<IFuncRespawnRoomAutoList::AutoList().Count(); ++i)
	{
		CFuncRespawnRoom *respawnRoom = static_cast<CFuncRespawnRoom *>( IFuncRespawnRoomAutoList::AutoList()[i] );
		if (respawnRoom->GetActive() && !respawnRoom->m_bDisabled)
		{
			for (int j=0; j<ITFTeamSpawnAutoList::AutoList().Count(); ++j)
			{
				CTFTeamSpawn *teamSpawn = static_cast<CTFTeamSpawn *>( ITFTeamSpawnAutoList::AutoList()[j] );
				if (/*(teamSpawn + 280)(teamspawn, 0) teamspawn->HasDataObjectType( GROUNDLINK )? &&*/!teamSpawn->IsDisabled() && respawnRoom->PointIsWithin( teamSpawn->GetAbsOrigin() ))
				{
					Extent ext;
					ext.Init( respawnRoom );

					if (teamSpawn->GetTeamNumber() == TF_TEAM_RED)
					{
						CCollectAndLabelSpawnRooms func( respawnRoom, TF_TEAM_RED, &m_spawnAreasTeam1 );
						ForAllAreasOverlappingExtent( func, ext );
					}
					else
					{
						CCollectAndLabelSpawnRooms func( respawnRoom, TF_TEAM_BLUE, &m_spawnAreasTeam2 );
						ForAllAreasOverlappingExtent( func, ext );
					}
				}
			}
		}
	}

	for (int i=0; i<m_spawnAreasTeam1.Count(); ++i)
	{
		CollectAndMarkSpawnRoomExits( m_spawnAreasTeam1[i], &m_spawnExitsTeam1 );
	}

	for (int i=0; i<m_spawnAreasTeam2.Count(); ++i)
	{
		CollectAndMarkSpawnRoomExits( m_spawnAreasTeam2[i], &m_spawnExitsTeam2 );
	}

	for (CBaseEntity *pEnt = gEntList.FirstEnt(); pEnt; pEnt = gEntList.NextEnt( pEnt ))
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( GetNearestNavArea( pEnt->GetAbsOrigin() ) );
		if (area)
		{
			if (FClassnameIs( pEnt, "item_ammopack*" ))
				area->AddTFAttributes( AMMO );
			else if (FClassnameIs( pEnt, "item_healthkit*" ))
				area->AddTFAttributes( HEALTH );
		}
	}

	for (int i=0; i<MAX_CONTROL_POINTS; ++i)
	{
		for (int j=0; j<m_CPAreas[i].Count(); ++j)
		{
			m_CPAreas[i][j]->AddTFAttributes( CONTROL_POINT );
		}
	}
}

// Irrelevant MvM computations
/*void CTFNavMesh::ComputeBombTargetDistance()
{
	// Back traces from a capture zone entity to each area to figure out
	// the distance between it and the area as long as the area is reachable
}*/

/*void CTFNavMesh::ComputeLegalBombDropAreas()
{
	// Flood fills from an area it found in TheNavAreas with BLUE_SPAWN_ROOM flag
	// with BOMB_DROP flag if not BLUE_SPAWN_ROOM|RED_SPAWN_ROOM
}*/

void CTFNavMesh::OnObjectChanged()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	ResetMeshAttributes( false );

	CUtlVector<CBaseObject *> sentries;
	for (int i=0; i < IBaseObjectAutoList::AutoList().Count(); ++i)
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if (obj && obj->ObjectType() == OBJ_SENTRYGUN /*&& !DWORD(obj + 1404) && !BYTE(obj + 2584)*/)
		{
			sentries.AddToTail( obj );
		}
	}

	if (!sentries.IsEmpty())
	{
		for (int i=0; i < TheNavAreas.Count(); ++i)
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
			for (int j=0; j < sentries.Count(); ++j)
			{
				CBaseObject *obj = sentries[j];
				Vector sentryPos = obj->GetAbsOrigin();
				int team = obj->GetTeamNumber();

				Vector areaPos( 0.0f );
				area->GetClosestPointOnArea( sentryPos, &areaPos );

				if (areaPos.DistToSqr( sentryPos ) < Square( SENTRYGUN_BASE_RANGE ))
				{
					if (!area->HasAttributes( BLUE_SENTRY|RED_SENTRY ))
						m_sentryAreas.AddToTail( area );

					area->AddTFAttributes( (TFNavAttributeType)( 128 * ( team != TF_TEAM_BLUE ) + 128 ) );
				}
			}
		}
	}
}

// TODO: Why do they recompute so much so often?
void CTFNavMesh::RecomputeInternalData()
{
	RemoveAllMeshDecoration();
	DecorateMesh();
	CollectControlPointAreas();
	ComputeBlockedAreas();
	ComputeIncursionDistances();
	ComputeInvasionAreas();
	//ComputeLegalBombDropAreas();
	//ComputeBombTargetDistance();

	for (int i=0; i<TheNavAreas.Count(); ++i)
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );

		if (m_pointState <= CP_STATE_RESET)
		{
			if (area->HasTFAttributes( BLOCKED_UNTIL_POINT_CAPTURE ))
				area->AddTFAttributes( BLOCKED );
		}

		if (m_pointState == CP_STATE_OWNERSHIP_CHANGED)
		{
			if (area->HasTFAttributes( BLOCKED_UNTIL_POINT_CAPTURE ))
			{
				if (area->HasTFAttributes( WITH_SECOND_POINT ) && m_pointChangedIdx > 0)
					area->RemoveTFAttributes( BLOCKED );
				else if (area->HasTFAttributes( WITH_THIRD_POINT ) && m_pointChangedIdx > 1)
					area->RemoveTFAttributes( BLOCKED );
				else if (area->HasTFAttributes( WITH_FOURTH_POINT ) && m_pointChangedIdx > 2)
					area->RemoveTFAttributes( BLOCKED );
				else if (area->HasTFAttributes( WITH_FIFTH_POINT ) && m_pointChangedIdx > 3)
					area->RemoveTFAttributes( BLOCKED );
			}
			else if (area->HasTFAttributes( BLOCKED_AFTER_POINT_CAPTURE ))
			{
				if (area->HasTFAttributes( WITH_SECOND_POINT ) && m_pointChangedIdx > 0)
					area->AddTFAttributes( BLOCKED );
				else if (area->HasTFAttributes( WITH_THIRD_POINT ) && m_pointChangedIdx > 1)
					area->AddTFAttributes( BLOCKED );
				else if (area->HasTFAttributes( WITH_FOURTH_POINT ) && m_pointChangedIdx > 2)
					area->AddTFAttributes( BLOCKED );
				else if (area->HasTFAttributes( WITH_FIFTH_POINT ) && m_pointChangedIdx > 3)
					area->AddTFAttributes( BLOCKED );
			}
		}
	}

	m_recomputeTimer.Invalidate();
}

void CTFNavMesh::RemoveAllMeshDecoration()
{
	for (int i=0; i < TheNavAreas.Count(); ++i)
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		area->RemoveTFAttributes( TF_ATTRIBUTE_RESET );
	}

	m_sentryAreas.RemoveAll();
	OnObjectChanged();
}

void CTFNavMesh::ResetMeshAttributes( bool fullReset )
{
	for (int i=0; i<m_sentryAreas.Count(); ++i)
		m_sentryAreas[i]->RemoveTFAttributes( BLUE_SENTRY|RED_SENTRY );

	m_sentryAreas.RemoveAll();

	if (fullReset)
	{
		m_recomputeTimer.Start( 2.0f );
		m_pointState = 0;
		m_pointChangedIdx = 0;
	}
}

void CTFNavMesh::UpdateDebugDisplay() const
{
	if (!engine->IsDedicatedServer())
	{
		CBasePlayer *host = UTIL_GetListenServerHost();
		if (host && host->IsConnected())
		{
			for (int i=0; i<TheNavAreas.Count(); ++i)
			{
				CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
				Vector center = area->GetCenter();

				if (tf_show_in_combat_areas.GetBool() && area->IsInCombat())
				{
					float flIntensity = area->GetCombatIntensity();
					area->DrawFilled( flIntensity * 255, 0, 0, 255 );
				}

				if (tf_show_blocked_areas.GetBool())
				{
					CUtlString string;
					if (area->IsBlocked( TF_TEAM_RED ) && area->IsBlocked( TF_TEAM_BLUE ))
						string = "Blocked for All";
					else if (area->IsBlocked( TF_TEAM_RED ))
						string = "Blocked for Red";
					else if (area->IsBlocked( TF_TEAM_BLUE ))
						string = "Blocked for Blue";

					if (area == GetSelectedArea() && !string.IsEmpty())
						NDebugOverlay::Text( center, string, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}

				if (tf_show_mesh_decoration.GetBool())
				{
					if (area == GetSelectedArea())
					{
						if (area->HasTFAttributes( BLUE_SPAWN_ROOM ))
						{
							if (!area->HasTFAttributes( SPAWN_ROOM_EXIT ))
							{
								NDebugOverlay::Text( center, "Blue Spawn Room", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
								return;
							}

							NDebugOverlay::Text( center, "Blue Spawn Exit", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							return;
						}

						if (area->HasTFAttributes( RED_SPAWN_ROOM ))
						{
							if (!area->HasTFAttributes( SPAWN_ROOM_EXIT ))
							{
								NDebugOverlay::Text( center, "Red Spawn Room", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
								return;
							}

							NDebugOverlay::Text( center, "Red Spawn Exit", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							return;
						}

						if (area->HasTFAttributes( HEALTH ) || area->HasTFAttributes( AMMO ))
						{
							if (!area->HasTFAttributes( AMMO ))
							{
								NDebugOverlay::Text( center, "Health", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
								return;
							}

							if (!area->HasTFAttributes( HEALTH ))
							{
								NDebugOverlay::Text( center, "Ammo", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
								return;
							}

							NDebugOverlay::Text( center, "Health & Ammo", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							return;
						}

						if (area->HasTFAttributes( CONTROL_POINT ))
						{
							NDebugOverlay::Text( center, "Control Point", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							return;
						}
					}
				}

				// TODO
			}
		}
	}
}
