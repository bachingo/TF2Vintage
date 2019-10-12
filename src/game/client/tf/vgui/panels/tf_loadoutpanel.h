#ifndef TFMAINMENULOADOUTPANEL_H
#define TFMAINMENULOADOUTPANEL_H

#include "tf_dialogpanelbase.h"
#include "tf_inventory.h"

class CTFAdvModelPanel;
class CTFWeaponSetPanel;
class CModelPanel;
class CTFAdvButton;
class CTFAdvItemButton;
class CTFLoadoutPanel_Inventory;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFLoadoutPanel : public CTFDialogPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFLoadoutPanel, CTFDialogPanelBase);

public:
	CTFLoadoutPanel(vgui::Panel* parent, const char *panelName);
	virtual ~CTFLoadoutPanel();
	bool Init();
	void PerformLayout();
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void OnThink();
	void OnTick();
	virtual void Paint();
	bool InSelection = false;
	void OnCommand(const char* command);
	void Hide();
	void Show();
	void Show_Inventory();
	void DefaultLayout();
	void GameLayout();
	void SetWeaponPreset(int iClass, int iSlot, int iPreset);
	void SetCurrentClass(int iClass);
	void SetCurrentSlot(int iSlot) { m_iCurrentSlot = iSlot; };
	int  GetAnimSlot( CEconItemDefinition *pItemDef, int iClass );
	const char *GetWeaponModel( CEconItemDefinition *pItemDef, int iClass );
	const char *GetExtraWearableModel( CEconItemDefinition *pItemDef );
	void UpdateModelWeapons( void );
	void SetModelClass(int iClass);
	void SetSlotAndPreset(int iSlot, int iPreset);
	void SideRow(int iRow, int iDir);
	void ResetRows();

private:
	CTFAdvModelPanel *m_pClassModelPanel;
	CModelPanel		*m_pGameModelPanel;
	CTFWeaponSetPanel *m_pWeaponSetPanel;
	CUtlVector<CTFAdvItemButton*> m_pWeaponIcons;
	CUtlVector<CTFAdvItemButton*> m_pSlideButtons;
	CUtlVector<int> m_RawIDPos;
	MESSAGE_FUNC(UpdateModelPanels, "ControlModified");
	int	m_iCurrentClass;
	int	m_iCurrentSlot;
	int m_iCurrentSkin;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFWeaponSetPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CTFWeaponSetPanel, vgui::EditablePanel);

public:
	CTFWeaponSetPanel(vgui::Panel* parent, const char *panelName);
	void OnCommand(const char* command);
};

/*class CTFLoadoutPanel_Inventory : public CTFDialogPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFLoadoutPanel_Inventory, CTFDialogPanelBase);

	CTFLoadoutPanel_Inventory(vgui::Panel* parent, const char *panelName);
	virtual ~CTFLoadoutPanel_Inventory();
	bool Init();
	void PerformLayout();
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void OnThink();
	void OnTick();
	virtual void Paint();
	void OnCommand(const char* command);
	void Hide();
	void Show();
	void DefaultLayout();
	void GameLayout();
	void SetWeaponPreset(int iClass, int iSlot, int iPreset);
	void SetCurrentClass(int iClass);
	void SetCurrentSlot(int iSlot) { m_iCurrentSlot = iSlot; };
	int  GetAnimSlot(CEconItemDefinition *pItemDef, int iClass);
	const char *GetWeaponModel(CEconItemDefinition *pItemDef, int iClass);
	const char *GetExtraWearableModel(CEconItemDefinition *pItemDef);
	void UpdateModelWeapons(void);
	void SetModelClass(int iClass);
	void SetSlotAndPreset(int iSlot, int iPreset);
	void SideRow(int iRow, int iDir);
	void ResetRows();

private:
	CTFAdvModelPanel *m_pClassModelPanel;
	CModelPanel		*m_pGameModelPanel;
	CTFWeaponSetPanel *m_pWeaponSetPanel;
	CUtlVector<CTFAdvItemButton*> m_pWeaponIcons;
	CUtlVector<CTFAdvItemButton*> m_pSlideButtons;
	CUtlVector<int> m_RawIDPos;
	MESSAGE_FUNC(UpdateModelPanels, "ControlModified");
	int	m_iCurrentClass;
	int	m_iCurrentSlot;
	int m_iCurrentSkin;
};*/

#endif // TFMAINMENULOADOUTPANEL_H
