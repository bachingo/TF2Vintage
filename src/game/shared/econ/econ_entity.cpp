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

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( C_EconEntity )
	DEFINE_PRED_TYPEDESCRIPTION( m_AttributeManager, CAttributeContainer ),
END_PREDICTION_DATA()
#endif

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
	BaseClass::OnDataChanged( updateType );

	m_AttributeManager.OnDataChanged( updateType );

	UpdateAttachmentModels();
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
bool C_EconEntity::IsOverridingViewmodel( void ) const
{
	if ( GetMaterialOverride( GetTeamNumber() ) )
		return true;

	if ( !m_hAttachmentParent )
		return false;

	CEconItemDefinition *pStatic = GetItem()->GetStaticData();
	if ( pStatic == nullptr )
		return false;

	PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( GetTeamNumber() );
	if ( !pVisuals->attached_models.IsEmpty() )
		return true;

	return false;
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
	m_aAttachments.RemoveAll();

	if ( GetItem()->GetStaticData() )
	{
		CEconItemDefinition *pItem = GetItem()->GetStaticData();

		if ( AttachmentModelsShouldBeVisible() )
		{
			PerTeamVisuals_t *pVisuals = pItem->GetVisuals( GetTeamNumber() );
			if ( pVisuals )
			{
				for ( int i=0; i<pVisuals->attached_models.Count(); ++i )
				{
					AttachedModel_t attachment = pVisuals->attached_models[i];
					int iMdlIndex = modelinfo->GetModelIndex( attachment.model );
					if ( iMdlIndex >= 0 )
					{
						AttachedModelData_t attachmentData;
						attachmentData.model = modelinfo->GetModel( iMdlIndex );
						attachmentData.modeltype = attachment.model_display_flags;

						m_aAttachments.AddToTail( attachmentData );
					}
				}
			}

			if ( pItem->attach_to_hands || pItem->attach_to_hands_vm_only )
			{
				C_BasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
				if ( pPlayer && pPlayer->IsAlive() && !pPlayer->ShouldDrawThisPlayer() )
				{
					if ( !m_hAttachmentParent || m_hAttachmentParent != GetMoveParent() )
					{
						// Some validation or something
						return;
					}

					CBaseViewModel *pViewmodel = pPlayer->GetViewModel();
					if ( !pViewmodel )
					{
						// Same thing as above
						return;
					}

					C_ViewmodelAttachmentModel *pAddon = new C_ViewmodelAttachmentModel;
					if ( !pAddon )
						return;

					if ( pAddon->InitializeAsClientEntity( GetItem()->GetPlayerDisplayModel(), RENDER_GROUP_VIEW_MODEL_OPAQUE ) )
					{
						pAddon->SetOwnerEntity( this );
						pAddon->SetParent( pViewmodel );
						pAddon->SetLocalOrigin( vec3_origin );
						pAddon->UpdatePartitionListEntry();
						pAddon->CollisionProp()->UpdatePartition();
						pAddon->UpdateVisibility();

						m_hAttachmentParent = pAddon;
					}
				}
				else
				{
					if ( m_hAttachmentParent )
						m_hAttachmentParent->Release();
				}
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
	if ( iTeam < 4 )
		m_aMaterials[iTeam].Init( pszMaterial, "ClientEffect textures", true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconEntity::SetMaterialOverride( int iTeam, CMaterialReference &material )
{
	if ( iTeam < 4 )
		m_aMaterials[iTeam].Init( material );
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::UpdateModelToClass( void )
{
	MDLCACHE_CRITICAL_SECTION();

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner == nullptr )
		return;


}

#endif

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
}

//-----------------------------------------------------------------------------
// Purpose: Update visible bodygroups
//-----------------------------------------------------------------------------
void CEconEntity::UpdatePlayerBodygroups( void )
{
	CTFPlayer *pPlayer = dynamic_cast < CTFPlayer * >( GetOwnerEntity() );

	if ( !pPlayer )
	{
		return;
	}

	// bodygroup enabling/disabling
	CEconItemDefinition *pStatic = GetItem()->GetStaticData();
	if ( pStatic )
	{
		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals();
		if ( pVisuals )
		{
			for ( int i = 0; i < pPlayer->GetNumBodyGroups(); i++ )
			{
				unsigned int index = pVisuals->player_bodygroups.Find( pPlayer->GetBodygroupName( i ) );
				if ( pVisuals->player_bodygroups.IsValidIndex( index ) )
				{
					bool bTrue = pVisuals->player_bodygroups.Element( index );
					if ( bTrue )
					{
						pPlayer->SetBodygroup( i , 1 );
					}
					else
					{
						pPlayer->SetBodygroup( i , 0 );
					}
				}
			}
		}
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
		for ( int i=0; i<pEconEntity->m_aAttachments.Count(); ++i )
		{
			if ( pEconEntity->m_aAttachments[i].model && ( pEconEntity->m_aAttachments[i].modeltype & iModelType ) )
			{
				ClientModelRenderInfo_t newInfo;
				V_memcpy( &newInfo, pInfo, sizeof( ClientModelRenderInfo_t ) );
				newInfo.pRenderable = (IClientRenderable *)pAnimating;
				newInfo.instance = MODEL_INSTANCE_INVALID;
				newInfo.entity_index = pAnimating->entindex();
				newInfo.pModel = pEconEntity->m_aAttachments[i].model;
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
