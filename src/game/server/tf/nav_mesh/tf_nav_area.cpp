
#include "cbase.h"

#include "nav_mesh.h"
#include "nav_colors.h"
#include "fmtstr.h"
#include "props_shared.h"

#include "functorutils.h"
#include "team.h"
#include "nav_entities.h"

#include "tf_nav_area.h"

#include "tf_bot.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_nav_in_combat_duration( "tf_nav_in_combat_duration", "30", FCVAR_CHEAT, "How long after gunfire occurs is this area still considered to be 'in combat'" );
ConVar tf_nav_combat_build_rate( "tf_nav_combat_build_rate", "0.05", FCVAR_CHEAT, "Gunfire/second increase (combat caps at 1.0)" );
ConVar tf_nav_combat_decay_rate( "tf_nav_combat_decay_rate", "0.022", FCVAR_CHEAT, "Decay/second toward zero" );

ConVar tf_nav_show_incursion_distances( "tf_nav_show_incursion_distance", "0", FCVAR_CHEAT, "Display travel distances from current spawn room (1=red, 2=blue)" );
ConVar tf_nav_show_turf_ownership( "tf_nav_show_turf_ownership", "0", FCVAR_CHEAT, "Color nav area by smallest incursion distance" );
ConVar tf_show_incursion_range( "tf_show_incursion_range", "0", FCVAR_CHEAT, "1 = red, 2 = blue" );
ConVar tf_show_incursion_range_min( "tf_show_incursion_range_min", "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values" );
ConVar tf_show_incursion_range_max( "tf_show_incursion_range_max", "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values" );
ConVar tf_show_sniper_areas( "tf_show_sniper_areas", "0", FCVAR_CHEAT );
ConVar tf_show_sniper_areas_safety_range( "tf_show_sniper_areas_safety_range", "1000", FCVAR_CHEAT );

int CTFNavArea::m_masterTFMark = 1;


class CollectInvasionAreas
{
public:
	CollectInvasionAreas( CTFNavArea *startArea, CUtlVector<CTFNavArea *> *redAreas, CUtlVector<CTFNavArea *> *blueAreas, int marker )
		: m_redAreas( redAreas ), m_blueAreas( blueAreas ), m_pArea( startArea )
	{
		m_iMarker = marker;
	}

	bool operator()( CNavArea *a )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( a );
		for ( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			for ( int i=0; i<area->GetAdjacentAreas( (NavDirType)dir )->Count(); ++i )
			{
				CTFNavArea *other = static_cast<CTFNavArea *>( ( *area->GetAdjacentAreas( (NavDirType)dir ) )[i].area );
				if ( other->m_TFSearchMarker == m_iMarker )
					continue;

				if ( area->GetIncursionDistance( TF_TEAM_BLUE ) <= other->GetIncursionDistance( TF_TEAM_BLUE ) ||
					 area->GetIncursionDistance( TF_TEAM_BLUE ) > m_pArea->GetIncursionDistance( TF_TEAM_BLUE ) + 100.0f )
					continue;

				m_redAreas->AddToTail( other );

				if ( area->GetIncursionDistance( TF_TEAM_RED ) <= other->GetIncursionDistance( TF_TEAM_RED ) ||
					 area->GetIncursionDistance( TF_TEAM_RED ) > m_pArea->GetIncursionDistance( TF_TEAM_RED ) + 100.0f )
					continue;

				m_blueAreas->AddToTail( other );
			}

			for ( int i=0; i<area->GetIncomingConnections( (NavDirType)dir )->Count(); ++i )
			{
				CTFNavArea *other = static_cast<CTFNavArea *>( ( *area->GetIncomingConnections( (NavDirType)dir ) )[i].area );
				if ( other->m_TFSearchMarker == m_iMarker )
					continue;

				if ( area->GetIncursionDistance( TF_TEAM_BLUE ) <= other->GetIncursionDistance( TF_TEAM_BLUE ) ||
					 area->GetIncursionDistance( TF_TEAM_BLUE ) > m_pArea->GetIncursionDistance( TF_TEAM_BLUE ) + 100.0f )
					continue;

				m_redAreas->AddToTail( other );

				if ( area->GetIncursionDistance( TF_TEAM_RED ) <= other->GetIncursionDistance( TF_TEAM_RED ) ||
					 area->GetIncursionDistance( TF_TEAM_RED ) > m_pArea->GetIncursionDistance( TF_TEAM_RED ) + 100.0f )
					continue;

				m_blueAreas->AddToTail( other );
			}
		}

		return true;
	}

private:
	CTFNavArea *const m_pArea;
	CUtlVector<CTFNavArea *> *m_redAreas;
	CUtlVector<CTFNavArea *> *m_blueAreas;
	int m_iMarker;
};


CTFNavArea::CTFNavArea()
{
	Q_memset( &m_aIncursionDistances, 0, sizeof( m_aIncursionDistances ) );
	m_flBombTargetDistance = -1.0f;
}

CTFNavArea::~CTFNavArea()
{
	for ( int i = 0; i < 4; i++ )
		m_InvasionAreas[i].Purge();
}

void CTFNavArea::OnServerActivate()
{
	CNavArea::OnServerActivate();

	for ( int i = 0; i < 4; i++ )
		m_InvasionAreas[i].RemoveAll();

	m_fCombatIntensity = 0;
}

void CTFNavArea::OnRoundRestart()
{
	CNavArea::OnRoundRestart();

	m_fCombatIntensity = 0;
}

void CTFNavArea::Save( CUtlBuffer &fileBuffer, unsigned int version ) const
{
	CNavArea::Save( fileBuffer, version );
	fileBuffer.PutUnsignedInt( m_nAttributes );
}

NavErrorType CTFNavArea::Load( CUtlBuffer &fileBuffer, unsigned int version, unsigned int subVersion )
{
	if ( subVersion > TheNavMesh->GetSubVersionNumber() )
	{
		Warning( "Unknown NavArea sub-version number\n" );
		return NAV_INVALID_FILE;
	}
	else
	{
		CNavArea::Load( fileBuffer, version, subVersion );
		if ( subVersion <= 1 )
		{
			m_nAttributes = 0;
			return NAV_OK;
		}
		else
		{
			m_nAttributes = fileBuffer.GetUnsignedInt();
			if ( !fileBuffer.IsValid() )
			{
				Warning( "Can't read TF-specific attributes\n" );
				return NAV_INVALID_FILE;
			}
		}
	}

	return NAV_OK;
}

void CTFNavArea::UpdateBlocked( bool force, int teamID )
{
	//CNavArea::UpdateBlocked( force, teamID );
}

bool CTFNavArea::IsBlocked( int teamID, bool ignoreNavBlockers ) const
{
	if ( !( m_nAttributes & UNBLOCKABLE ) )
	{
		if ( !( m_nAttributes & BLOCKED ) )
		{
			if ( teamID != TF_TEAM_RED )
			{
				if ( teamID == TF_TEAM_BLUE && ( m_nAttributes & RED_ONE_WAY_DOOR ) )
					return true;

				return CNavArea::IsBlocked( teamID, ignoreNavBlockers );
			}

			if ( !( m_nAttributes & BLUE_ONE_WAY_DOOR ) )
				return CNavArea::IsBlocked( teamID, ignoreNavBlockers );
		}

		return true;
	}

	return false;
}

void CTFNavArea::Draw() const
{
	CNavArea::Draw();

	if ( tf_nav_show_incursion_distances.GetBool() )
	{
		NDebugOverlay::Text( GetCenter(),
							 UTIL_VarArgs( "R:%3.1f   B:%3.1f", m_aIncursionDistances[TF_TEAM_RED], m_aIncursionDistances[TF_TEAM_BLUE] ),
							 true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}

	if ( tf_nav_show_turf_ownership.GetBool() )
	{
		float flRadius = 500.0f; //tf_nav_show_turf_ownership_range.GetFloat(); // this is some development-only cvar that I can't find
		bool bRedOwnsMe = IsAwayFromInvasionAreas( TF_TEAM_RED, flRadius );
		bool bBluOwnsMe = IsAwayFromInvasionAreas( TF_TEAM_BLUE, flRadius );

		if ( bBluOwnsMe )
		{
			if ( bRedOwnsMe )
			{
				DrawFilled( 255, 0, 255, 255 );
			}
			else
			{
				DrawFilled( 0, 0, 255, 255 );
			}
		}
		else
		{
			DrawFilled( 255, 0, 0, 255 );
		}
	}

	if ( tf_show_incursion_range.GetInt() )
	{
		float flIncursion = -1.0f;
		if ( tf_show_incursion_range.GetInt() <= 3 )
		{
			flIncursion = GetIncursionDistance( tf_show_incursion_range.GetInt() );
			if ( flIncursion < tf_show_incursion_range_min.GetFloat() )
				return;
		}
		else
		{
			if ( tf_show_incursion_range_min.GetFloat() > -1.0f )
				return;
		}

		if ( flIncursion <= tf_show_incursion_range_max.GetFloat() )
			DrawFilled( 0, 255, 0, 255 );
	}

	extern ConVar tf_show_blocked_areas;
	if ( tf_show_blocked_areas.GetBool() )
	{
		if ( HasTFAttributes( BLOCKED ) )
			DrawFilled( 255, 0, 0, 255 );

		if ( IsBlocked( TF_TEAM_RED ) && IsBlocked( TF_TEAM_BLUE ) )
			DrawFilled( 100, 0, 100, 255 );
		else if ( IsBlocked( TF_TEAM_RED ) )
			DrawFilled( 100, 0, 0, 255 );
		else if ( IsBlocked( TF_TEAM_BLUE ) )
			DrawFilled( 0, 0, 100, 255 );
	}

	// moved here from CTFNavMesh::UpdateDebugDisplay
	extern ConVar tf_show_mesh_decoration;
	if ( tf_show_mesh_decoration.GetBool() )
	{
		if ( HasAttributes( NO_SPAWNING ) )
		{
			DrawFilled( 100, 100, 0, 255 );
			return;
		}

		if ( HasTFAttributes( RESCUE_CLOSET ) )
		{
			DrawFilled( 0, 255, 255, 255 );
			return;
		}

		if ( HasTFAttributes( BLUE_SPAWN_ROOM ) )
		{
			if ( HasTFAttributes( SPAWN_ROOM_EXIT ) )
			{
				DrawFilled( 100, 100, 255, 255 );
				return;
			}

			DrawFilled( 0, 0, 100, 255 );
			return;
		}

		if ( HasTFAttributes( RED_SPAWN_ROOM ) )
		{
			if ( HasTFAttributes( SPAWN_ROOM_EXIT ) )
			{
				DrawFilled( 255, 100, 100, 255 );
				return;
			}

			DrawFilled( 100, 0, 0, 255 );
			return;
		}

		if ( HasTFAttributes( HEALTH ) || HasTFAttributes( AMMO ) )
		{
			if ( !HasTFAttributes( AMMO ) )
			{
				DrawFilled( 255, 150, 150, 255 );
				return;
			}

			if ( !HasTFAttributes( HEALTH ) )
			{
				DrawFilled( 100, 100, 100, 255 );
				return;
			}

			DrawFilled( 255, 0, 255, 255 );
			return;
		}

		if ( HasTFAttributes( CONTROL_POINT ) )
		{
			DrawFilled( 0, 255, 0, 255 );
			return;
		}

		if ( HasTFAttributes( DOOR_NEVER_BLOCKS ) )
		{
			DrawFilled( 0, 100, 0, 255 );
			return;
		}

		if ( HasTFAttributes( DOOR_ALWAYS_BLOCKS ) )
		{
			DrawFilled( 100, 0, 100, 255 );
			return;
		}

		if ( HasTFAttributes( UNBLOCKABLE ) )
		{
			DrawFilled( 0, 200, 100, 255 );
			return;
		}

		if ( HasTFAttributes( SNIPER_SPOT ) )
		{
			DrawFilled( 255, 255, 0, 255 );
			return;
		}

		if ( HasTFAttributes( SENTRY_SPOT ) )
		{
			DrawFilled( 255, 100, 0, 255 );
			return;
		}

		if ( HasTFAttributes( BLOCKED_UNTIL_POINT_CAPTURE ) )
		{
			DrawFilled( 0, 255, 255, 255 );
			return;
		}

		if ( HasTFAttributes( BLOCKED_AFTER_POINT_CAPTURE ) )
		{
			DrawFilled( 255, 255, 0, 255 );
			return;
		}
	}
}

void CTFNavArea::CustomAnalysis( bool isIncremental )
{
	;
}

void CTFNavArea::CollectNextIncursionAreas( int teamNum, CUtlVector<CTFNavArea *> *areas )
{
	areas->RemoveAll();
	// TODO
}

void CTFNavArea::CollectPriorIncursionAreas( int teamNum, CUtlVector<CTFNavArea *> *areas )
{
	areas->RemoveAll();
	// TODO
}

CTFNavArea *CTFNavArea::GetNextIncursionArea( int teamNum ) const
{
	CTFNavArea *result = NULL;

	float incursionDist = GetIncursionDistance( teamNum );

	for ( int i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < m_InvasionAreas[i].Count(); j++ )
		{
			float otherIncursionDist = m_InvasionAreas[i][j]->GetIncursionDistance( teamNum );

			if ( otherIncursionDist > incursionDist )
			{
				incursionDist = fmaxf( incursionDist, otherIncursionDist );
				result = m_InvasionAreas[i].Element( j );
			}
		}
	}
	
	return result;
}

void CTFNavArea::ComputeInvasionAreaVectors()
{
	for ( int i=0; i<4; ++i )
		m_InvasionAreas[i].RemoveAll();

	static int searchMarker = RandomInt( 0, Square( 1024 ) );
	searchMarker++;

	auto MarkVisibleSet = [ = ]( CNavArea *a ) {
		CTFNavArea *area = static_cast<CTFNavArea *>( a );
		area->m_TFSearchMarker = searchMarker;

		return true;
	};
	ForAllCompletelyVisibleAreas( MarkVisibleSet );

	CollectInvasionAreas functor( this, &m_InvasionAreas[TF_TEAM_RED], &m_InvasionAreas[TF_TEAM_BLUE], searchMarker );
	ForAllCompletelyVisibleAreas( functor );
}

bool CTFNavArea::IsAwayFromInvasionAreas( int teamNum, float radius ) const
{
	Assert( teamNum >= 0 && teamNum < 4 );
	if ( teamNum < 4 )
	{
		const CUtlVector<CTFNavArea *> &invasionAreas = m_InvasionAreas[teamNum];
		for ( int i=0; i<invasionAreas.Count(); ++i )
		{
			CTFNavArea *area = invasionAreas[i];
			if ( Square( radius ) > ( m_center - area->GetCenter() ).LengthSqr() )
				return false;
		}
	}

	return true;
}

void CTFNavArea::AddPotentiallyVisibleActor( CBaseCombatCharacter *actor )
{
	int team;
	if (!actor || ( team = actor->GetTeamNumber() ) > 3)
		return;

	if ( ToTFBot( actor ) )
		return;

	for ( int i=0; i<m_PVNPCs[team].Count(); ++i )
	{
		CBaseCombatCharacter *npc = m_PVNPCs[team][i];
		if ( actor == npc )
			return;
	}

	m_PVNPCs[team].AddToTail( actor );
}

float CTFNavArea::GetCombatIntensity() const
{
	float intensity = 0.0f;
	if ( m_combatTimer.HasStarted() )
	{
		const float combatTime = m_combatTimer.GetElapsedTime();
		intensity = fmax( m_fCombatIntensity - ( combatTime * tf_nav_combat_decay_rate.GetFloat() ), 0.0f );
	}
	return intensity;
}

bool CTFNavArea::IsInCombat() const
{
	return GetCombatIntensity() > 0.01f;
}

void CTFNavArea::OnCombat()
{
	m_combatTimer.Reset();
	m_fCombatIntensity = fmin( m_fCombatIntensity + tf_nav_combat_build_rate.GetFloat(), 1.0f );
}
