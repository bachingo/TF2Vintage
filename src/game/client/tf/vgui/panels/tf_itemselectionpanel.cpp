#include "cbase.h"
#include "tf_itemselectionpanel.h"
#include "tf_mainmenu.h"
#include "controls/tf_advitembutton.h"
#include "controls/tf_advmodelpanel.h"
#include "basemodelpanel.h"
#include <vgui/ILocalize.h>
#include "script_parser.h"
#include "econ_item_view.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SLOTSPACING 10
static CTFWeaponSelectPanel *s_pWeaponButton;

static CTFWeaponSelectPanel *s_pScrollButtonDown;
static CTFWeaponSelectPanel *s_pScrollButtonUp;

bool g_bShowItemMenu = false;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFWeaponSelectPanel::CTFWeaponSelectPanel(vgui::Panel* parent, const char *panelName) : vgui::EditablePanel(parent, panelName)
{
}

void CTFWeaponSelectPanel::OnCommand(const char* command)
{
	GetParent()->OnCommand(command);
}

static class CTFWeaponSelectionScriptParser : public C_ScriptParser
{
public:
	DECLARE_CLASS_GAMEROOT(CTFWeaponSelectionScriptParser, C_ScriptParser);

	void Parse(KeyValues *pKeyValuesData, bool bWildcard, const char *szFileWithoutEXT)
	{
		_WeaponData sTemp;
		Q_strncpy(sTemp.szWorldModel, pKeyValuesData->GetString("playermodel", ""), sizeof(sTemp.szWorldModel));
		Q_strncpy(sTemp.szPrintName, pKeyValuesData->GetString("printname", ""), sizeof(sTemp.szPrintName));
		CUtlString szWeaponType = pKeyValuesData->GetString("WeaponType");

		int iType = -1;
		if (szWeaponType == "primary")
			iType = TF_WPN_TYPE_PRIMARY;
		else if (szWeaponType == "secondary")
			iType = TF_WPN_TYPE_SECONDARY;
		else if (szWeaponType == "melee")
			iType = TF_WPN_TYPE_MELEE;
		else if (szWeaponType == "grenade")
			iType = TF_WPN_TYPE_GRENADE;
		else if (szWeaponType == "building")
			iType = TF_WPN_TYPE_BUILDING;
		else if (szWeaponType == "pda")
			iType = TF_WPN_TYPE_PDA;
		else if (szWeaponType == "item1")
			iType = TF_WPN_TYPE_ITEM1;
		else if (szWeaponType == "item2")
			iType = TF_WPN_TYPE_ITEM2;
		else if (szWeaponType == "head")
			iType = TF_WPN_TYPE_HEAD;
		else if (szWeaponType == "misc" || szWeaponType == "misc2" || szWeaponType == "action" || szWeaponType == "zombie" || szWeaponType == "medal" )
			iType = TF_WPN_TYPE_MISC;

		sTemp.m_iWeaponType = iType >= 0 ? iType : TF_WPN_TYPE_PRIMARY;
		m_WeaponInfoDatabase.Insert(szFileWithoutEXT, sTemp);
	};

	_WeaponData *GetTFWeaponInfo(const char *name)
	{
		return &m_WeaponInfoDatabase[m_WeaponInfoDatabase.Find(name)];
	}

private:
	CUtlDict< _WeaponData, unsigned short > m_WeaponInfoDatabase;
} g_TFWeaponScriptParser;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFItemPanel::CTFItemPanel(vgui::Panel* parent, const char *panelName) : CTFMenuPanelBase(parent, panelName)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFItemPanel::~CTFItemPanel()
{
	m_pWeaponIcons.RemoveAll();
	m_pSlideButtons.RemoveAll();
}

bool CTFItemPanel::Init()
{
	BaseClass::Init();

	m_iCurrentClass = TF_CLASS_SCOUT;
	m_iCurrentSlot = TF_LOADOUT_SLOT_PRIMARY;
	m_pGameModelPanel = new CModelPanel(this, "gamemodelpanel");
	m_pWeaponSetPanel = new CTFWeaponSelectPanel(this, "weaponsetpanel");
	s_pWeaponButton = new CTFWeaponSelectPanel(this, "weaponbutton");
	s_pScrollButtonDown = new CTFWeaponSelectPanel(this, "ScrollButtonDown");
	s_pScrollButtonUp = new CTFWeaponSelectPanel(this, "ScrollButtonUp");
	g_TFWeaponScriptParser.InitParser("scripts/tf_weapon_*.txt", true, false);

	for (int i = 0; i < INVENTORY_VECTOR_NUM_SELECTION; i++)
	{
		m_pWeaponIcons.AddToTail(new CTFAdvItemButton(m_pWeaponSetPanel, "WeaponIcons", "DUK"));
	}
	for (int i = 0; i < INVENTORY_ROWNUM_SELECTION  * 2; i++)
	{
		m_pSlideButtons.AddToTail(new CTFAdvItemButton(m_pWeaponSetPanel, "SlideButton", "DUK"));
	}
	for (int i = 0; i < INVENTORY_ROWNUM_SELECTION ; i++)
	{
		m_RawIDPos.AddToTail(0);
	}

	return true;
}

void CTFItemPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("resource/UI/main_menu/itemselectionpanel.res");
}

void CTFItemPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	for (int iSlot = 0; iSlot < INVENTORY_ROWNUM_SELECTION ; iSlot++)
	{
		for (int iPreset = 0; iPreset < INVENTORY_COLNUM_SELECTION; iPreset++)
		{
			CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[INVENTORY_COLNUM_SELECTION * iSlot + iPreset];
			m_pWeaponButton->SetSize(s_pWeaponButton->GetWide(), s_pWeaponButton->GetTall());
			m_pWeaponButton->SetPos(XRES(0), iPreset * YRES(s_pWeaponButton->GetTall() + 10));
			m_pWeaponButton->SetBorderVisible(true);
			m_pWeaponButton->SetBorderByString("AdvRoundedButtonDisabled", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
			m_pWeaponButton->SetLoadoutSlot(m_iCurrentSlot, iPreset);
		}

		CTFAdvItemButton *m_pSlideButtonUp = m_pSlideButtons[iSlot];
		CTFAdvItemButton *m_pSlideButtonDown = m_pSlideButtons[iSlot + 1];

		m_pSlideButtonUp->SetSize(s_pScrollButtonUp->GetWide(), s_pScrollButtonUp->GetTall());
		m_pSlideButtonUp->SetPos(s_pScrollButtonUp->GetXPos(), iSlot * YRES((s_pScrollButtonUp->GetYPos() + SLOTSPACING)));
		m_pSlideButtonUp->SetText("^");
		m_pSlideButtonUp->SetBorderVisible(true);
		m_pSlideButtonUp->SetBorderByString("AdvRoundedButtonDefault", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
		char szCommand[64];
		Q_snprintf(szCommand, sizeof(szCommand), "SlideU%i", iSlot);
		m_pSlideButtonUp->SetCommandString(szCommand);

		m_pSlideButtonDown->SetSize(s_pScrollButtonDown->GetWide(), s_pScrollButtonDown->GetTall());
		m_pSlideButtonDown->SetPos(s_pScrollButtonDown->GetXPos(), m_pWeaponSetPanel->GetTall() - s_pScrollButtonDown->GetTall());
		m_pSlideButtonDown->SetText("v");
		m_pSlideButtonDown->SetBorderVisible(true);
		m_pSlideButtonDown->SetBorderByString("AdvRoundedButtonDefault", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
		Q_snprintf(szCommand, sizeof(szCommand), "SlideD%i", iSlot);
		m_pSlideButtonDown->SetCommandString(szCommand);
	}
	//m_pWeaponSetPanel->SetPos( 0, m_pWeaponSetPanel->GetYPos() );

	DefaultLayout();
};

void CTFItemPanel::SetCurrentClassAndSlot(int iClass, int iSlot)
{
	if (m_iCurrentClass != iClass)
		m_iCurrentClass = iClass;

	m_iCurrentSlot = g_aClassLoadoutSlots[iClass][iSlot];
	
	ResetRows();
	DefaultLayout();
};


void CTFItemPanel::OnCommand(const char* command)
{
	if ( !Q_strcmp ( command, "back" ) || (!Q_strcmp ( command, "vguicancel" )) )
	{
		g_bShowItemMenu = false;
		Hide ();
	}
	else if ( !Q_strncmp ( command, "loadout", 7 ) )
	{
		GetParent ()->OnCommand ( command );
		return;
	}
	else
	{
		char buffer[64];
		const char* szText;
		char strText[40];


		for ( int i = 0; i < 2; i++ )
		{
			szText = (i == 0 ? "SlideU" : "SlideD");
			Q_strncpy ( strText, command, Q_strlen ( szText ) + 1 );
			if ( !Q_strcmp ( strText, szText ) )
			{
				V_strcpy_safe ( buffer, command + Q_strlen ( szText ) );
				SlideColumn ( atoi ( buffer ), (i == 0 ? -1 : 1) );
				return;
			}
		}
	}
}

void CTFItemPanel::SetSlotAndPreset(int iSlot, int iPreset)
{
	SetCurrentSlot(iSlot);
	SetWeaponPreset(m_iCurrentClass, m_iCurrentSlot, iPreset);
}

void CTFItemPanel::SlideColumn(int iCol, int iDir)
{
	m_RawIDPos[iCol] += iDir;

	for (int iPreset = 0; iPreset < INVENTORY_COLNUM_SELECTION; iPreset++)
	{
		CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[INVENTORY_COLNUM_SELECTION * iCol + iPreset];
		int _x, _y;
		m_pWeaponButton->GetPos(_x, _y);
		int y = (iPreset - m_RawIDPos[iCol]) * YRES(s_pWeaponButton->GetTall() + SLOTSPACING);
		AnimationController::PublicValue_t p_AnimHover(_x, y);
		vgui::GetAnimationController()->RunAnimationCommand(m_pWeaponButton, "Position", p_AnimHover, 0.0f, 0.15f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL);
	}

	DefaultLayout();
}

void CTFItemPanel::ResetRows()
{
	for (int iSlot = 0; iSlot < INVENTORY_ROWNUM_SELECTION; iSlot++)
	{
		m_RawIDPos[iSlot] = 0;
		for (int iPreset = 0; iPreset < INVENTORY_COLNUM_SELECTION; iPreset++)
		{
			CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[INVENTORY_COLNUM_SELECTION * iSlot + iPreset];
			m_pWeaponButton->SetPos(XRES(0), iPreset * YRES(s_pWeaponButton->GetTall() + SLOTSPACING));
			//m_pWeaponButton->SetPos(iSlot * XRES((s_pWeaponButton->GetXPos())), (iPreset - m_RawIDPos[iSlot]) * YRES(s_pWeaponButton->GetYPos()));
		}
	}
}

void CTFItemPanel::Show()
{
	BaseClass::Show();

	
	DefaultLayout();
};

void CTFItemPanel::Hide()
{
	BaseClass::Hide();
};


void CTFItemPanel::OnTick()
{
	BaseClass::OnTick();
};

void CTFItemPanel::OnThink()
{
	BaseClass::OnThink();
};

void CTFItemPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();

	int iClassIndex = m_iCurrentClass;
	SetDialogVariable("classname", g_pVGuiLocalize->Find(g_aPlayerClassNames[iClassIndex]));

	int iRowTall = 0;
	int iPos = m_RawIDPos[0];
	CTFAdvItemButton *m_pSlideButtonUp = m_pSlideButtons[0];
	CTFAdvItemButton *m_pSlideButtonDown = m_pSlideButtons[1];

	//Msg("Loadout Slot is: %i", iSlot);
	
	for (int iRow = 0; iRow < INVENTORY_WEAPONS_COUNT; iRow++)
	{
		CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[iRow];
		CEconItemView *pItem = NULL;

		if (m_iCurrentSlot != -1)
		{
			pItem = GetTFInventory()->GetItem(iClassIndex, m_iCurrentSlot, iRow);
		}

		CEconItemDefinition *pItemData = pItem ? pItem->GetStaticData() : NULL;

		if (pItemData)
		{
			m_pWeaponButton->SetVisible(true);
			m_pWeaponButton->SetItemDefinition(pItemData);
			m_pWeaponButton->SetLoadoutSlot(m_iCurrentSlot, iRow);

			int iWeaponPreset = GetTFInventory()->GetWeaponPreset(iClassIndex, m_iCurrentSlot);
			if (iRow == iWeaponPreset)
			{
				m_pWeaponButton->SetBorderByString("AdvRoundedButtonDefault", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
			}
			else
			{
				m_pWeaponButton->SetBorderByString("AdvRoundedButtonDisabled", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
			}
			m_pWeaponButton->GetButton()->SetSelected((iRow == iWeaponPreset));
			iRowTall += m_pWeaponButton->GetTall() + 10;
		}
		else
		{
			m_pWeaponButton->SetVisible(false);
		}
	}
	if (iRowTall > m_pWeaponSetPanel->GetTall())
	{
		if (iPos == 0)	//Down
		{
			m_pSlideButtonUp->SetVisible(false);
			m_pSlideButtonDown->SetVisible(true);
		}
		else if (m_pWeaponSetPanel->GetTall() >= iRowTall - iPos*(s_pWeaponButton->GetTall() + 10))	//Up
		{
			m_pSlideButtonUp->SetVisible(true);
			m_pSlideButtonDown->SetVisible(false);
		}
		else  //middle
		{
			m_pSlideButtonUp->SetVisible(true);
			m_pSlideButtonDown->SetVisible(true);
		}
	}
	else
	{
		m_pSlideButtonUp->SetVisible(false);
		m_pSlideButtonDown->SetVisible(false);
	}
};

void CTFItemPanel::GameLayout()
{
	BaseClass::GameLayout();

};

void CTFItemPanel::SetWeaponPreset(int iClass, int iSlot, int iPreset)
{
	GetTFInventory()->SetWeaponPreset(iClass, iSlot, iPreset);
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pPlayer)
	{
		char szCmd[64];
		Q_snprintf(szCmd, sizeof(szCmd), "weaponpresetclass %d %d %d", iClass, iSlot, iPreset);
		engine->ExecuteClientCmd(szCmd);
	}

	DefaultLayout();
}
