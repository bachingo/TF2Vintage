//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_rocket.h"
#include "particles_new.h"
#include "tf_gamerules.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_Rocket, DT_TFProjectile_Rocket )
	RecvPropBool( RECVINFO( m_bCritical ) ),
END_NETWORK_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::C_TFProjectile_Rocket( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::~C_TFProjectile_Rocket( void )
{
	ParticleProp()->StopEmission();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged(updateType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	int iAttachment = LookupAttachment( "trail" );
	if( iAttachment > -1 )
	{
		if ( enginetrace->GetPointContents( GetAbsOrigin() ) & MASK_WATER )
		{
			ParticleProp()->Create( "rockettrail_underwater", PATTACH_POINT_FOLLOW, iAttachment );
		}
		else
		{	
			ParticleProp()->Create( GetTrailParticleName(), PATTACH_POINT_FOLLOW, iAttachment );
		}

		int nUseMiniRockets = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, nUseMiniRockets, mini_rockets );
		if ( nUseMiniRockets == 1 )
		{
			C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
			if( pOwner && pOwner->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
				ParticleProp()->Create( "rockettrail_airstrike_line", PATTACH_POINT_FOLLOW, iAttachment );
		}
	}

	if ( m_bCritical )
	{
		const char *pszEffectName = "";
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				pszEffectName = "critical_rocket_red";
				break;
			case TF_TEAM_BLUE:
				pszEffectName = "critical_rocket_blue";
				break;
			default:
				pszEffectName = "eyeboss_projectile";
				break;
		}
		
		ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
}

const char *C_TFProjectile_Rocket::GetTrailParticleName( void )
{
	int nUseMiniRockets = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, nUseMiniRockets, mini_rockets );
	if ( nUseMiniRockets == 1 )
		return "rockettrail_airstrike";

	if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		return "halloween_rockettrail";

	return "rockettrail";
}
