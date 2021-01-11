#include "cbase.h"
#include "tf_pausemenupanel.h"
#include "controls/tf_advbutton.h"
#include "tf_notificationmanager.h"
#include "vgui_avatarimage.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPauseMenuPanel::CTFPauseMenuPanel(vgui::Panel* parent, const char *panelName) : CTFMenuPanelBase(parent, panelName)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFPauseMenuPanel::~CTFPauseMenuPanel()
{

}

bool CTFPauseMenuPanel::Init()
{
	BaseClass::Init();

	if (steamapicontext->SteamUser())
	{
		m_SteamID = steamapicontext->SteamUser()->GetSteamID();
	}

	m_pNotificationButton = NULL;
	m_pProfileAvatar = NULL;
	m_pVersionLabel = NULL;
	bInMenu = false;
	bInGame = true;
	return true;
};


void CTFPauseMenuPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("resource/UI/main_menu/PauseMenuPanel.res");
	m_pVersionLabel = dynamic_cast<CExLabel *>(FindChildByName("VersionLabel"));
	m_pNotificationButton = dynamic_cast<CTFAdvButton*>(FindChildByName("NotificationButton"));
	m_pProfileAvatar = dynamic_cast<CAvatarImagePanel *>(FindChildByName("AvatarImage"));
}

void CTFPauseMenuPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if (m_pProfileAvatar)
	{
		m_pProfileAvatar->SetPlayer(m_SteamID, k_EAvatarSize64x64);
		m_pProfileAvatar->SetShouldDrawFriendIcon(false);
	}

	char szNickName[64];
	V_strcpy_safe(szNickName, ( steamapicontext->SteamFriends() ) ? steamapicontext->SteamFriends()->GetPersonaName() : "Unknown");
	//SetDialogVariable( "nickname", szNickName ); 
	SetDialogVariable("playername", szNickName); // easier than changing all the language resource files

	if (m_pVersionLabel)
	{
		char verString[64];
		Q_snprintf(verString, sizeof(verString), "Version: %s", GetNotificationManager()->GetVersionString());
		m_pVersionLabel->SetText(verString);
	}
};


void CTFPauseMenuPanel::OnCommand(const char* command)
{
	if (!Q_strcmp(command, "newquit"))
	{
		MAINMENU_ROOT->ShowPanel(QUIT_MENU);
	}
	else if (!Q_strcmp(command, "newoptionsdialog"))
	{
		MAINMENU_ROOT->ShowPanel(OPTIONSDIALOG_MENU);
	}
	else if (!Q_strcmp(command, "newloadout"))
	{
		bFromPause = true;
		MAINMENU_ROOT->ShowPanel(LOADOUT_MENU);
	}
	else if (!Q_strcmp(command, "newstats"))
	{
		MAINMENU_ROOT->ShowPanel(STATSUMMARY_MENU);
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CTFPauseMenuPanel::OnTick()
{
	BaseClass::OnTick();
};

void CTFPauseMenuPanel::OnThink()
{
	BaseClass::OnThink();
};

void CTFPauseMenuPanel::Show()
{
	BaseClass::Show();
	//vgui::GetAnimationController()->RunAnimationCommand(this, "Alpha", 255, 0.0f, 0.5f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
};

void CTFPauseMenuPanel::Hide()
{
	BaseClass::Hide();
	//vgui::GetAnimationController()->RunAnimationCommand(this, "Alpha", 0, 0.0f, 0.1f, vgui::AnimationController::INTERPOLATOR_LINEAR);
};


void CTFPauseMenuPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();
};

void CTFPauseMenuPanel::GameLayout()
{
	BaseClass::GameLayout();
};