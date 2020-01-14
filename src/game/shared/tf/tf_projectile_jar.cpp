#include "cbase.h"
#include "tf_projectile_jar.h"
#include "tf_gamerules.h"
#include "effect_dispatch_data.h"
#include "tf_weapon_jar.h"

#ifdef GAME_DLL
#include "tf_fx.h"
#else
#include "particles_new.h"
#endif


#define TF_WEAPON_JAR_MODEL		"models/weapons/c_models/urinejar.mdl"
#define TF_WEAPON_FESTIVE_URINE_MODEL "models/weapons/c_models/c_xms_urinejar.mdl"
#define TF_WEAPON_JARMILK_MODEL "models/weapons/c_models/c_madmilk/c_madmilk.mdl"
#define TF_WEAPON_JAR_LIFETIME  2.0f

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Jar, DT_TFProjectile_Jar )

BEGIN_NETWORK_TABLE( CTFProjectile_Jar, DT_TFProjectile_Jar )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropInt( RECVINFO( m_nSkin ) ),
#else
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropInt( SENDINFO( m_nSkin ), 0, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_Jar )
END_DATADESC()
#endif

ConVar tf_jar_show_radius( "tf_jar_show_radius", "0", FCVAR_REPLICATED | FCVAR_CHEAT /*| FCVAR_DEVELOPMENTONLY*/, "Render jar radius." );

ConVar tf2v_use_extinguish_cooldown( "tf2v_use_extinguish_cooldown", "0", FCVAR_REPLICATED, "Enables -20% cooldown for extinguishing a teammate with a jar." );

LINK_ENTITY_TO_CLASS( tf_projectile_jar, CTFProjectile_Jar );
PRECACHE_REGISTER( tf_projectile_jar );

CTFProjectile_Jar::CTFProjectile_Jar()
{
}

CTFProjectile_Jar::~CTFProjectile_Jar()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef GAME_DLL
CTFProjectile_Jar *CTFProjectile_Jar::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_Jar *pJar = static_cast<CTFProjectile_Jar *>( CBaseEntity::CreateNoSpawn( "tf_projectile_jar", vecOrigin, vecAngles, pOwner ) );

	if ( pJar )
	{
		// Set scorer.
		pJar->SetScorer( pScorer );

		// Set firing weapon.
		pJar->SetLauncher( pWeapon );

		DispatchSpawn( pJar );

		pJar->InitGrenade( vecVelocity, angVelocity, pOwner, weaponInfo );

		pJar->ApplyLocalAngularVelocityImpulse( angVelocity );
		
		if ( TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
		{
			switch (pOwner->GetTeamNumber())
			{
			case TF_TEAM_RED:
				pJar->m_nSkin = 0;
				break;
			case TF_TEAM_BLUE:
				pJar->m_nSkin = 1;
				break;
			}
		}
	}

	return pJar;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::Precache( void )
{
	PrecacheModel( TF_WEAPON_JAR_MODEL );
	PrecacheModel( TF_WEAPON_FESTIVE_URINE_MODEL );

	PrecacheTeamParticles( "peejar_trail_%s", false, g_aTeamNamesShort );
	PrecacheParticleSystem( "peejar_impact" );

	PrecacheScriptSound( "Jar.Explode" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::Spawn( void )
{
	if ( GetEffectCondition() != TF_COND_URINE )
		SetModel( TF_WEAPON_JARMILK_MODEL );
	else 
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
			SetModel( TF_WEAPON_FESTIVE_URINE_MODEL );
		else
			SetModel( TF_WEAPON_JAR_MODEL );
	}
	
	SetDetonateTimerLength( TF_WEAPON_JAR_LIFETIME );

	BaseClass::Spawn();
	SetTouch( &CTFProjectile_Jar::JarTouch );

	// Pumpkin Bombs
	AddFlag( FL_GRENADE );

	// Don't collide with anything for a short time so that we never get stuck behind surfaces
	SetCollisionGroup( TFCOLLISION_GROUP_NONE );

	AddSolidFlags( FSOLID_TRIGGER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::Explode( trace_t *pTrace, int bitsDamageType )
{
	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( m_hLauncher.Get() );

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	//EmitSound( "Jar.Explode" );
	CPVSFilter filter( vecOrigin );
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), -1 );

	// Damage.
	CBaseEntity *pAttacker = NULL;
	if( pWeapon )
		pAttacker = pWeapon->GetOwnerEntity();

	float flRadius = GetDamageRadius();

	CTakeDamageInfo newInfo( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), GetDamageType() );
	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info = &newInfo;
	radiusInfo.m_vecSrc = vecOrigin;
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_flSelfDamageRadius = 121.0f; // Original rocket radius?

	// If we extinguish a friendly player reduce our recharge time by 20%
	if ( TFGameRules()->RadiusJarEffect( radiusInfo, GetEffectCondition() ) && m_iDeflected == 0 && pWeapon ) 
	{
		float flCooldownReduction = 1.0f;
		
		if ( tf2v_use_extinguish_cooldown.GetBool() )
			flCooldownReduction *= 0.8f;
		
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flCooldownReduction, "extinguish_reduces_cooldown" );
		pWeapon->SetEffectBarProgress( pWeapon->GetEffectBarProgress() * flCooldownReduction );
	}

	// Debug!
	if ( tf_jar_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );
	
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::JarTouch( CBaseEntity *pOther )
{
	if ( pOther == GetThrower() )
	{
		// Make us solid if we're not already
		if ( GetCollisionGroup() == TFCOLLISION_GROUP_NONE )
		{
			SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
		}
		return;
	}

	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// Blow up if we hit a player
	if ( pOther->IsPlayer() )
	{
		Explode( &pTrace, GetDamageType() );
	}
	// We should bounce off of certain surfaces (resupply cabinets, spawn doors, etc.)
	else
	{
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::Detonate()
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Explode( &tr, GetDamageType() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
	{
		return;
	}

	// TODO: This needs to be redone properly
	/*if ( pHitEntity->GetMoveType() == MOVETYPE_NONE )
	{
		// Blow up
		SetThink( &CTFProjectile_Jar::Detonate );
		SetNextThink( gpGlobals->curtime );
	}*/

		// Blow up
		SetThink( &CTFProjectile_Jar::Detonate );
		SetNextThink( gpGlobals->curtime );

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Jar::GetAssistant( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

bool CTFProjectile_Jar::IsDeflectable(void)
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
void CTFProjectile_Jar::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	/*// Get jar's speed.
	float flVel = GetAbsVelocity().Length();

	QAngle angForward;
	VectorAngles( vecDir, angForward );
	AngularImpulse angVelocity( ( 600, random->RandomInt( -1200, 1200 ), 0 ) );

	// Now change jar's direction.
	SetAbsAngles( angForward );
	SetAbsVelocity( vecDir * flVel );

	// Bounce it up a bit
	ApplyLocalAngularVelocityImpulse( angVelocity );

	IncremenentDeflected();
	SetOwnerEntity( pDeflectedBy );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );
	SetScorer( pDeflectedBy );*/

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
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Jar::OnDataChanged( DataUpdateType_t updateType )
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
void C_TFProjectile_Jar::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	const char *pszEffectName = ConstructTeamParticle( "peejar_trail_%s", GetTeamNumber(), false, g_aTeamNamesShort );

	ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
//-----------------------------------------------------------------------------
int CTFProjectile_Jar::DrawModel( int flags )
{
	if ( gpGlobals->curtime - m_flCreationTime < 0.1f )
		return 0;

	return BaseClass::DrawModel( flags );
}
#endif

//=============================================================================
//
// Weapon JarMilk
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_JarMilk, DT_TFProjectile_JarMilk )

BEGIN_NETWORK_TABLE( CTFProjectile_JarMilk, DT_TFProjectile_JarMilk )
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_JarMilk )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_jar_milk, CTFProjectile_JarMilk );
PRECACHE_REGISTER( tf_projectile_jar_milk );

#ifdef GAME_DLL
CTFProjectile_JarMilk *CTFProjectile_JarMilk::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_JarMilk *pJar = static_cast<CTFProjectile_JarMilk *>( CBaseEntity::CreateNoSpawn( "tf_projectile_jar_milk", vecOrigin, vecAngles, pOwner ) );

	if ( pJar )
	{
		// Set scorer.
		pJar->SetScorer( pScorer );

		// Set firing weapon.
		pJar->SetLauncher( pWeapon );

		DispatchSpawn( pJar );

		pJar->InitGrenade( vecVelocity, angVelocity, pOwner, weaponInfo );

		pJar->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pJar;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarMilk::Precache( void )
{
	PrecacheModel( TF_WEAPON_JARMILK_MODEL );
	PrecacheParticleSystem( "peejar_impact_milk" );

	BaseClass::Precache();
}
#endif