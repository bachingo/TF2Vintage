//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the Sticky Bolt code. This constraints ragdolls to the world
//			after being hit by a crossbow bolt. If something here is acting funny
//			let me know - Adrian.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "fx.h"
#include "decals.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "engine/IEngineTrace.h"
#include "vphysics/constraints.h"
#include "engine/ivmodelinfo.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "engine/ivdebugoverlay.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_player.h"
#include "tf_shareddefs.h"


const char *g_pszArrowModelClient[] =
{
	"models/weapons/w_models/w_arrow.mdl",
	"models/weapons/w_models/w_syringe_proj.mdl",
	"models/weapons/w_models/w_repair_claw.mdl",
	"models/weapons/w_models/w_arrow_xmas.mdl",
	"models/weapons/c_models/c_crusaders_crossbow/c_crusaders_crossbow_xmas_proj.mdl",
	"models/weapons/c_models/c_grapple_proj.mdl",
};

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IPhysicsSurfaceProps *physprops;
IPhysicsObject *GetWorldPhysObject( void );

extern ITempEnts* tempents;

class CRagdollBoltEnumerator : public IPartitionEnumerator
{
public:
	//Forced constructor   
	CRagdollBoltEnumerator( Ray_t& shot, Vector vOrigin )
	{
		m_rayShot = shot;
		m_vWorld = vOrigin;
	}

	//Actual work code
	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

		if ( pModel == NULL )
			return ITERATION_CONTINUE;

		trace_t tr;
		enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pModel, &tr );

		IPhysicsObject	*pPhysicsObject = NULL;
		
		//Find the real object we hit.
		if( tr.physicsbone >= 0 )
		{
			//Msg( "\nPhysics Bone: %i\n", tr.physicsbone );
			if ( pModel->m_pRagdoll )
			{
				CRagdoll *pCRagdoll = dynamic_cast < CRagdoll * > ( pModel->m_pRagdoll );

				if ( pCRagdoll )
				{
					ragdoll_t *pRagdollT = pCRagdoll->GetRagdoll();

					if ( tr.physicsbone < pRagdollT->listCount )
					{
						pPhysicsObject = pRagdollT->list[tr.physicsbone].pObject;
					}
				}
			}
		}

		if ( pPhysicsObject == NULL )
			return ITERATION_CONTINUE;

		if ( tr.fraction < 1.0 )
		{
			IPhysicsObject *pReference = GetWorldPhysObject();

			if ( pReference == NULL || pPhysicsObject == NULL )
				 return ITERATION_CONTINUE;
			
			float flMass = pPhysicsObject->GetMass();
			pPhysicsObject->SetMass( flMass * 2 );

			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
		
			pReference->WorldToLocal( &ballsocket.constraintPosition[0], m_vWorld );
			pPhysicsObject->WorldToLocal( &ballsocket.constraintPosition[1], tr.endpos );
	
			physenv->CreateBallsocketConstraint( pReference, pPhysicsObject, NULL, ballsocket );

			//Play a sound
			CPASAttenuationFilter filter( pEnt );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName =  "Weapon_Crossbow.BoltSkewer";
			ep.m_flVolume = 1.0f;
			ep.m_SoundLevel = SNDLVL_NORM;
			ep.m_pOrigin = &pEnt->GetAbsOrigin();

			C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, ep );
	
			return ITERATION_STOP;
		}

		return ITERATION_CONTINUE;
	}

private:
	Ray_t	m_rayShot;
	Vector  m_vWorld;
};

void CreateCrossbowBolt( const Vector &vecOrigin, const Vector &vecDirection )
{
	const model_t *pModel = engine->LoadModel( "models/crossbow_bolt.mdl" );

	QAngle vAngles;
	VectorAngles( vecDirection, vAngles );

	tempents->SpawnTempModel( pModel, vecOrigin - vecDirection * 8, vAngles, Vector( 0, 0, 0 ), 30.0f, FTENT_NONE );
}

void StickRagdollNow( const Vector &vecOrigin, const Vector &vecDirection )
{
	Ray_t	shotRay;
	trace_t tr;
	
	UTIL_TraceLine( vecOrigin, vecOrigin + vecDirection * 16, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.surface.flags & SURF_SKY )
		return;

	Vector vecEnd = vecOrigin - vecDirection * 128;

	shotRay.Init( vecOrigin, vecEnd );

	CRagdollBoltEnumerator ragdollEnum( shotRay, vecOrigin );
	partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );
	
	CreateCrossbowBolt( vecOrigin, vecDirection );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void StickyBoltCallback( const CEffectData &data )
{
	 StickRagdollNow( data.m_vOrigin, data.m_vNormal );
}

DECLARE_CLIENT_EFFECT( "BoltImpact", StickyBoltCallback );


void CreateTFCrossbowBolt( const Vector &vecOrigin, const Vector &vecDirection, int iProjType, byte nSkin )
{
	Assert( iProjType > 0 && iProjType < TF_NUM_PROJECTILES );

	const char *pszModel; float flModelScale;
	switch ( iProjType )
	{
		case TF_PROJECTILE_ARROW:
			pszModel = g_pszArrowModelClient[0];
			flModelScale = 1.f;
			break;
		case TF_PROJECTILE_HEALING_BOLT:
			pszModel = g_pszArrowModelClient[1];
			flModelScale = 1.6f;
			break;
		case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
			pszModel = g_pszArrowModelClient[2];
			flModelScale = 1.f;
			break;
		case TF_PROJECTILE_FESTIVE_ARROW:
			pszModel = g_pszArrowModelClient[3];
			flModelScale = 1.f;
			break;
		case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
			pszModel = g_pszArrowModelClient[4];
			flModelScale = 1.6f;
			break;
		case TF_PROJECTILE_GRAPPLINGHOOK:
			pszModel = g_pszArrowModelClient[5];
			flModelScale = 1.f;
			break;
		default:
			Warning( " Unsupported Projectile type in CreateTFCrossbowBolt - %d\n\n", iProjType );
			return;
	}
	const model_t *pModel = engine->LoadModel( pszModel );

	QAngle vAngles;
	VectorAngles( vecDirection, vAngles );

	C_LocalTempEntity *pTemp = tempents->SpawnTempModel( pModel, vecOrigin - vecDirection * 5.f, vAngles, vec3_origin, TEMP_OBJECT_LIFETIME, FTENT_NONE );
	if ( pTemp )
	{
		pTemp->SetModelScale( flModelScale );
		pTemp->m_nSkin = nSkin;
	}
}

void StickTFRagdollNow( const Vector &vecOrigin, const Vector &vecDirection, ClientEntityHandle_t const &pEntity, int iBone, int iPhysicsBone, int iOwner, int iHitGroup, int iVictim, int iProjType, byte nSkin )
{
	trace_t tr;
	UTIL_TraceLine( vecOrigin - vecDirection * 16.f, vecOrigin + vecDirection * 64.f, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.surface.flags & SURF_SKY )
		return;

	C_BaseAnimating *pModel = dynamic_cast<C_BaseAnimating *>( ClientEntityList().GetBaseEntityFromHandle( pEntity ) );
	if ( pModel && pModel->m_pRagdoll )
	{
		IPhysicsObject *pPhysics = NULL;

		ragdoll_t *pRagdoll = pModel->m_pRagdoll->GetRagdoll();
		if ( iPhysicsBone < pRagdoll->listCount )
		{
			pPhysics = pRagdoll->list[ iPhysicsBone ].pObject;
		}

		if ( GetWorldPhysObject() && pPhysics )
		{
			Vector vecPhyOrigin; QAngle vecPhyAngle;
			pPhysics->GetPosition( &vecPhyOrigin, &vecPhyAngle );

			const float flRand = (float)( rand() / VALVE_RAND_MAX );
			vecPhyOrigin = vecOrigin - ( vecDirection * flRand * 7.f + vecDirection * 7.f );
			pPhysics->SetPosition( vecPhyOrigin, vecPhyAngle, true );

			pPhysics->EnableMotion( false );

			float flMaxMass = 0;
			for ( int i = 0; i < pRagdoll->listCount; i++ )
				flMaxMass = Max( flMaxMass, pRagdoll->list[i].pObject->GetMass() );

			// normalize masses across all bones to prevent attempts at breaking constraint
			int nIndex = pRagdoll->list[ iPhysicsBone ].parentIndex;
			while ( nIndex >= 0 )
			{
				if ( pRagdoll->list[ nIndex ].pConstraint )
				{
					const float flMass = Max( pRagdoll->list[ nIndex ].pObject->GetMass(), flMaxMass );
					pRagdoll->list[ nIndex ].pObject->SetMass( flMass );
				}

				nIndex = pRagdoll->list[ nIndex ].parentIndex;
			}
		}
	}

	UTIL_ImpactTrace( &tr, DMG_GENERIC );
	CreateTFCrossbowBolt( vecOrigin, vecDirection, iProjType, nSkin );

	// Notify achievements
	if ( iHitGroup == HITGROUP_HEAD )
	{
		C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->entindex() == iOwner )
		{
			C_TFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictim ) );
			if ( pVictim && pVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_pinned" );
				if ( event )
					gameeventmanager->FireEventClientSide( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void TFStickyBoltCallback( const CEffectData &data )
{
	// This is a mess
	StickTFRagdollNow( data.m_vOrigin, 
					   data.m_vNormal,
					   data.m_hEntity,
					   0,
					   data.m_nMaterial,
					   data.m_nHitBox,
					   data.m_nDamageType,
					   data.m_nSurfaceProp,
					   data.m_fFlags,
					   data.m_nColor );
}

DECLARE_CLIENT_EFFECT( "TFBoltImpact", TFStickyBoltCallback );
