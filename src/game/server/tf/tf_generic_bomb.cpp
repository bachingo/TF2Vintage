//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "world.h"
#include "triggers.h"
#include "tf_generic_bomb.h"
#include "tf_gamerules.h"
#include "te_particlesystem.h"
#include "particle_parse.h"
#include "tf_fx.h"
#include "tf_player.h"
#include "tf_projectile_base.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weaponbase_grenadeproj.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_AUTO_LIST( ITFGenericBomb );

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

	DEFINE_FUNCTION( CTFGenericBombShim::Touch ),
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

	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );

	char const *szModel = STRING( GetModelName() );
	if ( !szModel || !szModel[0] )
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

	trace_t	tr;
	Vector vecForward = GetAbsVelocity();
	VectorNormalize( vecForward );
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + 60*vecForward , MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	Vector vecOrigin = GetAbsOrigin(); QAngle vecAngles = GetAbsAngles();
	int iAttachment = LookupAttachment("alt-origin");
	if ( iAttachment > 0 )
		GetAttachment( iAttachment, vecOrigin, vecAngles );

	CPVSFilter filter( GetAbsOrigin() );
	if ( STRING( m_iszParticleName ) )
		TE_TFParticleEffect( filter, 0.0, STRING( m_iszParticleName ), vecOrigin, vecAngles, NULL, PATTACH_CUSTOMORIGIN );
	if ( STRING( m_iszExplodeSound ) )
		EmitSound( STRING( m_iszExplodeSound ) );

	SetSolid( SOLID_NONE ); 

	CBaseEntity *pAttacker = this;

	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
		pAttacker = info.GetAttacker();

	CTakeDamageInfo info_modified( this, pAttacker, m_flDamage, DMG_BLAST );

	if ( m_bFriendlyFire )
		info_modified.SetForceFriendlyFire( true );

	TFGameRules()->RadiusDamage( info_modified, GetAbsOrigin(), m_flRadius, CLASS_NONE, this );

	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch");

	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();

	m_OnDetonate.FireOutput( this, this );
	BaseClass::Event_Killed( info );
}

void CTFGenericBomb::InputDetonate( inputdata_t &inputdata )
{
	CTakeDamageInfo empty;
	Event_Killed( empty );
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


IMPLEMENT_AUTO_LIST( ITFPumpkinBomb );

LINK_ENTITY_TO_CLASS( tf_pumpkin_bomb, CTFPumpkinBomb );

CTFPumpkinBomb::CTFPumpkinBomb()
{
	m_bKilled = false;
	m_bPrecached = false;
	m_iTeam = TF_TEAM_NPC;
	m_flDamage = 150.0f;
	m_flScale = 1.0f;
	m_flRadius = 300.0f;
	m_flLifeTime = -1.0f;
}

void CTFPumpkinBomb::Precache( void )
{
	BaseClass::Precache();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	int iMdlIdx = PrecacheModel( "models/props_halloween/pumpkin_explode.mdl" );
	PrecacheGibsForModel( iMdlIdx );

	PrecacheModel( "models/props_halloween/pumpkin_explode_teamcolor.mdl" );

	PrecacheScriptSound( "Halloween.PumpkinExplode" );

	CBaseEntity::SetAllowPrecache( allowPrecache );

	m_bPrecached = true;
}

void CTFPumpkinBomb::Spawn( void )
{
	if ( !m_bPrecached )
		Precache();

	if ( m_iTeam == TF_TEAM_NPC )
	{
		SetModel( "models/props_halloween/pumpkin_explode.mdl" );
		SetMoveType( MOVETYPE_VPHYSICS );
		SetSolid( SOLID_VPHYSICS );
	}
	else
	{
		SetModel( "models/props_halloween/pumpkin_explode_teamcolor.mdl" );

		switch ( m_iTeam )
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

		SetCollisionGroup( TFCOLLISION_GROUP_PUMPKIN_BOMB );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
		SetSolid( SOLID_BBOX );
	}

	BaseClass::Spawn();

	m_iHealth = 1;
	m_bKilled = false;
	m_takedamage = DAMAGE_YES;
	SetModelScale( m_flScale );

	if ( m_flLifeTime > 0.0f )
	{
		SetThink( &CTFPumpkinBomb::RemovePumpkin );
		SetNextThink( m_flLifeTime + gpGlobals->curtime );
	}
	SetTouch( &CTFPumpkinBombShim::Touch );
}

int CTFPumpkinBomb::OnTakeDamage( CTakeDamageInfo const &info )
{
	if ( m_iTeam != TF_TEAM_NPC )
	{
		CPVSFilter filter( GetAbsOrigin() );
		TE_TFParticleEffect( filter, 0, ConstructTeamParticle( "spell_pumpkin_mirv_goop_%s", m_iTeam ), GetAbsOrigin(), vec3_angle );
	}

	CBaseEntity *pAttacker = info.GetAttacker();
	if ( pAttacker && pAttacker->GetTeamNumber() == m_iTeam )
	{
		SetHealth( 1 );
		return BaseClass::OnTakeDamage( info );
	}

	if ( m_iTeam != TF_TEAM_NPC )
	{
		RemovePumpkin();
		return 0;
	}

	return BaseClass::OnTakeDamage( info );
}

void CTFPumpkinBomb::Event_Killed( CTakeDamageInfo const &info )
{
	if ( m_bKilled )
		return;
	m_bKilled = true;

	CBaseEntity *pAttacker = info.GetAttacker();
	Vector vecOrigin = GetAbsOrigin();
	if ( m_iTeam != TF_TEAM_NPC )
	{
		if ( pAttacker && pAttacker->GetTeamNumber() != m_iTeam )
		{
			RemovePumpkin();
			return;
		}
	}

	trace_t	tr;
	UTIL_TraceLine( vecOrigin + Vector(0,0,6.0f), vecOrigin - Vector(0,0,32.0f), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	CPVSFilter filter( vecOrigin );
	TE_TFExplosion( filter, 0, vecOrigin, tr.plane.normal, TF_WEAPON_PUMPKIN_BOMB, -1, -1 );
	TE_TFParticleEffect( filter, 0, "pumpkin_explode", vecOrigin, GetAbsAngles() );

	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch");

	SetSolid( SOLID_NONE );

	if ( pAttacker )
		ChangeTeam( pAttacker->GetTeamNumber() );

	if ( TFGameRules() )
	{
		CTakeDamageInfo newInfo( this, info.GetAttacker(), m_flDamage, DMG_BLAST|DMG_HALF_FALLOFF|DMG_NOCLOSEDISTANCEMOD, m_bSpell ? TF_DMG_CUSTOM_SPELL_MIRV : TF_DMG_CUSTOM_PUMPKIN_BOMB );
		CTFRadiusDamageInfo radiusInfo( &newInfo, GetAbsOrigin(), m_flRadius );
		TFGameRules()->RadiusDamage( radiusInfo );
	}

	Break();

	BaseClass::Event_Killed( info );
}

void CTFPumpkinBomb::PumpkinTouch( CBaseEntity *pOther )
{
	if ( pOther == nullptr )
		return;

	if ( !( pOther->GetFlags() & FL_GRENADE ) )
	{
		if ( FStrEq( pOther->GetClassname(), "trigger_hurt" ) )
			RemovePumpkin();

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
		CTFBaseProjectile *pProj = dynamic_cast<CTFBaseProjectile *>( pOther );
		if ( pProj )
		{
			pPlayer = ToTFPlayer( pProj->GetScorer() );
		}
	}

	if ( m_iTeam != TF_TEAM_NPC && pOther->GetTeamNumber() != m_iTeam )
	{
		RemovePumpkin();
		return;
	}

	CTakeDamageInfo info( pOther, pPlayer, 10.0f, DMG_GENERIC );
	TakeDamage( info );
}

void CTFPumpkinBomb::RemovePumpkin( void )
{
	const char *szEffectName;
	switch ( m_iTeam )
	{
		case TF_TEAM_RED:
			szEffectName = "spell_pumpkin_mirv_goop_red";
			break;
		case TF_TEAM_BLUE:
			szEffectName = "spell_pumpkin_mirv_goop_blue";
			break;

		default:
			szEffectName = "spell_pumpkin_mirv_goop_blue";
			break;
	}

	CPVSFilter filter( GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0, szEffectName, GetAbsOrigin(), vec3_angle );

	UTIL_Remove( this );
}

void CTFPumpkinBomb::Break()
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();
}
