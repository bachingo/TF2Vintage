//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//
#include "cbase.h"
#include "tf_viewmodel.h"
#include "tf_shareddefs.h"
#include "tf_weapon_minigun.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"

// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "prediction.h"

#endif

#include "bone_setup.h"	//temp

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_viewmodel, CTFViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( TFViewModel, DT_TFViewModel )

BEGIN_NETWORK_TABLE( CTFViewModel, DT_TFViewModel )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
CTFViewModel::CTFViewModel() : m_LagAnglesHistory("CPredictedViewModel::m_LagAnglesHistory")
{
	m_vLagAngles.Init();
	m_LagAnglesHistory.Setup( &m_vLagAngles, 0 );
	m_vLoweredWeaponOffset.Init();
}
#else
CTFViewModel::CTFViewModel()
{
	m_iViewModelType = VMTYPE_NONE;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFViewModel::~CTFViewModel()
{
	SetViewModelType( VMTYPE_NONE );
#ifdef CLIENT_DLL
	for ( int i = 0; i < MAX_VIEWMODELS; i++ )
	{
		RemoveViewmodelAddon( i );
	}
#endif

}

#ifdef CLIENT_DLL
// TODO:  Turning this off by setting interp 0.0 instead of 0.1 for now since we have a timing bug to resolve
ConVar cl_wpn_sway_interp( "cl_wpn_sway_interp", "0.0", FCVAR_CLIENTDLL );
ConVar cl_wpn_sway_scale( "cl_wpn_sway_scale", "5.0", FCVAR_CLIENTDLL );
ConVar v_viewmodel_offset_x( "viewmodel_offset_x", "0", FCVAR_ARCHIVE );
ConVar v_viewmodel_offset_y( "viewmodel_offset_y", "0", FCVAR_ARCHIVE );
ConVar v_viewmodel_offset_z( "viewmodel_offset_z", "0", FCVAR_ARCHIVE );
ConVar tf_use_min_viewmodels ( "tf_use_min_viewmodels", "0", FCVAR_ARCHIVE, "Use minimized viewmodels." );
#endif

//-----------------------------------------------------------------------------
// Purpose:  Adds head bob for off hand models
//-----------------------------------------------------------------------------
void CTFViewModel::AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles )
{
#ifdef CLIENT_DLL
	// if we are an off hand view model (index 1) and we have a model, add head bob.
	// (Head bob for main hand model added by the weapon itself.)
	if ( ViewModelIndex() == 1 && GetModel() != null )
	{
		CalcViewModelBobHelper( owner, &m_BobState );
		AddViewModelBobHelper( eyePosition, eyeAngles, &m_BobState );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewModel::SetWeaponModel( const char *modelname, CBaseCombatWeapon *weapon )
{
	BaseClass::SetWeaponModel( modelname, weapon );

#ifdef CLIENT_DLL
	if ( !modelname )
	{
		for ( int i = 0; i < MAX_VIEWMODELS; i++ )
		{
			RemoveViewmodelAddon( i );
		}
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewModel::UpdateViewmodelAddon( const char *pszModelname, int index /*= 0*/ )
{
	C_ViewmodelAttachmentModel *pAddon = m_hViewmodelAddon[index].Get();

	if ( pAddon )
	{
		if ( pAddon->GetModelIndex() == modelinfo->GetModelIndex( pszModelname ) )
		{
			pAddon->m_nSkin = GetSkin();

			if ( C_BasePlayer::GetLocalPlayer() != GetOwner() ) // Spectator fix
			{
				pAddon->FollowEntity( this );
				pAddon->m_nRenderFX = m_nRenderFX;
				pAddon->UpdateVisibility();
				pAddon->SetViewmodel( this );
			}
			return; // we already have the correct add-on
		}
		else
		{
			RemoveViewmodelAddon( index );
		}
	}

	pAddon = new class C_ViewmodelAttachmentModel;
	if ( !pAddon )
		return;

	if ( pAddon->InitializeAsClientEntity( pszModelname, RENDER_GROUP_VIEW_MODEL_TRANSLUCENT ) == false )
	{
		pAddon->Release();
		return;
	}

	m_hViewmodelAddon[index] = pAddon;
	pAddon->m_nSkin = GetSkin();
	pAddon->FollowEntity( this );
	pAddon->UpdatePartitionListEntry();
	pAddon->CollisionProp()->MarkPartitionHandleDirty();
	pAddon->UpdateVisibility();
	pAddon->SetViewmodel( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewModel::RemoveViewmodelAddon( int index /*= 0*/  )
{
	if ( m_hViewmodelAddon[index].Get() )
	{
		m_hViewmodelAddon[index]->SetModel( "" );
		m_hViewmodelAddon[index]->Remove();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFViewModel::LookupAttachment( const char *pAttachmentName )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon[0].Get();
		if ( pEnt )
			return pEnt->LookupAttachment( pAttachmentName );
	}

	return BaseClass::LookupAttachment( pAttachmentName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFViewModel::GetAttachment( int number, matrix3x4_t &matrix )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon[0].Get();
		if ( pEnt )
			return pEnt->GetAttachment( number, matrix );
	}

	return BaseClass::GetAttachment( number, matrix );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFViewModel::GetAttachment( int number, Vector &origin )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon[0].Get();
		if ( pEnt )
			return pEnt->GetAttachment( number, origin );
	}

	return BaseClass::GetAttachment( number, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFViewModel::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon[0].Get();
		if ( pEnt )
			return pEnt->GetAttachment( number, origin, angles );
	}

	return BaseClass::GetAttachment( number, origin, angles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFViewModel::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	if ( GetViewModelType() == VMTYPE_TF2 )
	{
		C_ViewmodelAttachmentModel *pEnt = m_hViewmodelAddon[0].Get();
		if ( pEnt )
			return pEnt->GetAttachmentVelocity( number, originVel, angleVel );
	}

	return BaseClass::GetAttachmentVelocity( number, originVel, angleVel );
}

#endif

void CTFViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
#ifdef CLIENT_DLL
	if ( prediction->InPrediction() )
	{
		return;
	}

	if ( cl_wpn_sway_interp.GetFloat() <= 0.0f )
	{
		return;
	}

	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat(), false );

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat() );

	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors( -angleDiff, &vLaggedForward, 0, 0 );
	Vector vForwardDiff = Vector(1,0,0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= cl_wpn_sway_scale.GetFloat();
	origin += forward*vForwardDiff.x + right*-vForwardDiff.y + up*vForwardDiff.z;

#endif
}

#ifdef CLIENT_DLL
ConVar cl_gunlowerangle( "cl_gunlowerangle", "90", FCVAR_CLIENTDLL );
ConVar cl_gunlowerspeed( "cl_gunlowerspeed", "2", FCVAR_CLIENTDLL );
#endif

void CTFViewModel::CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles )
{
#if defined( CLIENT_DLL )

	Vector vecNewOrigin = eyePosition;
	QAngle vecNewAngles = eyeAngles;

	// Check for lowering the weapon
	C_TFPlayer *pPlayer = ToTFPlayer( owner );

	Assert( pPlayer );

	bool bLowered = pPlayer->IsWeaponLowered();

	QAngle vecLoweredAngles(0,0,0);

	m_vLoweredWeaponOffset.x = Approach( bLowered ? cl_gunlowerangle.GetFloat() : 0, m_vLoweredWeaponOffset.x, cl_gunlowerspeed.GetFloat() );
	vecLoweredAngles.x += m_vLoweredWeaponOffset.x;

	vecNewAngles += vecLoweredAngles;


	// Viewmodel offset
	Vector	forward, right, up;
	AngleVectors(eyeAngles, &forward, &right, &up);

	// Don't use offsets if minimal viewmodels are active
	if ( tf_use_min_viewmodels.GetBool() )
	{
		vecNewOrigin += forward*m_vOffset.x + right*m_vOffset.y + up*m_vOffset.z;
	}
	else
	{
		vecNewOrigin += forward*v_viewmodel_offset_x.GetFloat() + right*v_viewmodel_offset_y.GetFloat() + up*v_viewmodel_offset_z.GetFloat();
	}

	BaseClass::CalcViewModelView( owner, vecNewOrigin, vecNewAngles );

#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Don't render the weapon if its supposed to be lowered and we have 
// finished the lowering animation
//-----------------------------------------------------------------------------
int CTFViewModel::DrawModel( int flags )
{
	// Check for lowering the weapon
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	Assert( pPlayer );

	bool bLowered = pPlayer->IsWeaponLowered();

	if ( bLowered && fabs( m_vLoweredWeaponOffset.x - cl_gunlowerangle.GetFloat() ) < 0.1 )
	{
		// fully lowered, stop drawing
		return 1;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && 
		pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = ToTFPlayer( pLocalPlayer->GetObserverTarget() );

		if ( pPlayer != GetOwner() )
			return 0;
	}

	if ( pPlayer->IsAlive() == false )
	{
		 return 0;
	}

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewModel::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )GetOwningWeapon();

	if ( !pWeapon ) 
		return;

	if ( pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
	{
		CTFMinigun *pMinigun = ( CTFMinigun * )pWeapon;

		int iBarrelBone = Studio_BoneIndexByName( hdr, "v_minigun_barrel" );

		Assert( iBarrelBone != -1 );

		if ( iBarrelBone != -1 && ( hdr->boneFlags( iBarrelBone ) & boneMask ) )
		{
			RadianEuler a;
			QuaternionAngles( q[iBarrelBone], a );

			a.x = pMinigun->GetBarrelRotation();

			AngleQuaternion( a, q[iBarrelBone] );
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewModel::ProcessMuzzleFlashEvent()
{
	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )GetOwningWeapon();

	if ( !pWeapon || C_BasePlayer::ShouldDrawLocalPlayer() ) 
		return;

	pWeapon->ProcessMuzzleFlashEvent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFViewModel::GetSkin()
{
	int nSkin = BaseClass::GetSkin();

	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )GetOwningWeapon();

	if ( !pWeapon ) 
		return nSkin;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		if ( pWeapon->GetTFWpnData().m_bHasTeamSkins_Viewmodel )
		{
			switch( pPlayer->GetTeamNumber() )	
			{
			case TF_TEAM_RED:
				nSkin = 0;
				break;
			case TF_TEAM_BLUE:
				nSkin = 1;
				break;
			}
		}	
	}

	return nSkin;
}

//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
void CTFViewModel::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	// Don't process animevents if it's not drawn.
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner->ShouldDrawThisPlayer() )
		return;

	BaseClass::FireEvent( origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose: Update minimal viewmodel positions
//-----------------------------------------------------------------------------
void CTFViewModel::CalcMinViewmodelOffset( C_TFPlayer *owner )
{
	// Always update this even if we're not using min viewmodels
	// in case the player decides to activate them

	if ( !owner )
		return;

	C_TFWeaponBase *pWeapon = owner->GetActiveTFWeapon();
	if ( pWeapon )
	{
		vec_t vecOffset[3] = { 0.0f, 0.0f, 0.0f };
		string_t strOffset = pWeapon->GetViewModelOffset();
		if ( strOffset != NULL_STRING )
		{
			UTIL_StringToVector( vecOffset, strOffset );
		}

		m_vOffset.Init( vecOffset[0], vecOffset[1], vecOffset[2] );

		if ( m_vOffset.x == 0.0f && m_vOffset.y == 0.0f && m_vOffset.z == 0.0f )
		{
			Warning( "No offset specified for minimal viewmodel. Defaulting to 0\n" );
		}

	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CTFViewModel::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( BaseClass::OnPostInternalDrawModel( pInfo ) )
	{
		C_EconEntity *pEntity = GetOwningWeapon();
		if ( pEntity )
		{
			DrawEconEntityAttachedModels( this, pEntity, pInfo, 2 );
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CViewModelInvisProxy : public CEntityMaterialProxy
{
public:

	CViewModelInvisProxy( void );
	virtual ~CViewModelInvisProxy( void );
	virtual bool Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial * GetMaterial();

private:
	IMaterialVar *m_pPercentInvisible;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CViewModelInvisProxy::CViewModelInvisProxy( void )
{
	m_pPercentInvisible = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CViewModelInvisProxy::~CViewModelInvisProxy( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input : *pMaterial - 
//-----------------------------------------------------------------------------
bool CViewModelInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bFound;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bFound );

	return bFound;
}

ConVar tf_vm_min_invis( "tf_vm_min_invis", "0.22", FCVAR_DEVELOPMENTONLY, "minimum invisibility value for view model", true, 0.0, true, 1.0 );
ConVar tf_vm_max_invis( "tf_vm_max_invis", "0.5", FCVAR_DEVELOPMENTONLY, "maximum invisibility value for view model", true, 0.0, true, 1.0 );

//-----------------------------------------------------------------------------
// Purpose: 
// Input :
//-----------------------------------------------------------------------------
void CViewModelInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	if ( !pEnt )
		return;

	C_TFViewModel *pVM;
	C_ViewmodelAttachmentModel *pVMAddon = dynamic_cast< C_ViewmodelAttachmentModel * >( pEnt );
	if ( pVMAddon )
	{
		pVM = dynamic_cast< C_TFViewModel * >( pVMAddon->m_viewmodel.Get() );
	}
	else
	{
		pVM = dynamic_cast< C_TFViewModel * >( pEnt );
	}

	if ( !pVM )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pVM->GetOwner() );

	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	float flPercentInvisible = pPlayer->GetPercentInvisible();

	// remap from 0.22 to 0.5
	// but drop to 0.0 if we're not invis at all
	float flWeaponInvis = ( flPercentInvisible < 0.01 ) ?
	0.0 :
	RemapVal( flPercentInvisible, 0.0, 1.0, tf_vm_min_invis.GetFloat(), tf_vm_max_invis.GetFloat() );

	if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
	{
		// Hacky fix to make viewmodel blink more obvious
		m_pPercentInvisible->SetFloatValue( flWeaponInvis - 0.1 );
	}
	else
	{
		m_pPercentInvisible->SetFloatValue( flWeaponInvis );
	}
}

IMaterial *CViewModelInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}

EXPOSE_INTERFACE( CViewModelInvisProxy, IMaterialProxy, "vm_invis" IMATERIAL_PROXY_INTERFACE_VERSION );


#endif // CLIENT_DLL
