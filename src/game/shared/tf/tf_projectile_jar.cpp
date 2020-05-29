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

#ifdef GAME_DLL
ConVar tf2v_use_new_guillotine("tf2v_use_new_guillotine", "0", FCVAR_NOTIFY, "Replaces the Crit and Minicrits for cooldown reduction.");
#endif


#define TF_WEAPON_JAR_MODEL		"models/weapons/c_models/urinejar.mdl"
#define TF_WEAPON_FESTIVE_URINE_MODEL "models/weapons/c_models/c_xms_urinejar.mdl"
#define TF_WEAPON_JARMILK_MODEL "models/weapons/c_models/c_madmilk/c_madmilk.mdl"
#define TF_WEAPON_CLEAVER_MODEL	"models/workshop_partner/weapons/c_models/c_sd_cleaver/c_sd_cleaver.mdl"
#define TF_WEAPON_JARGAS_MODEL "models/weapons/c_models/c_gascan/c_gascan.mdl"

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
	PrecacheScriptSound("Weapon_GasCan.Explode");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Jar::Spawn( void )
{
	switch (GetEffectCondition())
	{
		case TF_COND_URINE:
			if ( TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
				SetModel( TF_WEAPON_FESTIVE_URINE_MODEL );
			else
				SetModel( TF_WEAPON_JAR_MODEL );
			break;
			
		case TF_COND_MAD_MILK:
			SetModel( TF_WEAPON_JARMILK_MODEL );
			break;
			
		case TF_COND_BLEEDING:
			SetModel( TF_WEAPON_CLEAVER_MODEL );
			break;
			
		case TF_COND_GAS:
			SetModel( TF_WEAPON_JARGAS_MODEL );
			break;
		
	}

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
	radiusInfo.m_flSelfDamageRadius = flRadius;

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
	switch (pDeflectedBy->GetTeamNumber())
	{
		case TF_TEAM_RED:
			m_nSkin = 0;
			break;
		case TF_TEAM_BLUE:
			m_nSkin = 1;
			break;
	}
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

IMPLEMENT_NETWORKCLASS_ALIASED(TFProjectile_Cleaver, DT_TFProjectile_Cleaver)

BEGIN_NETWORK_TABLE(CTFProjectile_Cleaver, DT_TFProjectile_Cleaver)
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC(CTFProjectile_Cleaver)
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS(tf_projectile_cleaver, CTFProjectile_Cleaver);
PRECACHE_REGISTER(tf_projectile_cleaver);

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Cleaver::Precache( void )
{
	PrecacheModel( TF_WEAPON_CLEAVER_MODEL );
	PrecacheScriptSound( "Cleaver.ImpactFlesh" );
	PrecacheScriptSound( "Cleaver.ImpactWorld" );

	BaseClass::Precache();
}
#endif

#ifdef GAME_DLL
CTFProjectile_Cleaver *CTFProjectile_Cleaver::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_Cleaver *pCleaver = static_cast<CTFProjectile_Cleaver *>( CBaseEntity::CreateNoSpawn( "tf_projectile_cleaver", vecOrigin, vecAngles, pOwner ) );

	if ( pCleaver )
	{
		// Set scorer.
		pCleaver->SetScorer( pScorer );

		// Set firing weapon.
		pCleaver->SetLauncher( pWeapon );

		DispatchSpawn( pCleaver );

		pCleaver->InitGrenade( vecVelocity, angVelocity, pOwner, weaponInfo );

		pCleaver->ApplyLocalAngularVelocityImpulse( angVelocity );
		
		pCleaver->m_flCreationTime = gpGlobals->curtime;
	}

	return pCleaver;
}

void CTFProjectile_Cleaver::JarTouch( CBaseEntity *pOther )
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

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace.fraction != 1.0)
	{
		SetAbsOrigin( pTrace.endpos + ( pTrace.plane.normal * 1.0f ) );
	}

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer )
	{

		float flAirTime = gpGlobals->curtime - m_flCreationTime;
				
		bool bMiniCrit = false;
		if ( flAirTime >= 1.0 && !tf2v_use_new_guillotine.GetBool() )
		{
				// We get a Minicrit if we've been flying in the air for at least a whole second.
				bMiniCrit = true;
		}
		else if ( flAirTime >= 0.5 && tf2v_use_new_guillotine.GetBool() )
		{
			// Reduce our cooldown a little, if we flew over half a second.
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( m_hLauncher.Get() );
			if (pWeapon)
				pWeapon->SetEffectBarProgress( pWeapon->GetEffectBarProgress() * 0.75 ); // 4.5s / 6s = 75%.
		}
		
		// We get crits if we hit someone stunned.
		bool bCriticalHit = false;
		if ( pPlayer->m_Shared.InCond(TF_COND_STUNNED) && !tf2v_use_new_guillotine.GetBool() )
			bCriticalHit = true;
		
		// Add the crits/minicrits to our damages.
		int iDamageType = GetDamageType();
		if ( bCriticalHit )
		{
			iDamageType |= DMG_CRITICAL;
		}
		else if ( bMiniCrit )
		{
			iDamageType |= DMG_MINICRITICAL;
		}
		
		// We deal with direct contact, do the regular logic.
		CTakeDamageInfo info( this, pAttacker, m_hLauncher.Get(), GetDamage(), iDamageType, (bMiniCrit ? TF_DMG_CUSTOM_CLEAVER_CRIT : TF_DMG_CUSTOM_CLEAVER) );
		Vector vectorReported = pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin ;
		info.SetReportedPosition( vectorReported);
		pOther->TakeDamage( info );
		
		// Also make them bleed too!
		CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase *>(m_hLauncher.Get());
		if (ToTFPlayer(pAttacker) && pTFWeapon)
			pPlayer->m_Shared.MakeBleed(ToTFPlayer(pAttacker), pTFWeapon, 5.0, TF_BLEEDING_DAMAGE);
		
		// Hit player, do impact sound
		CPVSFilter filter( vecOrigin );
		EmitSound( filter, pPlayer->entindex(), "Cleaver.ImpactFlesh" );
		
	}
	else
	{
		// Nothing interesting here.
	
		// Hit something other a player, make a CLANG.
		CPVSFilter filter( vecOrigin );
		EmitSound( filter, pPlayer->entindex(), "Cleaver.ImpactWorld" );
	}

	// Remove.
	UTIL_Remove( this );
}
#endif

//=============================================================================
//
// Weapon JarGas
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_JarGas, DT_TFProjectile_JarGas )

BEGIN_NETWORK_TABLE( CTFProjectile_JarGas, DT_TFProjectile_JarGas )
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_JarGas )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_jar_gas, CTFProjectile_JarGas );
PRECACHE_REGISTER( tf_projectile_jar_gas );

#ifdef GAME_DLL
CTFProjectile_JarGas *CTFProjectile_JarGas::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseCombatCharacter *pOwner, CBaseEntity *pScorer, const AngularImpulse &angVelocity, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_JarGas *pJar = static_cast<CTFProjectile_JarGas *>( CBaseEntity::CreateNoSpawn( "tf_projectile_jar_gas", vecOrigin, vecAngles, pOwner ) );

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
void CTFProjectile_JarGas::Precache( void )
{
	PrecacheModel( TF_WEAPON_JARGAS_MODEL );
	PrecacheParticleSystem( "peejar_impact_gas" );

	BaseClass::Precache();
}
#endif