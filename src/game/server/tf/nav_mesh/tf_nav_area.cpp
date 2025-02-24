//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_nav_area.h
// TF specific nav area
// Michael Booth, February 2009

#include "cbase.h"
#include "tf_nav_mesh.h"
#include "tf_nav_area.h"
#include "tf_gamerules.h"
#include "bot/tf_bot.h"
#include "nav_pathfind.h"
#include "vscript_server.h"

ConVar tf_nav_show_incursion_distance( "tf_nav_show_incursion_distance", "0", FCVAR_CHEAT, "Display travel distances from current spawn room (1=red, 2=blue)" );
ConVar tf_nav_show_bomb_target_distance( "tf_nav_show_bomb_target_distance", "0", FCVAR_CHEAT, "Display travel distances to bomb target (MvM mode)" );
ConVar tf_nav_show_turf_ownership( "tf_nav_show_turf_ownership", "0", FCVAR_CHEAT, "Color nav area by smallest incursion distance" );

ConVar tf_nav_in_combat_duration( "tf_nav_in_combat_duration", "30", FCVAR_CHEAT, "How long after gunfire occurs is this area still considered to be 'in combat'" );

ConVar tf_nav_combat_build_rate( "tf_nav_combat_build_rate", "0.05", FCVAR_CHEAT, "Gunfire/second increase (combat caps at 1.0)" );
ConVar tf_nav_combat_decay_rate( "tf_nav_combat_decay_rate", "0.022", FCVAR_CHEAT, "Decay/second toward zero" );

ConVar tf_show_sniper_areas( "tf_show_sniper_areas", "0", FCVAR_CHEAT );
ConVar tf_show_sniper_areas_safety_range( "tf_show_sniper_areas_safety_range", "1000", FCVAR_CHEAT );

ConVar tf_show_incursion_range( "tf_show_incursion_range", "0", FCVAR_CHEAT, "1 = red, 2 = blue" );
ConVar tf_show_incursion_range_min( "tf_show_incursion_range_min", "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values" );
ConVar tf_show_incursion_range_max( "tf_show_incursion_range_max", "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values" );

//--------------------------------------------------------------------------------------------------------------
// Script access to manipulate the nav
//--------------------------------------------------------------------------------------------------------------

BEGIN_SCRIPTENUM( ETFNavAttributeType, "Team Fortress nav attributes" )
	DEFINE_ENUMCONST( TF_NAV_INVALID, "" )
	DEFINE_ENUMCONST( TF_NAV_BLOCKED, "" )
	DEFINE_ENUMCONST( TF_NAV_SPAWN_ROOM_RED, "" )
	DEFINE_ENUMCONST( TF_NAV_SPAWN_ROOM_BLUE, "" )
	DEFINE_ENUMCONST( TF_NAV_SPAWN_ROOM_EXIT, "" )
	DEFINE_ENUMCONST( TF_NAV_HAS_AMMO, "" )
	DEFINE_ENUMCONST( TF_NAV_HAS_HEALTH, "" )
	DEFINE_ENUMCONST( TF_NAV_CONTROL_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_BLUE_SENTRY_DANGER, "" )
	DEFINE_ENUMCONST( TF_NAV_RED_SENTRY_DANGER, "" )
	DEFINE_ENUMCONST( TF_NAV_BLUE_SETUP_GATE, "" )
	DEFINE_ENUMCONST( TF_NAV_RED_SETUP_GATE, "" )
	DEFINE_ENUMCONST( TF_NAV_BLOCKED_AFTER_POINT_CAPTURE, "" )
	DEFINE_ENUMCONST( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE, "" )
	DEFINE_ENUMCONST( TF_NAV_BLUE_ONE_WAY_DOOR, "" )
	DEFINE_ENUMCONST( TF_NAV_RED_ONE_WAY_DOOR, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_SECOND_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_THIRD_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_FOURTH_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_WITH_FIFTH_POINT, "" )
	DEFINE_ENUMCONST( TF_NAV_SNIPER_SPOT, "" )
	DEFINE_ENUMCONST( TF_NAV_SENTRY_SPOT, "" )
	DEFINE_ENUMCONST( TF_NAV_ESCAPE_ROUTE, "" )
	DEFINE_ENUMCONST( TF_NAV_ESCAPE_ROUTE_VISIBLE, "" )
	DEFINE_ENUMCONST( TF_NAV_NO_SPAWNING, "" )
	DEFINE_ENUMCONST( TF_NAV_RESCUE_CLOSET, "" )
	DEFINE_ENUMCONST( TF_NAV_BOMB_CAN_DROP_HERE, "" )
	DEFINE_ENUMCONST( TF_NAV_DOOR_NEVER_BLOCKS, "" )
	DEFINE_ENUMCONST( TF_NAV_DOOR_ALWAYS_BLOCKS, "" )
	DEFINE_ENUMCONST( TF_NAV_UNBLOCKABLE, "" )
	DEFINE_ENUMCONST( TF_NAV_PERSISTENT_ATTRIBUTES, "" )
END_SCRIPTENUM()


BEGIN_SCRIPTDESC( CTFNavArea, CNavArea, "TF navigation area" )
	DEFINE_SCRIPTFUNC( SetAttributeTF, "Set TF-specific area attributes" )
	DEFINE_SCRIPTFUNC( HasAttributeTF, "Has TF-specific area attribute bits" )
	DEFINE_SCRIPTFUNC( ClearAttributeTF, "Clear TF-specific area attribute bits" )

	DEFINE_SCRIPTFUNC( FindRandomSpot, "Get random origin within extent of area" )
	DEFINE_SCRIPTFUNC( IsPotentiallyVisibleToTeam, "( team ) - Return true if any portion of this area is visible to anyone on the given team" )
	DEFINE_SCRIPTFUNC( IsCompletelyVisibleToTeam, "( team ) - Return true if given area is completely visible from somewhere in this area by someone on the team" )
	DEFINE_SCRIPTFUNC( IsBottleneck, "Returns true if area is a bottleneck" )
	DEFINE_SCRIPTFUNC( IsValidForWanderingPopulation, "Returns true if area is valid for wandering population" )
	DEFINE_SCRIPTFUNC( GetTravelDistanceToBombTarget, "Gets the travel distance to the MvM bomb target" )
	DEFINE_SCRIPTFUNC( IsReachableByTeam,"Is this area reachable by the given team?" )

	DEFINE_SCRIPTFUNC( IsTFMarked, "Is this nav area marked with the current marking scope?" )
	DEFINE_SCRIPTFUNC( TFMark, "Mark this nav area with the current marking scope." )
END_SCRIPTDESC()

//--------------------------------------------------------------------------------------------------------
/**
 * Invoked when a door is created
 */
void CTFNavArea::OnDoorCreated( CBaseEntity *door )
{
	m_hDoor = door;
}


//--------------------------------------------------------------------------------------------------------
// return a door contained in this area
CBaseEntity *CTFNavArea::GetDoor( void ) const
{
	return m_hDoor;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Return a random spot in this area
 */
Vector CTFNavArea::FindRandomSpot( void ) const
{
	const float margin = 25.0f;
	Vector spot;
	
	if (GetSizeX() < 2.0f * margin || GetSizeY() < 2.0f * margin)
	{
		spot = GetCenter();
		spot.z += 10.0f;
	}
	else
	{
		spot.x = GetCorner( NORTH_WEST ).x + margin + RandomFloat( 0.0f, GetSizeX() - 2.0f * margin );
		spot.y = GetCorner( NORTH_WEST ).y + margin + RandomFloat( 0.0f, GetSizeY() - 2.0f * margin );
		spot.z = GetZ( spot.x, spot.y ) + 10.0f;
	}
	
	return spot;
}

//--------------------------------------------------------------------------------------------------------
/**
 * A bottleneck is a small nav area with connections on only two opposing sides (ie: a doorway)
 */
bool CTFNavArea::IsBottleneck( void ) const
{
	const float narrow = 2.1f * GenerationStepSize;

	if ( GetAdjacentCount( NORTH ) == 0 && GetAdjacentCount( SOUTH ) == 0 && 
		 GetAdjacentCount( EAST ) > 0 && GetAdjacentCount( WEST ) > 0 )
	{
		if ( GetSizeY() < narrow )
		{
			return true;
		}
	}
	else if ( GetAdjacentCount( NORTH ) > 0 && GetAdjacentCount( SOUTH ) > 0 && 
			 GetAdjacentCount( EAST ) == 0 && GetAdjacentCount( WEST ) == 0 )
	{
		if ( GetSizeX() < narrow )
		{
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------------------------
CTFNavArea::CTFNavArea( void )
{
	m_attributeFlags = 0;
	m_wanderCount = 0;
	m_combatIntensity = 0.0f;
	m_distanceToBombTarget = 0.0f;
	m_TFMark = 0;
	m_invasionSearchMarker = (unsigned int)-1;
	m_hScriptInstance = NULL;
}

CTFNavArea::~CTFNavArea( void )
{
	if ( g_pScriptVM && m_hScriptInstance )
	{
		g_pScriptVM->RemoveInstance( m_hScriptInstance );
		m_hScriptInstance = NULL;
	}
}

//------------------------------------------------------------------------------------------------
/**
 * (EXTEND) invoked when map is initially loaded
 */
void CTFNavArea::OnServerActivate( void )
{
	BaseClass::OnServerActivate();

	ClearAllPotentiallyVisibleActors();
}

//------------------------------------------------------------------------------------------------
/**
 * (EXTEND) invoked for each area when the round restarts
 */
void CTFNavArea::OnRoundRestart( void )
{
	BaseClass::OnRoundRestart();

	ClearAllPotentiallyVisibleActors();

	m_combatIntensity = 0.0f;
}

//------------------------------------------------------------------------------------------------
/**
 * For game-specific analysis
 */
void CTFNavArea::CustomAnalysis( bool isIncremental )
{

}


//------------------------------------------------------------------------------------------------
/**
 * Draw area for debugging & editing
 */
void CTFNavArea::Draw( void ) const
{
	CNavArea::Draw();

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() && m_wanderCount > 0 )
	{
		NDebugOverlay::Text( GetCenter(), UTIL_VarArgs( "%d", m_wanderCount ), false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}
#endif // TF_RAID_MODE

	if ( tf_nav_show_incursion_distance.GetBool() )
	{
		NDebugOverlay::Text( GetCenter(), UTIL_VarArgs( "R:%3.1f   B:%3.1f", GetIncursionDistance( TF_TEAM_RED ), GetIncursionDistance( TF_TEAM_BLUE ) ), false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}

	if ( tf_nav_show_bomb_target_distance.GetBool() )
	{
		NDebugOverlay::Text( GetCenter(), UTIL_VarArgs( "%3.1f", GetTravelDistanceToBombTarget() ), false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}

	if ( tf_show_sniper_areas.GetBool() )
	{
		bool redSniper = IsAwayFromInvasionAreas( TF_TEAM_RED, tf_show_sniper_areas_safety_range.GetFloat() );
		bool blueSniper = IsAwayFromInvasionAreas( TF_TEAM_BLUE, tf_show_sniper_areas_safety_range.GetFloat() );

		if ( blueSniper )
		{
			if ( redSniper )
			{
				// both teams like this spot?
				DrawFilled( 255, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
			else
			{
				// blue sniper area
				DrawFilled( 0, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}
		else if ( redSniper )
		{
			// red sniper area
			DrawFilled( 255, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
	}

	int rangeTeam = tf_show_incursion_range.GetInt();
	if ( rangeTeam > 0 )
	{
		rangeTeam += ( TF_TEAM_RED - 1);

		float range = GetIncursionDistance( rangeTeam );
		if ( range >= tf_show_incursion_range_min.GetFloat() && range <= tf_show_incursion_range_max.GetFloat() )
		{
			DrawFilled( 0, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
	}
}


//------------------------------------------------------------------------------------------------
/**
 * Return adjacent area with largest increase in incursion distance
 */
CTFNavArea *CTFNavArea::GetNextIncursionArea( int team ) const
{
	CTFNavArea *nextIncursionArea = NULL;
	float nextIncursionDistance = GetIncursionDistance( team );

	for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
	{
		const NavConnectVector *adjVector = GetAdjacentAreas( (NavDirType)dir );
		FOR_EACH_VEC( (*adjVector), bit )
		{
			CTFNavArea *adjArea = static_cast< CTFNavArea * >( (*adjVector)[ bit ].area );

			if ( adjArea->GetIncursionDistance( team ) > nextIncursionDistance )
			{
				nextIncursionArea = adjArea;
				nextIncursionDistance = adjArea->GetIncursionDistance( team );
			}
		}
	}

	return nextIncursionArea;
}


//-----------------------------------------------------------------------------
// Populate 'priorVector' with a collection of adjacent areas that have a lower incursion distance that this area
void CTFNavArea::CollectPriorIncursionAreas( int team, CUtlVector< CTFNavArea * > *priorVector )
{
	float myIncursionDistance = GetIncursionDistance( team );
	
	priorVector->RemoveAll();

	for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
	{
		const NavConnectVector *adjVector = GetAdjacentAreas( (NavDirType)dir );
		FOR_EACH_VEC( (*adjVector), bit )
		{
			CTFNavArea *adjArea = static_cast< CTFNavArea * >( (*adjVector)[ bit ].area );

			if ( adjArea->GetIncursionDistance( team ) < myIncursionDistance )
			{
				priorVector->AddToTail( adjArea );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Populate 'priorVector' with a collection of adjacent areas that have a higher incursion distance that this area
void CTFNavArea::CollectNextIncursionAreas( int team, CUtlVector< CTFNavArea * > *priorVector )
{
	float myIncursionDistance = GetIncursionDistance( team );

	priorVector->RemoveAll();

	for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
	{
		const NavConnectVector *adjVector = GetAdjacentAreas( (NavDirType)dir );
		FOR_EACH_VEC( (*adjVector), bit )
		{
			CTFNavArea *adjArea = static_cast< CTFNavArea * >( (*adjVector)[ bit ].area );

			if ( adjArea->GetIncursionDistance( team ) > myIncursionDistance )
			{
				priorVector->AddToTail( adjArea );
			}
		}
	}
}


//-----------------------------------------------------------------------------
/**
 * Return true if this area is at least safetyRange units away from all invasion areas
 */
bool CTFNavArea::IsAwayFromInvasionAreas( int myTeam, float safetyRange ) const
{
	const CUtlVector< CTFNavArea * > &invasionVector = GetEnemyInvasionAreaVector( myTeam );
	FOR_EACH_VEC( invasionVector, vit )
	{
		CTFNavArea *invasionArea = invasionVector[ vit ];

		if ( ( invasionArea->GetCenter() - GetCenter() ).IsLengthLessThan( safetyRange ) )
		{
			// too close to incoming enemy route to snipe
			return false;
		}			
	}

	return true;
}


//-----------------------------------------------------------------------------
class MarkVisibleSet
{
public:
	MarkVisibleSet( unsigned int marker )
	{
		m_marker = marker;
	}

	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( baseArea );
		area->SetInvasionSearchMarker( m_marker );
		return true;
	}

	unsigned int m_marker;
};

		
//-----------------------------------------------------------------------------
class CollectInvasionAreas
{
public:
	CollectInvasionAreas( unsigned int marker, CTFNavArea *homeArea, CUtlVector< CTFNavArea * > *redInvasionAreaVector, CUtlVector< CTFNavArea * > *blueInvasionAreaVector )
	{
		m_homeArea = homeArea;
		m_visibleMarker = marker;
		m_redInvasionAreaVector = redInvasionAreaVector;
		m_blueInvasionAreaVector = blueInvasionAreaVector;
	}

	void FilterArea( CTFNavArea *area, CTFNavArea *adjArea )
	{
		if ( adjArea->IsInvasionSearchMarked( m_visibleMarker ) )
		{
			// also in PVS - can't be invasion area
			return;
		}

		const float behindTolerance = 100.0;

		// adjacent area is not in PVS, test if adjacent area not penetrated as far, if so it is an invasion area
		if ( area->GetIncursionDistance( TF_TEAM_BLUE ) > adjArea->GetIncursionDistance( TF_TEAM_BLUE ) )
		{
			if ( area->GetIncursionDistance( TF_TEAM_BLUE ) > m_homeArea->GetIncursionDistance( TF_TEAM_BLUE ) + behindTolerance )
			{
				// this area is farther "in" than we are - don't search further
				return;
			}

			m_redInvasionAreaVector->AddToTail( adjArea );
		}

		if ( area->GetIncursionDistance( TF_TEAM_RED ) > adjArea->GetIncursionDistance( TF_TEAM_RED ) )
		{
			if ( area->GetIncursionDistance( TF_TEAM_RED ) > m_homeArea->GetIncursionDistance( TF_TEAM_RED ) + behindTolerance )
			{
				// this area is farther "in" than we are - don't search further
				return;
			}

			m_blueInvasionAreaVector->AddToTail( adjArea );
		}
	}

	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( baseArea );

		// explore adjacent floor areas
		int dir;
		for( dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			int count = area->GetAdjacentCount( (NavDirType)dir );
			for( int i=0; i<count; ++i )
			{
				CTFNavArea *adjArea = static_cast< CTFNavArea * >( area->GetAdjacentArea( (NavDirType)dir, i ) );

				FilterArea( area, adjArea );
			}
		}

		// include areas that connect TO this area via a one-way link, since the enemy is coming TO us
		for( dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			const NavConnectVector *list = area->GetIncomingConnections( (NavDirType)dir );

			FOR_EACH_VEC( (*list), it )
			{
				NavConnect connect = (*list)[ it ];				

				FilterArea( area, static_cast< CTFNavArea * >( connect.area ) );
			}
		}

		return true;
	}

	CTFNavArea *m_homeArea;
	CUtlVector< CTFNavArea * > *m_redInvasionAreaVector;
	CUtlVector< CTFNavArea * > *m_blueInvasionAreaVector;
	unsigned int m_visibleMarker;
};


//------------------------------------------------------------------------------------------------
/**
 * Find invasion areas where enemies enter from
 */
void CTFNavArea::ComputeInvasionAreaVectors( void )
{
	static unsigned int searchMarker = RandomInt( 0, 1024*1024 );

	for( int i=0; i<TF_TEAM_COUNT; ++i )
	{
		m_invasionAreaVector[ i ].RemoveAll();
	}

	++searchMarker;

	// mark all potentially visible areas for quick testing during the search
	MarkVisibleSet marker( searchMarker );
	ForAllCompletelyVisibleAreas( marker );

	// search boundary of potentially visible area set for area pairs where
	// the area in the PVS has a higher incursion distance than an adjacent
	// area outside of the PVS - an invasion area

	CollectInvasionAreas collector( searchMarker, this, &m_invasionAreaVector[ TF_TEAM_RED ], &m_invasionAreaVector[ TF_TEAM_BLUE ] );
	ForAllCompletelyVisibleAreas( collector );
}


//------------------------------------------------------------------------------------------------
bool CTFNavArea::IsBlocked( int teamID, bool ignoreNavBlockers ) const
{
	if ( HasAttributeTF( TF_NAV_UNBLOCKABLE ) )
		return false;

	if ( HasAttributeTF( TF_NAV_BLOCKED ) )
		return true;

	// temporary fix:
	if ( teamID == TF_TEAM_RED && HasAttributeTF( TF_NAV_BLUE_ONE_WAY_DOOR ) )
		return true;

	if ( teamID == TF_TEAM_BLUE && HasAttributeTF( TF_NAV_RED_ONE_WAY_DOOR ) )
		return true;

	return CNavArea::IsBlocked( teamID, ignoreNavBlockers );
}


//------------------------------------------------------------------------------------------------
void CTFNavArea::Save( CUtlBuffer &fileBuffer, unsigned int version ) const
{
	CNavArea::Save( fileBuffer, version );

	// save attribute flags
	unsigned int attributes = m_attributeFlags & TF_NAV_PERSISTENT_ATTRIBUTES;
	fileBuffer.PutUnsignedInt( attributes );
}


//------------------------------------------------------------------------------------------------
NavErrorType CTFNavArea::Load( CUtlBuffer &fileBuffer, unsigned int version, unsigned int subVersion )
{
	// load base class data
	CNavArea::Load( fileBuffer, version, subVersion );

	if ( subVersion > TheNavMesh->GetSubVersionNumber() )
	{
		Warning( "Unknown NavArea sub-version number\n" );
		return NAV_INVALID_FILE;
	}
	else if ( subVersion <= 1 )
	{
		// no data
		m_attributeFlags = 0;
		return NAV_OK;
	}

	m_attributeFlags = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() )
	{
		Warning( "Can't read TF-specific attributes\n" );
		return NAV_INVALID_FILE;
	}

	return NAV_OK;
}


//--------------------------------------------------------------------------------------------------------
unsigned int CTFNavArea::m_masterTFMark = 1;


//--------------------------------------------------------------------------------------------------------
void CTFNavArea::MakeNewTFMarker( void )
{
	++m_masterTFMark;
}


//--------------------------------------------------------------------------------------------------------
void CTFNavArea::ResetTFMarker( void )
{
	m_masterTFMark = 1;
}


//--------------------------------------------------------------------------------------------------------
bool CTFNavArea::IsTFMarked( void ) const
{
	return ( m_TFMark == m_masterTFMark );
}


//--------------------------------------------------------------------------------------------------------
void CTFNavArea::TFMark( void )
{
	m_TFMark = m_masterTFMark;
}


//--------------------------------------------------------------------------------------------------------
bool CTFNavArea::IsValidForWanderingPopulation( void ) const
{
	if ( HasAttributeTF( TF_NAV_BLOCKED | TF_NAV_SPAWN_ROOM_RED | TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_NO_SPAWNING | TF_NAV_RESCUE_CLOSET ) )
		return false;
		
	return true;
}


//--------------------------------------------------------------------------------------------------------
void CTFNavArea::AddPotentiallyVisibleActor( CBaseCombatCharacter *who )
{
	if ( who == NULL )
	{
		return;
	}

	int team = who->GetTeamNumber();
	if ( team < 0 || team >= TF_TEAM_COUNT )
		return;

	CTFBot *bot = ToTFBot( who );
	if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
		return;

	if ( m_potentiallyVisibleActor[ team ].Find( who ) == m_potentiallyVisibleActor[ team ].InvalidIndex() )
	{
		m_potentiallyVisibleActor[ team ].AddToTail( who );
	}
}



//--------------------------------------------------------------------------------------------------------
float CTFNavArea::GetCombatIntensity( void ) const
{
	if ( !m_combatTimer.HasStarted() )
	{
		return 0.0f;
	}

	float actualIntensity = m_combatIntensity - m_combatTimer.GetElapsedTime() * tf_nav_combat_decay_rate.GetFloat();

	if ( actualIntensity < 0.0f )
	{
		actualIntensity = 0.0f;
	}

	return actualIntensity;
}


//--------------------------------------------------------------------------------------------------------
// Invoked when combat happens in/near this area
void CTFNavArea::OnCombat( void )
{
	m_combatIntensity += tf_nav_combat_build_rate.GetFloat();
	if ( m_combatIntensity > 1.0f )
	{
		m_combatIntensity = 1.0f;
	}

	m_combatTimer.Start();
}


//--------------------------------------------------------------------------------------------------------
bool CTFNavArea::IsInCombat( void ) const
{
	return GetCombatIntensity() > 0.01f;
}


