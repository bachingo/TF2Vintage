#ifndef TFMAINMENUITEMSELECTIONPANEL_H
#define TFMAINMENUITEMSELECTIONPANEL_H

#include "tf_menupanelbase.h"
#include "tf_inventory.h"

class CTFWeaponSelectPanel;
class CModelPanel;
class CTFAdvButton;
class CTFAdvItemButton;
extern bool g_bShowItemMenu;

static int g_aClassLoadoutSlots[TF_CLASS_COUNT_ALL][INVENTORY_ROWNUM] =
{
	{
		-1, -1, -1,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		-1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		-1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		-1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		-1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		-1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		-1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		-1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_PDA2,
		TF_LOADOUT_SLOT_BUILDING,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_PDA1,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
		TF_LOADOUT_SLOT_MISC3,
	},
};

struct _WeaponData
{
	char szWorldModel[64];
	char iconActive[64];
	char iconInactive[64];
	char szPrintName[64];
	int m_iWeaponType;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFItemPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFItemPanel, CTFMenuPanelBase);

public:
	CTFItemPanel(vgui::Panel* parent, const char *panelName);
	virtual ~CTFItemPanel();
	bool Init();
	void PerformLayout();
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void OnThink();
	void OnTick();
	void OnCommand(const char* command);
	void Hide();
	void Show();
	void DefaultLayout();
	void GameLayout();
	void SetCurrentClassAndSlot(int iClass, int iSlot);
	void SetupItemsPage( int page );
	void SetupItem( int iItem );

private:
	CTFWeaponSelectPanel *m_pWeaponSetPanel;
	CUtlVector<CTFAdvItemButton*> m_pWeaponIcons;
	//MESSAGE_FUNC(UpdateModelPanels, "ControlModified");
	CExLabel *m_pItemSlotLabel;

	int m_nPage;
	int m_nPageCount;

	int	m_iCurrentClass;
	int	m_iCurrentSlot;
	int m_iCurrentSkin;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFWeaponSelectPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CTFWeaponSelectPanel, vgui::EditablePanel);

public:
	CTFWeaponSelectPanel(vgui::Panel* parent, const char *panelName);
	void OnCommand(const char* command);
	void ApplySettings( KeyValues *inResourceData );

	int m_nItemColumns;
	int m_nItemRows;
	int m_nItemXSpace;
	int m_nItemYSpace;
	int m_nItemWidth;
	int m_nItemHeight;
};

#endif // TFMAINMENUITEMSELECTIONPANEL_H
