#include "cbase.h"
#include "tf_wearable.h"
#include "tf_gamerules.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearable, DT_TFWearable );

BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearable )
#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bExtraWearable ) ),
	SendPropBool( SENDINFO( m_bDisguiseWearable ) ),
	SendPropEHandle( SENDINFO( m_hWeaponAssociatedWith ) ),
#else
	RecvPropBool( RECVINFO( m_bExtraWearable ) ),
	RecvPropBool( RECVINFO( m_bDisguiseWearable ) ),
	RecvPropEHandle( RECVINFO( m_hWeaponAssociatedWith ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable, CTFWearable );
PRECACHE_REGISTER( tf_wearable );

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableVM, DT_TFWearableVM );

BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearableVM )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable_vm, CTFWearableVM );
PRECACHE_REGISTER( tf_wearable_vm );


#define PARACHUTE_MODEL_OPEN	"models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack_open.mdl"
#define PARACHUTE_MODEL_CLOSED	"models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack.mdl"

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::Equip( CBasePlayer *pPlayer )
{
	BaseClass::Equip( pPlayer );
	UpdateModelToClass();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::UpdateModelToClass( void )
{
	if ( IsExtraWearable() && GetItem()->GetStaticData() )
	{
		SetModel( GetItem()->GetStaticData()->GetExtraWearableModel() );
	}
	else
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

		if ( pOwner && !IsDisguiseWearable() )
		{
			const char *pszModel = GetItem()->GetPlayerDisplayModel( pOwner->GetPlayerClass()->GetClassIndex() );

			if ( pszModel[0] != '\0' )
			{
				SetModel( pszModel );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::UpdatePlayerBodygroups( int bOnOff )
{
	BaseClass::UpdatePlayerBodygroups( bOnOff );
	if ( m_bDisguiseWearable )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pOwner )
			return;

		CEconItemDefinition *pStatic = GetItem()->GetStaticData();
		if ( !pStatic )
			return;

		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals();
		if ( !pVisuals )
			return;

		CTFPlayer *pDisguiseTarget = ToTFPlayer( pOwner->m_Shared.GetDisguiseTarget() );
		if ( !pDisguiseTarget )
			return;

		int iDisguiseBody = pOwner->m_Shared.GetDisguiseBody();
		for ( int i = 0; i < pDisguiseTarget->GetNumBodyGroups(); i++ )
		{
			unsigned int nIndex = pVisuals->player_bodygroups.Find( pDisguiseTarget->GetBodygroupName( i ) );
			if ( !pVisuals->player_bodygroups.IsValidIndex( nIndex ) )
				continue;

			::SetBodygroup( pDisguiseTarget->GetModelPtr(), iDisguiseBody, i, bOnOff );
		}

		pOwner->m_Shared.SetDisguiseBody( iDisguiseBody );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::Break( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( GetSkin() );
	MessageEnd();
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFWearable::GetWorldModelIndex( void )
{
	if ( m_nWorldModelIndex == 0 )
		return m_nModelIndex;

	const int iParachuteOpen = modelinfo->GetModelIndex( PARACHUTE_MODEL_OPEN );
	const int iParachuteClosed = modelinfo->GetModelIndex( PARACHUTE_MODEL_CLOSED );
	if ( m_nModelIndex == iParachuteOpen || m_nModelIndex == iParachuteClosed )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pPlayer )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_PARACHUTE_DEPLOYED ) )
				return iParachuteOpen;
			else
				return iParachuteClosed;
		}
	}

	if ( TFGameRules() )
	{
		const char *pBaseName = modelinfo->GetModelName( modelinfo->GetModel( m_nWorldModelIndex ) );
		const char *pTranslatedName = TFGameRules()->TranslateEffectForVisionFilter( "weapons", pBaseName );

		if ( pTranslatedName != pBaseName )
		{
			return modelinfo->GetModelIndex( pTranslatedName );
		}
	}

	return m_nWorldModelIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFWearable::ValidateModelIndex( void )
{
	m_nModelIndex = GetWorldModelIndex();

	BaseClass::ValidateModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Overlay Uber
//-----------------------------------------------------------------------------
int C_TFWearable::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bNotViewModel = ( ( pOwner && !pOwner->IsLocalPlayer() ) || C_BasePlayer::ShouldDrawLocalPlayer() );
	bool bUseInvulnMaterial = ( bNotViewModel && pOwner && pOwner->m_Shared.InCond( TF_COND_INVULNERABLE ) );
	bUseInvulnMaterial |= ( pOwner->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGE ) && gpGlobals->curtime < (pOwner->GetLastDamageTime() + 2.0f) );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( pOwner->GetInvulnMaterial() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFWearable::UpdateModelToClass(void)
{
	if ( IsExtraWearable() && GetItem()->GetStaticData() )
	{
		SetModel( GetItem()->GetStaticData()->GetExtraWearableModel() );
	}
	else
	{
		C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( pOwner )
		{
			const char *pszModel = nullptr;
			if ( pOwner->m_Shared.InCond(TF_COND_DISGUISED) && pOwner->IsEnemyPlayer() )
			{
				pszModel = GetItem()->GetPlayerDisplayModel( pOwner->m_Shared.GetDisguiseClass() );
			}
			else
			{
				pszModel = GetItem()->GetPlayerDisplayModel( pOwner->GetPlayerClass()->GetClassIndex() );
			}


			if ( pszModel && *pszModel )
			{
				SetModel( pszModel );
			}
		}

	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for showing and hiding regular/disguise wearables.
//-----------------------------------------------------------------------------
bool C_TFWearable::ShouldDraw()
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	// If disguised, show/hide normal and disguise wearables differently.
	if ( pOwner && ( pOwner->m_Shared.InCond( TF_COND_DISGUISED ) && pOwner->IsEnemyPlayer() ) )
	{
		// Swap between showing disguise wearables and normal wearables to enemy players.
		if (m_bDisguiseWearable)
			return BaseClass::ShouldDraw();
		else
			return false;			
	}
	
	// By default normal wearables are on, disguise wearables are not.
	if (m_bDisguiseWearable)
		return false;
	else
		return BaseClass::ShouldDraw();
	
	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWearable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_nWorldModelIndex = m_nModelIndex;
	}
}

#endif

void CTFWearable::ReapplyProvision( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// Wearables worn by our disguise don't provide attribute bonuses
		if ( IsDisguiseWearable() )
			return;
	}

	// Extra wearables don't provide attribute bonuses
	if ( IsExtraWearable() )
		return;

	BaseClass::ReapplyProvision();
}
