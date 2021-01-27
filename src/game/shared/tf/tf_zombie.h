//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_ZOMBIE_H
#define TF_ZOMBIE_H
#ifdef _WIN32
#pragma once
#endif


#if defined( CLIENT_DLL )
#include "NextBot/c_NextBot.h"
#define NextBotCombatCharacter C_NextBotCombatCharacter
#define CZombie C_Zombie
#else

#include "nav_mesh.h"
#include "NextBot.h"
#include "NextBotBehavior.h"

class CZombie;
class CNavArea;

DECLARE_AUTO_LIST( IZombieAutoList )

class CZombiePathCost
{
public:
	CZombiePathCost(CZombie *actor)
		: m_Actor( actor ) {}

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const;

private:
	CZombie *m_Actor;
};
#endif

class CZombie : public NextBotCombatCharacter
#if defined( GAME_DLL )
	, IZombieAutoList
#endif
{
	DECLARE_CLASS( CZombie, NextBotCombatCharacter );
public:

	CZombie();
	virtual ~CZombie();

	DECLARE_NETWORKCLASS();

#if !defined( CLIENT_DLL )
	DECLARE_INTENTION_INTERFACE( CZombie )

	typedef enum
	{
		NORMAL,
		KING,
		MINION
	} SkeletonType_t;
	static CZombie*	SpawnAtPos( Vector const &pos, float flLifeTime, int iTeamNum = TF_TEAM_NPC, CBaseEntity *pOwner = NULL, SkeletonType_t eType = NORMAL );
	
	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	UpdateOnRemove( void );
	virtual void	Event_Killed( CTakeDamageInfo const &info );
	virtual int		OnTakeDamage_Alive( CTakeDamageInfo const &info );

	void			AddHat( char const *szModelName );
	void			SetSkeletonType( SkeletonType_t eType );
	void			PrecacheZombie( void );
	bool			ShouldSuicide( void ) const;

	virtual IBody*	GetBodyInterface( void ) const { return m_body; }
	virtual ILocomotion *GetLocomotionInterface( void ) const { return m_locomotor; }

	IBody *m_body;
	ILocomotion *m_locomotor;

	SkeletonType_t m_nType;

	CHandle<CBaseEntity> m_hHat;

	float m_flAttRange;
	float m_flAttDamage;

	CountdownTimer m_timeTillDeath;
	bool m_bNoteShouldDie;
#else
	virtual bool ShouldCollide( int contentsMask, int collisionGroup ) const OVERRIDE;
	virtual void BuildTransformations( CStudioHdr *pStudio, Vector *pos, Quaternion *q, matrix3x4_t const& cameraTransformation, int boneMask, CBoneBitList &boneComputed) OVERRIDE;
#endif

	CNetworkVar( float, m_flHeadScale );
};

#endif
