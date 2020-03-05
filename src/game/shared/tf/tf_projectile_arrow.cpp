//=============================================================================//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_arrow.h"
#include "effect_dispatch_data.h"
#include "tf_gamerules.h"

#ifdef GAME_DLL
	#include "SpriteTrail.h"
	#include "props_shared.h"
	#include "debugoverlay_shared.h"
	#include "collisionutils.h"
	#include "te_effect_dispatch.h"
	#include "decals.h"
	#include "bone_setup.h"
	#include "tf_fx.h"
	#include "tf_gamestats.h"
	#include "tf_generic_bomb.h"
	#include "tf_obj.h"
	#include "tf_halloween_boss.h"
#endif

#ifdef GAME_DLL
ConVar tf_debug_arrows( "tf_debug_arrows", "0", FCVAR_CHEAT );
#endif

extern ConVar tf2v_minicrits_on_deflect;


const char *g_pszArrowModels[] =
{
	"models/weapons/w_models/w_arrow.mdl",
	"models/weapons/w_models/w_syringe_proj.mdl",
	"models/weapons/w_models/w_repair_claw.mdl",
	"models/weapons/w_models/w_arrow_xmas.mdl",
	"models/weapons/c_models/c_crusaders_crossbow/c_crusaders_crossbow_xmas_proj.mdl",
	"models/weapons/c_models/c_grapple_proj.mdl",
};

#define ARROW_FADE_TIME		3.f
#define MASK_TFARROWS		CONTENTS_SOLID|CONTENTS_HITBOX|CONTENTS_MONSTER


class CTraceFilterCollisionArrows : public CTraceFilterEntitiesOnly
{
public:
	CTraceFilterCollisionArrows( CBaseEntity *pPass1, CBaseEntity *pPass2 )
		: m_pArrow( pPass1 ), m_pOwner( pPass2 ) {}

	virtual bool ShouldHitEntity( IHandleEntity *pEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pEntity, m_pArrow ) )
			return false;

		const CBaseEntity *pEntTouch = EntityFromEntityHandle( pEntity );
		if ( pEntTouch == nullptr )
			return true;

		if ( pEntTouch == m_pOwner )
			return false;

		int iCollisionGroup = pEntTouch->GetCollisionGroup();
		if ( iCollisionGroup == COLLISION_GROUP_DEBRIS )
			return false;

		if ( iCollisionGroup == TFCOLLISION_GROUP_GRENADES )
			return false;

		if ( iCollisionGroup == TFCOLLISION_GROUP_ROCKETS )
			return false;

		if ( iCollisionGroup == TFCOLLISION_GROUP_RESPAWNROOMS )
			return false;

		if ( iCollisionGroup == TFCOLLISION_GROUP_ARROWS )
			return false;

		return iCollisionGroup != COLLISION_GROUP_NONE;
	}

private:
	IHandleEntity *m_pArrow;
	IHandleEntity *m_pOwner;
};

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Arrow, DT_TFProjectile_Arrow )
BEGIN_NETWORK_TABLE( CTFProjectile_Arrow, DT_TFProjectile_Arrow )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropBool( RECVINFO( m_bFlame ) ),
	RecvPropInt( RECVINFO( m_iProjType ) ),
#else
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropBool( SENDINFO( m_bFlame ) ),
	SendPropInt( SENDINFO( m_iProjType ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_Arrow )
	DEFINE_ENTITYFUNC( ArrowTouch )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_arrow, CTFProjectile_Arrow );
PRECACHE_REGISTER( tf_projectile_arrow );

CTFProjectile_Arrow::CTFProjectile_Arrow()
{
}

CTFProjectile_Arrow::~CTFProjectile_Arrow()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef GAME_DLL

CTFProjectile_Arrow *CTFProjectile_Arrow::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, float flSpeed, float flGravity, bool bFlame, CBaseEntity *pOwner, CBaseEntity *pScorer, int iType )
{
	const char *pszEntClass = "tf_projectile_arrow";
	switch ( iType )
	{
		case TF_PROJECTILE_HEALING_BOLT:
		case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
			pszEntClass = "tf_projectile_healing_bolt";
			break;
		default:
			pszEntClass = "tf_projectile_arrow";
			break;
	}
	CTFProjectile_Arrow *pArrow = static_cast<CTFProjectile_Arrow *>( CBaseEntity::CreateNoSpawn( pszEntClass, vecOrigin, vecAngles, pOwner ) );

	if ( pArrow )
	{
		// Set team.
		pArrow->ChangeTeam( pOwner->GetTeamNumber() );

		// Set scorer.
		pArrow->SetScorer( pScorer );

		// Set firing weapon.
		pArrow->SetLauncher( pWeapon );
		
		// Compensate iTypes from shareddefs to a more usable range for our use.
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
		{
			switch ( iType )  // If it's the holidays, use festive projectiles.
			{
				case TF_PROJECTILE_ARROW:
					iType = TF_PROJECTILE_FESTIVE_ARROW;
					break;
				case TF_PROJECTILE_HEALING_BOLT:
					iType = TF_PROJECTILE_FESTIVE_HEALING_BOLT;
					break;
			}
		}
		
		const char *pszArrowModel = "";
		switch ( iType )
		{
			case TF_PROJECTILE_ARROW:
				pszArrowModel = "models/weapons/w_models/w_arrow.mdl";
				break;
			case TF_PROJECTILE_HEALING_BOLT:
				pszArrowModel = "models/weapons/w_models/w_syringe_proj.mdl";
				break;
			case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
				pszArrowModel = "models/weapons/w_models/w_repair_claw.mdl";
				break;
			case TF_PROJECTILE_FESTIVE_ARROW:
				pszArrowModel = "models/weapons/w_models/w_arrow_xmas.mdl";
				break;
			case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
				pszArrowModel = "models/weapons/c_models/c_crusaders_crossbow/c_crusaders_crossbow_xmas_proj.mdl";
				break;
			case TF_PROJECTILE_GRAPPLINGHOOK:
				pszArrowModel = "models/weapons/c_models/c_grapple_proj.mdl";
				break;
		}
		
		if ( iType == TF_PROJECTILE_ARROW || iType == TF_PROJECTILE_FESTIVE_ARROW )	// Huntsman Arrows.
		{
			// Set flame arrow.
			pArrow->SetFlameArrow( bFlame );
			
			// Use the default skin.
			pArrow->m_nSkin = 0;
		}
		else
		{
			//Never light on fire.
			pArrow->SetFlameArrow( false );
		}

		// Set arrow type.
		pArrow->SetType( iType );
		pArrow->SetModel( pszArrowModel );

		// Spawn.
		DispatchSpawn( pArrow );
		pArrow->m_flTrailReflectLifetime = 0;

		// Setup the initial velocity.
		Vector vecForward, vecRight, vecUp;
		AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );
	
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flSpeed, mult_projectile_speed );

		Vector vecVelocity = vecForward * flSpeed;
		pArrow->SetAbsVelocity( vecVelocity );
		pArrow->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		// Setup the initial angles.
		QAngle angles;
		VectorAngles( vecVelocity, angles );
		pArrow->SetAbsAngles( angles );

		pArrow->SetGravity( flGravity );

		return pArrow;
	}

	return pArrow;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Precache( void )
{
	// Precache all arrow models we have.
	for ( int i = 0; i < ARRAYSIZE( g_pszArrowModels ); i++ )
	{
		int iIndex = PrecacheModel( g_pszArrowModels[i] );
		PrecacheGibsForModel( iIndex );
	}

	for ( int i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; i++ )
	{
		PrecacheModel( ConstructTeamParticle( "effects/arrowtrail_%s.vmt", i, false, g_aTeamNamesShort ) );
		PrecacheModel( ConstructTeamParticle( "effects/healingtrail_%s.vmt", i, false, g_aTeamNamesShort ) );
		PrecacheModel( ConstructTeamParticle( "effects/repair_claw_trail_%s.vmt", i, false, g_aTeamParticleNames ) );
	}

	// Precache flame effects
	PrecacheParticleSystem( "flying_flaming_arrow" );

	PrecacheScriptSound( "Weapon_Arrow.ImpactFlesh" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactMetal" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactWood" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactConcrete" );
	PrecacheScriptSound( "Weapon_Arrow.Nearmiss" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Spawn( void )
{

	BaseClass::Spawn();

#ifdef TF_ARROW_FIX
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
#endif

	if ( m_iProjType == TF_PROJECTILE_HEALING_BOLT || m_iProjType == TF_PROJECTILE_FESTIVE_HEALING_BOLT )
		SetModelScale( 3.0f );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );

	UTIL_SetSize( this, -Vector( 1, 1, 1 ), Vector( 1, 1, 1 ) );

	SetSolid( SOLID_BBOX );
	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

	AddEffects( EF_NOSHADOW );
	AddFlag( FL_GRENADE );

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

	m_flCreateTime = gpGlobals->curtime;

	CreateTrail();

	SetTouch( &CTFProjectile_Arrow::ArrowTouch );
	SetThink( &CTFProjectile_Arrow::FlyThink );
	SetNextThink(gpGlobals->curtime);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Arrow::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::ArrowTouch( CBaseEntity *pOther )
{
	float flTimeAlive = gpGlobals->curtime - m_flCreateTime;
	if ( flTimeAlive >= 10.0 )
	{
		Warning( "Arrow alive for %f3.2 seconds\n", flTimeAlive );
		UTIL_Remove( this );
	}

	// Verify a correct "other."
	Assert( pOther );
	if ( m_bImpacted )
		return;

	bool bImpactedItem = false;
	if ( pOther->IsCombatItem() )
		bImpactedItem = !InSameTeam( pOther );

	CTFPumpkinBomb *pPumpkin = dynamic_cast<CTFPumpkinBomb *>( pOther );

	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) && !pPumpkin && !bImpactedItem )
	{
		return;
	}

	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	CBaseCombatCharacter *pActor = dynamic_cast<CBaseCombatCharacter *>( pOther );
	if ( pActor == nullptr )
	{
		pActor = dynamic_cast<CBaseCombatCharacter *>( pOther->GetOwnerEntity() );
	}

	//CTFRobotDestruction_Robot *pRobot = dynamic_cast<CTFRobotDestruction_Robot *>( pOther );
	//CTFMerasmusTrickOrTreatProp *pMerasProp = dynamic_cast<CTFMerasmusTrickOrTreatProp *>( pOther );

	if ( !FNullEnt( pOther->edict() ) &&
		( pActor != nullptr || pPumpkin != nullptr/* || pMerasProp != nullptr || pRobot != nullptr*/ || bImpactedItem ) )
	{
		CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>( pOther );
		if ( !pAnimating )
		{
			UTIL_Remove( this );
			return;
		}

		CStudioHdr *pStudioHdr = pAnimating->GetModelPtr();
		if ( !pStudioHdr )
		{
			UTIL_Remove( this );
			return;
		}

		mstudiohitboxset_t *pSet = pStudioHdr->pHitboxSet( pAnimating->GetHitboxSet() );
		if ( !pSet )
		{
			UTIL_Remove( this );
			return;
		}

		// Determine where we should land
		Vector vecOrigin = GetAbsOrigin();
		Vector vecDir = GetAbsVelocity();

		trace_t tr;

		CTraceFilterCollisionArrows filter( this, GetOwnerEntity() );
		UTIL_TraceLine( vecOrigin, vecOrigin + vecDir * gpGlobals->frametime, MASK_TFARROWS, &filter, &tr );

		if ( tr.m_pEnt && tr.m_pEnt->GetTeamNumber() != GetTeamNumber() )
		{
			mstudiobbox_t *pBox = pSet->pHitbox( tr.hitbox );
			if ( pBox )
			{
				if ( !StrikeTarget( pBox, pOther ) )
					BreakArrow();

				if ( !m_bImpacted )
					SetAbsOrigin( vecOrigin );

				if ( bImpactedItem )
					BreakArrow();

				m_bImpacted = true;
				return;
			}
		}

		Vector vecFwd;
		AngleVectors( GetAbsAngles(), &vecFwd );
		Vector vecArrowEnd = GetAbsOrigin() + vecFwd * 16;

		// Find the closest hitbox we crossed
		float flClosest = 99999.f;
		mstudiobbox_t *pBox = NULL, *pCurrent = NULL;
		for ( int i = 0; i < pSet->numhitboxes; i++ )
		{
			pCurrent = pSet->pHitbox( i );

			Vector boxPosition;
			QAngle boxAngles;
			pAnimating->GetBonePosition( pCurrent->bone, boxPosition, boxAngles );

			Ray_t ray;
			ray.Init( boxPosition, vecArrowEnd );

			trace_t trace;
			IntersectRayWithBox( ray, boxPosition + pCurrent->bbmin, boxPosition + pCurrent->bbmax, 0, &trace );

			float flDistance = ( trace.endpos - vecArrowEnd ).Length();
			if ( flDistance < flClosest )
			{
				pBox = pCurrent;
				flClosest = flDistance;
			}
		}

		if ( pBox )
		{
			if ( !StrikeTarget( pBox, pOther ) )
				BreakArrow();

			if ( !m_bImpacted )
				SetAbsOrigin( vecOrigin );

			if ( bImpactedItem )
				BreakArrow();

			m_bImpacted = true;
		}
	}
	else
	{
		CheckSkyboxImpact( pOther );
		// TODO: Achievment hunters
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::StrikeTarget( mstudiobbox_t *pBox, CBaseEntity *pTarget )
{
	if ( pTarget == nullptr )
		return false;

	if ( pTarget->IsBaseObject() && InSameTeam( pTarget ) )
		HealBuilding( pTarget );

	CTFPlayer *pPlayer = ToTFPlayer( pTarget );
	if ( pPlayer && pPlayer->m_Shared.IsInvulnerable() )
		return false;

	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>( pTarget );
	if ( pAnimating == nullptr )
		return false;

	bool bBreakArrow = false;
	if ( dynamic_cast<CHalloweenBaseBoss *>( pTarget )/* || dynamic_cast<CTFTankBoss *>( pTarget )*/ )
		bBreakArrow = true;

	if ( !bBreakArrow )
	{
		if ( !PositionArrowOnBone( pBox, pAnimating ) )
			return false;
	}

	bool bHeadshot = false;
	if ( pBox->group == HITGROUP_HEAD && CanHeadshot() )
		bHeadshot = true;

	Vector vecOrigin = GetAbsOrigin();
	Vector vecDir = GetAbsVelocity();
	VectorNormalizeFast( vecDir );

	CBaseEntity *pAttacker = GetScorer();
	if ( pAttacker == nullptr )
	{
		pAttacker = GetOwnerEntity();
	}

	int iDmgCustom = TF_DMG_CUSTOM_NONE;
	int iDmgType = GetDamageType();
	bool bImpact = true; // TODO: Some strange check involving a UtlVector on the arrow, possibly for pierce

	if( pAttacker )
	{
		if ( InSameTeam( pTarget ) )
		{
			if ( bImpact )
				ImpactTeamPlayer( ToTFPlayer( pTarget ) );
		}
		else
		{
			IScorer *pScorer = dynamic_cast<IScorer *>( pAttacker );
			if ( pScorer )
				pAttacker = pScorer->GetScorer();

			if ( m_bFlame )
			{
				iDmgType |= DMG_IGNITE;
				iDmgCustom = TF_DMG_CUSTOM_BURNING_ARROW;
			}

			if ( bHeadshot )
			{
				iDmgType |= DMG_CRITICAL;
				iDmgCustom = TF_DMG_CUSTOM_HEADSHOT;
			}

			if ( m_bCritical )
				iDmgType |= DMG_CRITICAL;

			if ( bImpact )
			{
				CTakeDamageInfo info( this, pAttacker, m_hLauncher, GetAbsOrigin(), GetAbsVelocity(), GetDamage(), iDmgType, iDmgCustom );
				pTarget->TakeDamage( info );

				PlayImpactSound( ToTFPlayer( pAttacker ), "Weapon_Arrow.ImpactFlesh", true );
			}
		}

		if( !m_bImpacted && !bBreakArrow )
		{
			Vector vecBoneOrigin;
			QAngle vecBoneAngles;
			int iBone, iPhysicsBone;
			GetBoneAttachmentInfo( pBox, pAnimating, vecBoneOrigin, vecBoneAngles, iBone, iPhysicsBone );

			if ( pPlayer && !pPlayer->IsAlive() )
			{
				if ( CheckRagdollPinned( vecOrigin, vecDir, iBone, iPhysicsBone, pPlayer->m_hRagdoll, pBox->group, pPlayer->entindex() ) )
				{
					pPlayer->StopRagdollDeathAnim();
				}
				else
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "arrow_impact" );

					if ( event )
					{
						event->SetInt( "attachedEntity", pTarget->entindex() );
						event->SetInt( "shooter", pAttacker->entindex() );
						event->SetInt( "projectileType", GetProjectileType() );
						event->SetInt( "boneIndexAttached", iBone );
						event->SetFloat( "bonePositionX", vecBoneOrigin.x );
						event->SetFloat( "bonePositionY", vecBoneOrigin.y );
						event->SetFloat( "bonePositionZ", vecBoneOrigin.z );
						event->SetFloat( "boneAnglesX", vecBoneAngles.x );
						event->SetFloat( "boneAnglesY", vecBoneAngles.y );
						event->SetFloat( "boneAnglesZ", vecBoneAngles.z );

						gameeventmanager->FireEvent( event );
					}
				}
			}
			else
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "arrow_impact" );

				if ( event )
				{
					event->SetInt( "attachedEntity", pTarget->entindex() );
					event->SetInt( "shooter", pAttacker->entindex() );
					event->SetInt( "projectileType", GetProjectileType() );
					event->SetInt( "boneIndexAttached", iBone );
					event->SetFloat( "bonePositionX", vecBoneOrigin.x );
					event->SetFloat( "bonePositionY", vecBoneOrigin.y );
					event->SetFloat( "bonePositionZ", vecBoneOrigin.z );
					event->SetFloat( "boneAnglesX", vecBoneAngles.x );
					event->SetFloat( "boneAnglesY", vecBoneAngles.y );
					event->SetFloat( "boneAnglesZ", vecBoneAngles.z );

					gameeventmanager->FireEvent( event );
				}
			}

			FadeOut( ARROW_FADE_TIME );
		}
	}

	trace_t tr;
	CTraceFilterCollisionArrows filter( this, GetOwnerEntity() );
	UTIL_TraceLine( vecOrigin, vecOrigin - vecDir * gpGlobals->frametime, MASK_TFARROWS, &filter, &tr );

	UTIL_ImpactTrace( &tr, DMG_GENERIC );

	return !bBreakArrow;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::CheckSkyboxImpact( CBaseEntity *pOther )
{
	Vector vecFwd = GetAbsVelocity();
	vecFwd.NormalizeInPlace();

	Vector vecOrigin = GetAbsOrigin();

	trace_t tr;
	UTIL_TraceLine( vecOrigin, vecOrigin + vecFwd * 32, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );

	if ( tr.fraction < 1.0f && ( tr.surface.flags & SURF_SKY ) )
	{
		FadeOut( ARROW_FADE_TIME );
		return true;
	}

	if ( !FNullEnt( pOther->edict() ) )
	{
		BreakArrow();
		return false;
	}

	CEffectData	data;
	data.m_vOrigin = tr.endpos;
	data.m_vNormal = vecFwd;
	data.m_nEntIndex = pOther->entindex();
	data.m_fFlags = GetProjectileType();
	data.m_nColor = (GetTeamNumber() == TF_TEAM_BLUE); //Skin

	DispatchEffect( "TFBoltImpact", data );

	const char* pszImpactSound = "Weapon_Arrow.ImpactMetal";
	surfacedata_t *psurfaceData = physprops->GetSurfaceData( tr.surface.surfaceProps );
	if( psurfaceData )
	{
		switch ( psurfaceData->game.material )
		{
			case CHAR_TEX_CONCRETE:
				pszImpactSound = "Weapon_Arrow.ImpactConcrete";
				break;
			case CHAR_TEX_WOOD:
				pszImpactSound = "Weapon_Arrow.ImpactWood";
				break;
			default:
				pszImpactSound = "Weapon_Arrow.ImpactMetal";
				break;
		}
	}

	// Play sound
	if ( pszImpactSound )
	{
		PlayImpactSound( ToTFPlayer( GetScorer() ), pszImpactSound );
	}

	FadeOut( ARROW_FADE_TIME );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::HealBuilding( CBaseEntity *pTarget )
{
	if ( !pTarget->IsBaseObject() )
		return;

	CBasePlayer *pOwner = GetScorer();
	if ( pOwner == nullptr )
		return;

	if ( GetTeamNumber() != pTarget->GetTeamNumber() )
		return;

	int nArrowHealsBuilding = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, nArrowHealsBuilding, arrow_heals_buildings );
	if ( nArrowHealsBuilding == 0 )
		return;

	CBaseObject *pObject = dynamic_cast<CBaseObject *>( pTarget );
	if ( pObject == nullptr )
		return;

	if ( pObject->HasSapper() || pObject->IsBeingCarried() || pObject->IsRedeploying() )
		return;

	int nHealth = pObject->GetHealth();
	int nHealthToAdd = Min( nArrowHealsBuilding + nHealth, pObject->GetMaxHealth() );

	if ( ( nHealthToAdd - nHealth ) > 0 )
	{
		pObject->SetHealth( nHealthToAdd );

		IGameEvent *event = gameeventmanager->CreateEvent( "building_healed" );
		if ( event )
		{
			event->SetInt( "priority", 1 ); // HLTV priority
			event->SetInt( "building", pObject->entindex() );
			event->SetInt( "healer", pOwner->entindex() );
			event->SetInt( "amount", nHealthToAdd - nHealth );

			gameeventmanager->FireEvent( event );
		}

		CPVSFilter filter( GetAbsOrigin() );
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_BLUE:
				TE_TFParticleEffect( filter, 0, "repair_claw_heal_blue", GetAbsOrigin(), vec3_angle );
				break;
			default:
				TE_TFParticleEffect( filter, 0, "repair_claw_heal_red", GetAbsOrigin(), vec3_angle );
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::FlyThink(void)
{
	QAngle angles;

	VectorAngles( GetAbsVelocity(), angles );

	SetAbsAngles( angles );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Arrow::GetDamageType()
{
	int iDmgType = BaseClass::GetDamageType();

	// Buff banner mini-crit calculations
	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )m_hLauncher.Get();
	if ( pWeapon )
	{
		pWeapon->CalcIsAttackMiniCritical();
		if ( pWeapon->IsCurrentAttackAMiniCrit() )
		{
			iDmgType |= DMG_MINICRITICAL;
		}
	}

	if ( m_iProjType == TF_PROJECTILE_HEALING_BOLT || m_iProjType == TF_PROJECTILE_FESTIVE_HEALING_BOLT || m_iProjType == TF_PROJECTILE_BUILDING_REPAIR_BOLT )
	{
		iDmgType |= DMG_USEDISTANCEMOD;
	}
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}
	if ( CanHeadshot() )
	{
		iDmgType |= DMG_USE_HITLOCATIONS;
	}
	if ( m_bFlame == true )
	{
		iDmgType |= DMG_IGNITE;	
	}
	if ( ( m_iDeflected > 0 ) && ( tf2v_minicrits_on_deflect.GetBool() ) )
	{
		iDmgType |= DMG_MINICRITICAL;
	}

	return iDmgType;
}

bool CTFProjectile_Arrow::IsDeflectable(void)
{
	// Don't deflect projectiles with non-deflect attributes.
	if ( m_hLauncher.Get() )
	{
		// Check to see if this is a non-deflectable projectile, like an energy projectile.
		int nCannotDeflect = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher.Get(), nCannotDeflect, energy_weapon_no_deflect );
		if ( nCannotDeflect != 0 )
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	CTFPlayer *pDeflector = ToTFPlayer( pDeflectedBy );
	if ( pDeflector == nullptr || m_iProjType == TF_PROJECTILE_GRAPPLINGHOOK ) // Don't allow grappling hooks to be deflected.
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
		pOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );

	// Get arrow's speed.
	float flVel = GetAbsVelocity().Length();

	QAngle angForward;
	VectorAngles( vecDir, angForward );

	// Now change arrow's direction.
	SetAbsAngles( angForward );
	SetAbsVelocity( vecDir * flVel );

	// And change owner.
	IncremenentDeflected();
	SetOwnerEntity( pDeflectedBy );
	SetScorer( pDeflectedBy );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );

	if ( m_iProjType != TF_PROJECTILE_ARROW && m_iProjType != TF_PROJECTILE_FESTIVE_ARROW )
	{
		m_nSkin = ( pDeflectedBy->GetTeamNumber() - 2 );
	}

	if ( pDeflector->m_Shared.IsCritBoosted() )
		m_bCritical = true;

	// Change trail color.
	if ( m_hSpriteTrail.Get() )
	{
		m_hSpriteTrail->Remove();
	}

	CreateTrail();
}

void CTFProjectile_Arrow::IncremenentDeflected( void )
{
	m_iDeflected++;
	m_flTrailReflectLifetime = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::CanHeadshot( void )
{
	return ( m_iProjType == TF_PROJECTILE_ARROW || m_iProjType == TF_PROJECTILE_FESTIVE_ARROW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFProjectile_Arrow::GetTrailParticleName( void )
{

	const char *pszFormat = NULL;
	bool bLongTeamName = false;

	switch( m_iProjType )
	{
	case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
		pszFormat = "effects/repair_claw_trail_%s.vmt";
		bLongTeamName = true;
		break;
	case TF_PROJECTILE_HEALING_BOLT:
	case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
		pszFormat = "effects/healingtrail_%s.vmt";
		break;
	default:
		pszFormat = "effects/arrowtrail_%s.vmt";
		break;
	}

	return ConstructTeamParticle( pszFormat, GetTeamNumber(), false, bLongTeamName ? g_aTeamParticleNames : g_aTeamNamesShort );

}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::CreateTrail( void )
{
	if ( IsDormant() || m_hSpriteTrail != nullptr )
		return;

	if ( m_iProjType == TF_PROJECTILE_HEALING_BOLT || m_iProjType == TF_PROJECTILE_FESTIVE_HEALING_BOLT )
		return;

	CSpriteTrail *pTrail = CSpriteTrail::SpriteTrailCreate( GetTrailParticleName(), GetAbsOrigin(), true );
	if ( pTrail )
	{
		pTrail->FollowEntity( this );
		pTrail->SetTransparency( kRenderTransAlpha, -1, -1, -1, 255, kRenderFxNone );
		pTrail->SetStartWidth( m_iProjType == TF_PROJECTILE_BUILDING_REPAIR_BOLT ? 5.0f : 3.0f );
		pTrail->SetTextureResolution( 0.01f );
		pTrail->SetLifeTime( 0.3f );
		pTrail->SetAttachment( this, PATTACH_ABSORIGIN );
		pTrail->SetContextThink( &CTFProjectile_Arrow::RemoveTrail, gpGlobals->curtime + 3.0f, "FadeTrail" );

		m_hSpriteTrail = pTrail;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::RemoveTrail( void )
{
	if ( !m_hSpriteTrail )
		return;

	if( m_flTrailReflectLifetime <= 0 )
	{
		UTIL_Remove( m_hSpriteTrail.Get() );
		m_flTrailReflectLifetime = 1.0f;
	}
	else
	{
		CSpriteTrail *pSprite = dynamic_cast<CSpriteTrail *>( m_hSpriteTrail.Get() );
		if ( pSprite )
			pSprite->SetBrightness( m_flTrailReflectLifetime * 128.f );

		m_flTrailReflectLifetime -= 0.1;

		SetContextThink( &CTFProjectile_Arrow::RemoveTrail, gpGlobals->curtime, "FadeTrail" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::AdjustDamageDirection( CTakeDamageInfo const &info, Vector &vecDirection, CBaseEntity *pEntity )
{
	if ( pEntity )
		vecDirection = ( info.GetDamagePosition() - info.GetDamageForce() ) - pEntity->WorldSpaceCenter();
}

//-----------------------------------------------------------------------------
// Purpose: Setup to remove ourselves
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::FadeOut( int iTime )
{
	SetMoveType( MOVETYPE_NONE, MOVECOLLIDE_DEFAULT );
	SetAbsVelocity( vec3_origin );
	SetSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );

	SetContextThink( &CTFProjectile_Arrow::RemoveThink, gpGlobals->curtime + iTime, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Sends to the client information for arrow gibs
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::BreakArrow( void )
{
	FadeOut( ARROW_FADE_TIME );

	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::PositionArrowOnBone( mstudiobbox_t *pbox, CBaseAnimating *pAnim )
{
	CStudioHdr *pStudioHdr = pAnim->GetModelPtr();	
	if ( !pStudioHdr )
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnim->GetHitboxSet() );

	if ( !set->numhitboxes || pbox->bone > 127 )
		return false;

	CBoneCache *pCache = pAnim->GetBoneCache();
	if ( !pCache )
		return false;

	matrix3x4_t *pBone = pCache->GetCachedBone( pbox->bone );
	if ( pBone == nullptr )
		return false;
	
	Vector vecMins, vecMaxs, vecResult;
	TransformAABB( *pBone, pbox->bbmin, pbox->bbmax, vecMins, vecMaxs );
	vecResult = vecMaxs - vecMins;

	// This is a mess
	SetAbsOrigin( ( vecResult * 0.6f + vecMins ) - ( rand() / RAND_MAX * vecResult ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::GetBoneAttachmentInfo( mstudiobbox_t *pbox, CBaseAnimating *pAnim, Vector &vecOrigin, QAngle &vecAngles, int &bone, int &iPhysicsBone )
{
	bone = pbox->bone;
	iPhysicsBone = pAnim->GetPhysicsBone( bone );
	//pAnim->GetBonePosition( bone, vecOrigin, vecAngles );

	matrix3x4_t arrowToWorld, boneToWorld, invBoneToWorld, boneToWorldTransform;
	MatrixCopy( EntityToWorldTransform(), arrowToWorld );
	pAnim->GetBoneTransform( bone, boneToWorld );

	MatrixInvert( boneToWorld, invBoneToWorld );
	ConcatTransforms( invBoneToWorld, arrowToWorld, boneToWorldTransform );
	MatrixAngles( boneToWorldTransform, vecAngles );
	MatrixGetColumn( boneToWorldTransform, 3, vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::CheckRagdollPinned( Vector const& vecOrigin, Vector const& vecDirection, int iBone, int iPhysBone, CBaseEntity *pOther, int iHitGroup, int iVictim )
{
	trace_t tr;
	UTIL_TraceLine( vecOrigin, vecOrigin + vecDirection * 120.f, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0f && tr.DidHitWorld() )
	{
		CEffectData data;
		data.m_vOrigin = tr.endpos;
		data.m_vNormal = vecDirection;
		data.m_nEntIndex = pOther->entindex();
		data.m_fFlags = GetProjectileType();
		data.m_nAttachmentIndex = iBone;
		data.m_nMaterial = iPhysBone;
		data.m_nDamageType = iHitGroup;
		data.m_nSurfaceProp = iVictim;
		data.m_nColor = (GetTeamNumber() == TF_TEAM_BLUE); //Skin

		if( GetScorer() )
			data.m_nHitBox = GetScorer()->entindex();

		DispatchEffect( "TFBoltImpact", data );

		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Purpose: Play the impact sound to nearby players of the recipient and the attacker
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::PlayImpactSound( CTFPlayer *pAttacker, const char *pszImpactSound, bool bIsPlayerImpact /*= false*/ )
{
	if ( pAttacker )
	{
		CRecipientFilter filter;
		filter.AddRecipientsByPAS( GetAbsOrigin() );

		// Only play the sound locally to the attacker if it's a player impact
		if ( bIsPlayerImpact )
		{
			filter.RemoveRecipient( pAttacker );

			CSingleUserRecipientFilter filterAttacker( pAttacker );
			EmitSound( filterAttacker, pAttacker->entindex(), pszImpactSound );
		}

		EmitSound( filter, entindex(), pszImpactSound );
	}
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Arrow::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		if ( m_bFlame )
			ParticleProp()->Create( "flying_flaming_arrow", PATTACH_POINT_FOLLOW, "muzzle" );
	}

	if ( m_bCritical )
	{
		if ( updateType == DATA_UPDATE_CREATED || m_iDeflected != m_iDeflectedParity )
			CreateCritTrail();
	}

	m_iDeflectedParity = m_iDeflected;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Arrow::CreateCritTrail( void )
{
	if ( IsDormant() )
		return;

	if ( m_pCritEffect )
	{
		ParticleProp()->StopEmission( m_pCritEffect );
		m_pCritEffect = NULL;
	}

	char const *pszEffect = ConstructTeamParticle( "critical_rocket_%s", GetTeamNumber() );
	m_pCritEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Arrow::ClientThink( void )
{
	if ( !m_bWhizzed && gpGlobals->curtime > m_flCheckNearMiss )
	{
		CheckNearMiss();
		m_flCheckNearMiss = gpGlobals->curtime + 0.05f;
	}

	if ( !m_bCritical )
	{
		if ( m_pCritEffect )
		{
			ParticleProp()->StopEmission( m_pCritEffect );
			m_pCritEffect = NULL;
		}
	}

	if( m_pAttachedTo.Get() )
	{
		if ( gpGlobals->curtime < m_flDieTime )
		{
			Remove();
			return;
		}

		if ( m_pAttachedTo->GetEffects() & EF_NODRAW )
		{
			if ( !( GetEffects() & EF_NODRAW ) )
			{
				AddEffects( EF_NODRAW );
				UpdateVisibility();
			}
		}
	}

	if ( IsDormant() && !( GetEffects() & EF_NODRAW ) )
	{
		AddEffects( EF_NODRAW );
		UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Arrow::CheckNearMiss( void )
{
	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocal == nullptr || !pLocal->IsAlive() )
		return;

	if ( pLocal->GetTeamNumber() == GetTeamNumber() )
		return;

	Vector vecOrigin = GetAbsOrigin();
	Vector vecTarget = pLocal->GetAbsOrigin();

	Vector vecFwd;
	AngleVectors( GetAbsAngles(), &vecFwd );

	Vector vecDirection = vecOrigin + vecFwd * 200;
	if ( ( vecDirection - vecTarget ).LengthSqr() > ( vecOrigin - vecTarget ).LengthSqr() )
	{
		// We passed right by him between frames, doh!
		m_bWhizzed = true;
		return;
	}

	Vector vecClosest; float flDistance;
	CalcClosestPointOnLineSegment( vecTarget, vecOrigin, vecDirection, vecClosest, &flDistance );

	flDistance = ( vecClosest - vecTarget ).Length();
	if ( flDistance <= 120.f )
	{
		m_bWhizzed = true;
		SetNextClientThink( CLIENT_THINK_NEVER );

		trace_t tr;
		UTIL_TraceLine( vecOrigin, vecOrigin + vecFwd * 400, MASK_TFARROWS, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.DidHit() )
			return;

		EmitSound_t parm;
		parm.m_pSoundName = "Weapon_Arrow.Nearmiss";

		CSingleUserRecipientFilter filter( pLocal );
		C_BaseEntity::EmitSound( filter, entindex(), parm );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Arrow::NotifyBoneAttached( C_BaseAnimating* attachTarget )
{
	BaseClass::NotifyBoneAttached( attachTarget );

	m_bAttachment = true;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

#endif
