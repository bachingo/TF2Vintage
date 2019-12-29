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
	if (pOwner->m_Shared.InCond(TF_COND_INVULNERABLE))
	{
		int iVisibleTeam = pOwner->GetTeamNumber();
		int nSkin;

		// if this player is disguised and on the other team, use disguise team
		if (pOwner->m_Shared.InCond(TF_COND_DISGUISED) && pOwner->IsEnemyPlayer())
		{
			iVisibleTeam = pOwner->m_Shared.GetDisguiseTeam();
		}

		// Our mask's uber skins are on 9 and 10 (0 and 1 offset 9)
		switch (iVisibleTeam)
		{
			case TF_TEAM_RED:
			nSkin = 9;
			break;

			case TF_TEAM_BLUE:
			nSkin = 10;
			break;
		
			default:
			nSkin = 9;
			break;
		}
		return nSkin;
	}
		
		// If this is an enemy spy disguised as a spy show a fake disguise class.
		if ( pOwner->IsEnemyPlayer() && pOwner->m_Shared.GetDisguiseClass() == TF_CLASS_SPY )
		{
			return ( pOwner->m_Shared.GetMaskClass() - 1 );
		}
		else
		{
			return ( pOwner->m_Shared.GetDisguiseClass() - 1 );
		}

	return BaseClass::GetSkin();
}
