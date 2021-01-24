#include "cbase.h"
#include "econ_item_schema.h"
#include "econ_item_system.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"

#include "tf_gamerules.h"


#if defined(CLIENT_DLL)
#define UTIL_VarArgs  VarArgs
#endif



//-----------------------------------------------------------------------------
// CEconItemDefinition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemDefinition::~CEconItemDefinition()
{
	FOR_EACH_VEC( attributes, i )
	{
		CEconAttributeDefinition const *pDefinition = attributes[i].GetStaticData();
		pDefinition->type->UnloadEconAttributeValue( &attributes[i].value );
	}

	for ( int team = TEAM_UNASSIGNED; team < TF_TEAM_COUNT; team++ )
	{
		if ( visual[ team ] )
			delete visual[ team ];
	}

	if( definition )
		definition->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PerTeamVisuals_t *CEconItemDefinition::GetVisuals( int iTeamNum /*= TEAM_UNASSIGNED*/ )
{
	if ( iTeamNum > LAST_SHARED_TEAM && iTeamNum < TF_TEAM_COUNT )
	{
		if ( visual[ iTeamNum ] == NULL )
			return visual[ TEAM_UNASSIGNED ];

		return visual[ iTeamNum ];
	}

	return visual[ TEAM_UNASSIGNED ];
}

char const *CEconItemDefinition::GetPerClassModel( int iClass /*= TF_CLASS_UNDEFINED*/ )
{
	if ( iClass < TF_CLASS_COUNT_ALL )
	{
		if( model_player_per_class[ iClass ] && model_player_per_class[ iClass ][0] != '\0' )
			return model_player_per_class[ iClass ];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemDefinition::GetLoadoutSlot( int iClass /*= TF_CLASS_UNDEFINED*/ )
{
	if ( iClass && item_slot_per_class[ iClass ] != -1 )
	{
		return item_slot_per_class[ iClass ];
	}

	return item_slot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CEconItemDefinition::GetPlayerModel( void ) const
{
	if ( ( v_model && v_model[0] != '\0' ) && UseOldWeaponModels() )
		return v_model;

	if ( model_player && model_player[0] != '\0' )
		return model_player;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CEconItemDefinition::GetWorldModel( void ) const
{
	if ( ( w_model && w_model[0] != '\0' ) && UseOldWeaponModels() )
		return w_model;

	if ( model_world && model_world[0] != '\0' )
		return model_world;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemDefinition::GetAttachToHands( void ) const
{
	// This only applies to base items right now, but this allows it to be expanded later.
	if ( CanUseOldModel() )
	{
		if ( UseOldWeaponModels() )
			return 0;
	}

	return attach_to_hands;
}

//-----------------------------------------------------------------------------
// Purpose: Generate item name to show in UI with prefixes, qualities, etc...
//-----------------------------------------------------------------------------
const wchar_t *CEconItemDefinition::GenerateLocalizedFullItemName( void )
{
	static wchar_t wszFullName[256];
	wszFullName[0] = '\0';

	wchar_t wszPrefix[128];
	wchar_t wszQuality[128];
	wszPrefix[0] = '\0';
	wszQuality[0] = '\0';
	
	int iItemNameFlags = 0;


	if ( propername )
	{
		const wchar_t *pszPrepend = g_pVGuiLocalize->Find( "#TF_Unique_Prepend_Proper_Quality" );

		if ( pszPrepend )
		{
			V_wcsncpy( wszPrefix, pszPrepend, sizeof( wszPrefix ) );
			iItemNameFlags += 1;
		}
	}
	
	// Quality prefixes are a bit annoying, so don't bother loading them.
	/* 
	if ( item_quality != QUALITY_NORMAL )
	{
		// Live TF2 allows multiple qualities per item but eh, we don't need that for now.
		const wchar_t *pszQuality = g_pVGuiLocalize->Find( g_szQualityLocalizationStrings[item_quality] );

		if ( pszQuality )
		{
			V_wcsncpy( wszQuality, pszQuality, sizeof( wszQuality ) );
			iItemNameFlags += 2;
		}
	} */

	// Attach the original item name after we're done with all the prefixes.
	wchar_t wszItemName[256];

	const wchar_t *pszLocalizedName = g_pVGuiLocalize->Find( item_name );
	if ( pszLocalizedName && pszLocalizedName[0] )
	{
		V_wcsncpy( wszItemName, pszLocalizedName, sizeof( wszItemName ) );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( item_name, wszItemName, sizeof( wszItemName ) );
	}
	
	// Instead of using an if/else tree, just use a switch with our naming flags.
	switch (iItemNameFlags)
	{
		case 1:		// Prefix
			g_pVGuiLocalize->ConstructString( wszFullName, sizeof( wszFullName ), L"%s1 %s2", 2,
			wszPrefix, wszItemName ); // THE Itemname
			break;
		case 2:		// Quality
			g_pVGuiLocalize->ConstructString( wszFullName, sizeof( wszFullName ), L"%s1 %s2", 2,
			wszQuality, wszItemName ); // QUALITY Itemname
			break;
		case 3:		// Prefix+Quality
			g_pVGuiLocalize->ConstructString( wszFullName, sizeof( wszFullName ), L"%s1 %s2 %s3", 3,
			wszPrefix, wszQuality, wszItemName ); // THE QUALITY Itemname (Note: standard TF2 drops the prefix, but this sounds better.)
			break;
		default:	// No quality or prefix
			g_pVGuiLocalize->ConstructString( wszFullName, sizeof( wszFullName ), L"%s1", 1,
			wszItemName );
			break;	// Itemname
	}

	return wszFullName;
}

//-----------------------------------------------------------------------------
// Purpose: Generate item name without qualities, prefixes, etc. for disguise HUD...
//-----------------------------------------------------------------------------
const wchar_t *CEconItemDefinition::GenerateLocalizedItemNameNoQuality( void )
{
	static wchar_t wszFullName[256];
	wszFullName[0] = '\0';

	wchar_t wszPrefix[128];
	wszPrefix[0] = '\0';

	// Attach "the" if necessary.
	if ( propername )
	{
		const wchar_t *pszPrepend = g_pVGuiLocalize->Find( "#TF_Unique_Prepend_Proper_Quality" );

		if ( pszPrepend )
		{
			V_wcsncpy( wszPrefix, pszPrepend, sizeof( wszPrefix ) );
		}
	}

	// Attach the original item name after we're done with all the prefixes.
	wchar_t wszItemName[256];

	const wchar_t *pszLocalizedName = g_pVGuiLocalize->Find( item_name );
	if ( pszLocalizedName && pszLocalizedName[0] )
	{
		V_wcsncpy( wszItemName, pszLocalizedName, sizeof( wszItemName ) );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( item_name, wszItemName, sizeof( wszItemName ) );
	}

	if ( wszPrefix[0] == '\0' )	// If we don't use a prefix, just use the itemname.
	{
		g_pVGuiLocalize->ConstructString( wszFullName, sizeof( wszFullName ), L"%s1", 1,
		wszItemName );	// Itemname
	}
	else	// Add prefix.
	{
		g_pVGuiLocalize->ConstructString( wszFullName, sizeof( wszFullName ), L"%s1 %s2", 2,
		wszPrefix, wszItemName ); // THE Itemname
	}

	return wszFullName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDefinition::IterateAttributes( IEconAttributeIterator *iter ) const
{
	FOR_EACH_VEC( attributes, i )
	{
		CEconAttributeDefinition const *pDefinition = attributes[i].GetStaticData();
		if ( pDefinition == nullptr )
			continue;

		if ( !pDefinition->type->OnIterateAttributeValue( iter, pDefinition, attributes[i].value ) )
			break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool static_attrib_t::BInitFromKV_SingleLine( KeyValues *const kv )
{
	CEconAttributeDefinition *pAttrib = GetItemSchema()->GetAttributeDefinitionByName( kv->GetName() );
	if( pAttrib == nullptr || pAttrib->index == -1 || pAttrib->type == nullptr )
		return false;

	iAttribIndex = pAttrib->index;

	pAttrib->type->InitializeNewEconAttributeValue( &value );

	if ( !pAttrib->type->BConvertStringToEconAttributeValue( pAttrib, kv->GetString(), &value ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool static_attrib_t::BInitFromKV_MultiLine( KeyValues *const kv )
{
	CEconAttributeDefinition *pAttrib = GetItemSchema()->GetAttributeDefinitionByName( kv->GetName() );
	if( pAttrib == nullptr || pAttrib->index == -1 || pAttrib->type == nullptr )
		return false;

	iAttribIndex = pAttrib->index;

	pAttrib->type->InitializeNewEconAttributeValue( &value );

	if ( !pAttrib->type->BConvertStringToEconAttributeValue( pAttrib, kv->GetString( "value" ), &value ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconAttributeDefinition const *static_attrib_t::GetStaticData( void ) const
{
	return GetItemSchema()->GetAttributeDefinition( iAttribIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ISchemaAttributeType const *static_attrib_t::GetAttributeType( void ) const
{
	CEconAttributeDefinition const *pAttrib = GetStaticData();
	if ( pAttrib )
		return pAttrib->type;
	return nullptr;
}


//-----------------------------------------------------------------------------
// Purpose: Construction
//-----------------------------------------------------------------------------
CAttribute_String::CAttribute_String()
{
	m_nLength = 0;
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAttribute_String::CAttribute_String( CAttribute_String const &src )
{
	CAttribute_String();

	*this = src;
}

//-----------------------------------------------------------------------------
// Purpose: Destruction
//-----------------------------------------------------------------------------
CAttribute_String::~CAttribute_String()
{
	m_pString.Clear();
}


//-----------------------------------------------------------------------------
// Purpose: Copy assignment
//-----------------------------------------------------------------------------
CAttribute_String &CAttribute_String::operator=( CAttribute_String const &src )
{
	Assert( this != &src );

	if ( src.m_bInitialized )
	{
		*this = src.Get();
	}

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: String assignment
//-----------------------------------------------------------------------------
CAttribute_String &CAttribute_String::operator=( char const *src )
{
	Initialize();

	m_pString.Set( src );
	m_nLength = V_strlen( src );

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void EconPerTeamVisuals::operator=( EconPerTeamVisuals const &src )
{
	DeepCopyMap( src.animation_replacement, &animation_replacement );
	DeepCopyMap( src.player_bodygroups, &player_bodygroups );

	playback_activity.Purge();
	playback_activity = src.playback_activity;

	attached_models.Purge();
	attached_models = src.attached_models;

	styles.PurgeAndDeleteElements();
	styles = src.styles;

	skin = src.skin;
	use_per_class_bodygroups = src.use_per_class_bodygroups;
	vm_bodygroup_override = src.vm_bodygroup_override;
	vm_bodygroup_state_override = src.vm_bodygroup_state_override;
	wm_bodygroup_override = src.wm_bodygroup_override;
	wm_bodygroup_state_override = src.wm_bodygroup_state_override;
	custom_particlesystem = src.custom_particlesystem;
	muzzle_flash = src.muzzle_flash;
	tracer_effect = src.tracer_effect;
	material_override = src.material_override;

	Q_memcpy( &aCustomWeaponSounds, src.aCustomWeaponSounds, sizeof( aCustomWeaponSounds ) );
	Q_memcpy( &aWeaponSounds, src.aWeaponSounds, sizeof( aWeaponSounds ) );
}
