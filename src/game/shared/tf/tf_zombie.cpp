//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_zombie.h"
#include "props_shared.h"
#include "particle_parse.h"

#if defined( CLIENT_DLL )

#else
#include "Path/NextBotPath.h"
#include "NextBotGroundLocomotion.h"
#include "player_vs_environment/zombie_behavior.h"
#include "player_vs_environment/headless_hatman.h"

ConVar tf_max_active_zombies( "tf_max_active_zombies", "30", FCVAR_CHEAT );

IMPLEMENT_AUTO_LIST( IZombieAutoList );

IMPLEMENT_INTENTION_INTERFACE( CZombie, CZombieBehavior )

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( Zombie, DT_TFZombie )
BEGIN_NETWORK_TABLE( CZombie, DT_TFZombie )
#if defined( CLIENT_DLL )
	RecvPropFloat( RECVINFO( m_flHeadScale ) ),
#else
	SendPropFloat( SENDINFO( m_flHeadScale ) ),
#endif
END_NETWORK_TABLE();

#if defined( GAME_DLL )
static char const *const s_skeletonHatModels[] ={
	"models/player/items/all_class/skull_scout.mdl",
	"models/workshop/player/items/scout/hw2013_boston_bandy_mask/hw2013_boston_bandy_mask.mdl",
	"models/workshop/player/items/demo/hw2013_blackguards_bicorn/hw2013_blackguards_bicorn.mdl",
	"models/player/items/heavy/heavy_big_chief.mdl",
};

class CZombieLocomotion : public NextBotGroundLocomotion
{
	DECLARE_CLASS( CZombieLocomotion, NextBotGroundLocomotion );
public:
	CZombieLocomotion(INextBot *bot)
		: BaseClass( bot ) {}

	virtual float GetMaxJumpHeight( void ) const OVERRIDE { return 18.0f; }
	virtual float GetStepHeight( void ) const OVERRIDE { return 18.0f; }
	virtual float GetMaxYawRate( void ) const OVERRIDE { return 200.0f; }
	virtual float GetRunSpeed( void ) const OVERRIDE { return 300.0f; }
	virtual bool ShouldCollideWith( CBaseEntity const *other ) const OVERRIDE { return false; }
};

float CZombiePathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
{
	if ( fromArea == nullptr )
	{
		// first area in path; zero cost
		return 0.0f;
	}

	if ( !m_Actor->GetLocomotionInterface()->IsAreaTraversable( area ) )
	{
		// dead end
		return -1.0f;
	}

	if ( ladder != nullptr )
		length = ladder->m_length;
	else if ( length <= 0.0f )
		length = ( area->GetCenter() - fromArea->GetCenter() ).Length();

	const float dz = fromArea->ComputeAdjacentConnectionHeightChange( area );
	if ( dz >= m_Actor->GetLocomotionInterface()->GetStepHeight() )
	{
		if ( dz >= m_Actor->GetLocomotionInterface()->GetMaxJumpHeight() )
			return -1.0f;

		// ?we won't actually get here according to the locomotor?
		length *= 5;
	}
	else
	{
		if ( dz < -m_Actor->GetLocomotionInterface()->GetDeathDropHeight() )
			return -1.0f;
	}

	// Consistently random cost modifier based on our last enemy
	int seed = ( 0.1 * gpGlobals->curtime ) + 1;
	float cosine = 0.0f;
	if ( m_Actor->GetLastAttacker() )
	{
		seed *= m_Actor->GetLastAttacker()->entindex() * area->GetID();
		cosine = ( seed >> 16 ) * VALVE_RAND_MAX + seed;
	}
	// rather large modifier to persuade them to move towards the last attacker
	length = ( ( FastCos( cosine ) + 1.0f ) * 50.0f + 1.0f ) * length;

	return length + fromArea->GetCostSoFar();
}

CZombie *CZombie::SpawnAtPos( Vector const &pos, float flLifeTime, int iTeamNum, CBaseEntity *pOwner, SkeletonType_t eType )
{
	CZombie *pZombie = (CZombie *)CreateEntityByName( "tf_zombie" );
	if ( pZombie )
	{
		pZombie->SetOwnerEntity( pOwner );
		pZombie->ChangeTeam( iTeamNum );
		pZombie->SetAbsOrigin( pos );

		DispatchSpawn( pZombie );

		if ( flLifeTime > 0.0f )
			pZombie->m_timeTillDeath.Start( flLifeTime );

		pZombie->SetSkeletonType( eType );

		return pZombie;
	}

	return NULL;
}

class CZombieSpawner : public CPointEntity
{
	DECLARE_CLASS( CZombieSpawner, CPointEntity );
public:
	CZombieSpawner();
	virtual ~CZombieSpawner() {
		m_hZombies.RemoveAll();
	}

	DECLARE_DATADESC()

	virtual void Spawn( void );
	virtual void Think( void );

	void InputEnable( inputdata_t &input );
	void InputDisable( inputdata_t &input );
	void InputSetMaxActiveZombies( inputdata_t &input );

private:
	float m_flZombieLifeTime;
	int m_nMaxActiveZombies;
	bool m_bEnabled;
	bool m_bInfiniteZombies;
	CZombie::SkeletonType_t m_nSkeletonType;
	int m_iNumSpawned;
	CUtlVector< CHandle<CZombie> > m_hZombies;
};

BEGIN_DATADESC( CZombieSpawner )
	DEFINE_KEYFIELD( m_flZombieLifeTime, FIELD_FLOAT, "zombie_lifetime" ),
	DEFINE_KEYFIELD( m_nMaxActiveZombies, FIELD_INTEGER, "max_zombies" ),
	DEFINE_KEYFIELD( m_bInfiniteZombies, FIELD_BOOLEAN, "infinite_zombies" ),
	DEFINE_KEYFIELD( m_nSkeletonType, FIELD_INTEGER, "skeleton_type" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxActiveZombies", InputSetMaxActiveZombies ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_zombie_spawner, CZombieSpawner );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CZombieSpawner::CZombieSpawner()
{
	m_flZombieLifeTime = 0;
	m_bEnabled = true;
	m_bInfiniteZombies = false;
	m_iNumSpawned = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombieSpawner::Spawn( void )
{
	BaseClass::Spawn();
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombieSpawner::Think( void )
{
	CBaseHandle null;
	m_hZombies.FindAndFastRemove( null );

	if ( !m_bEnabled )
	{
		SetNextThink( gpGlobals->curtime + 0.2f );
		return;
	}

	if ( m_bInfiniteZombies  )
	{
		if ( m_hZombies.Count() > m_nMaxActiveZombies )
		{
			SetNextThink( gpGlobals->curtime + 0.2f );
			return;
		}
	}
	else if ( m_iNumSpawned >= m_nMaxActiveZombies )
	{
		SetNextThink( gpGlobals->curtime + 0.2f );
		return;
	}

	CZombie *pZombie = CZombie::SpawnAtPos( GetAbsOrigin(), m_flZombieLifeTime, TF_TEAM_NPC, NULL, m_nSkeletonType );
	if ( pZombie )
	{
		++m_iNumSpawned;

		CHandle<CZombie> hndl( pZombie );
		m_hZombies.AddToTail( hndl );
	}

	SetNextThink( gpGlobals->curtime + RandomFloat( 1.5, 3.0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombieSpawner::InputEnable( inputdata_t &data )
{
	m_bEnabled = true;

	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombieSpawner::InputDisable( inputdata_t &data )
{
	m_bEnabled = false;
	m_iNumSpawned = 0;
	m_hZombies.RemoveAll();

	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombieSpawner::InputSetMaxActiveZombies( inputdata_t &data )
{
	m_nMaxActiveZombies = data.value.Int();
}
#endif

CZombie::CZombie()
{
#if defined( GAME_DLL )
	m_intention = new CZombieIntention( this );
	m_body = new CHeadlessHatmanBody( this );
	m_locomotor = new CZombieLocomotion( this );

	m_timeTillDeath.Invalidate();
	m_bNoteShouldDie = false;
	m_flAttDamage = 30.0f;
	m_flAttRange = 50.0f;
#endif

	m_flHeadScale = 1.0f;
}

CZombie::~CZombie()
{
#if defined( GAME_DLL )
	delete m_intention;
	delete m_body;
	delete m_locomotor;
#endif
}

#if !defined( CLIENT_DLL )

void CZombie::Spawn( void )
{
	Precache();
	SetModel( "models/bots/skeleton_sniper/skeleton_sniper.mdl" );

	BaseClass::Spawn();

	SetMaxHealth( 50 );
	SetHealth( 50 );

	AddFlag( FL_NPC );

	SetAbsAngles( QAngle( 0, RandomFloat( 0, 360.f ), 0 ) );

	GetBodyInterface()->StartActivity( ACT_TRANSITION );

	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			m_nSkin = 0;
			break;
		case TF_TEAM_BLUE:
			m_nSkin = 1;
			break;
		default:
			m_nSkin = 2;
			break;
	}

	int nNumLeft = IZombieAutoList::AutoList().Count() - tf_max_active_zombies.GetInt();
	if ( nNumLeft > 0 )
	{
		// send a notice to the oldest living zombies that they should self-destruct
		FOR_EACH_VEC( IZombieAutoList::AutoList(), i )
		{
			CZombie *pZombie = (CZombie *)IZombieAutoList::AutoList()[i];
			if ( !pZombie )
				continue;

			if ( pZombie->m_nType != SkeletonType_t::KING )
			{
				pZombie->m_bNoteShouldDie = true;
				--nNumLeft;
			}

			if ( nNumLeft == 0 )
				break;
		}
	}
}

void CZombie::Precache( void )
{
	BaseClass::Precache();
	
	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheZombie();

	CBaseEntity::SetAllowPrecache( allowPrecache );
}

void CZombie::UpdateOnRemove( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();

	UTIL_Remove( m_hHat.Get() );

	BaseClass::UpdateOnRemove();
}

void CZombie::Event_Killed( CTakeDamageInfo const &info )
{
	EmitSound( "Halloween_skeleton_break" );

	// Achievement for something goes here

	BaseClass::Event_Killed( info );
}

int CZombie::OnTakeDamage_Alive( CTakeDamageInfo const &info )
{
	if ( info.GetAttacker() == nullptr )
		return 0;

	if ( info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
		return 0;

	if ( !IsPlayingGesture( ACT_MP_GESTURE_FLINCH_ITEM1 ) )
		AddGesture( ACT_MP_GESTURE_FLINCH_ITEM1 );

	char const *pszDamageEffect = "spell_skeleton_goop_green";
	if ( GetTeamNumber() != TF_TEAM_NPC )
	{
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				pszDamageEffect = "spell_pumpkin_mirv_goop_red";
				break;
			case TF_TEAM_BLUE:
				pszDamageEffect = "spell_pumpkin_mirv_goop_blue";
				break;
			default:
				break;
		}
	}

	DispatchParticleEffect( pszDamageEffect, info.GetDamagePosition(), GetAbsAngles() );

	return BaseClass::OnTakeDamage_Alive( info );
}

void CZombie::AddHat( char const *szModelName )
{
	if ( m_hHat != nullptr )
		return;

	int headBone = LookupBone( "bip_head" );
	if ( headBone == -1 )
		return;

	CBaseEntity *pProp = CreateEntityByName( "prop_dynamic" );
	if ( pProp )
	{
		pProp->SetModel( szModelName );

		Vector vecOrigin; QAngle vecAngles;
		GetBonePosition( headBone, vecOrigin, vecAngles );
		pProp->SetAbsOrigin(vecOrigin);
		pProp->SetAbsAngles(vecAngles);

		pProp->FollowEntity( this );

		m_hHat = pProp;
	}
}

void CZombie::SetSkeletonType( SkeletonType_t eType )
{
	m_nType = eType;
	if ( eType == MINION )
	{
		SetModel( "models/bots/skeleton_sniper/skeleton_sniper.mdl" );

		SetModelScale( 0.5f );
		m_flHeadScale = 3.0f;

		if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
			AddHat( s_skeletonHatModels[ RandomInt( 0, 3 ) ] );

		m_flAttRange = 40.0f;
		m_flAttDamage = 20.0f;
	}
	else if ( eType == KING )
	{
		SetModel( "models/bots/skeleton_sniper/skeleton_sniper_boss.mdl" );
		SetModelScale( 2.0f );

		SetMaxHealth( 1000 );
		SetHealth( 1000 );

		AddHat( "models/player/items/demo/crown.mdl" );

		m_flAttRange = 100.0f;
		m_flAttDamage = 100.0f;
	}
}

void CZombie::PrecacheZombie( void )
{
	int iMdlIdx = PrecacheModel( "models/bots/skeleton_sniper/skeleton_sniper.mdl" );
	PrecacheGibsForModel( iMdlIdx );

	iMdlIdx = PrecacheModel( "models/bots/skeleton_sniper_boss/skeleton_sniper_boss.mdl" );
	PrecacheGibsForModel( iMdlIdx );

	PrecacheModel( "models/player/items/demo/crown.mdl" );

	if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		for ( int i=0; i < NELEMS( s_skeletonHatModels ); ++i )
			PrecacheModel( s_skeletonHatModels[i] );
	}

	PrecacheParticleSystem("bomibomicon_ring");
	PrecacheParticleSystem("spell_pumpkin_mirv_goop_red");
	PrecacheParticleSystem("spell_pumpkin_mirv_goop_blue");
	PrecacheParticleSystem("spell_skeleton_goop_green");

	PrecacheScriptSound( "Halloween_skeleton_break" );
	PrecacheScriptSound( "Halloween_skeleton_laugh_small" );
	PrecacheScriptSound( "Halloween_skeleton_laugh_medium" );
	PrecacheScriptSound( "Halloween_skeleton_laugh_giant" );
}

bool CZombie::ShouldSuicide( void ) const
{
	if ( m_timeTillDeath.HasStarted() && m_timeTillDeath.IsElapsed() )
		return true;

	if ( GetOwnerEntity() && GetOwnerEntity()->GetTeamNumber() != GetTeamNumber() )
		return true;

	return m_bNoteShouldDie;
}

#else

bool CZombie::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
		return false;

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

void CZombie::BuildTransformations( CStudioHdr *pStudio, Vector *pos, Quaternion *q, matrix3x4_t const& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( pStudio, pos, q, cameraTransform, boneMask, boneComputed );
	m_iPrevBoneMask = BONE_USED_BY_ANYTHING;
	BuildBigHeadTransformation( this, pStudio, pos, q, cameraTransform, boneMask, boneComputed, m_flHeadScale );
}
#endif
