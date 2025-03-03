//=============================================================================
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_spymask.h"
#include "c_tf_player.h"

C_TFSpyMask::C_TFSpyMask()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TFSpyMask::InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup )
{
	if ( BaseClass::InitializeAsClientEntity( pszModelName, renderGroup ) )
	{
		AddEffects( EF_BONEMERGE | EF_NOSHADOW | EF_BONEMERGE_FASTCULL );
		
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Show mask to player's teammates.
//-----------------------------------------------------------------------------
bool C_TFSpyMask::ShouldDraw( void )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( !pOwner )
		return false;

	if ( pOwner->IsEnemyPlayer() && pOwner->m_Shared.GetDisguiseClass() != TF_CLASS_SPY )
		return false;

	if ( !pOwner->ShouldDrawThisPlayer() )
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Show player's disguise class.
//-----------------------------------------------------------------------------
int C_TFSpyMask::GetSkin( void )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner == nullptr )
		return BaseClass::GetSkin();
	
	// If the target is being ubered, show the uber skin instead.
	if ( pOwner->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		int iVisibleTeam = pOwner->GetTeamNumber();

		// if this player is disguised and on the other team, use disguise team
		if ( pOwner->m_Shared.InCond( TF_COND_DISGUISED ) && pOwner->IsEnemyPlayer() )
		{
			iVisibleTeam = pOwner->m_Shared.GetDisguiseTeam();
		}
		
		// Convert to local team skin number.
		return ( iVisibleTeam - 2 ) + 9; // Our mask's uber skins are on 9 10, 11, 12 (0, 1, 2, 3 offset 9)
	}
		
	// If this is an enemy spy disguised as a spy show a fake disguise class.
	if ( pOwner->IsEnemyPlayer() && pOwner->m_Shared.GetDisguiseClass() == TF_CLASS_SPY )
	{
		return ( pOwner->m_Shared.GetDisguiseMask() - 1 );
	}
	else
	{
		return ( pOwner->m_Shared.GetDisguiseClass() - 1 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Overlay Uber
//-----------------------------------------------------------------------------
int C_TFSpyMask::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bUseInvulnMaterial = ( pOwner && pOwner->m_Shared.InCond( TF_COND_INVULNERABLE ) );
	bUseInvulnMaterial |= ( pOwner && pOwner->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) && gpGlobals->curtime < ( pOwner->GetLastDamageTimeMvMOnly() + 2.0f ) );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}
