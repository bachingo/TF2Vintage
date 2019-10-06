//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_hint_entity.h"


BEGIN_DATADESC( CBaseTFBotHintEntity )
	DEFINE_KEYFIELD( m_isDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC();

IMPLEMENT_AUTO_LIST( ITFBotHintEntityAutoList )


CBaseTFBotHintEntity::CBaseTFBotHintEntity()
{
}

CBaseTFBotHintEntity::~CBaseTFBotHintEntity()
{
}


void CBaseTFBotHintEntity::InputEnable( inputdata_t& inputdata )
{
	m_isDisabled = false;
}

void CBaseTFBotHintEntity::InputDisable( inputdata_t& inputdata )
{
	m_isDisabled = true;
}


bool CBaseTFBotHintEntity::OwnerObjectFinishBuilding() const
{
	CBaseObject *obj = dynamic_cast<CBaseObject *>( GetOwnerEntity() );
	if ( !obj )
		return false;

	return !obj->IsBuilding();
}

bool CBaseTFBotHintEntity::OwnerObjectHasNoOwner() const
{
	CBaseObject *obj = dynamic_cast<CBaseObject *>( GetOwnerEntity() );
	if ( !obj  )
		return false;

	if ( !obj->GetBuilder() )
		return true;

	if ( !obj->GetBuilder()->IsPlayerClass( TF_CLASS_ENGINEER ) )
		Warning( "Object has an owner that's not an engineer.\n" );

	return false;
}
