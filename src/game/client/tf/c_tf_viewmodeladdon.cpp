//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_tf_viewmodeladdon.h"
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "model_types.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_drawothermodels;

CBaseEntity *C_ViewmodelAttachmentModel::GetOwnerViaInterface( void )
{
	return m_hOwner;
}

int C_ViewmodelAttachmentModel::InternalDrawModel( int flags )
{
	Assert( m_ViewModel );
	CMatRenderContextPtr pRenderContext(materials);

	if (m_ViewModel->ShouldFlipViewModel())
		pRenderContext->CullMode(MATERIAL_CULLMODE_CW);

	int ret = BaseClass::InternalDrawModel(flags);

	pRenderContext->CullMode(MATERIAL_CULLMODE_CCW);

	return ret;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool C_ViewmodelAttachmentModel::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( BaseClass::OnPostInternalDrawModel( pInfo ) )
	{
		if ( m_hOwner )
			DrawEconEntityAttachedModels( this, m_hOwner, pInfo, AM_VIEWMODEL );

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: We're overriding this because DrawModel keeps calling the FollowEntity DrawModel function
//			which in this case is CTFViewModel::DrawModel.
//			This is basically just a straight copy of C_BaseAnimating::DrawModel, without the FollowEntity part
//-----------------------------------------------------------------------------
int C_ViewmodelAttachmentModel::DrawOverriddenViewmodel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	int drawn = 0;

	ValidateModelIndex();

	if (r_drawothermodels.GetInt())
	{
		MDLCACHE_CRITICAL_SECTION();

		int extraFlags = 0;
		if (r_drawothermodels.GetInt() == 2)
		{
			extraFlags |= STUDIO_WIREFRAME;
		}

		if (flags & STUDIO_SHADOWDEPTHTEXTURE)
		{
			extraFlags |= STUDIO_SHADOWDEPTHTEXTURE;
		}

		if (flags & STUDIO_SSAODEPTHTEXTURE)
		{
			extraFlags |= STUDIO_SSAODEPTHTEXTURE;
		}

		// Necessary for lighting blending
		CreateModelInstance();

		drawn = InternalDrawModel(flags | extraFlags);
	}

	// If we're visualizing our bboxes, draw them
	DrawBBoxVisualizations();

	return drawn;
}

int C_ViewmodelAttachmentModel::DrawModel( int flags )
{
	if ( !IsVisible() )
		return 0;

	if (m_ViewModel.Get() == NULL)
		return 0;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_TFPlayer *pPlayer = ToTFPlayer( m_ViewModel.Get()->GetOwner() );

	if ( pLocalPlayer && pLocalPlayer->IsObserver() 
		&& pLocalPlayer->GetObserverTarget() != m_ViewModel.Get()->GetOwner() )
		return false;

	if ( pLocalPlayer && !pLocalPlayer->IsObserver() && ( pLocalPlayer != pPlayer ) )
		return false;

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ViewmodelAttachmentModel::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );
	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )m_ViewModel->GetOwningWeapon();
	if ( !pWeapon )
		return;
	if ( m_ViewModel->GetViewModelType() == VMTYPE_TF2 )
	{
		pWeapon->SetMuzzleAttachment( LookupAttachment( "muzzle" ) );
	}
	pWeapon->ViewModelAttachmentBlending( hdr, pos, q, currentTime, boneMask );
}