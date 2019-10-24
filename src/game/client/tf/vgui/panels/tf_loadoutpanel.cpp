#include "cbase.h"
#include "tf_loadoutpanel.h"
#include "tf_mainmenu.h"
#include "controls/tf_advitembutton.h"
#include "controls/tf_advmodelpanel.h"
#include "basemodelpanel.h"
#include <vgui/ILocalize.h>
#include "script_parser.h"
#include "econ_item_view.h"
#include "tier0/icommandline.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PANEL_WIDE 110
#define PANEL_TALL 70
static CTFAdvItemButton *s_pWeaponButton1;
static CTFAdvItemButton *s_pWeaponButton2;
static CTFAdvItemButton *s_pWeaponButton3;
static CTFAdvItemButton *s_pWeaponButton4;
static CTFAdvItemButton *s_pWeaponButton5;
static CTFAdvItemButton *s_pWeaponButton6;
//static CTFWeaponSetPanel *s_pWeaponSpace;

static char* pszClassModels[TF_CLASS_COUNT_ALL] =
{
	"",
	"models/player/scout.mdl",
	"models/player/sniper.mdl",
	"models/player/soldier.mdl",
	"models/player/demo.mdl",
	"models/player/medic.mdl",
	"models/player/heavy.mdl",
	"models/player/pyro.mdl",
	"models/player/spy.mdl",
	"models/player/engineer.mdl",
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFWeaponSetPanel::CTFWeaponSetPanel( vgui::Panel* parent, const char *panelName ) : vgui::EditablePanel( parent, panelName )
{
}

void CTFWeaponSetPanel::OnCommand( const char* command )
{
	GetParent()->OnCommand( command );
}

class CTFWeaponScriptParser : public C_ScriptParser
{
public:
	DECLARE_CLASS_GAMEROOT( CTFWeaponScriptParser, C_ScriptParser );

	void Parse( KeyValues *pKeyValuesData, bool bWildcard, const char *szFileWithoutEXT )
	{
		_WeaponData sTemp;
		Q_strncpy( sTemp.szWorldModel, pKeyValuesData->GetString( "playermodel", "" ), sizeof( sTemp.szWorldModel ) );
		Q_strncpy( sTemp.szPrintName, pKeyValuesData->GetString( "printname", "" ), sizeof( sTemp.szPrintName ) );
		CUtlString szWeaponType = pKeyValuesData->GetString( "WeaponType" );

		int iType = -1;
		if ( szWeaponType == "primary" )
			iType = TF_WPN_TYPE_PRIMARY;
		else if ( szWeaponType == "secondary" )
			iType = TF_WPN_TYPE_SECONDARY;
		else if ( szWeaponType == "melee" )
			iType = TF_WPN_TYPE_MELEE;
		else if ( szWeaponType == "grenade" )
			iType = TF_WPN_TYPE_GRENADE;
		else if ( szWeaponType == "building" )
			iType = TF_WPN_TYPE_BUILDING;
		else if ( szWeaponType == "pda" )
			iType = TF_WPN_TYPE_PDA;
		else if ( szWeaponType == "item1" )
			iType = TF_WPN_TYPE_ITEM1;
		else if ( szWeaponType == "item2" )
			iType = TF_WPN_TYPE_ITEM2;
		else if ( szWeaponType == "head" )
			iType = TF_WPN_TYPE_HEAD;
		else if ( szWeaponType == "misc" || szWeaponType == "misc2" || szWeaponType == "action" || szWeaponType == "zombie" )
			iType = TF_WPN_TYPE_MISC;

		sTemp.m_iWeaponType = iType >= 0 ? iType : TF_WPN_TYPE_PRIMARY;
		m_WeaponInfoDatabase.Insert( szFileWithoutEXT, sTemp );
	};

	_WeaponData *GetTFWeaponInfo( const char *name )
	{
		return &m_WeaponInfoDatabase[m_WeaponInfoDatabase.Find( name )];
	}

private:
	CUtlDict< _WeaponData, unsigned short > m_WeaponInfoDatabase;
};
CTFWeaponScriptParser g_TFWeaponScriptParser;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFLoadoutPanel::CTFLoadoutPanel(vgui::Panel* parent, const char *panelName) : CTFMenuPanelBase(parent, panelName)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFLoadoutPanel::~CTFLoadoutPanel()
{
	//m_pWeaponIcons.RemoveAll();
}

bool CTFLoadoutPanel::Init()
{
	BaseClass::Init();

	m_iCurrentClass = TF_CLASS_SCOUT;
	m_iCurrentSlot = TF_LOADOUT_SLOT_PRIMARY;
	m_pClassModelPanel = new CTFAdvModelPanel( this, "classmodelpanel" );
	m_pGameModelPanel = new CModelPanel( this, "gamemodelpanel" );
	//m_pWeaponSetPanel = new CTFWeaponSetPanel( this, "weaponsetpanel" );
	s_pWeaponButton1 = new CTFAdvItemButton(this, "weaponbutton1", "DUK");
	s_pWeaponButton2 = new CTFAdvItemButton(this, "weaponbutton2", "DUK");
	s_pWeaponButton3 = new CTFAdvItemButton(this, "weaponbutton3", "DUK");
	s_pWeaponButton4 = new CTFAdvItemButton(this, "weaponbutton4", "DUK");
	s_pWeaponButton5 = new CTFAdvItemButton(this, "weaponbutton5", "DUK");
	s_pWeaponButton6 = new CTFAdvItemButton(this, "weaponbutton6", "DUK");
	m_pItemPanel = NULL;
	g_TFWeaponScriptParser.InitParser( "scripts/tf_weapon_*.txt", true, false );

	/*for ( int i = 0; i < INVENTORY_VECTOR_NUM; i++ )
	{
		m_pWeaponIcons.AddToTail( new CTFAdvItemButton( m_pWeaponSetPanel, "WeaponIcons", "DUK" ) );
	}
	for ( int i = 0; i < INVENTORY_ROWNUM; i++ )
	{
		m_RawIDPos.AddToTail( 0 );
	}*/


	for ( int iClassIndex = 0; iClassIndex < TF_CLASS_COUNT_ALL; iClassIndex++ )
	{
		if ( pszClassModels[iClassIndex][0] != '\0' )
			modelinfo->FindOrLoadModel( pszClassModels[iClassIndex] );

		for ( int iSlot = 0; iSlot < TF_LOADOUT_SLOT_BUFFER; iSlot++ )
		{
			for ( int iPreset = 0; iPreset < INVENTORY_COLNUM; iPreset++ )
			{
				int miscslot = 1;
				if ( iSlot == TF_LOADOUT_SLOT_MISC2 )
				{
					iSlot = TF_LOADOUT_SLOT_MISC1;
					miscslot = 2;
				}
				CEconItemView *pItem = GetTFInventory()->GetItem( iClassIndex, iSlot, iPreset );
				if ( miscslot == 2 )
				{
					iSlot = TF_LOADOUT_SLOT_MISC2;
					miscslot = 1;
				}

				if ( pItem )
				{
					const char *pszWeaponModel = GetWeaponModel( pItem->GetStaticData(), iClassIndex );

					if ( pszWeaponModel[0] != '\0' )
					{
						modelinfo->FindOrLoadModel( pszWeaponModel );
					}

					const char *pszExtraWearableModel = GetExtraWearableModel( pItem->GetStaticData() );

					if ( pszExtraWearableModel[0] != '\0' )
					{
						modelinfo->FindOrLoadModel( pszExtraWearableModel );
					}
				}
			}
		}
	}

	return true;
}

void CTFLoadoutPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/LoadoutPanel.res" );
	m_pItemPanel = dynamic_cast<CTFItemPanel*>(GetMenuPanel(ITEMSELCTION_MENU));
	//m_pVersionLabel = dynamic_cast<CExLabel *>(FindChildByName("VersionLabel"));
}

void CTFLoadoutPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	s_pWeaponButton1->SetLoadoutSlot(0, 0);
	s_pWeaponButton2->SetLoadoutSlot(1, 0);
	s_pWeaponButton3->SetLoadoutSlot(2, 0);
	s_pWeaponButton4->SetLoadoutSlot(3, 0);
	s_pWeaponButton5->SetLoadoutSlot(4, 0);
	s_pWeaponButton6->SetLoadoutSlot(5, 0);


	/*for ( int iSlot = 0; iSlot < INVENTORY_ROWNUM; iSlot++ )
	{
		for ( int iPreset = 0; iPreset < INVENTORY_COLNUM; iPreset++ )
		{
			CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[INVENTORY_COLNUM * iSlot + iPreset];
			//Original Loadout Button Size/Pos
			//m_pWeaponButton->SetSize( XRES( PANEL_WIDE ), YRES( PANEL_TALL ) );
			//m_pWeaponButton->SetPos( iPreset * XRES( ( PANEL_WIDE + 10 ) ), iSlot * YRES( ( PANEL_TALL + 5 ) ) );
			m_pWeaponButton->SetSize(XRES(s_pWeaponButton1->GetWide()), YRES(s_pWeaponButton1->GetTall()));
			if (iSlot >= 4)
			{
				m_pWeaponButton->SetPos(XRES((s_pWeaponButton2->GetXPos() + 10)), (iSlot - 5) * YRES((s_pWeaponButton2->GetYPos())));
			}
			else
			{
				m_pWeaponButton->SetPos(iPreset * XRES((s_pWeaponSpace->GetWide() + 10)), ((iSlot - 0.01) * YRES((s_pWeaponSpace->GetTall() - 12))));
			}
			m_pWeaponButton->SetBorderVisible( true );
			m_pWeaponButton->SetBorderByString( "AdvRoundedButtonDefault", "AdvRoundedButtonArmed", "AdvRoundedButtonDepressed" );
			m_pWeaponButton->SetLoadoutSlot( iSlot, iPreset );
		}
	}*/
};


void CTFLoadoutPanel::SetCurrentClass(int iClass)
{
	if (m_iCurrentClass == iClass)
		return;

	if (s_bItemMenu)
	{
		m_pItemPanel->SetEnabled(false);
		s_bItemMenu = false;
		MAINMENU_ROOT->HidePanel(ITEMSELCTION_MENU);
	}

	m_iCurrentClass = iClass;
	m_iCurrentSlot = g_aClassLoadoutSlots[iClass][0];
	//ResetRows();
	DefaultLayout();

};


void CTFLoadoutPanel::OnCommand( const char* command )
{
	if ( !Q_strcmp( command, "back" ) || ( !Q_strcmp( command, "vguicancel" ) ) )
	{
		Hide();
	}
	else if ( !Q_strcmp( command, "select_scout" ) )
	{
		SetCurrentClass( TF_CLASS_SCOUT );
	}
	else if ( !Q_strcmp( command, "select_soldier" ) )
	{
		SetCurrentClass( TF_CLASS_SOLDIER );
	}
	else if ( !Q_strcmp( command, "select_pyro" ) )
	{
		SetCurrentClass( TF_CLASS_PYRO );
	}
	else if ( !Q_strcmp( command, "select_demoman" ) )
	{
		SetCurrentClass( TF_CLASS_DEMOMAN );
	}
	else if ( !Q_strcmp( command, "select_heavyweapons" ) )
	{
		SetCurrentClass( TF_CLASS_HEAVYWEAPONS );
	}
	else if ( !Q_strcmp( command, "select_engineer" ) )
	{
		SetCurrentClass( TF_CLASS_ENGINEER );
	}
	else if ( !Q_strcmp( command, "select_medic" ) )
	{
		SetCurrentClass( TF_CLASS_MEDIC );
	}
	else if ( !Q_strcmp( command, "select_sniper" ) )
	{
		SetCurrentClass( TF_CLASS_SNIPER );
	}
	else if ( !Q_strcmp( command, "select_spy" ) )
	{
		SetCurrentClass( TF_CLASS_SPY );
	}
	else
	{
		/*char buffer[64];
		const char* szText;
		char strText[40];*/

		if (!s_bItemMenu)
		{
			CTFItemPanel *ItemPanel = dynamic_cast<CTFItemPanel*>(GetMenuPanel(ITEMSELCTION_MENU));
			ItemPanel->SetEnabled(true);
			const char *sChar = strchr(command, ' ');
			int iSlot = atoi(sChar + 1);
			ItemPanel->SetCurrentClassAndSlot(m_iCurrentClass, iSlot);
			s_bItemMenu = true;

			//ItemPanel->SetCurrentClass(m_iCurrentClass);
			MAINMENU_ROOT->ShowPanel(ITEMSELCTION_MENU);


		}
		else
		{
			CTFItemPanel *ItemPanel = dynamic_cast<CTFItemPanel*>(GetMenuPanel(ITEMSELCTION_MENU));

			ItemPanel->SetEnabled(false);
			s_bItemMenu = false;
			MAINMENU_ROOT->HidePanel(ITEMSELCTION_MENU);
			if (!Q_strncmp(command, "loadout", 7))
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

			}
		}

		BaseClass::OnCommand(command);
	}
}

void CTFLoadoutPanel::SetSlotAndPreset( int iSlot, int iPreset )
{
	SetCurrentSlot( iSlot );
	SetWeaponPreset( m_iCurrentClass, m_iCurrentSlot, iPreset );
}

/*void CTFLoadoutPanel::ResetRows()
{
	for ( int iSlot = 0; iSlot < INVENTORY_ROWNUM; iSlot++ )
	{
		m_RawIDPos[iSlot] = 0;
		for ( int iPreset = 0; iPreset < INVENTORY_COLNUM; iPreset++ )
		{
			CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[INVENTORY_COLNUM * iSlot + iPreset];
			//Original
			//m_pWeaponButton->SetPos( iPreset * XRES( ( PANEL_WIDE + 10 ) ), iSlot * YRES( ( PANEL_TALL + 5 ) ) );
			if (iSlot >= 4)
			{
				m_pWeaponButton->SetPos(XRES((s_pWeaponButton2->GetXPos() + 5)), (iSlot - 5) * YRES((s_pWeaponButton2->GetYPos())));
			}
			else
			{
				m_pWeaponButton->SetPos(iPreset * XRES((s_pWeaponSpace->GetWide() + 10)), (iSlot - 0.01) * YRES((s_pWeaponSpace->GetTall() - 12)));
			}
		}
	}
}*/

int CTFLoadoutPanel::GetAnimSlot( CEconItemDefinition *pItemDef, int iClass )
{
	if ( !pItemDef )
		return -1;

	int iSlot = pItemDef->anim_slot;
	if ( iSlot == -1 )
	{
		// Fall back to script file data.
		const char *pszClassname = TranslateWeaponEntForClass( pItemDef->item_class, iClass );
		_WeaponData *pWeaponInfo = g_TFWeaponScriptParser.GetTFWeaponInfo( pszClassname );
		Assert( pWeaponInfo );

		iSlot = pWeaponInfo->m_iWeaponType;
	}

	return iSlot;
}

const char *CTFLoadoutPanel::GetWeaponModel( CEconItemDefinition *pItemDef, int iClass )
{
	if ( !pItemDef )
		return "";

	if ( pItemDef->act_as_wearable )
	{
		if ( pItemDef->model_player_per_class[iClass][0] != '\0' )
			return pItemDef->model_player_per_class[iClass];

		return pItemDef->model_player;
	}

	const char *pszModel = pItemDef->model_world;

	if ( pszModel[0] == '\0' && pItemDef->attach_to_hands == 1 )
	{
		pszModel = pItemDef->model_player;
	}

	return pszModel;
}

const char *CTFLoadoutPanel::GetExtraWearableModel( CEconItemDefinition *pItemDef )
{
	if ( pItemDef && pItemDef->extra_wearable )
	{
		return pItemDef->extra_wearable;
	}

	return "";
}

void CTFLoadoutPanel::UpdateModelWeapons( void )
{
	m_pClassModelPanel->ClearMergeMDLs();
	int iAnimationIndex = -1;

	// Get active weapon info.
	int iPreset = GetTFInventory()->GetWeaponPreset( m_iCurrentClass, m_iCurrentSlot );
	CEconItemView *pActiveItem = GetTFInventory()->GetItem( m_iCurrentClass, m_iCurrentSlot, iPreset );
	Assert( pActiveItem );

	if ( pActiveItem )
	{
		iAnimationIndex = GetAnimSlot( pActiveItem->GetStaticData(), m_iCurrentClass );

		// Can't be an active weapon without animation.
		if ( iAnimationIndex < 0 )
			pActiveItem = NULL;
	}

	for ( int iRow = 0; iRow < INVENTORY_ROWNUM; iRow++ )
	{
		int iSlot = g_aClassLoadoutSlots[m_iCurrentClass][iRow];
		if ( iSlot == -1 )
			continue;

		int iWeapon = GetTFInventory()->GetWeaponPreset( m_iCurrentClass, iSlot );
		CEconItemView *pItem = GetTFInventory()->GetItem( m_iCurrentClass, iSlot, iWeapon );
		CEconItemDefinition *pItemDef = pItem ? pItem->GetStaticData() : NULL;

		if ( !pItemDef )
			continue;

		if ( !pActiveItem )
		{
			// No active weapon, try this one.
			int iAnimSlot = GetAnimSlot( pItemDef, m_iCurrentClass );
			if ( iAnimSlot >= 0 )
			{
				pActiveItem = pItem;
				iAnimationIndex = iAnimSlot;
			}
		}

		// If this is the active weapon or it's a wearable, add its model.
		if ( pItem == pActiveItem || pItemDef->act_as_wearable )
		{
			const char *pszModel = GetWeaponModel( pItemDef, m_iCurrentClass );
			if ( pszModel[0] != '\0' )
			{
				m_pClassModelPanel->SetMergeMDL(pszModel, NULL, m_iCurrentSkin);
			}
		}

		const char *pszExtraWearableModel = GetExtraWearableModel( pItem->GetStaticData() );
		if ( pszExtraWearableModel[0] != '\0' )
		{
			m_pClassModelPanel->SetMergeMDL( pszExtraWearableModel, NULL, m_iCurrentSkin );
		}
	}

	// Set the animation.
	m_pClassModelPanel->SetAnimationIndex( iAnimationIndex >= 0 ? iAnimationIndex : TF_WPN_TYPE_PRIMARY );

	m_pClassModelPanel->Update();
}

void CTFLoadoutPanel::Show()
{
	BaseClass::Show();
	vgui::GetAnimationController()->RunAnimationCommand(this, "Alpha", 255, 0.0f, 0.15f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		int iClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();
		if ( iClass >= TF_CLASS_SCOUT )
			SetCurrentClass( pPlayer->m_Shared.GetDesiredPlayerClassIndex() );
	}
	DefaultLayout();
};

void CTFLoadoutPanel::Hide()
{
	BaseClass::Hide();
	GetMenuPanel(CURRENT_MENU)->Show();
	vgui::GetAnimationController()->RunAnimationCommand(this, "Alpha", 0, 0.0f, 0.1f, vgui::AnimationController::INTERPOLATOR_LINEAR);
	if (InGame() && !bFromPause)
		engine->ExecuteClientCmd("escape");
	//MAINMENU_ROOT->ShowPanel(PAUSE_MENU);
};


void CTFLoadoutPanel::OnTick()
{
	BaseClass::OnTick();
};

void CTFLoadoutPanel::OnThink()
{
	BaseClass::OnThink();
};

void CTFLoadoutPanel::SetModelClass( int iClass )
{
	m_pClassModelPanel->SetModelName( strdup( pszClassModels[iClass] ), m_iCurrentSkin );
}

void CTFLoadoutPanel::UpdateModelPanels()
{
	int iClassIndex = m_iCurrentClass;

	m_pClassModelPanel->SetVisible( true );
	m_pGameModelPanel->SetVisible( false );
	//m_pWeaponSetPanel->SetVisible( true );

	SetModelClass( iClassIndex );
	UpdateModelWeapons();
}

void CTFLoadoutPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pPlayer && pPlayer->m_iOldTeam == TF_TEAM_BLUE)
	{
		m_iCurrentSkin = TF_TEAM_BLUE - 2;
	}
	else
	{
		m_iCurrentSkin = 0;
	}

	/*C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pLocalPlayer && pLocalPlayer->GetTeamNumber() >= TF_TEAM_RED)
	{
		m_iCurrentSkin = pLocalPlayer->GetTeamNumber() - 2;
	}
	else
	{
		m_iCurrentSkin = 0;
	}*/

	UpdateModelPanels();

	int iClassIndex = m_iCurrentClass;
	SetDialogVariable( "classname", g_pVGuiLocalize->Find( g_aPlayerClassNames[iClassIndex] ) );

	for (int iRow = 0; iRow < INVENTORY_ROWNUM; iRow++)
	{
		int iSlot = g_aClassLoadoutSlots[iClassIndex][iRow];

		for (int iColumn = 0; iColumn < INVENTORY_COLNUM; iColumn++)
		{
			CEconItemView *pItem = NULL;

			int iWeaponPreset = GetTFInventory()->GetWeaponPreset(iClassIndex, iSlot);
			if (iSlot != -1)
			{
				for (int i = 0; i < INVENTORY_WEAPONS_COUNT; i++)
				{
					pItem = GetTFInventory()->GetItem(iClassIndex, iSlot, i);
					if (i == iWeaponPreset)
						break;
				}
			}

			CEconItemDefinition *pItemData = pItem ? pItem->GetStaticData() : NULL;
			if (pItemData)
			{
				switch (iRow) {
				case 0:
					s_pWeaponButton1->SetItemDefinition(pItemData);
					s_pWeaponButton1->GetButton()->SetSelected((iColumn == iWeaponPreset));
				case 1:
					s_pWeaponButton2->SetItemDefinition(pItemData);
					s_pWeaponButton2->GetButton()->SetSelected((iColumn == iWeaponPreset));
				case 2:
					s_pWeaponButton3->SetItemDefinition(pItemData);
					s_pWeaponButton3->GetButton()->SetSelected((iColumn == iWeaponPreset));
				case 3:
					s_pWeaponButton4->SetItemDefinition(pItemData);
					s_pWeaponButton4->GetButton()->SetSelected((iColumn == iWeaponPreset));
				case 4:
					s_pWeaponButton5->SetItemDefinition(pItemData);
					s_pWeaponButton5->GetButton()->SetSelected((iColumn == iWeaponPreset));
				case 5:
					s_pWeaponButton6->SetItemDefinition(pItemData);
					s_pWeaponButton6->GetButton()->SetSelected((iColumn == iWeaponPreset));
				default:
					continue;

				}
			}
		}
	}
};

void CTFLoadoutPanel::GameLayout()
{
	BaseClass::GameLayout();

};

void CTFLoadoutPanel::SetWeaponPreset( int iClass, int iSlot, int iPreset )
{
	GetTFInventory()->SetWeaponPreset( iClass, iSlot, iPreset );
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		char szCmd[64];
		Q_snprintf( szCmd, sizeof( szCmd ), "weaponpresetclass %d %d %d", iClass, iSlot, iPreset );
		engine->ExecuteClientCmd( szCmd );
	}

	DefaultLayout();
}
