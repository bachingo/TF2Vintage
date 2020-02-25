#include "cbase.h"
#include "econ_item_schema.h"
#include "econ_item_system.h"
#include "attribute_types.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"

//-----------------------------------------------------------------------------
// CEconItemAttribute
//-----------------------------------------------------------------------------

BEGIN_NETWORK_TABLE_NOBASE( CEconItemAttribute, DT_EconItemAttribute )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iAttributeDefinitionIndex ) ),
	RecvPropInt( RECVINFO( m_iRawValue32 ) ),
#else
	SendPropInt( SENDINFO( m_iAttributeDefinitionIndex ) ),
	SendPropInt( SENDINFO( m_iRawValue32 ), -1, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute::CEconItemAttribute( CEconItemAttribute const &src )
{
	m_iAttributeDefinitionIndex = src.m_iAttributeDefinitionIndex;
	m_iRawValue32 = src.m_iRawValue32;
	m_iAttributeClass = src.m_iAttributeClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, float flValue, const char *pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	
	m_iRawValue32 = FloatBits( flValue );


	if ( pszAttributeClass )
	{
		m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pszAttributeClass );
	}
	else
	{
		CEconAttributeDefinition *pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pAttribDef->attribute_class );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, const char *pszValue, const char *pszAttributeClass /*= NULL*/ )
{
	m_iAttributeDefinitionIndex = iIndex;
	
	m_iRawValue32 = *(unsigned int *)STRING( AllocPooledString( pszValue ) );


	if ( pszAttributeClass )
	{
		m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pszAttributeClass );
	}
	else
	{
		CEconAttributeDefinition *pAttribDef = GetStaticData();
		if ( pAttribDef )
		{
			m_iAttributeClass = AllocPooledString_StaticConstantStringPointer( pAttribDef->attribute_class );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute &CEconItemAttribute::operator=( CEconItemAttribute const &src )
{
	m_iAttributeDefinitionIndex = src.m_iAttributeDefinitionIndex;
	m_iRawValue32 = src.m_iRawValue32;
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
	attributes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PerTeamVisuals_t *CEconItemDefinition::GetVisuals( int iTeamNum /*= TEAM_UNASSIGNED*/ )
{
	if ( iTeamNum > LAST_SHARED_TEAM && iTeamNum < TF_TEAM_COUNT )
	{
		return &visual[iTeamNum];
	}

	return &visual[TEAM_UNASSIGNED];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemDefinition::GetLoadoutSlot( int iClass /*= TF_CLASS_UNDEFINED*/ )
{
	if ( iClass && item_slot_per_class[iClass] != -1 )
	{
		return item_slot_per_class[iClass];
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
		if ( !pDefinition->type->OnIterateAttributeValue( iter, pDefinition, attributes[ i ].value ) )
			return;
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
// Purpose: 
//-----------------------------------------------------------------------------
void CAttribute_String::Clear( void )
{
	if ( m_bInitialized && m_pString != &_default_value_ )
	{
		m_pString->Clear();
	}

	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: Copy assignment
//-----------------------------------------------------------------------------
CAttribute_String &CAttribute_String::operator=( CAttribute_String const &src )
{
	Assert( this != &src );

	if ( src.m_bInitialized )
	{
		Initialize();
		
		m_pString->Set( src.Get() );
	}

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: String assignment
//-----------------------------------------------------------------------------
CAttribute_String &CAttribute_String::operator=( char const *src )
{
	Clear();
	Initialize();

	m_pString->Set( src );

	return *this;
}

CUtlConstString CAttribute_String::_default_value_;
