#include "cbase.h"
#include "attribute_manager.h"
#include "econ_item_schema.h"

#ifdef CLIENT_DLL
#include "prediction.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ATTRIB_REAPPLY_PARITY_BITS 3

BEGIN_NETWORK_TABLE_NOBASE( CAttributeManager, DT_AttributeManager )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE();

ConVar tf2v_attrib_mult( "tf2v_attrib_mult", "1" , FCVAR_NOTIFY | FCVAR_REPLICATED, "Amount to multiply on attribute values." );

CAttributeManager::CAttributeManager()
{
	m_bParsingMyself = false;
	m_iReapplyProvisionParity = 0;
}

CAttributeManager::~CAttributeManager()
{
	m_AttributeProviders.Purge();
}

#ifdef CLIENT_DLL
void CAttributeManager::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_iOldReapplyProvisionParity = m_iReapplyProvisionParity;
}

void CAttributeManager::OnDataChanged( DataUpdateType_t updateType )
{
	// If parity ever falls out of sync we can catch up here.
	if ( m_iReapplyProvisionParity != m_iOldReapplyProvisionParity )
	{
		if ( m_hOuter )
		{
			IHasAttributes *pAttributes = m_hOuter->GetHasAttributesInterfacePtr();
			pAttributes->ReapplyProvision();
			m_iOldReapplyProvisionParity = m_iReapplyProvisionParity;
		}
	}
}

#endif

void CAttributeManager::AddProvider( CBaseEntity *pEntity )
{
	m_AttributeProviders.AddToTail( pEntity );
}

void CAttributeManager::RemoveProvider( CBaseEntity *pEntity )
{
	m_AttributeProviders.FindAndRemove( pEntity );
}

void CAttributeManager::ProvideTo( CBaseEntity *pEntity )
{
	if ( !pEntity || !m_hOuter.Get() )
		return;

	IHasAttributes *pAttributes = pEntity->GetHasAttributesInterfacePtr();

	if ( pAttributes )
	{
		pAttributes->GetAttributeManager()->AddProvider( m_hOuter.Get() );
	}

#ifdef CLIENT_DLL
	if ( prediction->InPrediction() )
#endif
	m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ( ( 1 << ATTRIB_REAPPLY_PARITY_BITS ) - 1 );
}

void CAttributeManager::StopProvidingTo( CBaseEntity *pEntity )
{
	if ( !pEntity || !m_hOuter.Get() )
		return;

	IHasAttributes *pAttributes = pEntity->GetHasAttributesInterfacePtr();

	if ( pAttributes )
	{
		pAttributes->GetAttributeManager()->RemoveProvider( m_hOuter.Get() );
	}

#ifdef CLIENT_DLL
	if ( prediction->InPrediction() )
#endif
	m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ( ( 1 << ATTRIB_REAPPLY_PARITY_BITS ) - 1 );
}

void CAttributeManager::InitializeAttributes( CBaseEntity *pEntity )
{
	Assert( pEntity->GetHasAttributesInterfacePtr() != NULL );

	m_hOuter.Set( pEntity );
	m_bParsingMyself = false;
}

float CAttributeManager::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return flValue;

	// Safeguard to prevent potential infinite loops.
	m_bParsingMyself = true;

	for ( int i = 0; i < m_AttributeProviders.Count(); i++ )
	{
		CBaseEntity *pProvider = m_AttributeProviders[i].Get();

		if ( !pProvider || pProvider == pEntity )
			continue;

		IHasAttributes *pAttributes = pProvider->GetHasAttributesInterfacePtr();

		if ( pAttributes )
		{
			flValue = pAttributes->GetAttributeManager()->ApplyAttributeFloat( flValue, pEntity, strAttributeClass, pOutProviders );
		}
	}

	IHasAttributes *pAttributes = m_hOuter->GetHasAttributesInterfacePtr();
	CBaseEntity *pOwner = pAttributes->GetAttributeOwner();

	if ( pOwner )
	{
		IHasAttributes *pOwnerAttrib = pOwner->GetHasAttributesInterfacePtr();
		if ( pOwnerAttrib )
		{
			flValue = pOwnerAttrib->GetAttributeManager()->ApplyAttributeFloat( flValue, pEntity, strAttributeClass, pOutProviders );
		}
	}

	m_bParsingMyself = false;

	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose: Search for an attribute on our providers.
//-----------------------------------------------------------------------------
string_t CAttributeManager::ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return strValue;

	// Safeguard to prevent potential infinite loops.
	m_bParsingMyself = true;

	for ( int i = 0; i < m_AttributeProviders.Count(); i++ )
	{
		CBaseEntity *pProvider = m_AttributeProviders[i].Get();
		if ( !pProvider || pProvider == pEntity )
			continue;

		IHasAttributes *pAttributes = pProvider->GetHasAttributesInterfacePtr();
		if ( pAttributes )
		{
			strValue = pAttributes->GetAttributeManager()->ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutProviders );
		}
	}

	IHasAttributes *pAttributes = m_hOuter->GetHasAttributesInterfacePtr();
	CBaseEntity *pOwner = pAttributes->GetAttributeOwner();

	if ( pOwner )
	{
		IHasAttributes *pOwnerAttrib = pOwner->GetHasAttributesInterfacePtr();
		if ( pOwnerAttrib )
		{
			strValue = pOwnerAttrib->GetAttributeManager()->ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutProviders );
		}
	}

	m_bParsingMyself = false;

	return strValue;
}


#if defined( CLIENT_DLL )
EXTERN_RECV_TABLE( DT_ScriptCreatedItem );
#else
EXTERN_SEND_TABLE( DT_ScriptCreatedItem );
#endif

BEGIN_NETWORK_TABLE_NOBASE( CAttributeContainer, DT_AttributeContainer )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
	RecvPropDataTable( RECVINFO_DT( m_Item ), 0, &REFERENCE_RECV_TABLE( DT_ScriptCreatedItem ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropDataTable( SENDINFO_DT( m_Item ), &REFERENCE_SEND_TABLE( DT_ScriptCreatedItem ) ),
#endif
END_NETWORK_TABLE();

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA_NO_BASE( CAttributeContainer )
	DEFINE_PRED_FIELD( m_iReapplyProvisionParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA();
#endif

void CAttributeContainer::InitializeAttributes( CBaseEntity *pEntity )
{
	BaseClass::InitializeAttributes( pEntity );
	m_Item.GetAttributeList()->SetManager( this );
}

float CAttributeContainer::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return flValue;

	m_bParsingMyself = true;

	CEconItemAttributeIterator_ApplyAttributeFloat func( m_hOuter.Get(), strAttributeClass, &flValue, pOutProviders );
	GetItem()->IterateAttributes( func );

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeFloat( flValue, pEntity, strAttributeClass, pOutProviders );
}

//-----------------------------------------------------------------------------
// Purpose: Search for an attribute and apply its value.
//-----------------------------------------------------------------------------
string_t CAttributeContainer::ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return strValue;

	m_bParsingMyself = true;

	CEconItemAttributeIterator_ApplyAttributeString func( m_hOuter.Get(), strAttributeClass, &strValue, pOutProviders );
	GetItem()->IterateAttributes( func );

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutProviders );
}

void CAttributeContainer::OnAttributesChanged( void )
{
	BaseClass::OnAttributesChanged();
}
