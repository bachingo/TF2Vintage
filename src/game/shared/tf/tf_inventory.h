//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple Inventory
// by MrModez
// $NoKeywords: $
//=============================================================================//
#ifndef TF_INVENTORY_H
#define TF_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
//#include "server_class.h"
#include "igamesystem.h"
#include "tf_playeranimstate.h"
#include "tf_shareddefs.h"
#include "tf_weapon_parse.h"
#include "filesystem.h" 
#if defined( CLIENT_DLL )
#include "c_tf_player.h"
#include "filesystem.h"
#endif

#define INVENTORY_WEAPONS		62
#define INVENTORY_WEAPONS_COUNT	63
#define INVENTORY_COLNUM		1
#define INVENTORY_ROWNUM		7
#define INVENTORY_VECTOR_NUM	INVENTORY_COLNUM * INVENTORY_ROWNUM

//Item Selection Def's
#define INVENTORY_WEAPONS_SELECTION		62
#define INVENTORY_WEAPONS_COUNT_SELECTION	63
#define INVENTORY_COLNUM_SELECTION		63
#define INVENTORY_ROWNUM_SELECTION		1
#define INVENTORY_VECTOR_NUM_SELECTION	INVENTORY_COLNUM_SELECTION * INVENTORY_ROWNUM_SELECTION

class CTFInventory : public CAutoGameSystemPerFrame
{
public:
	CTFInventory();
	~CTFInventory();

	virtual char const *Name() { return "CTFInventory"; }

	virtual bool Init( void );
	virtual void LevelInitPreEntity( void );

	int GetNumPresets( int iClass, int iSlot );
	int GetWeapon( int iClass, int iSlot );
	CEconItemView *GetItem( int iClass, int iSlot, int iNum );
	bool CheckValidSlot( int iClass, int iSlot, bool bHudCheck = false );
	bool CheckValidWeapon( int iClass, int iSlot, int iWeapon, bool bHudCheck = false );

#if defined( CLIENT_DLL )
	int GetWeaponPreset( int iClass, int iSlot );
	void SetWeaponPreset( int iClass, int iSlot, int iPreset );
	const char* GetSlotName( int iSlot );
	
	// Presets.
	int GetCurrentLoadoutSlot(int iClass);
	void ChangeLoadoutSlot(int iClass, int iLoadoutSlot);

#endif

private:
	static const int Weapons[TF_CLASS_COUNT_ALL][TF_PLAYER_WEAPON_COUNT];
	CUtlVector<CEconItemView *> m_Items[TF_CLASS_COUNT_ALL][TF_LOADOUT_SLOT_COUNT];

#if defined( CLIENT_DLL )
	virtual bool CheckSpecialItemAccess( void );
	void LoadInventory();
	void ResetInventory();
	void SaveInventory();
	KeyValues* m_pInventory;
#endif
};

CTFInventory *GetTFInventory();

#endif // TF_INVENTORY_H
