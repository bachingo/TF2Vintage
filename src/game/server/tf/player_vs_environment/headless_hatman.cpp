//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "datacache/imdlcache.h"
#include "props_shared.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "entity_bossresource.h"
#include "headless_hatman_behavior.h"
#include "headless_hatman.h"

ConVar tf_halloween_bot_health_base( "tf_halloween_bot_health_base", "3000", FCVAR_CHEAT );
ConVar tf_halloween_bot_health_per_player( "tf_halloween_bot_health_per_player", "200", FCVAR_CHEAT );
ConVar tf_halloween_bot_min_player_count( "tf_halloween_bot_min_player_count", "1", FCVAR_CHEAT );
ConVar tf_halloween_bot_speed( "tf_halloween_bot_speed", "400", FCVAR_CHEAT );
ConVar tf_halloween_bot_attack_range( "tf_halloween_bot_attack_range", "200", FCVAR_CHEAT );
ConVar tf_halloween_bot_speed_recovery_rate( "tf_halloween_bot_speed_recovery_rate", "100", FCVAR_CHEAT, "Movement units/second" );
ConVar tf_halloween_bot_chase_duration( "tf_halloween_bot_chase_duration", "30", FCVAR_CHEAT );
ConVar tf_halloween_bot_chase_range( "tf_halloween_bot_chase_range", "1500", FCVAR_CHEAT );
ConVar tf_halloween_bot_quit_range( "tf_halloween_bot_quit_range", "2000", FCVAR_CHEAT );
ConVar tf_halloween_bot_terrify_radius( "tf_halloween_bot_terrify_radius", "500", FCVAR_CHEAT );


IMPLEMENT_INTENTION_INTERFACE( CHeadlessHatman, CHeadlessHatmanBehavior )


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CHeadlessHatmanLocomotion::GetRunSpeed( void ) const
{
	return tf_halloween_bot_speed.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHeadlessHatmanLocomotion::ShouldCollideWith( const CBaseEntity *other ) const
{
	if ( !TFGameRules() || !other )
		return true;

	if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) && other->IsPlayer() )
		return false;

	return true;
}



CHeadlessHatmanBody::CHeadlessHatmanBody( INextBot *actor )
	: IBody( actor )
{
	m_iMoveX = -1;
	m_iMoveY = -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHeadlessHatmanBody::Update( void )
{
	CBaseCombatCharacter *actor = GetBot()->GetEntity();
	if ( m_iMoveX < 0 )
		m_iMoveX = actor->LookupPoseParameter( "move_x" );
	if ( m_iMoveY < 0 )
		m_iMoveY = actor->LookupPoseParameter( "move_y" );

	float flSpeed = GetBot()->GetLocomotionInterface()->GetGroundSpeed();
	if ( flSpeed >= 0.01f ) // only animate if moving enough
	{
		Vector vecFwd, vecRight;
		actor->GetVectors( &vecFwd, &vecRight, nullptr );

		Vector vecDir = GetBot()->GetLocomotionInterface()->GetGroundMotionVector();

		if ( m_iMoveX >= 0 )
			actor->SetPoseParameter( m_iMoveX, vecDir.Dot( vecFwd ) );
		if ( m_iMoveY >= 0 )
			actor->SetPoseParameter( m_iMoveY, vecDir.Dot( vecRight ) );
	}
	else
	{
		if ( m_iMoveX >= 0 )
			actor->SetPoseParameter( m_iMoveX, 0.0f );
		if ( m_iMoveY >= 0 )
			actor->SetPoseParameter( m_iMoveY, 0.0f );
	}

	if ( actor->m_flGroundSpeed )
		actor->SetPlaybackRate( Clamp( flSpeed / actor->m_flGroundSpeed, -4.0f, 12.0f ) );

	actor->StudioFrameAdvance();
	actor->DispatchAnimEvents( actor );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHeadlessHatmanBody::StartActivity( Activity act, unsigned int flags )
{
	CBaseCombatCharacter *actor = GetBot()->GetEntity();

	int iSequence = actor->SelectWeightedSequence( act );
	if ( iSequence )
	{
		m_Activity = act;

		actor->SetSequence( iSequence );
		actor->SetPlaybackRate( 1.0f );
		actor->SetCycle( 0.0f );
		actor->ResetSequenceInfo();

		return true;
	}

	return false;
}


IMPLEMENT_SERVERCLASS_ST( CHeadlessHatman, DT_HeadlessHatman )
END_SEND_TABLE()

BEGIN_DATADESC( CHeadlessHatman )
END_DATADESC()

LINK_ENTITY_TO_CLASS( headless_hatman, CHeadlessHatman );

CHeadlessHatman::CHeadlessHatman()
{
	m_intention = new CHeadlessHatmanIntention( this );
	m_locomotor = new CHeadlessHatmanLocomotion( this );
	m_body = new CHeadlessHatmanBody( this );
}

CHeadlessHatman::~CHeadlessHatman()
{
	delete m_intention;
	delete m_locomotor;
	delete m_body;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHeadlessHatman::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( "models/bots/headless_hatman.mdl" );

	m_pAxe = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_pAxe )
	{
		m_pAxe->SetModel( GetWeaponModel() );
		m_pAxe->FollowEntity( this );
	}

	int iHealth = tf_halloween_bot_health_base.GetInt();
	int iNumPlayers = GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() + GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers();
	int iMinPlayers = tf_halloween_bot_min_player_count.GetInt();
	if ( iNumPlayers > iMinPlayers )
		iHealth += tf_halloween_bot_health_per_player.GetInt() * ( iNumPlayers - iMinPlayers );

	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	m_hTarget = nullptr;

	m_vecSpawn = GetAbsOrigin();

	SetBloodColor( DONT_BLEED );

	if ( !TFGameRules() || !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		if ( g_pMonsterResource )
			g_pMonsterResource->SetBossHealthPercentage( 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHeadlessHatman::Precache( void )
{
	BaseClass::Precache();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheHeadlessHatman();

	CBaseEntity::SetAllowPrecache( allowPrecache );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CHeadlessHatman::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( g_pMonsterResource && TFGameRules() && !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		float flHPPercent = (float)GetHealth() / GetMaxHealth();
		if ( flHPPercent <= 0.0f )
			g_pMonsterResource->HideBossHealthMeter();
		else
			g_pMonsterResource->SetBossHealthPercentage( flHPPercent );
	}

	extern void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity * pEntity );
	DispatchParticleEffect( "halloween_boss_injured", info.GetDamagePosition(), GetAbsAngles(), NULL );

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHeadlessHatman::Update( void )
{
	BaseClass::Update();
	// there's a "damage" pose parameter being inversely changed with health percentage
	// here, but the model doesn't appear to have such a thing
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHeadlessHatman::PrecacheHeadlessHatman( void )
{
	int iMdlIdx = PrecacheModel( "models/bots/headless_hatman.mdl" );
	PrecacheGibsForModel( iMdlIdx );

	if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		PrecacheModel( "models/weapons/c_models/c_big_mallet/c_big_mallet.mdl" );
	else
		PrecacheModel( "models/weapons/c_models/c_bigaxe/c_bigaxe.mdl" );

	PrecacheScriptSound( "Halloween.HeadlessBossSpawn" );
	PrecacheScriptSound( "Halloween.HeadlessBossSpawnRumble" );
	PrecacheScriptSound( "Halloween.HeadlessBossAttack" );
	PrecacheScriptSound( "Halloween.HeadlessBossAlert" );
	PrecacheScriptSound( "Halloween.HeadlessBossBoo" );
	PrecacheScriptSound( "Halloween.HeadlessBossPain" );
	PrecacheScriptSound( "Halloween.HeadlessBossLaugh" );
	PrecacheScriptSound( "Halloween.HeadlessBossDying" );
	PrecacheScriptSound( "Halloween.HeadlessBossDeath" );
	PrecacheScriptSound( "Halloween.HeadlessBossAxeHitFlesh" );
	PrecacheScriptSound( "Halloween.HeadlessBossAxeHitWorld" );
	PrecacheScriptSound( "Halloween.HeadlessBossFootfalls" );
	PrecacheScriptSound( "Player.IsNowIt" );
	PrecacheScriptSound( "Player.YouAreIt" );
	PrecacheScriptSound( "Player.TaggedOtherIt" );

	PrecacheParticleSystem( "halloween_boss_summon" );
	PrecacheParticleSystem( "halloween_boss_axe_hit_world" );
	PrecacheParticleSystem( "halloween_boss_injured" );
	PrecacheParticleSystem( "halloween_boss_death" );
	PrecacheParticleSystem( "halloween_boss_foot_impact" );
	PrecacheParticleSystem( "halloween_boss_eye_glow" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CHeadlessHatman::GetWeaponModel( void ) const
{
	if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		return "models/weapons/c_models/c_big_mallet/c_big_mallet.mdl";

	return "models/weapons/c_models/c_bigaxe/c_bigaxe.mdl";
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CHeadlessHatmanPathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
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

		// we won't actually get here according to the locomotor
		length *= 5;
	}
	else
	{
		if ( dz < -m_Actor->GetLocomotionInterface()->GetDeathDropHeight() )
			return -1.0f;
	}

	return length + fromArea->GetCostSoFar();
}
