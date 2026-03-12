#include "cbase.h"
#include "tf_itemselectionpanel.h"
#include "tf_mainmenu.h"
#include "controls/tf_advitembutton.h"
#include "basemodelpanel.h"
#include <vgui/ILocalize.h>
#include "script_parser.h"
#include "econ_item_view.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_bShowItemMenu = false;

// corresponds to g_LoadoutSlots.
// differs from live
const char *g_szEquipSlotHeader[] =
{
	"#ItemSel_PRIMARY",
	"#ItemSel_SECONDARY",
	"#ItemSel_MELEE",
	"#ItemSel_PDA",
	"#ItemSel_PDA",
	"#ItemSel_PDA",

	"#ItemSel_UTILITY",
	"#ItemSel_ACTION",

	"#ItemSel_HEAD",
	"#ItemSel_MISC",
	"#ItemSel_MISC",
	"#ItemSel_MISC",
	"#ItemSel_MISC",
	"#ItemSel_MISC",

	"#ItemSel_TAUNT",
	"#ItemSel_TAUNT",
	"#ItemSel_TAUNT",
	"#ItemSel_TAUNT",
	"#ItemSel_TAUNT",
	"#ItemSel_TAUNT",
	"#ItemSel_TAUNT",
	"#ItemSel_TAUNT"
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

void CTFWeaponSelectPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_nItemColumns = inResourceData->GetInt( "columns", 6 );
	m_nItemRows = inResourceData->GetInt( "rows", 3 );

	m_nItemXSpace = inResourceData->GetInt( "xspace", 4 );
	m_nItemYSpace = inResourceData->GetInt( "yspace", 3 );

	m_nItemWidth = inResourceData->GetInt( "itemwidth", 94 );
	m_nItemHeight = inResourceData->GetInt( "itemheight", 70 );
	
	InvalidateLayout( false, true ); // force ApplySchemeSettings to run
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
}

bool CTFItemPanel::Init()
{
	BaseClass::Init();

	m_iCurrentClass = TF_CLASS_SCOUT;
	m_iCurrentSlot = TF_LOADOUT_SLOT_PRIMARY;

	g_TFWeaponScriptParser.InitParser("scripts/tf_weapon_*.txt", true, false);

	char chEmptyLoc[32];
	wchar_t *wcLoc = g_pVGuiLocalize->Find("SelectNoItemSlot");
	g_pVGuiLocalize->ConvertUnicodeToANSI( wcLoc, chEmptyLoc, sizeof( chEmptyLoc ) );

	m_pWeaponSetPanel = new CTFWeaponSelectPanel( this, "weaponsetpanel" );
	m_pItemSlotLabel = new CExLabel( this, "ItemSlotLabel", "#PrimaryWeapon" );

	CTFAdvItemButton *itembutton = nullptr;
	for (int i = 0; i < INVENTORY_VECTOR_NUM_SELECTION; i++)
	{
		itembutton = new CTFAdvItemButton( m_pWeaponSetPanel, "WeaponIcons", chEmptyLoc );
		itembutton->SetVisible( false );
		m_pWeaponIcons.AddToTail( itembutton );
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
}

void CTFItemPanel::SetCurrentClassAndSlot(int iClass, int iSlot)
{
	if (m_iCurrentClass != iClass)
		m_iCurrentClass = iClass;

	m_iCurrentSlot = g_aClassLoadoutSlots[iClass][iSlot];

	DefaultLayout();
}


void CTFItemPanel::OnCommand(const char* command)
{
	if ( !Q_strcmp ( command, "back" ) || (!Q_strcmp ( command, "vguicancel" )) )
	{
		g_bShowItemMenu = false;
		Hide();
	}
	else if ( !Q_strncmp ( command, "loadout", 7 ) )
	{
		GetParent()->OnCommand ( command );
		return;
	}
	else if ( !Q_strcmp( command, "nextpage" ) )
	{
		if ( m_nPage + 1 < m_nPageCount )
		{
			SetupItemsPage( m_nPage + 1 );
		}
		return;
	}
	else if ( !Q_strcmp( command, "prevpage" ) )
	{
		if ( m_nPage - 1 >= 0 )
		{
			SetupItemsPage( m_nPage - 1 );
		}
		return;
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTFItemPanel::Show()
{
	BaseClass::Show();
	DefaultLayout();
}

void CTFItemPanel::Hide()
{
	BaseClass::Hide();
}

void CTFItemPanel::OnTick()
{
	BaseClass::OnTick();
}

void CTFItemPanel::OnThink()
{
	BaseClass::OnThink();
}

void CTFItemPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();

	if ( m_iCurrentSlot != -1 )
	{
		int iClassIndex = m_iCurrentClass;
		SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( g_aPlayerClassNames[iClassIndex] ) );
		m_pItemSlotLabel->SetText( g_szEquipSlotHeader[m_iCurrentSlot] );

		int itemCount = 0;
		for ( int i = 0; i < INVENTORY_VECTOR_NUM_SELECTION; i++ )
		{
			CEconItemView *pItem = NULL;
			if ( m_iCurrentSlot != -1 )
			{
				pItem = GetTFInventory()->GetItem( iClassIndex, m_iCurrentSlot, i );
			}
			if ( pItem )
			{
				itemCount++;
			}
		}
		//DevMsg( "Total items: %d\n", itemCount );
		m_nPageCount = Ceil2Int( itemCount / (1.0 * m_pWeaponSetPanel->m_nItemRows * m_pWeaponSetPanel->m_nItemColumns));
		SetupItemsPage( 0 );
	}
}

void CTFItemPanel::SetupItemsPage( int iPage )
{
	m_nPage = iPage;

	SetDialogVariable( "backpackpage", VarArgs( "%d/%d", m_nPage + 1, m_nPageCount ) );

	// hide all previously visible items
	for ( int i = 0; i < INVENTORY_VECTOR_NUM_SELECTION; i++ )
		m_pWeaponIcons[i]->SetVisible( false );

	int first = m_nPage * m_pWeaponSetPanel->m_nItemColumns * m_pWeaponSetPanel->m_nItemRows;
	int last = MIN( first + m_pWeaponSetPanel->m_nItemColumns * m_pWeaponSetPanel->m_nItemRows, INVENTORY_VECTOR_NUM_SELECTION );

	for ( int i = first; i < last; i++ )
	{
		SetupItem( i );
	}
}

void CTFItemPanel::SetupItem( int iItem )
{
	CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[iItem];
	CEconItemView *pItem = NULL;

	pItem = GetTFInventory()->GetItem( m_iCurrentClass, m_iCurrentSlot, iItem );
	if ( pItem )
	{
		CEconItemDefinition *pItemData = pItem->GetStaticData();

		if ( pItemData )
		{
			m_pWeaponButton->SetSize( XRES( m_pWeaponSetPanel->m_nItemWidth ) - m_pWeaponSetPanel->m_nItemXSpace, YRES( m_pWeaponSetPanel->m_nItemHeight ) - m_pWeaponSetPanel->m_nItemYSpace );
			m_pWeaponButton->SetBorderVisible( true );
			m_pWeaponButton->SetItemDefinition( pItemData );
			m_pWeaponButton->SetLoadoutSlot( m_iCurrentSlot, iItem );
			int iWeaponPreset = GetTFInventory()->GetWeaponPreset( m_iCurrentClass, m_iCurrentSlot );
			if ( iItem == iWeaponPreset )
			{
				m_pWeaponButton->SetBorderByString( "BackpackItemBorder_Unique", "BackpackItemMouseOverBorder_Unique", "BackpackItemMouseOverBorder_Unique" );
				m_pWeaponButton->GetButton()->SetSelected( true );
			}
			else
			{
				m_pWeaponButton->SetBorderByString( "EconItemBorder", "LoadoutItemMouseOverBorder", "LoadoutItemMouseOverBorder" );
			}
			int offset = m_nPage * m_pWeaponSetPanel->m_nItemColumns * m_pWeaponSetPanel->m_nItemRows;

			m_pWeaponButton->SetPos( ((iItem - offset) % m_pWeaponSetPanel->m_nItemColumns) * XRES( m_pWeaponSetPanel->m_nItemWidth ) + XRES( m_pWeaponSetPanel->m_nItemXSpace ),
									 ((iItem - offset) / m_pWeaponSetPanel->m_nItemColumns) * YRES( m_pWeaponSetPanel->m_nItemHeight ) + YRES( m_pWeaponSetPanel->m_nItemYSpace ));
			m_pWeaponButton->SetVisible( true );
		}
	}
	else
	{
		m_pWeaponButton->SetVisible( false );
	}
}

void CTFItemPanel::GameLayout()
{
	BaseClass::GameLayout();
}