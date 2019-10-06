//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot.h"
#include "tf_hint.h"


BEGIN_DATADESC( CTFBotHint )
	DEFINE_KEYFIELD( m_team, FIELD_INTEGER, "team" ),
	DEFINE_KEYFIELD( m_hint, FIELD_INTEGER, "hint" ),
	DEFINE_KEYFIELD( m_isDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( func_tfbot_hint, CTFBotHint );


CTFBotHint::CTFBotHint()
{
}

CTFBotHint::~CTFBotHint()
{
}

void CTFBotHint::Spawn( void )
{
	BaseClass::Spawn();

	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	SetMoveType( MOVETYPE_NONE );
	
	SetModel( STRING( GetModelName() ) );

	AddEffects( EF_NODRAW );

	VPhysicsInitShadow( false, false );

	UpdateNavDecoration();
}

void CTFBotHint::UpdateOnRemove( void )
{
	UpdateNavDecoration();
	BaseClass::UpdateOnRemove();
}

void CTFBotHint::InputEnable( inputdata_t & inputdata )
{
	m_isDisabled = false;
	UpdateNavDecoration();
}

void CTFBotHint::InputDisable( inputdata_t & inputdata )
{
	m_isDisabled = true;
	UpdateNavDecoration();
}

bool CTFBotHint::IsFor( CTFBot *bot ) const
{
	if ( !m_isDisabled )
	{
		if ( m_team > TEAM_UNASSIGNED )
			return bot->GetTeamNumber() == m_team;

		return true;
	}

	return false;
}

void CTFBotHint::UpdateNavDecoration()
{
	Extent extent;
	extent.Init( this );

	CUtlVector<CTFNavArea *> areas;
	TheNavMesh->CollectAreasOverlappingExtent( extent, &areas );

	unsigned int bits = 0;
	switch ( m_hint )
	{
		case 0:
			bits = SNIPER_SPOT;
			break;
		case 1:
			bits = SENTRY_SPOT;
			break;
		default:
			break;
	}

	for ( int i=0; i<areas.Count(); ++i )
	{
		CTFNavArea *pArea = areas[i];

		if ( m_isDisabled )
		{
			pArea->RemoveTFAttributes( bits );
			continue;
		}

		pArea->AddTFAttributes( bits );
	}
}
