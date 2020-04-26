//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "ghost.h"
#include "ghost_behavior.h"


IMPLEMENT_INTENTION_INTERFACE( CGhost, CGhostBehavior );

LINK_ENTITY_TO_CLASS( ghost, CGhost )

IMPLEMENT_AUTO_LIST( IGhostAutoList );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGhost *CGhost::Create( const Vector &vecOrigin, const QAngle &vecAngles, float lifetime )
{
	CGhost *pGhost = (CGhost *)CreateEntityByName( "ghost" );
	if ( pGhost )
	{
		DispatchSpawn( pGhost );

		pGhost->SetAbsOrigin( vecOrigin );
		pGhost->SetLocalAngles( vecAngles );
		pGhost->SetLifetime( lifetime );

		return pGhost;
	}

	return nullptr;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGhost::CGhost()
{
	m_intention = new CGhostIntention( this );
	m_locomotor = new CGhostLocomotion( this );

	m_flLifetime = 10.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGhost::~CGhost()
{
	if( m_intention )
		delete m_locomotor;
	if( m_locomotor )
		delete m_intention;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGhost::Precache()
{
	BaseClass::Precache();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheGhost();

	CBaseEntity::SetAllowPrecache( allowPrecache );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGhost::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( "models/props_halloween/ghost_no_hat.mdl" );

	SetCollisionGroup( COLLISION_GROUP_NONE );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CGhost::EyePosition( void )
{
	return GetAbsOrigin() + m_vecEyeOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CGhost::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGhost::PrecacheGhost()
{
	PrecacheModel( "models/props_halloween/ghost_no_hat.mdl" );
	PrecacheParticleSystem( "ghost_appearation" );
	PrecacheScriptSound( "Halloween.GhostMoan" );
	PrecacheScriptSound( "Halloween.GhostBoo" );
	PrecacheScriptSound( "Halloween.Haunted" );
}
