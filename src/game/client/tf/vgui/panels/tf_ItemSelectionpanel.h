#ifndef TFMAINMENUITEMSELECTIONPANEL_H
#define TFMAINMENUITEMSELECTIONPANEL_H

#include "Panels/tf_dialogpanelbase.h"
#include "tf_inventory.h"

class CTFWeaponSelectPanel;
class CModelPanel;
class CTFAdvButton;
class CTFAdvItemButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFItemPanel : public CTFDialogPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFItemPanel, CTFDialogPanelBase);

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
	void SetWeaponPreset(int iClass, int iSlot, int iPreset);
	void SetCurrentClass(int iClass);
	void SetCurrentClassAndSlot(int iClass, int iSlot);
	void SetCurrentSlot(int iSlot) { m_iCurrentSlot = iSlot; };
	int  GetAnimSlot(CEconItemDefinition *pItemDef, int iClass);
	void SetSlotAndPreset(int iSlot, int iPreset);
	void SideRow(int iRow, int iDir);
	void ResetRows();

private:
	CModelPanel		*m_pGameModelPanel;
	CTFWeaponSelectPanel *m_pWeaponSetPanel;
	CUtlVector<CTFAdvItemButton*> m_pWeaponIcons;
	CUtlVector<CTFAdvItemButton*> m_pSlideButtons;
	CUtlVector<int> m_RawIDPos;
	//MESSAGE_FUNC(UpdateModelPanels, "ControlModified");
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
};

#endif // TFMAINMENUITEMSELECTIONPANEL_H
