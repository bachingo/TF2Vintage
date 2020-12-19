#include "cbase.h"
#include "econ_item_view.h"
#include "econ_item_system.h"
#include "activitylist.h"

#ifdef CLIENT_DLL
#include "dt_utlvector_recv.h"
#else
#include "dt_utlvector_send.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_ATTRIBUTES_SENT 20

//-----------------------------------------------------------------------------
// CEconItemAttribute
//-----------------------------------------------------------------------------

BEGIN_NETWORK_TABLE_NOBASE( CEconItemAttribute, DT_EconItemAttribute )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iAttributeDefinitionIndex ) ),
	RecvPropInt( RECVINFO_NAME( m_flValue, m_iRawValue32 ) ),
	RecvPropFloat( RECVINFO( m_flValue ), SPROP_NOSCALE ),
	RecvPropInt( RECVINFO( m_nRefundableCurrency ) ),
#else
	SendPropInt( SENDINFO( m_iAttributeDefinitionIndex ), -1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NAME( m_flValue, m_iRawValue32 ), 32, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nRefundableCurrency ), -1, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute::CEconItemAttribute( CEconItemAttribute const &src )
{
	m_iAttributeDefinitionIndex = src.m_iAttributeDefinitionIndex;
	m_flValue = src.m_flValue;
	m_nRefundableCurrency = src.m_nRefundableCurrency;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, float flValue )
{
	m_iAttributeDefinitionIndex = iIndex;
	m_flValue = flValue;
	m_nRefundableCurrency = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( int iIndex, uint32 unValue )
{
	m_iAttributeDefinitionIndex = iIndex;
	m_flValue = *(float*)&unValue;
	m_nRefundableCurrency = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute &CEconItemAttribute::operator=( CEconItemAttribute const &src )
{
	m_iAttributeDefinitionIndex = src.m_iAttributeDefinitionIndex;
	m_flValue = src.m_flValue;
	m_nRefundableCurrency = src.m_nRefundableCurrency;

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
// Purpose: 
//-----------------------------------------------------------------------------
CEconAttributeDefinition const *CEconItemAttribute::GetStaticData( void ) const
{
	return GetItemSchema()->GetAttributeDefinition( m_iAttributeDefinitionIndex );
}


#ifdef CLIENT_DLL
BEGIN_RECV_TABLE_NOBASE( CAttributeList, DT_AttributeList )
	RecvPropUtlVectorDataTable( m_Attributes, MAX_ATTRIBUTES_SENT, DT_EconItemAttribute )
END_RECV_TABLE()
#else
BEGIN_SEND_TABLE_NOBASE( CAttributeList, DT_AttributeList )
	SendPropUtlVectorDataTable( m_Attributes, MAX_ATTRIBUTES_SENT, DT_EconItemAttribute )
END_SEND_TABLE()
#endif

BEGIN_DATADESC_NO_BASE( CAttributeList )
END_DATADESC()

#ifdef CLIENT_DLL
BEGIN_RECV_TABLE_NOBASE( CEconItemView, DT_ScriptCreatedItem )
	RecvPropInt( RECVINFO( m_iItemDefinitionIndex ) ),
	RecvPropInt( RECVINFO( m_iEntityQuality ) ),
	RecvPropInt( RECVINFO( m_iEntityLevel ) ),
	RecvPropInt( RECVINFO( m_iTeamNumber ) ),
	RecvPropBool( RECVINFO( m_bOnlyIterateItemViewAttributes ) ),
	RecvPropDataTable( RECVINFO_DT( m_AttributeList ), 0, &REFERENCE_RECV_TABLE( DT_AttributeList ) ),
END_RECV_TABLE()
#else
BEGIN_SEND_TABLE_NOBASE( CEconItemView, DT_ScriptCreatedItem )
	SendPropInt( SENDINFO( m_iItemDefinitionIndex ) ),
	SendPropInt( SENDINFO( m_iEntityQuality ) ),
	SendPropInt( SENDINFO( m_iEntityLevel ) ),
	SendPropInt( SENDINFO( m_iTeamNumber ) ),
	SendPropBool( SENDINFO( m_bOnlyIterateItemViewAttributes ) ),
	SendPropDataTable( SENDINFO_DT( m_AttributeList ), &REFERENCE_SEND_TABLE( DT_AttributeList ) ),
END_SEND_TABLE()
#endif

BEGIN_DATADESC_NO_BASE( CEconItemView )
	DEFINE_FIELD( m_iItemDefinitionIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_iEntityQuality, FIELD_INTEGER ),
	DEFINE_FIELD( m_iEntityLevel, FIELD_INTEGER ),
	DEFINE_FIELD( m_bOnlyIterateItemViewAttributes, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( m_AttributeList ),
END_DATADESC()

#define FIND_ELEMENT(map, key, val)			\
		unsigned int index = map.Find(key);	\
		if (index != map.InvalidIndex())	\
			val = map.Element(index)


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView()
{
	Init( -1 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView( CEconItemView const &other )
{
	Init( -1 );
	*this = other;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView( int iItemID )
{
	Init( iItemID );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemView::Init( int iItemID )
{
	SetItemDefIndex( iItemID );
	m_AttributeList.Init();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemView::SetItemDefIndex( int iItemID )
{
	m_iItemDefinitionIndex = iItemID;
	//m_pItemDef = GetItemSchema()->GetItemDefinition( m_iItemDefinitionIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemView::GetItemDefIndex( void ) const
{
	return m_iItemDefinitionIndex;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemView::SetItemQuality( int nQuality )
{
	m_iEntityQuality = nQuality;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemView::GetItemQuality( void ) const
{
	return m_iEntityQuality;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemView::SetItemLevel( int nLevel )
{
	m_iEntityLevel = nLevel;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemView::GetItemLevel( void ) const
{
	return m_iEntityLevel;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemDefinition *CEconItemView::GetStaticData( void ) const
{
	return GetItemSchema()->GetItemDefinition( m_iItemDefinitionIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemView::GetWorldDisplayModel( int iClass/* = 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();
	const char *pszModelName = NULL;

	if ( pStatic )
	{
		pszModelName = pStatic->GetWorldModel();

		// Assuming we're using same model for both 1st person and 3rd person view.
		if ( !pszModelName && pStatic->attach_to_hands == 1 )
			pszModelName = pStatic->GetPlayerModel();
		
		if ( pStatic->GetPerClassModel( iClass ) )
			pszModelName = pStatic->GetPerClassModel( iClass );
	}

	return pszModelName;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemView::GetPlayerDisplayModel( int iClass/* = 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		if ( pStatic->GetPerClassModel( iClass ) )
			return pStatic->GetPerClassModel( iClass );

		return pStatic->GetPlayerModel();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CEconItemView::GetEntityName()
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		Assert( pStatic->GetClassName() );
		return pStatic->GetClassName();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemView::IsCosmetic()
{
	bool result = false;
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		FIND_ELEMENT( pStatic->tags, "is_cosmetic", result );
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemView::GetAnimationSlot( void )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		return pStatic->anim_slot;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemView::GetItemSlot( void )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		return pStatic->item_slot;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CEconItemView::GetActivityOverride( int iTeamNumber, Activity actOriginalActivity )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		int iOverridenActivity = ACT_INVALID;

		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( iTeamNumber );
		if( pVisuals )
		{
			FIND_ELEMENT( pVisuals->animation_replacement, actOriginalActivity, iOverridenActivity );
		}

		if ( iOverridenActivity != ACT_INVALID )
			return (Activity)iOverridenActivity;
	}

	return actOriginalActivity;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemView::GetActivityOverride( int iTeamNumber, const char *name )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		int iOriginalAct = ActivityList_IndexForName( name );
		int iOverridenAct = ACT_INVALID;

		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( iTeamNumber );
		if( pVisuals )
		{
			FIND_ELEMENT( pVisuals->animation_replacement, iOriginalAct, iOverridenAct );
		}

		if ( iOverridenAct != ACT_INVALID )
			return ActivityList_NameForIndex( iOverridenAct );
	}

	return name;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemView::GetSoundOverride( int iIndex, int iTeamNum /*= 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( iTeamNum );
		if( pVisuals )
			return pVisuals->GetWeaponShootSound( iIndex );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemView::GetCustomSound( int iIndex, int iTeamNum /*= 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( iTeamNum );
		if( pVisuals )
			return pVisuals->GetCustomWeaponSound( iIndex );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CEconItemView::GetModifiedRGBValue( bool bAlternate )
{
	static CSchemaAttributeHandle pAttrDef_Paint( "set item tint rgb" );
	static CSchemaAttributeHandle pAttrDef_Paint2( "set item tint rgb 2" );

	const int BCompatTeamColor = 1;

	float flPaintRGB = 0, flPaintRGBAlt = 0;
	if ( pAttrDef_Paint )
	{
		if ( FindAttribute<uint32>( this, pAttrDef_Paint, &flPaintRGB ) )
		{
			if ( (uint32)flPaintRGB == BCompatTeamColor )
				return bAlternate ? 0x005885A2 : 0x00B8383B;

			if ( pAttrDef_Paint2 )
			{
				FindAttribute<uint32>( this, pAttrDef_Paint2, &flPaintRGBAlt );
			}
		}
	}

	return bAlternate ? (uint32)flPaintRGBAlt : (uint32)flPaintRGB;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemView::GetSkin( int iTeamNum, bool bViewmodel ) const
{
	if (iTeamNum <= TF_TEAM_COUNT)
	{
		//if !GetStaticData->GetVisuals( iTeamNum )->style.IsEmpty()
		PerTeamVisuals_t *pVisuals = GetStaticData()->GetVisuals( iTeamNum );
		if (pVisuals)
			return pVisuals->skin;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemView::HasCapability( const char* name )
{
	bool result = false;
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		FIND_ELEMENT( pStatic->capabilities, name, result );
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemView::HasTag( const char* name )
{
	bool result = false;
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		FIND_ELEMENT( pStatic->tags, name, result );
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemView::AddAttribute( CEconItemAttribute *pAttribute )
{
	// Make sure this attribute exists.
	CEconAttributeDefinition const *pAttribDef = pAttribute->GetStaticData();
	if ( pAttribDef )
		return m_AttributeList.SetRuntimeAttributeValue( pAttribDef, pAttribute->m_flValue );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemView::SkipBaseAttributes( bool bSkip )
{
	m_bOnlyIterateItemViewAttributes = bSkip;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemView::IterateAttributes( IEconAttributeIterator *iter ) const
{
	m_AttributeList.IterateAttributes( iter );

	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic && !m_bOnlyIterateItemViewAttributes )
	{
		pStatic->IterateAttributes( iter );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemView::GetExtraWearableModel( void ) const
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		// We have an extra wearable
		return pStatic->GetExtraWearableModel();
	}

	return "\0";
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemView &CEconItemView::operator=( CEconItemView const &rhs )
{
	m_iItemDefinitionIndex = rhs.m_iItemDefinitionIndex;
	m_iEntityQuality = rhs.m_iEntityQuality;
	m_iEntityLevel = rhs.m_iEntityLevel;
	m_iTeamNumber = rhs.m_iTeamNumber;
	m_bOnlyIterateItemViewAttributes = rhs.m_bOnlyIterateItemViewAttributes;

	m_AttributeList = rhs.m_AttributeList;

#ifdef GAME_DLL
	m_iClassNumber = rhs.m_iClassNumber;
#endif

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CAttributeList::CAttributeList()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::Init( void )
{
	m_Attributes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemAttribute const *CAttributeList::GetAttribByID( int iNum )
{
	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData()->index == iNum )
			return &m_Attributes[i];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemAttribute const *CAttributeList::GetAttribByName( char const *szName )
{
	CEconAttributeDefinition *pDefinition = GetItemSchema()->GetAttributeDefinitionByName( szName );

	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData()->index == pDefinition->index )
			return &m_Attributes[i];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::IterateAttributes( IEconAttributeIterator *iter ) const
{
	FOR_EACH_VEC( m_Attributes, i )
	{
		CEconAttributeDefinition const *pDefinition = m_Attributes[i].GetStaticData();
		if ( pDefinition == nullptr )
			continue;

		attrib_data_union_t value;
		value.flVal = m_Attributes[i].m_flValue;
		if ( !pDefinition->type->OnIterateAttributeValue( iter, pDefinition, value ) )
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAttributeList::SetRuntimeAttributeValue( const CEconAttributeDefinition *pDefinition, float flValue )
{
	Assert( pDefinition );
	if ( pDefinition == nullptr )
		return false;

	FOR_EACH_VEC( m_Attributes, i )
	{
		CEconItemAttribute *pAttrib = &m_Attributes[i];
		if ( pAttrib->GetStaticData() == pDefinition )
		{
			pAttrib->m_flValue = flValue;
			m_pManager->OnAttributesChanged();
			return true;
		}
	}

	CEconItemAttribute attrib( pDefinition->index, flValue );
	m_Attributes[ m_Attributes.AddToTail() ] = attrib;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAttributeList::RemoveAttribute( const CEconAttributeDefinition *pDefinition )
{
	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData() == pDefinition )
		{
			m_Attributes.Remove( i );
			m_pManager->OnAttributesChanged();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAttributeList::RemoveAttribByIndex( int iIndex )
{
	if( iIndex == m_Attributes.InvalidIndex() || iIndex > m_Attributes.Count() )
		return false;

	m_Attributes.Remove( iIndex );
	m_pManager->OnAttributesChanged();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Clear out dynamic attributes
//-----------------------------------------------------------------------------
void CAttributeList::RemoveAllAttributes( void )
{
	if( !m_Attributes.IsEmpty() )
	{
		m_Attributes.Purge();
		m_pManager->OnAttributesChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::SetRuntimeAttributeRefundableCurrency( CEconAttributeDefinition const *pAttrib, int iRefundableCurrency )
{
	for ( int i = 0; i < m_Attributes.Count(); i++ )
	{
		CEconItemAttribute *pAttribute = &m_Attributes[i];

		if ( pAttribute->m_iAttributeDefinitionIndex == pAttrib->index )
		{
			// Found existing attribute -- change value.
			pAttribute->m_nRefundableCurrency = iRefundableCurrency;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAttributeList::GetRuntimeAttributeRefundableCurrency( CEconAttributeDefinition const *pAttrib ) const
{
	for ( int i = 0; i < m_Attributes.Count(); i++ )
	{
		CEconItemAttribute const &pAttribute = m_Attributes[i];

		if ( pAttribute.m_iAttributeDefinitionIndex == pAttrib->index )
			return pAttribute.m_nRefundableCurrency;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::SetManager( CAttributeManager *pManager )
{
	m_pManager = pManager;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::NotifyManagerOfAttributeValueChanges( void )
{
	m_pManager->OnAttributesChanged();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeList::operator=( CAttributeList const &rhs )
{
	m_Attributes = rhs.m_Attributes;

	m_pManager = nullptr;
}
