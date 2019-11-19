//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_HALLOWEEN_BOSS_H
#define TF_HALLOWEEN_BOSS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CHalloweenBaseBoss C_HalloweenBaseBoss
#define NextBotCombatCharacter C_NextBotCombatCharacter
#include "NextBot/c_NextBot.h"
#else
#include "NextBot.h"
#include "nav_mesh.h"

class CDisableVision : public IVision
{
public:
	CDisableVision( INextBot *bot )
		: IVision( bot ) {}
	virtual ~CDisableVision() {}
	virtual void Reset( void ) {}
	virtual void Update( void ) {}
};
#endif

class CTFPlayer;

class CHalloweenBaseBoss : public NextBotCombatCharacter
{
	DECLARE_CLASS( CHalloweenBaseBoss, NextBotCombatCharacter )
public:
	CHalloweenBaseBoss();
	virtual ~CHalloweenBaseBoss();

	enum HalloweenBossType
	{
		HEADLESS_HATMAN = 1,
		EYEBALL_BOSS	= 2,
		MERASMUS		= 3
	};

#ifdef GAME_DLL
	struct AttackerInfo
	{
		CHandle<CTFPlayer> m_hPlayer;
		float m_flTimeDamaged;
		bool m_wasMelee;
	};
	CUtlVector<AttackerInfo> m_lastAttackers;

	struct DamageRateInfo
	{
		float m_flDamage;
		float m_flTimeDealt;
	};
	CUtlVector<DamageRateInfo> m_damageInfos;

	static CHalloweenBaseBoss *SpawnBossAtPos( HalloweenBossType type, const Vector &pos, int teamNum = TF_TEAM_NPC, CBaseEntity *pOwner = NULL );


	virtual void Spawn( void );
	virtual void UpdateOnRemove( void );

	virtual int OnTakeDamage( const CTakeDamageInfo& info );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo& info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

public: // INextBot
	virtual void Update( void );

private:
	void UpdateDamagePerSecond( void );
#endif

public:
	virtual int GetBossType( void ) const;
	virtual float GetCritInjuryMultiplier( void ) const;
	virtual int GetLevel( void ) const;

#ifdef GAME_DLL
	void Break( void );

	bool DidSpawnWithCheats( void ) const { return m_bSpawnedWithCheats; }

private:
	void RememberAttacker( CTFPlayer *pAttacker, bool bMelee, float flDmgAmount );

// Setting these to public so codebase can compile
public:
	float m_flDPSCounter;
	float m_flDPSMax;

	bool m_bSpawnedWithCheats;

protected:
	Vector m_vecSpawn;
#endif
};

#endif