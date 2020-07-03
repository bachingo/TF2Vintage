#include "cbase.h"
#include "tf_projectile_stunball.h"
#include "tf_gamerules.h"
#include "effect_dispatch_data.h"
#include "tf_weapon_bat.h"

#ifdef GAME_DLL
#include "tf_fx.h"
#include "tf_gamestats.h"
#else
#include "particles_new.h"
#endif


#define TF_STUNBALL_MODEL	  "models/weapons/w_models/w_baseball.mdl"
#define TF_STUNBALL_LIFETIME  15.0f
#define TF_BAUBLE_MODEL		  "models/weapons/c_models/c_xms_festive_ornament.mdl"

ConVar tf_scout_stunball_base_duration( "tf_scout_stunball_base_duration", "6.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Modifies stun duration of stunball" );

extern ConVar tf2v_minicrits_on_deflect;

ConVar tf2v_sandman_stun_type( "tf2v_sandman_stun_type", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Modifies Sandman stun types.", true, 0, true, 2 );

IMPLEMENT_NETWORKCLASS_ALIASED( TFStunBall, DT_TFStunBall )

BEGIN_NETWORK_TABLE( CTFStunBall, DT_TFStunBall )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCritical ) ),
#else
	SendPropBool( SENDINFO( m_bCritical ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTFStunBall )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_stunball, CTFStunBall );
PRECACHE_REGISTER( tf_projectile_stunball );

CTFStunBall::CTFStunBall()
{
}

CTFStunBall::~CTFStunBall()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef GAME_DLL
CTFStunBall *CTFStunBall::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo )
{
	CTFStunBall *pStunBall = static_cast<CTFStunBall *>( CBaseEntity::CreateNoSpawn( "tf_projectile_stunball", vecOrigin, vecAngles, pOwner ) );

	if ( pStunBall )
	{
		// Set scorer.
		pStunBall->SetScorer( pScorer );

		// Set firing weapon.
		pStunBall->SetLauncher( pWeapon );

		DispatchSpawn( pStunBall );

		pStunBall->InitGrenade( vecVelocity, angVelocity, pOwner, weaponInfo );

		pStunBall->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pStunBall;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStunBall::Precache( void )
{
	PrecacheModel( TF_STUNBALL_MODEL );
	PrecacheScriptSound( "TFPlayer.StunImpactRange" );
	PrecacheScriptSound( "TFPlayer.StunImpact" );

	PrecacheTeamParticles( "stunballtrail_%s", false );
	PrecacheTeamParticles( "stunballtrail_%s_crit", false );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStunBall::Spawn( void )
{
	SetModel( TF_STUNBALL_MODEL );
	SetDetonateTimerLength( TF_STUNBALL_LIFETIME );

	BaseClass::Spawn();
	SetTouch( &CTFStunBall::StunBallTouch );

	CreateTrail();

	// Pumpkin Bombs
	AddFlag( FL_GRENADE );

	// Don't collide with anything for a short time so that we never get stuck behind surfaces
	SetCollisionGroup( TFCOLLISION_GROUP_NONE );

	AddSolidFlags( FSOLID_TRIGGER );

	m_flCreationTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::Explode( trace_t *pTrace, int bitsDamageType )
{
	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( m_hLauncher.Get() );

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Damage.
	CTFPlayer *pAttacker = dynamic_cast< CTFPlayer * >( GetThrower() );
	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer * >( m_hEnemy.Get() );

	// Make sure the player is stunnable
	if ( pPlayer && pAttacker && CanStun( pPlayer ) )
	{
		float flAirTime = gpGlobals->curtime - m_flCreationTime;
		float flStunDuration = tf_scout_stunball_base_duration.GetFloat();
		Vector vecDir = GetAbsOrigin();
		VectorNormalize( vecDir );
		bool bIsMoonShot = ( tf2v_sandman_stun_type.GetInt() == 2 )  ? flAirTime >= 1.0f : flAirTime >= 0.8f;
		

		int iDamageAmt = 15;	// Balls do a fixed damage amount.
		if (tf2v_sandman_stun_type.GetInt() == 2 && bIsMoonShot)	// New Moonshot type increases damage by 50%.
			iDamageAmt *= 1.5;
		
		// Do damage.
		CTakeDamageInfo info( this, pAttacker, pWeapon, iDamageAmt, GetDamageType(), TF_DMG_CUSTOM_BASEBALL );
		CalculateBulletDamageForce( &info, pWeapon ? pWeapon->GetTFWpnData().iAmmoType : 0, vecDir, GetAbsOrigin() );
		info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
		pPlayer->DispatchTraceAttack( info, vecDir, pTrace );
		ApplyMultiDamage();

		if ( flAirTime > 0.1f )
		{

			int iBonus = 1;
			switch (tf2v_sandman_stun_type.GetInt())
			{
			case 0:							// Old, broken stun that full stunned everything including Ubers.
				if (bIsMoonShot)
				{
					// Maximum stun is base duration + 1 second
					flAirTime = 1.0f;
					flStunDuration += 1.0f;

					// 2 points for moonshots
					iBonus++;

					// Big stun
					pPlayer->m_Shared.StunPlayer(flStunDuration * (flAirTime), 0.0f, 1.0f, TF_STUNFLAGS_BIGBONK, pAttacker);
				}
				else
				{
					// Big Stun (but no crowd cheer)
					pPlayer->m_Shared.StunPlayer(flStunDuration * (flAirTime), 0.0f, 1.0f, TF_STUNFLAG_BONKSTUCK, pAttacker);
				}
				break;
			case 1:							// The middle of the road version, featuring full stuns on Moonshots and loser state on everything else.
				if (bIsMoonShot)
				{
					// Maximum stun is base duration + 1 second
					flAirTime = 1.0f;
					flStunDuration += 1.0f;

					// 2 points for moonshots
					iBonus++;

					// Big stun
					pPlayer->m_Shared.StunPlayer(flStunDuration * (flAirTime), 0.0f, 0.75f, TF_STUNFLAGS_BIGBONK, pAttacker);
				}
				else
				{
					// Small stun
					pPlayer->m_Shared.StunPlayer(flStunDuration * (flAirTime), 0.8f, 0.0f, TF_STUNFLAGS_SMALLBONK, pAttacker);
				}
				break;
			case 2:							// Modern variant that only slows down targets.
				if (bIsMoonShot)
				{
					// Maximum stun is base duration + 1 second
					flAirTime = 1.0f;
					flStunDuration += 1.0f;

					// 2 points for moonshots
					iBonus++;

				}
				// Slowdown only.
				pPlayer->m_Shared.StunPlayer(flStunDuration * (flAirTime), 0.4f, 0.0f, TF_STUNFLAG_SLOWDOWN, pAttacker);
				break;
			}


			pAttacker->SpeakConceptIfAllowed( MP_CONCEPT_STUNNED_TARGET );

			// Bonus points.
			IGameEvent *event_bonus = gameeventmanager->CreateEvent( "player_bonuspoints" );
			if ( event_bonus )
			{
				event_bonus->SetInt( "player_entindex", pPlayer->entindex() );
				event_bonus->SetInt( "source_entindex", pAttacker->entindex() );
				event_bonus->SetInt( "points", iBonus );

				gameeventmanager->FireEvent( event_bonus );
			}
			CTF_GameStats.Event_PlayerAwardBonusPoints( pAttacker, pPlayer, iBonus );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::StunBallTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer * >( pOther );

	// Make us solid once we reach our owner
	if ( GetCollisionGroup() == TFCOLLISION_GROUP_NONE )
	{
		if ( pOther == GetThrower() )
			SetCollisionGroup( COLLISION_GROUP_PROJECTILE );

		return;
	}

	// Stun the person we hit
	if ( pPlayer && ( gpGlobals->curtime - m_flCreationTime > 0.2f || GetTeamNumber() != pPlayer->GetTeamNumber() ) )
	{
		if ( !m_bTouched )
		{
			// Save who we hit for calculations
			m_hEnemy = pOther;
			m_hSpriteTrail->SUB_FadeOut();
			Explode( &pTrace, GetDamageType() );

			// Die a little bit after the hit
			SetDetonateTimerLength( 3.0f );
			m_bTouched = true;
		}

		CTFWeaponBase *pWeapon = pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_MELEE );
		if ( pWeapon && pWeapon->PickedUpBall( pPlayer ) )
		{
			UTIL_Remove( this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if this player can be stunned
//-----------------------------------------------------------------------------
bool CTFStunBall::CanStun( CTFPlayer *pOther )
{
	// Dead players can't be stunned
	if ( !pOther->IsAlive() )
		return false;

	// Don't stun team members
	if ( GetTeamNumber() == pOther->GetTeamNumber() )
		return false;

	// Don't stun players we can't damage
	if ( ( pOther->m_Shared.InCond( TF_COND_INVULNERABLE ) && tf2v_sandman_stun_type.GetInt() != 0 ) || pOther->m_Shared.InCond( TF_COND_PHASE ) )
		return false;
	
	// Don't stun players with megaheal.
	if ( pOther->m_Shared.InCond( TF_COND_MEGAHEAL ) )
		return false;
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::Detonate()
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
	{
		return;
	}

	// we've touched a surface
	m_bTouched = true;
	
	// Handle hitting skybox (disappear).
	surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	if ( pprops->game.material == 'X' )
	{
		// uncomment to destroy ball upon hitting sky brush
		//SetThink( &CTFGrenadePipebombProjectile::SUB_Remove );
		//SetNextThink( gpGlobals->curtime );
		return;
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStunBall::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFStunBall::GetAssistant( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

bool CTFStunBall::IsDeflectable(void)
{
	// Don't deflect projectiles with non-deflect attributes.
	if (m_hLauncher.Get())
	{
		// Check to see if this is a non-deflectable projectile, like an energy projectile.
		int nCannotDeflect = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(m_hLauncher.Get(), nCannotDeflect, energy_weapon_no_deflect);
		if (nCannotDeflect != 0)
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		Vector vecOldVelocity, vecVelocity;

		pPhysicsObject->GetVelocity( &vecOldVelocity, NULL );

		float flVel = vecOldVelocity.Length();

		vecVelocity = vecDir;
		vecVelocity *= flVel;
		AngularImpulse angVelocity( ( 600, random->RandomInt( -1200, 1200 ), 0 ) );

		// Now change grenade's direction.
		pPhysicsObject->SetVelocityInstantaneous( &vecVelocity, &angVelocity );
	}

	CBaseCombatCharacter *pBCC = pDeflectedBy->MyCombatCharacterPointer();

	IncremenentDeflected();
	m_hDeflectOwner = pDeflectedBy;
	SetThrower( pBCC );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );

	// Change trail color.
	if ( m_hSpriteTrail.Get() )
	{
		UTIL_Remove( m_hSpriteTrail.Get() );
	}

	CreateTrail();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFStunBall::GetDamageType()
{
	int iDmgType = BaseClass::GetDamageType();

	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}
	if ( ( m_iDeflected > 0 ) && ( tf2v_minicrits_on_deflect.GetBool() ) )
	{
		iDmgType |= DMG_MINICRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFStunBall::GetTrailParticleName( void )
{
	return ConstructTeamParticle( "effects/baseballtrail_%s.vmt", GetTeamNumber(), false, g_aTeamNamesShort );
}

// ----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::CreateTrail( void )
{
	CSpriteTrail *pTrail = CSpriteTrail::SpriteTrailCreate( GetTrailParticleName(), GetAbsOrigin(), true );

	if ( pTrail )
	{
		pTrail->FollowEntity( this );
		pTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 128, kRenderFxNone );
		pTrail->SetStartWidth( 9.0f );
		pTrail->SetTextureResolution( 0.01f );
		pTrail->SetLifeTime( 0.4f );
		pTrail->TurnOn();

		pTrail->SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 5.0f, "RemoveThink" );

		m_hSpriteTrail.Set( pTrail );
	}
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flCreationTime = gpGlobals->curtime;
		CreateTrails();
	}

	if ( m_iOldTeamNum && m_iOldTeamNum != m_iTeamNum )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStunBall::CreateTrails( void )
{
	if ( IsDormant() )
	return;

	if ( m_bCritical )
	{
		const char *pszEffectName = ConstructTeamParticle( "stunballtrail_%s_crit", GetTeamNumber(), false );
		ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
	else
	{
		const char *pszEffectName = ConstructTeamParticle( "stunballtrail_%s", GetTeamNumber(), false );
		ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
//-----------------------------------------------------------------------------
int CTFStunBall::DrawModel( int flags )
{
	if ( gpGlobals->curtime - m_flCreationTime < 0.1f )
		return 0;

	return BaseClass::DrawModel( flags );
}
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFBauble, DT_TFBauble )

BEGIN_NETWORK_TABLE( CTFBauble, DT_TFBauble )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCritical ) ),
#else
	SendPropBool( SENDINFO( m_bCritical ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTFBauble )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_bauble, CTFBauble );
PRECACHE_REGISTER( tf_projectile_bauble );
#ifdef GAME_DLL
CTFBauble *CTFBauble::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo )
{
	CTFBauble *pBauble = static_cast<CTFBauble *>( CBaseEntity::CreateNoSpawn( "tf_projectile_bauble", vecOrigin, vecAngles, pOwner ) );

	if ( pBauble )
	{
		// Set scorer.
		pBauble->SetScorer( pScorer );

		// Set firing weapon.
		pBauble->SetLauncher( pWeapon );

		DispatchSpawn( pBauble );

		pBauble->InitGrenade( vecVelocity, angVelocity, pOwner, weaponInfo );

		pBauble->ApplyLocalAngularVelocityImpulse( angVelocity );

	}

	return pBauble;
}

//-----------------------------------------------------------------------------
void CTFBauble::Precache( void )
{
	PrecacheModel( TF_BAUBLE_MODEL );
	PrecacheScriptSound( "BallBuster.OrnamentImpactRange" );
	PrecacheScriptSound( "BallBuster.OrnamentImpact" );
	PrecacheParticleSystem( "xms_ornament_glitter" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBauble::Spawn( void )
{
	SetModel( TF_BAUBLE_MODEL );
	SetDetonateTimerLength( TF_STUNBALL_LIFETIME );

	BaseClass::Spawn();
	SetTouch( &CTFBauble::BaubleTouch );

	CreateTrail();

	// Pumpkin Bombs
	AddFlag( FL_GRENADE );
	
	// Set the team skin.
	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			m_nSkin = 0;
			break;
		case TF_TEAM_BLUE:
			m_nSkin = 1;
			break;
		default:
			m_nSkin = 0;
			break;
	}

	// Don't collide with anything for a short time so that we never get stuck behind surfaces
	SetCollisionGroup( TFCOLLISION_GROUP_NONE );

	AddSolidFlags( FSOLID_TRIGGER );

	m_flCreationTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBauble::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	// Same as base, except we swap bauble colors. (Airblast magic!)
	BaseClass::Deflected( pDeflectedBy, vecDir );
	m_nSkin = ( pDeflectedBy->GetTeamNumber() - 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBauble::BaubleTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer * >( pOther );

	// Make us solid once we reach our owner
	if ( GetCollisionGroup() == TFCOLLISION_GROUP_NONE )
	{
		if ( pOther == GetThrower() )
			SetCollisionGroup( COLLISION_GROUP_PROJECTILE );

		return;
	}

	// Do direct hit and bleed damage to an enemy we hit.
	if ( pPlayer && ( ( gpGlobals->curtime - m_flCreationTime > 0.2f ) || ( GetTeamNumber() != pPlayer->GetTeamNumber() ) ) )
	{
		// Save who we hit for calculations
		m_hEnemy = pOther;
		m_hSpriteTrail->SUB_FadeOut();
		Explode( &pTrace, GetDamageType() );
		m_bTouched = true;
	}

	// Once we touched something (player/world) shatter the bauble.
	Shatter( &pTrace, GetDamageType() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBauble::Explode( trace_t *pTrace, int bitsDamageType )
{
	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( m_hLauncher.Get() );

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Damage.
	CTFPlayer *pAttacker = dynamic_cast< CTFPlayer * >( GetThrower() );
	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer * >( m_hEnemy.Get() );

	// Apply damage+bleed.
	if ( pPlayer && pAttacker )
	{
		float flAirTime = gpGlobals->curtime - m_flCreationTime;
		Vector vecDir = GetAbsOrigin();
		VectorNormalize( vecDir );
		
		// Fly longer than a second? Guaranteed critical.
		bool bCriticalHit = false;
		if (gpGlobals->curtime - flAirTime >= 1.0)
			bCriticalHit = true;

		int nDamageType = GetDamageType();
		if ( bCriticalHit )
		{
			nDamageType |= DMG_CRITICAL;
		}
		
		// Do damage.
		CTakeDamageInfo info( this, pAttacker, pWeapon, GetDamage(), nDamageType, TF_DMG_CUSTOM_BASEBALL );
		CalculateBulletDamageForce( &info, pWeapon ? pWeapon->GetTFWpnData().iAmmoType : 0, vecDir, GetAbsOrigin() );
		info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
		pPlayer->DispatchTraceAttack( info, vecDir, pTrace );
		ApplyMultiDamage();
		
		// Also make them bleed too!
		CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase *>(m_hLauncher.Get());
		if (ToTFPlayer(pAttacker) && pTFWeapon)
			pPlayer->m_Shared.MakeBleed(ToTFPlayer(pAttacker), pTFWeapon, 5.0, TF_BLEEDING_DAMAGE);
		
		// Ornament does a unique snowflake effect around players.
		DispatchParticleEffect( "xms_ornament_glitter", PATTACH_POINT_FOLLOW, pPlayer, "head" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBauble::Shatter( trace_t *pTrace, int bitsDamageType )
{
	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Damage.
	CTFPlayer *pAttacker = dynamic_cast< CTFPlayer * >( GetThrower() );

	// Add a small explosion.
	if ( pAttacker && pAttacker->GetTeamNumber() == TF_TEAM_RED )
	{
		DispatchParticleEffect( "xms_ornament_smash_red", GetAbsOrigin(), vec3_angle );
	}
	else
	{
		DispatchParticleEffect( "xms_ornament_smash_blue", GetAbsOrigin(), vec3_angle );
	}
	// Add the glass breaking sound.
	EmitSound( "BallBuster.OrnamentImpact" );
	
	int nDamage = (GetDamage() * 0.9);
	
	// We explode in a small radius, set us up as explosive damage.
	Vector vecOrigin = GetAbsOrigin();
	Vector vectorReported = pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin;
	CTakeDamageInfo newInfo(this, pAttacker, m_hLauncher.Get(), vec3_origin, vecOrigin, nDamage, GetDamageType(), TF_DMG_CUSTOM_BASEBALL, &vectorReported);
	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info = &newInfo;
	radiusInfo.m_vecSrc = vecOrigin;

	// Check the radius.
	float flRadius = 50;
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_flSelfDamageRadius = flRadius;

	TFGameRules()->RadiusDamage(radiusInfo);

	// Remove.
	UTIL_Remove(this);
}


#endif