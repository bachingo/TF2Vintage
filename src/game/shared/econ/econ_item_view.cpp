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

#ifdef CLIENT_DLL
BEGIN_RECV_TABLE_NOBASE( CAttributeList, DT_AttributeList )
	RecvPropUtlVector( RECVINFO_UTLVECTOR( m_Attributes ), MAX_ATTRIBUTES_SENT, RecvPropDataTable( NULL, 0, 0, &REFERENCE_RECV_TABLE( DT_EconItemAttribute ) ) )
END_RECV_TABLE()
#else
BEGIN_SEND_TABLE_NOBASE( CAttributeList, DT_AttributeList )
	SendPropUtlVector( SENDINFO_UTLVECTOR( m_Attributes ), MAX_ATTRIBUTES_SENT, SendPropDataTable( NULL, 0, &REFERENCE_SEND_TABLE( DT_EconItemAttribute ) ) )
END_SEND_TABLE()
#endif

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

#define FIND_ELEMENT(map, key, val)			\
		unsigned int index = map.Find(key);	\
		if (index != map.InvalidIndex())	\
			val = map.Element(index)


CEconItemView::CEconItemView()
{
	Init( -1 );
}


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

CEconItemDefinition *CEconItemView::GetStaticData( void ) const
{
	return GetItemSchema()->GetItemDefinition( m_iItemDefinitionIndex );
}

const char *CEconItemView::GetWorldDisplayModel( int iClass/* = 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();
	const char *pszModelName = NULL;

	if ( pStatic )
	{
		pszModelName = pStatic->model_world;
		

		// Assuming we're using same model for both 1st person and 3rd person view.
		if ( !pszModelName[0] && pStatic->attach_to_hands == 1 )
		{
			pszModelName = pStatic->model_player;
		}
		if (pStatic->model_player_per_class[iClass][0] != '\0' && pStatic->attach_to_hands == 1)
			pszModelName = pStatic->model_player_per_class[iClass];
	}

	return pszModelName;
}

const char *CEconItemView::GetPlayerDisplayModel( int iClass/* = 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		if ( pStatic->model_player_per_class[iClass][0] != '\0' )
			return pStatic->model_player_per_class[iClass];

		return pStatic->model_player;
	}

	return NULL;
}

const char* CEconItemView::GetEntityName()
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		return pStatic->item_class;
	}

	return NULL;
}

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

int CEconItemView::GetAnimationSlot( void )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		return pStatic->anim_slot;
	}

	return -1;
}

int CEconItemView::GetItemSlot( void )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		return pStatic->item_slot;
	}

	return -1;
}

Activity CEconItemView::GetActivityOverride( int iTeamNumber, Activity actOriginalActivity )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		int iOverridenActivity = ACT_INVALID;

		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( iTeamNumber );
		FIND_ELEMENT( pVisuals->animation_replacement, actOriginalActivity, iOverridenActivity );

		if ( iOverridenActivity != ACT_INVALID )
			return (Activity)iOverridenActivity;
	}

	return actOriginalActivity;
}

const char *CEconItemView::GetActivityOverride( int iTeamNumber, const char *name )
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		int iOriginalAct = ActivityList_IndexForName( name );
		int iOverridenAct = ACT_INVALID;
		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( iTeamNumber );

		FIND_ELEMENT( pVisuals->animation_replacement, iOriginalAct, iOverridenAct );

		if ( iOverridenAct != ACT_INVALID )
			return ActivityList_NameForIndex( iOverridenAct );
	}

	return name;
}

const char *CEconItemView::GetSoundOverride( int iIndex, int iTeamNum /*= 0*/ ) const
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( iTeamNum );
		return pVisuals->aWeaponSounds[iIndex];
	}

	return NULL;
}

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

bool CEconItemView::AddAttribute( CEconItemAttribute *pAttribute )
{
	// Make sure this attribute exists.
	EconAttributeDefinition *pAttribDef = pAttribute->GetStaticData();
	if ( pAttribDef )
		return m_AttributeList.SetRuntimeAttributeValue( pAttribDef, BitsToFloat( pAttribute->m_iRawValue32 ) );

	return false;
}

void CEconItemView::SkipBaseAttributes( bool bSkip )
{
	m_bOnlyIterateItemViewAttributes = bSkip;
}

void CEconItemView::IterateAttributes( IEconAttributeIterator &iter )
{
	m_AttributeList.IterateAttributes( iter );

	CEconItemDefinition *pStatic = GetStaticData();
	if ( pStatic && !m_bOnlyIterateItemViewAttributes )
	{
		pStatic->IterateAttributes( iter );
	}
}

const char *CEconItemView::GetExtraWearableModel( void ) const
{
	CEconItemDefinition *pStatic = GetStaticData();

	if ( pStatic )
	{
		// We have an extra wearable
		return pStatic->extra_wearable;
	}

	return "\0";
}

CEconItemView &CEconItemView::operator=( CEconItemView const &rhs )
{
	m_iItemDefinitionIndex = rhs.m_iItemDefinitionIndex;
	m_iEntityQuality = rhs.m_iEntityQuality;
	m_iEntityLevel = rhs.m_iEntityLevel;
	m_iTeamNumber = rhs.m_iTeamNumber;
	m_AttributeList = rhs.m_AttributeList;

#ifdef GAME_DLL
	m_bOnlyIterateItemViewAttributes = rhs.m_bOnlyIterateItemViewAttributes;
	m_iClassNumber = rhs.m_iClassNumber;
#endif

	return *this;
}

CAttributeList::CAttributeList()
{
}

void CAttributeList::Init( void )
{
	m_Attributes.Purge();
}

CEconItemAttribute const *CAttributeList::GetAttribByID( int iNum )
{
	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData()->index == iNum )
			return &m_Attributes[i];
	}

	return nullptr;
}

CEconItemAttribute const *CAttributeList::GetAttribByName( char const *szName )
{
	EconAttributeDefinition *pDefinition = GetItemSchema()->GetAttributeDefinitionByName( szName );

	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( m_Attributes[i].GetStaticData()->index == pDefinition->index )
			return &m_Attributes[i];
	}

	return nullptr;
}

void CAttributeList::IterateAttributes( IEconAttributeIterator &iter )
{
	FOR_EACH_VEC( m_Attributes, i )
	{
		if ( !iter.OnIterateAttributeValue( m_Attributes[i].GetStaticData(), m_Attributes[i].m_iRawValue32 ) )
			return;
	}
}

bool CAttributeList::SetRuntimeAttributeValue( const EconAttributeDefinition *pDefinition, float flValue )
{
	Assert( pDefinition );
	if ( pDefinition == nullptr )
		return false;

	FOR_EACH_VEC( m_Attributes, i )
	{
		CEconItemAttribute *pAttrib = &m_Attributes[ i ];
		if ( pAttrib->GetStaticData() == pDefinition )
		{
			pAttrib->m_iRawValue32 = FloatBits( flValue );
			m_pManager->OnAttributesChanged();
			return true;
		}
	}

	CEconItemAttribute attrib( pDefinition->index, flValue );
	m_Attributes[ m_Attributes.AddToTail() ] = attrib;

	return true;
}

bool CAttributeList::RemoveAttribute( const EconAttributeDefinition *pDefinition )
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

bool CAttributeList::RemoveAttribByIndex( int iIndex )
{
	if( iIndex == m_Attributes.InvalidIndex() || iIndex > m_Attributes.Count() )
		return false;

	m_Attributes.Remove( iIndex );
	m_pManager->OnAttributesChanged();
	return true;
}

void CAttributeList::SetManager( CAttributeManager *pManager )
{
	m_pManager = pManager;
}

void CAttributeList::NotifyManagerOfAttributeValueChanges( void )
{
	m_pManager->OnAttributesChanged();
}

CAttributeList &CAttributeList::operator=( CAttributeList const &rhs )
{
	m_Attributes.RemoveAll();
	m_Attributes = rhs.m_Attributes;
	m_pManager = nullptr;
	return *this;
}
