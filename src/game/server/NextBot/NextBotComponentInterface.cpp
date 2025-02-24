// NextBotComponentInterface.cpp
// Implentation of system methods for NextBot component interface
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "NextBotInterface.h"
#include "NextBotComponentInterface.h"
#include "vscript_server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

INextBotComponent::INextBotComponent( INextBot *bot )
{
	m_curInterval = TICK_INTERVAL;
	m_hScriptInstance = NULL;
	m_lastUpdateTime = 0;
	m_bot = bot;
	
	// register this component with the bot
	bot->RegisterComponent( this );
}

INextBotComponent::~INextBotComponent()
{
	if ( g_pScriptVM && m_hScriptInstance )
	{
		g_pScriptVM->RemoveInstance( m_hScriptInstance );
		m_hScriptInstance = NULL;
	}
}

HSCRIPT INextBotComponent::GetScriptInstance()
{
	if ( !m_hScriptInstance )
	{
		m_hScriptInstance = g_pScriptVM->RegisterInstance( this->GetScriptDesc(), this );
	}
	return m_hScriptInstance;
}

//--------------------------------------------------------------------------------------------------------------
static class INextBotComponentScriptInstanceHelper : public IScriptInstanceHelper
{
public:
	bool ToString( void *p, char *pBuf, int bufSize )	
	{
		INextBotComponent *pNextBotComponent = (INextBotComponent *)p;
		if ( pNextBotComponent && pNextBotComponent->GetBot() )
			V_snprintf( pBuf, bufSize, "([%d] NextBotComponent)", pNextBotComponent->GetBot()->GetBotId() );
		else
			V_snprintf( pBuf, bufSize, "(Invalid NextBotComponent)" );
		return true;
	}
} g_NextBotComponentScriptInstanceHelper;
DEFINE_SCRIPT_INSTANCE_HELPER( INextBotComponent, &g_NextBotComponentScriptInstanceHelper )

BEGIN_ENT_SCRIPTDESC_ROOT( INextBotComponent, "Next bot component" )
	DEFINE_SCRIPTFUNC( Reset, "Resets the internal update state" )
	DEFINE_SCRIPTFUNC( ComputeUpdateInterval, "Recomputes the component update interval" )
	DEFINE_SCRIPTFUNC( GetUpdateInterval, "Returns the component update interval" )
END_SCRIPTDESC();
