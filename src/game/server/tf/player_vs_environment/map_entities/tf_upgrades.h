//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_UPGRADES_H
#define TF_UPGRADES_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "GameEventListener.h"

struct CMannVsMachineUpgrades;
typedef struct
{
	char szName[128];
	float flValue;
	int iItemSlot;
} UpgradeAttribBlock_t;

class CUpgrades : public CBaseTrigger, public CGameEventListener
{
public:
	DECLARE_CLASS( CUpgrades, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void	Spawn( void );

	virtual void	FireGameEvent( IGameEvent *gameEvent );

	void			UpgradeTouch( CBaseEntity *pOther );
	virtual void	EndTouch( CBaseEntity *pEntity );

	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputReset( inputdata_t &inputdata );

	void			ApplyUpgradeAttributeBlock( UpgradeAttribBlock_t *pUpgradeBlock, int nUpgradeCount, CTFPlayer *pPlayer, bool bDowngrade );
	attrib_def_index_t ApplyUpgradeToItem( CTFPlayer *pPlayer, CEconItemView *pItem, int iUpgrade, int nCost, bool bDowngrade = false, bool bIsFresh = false );
	char const*		GetUpgradeAttributeName( int iUpgrade ) const;
	void			NotifyItemOnUpgrade( CTFPlayer *pPlayer, attrib_def_index_t nAttrDefIndex, bool bDowngrade = false );
	void			ReportUpgrade ( CTFPlayer *pPlayer, int iItemDef, int iAttributeDef, int nQuality, int nCost, bool bDowngrade, bool bIsFresh, bool bIsBottle = false );
	void			RestoreItemAttributeToBaseValue( CEconAttributeDefinition *pAttrib, CEconItemView *pItem );
	void			RestorePlayerAttributeToBaseValue( CEconAttributeDefinition *pAttrib, CTFPlayer *pPlayer );

private:
	int m_nStartDisabled;
	bool m_bEnabled;
};
extern CHandle<CUpgrades>	g_hUpgradeEntity;

attrib_def_index_t ApplyUpgrade_Default( CMannVsMachineUpgrades const& upgrade, CTFPlayer *pPlayer, CEconItemView *pItem, int nCost, bool bDowngrade );

#endif
