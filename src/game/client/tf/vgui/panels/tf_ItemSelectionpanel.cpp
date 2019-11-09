#include "cbase.h"
#include "tf_ItemSelectionpanel.h"
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

#define PANEL_WIDE 110
#define PANEL_TALL 70
static CTFWeaponSelectPanel *s_pWeaponButton;

static CTFWeaponSelectPanel *s_pScrollButton;
static CTFWeaponSelectPanel *s_pScrollButton2;

static int g_aClassLoadoutSlots[TF_CLASS_COUNT_ALL][INVENTORY_ROWNUM] =
{
	{
		-1, -1, -1,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_PDA2,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_PRIMARY,
		TF_LOADOUT_SLOT_SECONDARY,
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
	{
		TF_LOADOUT_SLOT_MELEE,
		TF_LOADOUT_SLOT_HAT,
		TF_LOADOUT_SLOT_MISC1,
		TF_LOADOUT_SLOT_MISC2,
	},
};

struct _WeaponData
{
	char szWorldModel[64];
	char iconActive[64];
	char iconInactive[64];
	char szPrintName[64];
	int m_iWeaponType;
};

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

class CTFWeaponSelectionScriptParser : public C_ScriptParser
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

		for (KeyValues *pData = pKeyValuesData->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey())
		{
			if (!Q_stricmp(pData->GetName(), "TextureData"))
			{
				for (KeyValues *pTextureData = pData->GetFirstSubKey(); pTextureData != NULL; pTextureData = pTextureData->GetNextKey())
				{
					if (!Q_stricmp(pTextureData->GetName(), "weapon"))
					{
						Q_strncpy(sTemp.iconInactive, pTextureData->GetString("file", ""), sizeof(sTemp.iconInactive));
					}
					if (!Q_stricmp(pTextureData->GetName(), "weapon_s"))
					{
						Q_strncpy(sTemp.iconActive, pTextureData->GetString("file", ""), sizeof(sTemp.iconActive));
					}
				}
			}
		}
		m_WeaponInfoDatabase.Insert(szFileWithoutEXT, sTemp);
	};

	_WeaponData *GetTFWeaponInfo(const char *name)
	{
		return &m_WeaponInfoDatabase[m_WeaponInfoDatabase.Find(name)];
	}

private:
	CUtlDict< _WeaponData, unsigned short > m_WeaponInfoDatabase;
};
CTFWeaponSelectionScriptParser g_TFWeaponScriptParser;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFItemPanel::CTFItemPanel(vgui::Panel* parent, const char *panelName) : CTFDialogPanelBase(parent, panelName)
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
	s_pScrollButton = new CTFWeaponSelectPanel(this, "Scrollbutton");
	s_pScrollButton2 = new CTFWeaponSelectPanel(this, "Scrollbutton2");
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


	/*for (int iClassIndex = 0; iClassIndex <= TF_LAST_NORMAL_CLASS; iClassIndex++)
	{
		if (pszClassModels[iClassIndex][0] != '\0')
			modelinfo->FindOrLoadModel(pszClassModels[iClassIndex]);

		for (int iSlot = 0; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++)
		{
			for (int iPreset = 0; iPreset < INVENTORY_COLNUM_SELECTION; iPreset++)
			{
				CEconItemView *pItem = GetTFInventory()->GetItem(iClassIndex, iSlot, iPreset);

				if (pItem)
				{
					const char *pszWeaponModel = GetWeaponModel(pItem->GetStaticData(), iClassIndex);

					if (pszWeaponModel[0] != '\0')
					{
						modelinfo->FindOrLoadModel(pszWeaponModel);
					}

					const char *pszExtraWearableModel = GetExtraWearableModel(pItem->GetStaticData());

					if (pszExtraWearableModel[0] != '\0')
					{
						modelinfo->FindOrLoadModel(pszExtraWearableModel);
					}
				}
			}
		}
	}*/

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
			m_pWeaponButton->SetSize(XRES(s_pWeaponButton->GetWide()), YRES(s_pWeaponButton->GetTall()));
			m_pWeaponButton->SetPos((iSlot * YRES((s_pWeaponButton->GetYPos()))), iPreset * XRES((s_pWeaponButton->GetXPos())) );
			m_pWeaponButton->SetBorderVisible(true);
			m_pWeaponButton->SetBorderByString("AdvRoundedButtonDefault", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
			m_pWeaponButton->SetLoadoutSlot(m_iCurrentSlot, iPreset);
		}

		CTFAdvItemButton *m_pSlideButtonL = m_pSlideButtons[iSlot * 2];
		CTFAdvItemButton *m_pSlideButtonR = m_pSlideButtons[(iSlot * 2) + 1];

		m_pSlideButtonL->SetSize(XRES(s_pScrollButton->GetWide()), YRES(s_pScrollButton->GetTall()));
		m_pSlideButtonL->SetPos(s_pScrollButton->GetXPos(), iSlot * YRES((s_pScrollButton->GetYPos() + 5)));
		m_pSlideButtonL->SetText("^");
		m_pSlideButtonL->SetBorderVisible(true);
		m_pSlideButtonL->SetBorderByString("AdvLeftButtonDefault", "AdvLeftButtonArmed", "AdvLeftButtonDepressed");
		char szCommand[64];
		Q_snprintf(szCommand, sizeof(szCommand), "SlideL%i", iSlot);
		m_pSlideButtonL->SetCommandString(szCommand);

		m_pSlideButtonR->SetSize(XRES(s_pScrollButton2->GetWide()), YRES(s_pScrollButton2->GetTall()));
		m_pSlideButtonR->SetPos(s_pScrollButton2->GetXPos(), m_pWeaponSetPanel->GetTall() - YRES(s_pScrollButton2->GetYPos()));
		m_pSlideButtonR->SetText("v");
		m_pSlideButtonR->SetBorderVisible(true);
		m_pSlideButtonR->SetBorderByString("AdvRightButtonDefault", "AdvRightButtonArmed", "AdvRightButtonDepressed");
		Q_snprintf(szCommand, sizeof(szCommand), "SlideR%i", iSlot);
		m_pSlideButtonR->SetCommandString(szCommand);
	}
};


void CTFItemPanel::SetCurrentClass(int iClass)
{
	if (m_iCurrentClass == iClass)
		return;


	m_iCurrentClass = iClass;
	m_iCurrentSlot = g_aClassLoadoutSlots[iClass][0];
	ResetRows();
	DefaultLayout();
};

void CTFItemPanel::SetCurrentClassAndSlot(int iClass, int iSlot)
{
	if (m_iCurrentClass != iClass)
		m_iCurrentClass = iClass;

	m_iCurrentSlot = g_aClassLoadoutSlots[iClass][iSlot];
	if (m_iCurrentClass == 8)
	{
		if (m_iCurrentSlot == 2)
		{
			m_iCurrentSlot = 1;
		}
		else if (m_iCurrentSlot == 4)
		{
			m_iCurrentSlot = 2;
		}
		else if (m_iCurrentSlot == 8)
		{
			m_iCurrentSlot = 4;
		}
	}
	else
	{
		m_iCurrentSlot = g_aClassLoadoutSlots[iClass][iSlot];
	}
	
	
	ResetRows();
	DefaultLayout();
};


void CTFItemPanel::OnCommand(const char* command)
{
	if (!Q_strcmp(command, "back") || (!Q_strcmp(command, "vguicancel")))
	{
		Hide();
	}
	else
	{
		if (!Q_strncmp(command, "loadout", 7))
		{
			GetParent()->OnCommand(command);
		}
		
		char buffer[64];
		const char* szText;
		char strText[40];

		/*if (!Q_strncmp(command, "loadout", 7))
		{
			const char *sChar = strchr(command, ' ');
			if (sChar)
			{
				int iSlot = atoi(sChar + 1);
				sChar = strchr(sChar + 1, ' ');
				if (sChar)
				{
					int iWeapon = atoi(sChar + 1);
					SetSlotAndPreset(iSlot, iWeapon);
				}
			}
			return;
		}*/

		for (int i = 0; i < 2; i++)
		{
			szText = (i == 0 ? "SlideL" : "SlideR");
			Q_strncpy(strText, command, Q_strlen(szText) + 1);
			if (!Q_strcmp(strText, szText))
			{
				Q_snprintf(buffer, sizeof(buffer), command + Q_strlen(szText));
				SideRow(atoi(buffer), (i == 0 ? -1 : 1));
				return;
			}
		}

		//BaseClass::OnCommand(command);
	}
}

void CTFItemPanel::SetSlotAndPreset(int iSlot, int iPreset)
{
	SetCurrentSlot(iSlot);
	SetWeaponPreset(m_iCurrentClass, m_iCurrentSlot, iPreset);
}

void CTFItemPanel::SideRow(int iRow, int iDir)
{
	m_RawIDPos[iRow] += iDir;

	for (int iPreset = 0; iPreset < INVENTORY_COLNUM_SELECTION; iPreset++)
	{
		CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[INVENTORY_COLNUM_SELECTION * iRow + iPreset];
		int _x, _y;
		m_pWeaponButton->GetPos(_x, _y);
		int y = (iPreset - m_RawIDPos[iRow]) * YRES((s_pWeaponButton->GetXPos()));
		AnimationController::PublicValue_t p_AnimHover(_x, y);
		vgui::GetAnimationController()->RunAnimationCommand(m_pWeaponButton, "Position", p_AnimHover, 0.0f, 0.1f, vgui::AnimationController::INTERPOLATOR_LINEAR, NULL);
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
			m_pWeaponButton->SetPos(iSlot * YRES((s_pWeaponButton->GetYPos())), iPreset * YRES((s_pWeaponButton->GetXPos())));
		}
	}
}

void CTFItemPanel::Show()
{
	BaseClass::Show();
	MAINMENU_ROOT->ShowPanel(SHADEBACKGROUND_MENU);

	
	DefaultLayout();
};

void CTFItemPanel::Hide()
{
	BaseClass::Hide();
	MAINMENU_ROOT->HidePanel(SHADEBACKGROUND_MENU);
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

	/*
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pLocalPlayer && pLocalPlayer->GetTeamNumber() >= TF_TEAM_RED)
	{
	m_iCurrentSkin = pLocalPlayer->GetTeamNumber() - 2;
	}
	else
	{
	m_iCurrentSkin = 0;
	}
	*/

	int iClassIndex = m_iCurrentClass;
	SetDialogVariable("classname", g_pVGuiLocalize->Find(g_aPlayerClassNames[iClassIndex]));
	
	for (int iRow = 0; iRow < INVENTORY_ROWNUM_SELECTION; iRow++)
	{
		int iColumnCount = 0;
		int iPresetID = 0;
		int iPos = m_RawIDPos[iRow];
		CTFAdvItemButton *m_pSlideButtonL = m_pSlideButtons[iRow * 2];
		CTFAdvItemButton *m_pSlideButtonR = m_pSlideButtons[(iRow * 2) + 1];

		int iSlot = m_iCurrentSlot;

		//Msg("Loadout Slot is: %i", iSlot);
		
		

		for (int iColumn = 0; iColumn < INVENTORY_COLNUM_SELECTION; iColumn++)
		{
			CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[INVENTORY_COLNUM_SELECTION * iRow + iColumn];

			//m_pWeaponButton->SetPos((iSlot * YRES((m_weaponbutton->GetYPos() + 5))), iColumn * XRES((m_weaponbutton->GetXPos())));
			CEconItemView *pItem = NULL;


			if (iSlot != -1)
			{
				pItem = GetTFInventory()->GetItem(iClassIndex, iSlot, iColumn);
			}

			CEconItemDefinition *pItemData = pItem ? pItem->GetStaticData() : NULL;

			if (pItemData)
			{
				m_pWeaponButton->SetVisible(true);
				m_pWeaponButton->SetItemDefinition(pItemData);
				m_pWeaponButton->SetLoadoutSlot(iSlot, iColumn);

				int iWeaponPreset = GetTFInventory()->GetWeaponPreset(iClassIndex, iSlot);
				if (iColumn == iWeaponPreset)
				{
					m_pWeaponButton->SetBorderByString("AdvRoundedButtonDefault", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
				}
				else
				{
					m_pWeaponButton->SetBorderByString("AdvRoundedButtonDisabled", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed");
				}
				m_pWeaponButton->GetButton()->SetSelected((iColumn == iWeaponPreset));

				if (iColumn == iWeaponPreset)
					iPresetID = iColumn;
				iColumnCount++;
			}
			else
			{
				m_pWeaponButton->SetVisible(false);
			}
		}
		if (iColumnCount > 2)
		{
			if (iPos == 0)	//left
			{
				m_pSlideButtonL->SetVisible(false);
				m_pSlideButtonR->SetVisible(true);
			}
			else if (iPos == iColumnCount - 2)	//right
			{
				m_pSlideButtonL->SetVisible(true);
				m_pSlideButtonR->SetVisible(false);
			}
			else  //middle
			{
				m_pSlideButtonL->SetVisible(true);
				m_pSlideButtonR->SetVisible(true);
			}
		}
		else
		{
			m_pSlideButtonL->SetVisible(false);
			m_pSlideButtonR->SetVisible(false);
		}
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
