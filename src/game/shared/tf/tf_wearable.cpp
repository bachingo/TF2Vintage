#include "cbase.h"
#include "tf_wearable.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearable, DT_TFWearable );

BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearable )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable, CTFWearable );
PRECACHE_REGISTER( tf_wearable );

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableVM, DT_TFWearableVM );

BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearableVM )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable_vm, CTFWearableVM );
PRECACHE_REGISTER( tf_wearable_vm );

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::Equip( CBasePlayer *pPlayer )
{
	BaseClass::Equip( pPlayer );
	UpdateModelToClass();

	// player_bodygroups
	if (!m_bDisguiseWearable)
		UpdatePlayerBodygroups();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::UpdateModelToClass( void )
{
	if ( m_bExtraWearable && GetItem()->GetStaticData() )
	{
		SetModel( GetItem()->GetStaticData()->extra_wearable );
	}
	else 
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

		if ( pOwner && !m_bDisguiseWearable)
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
// Purpose: Overlay Uber
//-----------------------------------------------------------------------------
int C_TFWearable::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bNotViewModel = ( ( pOwner && !pOwner->IsLocalPlayer() ) || C_BasePlayer::ShouldDrawLocalPlayer() );
	bool bUseInvulnMaterial = ( bNotViewModel && pOwner && pOwner->m_Shared.InCond( TF_COND_INVULNERABLE ) );
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
	if ( m_bExtraWearable && GetItem()->GetStaticData() )
	{
		SetModel( GetItem()->GetStaticData()->extra_wearable );
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

#endif