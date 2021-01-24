//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_basetempentity.h"
#include "c_te_effect_dispatch.h"
#include "tf_shareddefs.h"
#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
											const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, 
											int nSkin, const breakablepropparams_t &params );

void CreateDeathCallingCard( Vector const& vecOrigin, QAngle const& vAngle, int iVictimIndex, int iShooterIndex, int nCallingCard )
{
	if ( nCallingCard < 1 || nCallingCard > CALLING_CARD_COUNT )
	{
		Warning( "Attempted to Call CreateDeathCallingCard With invalid index %d", nCallingCard );
		return;
	}

	CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictimIndex ) );
	if ( !pVictim )
		return;

	breakablepropparams_t breakParams( vecOrigin, vAngle, vec3_origin, vec3_origin );
	breakParams.impactEnergyScale = 1.0f;

	breakmodel_t breakModel;
	V_strcpy_safe( breakModel.modelName, g_pszDeathCallingCardModels[ nCallingCard ] );

	breakModel.placementName[0] = 0;
	breakModel.fadeTime = RandomFloat( 7.0f, 10.0f );
	breakModel.fadeMinDist = 0.0f;
	breakModel.fadeMaxDist = 0.0f;
	breakModel.burstScale = 1.0f;
	breakModel.health = 1;
	breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
	breakModel.isRagdoll = false;
	breakModel.isMotionDisabled = false;
	breakModel.placementIsBone = false;

	CBaseEntity *pBreakModel = BreakModelCreateSingle( 
		pVictim, 
		&breakModel, 
		pVictim->GetAbsOrigin() + Vector( 0, 0, 50.0f ), 
		QAngle( 0, vAngle.y, 0 ), 
		vec3_origin, 
		vec3_origin, 
		0, 
		breakParams 
	);

	// Scale for some reason
	if ( pBreakModel )
	{
		CBaseAnimating *pAnim = dynamic_cast<CBaseAnimating *>( pBreakModel );
		if ( pAnim )
		{
			pAnim->SetModelScale( 0.9f );
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DeathCallingCard( const CEffectData &data )
{
	CreateDeathCallingCard( 
		data.m_vOrigin, 
		data.m_vAngles, 
		data.m_nAttachmentIndex,
		data.m_nHitBox,
		data.m_fFlags
	);
}

DECLARE_CLIENT_EFFECT( "TFDeathCallingCard", DeathCallingCard );