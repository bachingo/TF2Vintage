//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "econ_entity.h"
#include "eventlist.h"

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
	RecvPropDataTable( RECVINFO_DT( m_Item ), 0, &REFERENCE_RECV_TABLE( DT_ScriptCreatedItem ) ),
	RecvPropDataTable( RECVINFO_DT( m_AttributeManager ), 0, &REFERENCE_RECV_TABLE( DT_AttributeContainer ) ),
#else
	SendPropDataTable( SENDINFO_DT( m_Item ), &REFERENCE_SEND_TABLE( DT_ScriptCreatedItem ) ),
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
void CEconEntity::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_AttributeManager.OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_AttributeManager.OnDataChanged( updateType );

	UpdateAttachmentModels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
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
bool CEconEntity::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
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
bool CEconEntity::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( BaseClass::OnInternalDrawModel( pInfo ) )
	{
		DrawEconEntityAttachedModels( this, this, pInfo, AM_WORLDMODEL );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	// NUB
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::UpdateAttachmentModels( void )
{
	m_aAttachments.Purge();

	if ( GetItem() )
	{
		CEconItemDefinition *pItem = m_Item.GetStaticData();

		if ( AttachmentModelsShouldBeVisible() )
		{
			EconItemVisuals *pVisuals = pItem->GetVisuals( GetTeamNumber() );
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

			C_BasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
			if ( pPlayer && pPlayer->IsAlive() && !pPlayer->ShouldDrawThisPlayer() )
			{
				if ( m_hAttachmentParent )
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

				if ( pAddon->InitializeAsClientEntity( m_Item.GetPlayerDisplayModel(), RENDER_GROUP_VIEW_MODEL_TRANSLUCENT ) )
				{
					pAddon->SetParent( pViewmodel );
					pAddon->SetLocalOrigin( vec3_origin );
					pAddon->SetViewmodel( (C_TFViewModel *)pViewmodel );
					pAddon->UpdatePartitionListEntry();
					pAddon->CollisionProp()->MarkPartitionHandleDirty();
					pAddon->UpdateVisibility();

					m_hAttachmentParent = pAddon;

					return;
				}
			}
			else
			{
				if ( m_hAttachmentParent )
					m_hAttachmentParent->Release();
			}
		}
	}
	
	if ( m_hAttachmentParent )
		m_hAttachmentParent->Release();
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::SetItem( CEconItemView &newItem )
{
	m_Item = newItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CEconEntity::GetItem( void )
{
	return &m_Item;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::HasItemDefinition( void ) const
{
	return ( m_Item.GetItemDefIndex() >= 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut to get item ID.
//-----------------------------------------------------------------------------
int CEconEntity::GetItemID( void )
{
	return m_Item.GetItemDefIndex();
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
			m_AttributeManager.ProviteTo( pOwner );
			m_hOldOwner = pOwner;
		}
		else
		{
			m_hOldOwner = NULL;
		}
	}
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
	CEconItemDefinition *pStatic = m_Item.GetStaticData();
	if ( pStatic )
	{
		EconItemVisuals *pVisuals =	pStatic->GetVisuals();
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

CEconEntity::~CEconEntity()
{

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
				ClientModelRenderInfo_t newInfo( *pInfo );
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