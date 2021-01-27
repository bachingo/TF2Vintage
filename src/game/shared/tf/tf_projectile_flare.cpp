//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_flare.h"
#include "tf_gamerules.h"
#include "tf_weapon_flaregun.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "soundent.h"
#endif

#define TF_WEAPON_FLARE_MODEL		"models/weapons/w_models/w_flaregun_shell.mdl"
#define TF_FLARE_GRAVITY			0.3f
#define TF_FLARE_SPEED				2000.0f
#define TF_FLARE_DET_RADIUS			110.0f
#define TF_FLARE_DET_RADIUS_OLD		92.0f
#define TF_FLARE_SELF_DAMAGE_RADIUS	100.0f

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_Flare )
	DEFINE_THINKFUNC( ImpactThink ),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_flare, CTFProjectile_Flare );
PRECACHE_REGISTER( tf_projectile_flare );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Flare, DT_TFProjectile_Flare )
BEGIN_NETWORK_TABLE( CTFProjectile_Flare, DT_TFProjectile_Flare )
#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bCritical ) ),
#else
	RecvPropBool( RECVINFO( m_bCritical ) ),
#endif
END_NETWORK_TABLE()

extern ConVar tf_rocket_show_radius;
extern ConVar tf2v_minicrits_on_deflect;

ConVar tf2v_use_new_flare_radius( "tf2v_use_new_flare_radius", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Uses modern flare explosion sizes (110Hu)instead of older ones (92Hu) for exploding flares." );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_Flare::CTFProjectile_Flare()
{
#ifdef GAME_DLL
	m_bTauntShot = false;
	m_bCritical = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_Flare::~CTFProjectile_Flare()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission( m_hEffect );
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Precache()
{
	PrecacheModel( TF_WEAPON_FLARE_MODEL );

	PrecacheTeamParticles( "flaregun_trail_%s", true );
	PrecacheTeamParticles( "flaregun_trail_crit_%s", true );
	PrecacheParticleSystem( "drg_manmelter_projectile" );
	PrecacheTeamParticles( "scorchshot_trail_%s", true );
	PrecacheTeamParticles( "scorchshot_trail_crit_%s", true );
	PrecacheParticleSystem( "Explosions_MA_FlyingEmbers" );

	PrecacheTeamParticles( "pyrovision_flaregun_trail_%s", true );
	PrecacheTeamParticles( "pyrovision_flaregun_trail_crit_%s", true );
	PrecacheTeamParticles( "pyrovision_scorchshot_trail_%s", true );
	PrecacheTeamParticles( "pyrovision_scorchshot_trail_crit_%s", true );

	PrecacheScriptSound( "TFPlayer.FlareImpact" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Spawn()
{
	SetModel( TF_WEAPON_FLARE_MODEL );
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( TF_FLARE_GRAVITY );

	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;
	m_flCreateTime = gpGlobals->curtime;

	// If we made this while taunting, record it as a taunt shot.
	CTFPlayer *pScorer = ToTFPlayer( GetScorer() );
	if ( pScorer && pScorer->IsTaunting() )
	{
		m_bTauntShot = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::SetScorer( CBaseEntity *pScorer )
{
	m_hScorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Flare::GetScorer( void )
{
	return assert_cast<CBasePlayer *>( m_hScorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Flare::GetDamageType()
{
	int iDmgType = BaseClass::GetDamageType();

	// Buff banner mini-crit calculations
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( m_hLauncher.Get() );
	if ( pWeapon )
	{
		pWeapon->CalcIsAttackMiniCritical();
		if ( pWeapon->IsCurrentAttackAMiniCrit() )
		{
			iDmgType |= DMG_MINICRITICAL;
		}
	}

	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}
	if ( ( m_iDeflected > 0 ) && tf2v_minicrits_on_deflect.GetBool() )
	{
		iDmgType |= DMG_MINICRITICAL;
	}

	return iDmgType;
}

bool CTFProjectile_Flare::IsDeflectable( void )
{
	// Don't deflect projectiles with non-deflect attributes.
	if ( m_hLauncher )
	{
		// Check to see if this is a non-deflectable projectile, like an energy projectile.
		int nCannotDeflect = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, nCannotDeflect, energy_weapon_no_deflect );
		if ( nCannotDeflect != 0 )
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	CTFPlayer *pTFDeflector = ToTFPlayer( pDeflectedBy );
	if ( !pTFDeflector )
		return;

	CTFPlayer *pOldOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOldOwner )
		pOldOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );

	// Get rocket's speed.
	float flVel = GetAbsVelocity().Length();

	QAngle angForward;
	VectorAngles( vecDir, angForward );

	// Now change rocket's direction.
	SetAbsAngles( angForward );
	SetAbsVelocity( vecDir * flVel );

	// And change owner.
	SetScorer( pDeflectedBy );
	SetOwnerEntity( pDeflectedBy );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );
	SetLauncher( pTFDeflector->GetActiveWeapon() );

	if ( pTFDeflector->m_Shared.IsCritBoosted() )
		SetCritical( true );

	IncremenentDeflected();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer *>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	int nWeaponMode = TF_FLARE_MODE_NORMAL;
	Vector vecOrigin = GetAbsOrigin();
	CTFPlayer *pTFVictim = ToTFPlayer( pOther );

	CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun *>( m_hLauncher.Get() );
	if ( pFlareGun )
	{
		nWeaponMode = pFlareGun->GetFlareGunMode();

		if ( nWeaponMode == TF_FLARE_MODE_KNOCKBACK )
		{
			if ( pTFVictim )
			{
				SetCollisionGroup( COLLISION_GROUP_DEBRIS );

				CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), GetDamageType()|DMG_PREVENT_PHYSICS_FORCE, m_bTauntShot ? TF_DMG_CUSTOM_FLARE_PELLET : 0 );
				pTFVictim->TakeDamage( info );

				if ( pAttacker && pTFVictim->GetTeamNumber() != pAttacker->GetTeamNumber() )
				{
					// Quick Fix Uber is immune to the push force
					if ( !pTFVictim->m_Shared.InCond( TF_COND_MEGAHEAL ) )
					{
						Vector vecVelocity = GetAbsVelocity();
						VectorNormalize( vecVelocity );
						vecVelocity.z = 1.0;

						if ( !pTFVictim->m_Shared.InCond( TF_COND_KNOCKED_INTO_AIR ) )
						{
							pTFVictim->m_Shared.StunPlayer( 0.5f, 1.0f, 1.0f, TF_STUNFLAG_SLOWDOWN, ToTFPlayer( pAttacker ) );
						}

						const float flForce = pTFVictim->m_Shared.InCond( TF_COND_BURNING ) ? 400.0f : 100.0f;
						pTFVictim->ApplyAirBlastImpulse( vecVelocity * flForce );
					}
				}

				Vector vecVelocity = GetAbsVelocity();
				vecVelocity.x *= 0.07f;
				vecVelocity.y *= 0.07f;
				vecVelocity.z = 100.0f;
				SetAbsVelocity( vecVelocity + RandomVector( -2.0f, 2.0f ) );

				QAngle angForward;
				VectorAngles( vecVelocity, angForward );
				SetAbsAngles( angForward );

				QAngle angRotation = RandomAngle( 180.0f, 720.0f );
				angRotation.x *= RandomInt( -1, 1 );
				angRotation.y *= RandomInt( -1, 1 );
				angRotation.z *= RandomInt( -1, 1 );

				SetLocalAngularVelocity( angRotation );

				CPVSFilter filter( vecOrigin );
				EmitSound( filter, entindex(), "Rubber.BulletImpact" );

				// Save this entity as enemy, they will take 100% damage.
				if ( m_hEnemy == NULL )
					m_hEnemy = pTFVictim;

				return;
			}
		}
	}

	// Only impact once
	if ( m_flImpactTime > 0.0f )
		return;

	bool bWaitToImpact = false;
	if ( pTFVictim == NULL )
		bWaitToImpact = true;

	// Save this entity as enemy, they will take 100% damage.
	if ( m_hEnemy == NULL )
		m_hEnemy = pOther;

	if ( pTFVictim && pTFVictim->m_Shared.InCond( TF_COND_BURNING ) )
	{
		if ( nWeaponMode == TF_FLARE_MODE_NORMAL )
			SetCritical( true );
	}

	// Invisible.
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), GetDamageType(), TF_DMG_CUSTOM_BURNING_FLARE );
	pOther->TakeDamage( info );

	if ( bWaitToImpact )
	{
		SetMoveType( MOVETYPE_FLY );
		SetAbsVelocity( vec3_origin );

		m_vecImpactNormal = pTrace->plane.normal;
		m_flImpactTime = gpGlobals->curtime + 0.1f;

		SetContextThink( &CTFProjectile_Flare::ImpactThink, gpGlobals->curtime, "CTFProjectile_FlareThink" );

		if ( nWeaponMode == TF_FLARE_MODE_DETONATE || nWeaponMode == TF_FLARE_MODE_KNOCKBACK )
			Detonate( nWeaponMode != TF_FLARE_MODE_KNOCKBACK );
	}
	else
	{
		// Impact player sound.
		CPVSFilter filter( vecOrigin );
		EmitSound( filter, pOther->entindex(), "TFPlayer.FlareImpact" );

		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Airburst( trace_t *pTrace, bool bSelfDamage )
{
	// Invisible.
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	CBaseEntity *pAttacker = GetOwnerEntity();
	int nDamageType = GetDamageType();
	Vector vecOrigin = GetAbsOrigin();
	int nItemId = -1;
	WeaponSound_t nSound = SPECIAL1;

	if ( pAttacker )
	{
		CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun *>( m_hLauncher.Get() );
		if ( pFlareGun )
		{
			nItemId = pFlareGun->GetAttributeContainer()->GetItem()->GetItemDefIndex();
		}

		IScorer *pScorerInterface = dynamic_cast<IScorer *>( pAttacker );
		if ( pScorerInterface )
		{
			pAttacker = pScorerInterface->GetScorer();
		}

		const float flRadius = bSelfDamage ? 0.0f : GetFlareRadius();
		if ( bSelfDamage )
		{
			nDamageType |= DMG_BLAST;
			nSound = SPECIAL2;
		}

		if ( tf_rocket_show_radius.GetBool() )
		{
			DrawRadius( flRadius );
		}

		CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), nDamageType|DMG_HALF_FALLOFF, TF_DMG_CUSTOM_FLARE_EXPLOSION );
		CTFRadiusDamageInfo radiusInfo( &info, vecOrigin, flRadius, NULL, TF_FLARE_SELF_DAMAGE_RADIUS );
		TFGameRules()->RadiusDamage( radiusInfo );
	}

	CPVSFilter filter( vecOrigin );
	const char *pszParticle = bSelfDamage ? "Explosions_MA_FlyingEmbers" : "ExplosionCore_MidAir_Flare";
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), entindex(), nItemId, nSound );
	TE_TFParticleEffect( filter, 0.0f, pszParticle, vecOrigin, pTrace->plane.normal, vec3_angle );

	CSoundEnt::InsertSound( SOUND_COMBAT, vecOrigin, 1024, 3.0f );

	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFProjectile_Flare::GetFlareRadius( void ) const
{
	float flRadius = TF_FLARE_DET_RADIUS_OLD;
	if ( tf2v_use_new_flare_radius.GetBool() )
		flRadius = TF_FLARE_DET_RADIUS;

	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flRadius, mult_explosion_radius );

	return flRadius;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFProjectile_Flare::GetProjectileSpeed( void ) const
{
	float flSpeed = TF_FLARE_SPEED;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flSpeed, mult_projectile_speed );
	return flSpeed;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Flare *CTFProjectile_Flare::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_Flare *pFlare = static_cast<CTFProjectile_Flare *>( CBaseEntity::CreateNoSpawn( "tf_projectile_flare", vecOrigin, vecAngles, pOwner ) );

	if ( pFlare )
	{
		// Set team.
		pFlare->ChangeTeam( pOwner->GetTeamNumber() );

		// Set scorer.
		pFlare->SetScorer( pScorer );

		// Set firing weapon.
		pFlare->SetLauncher( pWeapon );

		// Initialize the owner.
		pFlare->SetOwnerEntity( pOwner );

		// Spawn.
		DispatchSpawn( pFlare );

		// Setup the initial velocity.
		Vector vecForward, vecRight, vecUp;
		AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

		Vector vecVelocity = vecForward * pFlare->GetProjectileSpeed();
		pFlare->SetAbsVelocity( vecVelocity );
		pFlare->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		float flGravity = TF_FLARE_GRAVITY;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flGravity, mult_projectile_speed );
		pFlare->SetGravity( flGravity );

		// Setup the initial angles.
		QAngle angles;
		VectorAngles( vecVelocity, angles );
		pFlare->SetAbsAngles( angles );
	}

	return pFlare;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Detonate( bool bSelfDamage )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	Airburst( &tr, bSelfDamage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::UpdateOnRemove( void )
{
	// Tell our launcher that we were removed
	CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun *>( m_hLauncher.Get() );
	if ( pFlareGun )
	{
		pFlareGun->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::ImpactThink( void )
{
	if ( m_flImpactTime < gpGlobals->curtime )
	{
		if ( m_hEnemy.IsValid() )
		{
			Vector vecOrigin = GetAbsOrigin();
			CPVSFilter filter( vecOrigin );
			TE_TFExplosion( filter, 0.0f, vecOrigin, m_vecImpactNormal, GetWeaponID(), m_hEnemy->entindex() );
		}

		UTIL_Remove( this );
	}
	else
	{
		SetContextThink( &CTFProjectile_Flare::ImpactThink, gpGlobals->curtime + 0.1f, "CTFProjectile_FlareThink" );
	}
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CTFFlareGun *pLauncher = dynamic_cast<CTFFlareGun *>( m_hLauncher.Get() );
		if ( pLauncher )
		{
			pLauncher->AddFlare( this );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	if ( m_hEffect )
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hEffect );

	const char *pszEffectName = nullptr;
	CTFFlareGun *pLauncher = dynamic_cast<CTFFlareGun *>( m_hLauncher.Get() );

	int nWeaponMode = TF_FLARE_MODE_NORMAL;
	if ( pLauncher )
		nWeaponMode = pLauncher->GetFlareGunMode();

	if ( nWeaponMode == TF_FLARE_MODE_REVENGE ) // Energy beam
	{
		pszEffectName = "drg_manmelter_projectile";
	}
	else if ( nWeaponMode == TF_FLARE_MODE_KNOCKBACK ) // Scorch Shot
	{
		const char *pszFormat = m_bCritical ? "scorchshot_trail_crit_%s" : "scorchshot_trail_%s";
		pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber(), false );
	}
	else // Standard flare
	{
		const char *pszFormat = m_bCritical ? "flaregun_trail_crit_%s" : "flaregun_trail_%s";
		pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber(), false );
	}

	m_hEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}

#endif