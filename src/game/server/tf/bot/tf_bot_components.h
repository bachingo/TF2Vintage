//========= Copyright © Valve LLC, All rights reserved. =======================
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

	virtual float GetHeadAimTrackingInterval( void ) const override;
};


class CTFBotLocomotion : public PlayerLocomotion
{
	DECLARE_CLASS( CTFBotLocomotion, PlayerLocomotion )
public:
	CTFBotLocomotion( INextBot *bot )
		: BaseClass( bot ) {}
	virtual ~CTFBotLocomotion() {}

	virtual void Update( void ) override;

	virtual void Approach( const Vector &pos, float goalWeight = 1.0f ) override;

	virtual void Jump( void ) override;

	virtual float GetMaxJumpHeight( void ) const override;
	virtual float GetDeathDropHeight( void ) const override;
	virtual float GetRunSpeed( void ) const override;

	virtual bool IsAreaTraversable( const CNavArea *baseArea ) const override;
	virtual bool IsEntityTraversable( CBaseEntity *ent, TraverseWhenType when = EVENTUALLY ) const override;
};


class CTFBotVision : public IVision
{
	DECLARE_CLASS( CTFBotVision, IVision )
public:
	CTFBotVision( INextBot *nextbot );
	virtual ~CTFBotVision();

	virtual void Reset( void ) override;
	virtual void Update( void ) override;

	virtual void CollectPotentiallyVisibleEntities( CUtlVector<CBaseEntity *> *ents ) override;

	virtual bool IsVisibleEntityNoticed( CBaseEntity *ent ) const override;
	virtual bool IsIgnored( CBaseEntity *ent ) const override;

	virtual float GetMaxVisionRange( void ) const override;
	virtual float GetMinRecognizeTime( void ) const override;

private:
	void UpdatePotentiallyVisibleNPCs();

	CUtlVector< CHandle<CBaseCombatCharacter> > m_PVNPCs;
	CountdownTimer m_updatePVNPCsTimer;
	CountdownTimer m_updateTimer;
};

#endif
