//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudEscort::CTFHudEscort(Panel *parent, const char *name) : EditablePanel(parent, name)
{
	m_flNextThink = 0.0f;

	vgui::ivgui()->AddTickSignal(GetVPanel(), 500);

	ListenForGameEvent("player_death");
	ListenForGameEvent("teamplay_setup_finished");
	ListenForGameEvent("teamplay_update_timer");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudEscort::IsVisible(void)
{
	if (IsTakingAFreezecamScreenshot())
		return false;

	if (!TFGameRules() || !TFGameRules()->IsInEscortMode())
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// load control settings...
	LoadControlSettings("resource/UI/objectivestatusescort.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::SetPlayingToLabelVisible(bool bVisible)
{
	bVisible = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::OnThink(void)
{
	if (IsVisible() && m_flNextThink < gpGlobals->curtime)
	{
		UpdateStatus();
		m_flNextThink = gpGlobals->curtime + 0.5;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::UpdateStatus(void)
{
	ObjectiveResource()->GetNumNodeHillData(TF_TEAM_BLUE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudEscort::FireGameEvent(IGameEvent *event)
{
	if (IsVisible())
	{
		m_flNextThink = gpGlobals->curtime;
	}
}
