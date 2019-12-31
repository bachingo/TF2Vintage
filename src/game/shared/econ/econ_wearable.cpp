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
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED( EconWearable, DT_EconWearable )

BEGIN_NETWORK_TABLE( CEconWearable, DT_EconWearable )
#ifdef GAME_DLL
	SendPropString( SENDINFO( m_ParticleName ) ),
	SendPropBool( SENDINFO( m_bExtraWearable ) ),
#else
	RecvPropString( RECVINFO( m_ParticleName ) ),
	RecvPropBool( RECVINFO( m_bExtraWearable ) ),
#endif
END_NETWORK_TABLE()

void CEconWearable::Spawn( void )
{
	InitializeAttributes();

	Precache();

	if ( m_bExtraWearable && GetItem()->GetStaticData() )
	{
		SetModel( GetItem()->GetStaticData()->extra_wearable );
	}
	else
	{
		SetModel( GetItem()->GetPlayerDisplayModel() );
	}

#if defined ( GAME_DLL )
	if (GetItem()->GetStaticData() )
		m_bItemFallsOff = GetItem()->GetStaticData()->itemfalloff;
#endif
	
	BaseClass::Spawn();

	AddEffects( EF_BONEMERGE );
	AddEffects( EF_BONEMERGE_FASTCULL );
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
	if ( pOwner->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iVisibleTeam = pOwner->m_Shared.GetDisguiseTeam();
	}

	switch ( iVisibleTeam )
	{
		case TF_TEAM_BLUE:
			return 1;
		case TF_TEAM_RED:
			return 0;
	}

	return m_nSkin;
}

void CEconWearable::UpdateWearableBodyGroups( CBasePlayer *pPlayer )
{
	PerTeamVisuals_t *visual = GetItem()->GetStaticData()->GetVisuals( GetTeamNumber() );
 	for ( unsigned int i = 0; i < visual->player_bodygroups.Count(); i++ )
	{
		const char *szBodyGroupName = visual->player_bodygroups.GetElementName(i);

		if ( szBodyGroupName )
		{
			int iBodyGroup = pPlayer->FindBodygroupByName( szBodyGroupName );
			int iBodyGroupValue = visual->player_bodygroups.Element(i);

			pPlayer->SetBodygroup( iBodyGroup, iBodyGroupValue );
		}
	}
}

void CEconWearable::SetParticle(const char* name)
{
#ifdef GAME_DLL
	Q_snprintf(m_ParticleName.GetForModify(), PARTICLE_MODIFY_STRING_SIZE, name);
#else
	Q_snprintf(m_ParticleName, PARTICLE_MODIFY_STRING_SIZE, name);
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
		FollowEntity( pPlayer, true );
		SetOwnerEntity( pPlayer );
		ChangeTeam( pPlayer->GetTeamNumber() );

		// Extra wearables don't provide attribute bonuses
		if ( !IsExtraWearable() )
			ReapplyProvision();
	}
}

void CEconWearable::UnEquip( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
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

#endif
