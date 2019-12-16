//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_MANAGER_H
#define TF_BOT_MANAGER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotManager.h"

class CTFBotManager : public NextBotManager
{
public:
	CTFBotManager();
	virtual ~CTFBotManager();

	virtual void Update( void ) override;
	virtual void OnMapLoaded( void ) override;
	virtual void OnRoundRestart( void ) override;

	void OnLevelShutdown( void );

	bool IsInOfflinePractice( void ) const;
	void SetIsInOfflinePractice( bool set );

	const char *GetRandomBotName( void );
	void ReloadBotNames( void );
	CTFBot *GetAvailableBotFromPool( void );
	bool RemoveBotFromTeamAndKick( int teamNum );
	void OnForceAddedBots( int count );
	void OnForceKickedBots( int count );

	bool IsAllBotTeam( int teamNum );

	bool IsMeleeOnly( void ) const;

private:
	bool LoadBotNames( void );
	void MaintainBotQuota( void );
	void RevertOfflinePracticeConvars( void );

	CUtlVector<string_t> m_BotNames;
	float m_flQuotaChangeTime;
};

extern CTFBotManager &TheTFBots( void );

const char *DifficultyToName( int iSkillLevel );
int NameToDifficulty( const char *pszSkillName );

void CreateBotName( int iTeamNum, int iClassIdx, int iSkillLevel, char *out, int outlen );

#endif