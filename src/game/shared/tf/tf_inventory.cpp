//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple Inventory
// by MrModez
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_inventory.h"
#include "econ_item_system.h"

#ifdef CLIENT_DLL

#include "tier0/icommandline.h"

#endif

static CTFInventory g_TFInventory;

CTFInventory *GetTFInventory()
{
	return &g_TFInventory;
}

CTFInventory::CTFInventory() : CAutoGameSystemPerFrame( "CTFInventory" )
{
#ifdef CLIENT_DLL
	m_pInventory = NULL;
#endif

	// Generate dummy base items.
	for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++ )
		{
			m_Items[iClass][iSlot].AddToTail( NULL );
		}
	}
};

CTFInventory::~CTFInventory()
{
#if defined( CLIENT_DLL )
	m_pInventory->deleteThis();
#endif
}

bool CTFInventory::Init( void )
{
#ifdef CLIENT_DLL
	bool bReskinsEnabled = CommandLine()->CheckParm( "-showreskins" );
	bool bSpecialsEnabled = CommandLine()->CheckParm( "-goldenboy" );
#else
	bool bReskinsEnabled = true;
	bool bSpecialsEnabled = true;
#endif

	GetItemSchema()->Init();

	// Generate item list.
	FOR_EACH_MAP( GetItemSchema()->m_Items, i )
	{
		int iItemID = GetItemSchema()->m_Items.Key( i );
		CEconItemDefinition *pItemDef = GetItemSchema()->m_Items.Element( i );

		if ( pItemDef->item_slot == -1 )
			continue;
		
		// Add it to each class that uses it.
		for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
		{
			if ( pItemDef->used_by_classes & ( 1 << iClass ) )
			{
				// Show it if it's either base item or has show_in_armory flag.
				int iSlot = pItemDef->GetLoadoutSlot( iClass );

				if (iSlot >= TF_LOADOUT_SLOT_PRIMARY && iSlot < TF_LOADOUT_SLOT_COUNT)
				{
					if ( iSlot <= TF_LOADOUT_SLOT_HAT || ( iSlot == TF_LOADOUT_SLOT_MEDAL || iSlot == TF_LOADOUT_SLOT_EVENT ) )	// Standard parse items in these slots.
					{
						if ( pItemDef->baseitem )
						{
							CEconItemView *pBaseItem = m_Items[iClass][iSlot][0];
							if ( pBaseItem != NULL )
							{
								Warning( "Duplicate base item %d for class %s in slot %s!\n", iItemID, g_aPlayerClassNames_NonLocalized[iClass], g_LoadoutSlots[iSlot] );
								delete pBaseItem;
							}

							CEconItemView *pNewItem = new CEconItemView( iItemID );
						
#if defined ( GAME_DLL )
							pNewItem->SetItemClassNumber( iClass );
#endif
							m_Items[iClass][iSlot][0] = pNewItem;
						}
						else if ( pItemDef->show_in_armory && ( pItemDef->is_reskin == 0 || bReskinsEnabled ) && ( pItemDef->specialitem == 0 || bSpecialsEnabled ) )
						{
							CEconItemView *pNewItem = new CEconItemView( iItemID );

#if defined ( GAME_DLL )
							pNewItem->SetItemClassNumber( iClass );
#endif
							m_Items[iClass][iSlot].AddToTail( pNewItem );
						}
					}
					else if (iSlot == TF_LOADOUT_SLOT_MISC1 ) // We need to duplicate across all misc slots.
					{
						for (int iMiscSlot = 1; iMiscSlot <= TF_PLAYER_MISC_COUNT; ++iMiscSlot)
						{
							switch (iMiscSlot)
							{
								case 1:
									iSlot = TF_LOADOUT_SLOT_MISC1;
									break;
									
								case 2:
									iSlot = TF_LOADOUT_SLOT_MISC2;
									break;
								
								case 3:
									iSlot = TF_LOADOUT_SLOT_MISC3;
									break;
							}
		
							if ( pItemDef->baseitem )
							{
								CEconItemView *pBaseItem = m_Items[iClass][iSlot][0];
								if ( pBaseItem != NULL )
								{
									Warning("Duplicate base item %d for class %s in slot %s!\n", iItemID, g_aPlayerClassNames_NonLocalized[iClass], g_LoadoutSlots[iSlot]);
									delete pBaseItem;
								}

								CEconItemView *pNewItem = new CEconItemView( iItemID );
							
#if defined ( GAME_DLL )
								pNewItem->SetItemClassNumber( iClass );
#endif
								m_Items[iClass][iSlot][0] = pNewItem;
							}
							else if ( pItemDef->show_in_armory && ( pItemDef->is_reskin == 0 || bReskinsEnabled ) && ( pItemDef->specialitem == 0 || bSpecialsEnabled ) )
							{
								CEconItemView *pNewItem = new CEconItemView( iItemID );

#if defined ( GAME_DLL )
								pNewItem->SetItemClassNumber( iClass );
#endif
								m_Items[iClass][iSlot].AddToTail(pNewItem);
							}
						}
					}
					else if (iSlot == TF_LOADOUT_SLOT_TAUNT1) // We need to duplicate across all taunt slots.
					{
						for (int iTauntSlot = 1; iTauntSlot <= TF_PLAYER_TAUNT_COUNT; ++iTauntSlot)
						{

							if (pItemDef->baseitem)
							{
								CEconItemView *pBaseItem = m_Items[iClass][iSlot][0];
								if (pBaseItem != NULL)
								{
									Warning("Duplicate base item %d for class %s in slot %s!\n", iItemID, g_aPlayerClassNames_NonLocalized[iClass], g_LoadoutSlots[iSlot]);
									delete pBaseItem;
								}

								CEconItemView *pNewItem = new CEconItemView(iItemID);

#if defined ( GAME_DLL )
								pNewItem->SetItemClassNumber(iClass);
#endif
								m_Items[iClass][iSlot][0] = pNewItem;
							}
							else if (pItemDef->show_in_armory && (pItemDef->is_reskin == 0 || bReskinsEnabled) && (pItemDef->specialitem == 0 || bSpecialsEnabled))
							{
								CEconItemView *pNewItem = new CEconItemView(iItemID);

#if defined ( GAME_DLL )
								pNewItem->SetItemClassNumber(iClass);
#endif
								m_Items[iClass][iSlot].AddToTail(pNewItem);
							}

							// Increase our slot value.
							iSlot += 1;
						}
						// Reset our slot back when we're done.
						iSlot = TF_LOADOUT_SLOT_TAUNT1;
					}
				}
			}
		}
	}


#if defined( CLIENT_DLL )
	LoadInventory();
#endif

	return true;
}

void CTFInventory::LevelInitPreEntity( void )
{
	GetItemSchema()->Precache();
}

int CTFInventory::GetNumPresets(int iClass, int iSlot)
{
	return m_Items[iClass][iSlot].Count();
};

int CTFInventory::GetWeapon(int iClass, int iSlot)
{
	return Weapons[iClass][iSlot];
};

CEconItemView *CTFInventory::GetItem( int iClass, int iSlot, int iNum )
{
	if ( CheckValidWeapon( iClass, iSlot, iNum ) == false )
		return NULL;

	return m_Items[iClass][iSlot][iNum];
};

bool CTFInventory::CheckValidSlot(int iClass, int iSlot, bool bHudCheck /*= false*/)
{
	if (iClass < TF_CLASS_UNDEFINED || iClass > TF_CLASS_COUNT_ALL)
		return false;

	int iCount = (bHudCheck ? INVENTORY_ROWNUM : TF_LOADOUT_SLOT_COUNT);

	// Array bounds check.
	if ( iSlot >= iCount || iSlot < 0 )
		return false;

	// Slot must contain a base item.
	if ( m_Items[iClass][iSlot][0] == NULL )
		return false;

	return true;
};

bool CTFInventory::CheckValidWeapon(int iClass, int iSlot, int iWeapon, bool bHudCheck /*= false*/)
{
	if (iClass < TF_CLASS_UNDEFINED || iClass > TF_CLASS_COUNT_ALL)
		return false;

	int iCount = ( bHudCheck ? INVENTORY_COLNUM : m_Items[iClass][iSlot].Count() );

	// Array bounds check.
	if ( iWeapon >= iCount || iWeapon < 0 )
		return false;

	// Don't allow switching if this class has no weapon at this position.
	if ( m_Items[iClass][iSlot][iWeapon] == NULL )
		return false;

	return true;
};

#if defined( CLIENT_DLL )
bool CTFInventory::CheckSpecialItemAccess()
{
	// Placeholder until we can implement SteamID checking here.
	return false;
	
}

void CTFInventory::LoadInventory()
{
	bool bExist = filesystem->FileExists("scripts/tf_inventory.txt", "MOD");
	if (bExist)
	{
		if (!m_pInventory)
		{
			m_pInventory = new KeyValues("Inventory");
		}
		m_pInventory->LoadFromFile(filesystem, "scripts/tf_inventory.txt");
	}
	else
	{
		ResetInventory();
	}
};

void CTFInventory::SaveInventory()
{
	m_pInventory->SaveToFile(filesystem, "scripts/tf_inventory.txt");
};

void CTFInventory::ResetInventory()
{
	if (m_pInventory)
	{
		m_pInventory->deleteThis();
	}

	m_pInventory = new KeyValues("Inventory");

	for (int i = TF_FIRST_NORMAL_CLASS; i <= TF_LAST_NORMAL_CLASS; i++)
	{
		KeyValues *pClassInv = new KeyValues(g_aPlayerClassNames_NonLocalized[i]);
		pClassInv->SetInt( "activeslot", 0 ); // Set our active loadout to A
		for (int j = 0; j < TF_MAX_PRESETS; j++) // A=0, B=1, C=2, D=3
		{
			KeyValues *pClassLoadoutSlot = new KeyValues(g_InventoryLoadoutPresets[j]);
			for (int k = 0; k < TF_LOADOUT_SLOT_COUNT; k++)
			{
				// Ignore special slots. (Gamemode specific, zombie, action)
				if (k == TF_LOADOUT_SLOT_UTILITY || k == TF_LOADOUT_SLOT_ACTION || k == TF_LOADOUT_SLOT_EVENT || k == TF_LOADOUT_SLOT_MEDAL )
					continue;
				
				pClassLoadoutSlot->SetInt( g_LoadoutSlots[k], 0 );
			}
			pClassInv->AddSubKey(pClassLoadoutSlot);
		}
		
		m_pInventory->AddSubKey(pClassInv);
	}

	SaveInventory();
}

int CTFInventory::GetWeaponPreset(int iClass, int iSlot)
{
	KeyValues *pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	if (!pClass)	//cannot find class node
	{	
		if (iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS)
			ResetInventory();
		return 0;
	}
	
	int iActiveSlot = pClass->GetInt("activeslot", 0);
	KeyValues *pClassLoadoutSlot = pClass->FindKey(g_InventoryLoadoutPresets[iActiveSlot]);
	if (!pClassLoadoutSlot)	//cannot find loadout node
	{	
		ResetInventory();
		return 0;
	}
			
	int iPreset = pClassLoadoutSlot->GetInt(g_LoadoutSlots[iSlot], -1);
	if (iPreset == -1 ) //cannot find slot node	
	{
		if ( ( iSlot != TF_LOADOUT_SLOT_UTILITY && iSlot != TF_LOADOUT_SLOT_ACTION && iSlot != TF_LOADOUT_SLOT_EVENT && iSlot != TF_LOADOUT_SLOT_MEDAL ) && iSlot < TF_LOADOUT_SLOT_COUNT )
			ResetInventory();
		return 0;
	}

	if ( CheckValidWeapon( iClass, iSlot, iPreset ) == false )
		return 0;
	
	return iPreset;
};

void CTFInventory::SetWeaponPreset(int iClass, int iSlot, int iPreset)
{
	KeyValues* pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	if (!pClass)	//cannot find class node
	{
		if (iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS)
			ResetInventory();
		pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	}
	int iActiveSlot = pClass->GetInt( "activeslot", 0 ); // Get the active loadout. Default to A.
	KeyValues* pClassLoadoutSlot = pClass->FindKey(g_InventoryLoadoutPresets[iActiveSlot]); // Find the loadout to save it to
	
	pClassLoadoutSlot->SetInt(GetSlotName(iSlot), iPreset);
	SaveInventory();
}

const char* CTFInventory::GetSlotName(int iSlot)
{
	return g_LoadoutSlots[iSlot];
};

int CTFInventory::GetCurrentLoadoutSlot(int iClass)
{
	int iActiveSlot = 0;
	KeyValues* pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	iActiveSlot = pClass->GetInt("activeslot", 0);
	return iActiveSlot;
}

void CTFInventory::ChangeLoadoutSlot(int iClass, int iLoadoutSlot)
{
	KeyValues* pClass = m_pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	pClass->SetInt( "activeslot", iLoadoutSlot );
	SaveInventory();
}

#endif

// Legacy array, used when we're forced to use old method of giving out weapons.
const int CTFInventory::Weapons[TF_CLASS_COUNT_ALL][TF_PLAYER_WEAPON_COUNT] =
{
	{

	},
	{
		TF_WEAPON_SCATTERGUN,
		TF_WEAPON_PISTOL_SCOUT,
		TF_WEAPON_BAT
	},
	{
		TF_WEAPON_SNIPERRIFLE,
		TF_WEAPON_SMG,
		TF_WEAPON_CLUB
	},
	{
		TF_WEAPON_ROCKETLAUNCHER,
		TF_WEAPON_SHOTGUN_SOLDIER,
		TF_WEAPON_SHOVEL
	},
	{
		TF_WEAPON_GRENADELAUNCHER,
		TF_WEAPON_PIPEBOMBLAUNCHER,
		TF_WEAPON_BOTTLE
	},
	{
		TF_WEAPON_SYRINGEGUN_MEDIC,
		TF_WEAPON_MEDIGUN,
		TF_WEAPON_BONESAW
	},
	{
		TF_WEAPON_MINIGUN,
		TF_WEAPON_SHOTGUN_HWG,
		TF_WEAPON_FISTS
	},
	{
		TF_WEAPON_FLAMETHROWER,
		TF_WEAPON_SHOTGUN_PYRO,
		TF_WEAPON_FIREAXE
	},
	{
		TF_WEAPON_REVOLVER,
		TF_WEAPON_NONE,
		TF_WEAPON_KNIFE,
		TF_WEAPON_PDA_SPY,
		TF_WEAPON_INVIS
	},
	{
		TF_WEAPON_SHOTGUN_PRIMARY,
		TF_WEAPON_PISTOL,
		TF_WEAPON_WRENCH,
		TF_WEAPON_PDA_ENGINEER_BUILD,
		TF_WEAPON_PDA_ENGINEER_DESTROY
	},
	{
		TF_WEAPON_SHOVELFIST
	},
};
