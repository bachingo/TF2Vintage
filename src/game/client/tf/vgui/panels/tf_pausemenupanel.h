#ifndef TFMAINMENUPAUSEPANEL_H
#define TFMAINMENUPAUSEPANEL_H

#include "tf_menupanelbase.h"
#include "steam/steam_api.h"

class CAvatarImagePanel;
class CTFAdvButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFPauseMenuPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFPauseMenuPanel, CTFMenuPanelBase);

public:
	CTFPauseMenuPanel(vgui::Panel* parent, const char *panelName);
	virtual ~CTFPauseMenuPanel();
	bool Init();

	void PerformLayout();
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void OnThink();
	void OnTick();
	void Show();
	void Hide();
	void OnCommand(const char* command);
	void DefaultLayout();
	void GameLayout();

private:
	CTFAdvButton	*m_pNotificationButton;
	CSteamID			m_SteamID;
	CAvatarImagePanel	*m_pProfileAvatar;
	CExLabel			*m_pVersionLabel;
};

#endif // TFMAINMENUPAUSEPANEL_H