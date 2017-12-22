//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: Hud logic for Payload and Payload Race gamemodes
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "c_baseentity.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui/IBorder.h>
#include <vgui_controls/Label.h>
#include "IconPanel.h"

#include "c_playerresource.h"
#include "teamplay_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_objectivestatus.h"
#include "tf_hud_escortobjectivestatus.h"
#include "tf_spectatorgui.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_freezepanel.h"

#include "tf_gamerules.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"

using namespace vgui;

DECLARE_BUILD_FACTORY(CTFHudEscort);
DECLARE_BUILD_FACTORY(CTFHudEscortProgressBar);
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudEscort::CTFHudEscort(Panel *parent, const char *name) : EditablePanel(parent, name)
{
	m_flNextThink = 0.0f;
	m_flTotalDistance = 0.0f;
	m_flNextAlphaIncrease = 0.0f;
	m_flNextAlphaDecrease = 0.0f;
	BarLength = 0;
	BarTall = 0;
	BarXpos = 0;
	BarYpos = 0;
	m_iCartPanelHome = 0;
	m_iCurrentCP = -1;
	iTeamCart = TF_TEAM_BLUE;
	b_Visible = false;
	b_InMinHud = false;

	vgui::ivgui()->AddTickSignal(GetVPanel(), 500);

	m_pHillArrow = scheme()->GetImage("../hud/cart_track_arrow", false);
	if (m_pHillArrow)
		m_pHillArrow->GetSize(iArrowWide, iArrowTall);
	else
	{
		m_pHillArrow = NULL;
		iArrowWide = 0;
		iArrowTall = 0;
	}
	m_pHillArrowFlipped = scheme()->GetImage("../hud/cart_track_arrow_flipped", false);
	if (!m_pHillArrow)
		m_pHillArrow = NULL;

	m_pCartPanel = NULL;
	m_pRecedeCountDown = NULL;
	m_pCapNumPlayers = NULL;
	m_pCart = NULL;
	m_pCartBottom = NULL;
	m_pAlert = NULL;
	m_pBackwards = NULL;
	m_pCapPlayer = NULL;
	m_pBlocked = NULL;

	m_pTeardrop = NULL;
	m_pProgress = NULL;
	m_pCapping = NULL;
	m_pTeardropIcon = NULL;
	m_pTeardropBlocked = NULL;

	m_iTrainSpeedLevel = 0;
	m_nNumCappers = 0;
	 m_flRecedeTime = 0;
;
	ListenForGameEvent("escort_play_alert");
	ListenForGameEvent("localplayer_changeteam");
	ListenForGameEvent("controlpoint_initialized");
	ListenForGameEvent("controlpoint_unlock_updated");
	ListenForGameEvent("controlpoint_updateimages");
	ListenForGameEvent("controlpoint_updatecapping");
	ListenForGameEvent("controlpoint_starttouch");
	ListenForGameEvent("controlpoint_endtouch");
	//ListenForGameEvent("localplayer_changeteam");
}


//-----------------------------------------------------------------------------
// Purpose: Determines hud visibility
//-----------------------------------------------------------------------------
bool CTFHudEscort::IsVisible(void)
{
	if (IsInFreezeCam())
	{
		return false;
	}

	if (!TFGameRules() || !TFGameRules()->IsInEscortMode())
	{
		return false;
	}

	if (TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING)
	{
		return false;
	}

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	 //load control settings...
	//SetControlSettings();
	//LoadControlSettings("resource/UI/objectivestatusescort.res");

}

//-----------------------------------------------------------------------------
// Purpose: Resets think
//-----------------------------------------------------------------------------
void CTFHudEscort::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05;
}

//-----------------------------------------------------------------------------
// Purpose: Resets values and prepares hud for initialization
//-----------------------------------------------------------------------------
void CTFHudEscort::Refresh(void)
{
	m_flTotalDistance = 0.0f;
	m_flNextAlphaIncrease = 0.0f;
	m_flNextAlphaDecrease = 0.0f;
	b_Visible = false;
	BarLength = 0;
	BarTall = 0;
	BarXpos = 0;
	BarYpos = 0;
	m_iCurrentCP = -1;
	m_iCartPanelHome = 0;
	for (int i = 0; i < m_ControlPoints.Count(); i++)
	{
		m_ControlPoints[i]->MarkForDeletion();
	}
	m_ControlPoints.RemoveAll();

	for (int i = 0; i < m_Hills.Count(); i++)
	{
		m_Hills[i]->MarkForDeletion();
	}
	m_Hills.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::SetPlayingToLabelVisible(bool bVisible)
{	
}

//-----------------------------------------------------------------------------
// Purpose: Loads control for hud and casts panel objects
//-----------------------------------------------------------------------------
bool CTFHudEscort::SetControlSettings(void)
{
	if (!TFGameRules()->HasMultipleTrains())
	{
		KeyValues *pCond = new KeyValues("Escort");
		if (ObjectiveResource()->GetNumNodeHillData(TF_TEAM_BLUE) > 0)
		{
			
			pCond->AddSubKey(new KeyValues("if_single_with_hills"));
			pCond->AddSubKey(new KeyValues("if_single_with_hills_blue"));
			pCond->AddSubKey(new KeyValues("if_team_blue"));
		}
		else
		{
			pCond->AddSubKey(new KeyValues("if_team_blue"));
		}
		LoadControlSettings("resource/UI/objectivestatusescort.res", 0, 0, pCond);
		pCond->deleteThis();
	}
	else 
	{
		KeyValues *pCond = new KeyValues("MultipleEscort");
		pCond->AddSubKey(new KeyValues("if_multiple_trains"));

		if (iTeamCart == GetLocalPlayerTeam())
		{
			pCond->AddSubKey(new KeyValues("if_multiple_trains_top"));
		}
		else
		{
			pCond->AddSubKey(new KeyValues("if_multiple_trains_bottom"));
		}

		if (iTeamCart == TF_TEAM_RED)
		{
			pCond->AddSubKey(new KeyValues("if_team_red"));
			pCond->AddSubKey(new KeyValues("if_multiple_trains_red"));
		}
		else if (iTeamCart == TF_TEAM_BLUE)
		{
			pCond->AddSubKey(new KeyValues("if_multiple_trains_blue"));
			pCond->AddSubKey(new KeyValues("if_team_blue"));
		}
		LoadControlSettings("resource/UI/objectivestatusescort.res",0,0,pCond);
		pCond->deleteThis();
	}

	m_pProgressBar = dynamic_cast<CTFHudEscortProgressBar *>(FindChildByName("ProgressBar"));
	if (m_pProgressBar)
	{
		m_pProgressBar->SetTeam(iTeamCart);
	}

	m_pCartPanel = dynamic_cast<vgui::EditablePanel *>(FindChildByName("EscortItemPanel"));
	if (m_pCartPanel)
	{
		m_pRecedeCountDown = dynamic_cast<CExLabel *>(m_pCartPanel->FindChildByName("RecedeTime"));
		m_pCapNumPlayers = dynamic_cast<CExLabel *>(m_pCartPanel->FindChildByName("CapNumPlayers"));
		m_pCart = dynamic_cast<vgui::ImagePanel *>(m_pCartPanel->FindChildByName("EscortItemImage"));
		m_pCartBottom = dynamic_cast<vgui::ImagePanel *>(m_pCartPanel->FindChildByName("EscortItemImageBottom"));
		m_pAlert = dynamic_cast<vgui::ImagePanel *>(m_pCartPanel->FindChildByName("EscortItemImageAlert"));
		m_pBackwards = dynamic_cast<vgui::ImagePanel *>(m_pCartPanel->FindChildByName("Speed_Backwards"));
		m_pCapPlayer = dynamic_cast<vgui::ImagePanel *>(m_pCartPanel->FindChildByName("CapPlayerImage"));
		m_pBlocked = dynamic_cast<vgui::ImagePanel *>(m_pCartPanel->FindChildByName("Blocked"));

		m_pTeardrop = dynamic_cast<vgui::EditablePanel *>(m_pCartPanel->FindChildByName("EscortTeardrop"));
		m_pProgress = dynamic_cast<vgui::Label *>(m_pTeardrop->FindChildByName("ProgressText"));
		m_pCapping = dynamic_cast<vgui::ImagePanel *>(m_pTeardrop->FindChildByName("Capping"));
		m_pTeardropIcon = dynamic_cast<CIconPanel *>(m_pTeardrop->FindChildByName("Teardrop"));
		m_pTeardropBlocked = dynamic_cast<CIconPanel *>(m_pTeardrop->FindChildByName("Blocked"));

		KeyValues *m_pData = new KeyValues("Position Values");
		FindChildByName("LevelBar")->GetSettings(m_pData);
		BarLength = m_pData->GetInt("wide");
		BarTall = m_pData->GetInt("tall");
		BarXpos = m_pData->GetInt("xpos");
		BarYpos = m_pData->GetInt("ypos");
		m_pCartPanel->GetSettings(m_pData);
		m_iCartPanelHome += m_pData->GetInt("xpos");
		m_pData->deleteThis();

		if ((GetLocalPlayerTeam() != iTeamCart && TeamplayRoundBasedRules()->HasMultipleTrains() && GetLocalPlayerTeam() > TEAM_SPECTATOR) || (GetLocalPlayerTeam() == TEAM_SPECTATOR && iTeamCart != TF_TEAM_RED))
		{
			m_pCart->SetVisible(false);
			m_pCartBottom->SetVisible(true);
		}

		b_Visible = true;
		return true;
	}
	return false;

}

//-----------------------------------------------------------------------------
// Purpose: Initializes control points 
//-----------------------------------------------------------------------------
void CTFHudEscort::SetupControlPoints(void)
{
	for (int i = 0; i < ObjectiveResource()->GetNumControlPoints(); i++)
	{
		vgui::ImagePanel *m_pCP = new vgui::ImagePanel(this, VarArgs("ControlPoint%d", i));
		KeyValues *m_pKV = new KeyValues(VarArgs("ControlPointSettings%d", i));
		FindChildByName("SimpleControlPointTemplate")->GetSettings(m_pKV);

		m_pCP->SetShouldScaleImage(true);
		m_pCP->SetPos(scheme()->GetProportionalScaledValue(m_pKV->GetInt("xpos") + ObjectiveResource()->GetPathDistance(i) * BarLength), scheme()->GetProportionalScaledValue(m_pKV->GetInt("ypos")));
		m_pCP->SetSize(scheme()->GetProportionalScaledValue(m_pKV->GetInt("wide")), scheme()->GetProportionalScaledValue(m_pKV->GetInt("tall")));
		m_pCP->SetImage(m_pKV->GetString("image"));
		m_pCP->SetZPos(m_pKV->GetInt("zpos"));
		//m_pCP->ApplySettings(m_pKV);
		m_ControlPoints.AddToTail(m_pCP);
		if (m_ControlPoints.IsValidIndex(i) && ObjectiveResource()->IsCPVisible(i) && ObjectiveResource()->IsInMiniRound(i))
		{
			UpdateOwners(i);
			m_ControlPoints[i]->SetVisible(true);
		}
		else
			m_ControlPoints[i]->SetVisible(false);
		m_pKV->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializes hills 
//-----------------------------------------------------------------------------
void CTFHudEscort::SetupHills(void)
{
	int iNumHill = ObjectiveResource()->GetNumNodeHillData(iTeamCart);
	if ( iNumHill > 0)
	{
		for (int i = 0; i < iNumHill; i++)
		{
			float m_flStart = 0, m_flEnd = 0, m_flRatio = 0;
			ObjectiveResource()->GetHillData(iTeamCart, i, m_flStart, m_flEnd);

			vgui::ImagePanel *m_pHill = new vgui::ImagePanel(this, VarArgs("Hill%d", i));
			m_pHill->SetPos(scheme()->GetProportionalScaledValue(BarXpos + m_flStart * BarLength), scheme()->GetProportionalScaledValue(BarYpos + 1));
			m_pHill->SetSize(scheme()->GetProportionalScaledValue((m_flEnd - m_flStart + 0.015) * BarLength), scheme()->GetProportionalScaledValue(BarTall));
			if (TeamplayRoundBasedRules()->HasMultipleTrains())
				m_pHill->SetZPos(4);
			else
				m_pHill->SetZPos(3);
			m_pHill->SetHorizontalTile(true);
			m_flRatio = static_cast<float>(BarTall) / static_cast<float>(iArrowTall);
			//m_pHillArrow->SetSize(scheme()->GetProportionalScaledValue(iArrowWide * m_flRatio), scheme()->GetProportionalScaledValue(BarTall) - 1);

			if (ObjectiveResource()->IsHillDownhill(iTeamCart, i))
			{
				m_pHillArrowFlipped->SetSize(scheme()->GetProportionalScaledValue(iArrowWide * m_flRatio), scheme()->GetProportionalScaledValue(BarTall) - 1);
				m_pHill->SetImage(m_pHillArrowFlipped);
			}
			else
			{
				m_pHillArrow->SetSize(scheme()->GetProportionalScaledValue(iArrowWide * m_flRatio), scheme()->GetProportionalScaledValue(BarTall) - 1);
				m_pHill->SetImage(m_pHillArrow);
			}

			m_pHill->SetAlpha(120);
			m_Hills.AddToTail(m_pHill);
			if (m_Hills.IsValidIndex(i))
				m_Hills[i]->SetVisible(true);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Changes current owner of a control point
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateOwners(int capIndex)
{
	if (m_ControlPoints.IsValidIndex(capIndex) && ObjectiveResource()->IsCPVisible(capIndex) && ObjectiveResource()->IsInMiniRound(capIndex))
	{
		if (!TFGameRules()->HasMultipleTrains() || ObjectiveResource()->GetNumNodeHillData(TF_TEAM_BLUE) > 0)
		{
			if (ObjectiveResource()->GetOwningTeam(capIndex) == TF_TEAM_BLUE)
				m_ControlPoints[capIndex]->SetImage("../hud/cart_point_blue_opaque");
			else if (ObjectiveResource()->GetOwningTeam(capIndex) == TF_TEAM_RED)
				m_ControlPoints[capIndex]->SetImage("../hud/cart_point_red_opaque");
			else
				m_ControlPoints[capIndex]->SetImage("../hud/cart_point_neutral_opaque");		
		}
		else
		{
			if (ObjectiveResource()->GetOwningTeam(capIndex) == TF_TEAM_BLUE)
				m_ControlPoints[capIndex]->SetImage("../hud/cart_point_blue");
			else if (ObjectiveResource()->GetOwningTeam(capIndex) == TF_TEAM_RED)
				m_ControlPoints[capIndex]->SetImage("../hud/cart_point_red");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates status and prepares next think
//-----------------------------------------------------------------------------
void CTFHudEscort::OnThink(void)
{
	if (IsVisible() && m_flNextThink < gpGlobals->curtime)
	{
		m_iTrainSpeedLevel = ObjectiveResource()->GetTrainSpeedLevel(iTeamCart);
		m_nNumCappers = ObjectiveResource()->GetNumCappers(iTeamCart);
		m_flRecedeTime = ObjectiveResource()->GetRecedeTime(iTeamCart) - gpGlobals->curtime;

		UpdateStatus();
		m_flNextThink = gpGlobals->curtime + 0.1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates hud
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateStatus(void)
{
	ConVarRef cl_hud_minmode("cl_hud_minmode", true);
	if (!TeamplayRoundBasedRules()->HasMultipleTrains())
	{
		/*if (TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING)
		{
			for (int i = 0; i < GetChildCount(); i++)
			{
				GetChild(i)->SetVisible(false);
			}
		}*/

		if (cl_hud_minmode.IsValid() && cl_hud_minmode.GetInt() != 0 && !b_InMinHud)
		{
			Refresh();
			b_InMinHud = true;
		}
		else if (cl_hud_minmode.IsValid() && !cl_hud_minmode.GetInt() == 0 && b_InMinHud)
		{
			Refresh();
			b_InMinHud = false;
		}
	}

	if (TeamplayRoundBasedRules()->State_Get() == GR_STATE_PREROUND)
	{
		Refresh();
	}

	if (!b_Visible && TeamplayRoundBasedRules()->State_Get() == GR_STATE_RND_RUNNING)
	{
		if (SetControlSettings())
		{
			SetupControlPoints();
			SetupHills();
		}
	}

	if (b_Visible)
	{
		if (TeamplayRoundBasedRules()->HasMultipleTrains() && m_pProgressBar)
		{
			m_pProgressBar->SetProgress(ObjectiveResource()->GetTotalProgress(iTeamCart) * BarLength);
		}

		UpdateItemPanel();
		UpdateTeardrop(m_iCurrentCP);
	}

}

//-----------------------------------------------------------------------------
// Purpose: Updates Cart Panel and its children
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateItemPanel(void)
{
	if (TFGameRules()->PointsMayBeCaptured())
	{
		float newPos = m_iCartPanelHome + ObjectiveResource()->GetTotalProgress(iTeamCart) * BarLength;
		m_flTotalDistance = newPos - m_iCartPanelHome;
		vgui::GetAnimationController()->RunAnimationCommand(m_pCartPanel, "xpos", scheme()->GetProportionalScaledValue(newPos), 0.0f, 0.0f, vgui::AnimationController::INTERPOLATOR_LINEAR);
	}

	if (m_iTrainSpeedLevel > 0)
	{
		m_pBackwards->SetVisible(false);
		m_pRecedeCountDown->SetVisible(false);
		m_pBlocked->SetVisible(false);

		if (m_nNumCappers >= 3)
		{
			m_pCapNumPlayers->SetText("x3");
			m_pCapNumPlayers->SetVisible(true);
			m_pCapPlayer->SetVisible(true);
		}
		else  if (m_nNumCappers == 2)
		{
			m_pCapNumPlayers->SetText("x2");
			m_pCapNumPlayers->SetVisible(true);
			m_pCapPlayer->SetVisible(true);
		}
		else if (m_nNumCappers == 1)
		{
			m_pCapNumPlayers->SetText("x1");
			m_pCapNumPlayers->SetVisible(true);
			m_pCapPlayer->SetVisible(true);
		}
		else
		{
			m_pCapNumPlayers->SetVisible(false);
			m_pCapPlayer->SetVisible(false);
		}
	}
	else if (m_iTrainSpeedLevel == 0)
	{
			m_pBackwards->SetVisible(false);
			m_pCapNumPlayers->SetVisible(false);
			m_pCapPlayer->SetVisible(false);

			if (m_nNumCappers == -1)
			{
				m_pBlocked->SetVisible(true);
			}
			else
			{
				m_pBlocked->SetVisible(false);
				if (m_flRecedeTime <= 20.0 && m_flRecedeTime > 0)
				{
					wchar_t buf[16];
					swprintf(buf, sizeof(buf) / sizeof(*buf), L"%d", static_cast<int>(ceil(m_flRecedeTime)));
					m_pRecedeCountDown->SetText(buf);
					m_pRecedeCountDown->SetVisible(true);
				}
				else
				{
					m_pRecedeCountDown->SetVisible(false);
				}
			}
	}
	else
	{
		m_pBlocked->SetVisible(false);
		m_pBackwards->SetVisible(true);
		m_pCapNumPlayers->SetVisible(false);
		m_pCapPlayer->SetVisible(false);
		m_pRecedeCountDown->SetVisible(false);
	}

	if (TFGameRules()->HasMultipleTrains())
	{
		if (ObjectiveResource()->GetTrackAlarm(iTeamCart))
		{
			if (m_flNextAlphaDecrease <= gpGlobals->curtime)
			{
				m_pAlert->SetVisible(1);
				vgui::GetAnimationController()->RunAnimationCommand(m_pAlert, "alpha", 0, 0.0f, 1.0f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
				//vgui::GetAnimationController()->RunAnimationCommand(m_pAlert, "alpha", 255, 3.0f, 2.0f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
				m_flNextAlphaDecrease = gpGlobals->curtime + 2.0f;
				m_flNextAlphaIncrease = gpGlobals->curtime + 1.0f;
			}
			else if (m_flNextAlphaIncrease <= gpGlobals->curtime)
			{
				m_pAlert->SetVisible(1);
				vgui::GetAnimationController()->RunAnimationCommand(m_pAlert, "alpha", 255, 0.0f, 1.0f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
				m_flNextAlphaIncrease = gpGlobals->curtime + 2.0f;
				m_flNextAlphaDecrease = gpGlobals->curtime + 1.0f;
			}
		}
		else
			m_pAlert->SetVisible(0);
	}

}

//-----------------------------------------------------------------------------
// Purpose: Updates teardrop and its children
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateTeardrop(int iCP)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	bool bInWinState = TeamplayRoundBasedRules() ? TeamplayRoundBasedRules()->RoundHasBeenWon() : false;

	if (iCP == -1)
	{
		m_pTeardrop->SetVisible(false);
		return;
	}

	int iPlayerTeam = pPlayer->GetTeamNumber();

	if (!bInWinState)
	{
		if (m_iTrainSpeedLevel > 0 && iTeamCart == iPlayerTeam)
		{
			m_pTeardrop->SetVisible(true);
			m_pTeardropBlocked->SetVisible(false);
			m_pProgress->SetVisible(false);
			m_pCapping->SetVisible(true);
		}
		else
		{
				if (m_nNumCappers > 0)
				{
						m_pTeardrop->SetVisible(false);
				}
				else
				{
					m_pTeardrop->SetVisible(true);
					m_pCapping->SetVisible(false);
					m_pTeardropBlocked->SetVisible(true);
					UpdateProgressText(iCP);
				}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determines if cap is blocked and returns progress text
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateProgressText(int iCP)
{
	if (!TeamplayRoundBasedRules()->HasMultipleTrains())
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if (!pPlayer || !m_pProgress)
			return;

		m_pProgress->SetVisible(true);

		int iCappingTeam = ObjectiveResource()->GetCappingTeam(iCP);
		int iPlayerTeam = pPlayer->GetTeamNumber();
		int iOwnerTeam = ObjectiveResource()->GetOwningTeam(iCP);

		if (!TeamplayGameRules()->PointsMayBeCaptured())
		{
			m_pProgress->SetText("#Team_Capture_NotNow");
			return;
		}

		else if (m_nNumCappers == -1)
		{
			if (iCappingTeam != TEAM_UNASSIGNED && iCappingTeam != iPlayerTeam)
			{
				m_pProgress->SetText("#Team_Blocking_Capture");
				return;
			}
			else if (iCappingTeam != TEAM_UNASSIGNED && iCappingTeam == iPlayerTeam)
			{
				m_pProgress->SetText("#Team_Capture_Blocked");
				return;
			}
		}

		else if (iOwnerTeam == iPlayerTeam)
		{
			m_pProgress->SetText("#Team_Capture_OwnPoint");
			return;
		}
	}

	m_pProgress->SetVisible(false);
	m_pTeardrop->SetVisible(false);
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Gets fired events and updates hud state accordingly
//-----------------------------------------------------------------------------
void CTFHudEscort::FireGameEvent(IGameEvent *event)
{
	const char *eventname = event->GetName();
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (IsVisible())
	{
		m_flNextThink = gpGlobals->curtime;

		if (FStrEq("controlpoints_initialized", eventname))
		{
			// Update Hud position
			if (!TeamplayRoundBasedRules()->HasMultipleTrains())
				Refresh();
			return;
		}
		else if (FStrEq("localplayer_changeteam", eventname))
		{
			// Update Hud position
			if (GetLocalPlayerTeam() > TEAM_SPECTATOR && !TeamplayRoundBasedRules()->HasMultipleTrains())
				Refresh();
			return;
		}
		else if (FStrEq("controlpoint_updateimages", eventname))
		{
			// Update the images of our control point
			for (int i = 0; i < m_ControlPoints.Count(); i++)
			{
				UpdateOwners(i);
			}
			return;
		}
		else if (FStrEq("controlpoint_starttouch", eventname))
		{
			int iPlayer = event->GetInt("player");
			if (pPlayer && iPlayer == pPlayer->entindex() && m_pTeardrop)
			{
				m_iCurrentCP = event->GetInt("area");
				UpdateTeardrop(m_iCurrentCP);
			}
			return;
		}
		else if (FStrEq("controlpoint_endtouch", eventname))
		{
			int iPlayer = event->GetInt("player");
			if (pPlayer && iPlayer == pPlayer->entindex() && m_pTeardrop)
			{
				m_iCurrentCP = -1;
				UpdateTeardrop(m_iCurrentCP);
			}
			return;
		}
		else if (FStrEq("escort_play_alert", eventname))
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if (pLocalPlayer && ObjectiveResource()->GetTotalProgress(iTeamCart) > 0.0)
			{
				if ((iTeamCart == TF_TEAM_BLUE  && event->GetBool("teamblue")) || (iTeamCart != TF_TEAM_BLUE && !event->GetBool("teamblue")))
				{
					if (event->GetBool("final"))
					{
						if ((iTeamCart == GetLocalPlayerTeam()) || (!TeamplayRoundBasedRules()->HasMultipleTrains() && GetLocalPlayerTeam() == TF_TEAM_BLUE))
							pLocalPlayer->EmitSound(MAKE_STRING(TEAM_TRAIN_FINAL_ALERT_ATTACK));
						else
							pLocalPlayer->EmitSound(MAKE_STRING(TEAM_TRAIN_FINAL_ALERT_DEFENSE));
					}
					else
					{
						if (iTeamCart == GetLocalPlayerTeam())
							pLocalPlayer->EmitSound(MAKE_STRING(TEAM_TRAIN_ALERT_ATTACK));
						else
							pLocalPlayer->EmitSound(MAKE_STRING(TEAM_TRAIN_ALERT_DEFENSE));
					}
				}
			}
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudMultipleEscort::CTFHudMultipleEscort(Panel *parent, const char *name) : EditablePanel(parent, name)
{
	m_flNextThink = 0.0f;
	b_Visible = false;
	b_InMinHud = false;

	vgui::ivgui()->AddTickSignal(GetVPanel(), 500);

	m_pBlueEscortPanel = NULL;
	m_pRedEscortPanel = NULL;

	ListenForGameEvent("controlpoints_initialized");
	ListenForGameEvent("localplayer_changeteam");
}

//-----------------------------------------------------------------------------
// Purpose: Determines hud visibility
//-----------------------------------------------------------------------------
bool CTFHudMultipleEscort::IsVisible(void)
{
	if (IsInFreezeCam())
	{
		return false;
	}

	if (!TFGameRules() || !TeamplayRoundBasedRules()->HasMultipleTrains())
	{
		return false;
	}

	if (TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING)
	{
		return false;
	}

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("resource/UI/objectivestatusmultipleescort.res");
}

//-----------------------------------------------------------------------------
// Purpose: Loads control for hud and casts panel objects
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::SetControlSettings(void)
{
	if (TFGameRules()->HasMultipleTrains())
	{
		KeyValues *pCond = new KeyValues("MultipleEscort");
		if (GetLocalPlayerTeam() == TF_TEAM_BLUE)
		{

			pCond->AddSubKey(new KeyValues("if_blue_is_top"));
			LoadControlSettings("resource/UI/objectivestatusmultipleescort.res", 0, 0, pCond);
		}
		else
		{
			pCond->AddSubKey(new KeyValues("if_red_is_top"));
			LoadControlSettings("resource/UI/objectivestatusmultipleescort.res", 0, 0, pCond);
		}
		pCond->deleteThis();

		m_pBlueEscortPanel = FindControl<CTFHudEscort>("BlueEscortPanel", true);
		m_pRedEscortPanel = FindControl<CTFHudEscort>("RedEscortPanel", true);
		m_pBlueEscortPanel->SetTeamCart(TF_TEAM_BLUE);
		m_pRedEscortPanel->SetTeamCart(TF_TEAM_RED);
		//m_pBlueEscortPanel = dynamic_cast<CTFHudEscort *>(FindChildByName("BlueEscortPanel"));
		//m_pRedEscortPanel = dynamic_cast<CTFHudEscort *>(FindChildByName("RedEscortPanel"));
		//m_pBlueEscortPanel->SetTeamCart(TF_TEAM_BLUE);
		//m_pRedEscortPanel->SetTeamCart(TF_TEAM_RED);
		b_Visible = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hud Think
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::OnThink()
{
	if (IsVisible() && m_flNextThink < gpGlobals->curtime)
	{
		UpdateStatus();
		m_flNextThink = gpGlobals->curtime + 0.1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates hud
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::UpdateStatus(void)
{
	ConVarRef cl_hud_minmode("cl_hud_minmode", true);

	/*if (TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING)
	{
		for (int i = 0; i < GetChildCount(); i++)
		{
			GetChild(i)->SetVisible(false);
		}
	}*/

	if (cl_hud_minmode.IsValid() && cl_hud_minmode.GetInt() != 0 && !b_InMinHud)
	{
		b_Visible = false;
		b_InMinHud = true;
	}
	else if (cl_hud_minmode.IsValid() && !cl_hud_minmode.GetInt() == 0 && b_InMinHud)
	{
		b_Visible = false;
		b_InMinHud = false;
	}

	if (TeamplayRoundBasedRules()->State_Get() == GR_STATE_PREROUND)
	{
		b_Visible = false;
	}

	if (!b_Visible && TeamplayRoundBasedRules()->State_Get() == GR_STATE_RND_RUNNING)
	{
		SetControlSettings();
	}

}

//-----------------------------------------------------------------------------
// Purpose: Resets think
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05;
}

//-----------------------------------------------------------------------------
// Purpose: Gets fired events and updates hud state accordingly
//-----------------------------------------------------------------------------
void CTFHudMultipleEscort::FireGameEvent(IGameEvent *event)
{
	const char *eventname = event->GetName();
	if (IsVisible())
	{
		m_flNextThink = gpGlobals->curtime;
		if (FStrEq("controlpoints_initialized", eventname))
		{
			// Update Hud position
			b_Visible = false;
			return;
		}
		else if (FStrEq("localplayer_changeteam", eventname))
		{
			// Update Hud position
			if (GetLocalPlayerTeam() > TEAM_SPECTATOR)
				b_Visible = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Progress bar for multiple escort
//-----------------------------------------------------------------------------
CTFHudEscortProgressBar::CTFHudEscortProgressBar(vgui::Panel *parent, const char *name) : vgui::ImagePanel(parent, name)
{
	SetImage("../hud/cart_track_neutral_opaque");
}

//-----------------------------------------------------------------------------
// Purpose: Sets team color
//-----------------------------------------------------------------------------
void CTFHudEscortProgressBar::SetTeam(int team)
{
	switch (team)
	{
	case TF_TEAM_RED:  SetImage("../hud/cart_track_red_opaque");
		break;
	case TF_TEAM_BLUE: SetImage("../hud/cart_track_blue_opaque");
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets progress along the course
//-----------------------------------------------------------------------------
void CTFHudEscortProgressBar::SetProgress(float progress)
{
	SetWide(scheme()->GetProportionalScaledValue(progress));
}

