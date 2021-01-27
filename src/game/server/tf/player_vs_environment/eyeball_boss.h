//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef EYEBALL_BOSS_H
#define EYEBALL_BOSS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_halloween_boss.h"
#include "Path/NextBotPath.h"

class CEyeBallBoss;
template<typename Actor> class Behavior;

class CEyeBallBossLocomotion : public ILocomotion
{
public:
	CEyeBallBossLocomotion( INextBot *bot );
	virtual ~CEyeBallBossLocomotion() {}

	virtual void            Update( void );
	virtual void            Reset( void );

	virtual float           GetStepHeight( void ) const;
	virtual float           GetMaxJumpHeight( void ) const;
	virtual float           GetDeathDropHeight( void ) const;

	virtual void            Approach( const Vector& goalPos, float goalWeight = 1.0f );
	virtual void            FaceTowards( const Vector& target );

	virtual const Vector&   GetGroundNormal( void ) const;

	virtual const Vector&   GetFeet( void ) const;

	virtual const Vector&   GetVelocity( void ) const;

	virtual float           GetDesiredSpeed( void ) const;
	virtual void            SetDesiredSpeed( float fSpeed );

	virtual float           GetDesiredAltitude( void ) const;
	virtual void            SetDesiredAltitude( float fHeight );

private:
	void MaintainAltitude( void );

	float m_desiredSpeed;
	float m_desiredAltitude;
	Vector m_vecMotion;
	float m_verticalSpeed;
	Vector m_localVelocity;
	Vector m_wishVelocity;

	friend class CEyeBallBoss;
};


class CEyeBallBossBody : public IBody
{
public:
	CEyeBallBossBody( CEyeBallBoss *me );
	virtual ~CEyeBallBossBody() { }

	virtual void Update( void );

	virtual float GetMaxHeadAngularVelocity( void ) const;

	virtual void AimHeadTowards( const Vector &lookAtPos,
								 LookAtPriorityType priority = BORING,
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = nullptr,
								 const char *reason = nullptr );
	virtual void AimHeadTowards( CBaseEntity *subject,
								 LookAtPriorityType priority = BORING,
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = nullptr,
								 const char *reason = nullptr );

private:
	Vector m_lookAtSpot;
	friend class CEyeBallBoss;
};


class CEyeBallBoss : public CHalloweenBaseBoss
{
	DECLARE_CLASS( CEyeBallBoss, CHalloweenBaseBoss )
public:
	DECLARE_INTENTION_INTERFACE( CEyeBallBoss )

	CEyeBallBoss();
	virtual ~CEyeBallBoss();

	enum
	{
		ATTITUDE_CALM,
		ATTITUDE_GRUMPY,
		ATTITUDE_ANGRY,
		ATTITUDE_HATEBLUE,
		ATTITUDE_HATERED
	};

	DECLARE_SERVERCLASS();

	virtual void			Precache( void );
	virtual void			Spawn( void );
	virtual void			UpdateOnRemove( void );
	virtual int				OnTakeDamage_Alive( const CTakeDamageInfo& info );
	virtual Vector			EyePosition( void );
	virtual void			UpdateLastKnownArea( void );

	virtual void			Update( void );

	virtual int				GetBossType( void ) const { return EYEBALL_BOSS; }
	virtual int				GetLevel( void ) const { return m_level; }

	virtual CEyeBallBossLocomotion *GetLocomotionInterface( void ) const OVERRIDE { return m_locomotor; }
	virtual CEyeBallBossBody *GetBodyInterface( void ) const OVERRIDE { return m_body; }
	virtual IVision*		GetVisionInterface( void ) const OVERRIDE { return m_vision; }

	void					JarateNearbyPlayer( float range );
	CBaseCombatCharacter*	GetVictim( void ) const;
	CBaseCombatCharacter*	FindNearestVisibleVictim( void );
	void					BecomeEnraged( float duration );
	const Vector&			PickNewSpawnSpot( void ) const;
	//int						GetState( void ) const { return m_iBossState; }

	void					LogPlayerInteraction( const char *event, CTFPlayer *pAttacker );

	CNetworkVector( m_lookAtSpot );

	CNetworkVar( int, m_attitude );
	CountdownTimer m_attitudeTimer;

	CountdownTimer m_idleTimer;
	CountdownTimer m_idleNoiseTimer;
	bool m_bTaunt;

	CountdownTimer m_lifeTimeDuration;
	float m_flTimeLeftAlive;

	CountdownTimer m_teleportTimer;

	CHandle<CBaseCombatCharacter> m_hTarget;

	static int m_level;

	int m_iOldHealth;

private:
	void					PrecacheEyeBallBoss( void );


	CEyeBallBossLocomotion *m_locomotor;
	CEyeBallBossBody *m_body;
	IVision *m_vision;

	int m_iAngerPose;

	CUtlVector<EHANDLE> m_hSpawnEnts;

};

#endif
