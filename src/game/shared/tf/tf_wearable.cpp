#include "cbase.h"
#include "tf_wearable.h"

#ifdef GAME_DLL
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearable, DT_TFWearable );

BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearable )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable, CTFWearable );
PRECACHE_REGISTER( tf_wearable );

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::Equip( CBasePlayer *pPlayer )
{
	BaseClass::Equip( pPlayer );
	UpdateModelToClass();

	// player_bodygroups
	UpdatePlayerBodygroups();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::UpdateModelToClass( void )
{
	if ( m_bExtraWearable && m_Item.GetStaticData() )
	{
		SetModel( m_Item.GetStaticData()->extra_wearable );
	}
	else 
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

		if ( pOwner )
		{
			const char *pszModel = m_Item.GetPlayerDisplayModel( pOwner->GetPlayerClass()->GetClassIndex() );

			if ( pszModel[0] != '\0' )
			{
				SetModel( pszModel );
			}
		}
	}
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
	if (bUseInvulnMaterial)
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if (bUseInvulnMaterial)
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

#endif
