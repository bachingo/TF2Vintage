//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "soundenvelope.h"
#include "logicrelay.h"
#include "tf_fx.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "entity_bossresource.h"
#include "entity_wheelofdoom.h"
#include "tf_weaponbase_grenadeproj.h"
#include "merasmus_components.h"
#include "merasmus_behavior.h"
#include "merasmus.h"


ConVar tf_merasmus_health_base( "tf_merasmus_health_base", "33750", FCVAR_CHEAT );
ConVar tf_merasmus_health_per_player( "tf_merasmus_health_per_player", "2500", FCVAR_CHEAT );
ConVar tf_merasmus_min_player_count( "tf_merasmus_min_player_count", "10", FCVAR_CHEAT );
ConVar tf_merasmus_lifetime( "tf_merasmus_lifetime", "120", FCVAR_CHEAT );
ConVar tf_merasmus_bomb_head_duration( "tf_merasmus_bomb_head_duration", "15", FCVAR_CHEAT );
ConVar tf_merasmus_bomb_head_per_team( "tf_merasmus_bomb_head_per_team", "1", FCVAR_CHEAT );
ConVar tf_merasmus_min_props_to_reveal( "tf_merasmus_min_props_to_reveal", "0.7", FCVAR_CHEAT );
ConVar tf_merasmus_should_disguise_threshold( "tf_merasmus_should_disguise_threshold", "0.45", FCVAR_CHEAT );
ConVar tf_merasmus_stun_duration( "tf_merasmus_stun_duration", "2", FCVAR_CHEAT );
extern ConVar tf_merasmus_spawn_interval;
extern ConVar tf_merasmus_spawn_interval_variation;

int CMerasmus::m_level = 1;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float MerasmusModifyDamage( CTakeDamageInfo const& info )
{
	const float flDamage = info.GetDamage();

	CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( info.GetInflictor() );
	if ( pSentry )
		return flDamage / 2;

	CTFProjectile_SentryRocket *pRocket = dynamic_cast<CTFProjectile_SentryRocket *>( info.GetInflictor() );
	if ( pRocket )
		return flDamage / 2;

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
	if ( pWeapon )
	{
		switch ( pWeapon->GetWeaponID() )
		{
			case TF_WEAPON_KNIFE:
			case TF_WEAPON_SNIPERRIFLE:
			case TF_WEAPON_COMPOUND_BOW:
			case TF_WEAPON_SNIPERRIFLE_DECAP:
			case TF_WEAPON_PEP_BRAWLER_BLASTER:
			case TF_WEAPON_SNIPERRIFLE_CLASSIC:
				return flDamage * 3;
			case TF_WEAPON_SCATTERGUN:
			case TF_WEAPON_REVOLVER:
				return flDamage * 2;
			case TF_WEAPON_MINIGUN:
			case TF_WEAPON_MINIGUN_REAL:
				return flDamage / 2;
			case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
				return flDamage * 1.75;
			case TF_WEAPON_SODA_POPPER:
				return flDamage * 1.5;
		}
	}

	return flDamage;
}

void BombHeadForTeam( int iTeam, float flDuration )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, iTeam, true );

	for ( CTFPlayer *pPlayer : players )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
			continue;

		pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_BOMB_HEAD, flDuration );
	}
}

void RemoveAllBombHeadFromPlayers( void )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, TF_TEAM_RED, true );
	CollectPlayers( &players, TF_TEAM_BLUE, true, true );

	for ( CTFPlayer *pPlayer : players )
	{
		if ( !pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
			continue;

		pPlayer->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Prevent sticky camping Merasmus' spawn point, naughty naughty
//-----------------------------------------------------------------------------
void RemoveAllGrenades( CMerasmus *pActor )
{
	CBaseEntity *pGrenades[1024];
	CFlaggedEntitiesEnum func( pGrenades, 1024, FL_GRENADE );

	int nGrenades = UTIL_EntitiesInSphere( pActor->GetAbsOrigin(), 400.0f, &func );
	for ( int i=0; i < nGrenades; ++i )
	{
		CBaseEntity *pGrenade = pGrenades[i];
		if ( pGrenade->IsPlayer() ) // Do they expect this?
			continue;

		pGrenade->SetThink( &CBaseEntity::SUB_Remove );
		pGrenade->SetNextThink( gpGlobals->curtime );

		pGrenade->AddEffects( EF_NODRAW );
	}
}

void CollectTargets( CBaseCombatCharacter *pActor, float fRadius, int iTeam, int nMaxTargets, CUtlVector<EHANDLE> *pOut )
{
	pOut->RemoveAll();

	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, iTeam, true );

	CUtlVector<CTFPlayer *> validPlayers;
	for ( CTFPlayer *pPlayer : players )
	{
		if ( ( pPlayer->WorldSpaceCenter() - pActor->EyePosition() ).Length() >= fRadius )
			continue;

		if ( !pActor->IsLineOfSightClear( pPlayer, CBaseCombatCharacter::IGNORE_NOTHING ) )
			continue;

		validPlayers.AddToTail( pPlayer );
	}

	while ( !validPlayers.IsEmpty() )
	{
		if ( pOut->Count() == nMaxTargets )
			break;

		int nRandom = RandomInt( 0, validPlayers.Count() - 1 );
		pOut->AddToTail( validPlayers[ nRandom ] );
		validPlayers.FastRemove( nRandom );
	}

	FOR_EACH_VEC( IBaseObjectAutoList::AutoList(), i )
	{
		CBaseObject *pObject = (CBaseObject *)IBaseObjectAutoList::AutoList()[i];
		if ( pObject->GetType() != OBJ_SENTRYGUN )
			continue;

		if ( ( pObject->WorldSpaceCenter() - pActor->EyePosition() ).Length() >= fRadius )
			continue;

		if ( !pActor->IsLineOfSightClear( pObject, CBaseCombatCharacter::IGNORE_NOTHING ) )
			continue;

		pOut->AddToTail( pObject );
	}
}


IMPLEMENT_INTENTION_INTERFACE( CMerasmus, CMerasmusBehavior )


IMPLEMENT_SERVERCLASS_ST( CMerasmus, DT_Merasmus )
	SendPropBool( SENDINFO( m_bRevealed ) ),
	SendPropBool( SENDINFO(m_bDoingAOEAttack)),
	SendPropBool( SENDINFO(m_bStunned)),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( merasmus, CMerasmus );

CMerasmus::CMerasmus()
{
	m_intention = new CMerasmusIntention( this );
	m_body = new CMerasmusBody( this );
	m_pGroundLoco = new CMerasmusLocomotion( this );
	m_pFlyingLoco = new CMerasmusFlyingLocomotion( this );

	m_bRevealed = false;
	m_bDoingAOEAttack = false;
	m_bStunned = false;

	m_hBossResource = g_pMonsterResource;
	ListenForGameEvent( "player_death" );
}

CMerasmus::~CMerasmus()
{
	delete m_intention;
	delete m_body;
	delete m_pGroundLoco;
	delete m_pFlyingLoco;
}

CTFWeaponBaseMerasmusGrenade *CMerasmus::CreateMerasmusGrenade( Vector const &vecOrigin, Vector const &vecVelocity, CBaseCombatCharacter *pOwner, float flModelScale )
{
	CTFWeaponBaseMerasmusGrenade *pGrenade = 
		(CTFWeaponBaseMerasmusGrenade *)CBaseEntity::Create( "tf_weaponbase_merasmus_grenade", vecOrigin, RandomAngle( 0, 90 ), pOwner );
	if ( pGrenade )
	{
		pGrenade->SetModel( "models/props_lakeside_event/bomb_temp.mdl" );
		DispatchSpawn( pGrenade );

		AngularImpulse impulse( 600, random->RandomFloat( -1200, 1200 ), 0 );
		pGrenade->InitGrenade( vecVelocity, impulse, pOwner, flModelScale * 50.0, flModelScale * 300.0 );
		pGrenade->SetDetonateTimerLength( 2.0f );

		pGrenade->SetModelScale( flModelScale );
		pGrenade->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );
	}

	return pGrenade;
}

bool CMerasmus::Zap( CBaseCombatCharacter *pCaster, char const *szAttachment, float fRadius, float fMinDamage, float fMaxDamage, int nMaxTargets, int iTargetTeam )
{
	CUtlVector<EHANDLE> targets;
	CollectTargets( pCaster, fRadius, iTargetTeam, nMaxTargets, &targets );

	if ( !targets.IsEmpty() )
	{
		for ( CBaseEntity *pEntity : targets )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pEntity );
			if ( pPlayer == nullptr )
			{
				if ( pEntity->IsBaseObject() )
					static_cast<CBaseObject *>( pEntity )->DetonateObject();
			}
			else
			{
				Vector impulse( 0, 0, 1000.0 );
				pPlayer->ApplyAbsVelocityImpulse( impulse );

				const float flDistance = pCaster->EyePosition().DistTo( pPlayer->WorldSpaceCenter() );
				const float flDamage = RemapValClamped( flDistance, 100.0, Square( fRadius / 2 ), fMinDamage, fMaxDamage );
				CTakeDamageInfo info( pCaster, pCaster, flDamage, DMG_BURN, TF_DMG_CUSTOM_MERASMUS_ZAP );

				pPlayer->TakeDamage( info );
				pPlayer->m_Shared.Burn( pCaster, 5.0 );
			}

			Vector vecOrigin; QAngle vecAngles;
			pCaster->GetAttachment( szAttachment, vecOrigin, vecAngles );

			te_tf_particle_effects_control_point_t controlPoint ={
				PATTACH_ABSORIGIN,
				pEntity->WorldSpaceCenter()
			};

			CReliableBroadcastRecipientFilter filter;
			TE_TFParticleEffectComplex( filter, 0, "merasmus_zap", vecOrigin, vecAngles, NULL, &controlPoint, pCaster );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( "models/bots/merasmus/merasmus.mdl" );
	SetBloodColor( DONT_BLEED );

	PlayHighPrioritySound( "Halloween.MerasmusAppears" );

	if( m_pFloatSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFloatSound );
		m_pFloatSound = NULL;
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_summoned" );
	if ( event )
	{
		event->SetInt( "level", GetLevel() );

		gameeventmanager->FireEvent( event );
	}

	TriggerLogicRelay( "boss_enter_relay", true );
	DispatchParticleEffect( "merasmus_spawn", GetAbsOrigin(), GetAbsAngles() );

	m_hWheelOfFate = (CWheelOfDoom *)gEntList.FindEntityByName( NULL, "wheel_of_fortress" );

	int iHealth = tf_merasmus_health_base.GetInt();
	const int iNumPlayers = GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() + GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers();
	const int iMinPlayers = tf_merasmus_min_player_count.GetInt();
	if ( iNumPlayers > iMinPlayers )
		iHealth += tf_merasmus_health_per_player.GetInt() * ( iNumPlayers - iMinPlayers );

	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	m_lifeTimeDuration.Start( tf_merasmus_lifetime.GetFloat() );

	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_STANDABLE );

	m_vecHome = GetAbsOrigin();

	CPVSFilter filter( GetAbsOrigin() );
	m_pFloatSound = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "Halloween.Merasmus_Float" );
	CSoundEnvelopeController::GetController().SoundChangeVolume( m_pFloatSound, 1.0, 100.f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::Precache( void )
{
	BaseClass::Precache();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheMerasmus();

	CBaseEntity::SetAllowPrecache( allowPrecache );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::UpdateOnRemove( void )
{
	Assert( m_hBossResource );
	m_hBossResource->HideBossHealthMeter();

	if( m_pFloatSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFloatSound );
		m_pFloatSound = NULL;
	}

	RemoveAllBombHeadFromPlayers();
	RemoveAllFakeProps();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CMerasmus::OnTakeDamage_Alive( CTakeDamageInfo const &info )
{
	// Invulnerable while hiding
	if( !m_bRevealed )
		return 0;

	// Don't hurt ourselves
	if ( IsSelf( info.GetAttacker() ) )
		return 0;

	CTakeDamageInfo newInfo{info};

	if ( m_bStunned )
		DispatchParticleEffect( "merasmus_blood", newInfo.GetDamagePosition(), GetAbsAngles() );
	else
		DispatchParticleEffect( "merasmus_blood_bits", newInfo.GetDamagePosition(), GetAbsAngles() );

	const float flDamage = MerasmusModifyDamage( info );
	newInfo.SetDamage( flDamage );

	if ( m_bDoingAOEAttack || m_bStunned )
		newInfo.AddDamageType( DMG_CRITICAL );

	int healthTaken = BaseClass::OnTakeDamage_Alive( newInfo );

	if ( m_hBossResource )
	{
		float flHPPercent = (float)GetHealth() / GetMaxHealth();
		if ( flHPPercent <= 0.0f )
			m_hBossResource->HideBossHealthMeter();
		else
			m_hBossResource->SetBossHealthPercentage( flHPPercent );
	}

	return healthTaken;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::FireGameEvent( IGameEvent *event )
{
	if ( V_stricmp( event->GetName(), "player_death" ) == 0 )
	{
		int iCustomKill = event->GetInt( "customkill", -1 );
		switch ( iCustomKill )
		{
			case TF_DMG_CUSTOM_MERASMUS_GRENADE:
				m_iGrenadeKillCombo++;
				break;
			case TF_DMG_CUSTOM_MERASMUS_ZAP:
				m_iZapKillCombo++;
				break;
			default:
				m_iPlayerKillCombo++;
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::Update( void )
{
	BaseClass::Update();
	// there's a "damage" pose parameter being inversely changed with health percentage
	// here, but the model doesn't appear to have such a thing
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ILocomotion *CMerasmus::GetLocomotionInterface( void ) const
{
	if ( m_bDoingAOEAttack )
		return m_pFlyingLoco;

	return m_pGroundLoco;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::PushPlayer( CTFPlayer *pTarget, float flStrength )
{
	Vector vecToPlayer = pTarget->EyePosition() - GetAbsOrigin();
	vecToPlayer.NormalizeInPlace();

	pTarget->ApplyAbsVelocityImpulse( vecToPlayer * flStrength );
}

//-----------------------------------------------------------------------------
// Purpose: Play a sound to all connected players
//-----------------------------------------------------------------------------
void CMerasmus::PlayHighPrioritySound( char const *szSoundName )
{
	CSoundParameters parms;
	if ( !GetParametersForSound( szSoundName, parms, NULL ) )
		return;

	CBroadcastRecipientFilter filter;
	EmitSound_t emit( parms );
	CBaseEntity::EmitSound( filter, entindex(), emit );
}

//-----------------------------------------------------------------------------
// Purpose: Play a sound to filtered players
//-----------------------------------------------------------------------------
void CMerasmus::PlayLowPrioritySound( IRecipientFilter &filter, char const *szSoundName )
{
	CSoundParameters parms;
	if ( !GetParametersForSound( szSoundName, parms, NULL ) )
		return;

	EmitSound_t emit( parms );
	CBaseEntity::EmitSound( filter, entindex(), emit );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::TriggerLogicRelay( char const *szRelayName, bool bTeleportTo )
{
	CLogicRelay *pRelay = (CLogicRelay *)gEntList.FindEntityByName( NULL, szRelayName );
	if ( pRelay == nullptr )
		return;

	inputdata_t empty;
	pRelay->InputTrigger( empty );

	if ( bTeleportTo )
		SetAbsOrigin( pRelay->GetAbsOrigin() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMerasmus::IsNextKilledPropMerasmus( void )
{
	int nIndex = 1;
	for ( int i=0; i < m_hTrickProps.Count(); ++i )
	{
		if ( m_hTrickProps[i].Get() )
			break;
		nIndex++;
	}

	return m_nHidingSpotIndex == nIndex;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::RemoveAllFakeProps( void )
{
	for ( int i=0; i < m_hTrickProps.Count(); ++i )
	{
		if ( !m_hTrickProps[i] )
			continue;

		UTIL_Remove( m_hTrickProps[i] );
	}

	m_hTrickProps.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMerasmus::ShouldDisguise( void )
{
	if( GetHealth() <= 0 )
		return false;

	int nDisguiseThreshold = m_nDisguiseLastHealth - GetHealth();
	return (nDisguiseThreshold / GetMaxHealth()) > tf_merasmus_should_disguise_threshold.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMerasmus::ShouldReveal( void )
{
	int nIndex = 0;
	for ( int i=0; i < m_hTrickProps.Count(); ++i )
	{
		if ( m_hTrickProps[i].Get() )
			break;
		nIndex++;
	}

	return m_nHidingSpotIndex <= nIndex;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMerasmus::ShouldLeave( void )
{
	return m_lifeTimeDuration.IsElapsed();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::OnDisguise( void )
{
	m_bRevealed = false;
	m_bDoingAOEAttack = false;

	DispatchParticleEffect( "merasmus_tp", GetAbsOrigin(), GetAbsAngles() );

	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );

	AddEffects( EF_NODRAW|EF_NOINTERP );

	int nMinProps = Min( m_hTrickProps.Count() * tf_merasmus_min_props_to_reveal.GetFloat(), 1.0f );
	m_nHidingSpotIndex = RandomInt( nMinProps, m_hTrickProps.Count() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::OnRevealed( bool bFound )
{
	m_bRevealed = true;
	m_nDisguiseLastHealth = GetHealth();

	if ( m_hHideNSeekWinner )
	{
		m_hHideNSeekWinner->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, 10.0 );
		m_hHideNSeekWinner->m_Shared.AddCond( TF_COND_SPEED_BOOST, 10.0 );
		m_hHideNSeekWinner->m_Shared.AddCond( TF_COND_INVULNERABLE, 10.0 );
	}
	m_hHideNSeekWinner = NULL;

	Assert( m_hBossResource );
	m_hBossResource->SetBossHealthPercentage( GetHealth() / GetMaxHealth() );

	if ( bFound )
		PlayHighPrioritySound( "Halloween.MerasmusDiscovered" );

	RemoveEffects( EF_NODRAW|EF_NOINTERP );

	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_STANDABLE );

	DispatchParticleEffect( "merasmus_spawn", GetAbsOrigin(), GetAbsAngles() );

	RemoveAllFakeProps();
	RemoveAllGrenades( this );

	TFGameRules()->PushAllPlayersAway( GetAbsOrigin(), 400.0, 500.0, TF_TEAM_RED, NULL );
	TFGameRules()->PushAllPlayersAway( GetAbsOrigin(), 400.0, 500.0, TF_TEAM_BLUE, NULL );

	// Teleport in facing the closest player

	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, TF_TEAM_RED, true );
	CollectPlayers( &players, TF_TEAM_BLUE, true, true );

	CTFPlayer *pClosest = NULL;
	float flMinDistance = FLT_MAX;

	for ( CTFPlayer *pPlayer : players )
	{
		const float flDistance = GetRangeSquaredTo( pPlayer );
		if ( flMinDistance > flDistance )
		{
			flMinDistance = flDistance;
			pClosest = pPlayer;
		}
	}

	QAngle angLook;
	if ( pClosest )
	{
		const Vector vecUp( 0, 0, 1.0 );
		const Vector2D vec2DToPl = ( pClosest->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D() );

		Vector vecToPlayer( vec2DToPl.x, vec2DToPl.y, 0 );
		vecToPlayer.NormalizeInPlace();

		VectorAngles( vecToPlayer, vecUp, angLook );
	}
	else
	{
		angLook[ YAW ] = RandomFloat( 0, 360 );
	}

	SetAbsAngles( angLook );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::OnBeginStun( void )
{
	EmitSound( "Halloween.Merasmus_Stun" );

	m_bStunned = true;
	m_bDoingAOEAttack = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::OnEndStun( void )
{
	m_bStunned = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::OnLeaveWhileInDisguise( void )
{
	CUtlVector<CBaseEntity *> remainingProps;
	for ( int i=0; i < m_hTrickProps.Count(); ++i )
	{
		if ( !m_hTrickProps[i] )
			continue;

		remainingProps.AddToTail( m_hTrickProps[i] );
	}

	if ( !remainingProps.IsEmpty() )
	{
		CBaseEntity *pProp = remainingProps.Random();
		SetAbsOrigin( pProp->GetAbsOrigin() );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::AddStun( CTFPlayer *pStunner )
{
	if ( !m_bRevealed )
		return;

	if ( !m_bStunned )
	{
		CPVSFilter filter( WorldSpaceCenter() );
		if ( RandomInt( 1, 10 ) == 9 )
			PlayLowPrioritySound( filter, "Halloween.MerasmusHitByBombRare" );
		else
			PlayLowPrioritySound( filter, "Halloween.MerasmusHitByBomb" );
	}

	pStunner->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, 10.0f );
	pStunner->m_Shared.AddCond( TF_COND_SPEED_BOOST, 10.0f );
	pStunner->m_Shared.AddCond( TF_COND_INVULNERABLE, 10.0f );

	pStunner->m_Shared.RemoveCond( TF_COND_STUNNED );
	pStunner->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );

	pStunner->BombHeadExplode( false );
	PushPlayer( pStunner, 300.0f );

	DispatchParticleEffect( "merasmus_dazed_explosion", WorldSpaceCenter(), GetAbsAngles() );

	if ( !m_bDoingAOEAttack )
	{
		++m_nStunCount;
		m_stunDuration.Start( tf_merasmus_stun_duration.GetFloat() );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::StartRespawnTimer( void )
{
	Assert( TFGameRules() );

	if ( GetLevel() <= 3 )
	{
		const float fSpawnInterval = tf_merasmus_spawn_interval.GetFloat();
		const float fSpawnVariation = tf_merasmus_spawn_interval_variation.GetFloat();
		TFGameRules()->StartBossTimer( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
	}
	else
	{
		TFGameRules()->StartBossTimer( RandomFloat( 50.0f, 60.0f ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmus::PrecacheMerasmus( void )
{
	int iMdlIndex = PrecacheModel( "models/bots/merasmus/merasmus.mdl" );
	PrecacheGibsForModel( iMdlIndex );

	for ( int i=0; i < ARRAYSIZE( gs_pszDisguiseProps ); ++i )
		PrecacheModel( gs_pszDisguiseProps[ i ] );

	PrecacheModel( "models/props_lakeside_event/bomb_temp.mdl" );
	PrecacheModel( "models/props_lakeside_event/bomb_temp_hat.mdl" );
	PrecacheModel( "models/props_halloween/bombonomicon.mdl" );

	PrecacheScriptSound( "Halloween.MerasmusAppears" );
	PrecacheScriptSound( "Halloween.MerasmusBanish" );
	PrecacheScriptSound( "Halloween.MerasmusCastFireSpell" );
	PrecacheScriptSound( "Halloween.MerasmusCastJarateSpell" );
	PrecacheScriptSound( "Halloween.MerasmusLaunchSpell" );
	PrecacheScriptSound( "Halloween.MerasmusControlPoint" );
	PrecacheScriptSound( "Halloween.MerasmusDepart" );
	PrecacheScriptSound( "Halloween.MerasmusDepartRare" );
	PrecacheScriptSound( "Halloween.MerasmusDiscovered" );
	PrecacheScriptSound( "Halloween.MerasmusGrenadeThrow" );
	PrecacheScriptSound( "Halloween.MerasmusHidden" );
	PrecacheScriptSound( "Halloween.MerasmusHiddenRare" );
	PrecacheScriptSound( "Halloween.MerasmusHitByBomb" );
	PrecacheScriptSound( "Halloween.MerasmusHitByBombRare" );
	PrecacheScriptSound( "Halloween.MerasmusInitiateHiding" );
	PrecacheScriptSound( "Halloween.MerasmusStaffAttack" );
	PrecacheScriptSound( "Halloween.MerasmusStaffAttackRare" );
	PrecacheScriptSound( "Halloween.MerasmusTauntFakeProp" );
	PrecacheScriptSound( "Halloween.HeadlessBossSpawn" );
	PrecacheScriptSound( "Halloween.Merasmus_Death" );
	PrecacheScriptSound( "Halloween.Merasmus_Float" );
	PrecacheScriptSound( "Halloween.Merasmus_Stun" );
	PrecacheScriptSound( "Halloween.Merasmus_Spell" );
	PrecacheScriptSound( "Halloween.Merasmus_Hiding_Explode" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeSoon" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeImminent" );
	PrecacheScriptSound( "Halloween.EyeballBossEscaped" );
	PrecacheScriptSound( "Halloween.HeadlessBossAxeHitFlesh" );

	PrecacheParticleSystem( "merasmus_spawn" );
	PrecacheParticleSystem( "merasmus_tp" );
	PrecacheParticleSystem( "merasmus_blood" );
	PrecacheParticleSystem( "merasmus_ambient_body" );
	PrecacheParticleSystem( "merasmus_shoot" );
	PrecacheParticleSystem( "merasmus_book_attack" );
	PrecacheParticleSystem( "merasmus_object_spawn" );
	PrecacheParticleSystem( "merasmus_zap" );
	PrecacheParticleSystem( "merasmus_dazed" );
	PrecacheParticleSystem( "merasmus_dazed_explosion" );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CMerasmusPathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
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
