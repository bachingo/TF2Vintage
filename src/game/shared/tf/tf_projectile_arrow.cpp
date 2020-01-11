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
#include "te_effect_dispatch.h"
#include "decals.h"
#include "bone_setup.h"
#include "tf_gamestats.h"
#endif

#ifdef GAME_DLL
ConVar tf_debug_arrows( "tf_debug_arrows", "0", FCVAR_CHEAT );
ConVar tf2v_healing_bolts("tf2v_healing_bolts", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables crossbow bolts to be able to heal teammates." );
ConVar tf2v_healing_bolts_heal_factor("tf2v_healing_bolts_heal_factor", "2", FCVAR_NOTIFY | FCVAR_REPLICATED, "Multiplication factor of healing when using a healing bolt." );
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

const char *g_pszArrowHits[] =
{
	"arrow_impact"
	"bolt_impact"
	"claw_impact"
	"arrow_impact"
	"bolt_impact"
};

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Arrow, DT_TFProjectile_Arrow )

BEGIN_NETWORK_TABLE( CTFProjectile_Arrow, DT_TFProjectile_Arrow )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropBool( RECVINFO( m_bFlame ) ),
	RecvPropInt( RECVINFO( m_iType ) ),
	RecvPropInt( RECVINFO( m_nSkin ) ),
#else
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropBool( SENDINFO( m_bFlame ) ),
	SendPropInt( SENDINFO( m_iType ), 0, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nSkin ), 0, SPROP_UNSIGNED ),
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
	bEmitting = false;
#else
	m_bCollideWithTeammates = false;
#endif
}

#ifdef GAME_DLL

CTFProjectile_Arrow *CTFProjectile_Arrow::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, float flSpeed, float flGravity, bool bFlame, CBaseEntity *pOwner, CBaseEntity *pScorer, int iType )
{
	CTFProjectile_Arrow *pArrow = static_cast<CTFProjectile_Arrow *>( CBaseEntity::CreateNoSpawn( "tf_projectile_arrow", vecOrigin, vecAngles, pOwner ) );

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
			switch (iType)  // If it's the holidays, use festive projectiles.
			{
			case TF_PROJECTILE_ARROW:
				iType = 3;
				break;
			case TF_PROJECTILE_HEALING_BOLT:
				iType = 4;
				break;
			case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
				iType = 2;
				break;
			case TF_PROJECTILE_FESTITIVE_ARROW:
				iType = 3;
				break;
			case TF_PROJECTILE_FESTITIVE_HEALING_BOLT:
				iType = 4;
				break;
			case TF_PROJECTILE_GRAPPLINGHOOK:
				iType = 5;
				break;
			}
		}
		else
		{
			switch (iType)
			{
			case TF_PROJECTILE_ARROW:
				iType = 0;
				break;
			case TF_PROJECTILE_HEALING_BOLT:
				iType = 1;
				break;
			case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
				iType = 2;
				break;
			case TF_PROJECTILE_FESTITIVE_ARROW:
				iType = 0;
				break;
			case TF_PROJECTILE_FESTITIVE_HEALING_BOLT:
				iType = 1;
				break;
			case TF_PROJECTILE_GRAPPLINGHOOK:
				iType = 5;
				break;
			}
		}	
		
		if (iType == 0 || iType == 3 )	// Huntsman Arrows.
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
			
			if (iType != 5) // Bolts & Repair Claw
			{
				// Set our skin.
				switch (pOwner->GetTeamNumber())
				{
				case TF_TEAM_RED:
					pArrow->m_nSkin = 0;
					break;
				case TF_TEAM_BLUE:
					pArrow->m_nSkin = 1;
					break;
				}
				// Set our speed.
				flSpeed = 2400.00f;
			}
			else // Grappling Hook
			{
				// Use the default skin.
				pArrow->m_nSkin = 0;
				// Set our speed.
				flSpeed = 1500.00f;
			}
		}

		// Set arrow type.
		pArrow->SetType(iType);
		pArrow->SetModel( g_pszArrowModels[iType] );

		// Spawn.
		DispatchSpawn( pArrow );

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
		int iIndex = PrecacheModel(g_pszArrowModels[i]);
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

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( 0.3f ); // TODO: Check again later.

	UTIL_SetSize( this, -Vector( 1, 1, 1 ), Vector( 1, 1, 1 ) );

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
	// Verify a correct "other."
	Assert( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
	{
		return;
	}

	// Handle hitting skybox (disappear).
	trace_t *pTrace = const_cast<trace_t *>( &CBaseEntity::GetTouchTrace() );
	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	Vector vecOrigin = GetAbsOrigin();
	Vector vecDir = GetAbsVelocity();
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( m_hLauncher.Get() );
	trace_t trHit, tr;
	trHit = *pTrace;
	const char* pszImpactSound = NULL;
	bool bPlayerImpact = false;

	if ( pPlayer && !pPlayer->m_Shared.IsInvulnerable() )
	{
		// Determine where we should land
		Vector vecDir = GetAbsVelocity();
		VectorNormalizeFast( vecDir );
		CStudioHdr *pStudioHdr = pPlayer->GetModelPtr();
		if ( !pStudioHdr )
			return;

		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pPlayer->GetHitboxSet() );
		if ( !set )
			return;

		// Oh boy... we gotta figure out the closest hitbox on player model to land a hit on.

		QAngle angHit;
		float flClosest = FLT_MAX;
		mstudiobbox_t *pBox = NULL, *pCurrentBox = NULL;
		//int bone = -1;
		//int group = 0;
		//Msg( "\nNum of Hitboxes: %i", set->numhitboxes );

		for ( int i = 0; i < set->numhitboxes; i++ )
		{
			pCurrentBox = set->pHitbox( i );
			//Msg( "\nGroup: %i", pBox->group );

			Vector boxPosition;
			QAngle boxAngles;
			pPlayer->GetBonePosition( pCurrentBox->bone, boxPosition, boxAngles );
			Vector vecCross = CrossProduct( vecOrigin + vecDir * 16, boxPosition );

			trace_t tr;
			UTIL_TraceLine( vecOrigin, boxPosition, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			float flLengthSqr = ( boxPosition - vecCross ).LengthSqr();
			if ( flLengthSqr < flClosest )
			{
				//Msg( "\nCLOSER: %i", pBox->group );
				//group = pBox->group;
				flClosest = flLengthSqr;
				trHit = tr;
				pBox = pCurrentBox;
			}
		}
		//Msg("\nClosest: %i\n", group);

		if ( tf_debug_arrows.GetBool() )
		{
			//Msg("\nHitBox: %i\nHitgroup: %i\n", trHit.hitbox, trHit.hitgroup);
			NDebugOverlay::Line( trHit.startpos, trHit.endpos, 0, 255, 0, true, 5.0f );
			NDebugOverlay::Line( vecOrigin, vecOrigin + vecDir * 16, 255, 0, 0, true, 5.0f );
		}
		pszImpactSound = "Weapon_Arrow.ImpactFlesh";
		bPlayerImpact = true;

		if ( !PositionArrowOnBone( pBox , pPlayer ) )
		{
			// This shouldn't happen
			UTIL_Remove( this );
			return;
		}

		Vector vecOrigin;
		QAngle vecAngles;
		int bone, iPhysicsBone;
		GetBoneAttachmentInfo( pBox, pPlayer, vecOrigin, vecAngles, bone, iPhysicsBone );

		// TODO: Redo the whole "BoltImpact" logic

		// CTFProjectile_Arrow::CheckRagdollPinned
		if( GetDamage() > pPlayer->GetHealth() )
		{
			// pPlayer->StopRagdollDeathAnim();
			Vector vForward;

			AngleVectors( GetAbsAngles(), &vForward );
			VectorNormalize ( vForward );

			UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vForward * 256, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0f )
			{
				//NDebugOverlay::Box( tr.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
				//NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

				if ( tr.m_pEnt == NULL || ( tr.m_pEnt && tr.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
				{
					CEffectData	data;

					data.m_vOrigin = tr.endpos;
					data.m_vNormal = vForward;
					data.m_nEntIndex = tr.fraction != 1.0f;
					data.m_iType = m_iType;
					data.m_nSkin = m_nSkin;
					DispatchEffect( "BoltImpact", data );
				}
			}
		}
		else
		{
			IGameEvent *event = gameeventmanager->CreateEvent( g_pszArrowHits[m_iType] );
			
			if ( event )
			{
				event->SetInt( "attachedEntity", pOther->entindex() );
				event->SetInt( "shooter", pAttacker->entindex() );
				event->SetInt( "boneIndexAttached", bone );
				event->SetFloat( "bonePositionX", vecOrigin.x );
				event->SetFloat( "bonePositionY", vecOrigin.y );
				event->SetFloat( "bonePositionZ", vecOrigin.z );
				event->SetFloat( "boneAnglesX", vecAngles.x );
				event->SetFloat( "boneAnglesY", vecAngles.y );
				event->SetFloat( "boneAnglesZ", vecAngles.z );
				
				gameeventmanager->FireEvent( event );
			}
		}
	}
	else if ( pOther->GetMoveType() == MOVETYPE_NONE )
	{	
		surfacedata_t *psurfaceData = physprops->GetSurfaceData( trHit.surface.surfaceProps );
		int iMaterial = psurfaceData->game.material;
		bool bArrowSound = false;

		if ( ( iMaterial == CHAR_TEX_CONCRETE ) || ( iMaterial == CHAR_TEX_TILE ) )
		{
			pszImpactSound = "Weapon_Arrow.ImpactConcrete";
			bArrowSound = true;
		}
		else if ( iMaterial == CHAR_TEX_WOOD )
		{
			pszImpactSound = "Weapon_Arrow.ImpactWood";
			bArrowSound = true;
		}
		else if ( ( iMaterial == CHAR_TEX_METAL ) || ( iMaterial == CHAR_TEX_VENT ) )
		{
			pszImpactSound = "Weapon_Arrow.ImpactMetal";
			bArrowSound = true;
		}

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vForward * 256, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0f )
		{
			//NDebugOverlay::Box( tr.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
			//NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr.m_pEnt == NULL || ( tr.m_pEnt && tr.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr.fraction != 1.0f;
				data.m_iType = m_iType;
				data.m_nSkin = m_nSkin;
				DispatchEffect( "BoltImpact", data );
			}
		}

		// If we didn't play a collision sound already, play a bullet collision sound for this prop
		if( !bArrowSound )
		{
			UTIL_ImpactTrace( &trHit, DMG_BULLET );
		}
		else
		{
			UTIL_ImpactTrace( &trHit, DMG_BULLET, "ImpactArrow" );
		}

		//UTIL_Remove( this );
	}
	else
	{
		// If we're an item with gibs (Arrow, Festive Arrow, Repair Claw) then gib.
		if ( ( m_iType == 0 ) || ( m_iType == 3 ) || ( m_iType == 2 ) )
			BreakArrow();
		UTIL_Remove( this );
	}

	// Play sound
	if ( pszImpactSound )
	{
		PlayImpactSound( ToTFPlayer( pAttacker ), pszImpactSound, bPlayerImpact );
	}

	int iCustomDamage = m_bFlame ? TF_DMG_CUSTOM_BURNING_ARROW : TF_DMG_CUSTOM_NONE;

	// Do damage.
	CTakeDamageInfo info( this, pAttacker, pWeapon, GetDamage(), GetDamageType(), iCustomDamage );
	CalculateBulletDamageForce( &info, pWeapon ? pWeapon->GetTFWpnData().iAmmoType : 0, vecDir, vecOrigin );
	info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
	pOther->DispatchTraceAttack( info, vecDir, &trHit );
	ApplyMultiDamage();
	
	if ( ( pPlayer && ( m_iType == 1 || m_iType == 4 ) ) && ( tf2v_healing_bolts.GetBool() || TFGameRules()->IsInMedievalMode() ) ) // Crossbow bolts can heal teammates if the convar is enabled, or it's medieval.
	{	
		CTFPlayer *pHealer = ToTFPlayer( pAttacker );
		// Is this someone we can heal?
		bool bCanBeHealed = false;
		bool bStealthed = pPlayer->m_Shared.InCond( TF_COND_STEALTHED );
		bool bDisguised = pPlayer->m_Shared.InCond( TF_COND_DISGUISED );

		// Check to see if they are restricted from healing.
		int nWeaponBlocksHealing = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nWeaponBlocksHealing, weapon_blocks_healing );

		// We can heal teammates and enemies that are disguised as teammates.
		if ( !bStealthed &&
			( pPlayer->InSameTeam( pHealer ) ||
			 ( bDisguised && pPlayer->m_Shared.GetDisguiseTeam() == pHealer->GetTeamNumber() ) ) && ( nWeaponBlocksHealing == 0 ) )
		{
			bCanBeHealed = true;
		}
		
		if (bCanBeHealed)
		{
			// We hit someone that can be healed, so heal them.
			int iHealthRestored = pPlayer->TakeHealth( ( GetDamage() * tf2v_healing_bolts_heal_factor.GetFloat() ), DMG_GENERIC ); // Bolts always heal double the damage.
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, iHealthRestored, mult_health_fromhealers );
			CTF_GameStats.Event_PlayerHealedOther( pHealer, iHealthRestored );

			IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
			if ( event )
			{
				event->SetInt( "patient", pPlayer->GetUserID() );
				event->SetInt( "healer", pHealer->GetUserID() );
				event->SetInt( "amount", iHealthRestored );
				
				gameeventmanager->FireEvent( event );
			}	
		}		
	}
	else if ( pOther->IsBaseObject() && pOther->GetTeamNumber() == ToTFPlayer(pAttacker)->GetTeamNumber() )
	{
		// We hit a friendly building.
		float flArrowheal = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(m_hLauncher.Get(), flArrowheal, arrow_heals_buildings);
		if (flArrowheal > 0)
		{
			// Heal our building.
			int	iAmountToHeal = min( (int)(flArrowheal), pOther->GetMaxHealth() - pOther->GetHealth() );
			float flHealthtoMetalRatio = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(m_hLauncher.Get(), flHealthtoMetalRatio, repair_health_to_metal_ratio_DISPLAY_ONLY);
			float flNewHealth;
			if ( flHealthtoMetalRatio != 0 )
			{
				// Deduct the metal from our pool.
				int iRepairCost = ceil( (float)( iAmountToHeal ) * (1 / flHealthtoMetalRatio) );
				// If it costs too much, downgrade how much we can heal by.
				if ( iRepairCost > ToTFPlayer(pAttacker)->GetBuildResources() )
				{
					iRepairCost = ToTFPlayer(pAttacker)->GetBuildResources();
				}
				ToTFPlayer(pAttacker)->RemoveBuildResources( iRepairCost );
				flNewHealth = min( pOther->GetMaxHealth(), pOther->GetHealth() + ( iRepairCost * flHealthtoMetalRatio ) );
			}
			else
			{
				// Don't worry about calculating out metal, just heal.
				flNewHealth = min( pOther->GetMaxHealth(), pOther->GetHealth() + iAmountToHeal );
			}

			pOther->SetHealth( flNewHealth );
		}
	}

	// Remove.
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::FlyThink(void)
{
	QAngle angles;

	VectorAngles(GetAbsVelocity(), angles);

	SetAbsAngles(angles);

	SetNextThink(gpGlobals->curtime + 0.1f);
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

	if ( ( m_iType == 1 || m_iType == 4 ) || m_iType == 2 )
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	if (m_iType != 5) // Don't allow grappling hooks to be deflected.
	{
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
		ChangeTeam( pDeflectedBy->GetTeamNumber() );
		if (m_iType != 0 && m_iType != 3)
		{
			m_nSkin = ( pDeflectedBy->GetTeamNumber() - 2 );
		}
		
		SetScorer( pDeflectedBy );

		// Change trail color.
		if ( m_hSpriteTrail.Get() )
		{
			UTIL_Remove( m_hSpriteTrail.Get() );
		}

		CreateTrail();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::CanHeadshot( void )
{
	// ( m_iType == TF_PROJECTILE_ARROW || m_iType == TF_PROJECTILE_FESTITIVE_ARROW )
	// The exception is that we translate this to our local type value. (0 and 3 respectively)
	return ( m_iType == 0 || m_iType == 3 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFProjectile_Arrow::GetTrailParticleName( void )
{

	const char *pszFormat = NULL;
	bool bLongTeamName = false;

	switch( m_iType )
	{
	case 3:
		pszFormat = "effects/repair_claw_trail_%s.vmt";
		bLongTeamName = true;
		break;
	case 1:
	case 4:
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
	CSpriteTrail *pTrail = CSpriteTrail::SpriteTrailCreate( GetTrailParticleName(), GetAbsOrigin(), true );

	if ( pTrail )
	{
		pTrail->FollowEntity( this );
		pTrail->SetTransparency( kRenderTransAlpha, -1, -1, -1, 255, kRenderFxNone );
		pTrail->SetStartWidth( m_iType == TF_PROJECTILE_BUILDING_REPAIR_BOLT ? 5.0f : 3.0f );
		pTrail->SetTextureResolution( 0.01f );
		pTrail->SetLifeTime( 0.3f );
		pTrail->TurnOn();

		pTrail->SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 3.0f, "RemoveThink" );

		m_hSpriteTrail.Set( pTrail );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::UpdateOnRemove( void )
{
	UTIL_Remove( m_hSpriteTrail.Get() );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Sends to the client information for arrow gibs
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::BreakArrow( void )
{
	SetMoveType( MOVETYPE_NONE, MOVECOLLIDE_DEFAULT );
	SetAbsVelocity( vec3_origin );
	SetSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );

	SetContextThink( &CTFProjectile_Arrow::RemoveThink, gpGlobals->curtime + 3.0, "ARROW_REMOVE_THINK" );

	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT(m_iType);
		WRITE_SHORT( m_nSkin );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::PositionArrowOnBone(mstudiobbox_t *pbox, CBaseAnimating *pAnim )
{
	matrix3x4_t *bones[MAXSTUDIOBONES];

	CStudioHdr *pStudioHdr = pAnim->GetModelPtr();	
	if ( !pStudioHdr )
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnim->GetHitboxSet() );

	if ( !set->numhitboxes || pbox->bone > 127 )
		return false;

	CBoneCache *pCache = pAnim->GetBoneCache();
	if ( !pCache )
		return false;

	pCache->ReadCachedBonePointers( bones, pStudioHdr->numbones() );
	
	Vector vecMins, vecMaxs, vecResult;
	TransformAABB( *bones[pbox->bone], pbox->bbmin, pbox->bbmax, vecMins, vecMaxs );
	vecResult = vecMaxs - vecMins;

	// This is a mess
	SetAbsOrigin( ( ( ( vecResult ) * 0.60000002f ) + vecMins ) + ( ( ( rand() / RAND_MAX ) *  vecResult ) * -0.2f ) );

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
void CTFProjectile_Arrow::CheckRagdollPinned( Vector &, Vector &, int, int, CBaseEntity *, int, int )
{
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
		SetNextClientThink( gpGlobals->curtime + 0.1f );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Arrow::ClientThink( void )
{
	if ( m_bAttachment && m_flDieTime < gpGlobals->curtime )
	{
		// Die
		SetNextClientThink( CLIENT_THINK_NEVER );
		Remove();
		return;
	}

	if ( m_bFlame && !bEmitting )
	{
		Light();
		SetNextClientThink( CLIENT_THINK_NEVER );
		return;
	}

	SetNextClientThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Arrow::Light( void )
{
	if ( ( ( IsDormant() || !m_bFlame ) ) && ( ( m_iType != 0 ) && ( m_iType != 3 ) ) )
		return;

	ParticleProp()->Create( "flying_flaming_arrow", PATTACH_ABSORIGIN_FOLLOW );
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
