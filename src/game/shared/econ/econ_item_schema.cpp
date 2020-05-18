#include "cbase.h"
#include "econ_item_schema.h"
#include "econ_item_system.h"
#include "attribute_types.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"

#if defined(CLIENT_DLL)
#define UTIL_VarArgs  VarArgs
#endif

//-----------------------------------------------------------------------------
// CEconItemAttribute
//-----------------------------------------------------------------------------

BEGIN_NETWORK_TABLE_NOBASE( CEconItemAttribute, DT_EconItemAttribute )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iAttributeDefinitionIndex ) ),
	RecvPropInt( RECVINFO_NAME( m_flValue, m_iRawValue32 ) ),
	RecvPropFloat( RECVINFO( m_flValue ), SPROP_NOSCALE ),
#else
	SendPropInt( SENDINFO( m_iAttributeDefinitionIndex ), -1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NAME( m_flValue, m_iRawValue32 ), 32, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute::CEconItemAttribute( CEconItemAttribute const &src )
{
	m_iAttributeDefinitionIndex = src.m_iAttributeDefinitionIndex;
	m_flValue = src.m_flValue;
	m_iAttributeClass = src.m_iAttributeClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, float flValue, const char *pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	
	m_flValue = flValue;


	if ( pszAttributeClass )
	{
		m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pszAttributeClass );
	}
	else
	{
		CEconAttributeDefinition *pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pAttribDef->GetClassName() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, const char *pszValue, const char *pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	
	m_flValue = *(float *)( (unsigned int *)STRING( AllocPooledString( pszValue ) ) );


	if ( pszAttributeClass )
	{
		m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pszAttributeClass );
	}
	else
	{
		CEconAttributeDefinition *pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pAttribDef->GetClassName() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute &CEconItemAttribute::operator=( CEconItemAttribute const &src )
{
	m_iAttributeDefinitionIndex = src.m_iAttributeDefinitionIndex;
	m_flValue = src.m_flValue;
	m_iAttributeClass = src.m_iAttributeClass;

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconAttributeDefinition *CEconItemAttribute::GetStaticData( void )
{
	return GetItemSchema()->GetAttributeDefinition( m_iAttributeDefinitionIndex );
}


//-----------------------------------------------------------------------------
// CEconAttributeDefinition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconAttributeDefinition &CEconAttributeDefinition::operator=( CEconAttributeDefinition const &rhs )
{
	index = rhs.index;
	type = rhs.type;
	string_attribute = rhs.string_attribute;
	description_format = rhs.description_format;
	effect_type = rhs.effect_type;
	hidden = rhs.hidden;
	stored_as_integer = rhs.stored_as_integer;
	m_iAttributeClass = rhs.m_iAttributeClass;

	definition = rhs.definition->MakeCopy();
	
	V_strcpy_safe( name, rhs.name );
	V_strcpy_safe( attribute_class, rhs.attribute_class );
	V_strcpy_safe( description_string, rhs.description_string );

	return *this;
}


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
	attributes.Purge();

	for ( int team = TEAM_UNASSIGNED; team < TF_TEAM_COUNT; team++ )
	{
		if ( visual[ team ] )
			delete visual[ team ];
	}

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
void CEconItemDefinition::IterateAttributes( IEconAttributeIterator *iter )
{
	FOR_EACH_VEC( attributes, i )
	{
		CEconAttributeDefinition const *pDefinition = attributes[i].GetStaticData();
		if ( !pDefinition->type->OnIterateAttributeValue( iter, pDefinition, attributes[i].value ) )
			return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemDefinition &CEconItemDefinition::operator=( CEconItemDefinition const &rhs )
{
	index = rhs.index;
	attributes = rhs.attributes;
	used_by_classes = rhs.used_by_classes;
	show_in_armory = rhs.show_in_armory;
	item_slot = rhs.item_slot;
	anim_slot = rhs.anim_slot;
	item_quality = rhs.item_quality;
	baseitem = rhs.baseitem;
	propername = rhs.propername;
	min_ilevel = rhs.min_ilevel;
	max_ilevel = rhs.max_ilevel;
	image_inventory_size_h = rhs.image_inventory_size_h;
	image_inventory_size_w = rhs.image_inventory_size_w;
	loadondemand = rhs.loadondemand;
	attach_to_hands = rhs.attach_to_hands;
	attach_to_hands_vm_only = rhs.attach_to_hands_vm_only;
	act_as_wearable = rhs.act_as_wearable;
	hide_bodygroups_deployed_only = rhs.hide_bodygroups_deployed_only;
	is_reskin = rhs.is_reskin;
	specialitem = rhs.specialitem;
	demoknight = rhs.demoknight;
	drop_type = rhs.drop_type;
	year = rhs.year;
	is_custom_content = rhs.is_custom_content;
	is_cut_content = rhs.is_cut_content;
	is_multiclass_item = rhs.is_multiclass_item;

	FOR_EACH_DICT_FAST( rhs.capabilities, i )
	{
		capabilities.Insert( rhs.capabilities.GetElementName( i ), rhs.capabilities.Element( i ) );
	}

	FOR_EACH_DICT_FAST( rhs.tags, i )
	{
		tags.Insert( rhs.tags.GetElementName( i ), rhs.tags.Element( i ) );
	}

	for ( int team = TEAM_UNASSIGNED; team < TF_TEAM_COUNT; ++team )
	{
		visual[team] = rhs.visual[team];
	}

	for ( int _class = TF_CLASS_UNDEFINED; _class < TF_CLASS_COUNT_ALL; ++_class )
	{
		model_player_per_class[_class] = NULL;
		item_slot_per_class[_class] = rhs.item_slot_per_class[_class];
	}

	definition = rhs.definition->MakeCopy();
	if ( definition )
	{
		name = definition->GetString( "name", "( unnamed )" );
		model_player = definition->GetString( "model_player" );
		model_vision_filtered = definition->GetString( "model_vision_filtered" );
		model_world = definition->GetString( "model_world" );
		extra_wearable = definition->GetString( "extra_wearable" );
		item_class = definition->GetString( "item_class" );
		item_type_name = definition->GetString( "item_type_name" );
		item_name = definition->GetString( "item_name" );
		item_description = definition->GetString( "item_description" );
		item_logname = definition->GetString( "item_logname" );
		item_iconname = definition->GetString( "item_iconname" );
		image_inventory = definition->GetString( "image_inventory" );
		equip_region = definition->GetString( "equip_region" );
		holiday_restriction = definition->GetString( "holiday_restriction" );

		if ( KeyValues *pSubData = definition->FindKey( "model_player_per_class" ) )
		{
			for ( KeyValues *pClassData = pSubData->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
			{
				const char *pszClass = pClassData->GetName();

				// Generic item, assign a model for every class.
				if ( !V_stricmp( pszClass, "basename" ) )
				{
					for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
					{
						// Add to the player model per class.
						model_player_per_class[i] = UTIL_VarArgs( pClassData->GetString(), g_aRawPlayerClassNamesShort[i] );
					}
				}
				else
				{
					int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
					if ( iClass != -1 )
					{
						model_player_per_class[iClass] = pClassData->GetString();
					}
				}
			}
		}
	}

	return *this;
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
// Purpose: Construction
//-----------------------------------------------------------------------------
CAttribute_String::CAttribute_String()
{
	m_pString = &_default_value_;
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
	if ( m_pString != &_default_value_ && m_pString != nullptr )
		delete m_pString;
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

	m_pString->Set( src );
	m_nLength = V_strlen( src );

	return *this;
}

CUtlConstString CAttribute_String::_default_value_;
