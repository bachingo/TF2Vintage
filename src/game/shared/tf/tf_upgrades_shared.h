//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_UPGRADES_SHARED_H
#define TF_UPGRADES_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFPlayer C_TFPlayer
#endif
class CTFPlayer;

struct CMannVsMachineUpgrades
{
	char szAttribute[128];
	char szIcon[MAX_PATH];
	float flIncrement;
	float flCap;
	int nCost;
	int nUIGroup;
	int nQuality;
	int nTier;
};

class CMannVsMachineUpgradeManager : public CAutoGameSystem
{
public:
	CMannVsMachineUpgradeManager();

	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();

	void LoadUpgradesFile( void );
	void LoadUpgradesFileFromPath( char const *pszPath );
	void ParseUpgradeBlockForUIGroup( KeyValues *pKVData, int iDefUIGroup );

	int GetAttributeIndexByName( char const *pszAttributeName );

	int GetUpgradeCount( void ) const { return m_Upgrades.Count(); }
	CUtlVector<CMannVsMachineUpgrades> &GetUpgradeVector( void ) { return m_Upgrades; }

private:
	CUtlVector< CMannVsMachineUpgrades > m_Upgrades;
	CUtlMap< char const*, int > m_UpgradeMap;
};

extern CMannVsMachineUpgradeManager g_MannVsMachineUpgrades;
int GetUpgradeStepData( CTFPlayer *pPlayer, int nWeaponSlot, int nUpgradeIndex, int *nCurrentStep, bool *bOverCap );

#endif
