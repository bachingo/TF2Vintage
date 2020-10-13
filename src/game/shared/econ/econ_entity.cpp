//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "econ_entity.h"
#include "eventlist.h"
#include "datacache/imdlcache.h"

#ifdef GAME_DLL
#include "activitylist.h"
#include "tf_player.h"
#else
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "model_types.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_ScriptCreatedItem )
#else
EXTERN_SEND_TABLE( DT_ScriptCreatedItem )
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( EconEntity, DT_EconEntity )

BEGIN_NETWORK_TABLE( CEconEntity, DT_EconEntity )
#ifdef CLIENT_DLL
	RecvPropDataTable( RECVINFO_DT( m_AttributeManager ), 0, &REFERENCE_RECV_TABLE( DT_AttributeContainer ) ),
#else
	SendPropDataTable( SENDINFO_DT( m_AttributeManager ), &REFERENCE_SEND_TABLE( DT_AttributeContainer ) ),
#endif
END_NETWORK_TABLE()

CEconEntity::CEconEntity()
{
	m_pAttributes = this;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconEntity::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_AttributeManager.OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconEntity::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		InitializeAttributes();
	}

	BaseClass::OnDataChanged( updateType );
	m_AttributeManager.OnDataChanged( updateType );

	UpdateAttachmentModels();

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CEconItemView *pItem = GetItem();
		for ( int i = 0; i < TF_TEAM_COUNT; i++ )
		{
			PerTeamVisuals_t *pVisuals = pItem->GetStaticData()->GetVisuals( i );
			if( pVisuals )
			{
				const char *pszMaterial = pVisuals->GetMaterialOverride();
				if ( pszMaterial )
				{
					m_aMaterials[i].Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconEntity::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN )
	{
		C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
		if ( pAttach)
		{
			pAttach->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
		}
	}
	else
		BaseClass::FireEvent( origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN )
	{
		C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
		if ( pAttach)
		{
			pAttach->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::IsTransparent( void )
{
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer )
		return pPlayer->IsTransparent();

	return BaseClass::IsTransparent();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::ViewModel_IsTransparent( void )
{
	if ( m_hAttachmentParent && m_hAttachmentParent->IsTransparent() )
		return true;
	
	return IsTransparent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::ViewModel_IsUsingFBTexture( void )
{
	if ( m_hAttachmentParent && m_hAttachmentParent->UsesPowerOfTwoFrameBufferTexture() )
		return true;
	
	return UsesPowerOfTwoFrameBufferTexture();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::IsOverridingViewmodel( void )
{
	if ( GetMaterialOverride( GetTeamNumber() ) )
		return true;

	if ( !m_hAttachmentParent )
		return false;

	CEconItemDefinition *pStatic = GetItem()->GetStaticData();
	if ( pStatic == nullptr )
		return false;

	PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( GetTeamNumber() );
	if ( pVisuals == nullptr )
		return false;

	if ( !pVisuals->attached_models.IsEmpty() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_EconEntity::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int nRetval = 0;
	bool bHadOverride = false, bAttachToHands = false;;
	const int iTeam = GetTeamNumber();

	CEconItemDefinition *pItem = GetItem()->GetStaticData();
	if ( pItem )
		bAttachToHands = pItem->attach_to_hands == VMTYPE_TF2 || pItem->attach_to_hands_vm_only == VMTYPE_TF2;

	if ( m_hAttachmentParent && m_hAttachmentParent->IsTransparent() )
		nRetval = pViewmodel->DrawOverriddenViewmodel( flags );

	if ( flags & STUDIO_RENDER )
	{
		bHadOverride = GetMaterialOverride( iTeam ) != NULL;
		if ( bHadOverride )
		{
			modelrender->ForcedMaterialOverride( GetMaterialOverride( iTeam ) );
			// 0x200 is OR'd into flags here
		}

		if ( m_hAttachmentParent )
		{
			m_hAttachmentParent->RemoveEffects( EF_NODRAW );
			m_hAttachmentParent->DrawModel( flags );
			m_hAttachmentParent->AddEffects( EF_NODRAW );
		}

		if ( bAttachToHands && bHadOverride )
		{
			modelrender->ForcedMaterialOverride( NULL );
			bHadOverride = false;
		}
	}

	if ( !m_hAttachmentParent || !m_hAttachmentParent->IsTransparent() )
		nRetval = pViewmodel->DrawOverriddenViewmodel( flags );

	if ( bHadOverride )
		modelrender->ForcedMaterialOverride( NULL );

	return nRetval;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_EconEntity::InternalDrawModel( int flags )
{
	if ( GetMaterialOverride( GetTeamNumber() ) == nullptr || !( flags & STUDIO_RENDER ) )
		return BaseClass::InternalDrawModel( flags );

	modelrender->ForcedMaterialOverride( m_aMaterials[ GetTeamNumber() ] );
	int result = BaseClass::InternalDrawModel( flags );
	modelrender->ForcedMaterialOverride( NULL );

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( BaseClass::OnInternalDrawModel( pInfo ) )
	{
		DrawEconEntityAttachedModels( this, this, pInfo, AM_WORLDMODEL );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Stubbed out
//-----------------------------------------------------------------------------
void C_EconEntity::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	// NUB
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconEntity::UpdateAttachmentModels( void )
{
	m_Attachments.Purge();

	if ( GetItem()->GetStaticData() && AttachmentModelsShouldBeVisible() )
	{
		CEconItemDefinition *pItem = GetItem()->GetStaticData();

		PerTeamVisuals_t *pVisuals = pItem->GetVisuals( GetTeamNumber() );
		if ( pVisuals )
		{
			for ( int i=0; i<pVisuals->attached_models.Count(); ++i )
			{
				AttachedModel_t const &attachment = pVisuals->attached_models[i];
				int iMdlIndex = modelinfo->GetModelIndex( attachment.model );
				if ( iMdlIndex >= 0 )
				{
					AttachedModelData_t attachmentData;
					attachmentData.model = modelinfo->GetModel( iMdlIndex );
					attachmentData.modeltype = attachment.model_display_flags;

					m_Attachments.AddToTail( attachmentData );
				}
			}
		}

		if ( pItem->attach_to_hands || pItem->attach_to_hands_vm_only )
		{
			C_BasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
			if ( pPlayer && !pPlayer->ShouldDrawThisPlayer() )
			{
				CBaseViewModel *pViewmodel = pPlayer->GetViewModel();
				if ( pViewmodel )
				{
					/*C_ViewmodelAttachmentModel *pAddon = new C_ViewmodelAttachmentModel;
					if ( !pAddon )
						return;

					pAddon->SetOwner( this );

					if ( pAddon->InitializeAsClientEntity( GetItem()->GetPlayerDisplayModel(), RENDER_GROUP_VIEW_MODEL_OPAQUE ) )
					{
						pAddon->SetParent( pViewmodel );
						pAddon->SetLocalOrigin( vec3_origin );
						pAddon->UpdatePartitionListEntry();
						pAddon->CollisionProp()->UpdatePartition();
						pAddon->UpdateVisibility();

						m_hAttachmentParent = pAddon;
					}*/
				}
			}
			else
			{
				if ( m_hAttachmentParent )
					m_hAttachmentParent->Release();
			}
		}
	}
	else
	{
		if ( m_hAttachmentParent )
			m_hAttachmentParent->Release();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::GetAttachment( int iAttachment, Vector &absOrigin )
{
	if ( m_hAttachmentParent.Get() )
		return m_hAttachmentParent->GetAttachment( iAttachment, absOrigin );

	return BaseClass::GetAttachment( iAttachment, absOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::GetAttachment( int iAttachment, Vector &absOrigin, QAngle &absAngles )
{
	if ( m_hAttachmentParent.Get() )
		return m_hAttachmentParent->GetAttachment( iAttachment, absOrigin, absAngles );

	return BaseClass::GetAttachment( iAttachment, absOrigin, absAngles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconEntity::GetAttachment( int iAttachment, matrix3x4_t &matrix )
{
	if ( m_hAttachmentParent.Get() )
		return m_hAttachmentParent->GetAttachment( iAttachment, matrix );

	return BaseClass::GetAttachment( iAttachment, matrix );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconEntity::SetMaterialOverride( int iTeam, const char *pszMaterial )
{
	Assert( iTeam >= 0 && iTeam < TF_TEAM_COUNT );
	m_aMaterials[iTeam].Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconEntity::SetMaterialOverride( int iTeam, CMaterialReference &material )
{
	Assert( iTeam >= 0 && iTeam < TF_TEAM_COUNT );
	m_aMaterials[iTeam].Init( material );
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::UpdateModelToClass( void )
{
#if defined(TF_DLL) || defined(TF_VINTAGE)
	MDLCACHE_CRITICAL_SECTION();

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner == nullptr )
		return;


#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::PlayAnimForPlaybackEvent( wearableanimplayback_t iPlayback )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwnerEntity() );
	if ( pOwner == nullptr )
		return;

	CEconItemDefinition *pDefinition = GetItem()->GetStaticData();
	if ( pDefinition == NULL )
		return;

	int iTeamNum = pOwner->GetTeamNumber();
	PerTeamVisuals_t *pVisuals = pDefinition->GetVisuals( iTeamNum );
	if ( pVisuals == NULL )
		return;

	const int nNumActs = pVisuals->playback_activity.Count();
	for ( int i=0; i < nNumActs; ++i )
	{
		activity_on_wearable_t *pActivity = &pVisuals->playback_activity[i];
		if ( pActivity->playback != iPlayback || !pActivity->activity_name || !pActivity->activity_name[0] )
			continue;

		if ( pActivity->activity == kActivityLookup_Unknown )
			pActivity->activity = ActivityList_IndexForName( pActivity->activity_name );

		const int nSequence = SelectWeightedSequence( (Activity)pActivity->activity );
		if ( nSequence != -1 )
		{
			ResetSequence( nSequence );
			SetCycle( 0 );

			if ( IsUsingClientSideAnimation() )
				ResetClientsideFrame();

			break;
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::SetItem( CEconItemView const &pItem )
{
	m_AttributeManager.SetItem( pItem );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::HasItemDefinition( void ) const
{
	return ( GetItem()->GetItemDefIndex() >= 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut to get item ID.
//-----------------------------------------------------------------------------
int CEconEntity::GetItemID( void ) const
{
	return GetItem()->GetItemDefIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Derived classes need to override this.
//-----------------------------------------------------------------------------
void CEconEntity::GiveTo( CBaseEntity *pEntity )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add or remove this from owner's attribute providers list.
//-----------------------------------------------------------------------------
void CEconEntity::ReapplyProvision( void )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	CBaseEntity *pOldOwner = m_hOldOwner.Get();

	if ( pOwner != pOldOwner )
	{
		if ( pOldOwner )
		{
			m_AttributeManager.StopProvidingTo( pOldOwner );
		}

		if ( pOwner )
		{
			m_AttributeManager.ProvideTo( pOwner );
			m_hOldOwner = pOwner;
		}
		else
		{
			m_hOldOwner = NULL;
		}
	}
}

void CEconEntity::InitializeAttributes( void )
{
	m_AttributeManager.InitializeAttributes( this );
	m_AttributeManager.SetProvidrType( PROVIDER_WEAPON );
}

//-----------------------------------------------------------------------------
// Purpose: Update visible bodygroups
//-----------------------------------------------------------------------------
void CEconEntity::UpdatePlayerBodygroups( int bOnOff )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return;

	// bodygroup enabling/disabling
	CEconItemDefinition *pStatic = GetItem()->GetStaticData();
	if ( !pStatic )
		return;

	PerTeamVisuals_t *pVisuals = pStatic->GetVisuals();
	if ( !pVisuals )
		return;

	for ( int i = 0; i < pPlayer->GetNumBodyGroups(); i++ )
	{
		unsigned int nIndex = pVisuals->player_bodygroups.Find( pPlayer->GetBodygroupName( i ) );
		if ( !pVisuals->player_bodygroups.IsValidIndex( nIndex ) )
			continue;
		
		int nState = pVisuals->player_bodygroups.Element( nIndex );
		if ( nState != bOnOff )
			continue;

		pPlayer->SetBodygroup( i, bOnOff );
	}

	const int iTeamNum = pPlayer->GetTeamNumber();
	PerTeamVisuals_t *pTeamVisuals = pStatic->GetVisuals( iTeamNum );
	if ( !pTeamVisuals )
		return;

	// world model overrides
	if ( pTeamVisuals->wm_bodygroup_override != -1 && pTeamVisuals->wm_bodygroup_state_override != -1 )
		pPlayer->SetBodygroup( pTeamVisuals->wm_bodygroup_override, pTeamVisuals->wm_bodygroup_state_override );

	// view model overrides
	if ( pTeamVisuals->vm_bodygroup_override != -1 && pTeamVisuals->vm_bodygroup_state_override != -1 )
	{
		CBaseViewModel *pVM = pPlayer->GetViewModel();
		if ( pVM && pVM->GetModelPtr() )
			pVM->SetBodygroup( pTeamVisuals->vm_bodygroup_override, pTeamVisuals->vm_bodygroup_state_override );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconEntity::UpdateOnRemove( void )
{
	SetOwnerEntity( NULL );
	ReapplyProvision();
	BaseClass::UpdateOnRemove();
}

#ifdef CLIENT_DLL
void DrawEconEntityAttachedModels( C_BaseAnimating *pAnimating, C_EconEntity *pEconEntity, ClientModelRenderInfo_t const *pInfo, int iModelType )
{
	if ( pAnimating && pEconEntity && pInfo )
	{
		// a check for pInfo->flags & 0x200 goes here

		for ( int i=0; i<pEconEntity->m_Attachments.Count(); ++i )
		{
			if ( pEconEntity->m_Attachments[i].model && ( pEconEntity->m_Attachments[i].modeltype & iModelType ) )
			{
				ClientModelRenderInfo_t newInfo;
				V_memcpy( &newInfo, pInfo, sizeof( ClientModelRenderInfo_t ) );
				newInfo.pRenderable = (IClientRenderable *)pAnimating;
				newInfo.instance = MODEL_INSTANCE_INVALID;
				newInfo.entity_index = pAnimating->entindex();
				newInfo.pModel = pEconEntity->m_Attachments[i].model;
				newInfo.pModelToWorld = &newInfo.modelToWorld;

				// Turns the origin + angles into a matrix
				AngleMatrix( newInfo.angles, newInfo.origin, newInfo.modelToWorld );

				DrawModelState_t state;
				matrix3x4_t *pBoneToWorld = NULL;
				bool bMarkAsDrawn = modelrender->DrawModelSetup( newInfo, &state, NULL, &pBoneToWorld );
				pAnimating->DoInternalDrawModel( &newInfo, ( bMarkAsDrawn && ( newInfo.flags & STUDIO_RENDER ) ) ? &state : NULL, pBoneToWorld );
			}
		}
	}
}
#endif
