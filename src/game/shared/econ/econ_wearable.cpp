//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//========================================================================//

#include "cbase.h"
#include "econ_wearable.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#include "vcollide.h"
#include "vcollide_parse.h"

extern ConVar r_propsmaxdist;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED( EconWearable, DT_EconWearable )

BEGIN_NETWORK_TABLE( CEconWearable, DT_EconWearable )
#ifdef GAME_DLL
	SendPropString( SENDINFO( m_ParticleName ) ),
#else
	RecvPropString( RECVINFO( m_ParticleName ) ),
#endif
END_NETWORK_TABLE()

void CEconWearable::Spawn( void )
{
	InitializeAttributes();

	Precache();

	if ( GetItem()->GetPlayerDisplayModel() )
	{
		SetModel( GetItem()->GetPlayerDisplayModel() );
	}
	
	BaseClass::Spawn();

	AddEffects( EF_BONEMERGE|EF_BONEMERGE_FASTCULL );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	SetBlocksLOS( false );
}

int CEconWearable::GetSkin( void )
{
	if ( GetItem() && GetItem()->GetSkin( GetTeamNumber(), false ) > -1 )
		return GetItem()->GetSkin( GetTeamNumber(), false );

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner == nullptr )
		return 0;

	int iVisibleTeam = GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
#ifdef CLIENT_DLL
	if ( pOwner->m_Shared.InCond( TF_COND_DISGUISED ) && pOwner->IsEnemyPlayer() )
	{
		iVisibleTeam = pOwner->m_Shared.GetDisguiseTeam();
	}
#endif

	switch ( iVisibleTeam )
	{
		case TF_TEAM_RED:
			return 0;
			break;

		case TF_TEAM_BLUE:
			return 1;
			break;

		case TF_TEAM_GREEN:
			return 2;
			break;

		case TF_TEAM_YELLOW:
			return 3;
			break;

		default:
			return 0;
			break;
	}

	return m_nSkin;
}

void CEconWearable::UpdateWearableBodyGroups( CBasePlayer *pPlayer )
{
	PerTeamVisuals_t *pVisuals = GetItem()->GetStaticData()->GetVisuals( GetTeamNumber() );
	if( pVisuals )
	{
		for ( unsigned int i = 0; i < pVisuals->player_bodygroups.Count(); i++ )
		{
			const char *szBodyGroupName = pVisuals->player_bodygroups.Key(i);

			if ( szBodyGroupName )
			{
				int iBodyGroup = pPlayer->FindBodygroupByName( szBodyGroupName );
				int iBodyGroupValue = pVisuals->player_bodygroups.Element(i);

				pPlayer->SetBodygroup( iBodyGroup, iBodyGroupValue );
			}
		}
	}
}

int CEconWearable::GetDropType( void )
{
	if ( GetItem() && GetItem()->GetStaticData() )
		return GetItem()->GetStaticData()->drop_type;
	
	return 0;
}


int CEconWearable::GetLoadoutSlot(void)
{
	if ( GetItem() && GetItem()->GetStaticData() )
		return GetItem()->GetStaticData()->item_slot;

	return TF_LOADOUT_SLOT_MISC1;
}


void CEconWearable::SetParticle(const char* name)
{
#ifdef GAME_DLL
	Q_strncpy( m_ParticleName.GetForModify(), name, PARTICLE_MODIFY_STRING_SIZE );
#else
	V_strcpy_safe( m_ParticleName, name );
#endif
}

void CEconWearable::GiveTo( CBaseEntity *pEntity )
{
#ifdef GAME_DLL
	CBasePlayer *pPlayer = ToBasePlayer( pEntity );

	if ( pPlayer )
	{
		pPlayer->EquipWearable( this );
	}
#endif
}

void CEconWearable::RemoveFrom( CBaseEntity *pEntity )
{
#ifdef GAME_DLL
	CBasePlayer *pPlayer = ToBasePlayer( pEntity );

	if ( pPlayer )
	{
		pPlayer->RemoveWearable( this );
	}
#endif
}

#ifdef GAME_DLL
void CEconWearable::Equip( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
		SetAbsVelocity( vec3_origin );

		CBaseEntity *pToFollow = pPlayer;
		if ( IsViewModelWearable() )
			pToFollow = pPlayer->GetViewModel();

		FollowEntity( pToFollow, true );

		SetTouch( NULL );
		SetOwnerEntity( pPlayer );
		ChangeTeam( pPlayer->GetTeamNumber() );
		ReapplyProvision();

	#ifdef GAME_DLL
		UpdateModelToClass();
		UpdatePlayerBodygroups( TURN_ON_BODYGROUPS );
		PlayAnimForPlaybackEvent( WEARABLEANIM_EQUIP );
	#endif
	}
}

void CEconWearable::UnEquip( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
	#ifdef GAME_DLL
		UpdatePlayerBodygroups( TURN_OFF_BODYGROUPS );
	#endif

		StopFollowingEntity();

		SetOwnerEntity( NULL );
		ReapplyProvision();
	}
}
#else

void CEconWearable::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );
	if ( type == DATA_UPDATE_DATATABLE_CHANGED )
	{
		if (Q_stricmp(m_ParticleName, "") && !m_pUnusualParticle)
		{
			m_pUnusualParticle = ParticleProp()->Create(m_ParticleName, PATTACH_ABSORIGIN_FOLLOW);
		}
	}
}

ShadowType_t CEconWearable::ShadowCastType( void )
{
	if ( ShouldDraw() )
	{
		return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
	}

	return SHADOWS_NONE;
}

bool CEconWearable::ShouldDraw( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwnerEntity() );

	if ( !pOwner )
		return false;

	if ( !pOwner->ShouldDrawThisPlayer() )
		return false;

	if ( !pOwner->IsAlive() )
		return false;

	return BaseClass::ShouldDraw();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconWearableGib::CEconWearableGib()
{
	m_bAttachedModel = false;
	m_flFadeTime = -1.0f;
}

CEconWearableGib::~CEconWearableGib()
{
	PhysCleanupFrictionSounds( this );
	VPhysicsDestroyObject();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CollideType_t CEconWearableGib::GetCollideType( void )
{
	return ENTITY_SHOULD_RESPOND;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconWearableGib::ImpactTrace(trace_t *pTrace, int dmgBits, char const *szWeaponName)
{
	if ( m_pPhysicsObject == nullptr )
		return;

	const Vector vecDir = pTrace->endpos - pTrace->startpos;
	if ( dmgBits & DMG_BLAST )
	{
		const Vector vecVelocity = vecDir * 500.0f;
		m_pPhysicsObject->ApplyForceCenter( vecVelocity );
	}
	else
	{
		const Vector vecWorldOffset = pTrace->startpos + ( pTrace->fraction * vecDir );
		const Vector vecVelocity = vecDir.Normalized() * 4000.0f;
		m_pPhysicsObject->ApplyForceOffset( vecVelocity, vecWorldOffset );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconWearableGib::Spawn( void )
{
	BaseClass::Spawn();
	m_takedamage = DAMAGE_EVENTS_ONLY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconWearableGib::SpawnClientEntity( void )
{
	if ( IsDynamicModelLoading() )
	{
		FinishModelInitialization();
		return;
	}

	m_bDynamicLoad = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CEconWearableGib::OnNewModel( void )
{
	CStudioHdr *pStudio = BaseClass::OnNewModel();

	if ( m_bDynamicLoad )
	{
		if ( !IsDynamicModelLoading() )
			FinishModelInitialization();
	}

	return pStudio;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconWearableGib::ClientThink( void )
{
	if ( m_flFadeTime > 0.0f )
	{
		if ( ( m_flFadeTime - gpGlobals->curtime ) < 1.0f )
		{
			if ( m_flFadeTime <= gpGlobals->curtime )
			{
				Release();
				return;
			}

			const float flAlpha = ( m_flFadeTime - gpGlobals->curtime ) / 1;
			SetRenderMode( kRenderTransTexture );
			SetRenderColorA( RoundFloatToByte( flAlpha * 255 ) );
		}
	}

	if ( m_flFadeTime <= 0.0f )
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
	else
	{
		if( ( m_flFadeTime - gpGlobals->curtime ) < 1.0f )
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		else
			SetNextClientThink( m_flFadeTime - 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconWearableGib::StartFadeOut( float flTime )
{
	m_flFadeTime = gpGlobals->curtime + flTime + 1.0f;
	if ( m_flFadeTime <= 0.0f )
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
	else
	{
		if( ( m_flFadeTime - gpGlobals->curtime ) < 1.0f )
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		else
			SetNextClientThink( m_flFadeTime - 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconWearableGib::FinishModelInitialization( void )
{
	if ( m_flFadeTime <= 0.0f )
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
	else
	{
		if( ( m_flFadeTime - gpGlobals->curtime ) < 1.0f )
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		else
			SetNextClientThink( m_flFadeTime - 1.0f );
	}

	const model_t *pModel = GetModel();
	if ( pModel )
	{
		Vector vecMins, vecMaxs;
		modelinfo->GetModelBounds( pModel, vecMins, vecMaxs );

		SetCollisionBounds( vecMins, vecMaxs );
	}

	if ( !m_bAttachedModel )
	{
		solid_t solid;
		if ( !PhysModelParseSolid( solid, this, GetModelIndex() ) )
		{
			DevMsg( "CEconWearableGib::FinishModelInitialization: PhysModelParseSolid failed for entity %i.\n", GetModelIndex() );
			return;
		}

		m_pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
		if ( m_pPhysicsObject == nullptr )
		{
			DevMsg( "CEconWearableGib::FinishModelInitialization: VPhysicsInitNormal() failed for %s.\n", GetModelName() );
			return;
		}
	}

	if ( m_fadeMinDist < 0.0f )
	{
		m_fadeMaxDist = r_propsmaxdist.GetFloat();
		m_fadeMinDist = r_propsmaxdist.GetFloat() * 0.75f;
	}

	Spawn();

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	UpdatePartitionListEntry();
	CollisionProp()->UpdatePartition();

	SetBlocksLOS( false );
	CreateShadow();
	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconWearableGib::Initialize( bool bAttached )
{
	m_bAttachedModel = bAttached;

	const char *model = STRING( GetModelName() );
	if( InitializeAsClientEntity( model, RENDER_GROUP_OPAQUE_ENTITY ) )
		return true;

	return false;
}

#endif
