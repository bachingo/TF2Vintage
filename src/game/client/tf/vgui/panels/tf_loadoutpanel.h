#ifndef TFMAINMENULOADOUTPANEL_H
#define TFMAINMENULOADOUTPANEL_H

#include "tf_menupanelbase.h"
#include "tf_inventory.h"
#include "panels/tf_itemselectionpanel.h"

class CTFAdvModelPanel;
class CTFWeaponSetPanel;
class CModelPanel;
class CTFAdvButton;
class CTFAdvItemButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFLoadoutPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFLoadoutPanel, CTFMenuPanelBase);

public:
	CTFLoadoutPanel(vgui::Panel* parent, const char *panelName);
	virtual ~CTFLoadoutPanel();
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
	void SetWeaponPreset(int iClass, int iSlot, int iPreset);
	void SetCurrentClass(int iClass);
	void SetCurrentSlot(int iSlot) { m_iCurrentSlot = iSlot; };
	int  GetAnimSlot( CEconItemDefinition *pItemDef, int iClass );
	const char *GetWeaponModel( CEconItemDefinition *pItemDef, int iClass );
	const char *GetExtraWearableModel( CEconItemDefinition *pItemDef );
	void UpdateModelWeapons( void );
	void UpdateMenuBodygroups(void);
	void SetModelClass(int iClass);
	void SetSlotAndPreset(int iSlot, int iPreset);
	void ResetRows();
	int GetCurrentClass(void) {return m_iCurrentClass;}

private:
	CTFAdvModelPanel *m_pClassModelPanel;
	CModelPanel		*m_pGameModelPanel;
	CUtlVector<CTFAdvItemButton*> m_pWeaponIcons;
	CUtlVector<CTFAdvItemButton*> m_pSlideButtons;
	CUtlVector<int> m_RawIDPos;
	MESSAGE_FUNC(UpdateModelPanels, "ControlModified");
	int	m_iCurrentClass;
	int	m_iCurrentSlot;
	int m_iCurrentSkin;
	CTFItemPanel *m_pItemPanel;
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

#endif // TFMAINMENULOADOUTPANEL_H
