//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "world.h"
#include "tf_generic_bomb.h"
#include "tf_gamerules.h"
#include "te_particlesystem.h"
#include "particle_parse.h"
#include "tf_fx.h"
#include "tf_player.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weaponbase_grenadeproj.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_generic_bomb, CTFGenericBomb );

BEGIN_DATADESC( CTFGenericBomb )	
	DEFINE_KEYFIELD( m_flDamage,		FIELD_FLOAT,		"damage" ),
	DEFINE_KEYFIELD( m_flRadius,		FIELD_FLOAT,		"radius" ),
	DEFINE_KEYFIELD( m_iHealth,			FIELD_INTEGER,		"health" ),
	DEFINE_KEYFIELD( m_iszParticleName,	FIELD_STRING,		"explode_particle"),
	DEFINE_KEYFIELD( m_iszExplodeSound,	FIELD_SOUNDNAME,	"sound" ),
	DEFINE_KEYFIELD( m_bFriendlyFire,	FIELD_INTEGER,		"friendlyfire" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Detonate", InputDetonate ),
	DEFINE_OUTPUT( m_OnDetonate, "OnDetonate" ),
END_DATADESC()


CTFGenericBomb::CTFGenericBomb()
{
	SetMaxHealth( 1 );
	SetHealth( 1 );
	m_flDamage = 50.0f;
	m_flRadius = 100.0f;
	m_bFriendlyFire = false;
}

void CTFGenericBomb::Precache()
{
	PrecacheParticleSystem( STRING( m_iszParticleName ) );
	PrecacheScriptSound( STRING( m_iszExplodeSound ) );
	BaseClass::Precache();
}

void CTFGenericBomb::Spawn()
{
	BaseClass::Spawn();

	// TODO: Implement team pumpkin bombs for spells
	if ( 1 )
	{
		// Normal bombs
		VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	}
	else
	{
		// Team bombs
		SetCollisionGroup( TFCOLLISION_GROUP_PUMPKIN_BOMB );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
		SetSolid( SOLID_BBOX );

		m_flRadius = 200.0f;
		m_flDamage = 80.0f;	
	}

	char *szModel = (char *)STRING( GetModelName() );
	if ( !szModel || !*szModel )
	{
		Warning( "prop at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	int index = PrecacheModel( szModel );
	PrecacheGibsForModel( index );
	Precache();
	SetModel( szModel );

	m_takedamage = DAMAGE_YES;
	SetTouch( &CTFGenericBomb::BombTouch );
}

void CTFGenericBomb::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;

	Vector absOrigin = WorldSpaceCenter();
	QAngle absAngles = GetAbsAngles();

	trace_t	tr;
	Vector vecForward = GetAbsVelocity();
	VectorNormalize( vecForward );
	UTIL_TraceLine ( absOrigin, absOrigin + 60*vecForward , MASK_SHOT, 
		this, COLLISION_GROUP_NONE, &tr);

#if 0
	int iAttachment = LookupAttachment("alt-origin");

	if ( iAttachment > 0 )
		GetAttachment( iAttachment, absOrigin, absAngles );
#endif

	CPVSFilter filter( GetAbsOrigin() );

	if ( STRING( m_iszParticleName ) )
		TE_TFParticleEffect( filter, 0.0, STRING( m_iszParticleName ), GetAbsOrigin(), GetAbsAngles(), NULL, PATTACH_CUSTOMORIGIN );
	if ( STRING( m_iszExplodeSound ) )
		EmitSound( STRING( m_iszExplodeSound ) );

	SetSolid( SOLID_NONE ); 

	CBaseEntity *pAttacker = this;

	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
		pAttacker = info.GetAttacker();

	CTakeDamageInfo info_modified( this, pAttacker, m_flDamage, DMG_BLAST, GetCustomDamageType() );

	if ( m_bFriendlyFire )
		info_modified.SetForceFriendlyFire( true );

	TFGameRules()->RadiusDamage( info_modified, absOrigin, m_flRadius, CLASS_NONE, this );

	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch");

	// FIXME: Gibs are causing crashes on some servers for unknown reasons

	/*UserMessageBegin( filter, "BreakModel" );
	WRITE_SHORT( GetModelIndex() );
	WRITE_VEC3COORD( absOrigin );
	WRITE_ANGLES( absAngles );
	MessageEnd();*/

	m_OnDetonate.FireOutput( this, this, 0.0f );
	BaseClass::Event_Killed( info );
}

void CTFGenericBomb::InputDetonate( inputdata_t &inputdata )
{
	m_takedamage = DAMAGE_NO;

	Vector absOrigin = GetAbsOrigin();
	QAngle absAngles = GetAbsAngles();

	// Trace used for ground scortching
	trace_t	tr;
	Vector vecForward = GetAbsVelocity();
	VectorNormalize( vecForward );
	UTIL_TraceLine ( absOrigin, absOrigin + 60*vecForward , MASK_SHOT, 
		this, COLLISION_GROUP_NONE, &tr);

	// Do explosion effects
	CPVSFilter filter( GetAbsOrigin() );
	if ( STRING( m_iszParticleName ) )
		TE_TFParticleEffect( filter, 0.0, STRING( m_iszParticleName ), GetAbsOrigin(), GetAbsAngles(), NULL, PATTACH_CUSTOMORIGIN );
	if ( STRING( m_iszExplodeSound ) )
		EmitSound( STRING( m_iszExplodeSound ) );

	SetSolid( SOLID_NONE ); 

	CBaseEntity *pAttacker = this;

	if ( inputdata.pActivator && inputdata.pActivator->IsPlayer() )
		pAttacker = inputdata.pActivator;

	CTakeDamageInfo info_modified( this, pAttacker, m_flDamage, DMG_BLAST );

	if ( m_bFriendlyFire )
		info_modified.SetForceFriendlyFire( true );

	TFGameRules()->RadiusDamage( info_modified, absOrigin, m_flRadius, CLASS_NONE, this );

	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch");

	// FIXME: Gibs are causing crashes on some servers for unknown reasons

	/*UserMessageBegin( filter, "BreakModel" );
	WRITE_SHORT( GetModelIndex() );
	WRITE_VEC3COORD( absOrigin );
	WRITE_ANGLES( absAngles );
	MessageEnd();*/
}

void CTFGenericBomb::BombTouch( CBaseEntity *pOther )
{
	// Make sure this a team fortress projectile
	if ( !pOther || !( pOther->GetFlags() & FL_GRENADE ) )
	{
		return;
	}

	CTFPlayer *pPlayer = NULL;
	CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>( pOther );
	if ( pGrenade )
	{
		pPlayer = ToTFPlayer( pGrenade->GetThrower() );
		pGrenade->ExplodeTouch( this );
	}
	
	CTFBaseRocket *pRocket = dynamic_cast<CTFBaseRocket *>( pOther );
	if ( pRocket )
	{
		pPlayer = ToTFPlayer( pRocket->GetOwnerEntity() );
		pRocket->RocketTouch( this );
	}
    
	if ( !pGrenade && !pRocket )
	{
		CBaseProjectile *pProj = dynamic_cast<CBaseProjectile *>( pOther );
		if ( pProj )
		{
			pPlayer = ToTFPlayer( pProj->GetOwnerEntity() );
		}
	}

	CTakeDamageInfo info( pOther, pPlayer, 10.0f, DMG_GENERIC );
	TakeDamage( info );
}

class CTFPumpkinBomb : public CTFGenericBomb
{
public:
	DECLARE_CLASS( CTFPumpkinBomb, CTFGenericBomb );

	virtual void	Spawn( void );

	virtual int		GetCustomDamageType( void ) { return TF_DMG_CUSTOM_PUMPKIN_BOMB; }

};

LINK_ENTITY_TO_CLASS( tf_pumpkin_bomb, CTFPumpkinBomb );

void CTFPumpkinBomb::Spawn( void )
{
	m_iszExplodeSound = MAKE_STRING( "Halloween.PumpkinExplode" );
	SetModelName( MAKE_STRING( "models/props_halloween/pumpkin_explode.mdl") );
	m_iszParticleName = MAKE_STRING( "pumpkin_explode" );
	m_flRadius = 300.0f; // 200.0f for MIRV pumpkin
	m_flDamage = 150.0f; // 80.0f for MIRV pumpkin
	BaseClass::Spawn();
}