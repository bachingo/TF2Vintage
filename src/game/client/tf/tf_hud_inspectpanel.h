//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_HUD_INSPECTPANEL_H
#define TF_HUD_INSPECTPANEL_H
#ifdef _WIN32
#pragma once
#endif

class CTFAdvModelPanel;

class CHudInspectPanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudInspectPanel, vgui::EditablePanel );
public:
	CHudInspectPanel( char const *pszElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual int GetRenderGroupPriority( void ) { return 35; }
	void		UserCmd_InspectTarget( void );
	C_TFPlayer	*GetInspectTarget( C_TFPlayer *pPlayer );
	int			HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

private:
	void LockInspectRenderGroup( bool bLock );
	void SetPanelVisible( bool bVisible );

	CTFAdvModelPanel *m_pItemPanel;
};

#endif
