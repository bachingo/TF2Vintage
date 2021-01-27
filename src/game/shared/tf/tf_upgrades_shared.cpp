#include "cbase.h"
#include "filesystem.h"
#include "tf_shareddefs.h"
#include "tf_upgrades_shared.h"
#include "tf_gamerules.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMannVsMachineUpgradeManager::CMannVsMachineUpgradeManager() :
	CAutoGameSystem( "MannVsMachineUpgradeManager" ),
	m_UpgradeMap( StringLessThan )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LevelInitPostEntity()
{
	LoadUpgradesFile();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LevelShutdownPostEntity()
{
	m_Upgrades.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LoadUpgradesFile( void )
{
	char const *pszPath = "scripts/items/mvm_upgrades.txt";
	if ( TFGameRules() )
	{
		char const *pszCustomUpgradesFile = TFGameRules()->GetCustomUpgradesFile();
		if( pszCustomUpgradesFile && pszCustomUpgradesFile[0] )
			pszPath = pszCustomUpgradesFile;
	}

	LoadUpgradesFileFromPath( pszPath );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LoadUpgradesFileFromPath( char const *pszPath )
{
	// Validate path is relative and valid
	if ( V_strstr( pszPath, " " ) || V_strstr( pszPath, "\\\\" ) )
		return;
	if ( V_strstr( pszPath, "\r" ) || V_strstr( pszPath, "\n" ) )
		return;
	if ( V_strstr( pszPath, ".." ) || V_strstr( pszPath, ":" ) || V_IsAbsolutePath( pszPath ) )
		return;
	char const *pszExt = V_GetFileExtension( pszPath );
	if ( !pszExt || !pszExt[0] || V_strcmp( pszExt, "txt" ) != 0 )
		return;

	KeyValuesAD kvUpgrades( "Upgrades" );
	if ( !kvUpgrades->LoadFromFile( filesystem, pszPath, "MOD" ) )
	{
		Warning( "Can't open %s\n", pszPath );
		return;
	}

	m_Upgrades.Purge();

	ParseUpgradeBlockForUIGroup( kvUpgrades->FindKey( "ItemUpgrades" ), UIGROUP_UPGRADE_ITEM );
	ParseUpgradeBlockForUIGroup( kvUpgrades->FindKey( "PlayerUpgrades" ), UIGROUP_UPGRADE_PLAYER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::ParseUpgradeBlockForUIGroup( KeyValues *pKVData, int iDefUIGroup )
{
	if ( !pKVData )
		return;

	FOR_EACH_SUBKEY( pKVData, pSubData )
	{
		if ( !pSubData->FindKey( "attribute" ) || !pSubData->FindKey( "icon" ) ||
			 !pSubData->FindKey( "increment" ) || !pSubData->FindKey( "cap" ) || !pSubData->FindKey( "cost" ) )
		{
			Warning( "Upgrades: One or more upgrades missing attribute, icon, increment, cap, or cost value.\n" );
			return;
		}

		char const *pszAttribute = pSubData->GetString( "attribute" );
		CEconAttributeDefinition const *pAttribute = GetItemSchema()->GetAttributeDefinitionByName( pszAttribute );
		if ( pAttribute == NULL )
		{
			Warning( "Upgrades: Invalid attribute reference! -- %s.\n", pszAttribute );
			continue;
		}

		if ( pAttribute->stored_as_integer )
		{
			Warning( "Upgrades: Attribute reference '%s' is not stored as a float!\n", pszAttribute );
			continue;
		}

		int nIndex = m_Upgrades.AddToTail();
		V_strcpy_safe( m_Upgrades[ nIndex ].szAttribute, pszAttribute );
		V_strcpy_safe( m_Upgrades[ nIndex ].szIcon, pSubData->GetString( "icon" ) );
		m_Upgrades[ nIndex ].flIncrement = pSubData->GetFloat( "increment" );
		m_Upgrades[ nIndex ].flCap = pSubData->GetFloat( "cap" );
		m_Upgrades[ nIndex ].nCost = pSubData->GetInt( "cost" );
		m_Upgrades[ nIndex ].nUIGroup = pSubData->GetInt( "ui_group", iDefUIGroup );
		m_Upgrades[ nIndex ].nQuality = pSubData->GetInt( "quality", MVM_UPGRADE_QUALITY_NORMAL );
		m_Upgrades[ nIndex ].nTier = pSubData->GetInt( "tier", 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineUpgradeManager::GetAttributeIndexByName( char const *pszAttributeName )
{
	int nIndex = m_UpgradeMap.Find( pszAttributeName );
	if ( nIndex != m_UpgradeMap.InvalidIndex() )
		return m_UpgradeMap[nIndex];

	FOR_EACH_VEC( m_Upgrades, i )
	{
		if ( FStrEq( m_Upgrades[i].szAttribute, pszAttributeName ) )
		{
			m_UpgradeMap.Insert( pszAttributeName, i );
			return i;
		}
	}

	return m_UpgradeMap.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetUpgradeStepData( CTFPlayer *pPlayer, int iItemSlot, int nUpgradeIndex, int *nCurrentStep, bool *bOverCap )
{
	Assert( nCurrentStep && bOverCap );
	if ( pPlayer == NULL )
		return 0;

	Assert( nUpgradeIndex >= 0 && nUpgradeIndex < g_MannVsMachineUpgrades.GetUpgradeCount() );
	CMannVsMachineUpgrades const &upgrade = g_MannVsMachineUpgrades.GetUpgradeVector()[ nUpgradeIndex ];

	CEconAttributeDefinition const *pAttribute = GetItemSchema()->GetAttributeDefinitionByName( upgrade.szAttribute );
	if ( pAttribute == NULL || pAttribute->stored_as_integer )
		return 0;

	CEconEntity *pEntity = pPlayer->GetEntityForLoadoutSlot( iItemSlot );
	CEconItemView const *pEconItem = pEntity->GetItem();

	// TODO
	/*CTFPowerupBottle *pPowerupBottle = dynamic_cast<CTFPowerupBottle *>( pEntity );
	if ( pPowerupBottle )
	{
		Assert( upgrade.nUIGroup == UIGROUP_UPGRADE_POWERUP );

		*nCurrentStep = 0;
		if ( FindAttribute( pEconItem, pAttribute ) )
			*nCurrentStep = pPowerupBottle->GetNumCharges();

		*bOverCap = pPowerupBottle->GetMaxNumCharges() == *nCurrentStep;

		return pPowerupBottle->GetMaxCharges();
	}*/

	bool bIsPercentage = pAttribute->description_format == ATTRIB_FORMAT_PERCENTAGE || pAttribute->description_format == ATTRIB_FORMAT_INVERTED_PERCENTAGE;
	const float flBaseValue = bIsPercentage ? 1.0f : 0.0f;
	float flCurrentValue = bIsPercentage ? 1.0f : 0.0f;

	if ( upgrade.nUIGroup == UIGROUP_UPGRADE_PLAYER )
	{
		FindAttribute<uint32>( pPlayer->GetAttributeList(), pAttribute, &flCurrentValue );
	}
	else
	{
		Assert( upgrade.nUIGroup == UIGROUP_UPGRADE_ITEM );
		if ( pEconItem )
			FindAttribute<uint32>( pEconItem, pAttribute, &flCurrentValue );
	}

	const float flCap = upgrade.flCap;
	const float flIncrement = upgrade.flIncrement;

	if ( AlmostEqual( flCurrentValue, flCap ) ||
		 ( flIncrement > 0 && flCurrentValue >= flCap ) ||
		 ( flIncrement < 0 && flCurrentValue <= flCap ) )
	{
		*bOverCap = true;
		*nCurrentStep = RoundFloatToInt( fabsf( ( flCurrentValue - flBaseValue ) / flIncrement ) );

		return *nCurrentStep;
	}

	int nNumSteps = RoundFloatToInt( fabsf( ( flCap - flBaseValue ) / flIncrement ) );
	*nCurrentStep = RoundFloatToInt( fabsf( ( flCurrentValue - flBaseValue ) / flIncrement ) );

	return nNumSteps;
}


//-----------------------------------------------------------------------------
CMannVsMachineUpgradeManager g_MannVsMachineUpgrades;
