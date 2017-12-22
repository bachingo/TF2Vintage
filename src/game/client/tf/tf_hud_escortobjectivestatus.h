//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_ESCORTOBJECTIVESTATUS_H
#define TF_HUD_ESCORTOBJECTIVESTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "GameEventListener.h"
#include "IconPanel.h"


#define TEAM_TRAIN_ALERT_DEFENSE	"Announcer.Cart.DefenseWarning"
#define TEAM_TRAIN_ALERT_ATTACK		"Announcer.Cart.AttackWarning"
#define TEAM_TRAIN_FINAL_ALERT_DEFENSE	"Announcer.Cart.DefenseFinalWarning"
#define TEAM_TRAIN_FINAL_ALERT_ATTACK	"Announcer.Cart.AttackFinalWarning"

class CTFHudEscortProgressBar;

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudEscort : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(CTFHudEscort, vgui::EditablePanel);

public:

	CTFHudEscort(vgui::Panel *parent, const char *name);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual bool IsVisible(void);
	virtual void Reset();
	void OnThink();
	void SetTeamCart(int Team) 
	{ 
		iTeamCart = Team; 
		Refresh();
	}

public: // IGameEventListener:
	virtual void FireGameEvent(IGameEvent *event);

private:
	void Refresh(void);
	void UpdateStatus(void);
	void Init(void);
	void SetPlayingToLabelVisible(bool bVisible);
	bool SetControlSettings(void);
	void SetupControlPoints(void);
	void SetupHills(void);
	void UpdateOwners(int capIndex);
	void UpdateHills(void);


	vgui::IImage					   *m_pHillArrow;
	vgui::IImage					   *m_pHillArrowFlipped;
	CUtlVector<vgui::ImagePanel *>     m_ControlPoints;
	CUtlVector<vgui::ImagePanel *>	   m_Hills;
	CTFHudEscortProgressBar			   *m_pProgressBar;

private:
	int							 BarLength;
	int							 BarTall;
	int							 BarXpos;
	int							 BarYpos;
	int							 iTeamCart;
	int							 iArrowWide;
	int							 iArrowTall;
	int							 m_iCurrentCP;
	float						 m_iCartPanelHome;
	float						 m_flTotalDistance;
	float						 m_flNextThink;
	float						 m_flNextAlphaIncrease;
	float						 m_flNextAlphaDecrease;
	float						 m_flControlThink;
	bool						 b_Visible;
	bool						 b_InMinHud;

	//EscortItemPanel methods
private:
	void UpdateItemPanel(void);

	vgui::EditablePanel			 *m_pCartPanel;
	CExLabel					 *m_pRecedeCountDown;
	CExLabel					 *m_pCapNumPlayers;
	vgui::ImagePanel			 *m_pCart;
	vgui::ImagePanel			 *m_pCartBottom;
	vgui::ImagePanel			 *m_pAlert;
	vgui::ImagePanel			 *m_pBackwards;
	vgui::ImagePanel			 *m_pCapPlayer;
	vgui::ImagePanel			 *m_pBlocked;

	int							 m_iTrainSpeedLevel;
	int							 m_nNumCappers;
	float						 m_flRecedeTime;

	//Teardrop
private:
	void UpdateTeardrop(int iCP);
	void UpdateProgressText(int iCP);

	vgui::EditablePanel			 *m_pTeardrop;
	vgui::Label					 *m_pProgress;
	vgui::ImagePanel			 *m_pCapping;
	CIconPanel					 *m_pTeardropIcon;
	CIconPanel					 *m_pTeardropBlocked;

};

class CTFHudMultipleEscort : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(CTFHudMultipleEscort, vgui::EditablePanel);

public:
	CTFHudMultipleEscort(vgui::Panel *parent, const char *name);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual bool IsVisible(void);
	virtual void Reset();
	void OnThink();

public: // IGameEventListener:
	virtual void FireGameEvent(IGameEvent *event);

private:
	void UpdateStatus(void);
	void SetControlSettings(void);

private:
	bool						 b_InMinHud;
	bool						 b_Visible;
	float						 m_flNextThink;
	CTFHudEscort				 *m_pRedEscortPanel;
	CTFHudEscort			     *m_pBlueEscortPanel;
};

class CTFHudEscortProgressBar : public vgui::ImagePanel
{
	DECLARE_CLASS_SIMPLE(CTFHudEscortProgressBar, vgui::ImagePanel);

public:
	CTFHudEscortProgressBar(vgui::Panel *parent, const char *name);

	void SetTeam(int team);
	void SetProgress(float progress);

private:

};
#endif	// TF_HUD_ESCORTOBJECTIVESTATUS_H