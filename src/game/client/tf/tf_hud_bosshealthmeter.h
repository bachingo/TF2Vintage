#ifndef TF_HUD_BOSSHEALTHMETER_H
#define TF_HUD_BOSSHEALTHMETER_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/ImagePanel.h>

class CHudBossHealthMeter : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudBossHealthMeter, vgui::EditablePanel );

public:
	CHudBossHealthMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

	void			Update( void );

private:
	vgui::ContinuousProgressBar *m_pStunMeter;
	vgui::EditablePanel *m_pHealthBarPanel;
	vgui::ImagePanel *m_pBarImage;
	vgui::ImagePanel *m_pBorderImage;

	CPanelAnimationVarAliasType( int, m_nHealthAlivePosY, "health_alive_pos_y", "42", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nHealthDeadPosY, "health_dead_pos_y", "90", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nHealthBarWide, "health_bar_wide", "168", "proportional_xpos" );
};

#endif