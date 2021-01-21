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

//static CTFWeaponSetPanel *s_pWeaponSpace;

static char* pszClassModels[TF_CLASS_COUNT_ALL] =
{
	"",
	"models/player/hwm/scout.mdl",
	"models/player/hwm/sniper.mdl",
	"models/player/hwm/soldier.mdl",
	"models/player/hwm/demo.mdl",
	"models/player/hwm/medic.mdl",
	"models/player/hwm/heavy.mdl",
	"models/player/hwm/pyro.mdl",
	"models/player/hwm/spy.mdl",
	"models/player/hwm/engineer.mdl",
	"models/player/saxton_hale_jungle_inferno/saxton_hale.mdl",
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

static class CTFWeaponScriptParser : public C_ScriptParser
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
		else if ( szWeaponType == "misc" || szWeaponType == "misc2" || szWeaponType == "action" || szWeaponType == "zombie" || szWeaponType == "medal" )
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
} g_TFWeaponScriptParser;

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
	m_pWeaponIcons.RemoveAll();
	
}

bool CTFLoadoutPanel::Init()
{
	BaseClass::Init();

	m_iCurrentClass = TF_CLASS_SCOUT;
	m_iCurrentSlot = TF_LOADOUT_SLOT_PRIMARY;
	m_pClassModelPanel = new CTFAdvModelPanel( this, "classmodelpanel" );
	m_pGameModelPanel = new CModelPanel( this, "gamemodelpanel" );
	m_pItemPanel = NULL;
	g_TFWeaponScriptParser.InitParser( "scripts/tf_weapon_*.txt", true, false );

	for ( int i = 0; i < INVENTORY_ROWNUM; i++ )
	{
		char szWeaponButton[64];
		Q_snprintf ( szWeaponButton, sizeof ( szWeaponButton ), "weaponbutton%i", i );
		m_pWeaponIcons.AddToTail ( new CTFAdvItemButton ( this, szWeaponButton, "DUK" ) );
	}

	for ( int iClassIndex = 0; iClassIndex < TF_CLASS_COUNT_ALL; iClassIndex++ )
	{
		if ( pszClassModels[iClassIndex][0] != '\0' )
			modelinfo->FindOrLoadModel( pszClassModels[iClassIndex] );
	}
	
	return true;
}

void CTFLoadoutPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/LoadoutPanel.res" );
	m_pItemPanel = dynamic_cast<CTFItemPanel*>( GetMenuPanel( ITEMSELCTION_MENU ) );
}

void CTFLoadoutPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	for ( int i = 0; i < INVENTORY_ROWNUM; i++ )
	{
		CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[i];
		m_pWeaponButton->SetLoadoutSlot( i, 0 );
	}
};


void CTFLoadoutPanel::SetCurrentClass(int iClass)
{
	if (m_iCurrentClass == iClass)
		return;

	if ( g_bShowItemMenu )
	{
		m_pItemPanel->SetEnabled(false);
		g_bShowItemMenu = false;
		MAINMENU_ROOT->HidePanel(ITEMSELCTION_MENU);
	}

	m_iCurrentClass = iClass;
	m_iCurrentSlot = g_aClassLoadoutSlots[iClass][0];
	DefaultLayout();
	UpdateMenuBodygroups();

};


void CTFLoadoutPanel::OnCommand ( const char* command )
{
	if ( !Q_strcmp ( command, "back" ) || (!Q_strcmp ( command, "vguicancel" )) )
	{
		Hide();
	}
	else if ( !Q_strcmp ( command, "select_scout" ) )
	{
		SetCurrentClass ( TF_CLASS_SCOUT );
	}
	else if ( !Q_strcmp ( command, "select_soldier" ) )
	{
		SetCurrentClass ( TF_CLASS_SOLDIER );
	}
	else if ( !Q_strcmp ( command, "select_pyro" ) )
	{
		SetCurrentClass ( TF_CLASS_PYRO );
	}
	else if ( !Q_strcmp ( command, "select_demoman" ) )
	{
		SetCurrentClass ( TF_CLASS_DEMOMAN );
	}
	else if ( !Q_strcmp ( command, "select_heavyweapons" ) )
	{
		SetCurrentClass ( TF_CLASS_HEAVYWEAPONS );
	}
	else if ( !Q_strcmp ( command, "select_engineer" ) )
	{
		SetCurrentClass ( TF_CLASS_ENGINEER );
	}
	else if ( !Q_strcmp ( command, "select_medic" ) )
	{
		SetCurrentClass ( TF_CLASS_MEDIC );
	}
	else if ( !Q_strcmp ( command, "select_sniper" ) )
	{
		SetCurrentClass ( TF_CLASS_SNIPER );
	}
	else if ( !Q_strcmp ( command, "select_spy" ) )
	{
		SetCurrentClass ( TF_CLASS_SPY );
	}
	else if ( !Q_strncmp ( command, "loadout", 7 ) )
	{
		if ( g_bShowItemMenu )
		{
			CTFItemPanel *ItemPanel = dynamic_cast< CTFItemPanel* >(GetMenuPanel ( ITEMSELCTION_MENU ));
			ItemPanel->SetEnabled ( false );
			g_bShowItemMenu = false;
			MAINMENU_ROOT->HidePanel ( ITEMSELCTION_MENU );
			const char *sChar = strchr ( command, ' ' );
			if ( sChar )
			{
				int iSlot = atoi ( sChar + 1 );
				sChar = strchr ( sChar + 1, ' ' );
				if ( sChar )
				{
					int iWeapon = atoi ( sChar + 1 );
					SetSlotAndPreset ( iSlot, iWeapon );
				}
			}
		}
		else
		{
			CTFItemPanel *ItemPanel = dynamic_cast< CTFItemPanel* >(GetMenuPanel ( ITEMSELCTION_MENU ));
			ItemPanel->SetEnabled ( true );
			const char *sChar = strchr ( command, ' ' );
			int iSlot = atoi ( sChar + 1 );
			ItemPanel->SetCurrentClassAndSlot ( m_iCurrentClass, iSlot );
			g_bShowItemMenu = true;
			MAINMENU_ROOT->ShowPanel ( ITEMSELCTION_MENU );
		}
		return;
	}
	else if ( !Q_strncmp( command, "loadpreset_", 11 ) )
	{
		const int iPresetIndex = atoi( command + 11 );
		if (iPresetIndex < TF_MAX_PRESETS)
		{
			GetTFInventory()->ChangeLoadoutSlot( m_iCurrentClass, iPresetIndex );
			
			// Need to update our character's active loadout ingame, since they swapped presets.
			C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
			if (pPlayer)
				pPlayer->LoadInventory();
			
			DefaultLayout();
		}
	}
	else
	{
		BaseClass::OnCommand ( command );
	}
}

void CTFLoadoutPanel::SetSlotAndPreset( int iSlot, int iPreset )
{
	SetCurrentSlot( iSlot );
	SetWeaponPreset( m_iCurrentClass, m_iCurrentSlot, iPreset );
}

int CTFLoadoutPanel::GetAnimSlot( CEconItemDefinition *pItemDef, int iClass )
{
	if ( !pItemDef )
		return -1;

	int iSlot = pItemDef->anim_slot;
	if ( iSlot == -1 )
	{
		// Fall back to script file data.
		const char *pszClassname = TranslateWeaponEntForClass( pItemDef->GetClassName(), iClass );
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
		if ( pItemDef->GetPerClassModel( iClass ) )
			return pItemDef->GetPerClassModel( iClass );

		return pItemDef->GetPlayerModel();
	}

	const char *pszModel = pItemDef->GetWorldModel();

	if ( !pszModel && pItemDef->attach_to_hands == 1 )
	{
		pszModel = pItemDef->GetPlayerModel();
	}
	if ( pItemDef->GetPerClassModel( iClass ) && pItemDef->attach_to_hands == 1 )
	{
		pszModel = pItemDef->GetPerClassModel( iClass );
	}

	return pszModel;
}

const char *CTFLoadoutPanel::GetExtraWearableModel( CEconItemDefinition *pItemDef )
{
	if ( pItemDef && pItemDef->GetExtraWearableModel() )
	{
		return pItemDef->GetExtraWearableModel();
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
			if ( pszModel && pszModel[0] != '\0' )
			{
				int nSkin = m_iCurrentSkin;
				PerTeamVisuals_t *pVisuals = NULL;
				switch ( m_iCurrentSkin )
				{
					case TF_TEAM_RED:
						pVisuals = pItemDef->GetVisuals( TF_TEAM_RED );
						break;
					case TF_TEAM_BLUE:
						pVisuals = pItemDef->GetVisuals( TF_TEAM_BLUE );
						break;
					case TF_TEAM_GREEN:
						pVisuals = pItemDef->GetVisuals( TF_TEAM_GREEN );
						nSkin -= 6; // Convert from player skin number to weapon skin number.
						break;
					case TF_TEAM_YELLOW:
						pVisuals = pItemDef->GetVisuals( TF_TEAM_YELLOW );
						nSkin -= 6; // Convert from player skin number to weapon skin number.
						break;
				}

				if (pVisuals && pVisuals->skin != -1)
				{
					nSkin = pVisuals->skin;
				}

				m_pClassModelPanel->SetMergeMDL(pszModel, NULL, nSkin);
			}
		}

		const char *pszExtraWearableModel = GetExtraWearableModel( pItem->GetStaticData() );
		if ( pszExtraWearableModel[0] != '\0' )
		{
			m_pClassModelPanel->SetMergeMDL( pszExtraWearableModel, NULL, m_iCurrentSkin );
		}
		

	}

	UpdateMenuBodygroups();

	// Set the animation.
	m_pClassModelPanel->SetAnimationIndex( iAnimationIndex >= 0 ? iAnimationIndex : TF_WPN_TYPE_PRIMARY );

	m_pClassModelPanel->Update();
}

void CTFLoadoutPanel::UpdateMenuBodygroups(void)
{
	int iPreset = GetTFInventory()->GetWeaponPreset(m_iCurrentClass, m_iCurrentSlot);
	CEconItemView *pActiveItem = GetTFInventory()->GetItem(m_iCurrentClass, m_iCurrentSlot, iPreset);
	if (!pActiveItem)
		return;
	
	// Reset all bodygroups back to normal first, to start with a clean bodygroup set.
	for (int i = 0; i < m_pClassModelPanel->GetNumBodyGroups(); i++)
	{
		m_pClassModelPanel->SetBodygroup(i, 0);
	}
	
	// Update all applicable bodygroups.
	// Check through all inventory slots and see if they affect bodygroups.
	for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot < TF_LOADOUT_SLOT_EVENT; iSlot++)
	{
		// Don't check special slots.
		if ( iSlot == TF_LOADOUT_SLOT_ACTION || iSlot == TF_LOADOUT_SLOT_UTILITY )
			continue;
		
		// Get item definitions.

		int iSlotPreset = GetTFInventory()->GetWeaponPreset(m_iCurrentClass, iSlot);
		CEconItemView *pEquippedItem = GetTFInventory()->GetItem(m_iCurrentClass, iSlot, iSlotPreset);
		if (!pEquippedItem)
			continue;
		CEconItemDefinition *pItemDef = pEquippedItem->GetStaticData();
		if (!pItemDef)
			continue;

		// For items with visual changes, update the appropriate bodygroup.
		PerTeamVisuals_t *pVisuals = pItemDef->GetVisuals();
		for (int i = 0; i < m_pClassModelPanel->GetNumBodyGroups(); i++)
		{
			// Get the bodygroup affected and update.
			if (pVisuals)
			{
				unsigned int index = pVisuals->player_bodygroups.Find(m_pClassModelPanel->GetBodygroupName(i));
				if (pVisuals->player_bodygroups.IsValidIndex(index))
				{
					bool bTrue = pVisuals->player_bodygroups.Element(index);
					if (bTrue)
					{
						if (((pEquippedItem == pActiveItem && pItemDef->hide_bodygroups_deployed_only) || ( !pItemDef->hide_bodygroups_deployed_only ) ) || pItemDef->act_as_wearable )
						{
							m_pClassModelPanel->SetBodygroup(i, 1);
						}
					}
				}
			}
		}
	}
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
	if (InGame() && !g_bFromPause)
		engine->ExecuteClientCmd("escape");
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
	// Reset all active bodygroups.
	for (int i = 0; i < m_pClassModelPanel->GetNumBodyGroups(); i++)
	{
		m_pClassModelPanel->SetBodygroup(i, 0);
	}
}

void CTFLoadoutPanel::UpdateModelPanels()
{
	int iClassIndex = m_iCurrentClass;

	m_pClassModelPanel->SetVisible( true );
	m_pGameModelPanel->SetVisible( false );

	SetModelClass( iClassIndex );
	UpdateModelWeapons();
}

void CTFLoadoutPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		switch ( pPlayer->GetTeamNumber() )
		{
		case TF_TEAM_RED:
			m_iCurrentSkin = 0;
			break;
		case TF_TEAM_BLUE:
			m_iCurrentSkin = 1;
			break;
		case TF_TEAM_GREEN:
			m_iCurrentSkin = 8;
			break;
		case TF_TEAM_YELLOW:
			m_iCurrentSkin = 9;
			break;
		}
	}

	UpdateModelPanels();

	int iClassIndex = m_iCurrentClass;
	SetDialogVariable( "classname", g_pVGuiLocalize->Find( g_aPlayerClassNames[iClassIndex] ) );

	for (int iRow = 0; iRow < INVENTORY_ROWNUM; iRow++)
	{
		int iSlot = g_aClassLoadoutSlots[iClassIndex][iRow];

		for ( int iColumn = 0; iColumn < INVENTORY_COLNUM; iColumn++ )
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
				CTFAdvItemButton *m_pWeaponButton = m_pWeaponIcons[iRow];
				m_pWeaponButton->SetItemDefinition( pItemData );
				m_pWeaponButton->GetButton()->SetSelected( (iColumn == iWeaponPreset) );
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
