//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Create Server Dialog with bot count selection
//
//=============================================================================//

#ifndef TF_CREATE_SERVER_DIALOG_H
#define TF_CREATE_SERVER_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Button.h>

//-----------------------------------------------------------------------------
// Purpose: Dialog for creating a local server with bot configuration
//-----------------------------------------------------------------------------
class CTFCreateServerDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CTFCreateServerDialog, vgui::Frame );

public:
	CTFCreateServerDialog( vgui::Panel *parent );
	~CTFCreateServerDialog();

	virtual void Activate();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void OnClose( void );

private:
	void LoadMapList();
	void StartServer();

	vgui::ComboBox *m_pMapComboBox;
	vgui::ComboBox *m_pBotCountComboBox;
	vgui::ComboBox *m_pBotDifficultyComboBox;
	vgui::Button *m_pStartButton;
	vgui::Button *m_pCancelButton;
};

// Global function to show the dialog
void ShowCreateServerDialog( vgui::Panel *pParent = NULL );

#endif // TF_CREATE_SERVER_DIALOG_H
