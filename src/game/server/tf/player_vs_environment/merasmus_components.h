//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_COMPONENTS_H
#define MERASMUS_COMPONENTS_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotGroundLocomotion.h"

class CMerasmusLocomotion : public NextBotGroundLocomotion
{
	friend class CMerasmus;
public:
	CMerasmusLocomotion( INextBot *actor )
		: NextBotGroundLocomotion( actor ) {}
	virtual ~CMerasmusLocomotion() { };

	virtual void Update( void ) OVERRIDE;

	virtual float GetStepHeight( void ) const OVERRIDE { return 18.0f; }
	virtual float GetMaxJumpHeight( void ) const OVERRIDE { return 18.0f; }

	virtual float GetMaxYawRate( void ) const OVERRIDE { return 200.0f; }
	virtual float GetRunSpeed( void ) const OVERRIDE;

	virtual bool ShouldCollideWith( const CBaseEntity *other ) const OVERRIDE;
};

class CMerasmusFlyingLocomotion : public ILocomotion
{
	friend class CMerasmus;
public:
	CMerasmusFlyingLocomotion( INextBot *actor )
		: ILocomotion( actor )
	{
		Reset();
	}
	virtual ~CMerasmusFlyingLocomotion() {}

	virtual void            Update( void ) OVERRIDE;
	virtual void            Reset( void ) OVERRIDE;

	virtual float           GetStepHeight( void ) const OVERRIDE;
	virtual float           GetMaxJumpHeight( void ) const OVERRIDE;
	virtual float           GetDeathDropHeight( void ) const OVERRIDE;

	virtual bool			ShouldCollideWith( const CBaseEntity *other ) const OVERRIDE;

	virtual void            Approach( const Vector& goalPos, float goalWeight = 1.0f );
	virtual void            FaceTowards( const Vector& target );

	virtual const Vector&   GetGroundNormal( void ) const OVERRIDE;

	virtual const Vector&   GetVelocity( void ) const OVERRIDE;

	virtual float           GetDesiredSpeed( void ) const;

	virtual float           GetDesiredAltitude( void ) const;
	virtual void            SetDesiredAltitude( float fHeight );

private:
	void MaintainAltitude( void );

	float m_verticalSpeed;
	Vector m_vecMotion;
	float m_desiredAltitude;
	Vector m_localVelocity;
	Vector m_wishVelocity;
};

class CMerasmusBody : public IBody
{
	friend class CMerasmus;
public:
	CMerasmusBody( INextBot *actor )
		: IBody( actor )
	{
		m_iMoveX = -1;
		m_iMoveY = -1;
	}
	virtual ~CMerasmusBody() { }

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

#endif
