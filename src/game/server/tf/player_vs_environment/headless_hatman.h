//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef HEADLESS_HATMAN_H
#define HEADLESS_HATMAN_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPath.h"
#include "tf_halloween_boss.h"

class CHeadlessHatman;


class CHeadlessHatmanPathCost : public IPathCost
{
public:
	CHeadlessHatmanPathCost( CHeadlessHatman *actor )
		: m_Actor( actor )
	{
	}

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const;

private:
	CHeadlessHatman *m_Actor;
};


class CHeadlessHatmanLocomotion : public NextBotGroundLocomotion
{
public:
	CHeadlessHatmanLocomotion( INextBot *actor )
		: NextBotGroundLocomotion( actor ) {}
	virtual ~CHeadlessHatmanLocomotion() { };

	virtual float GetStepHeight( void ) const OVERRIDE { return 18.0f; }
	virtual float GetMaxJumpHeight( void ) const OVERRIDE { return 18.0f; }

	virtual float GetMaxYawRate( void ) const OVERRIDE { return 200.0f; }
	virtual float GetRunSpeed( void ) const OVERRIDE;

	virtual bool ShouldCollideWith( const CBaseEntity *other ) const OVERRIDE;
};


class CHeadlessHatmanBody : public IBody
{
public:
	CHeadlessHatmanBody( INextBot *actor );
	virtual ~CHeadlessHatmanBody() { };

	virtual void Update( void ) OVERRIDE;

	virtual unsigned int GetSolidMask( void ) const OVERRIDE { return MASK_NPCSOLID|CONTENTS_PLAYERCLIP; }

	virtual Activity GetActivity( void ) const OVERRIDE { return m_Activity; }
	virtual bool StartActivity( Activity act, unsigned int flags = 0 ) OVERRIDE;
	virtual bool IsActivity( Activity act ) const OVERRIDE { return m_Activity == act; }

private:
	Activity m_Activity;

	int m_iMoveX;
	int m_iMoveY;
};


class CHeadlessHatman : public CHalloweenBaseBoss
{
	DECLARE_CLASS( CHeadlessHatman, CHalloweenBaseBoss )
public:
	DECLARE_INTENTION_INTERFACE( CHeadlessHatman )

	CHeadlessHatman();
	virtual ~CHeadlessHatman();

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo& info );

	virtual void Update( void );
	virtual IBody *GetBodyInterface( void ) const OVERRIDE { return m_body; }
	virtual ILocomotion *GetLocomotionInterface( void ) const OVERRIDE { return m_locomotor; }

public:
	virtual int GetBossType( void ) const { return HEADLESS_HATMAN; }

	const char *GetWeaponModel( void ) const;

	CBaseAnimating *GetWeapon( void ) const { return m_pAxe; }

private:
	void PrecacheHeadlessHatman( void );

	CHeadlessHatmanLocomotion *m_locomotor;
	CHeadlessHatmanBody *m_body;

	CBaseAnimating *m_pAxe;

	EHANDLE m_hTarget;
};

#endif