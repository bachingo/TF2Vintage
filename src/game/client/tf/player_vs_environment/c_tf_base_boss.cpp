//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tf_base_boss.h"


IMPLEMENT_CLIENTCLASS_DT( C_TFBaseBoss, DT_TFBaseBoss, CTFBaseBoss )
	RecvPropFloat( RECVINFO( m_lastHealthPercentage ) ),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( base_boss, C_TFBaseBoss );


ShadowType_t C_TFBaseBoss::ShadowCastType( void )
{
	if ( !IsVisible() )
		return SHADOWS_NONE;

	if ( IsEffectActive( EF_NODRAW|EF_NOSHADOW ) )
		return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}
