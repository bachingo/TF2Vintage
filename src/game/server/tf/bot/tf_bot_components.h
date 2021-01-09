//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_COMPONENTS_H
#define TF_BOT_COMPONENTS_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBot/Player/NextBotPlayerBody.h"
#include "NextBot/Player/NextBotPlayerLocomotion.h"
#include "NextBotVisionInterface.h"

class CTFBotBody : public PlayerBody
{
	DECLARE_CLASS( CTFBotBody, PlayerBody )
public:
	CTFBotBody( INextBot *bot )
		: BaseClass( bot ) {}
	virtual ~CTFBotBody() {}

	virtual float GetHeadAimTrackingInterval( void ) const OVERRIDE;
};


class CTFBotLocomotion : public PlayerLocomotion
{
	DECLARE_CLASS( CTFBotLocomotion, PlayerLocomotion )
public:
	CTFBotLocomotion( INextBot *bot )
		: BaseClass( bot ) {}
	virtual ~CTFBotLocomotion() {}

	virtual void Update( void ) OVERRIDE;

	virtual void Approach( const Vector &pos, float goalWeight = 1.0f ) OVERRIDE;

	virtual void Jump( void ) OVERRIDE;

	virtual float GetMaxJumpHeight( void ) const OVERRIDE;
	virtual float GetDeathDropHeight( void ) const OVERRIDE;
	virtual float GetRunSpeed( void ) const OVERRIDE;

	virtual bool IsAreaTraversable( const CNavArea *baseArea ) const OVERRIDE;
	virtual bool IsEntityTraversable( CBaseEntity *ent, TraverseWhenType when = EVENTUALLY ) const OVERRIDE;
};


class CTFBotVision : public IVision
{
	DECLARE_CLASS( CTFBotVision, IVision )
public:
	CTFBotVision( INextBot *nextbot );
	virtual ~CTFBotVision();

	virtual void Reset( void ) OVERRIDE;
	virtual void Update( void ) OVERRIDE;

	virtual void CollectPotentiallyVisibleEntities( CUtlVector<CBaseEntity *> *ents ) OVERRIDE;

	virtual bool IsVisibleEntityNoticed( CBaseEntity *ent ) const OVERRIDE;
	virtual bool IsIgnored( CBaseEntity *ent ) const OVERRIDE;

	virtual float GetMaxVisionRange( void ) const OVERRIDE;
	virtual float GetMinRecognizeTime( void ) const OVERRIDE;

private:
	void UpdatePotentiallyVisibleNPCs();

	CUtlVector< CHandle<CBaseCombatCharacter> > m_PVNPCs;
	CountdownTimer m_updatePVNPCsTimer;
	CountdownTimer m_updateTimer;
};

#endif
