#include "cbase.h"
#include "vprof.h"
#include "econ_item_schema.h"
#include "attribute_manager.h"

#if defined( TF_VINTAGE ) || defined( TF_VINTAGE_CLIENT )
#include "tf_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ATTRIB_REAPPLY_PARITY_BITS 6
#define ATTRIB_REAPPLY_PARITY_MASK ( ( 1 << ATTRIB_REAPPLY_PARITY_BITS ) - 1 )

BEGIN_DATADESC_NO_BASE( CAttributeManager )
	DEFINE_FIELD( m_iReapplyProvisionParity, FIELD_INTEGER ),
	DEFINE_FIELD( m_hOuter, FIELD_EHANDLE ),
	DEFINE_FIELD( m_ProviderType, FIELD_INTEGER ),
END_DATADESC()

BEGIN_NETWORK_TABLE_NOBASE( CAttributeManager, DT_AttributeManager )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_ProviderType ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_ProviderType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE();

FORCEINLINE void ApplyAttribute( CEconAttributeDefinition const *pDefinition, float *pOutput, float flValue )
{
	switch ( pDefinition->description_format )
	{
		case ATTRIB_FORMAT_ADDITIVE:
		case ATTRIB_FORMAT_ADDITIVE_PERCENTAGE:
		case ATTRIB_FORMAT_PARTICLE_INDEX:
		{
			*pOutput += flValue;
			break;
		}
		case ATTRIB_FORMAT_PERCENTAGE:
		case ATTRIB_FORMAT_INVERTED_PERCENTAGE:
		{
			*pOutput *= flValue;
			break;
		}
		case ATTRIB_FORMAT_OR:
		{
			// Oh, man...
			int iValue = FloatBits( *pOutput );
			iValue |= FloatBits( flValue );
			*pOutput = BitsToFloat( iValue );
			break;
		}
		case ATTRIB_FORMAT_KILLSTREAKEFFECT_INDEX:
		case ATTRIB_FORMAT_KILLSTREAK_IDLEEFFECT_INDEX:
		case ATTRIB_FORMAT_FROM_LOOKUP_TABLE:
		{
			*pOutput = flValue;
			break;
		}
		default:
		{
			return;
		}
	}
}

class CAttributeIterator_ApplyAttributeFloat : public CEconItemSpecificAttributeIterator
{
public:
	CAttributeIterator_ApplyAttributeFloat( EHANDLE hOwner, string_t iName, float outValue, ProviderVector *outVector )
		: m_hOwner( hOwner ), m_iName( iName ), m_flOut( outValue ), m_pOutProviders( outVector ) {}

	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *pDefinition, unsigned int value )
	{
		string_t name = pDefinition->GetCachedClass();
		if ( !FStrEq( STRING( name ), STRING( m_iName ) ) )
			return true;

		if ( m_pOutProviders )
		{
			if ( m_pOutProviders->Find( m_hOwner ) == m_pOutProviders->InvalidIndex() )
				m_pOutProviders->AddToTail( m_hOwner );
		}

		ApplyAttribute( pDefinition, &m_flOut, BitsToFloat( value ) );

		return true;
	}

	operator float() { return m_flOut; }

private:
	EHANDLE m_hOwner;
	string_t m_iName;
	float m_flOut;
	ProviderVector *m_pOutProviders;
};

class CAttributeIterator_ApplyAttributeString : public CEconItemSpecificAttributeIterator
{
public:
	CAttributeIterator_ApplyAttributeString( EHANDLE hOwner, string_t iName, string_t outValue, ProviderVector *outVector )
		: m_hOwner( hOwner ), m_iName( iName ), m_pOut( outValue ), m_pOutProviders( outVector ) {}

	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *pDefinition, CAttribute_String const &value )
	{
		string_t name = pDefinition->GetCachedClass();
		if ( !FStrEq( STRING( name ), STRING( m_iName ) ) )
			return true;

		if ( m_pOutProviders )
		{
			if ( m_pOutProviders->Find( m_hOwner ) == m_pOutProviders->InvalidIndex() )
				m_pOutProviders->AddToTail( m_hOwner );
		}

		m_pOut = AllocPooledString( value );

		// Break off and only match once
		return false;
	}

	operator string_t &() { return m_pOut; }

private:
	EHANDLE m_hOwner;
	string_t m_iName;
	string_t m_pOut;
	ProviderVector *m_pOutProviders;
};



CAttributeManager::CAttributeManager()
{
	m_bParsingMyself = false;
	m_iReapplyProvisionParity = 0;
	m_ProviderType = PROVIDER_ANY;
}

CAttributeManager::~CAttributeManager()
{
	m_AttributeProviders.Purge();
	m_AttributeReceivers.Purge();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_iOldReapplyProvisionParity = m_iReapplyProvisionParity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::OnDataChanged( DataUpdateType_t updateType )
{
	// If parity ever falls out of sync we can catch up here.
	if ( m_iReapplyProvisionParity != m_iOldReapplyProvisionParity )
	{
		if ( m_hOuter )
		{
			IHasAttributes *pAttribInterface = GetAttribInterface( m_hOuter );
			if( pAttribInterface )
			{
				pAttribInterface->ReapplyProvision();
			}
			ClearCache();
			m_iOldReapplyProvisionParity = m_iReapplyProvisionParity;
		}
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Invalidate cache if needed
//-----------------------------------------------------------------------------
int	CAttributeManager::GetGlobalCacheVersion() const
{
#if defined( TF_VINTAGE ) || defined( TF_VINTAGE_CLIENT )
	return TFGameRules() ? TFGameRules()->GetGlobalAttributeCacheVersion() : 0;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::AddProvider( CBaseEntity *pEntity )
{
	IHasAttributes *pAttributes = GetAttribInterface( pEntity );
	Assert( pAttributes );

	m_AttributeProviders.AddToTail( pEntity );
	pAttributes->GetAttributeManager()->AddReceiver( m_hOuter.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::RemoveProvider( CBaseEntity *pEntity )
{
	IHasAttributes *pAttributes = GetAttribInterface( pEntity );
	Assert( pAttributes );

	m_AttributeProviders.FindAndFastRemove( pEntity );
	pAttributes->GetAttributeManager()->RemoveReceiver( m_hOuter.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::AddReceiver( CBaseEntity *pEntity )
{
	m_AttributeReceivers.AddToTail( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::RemoveReceiver( CBaseEntity *pEntity )
{
	m_AttributeReceivers.FindAndFastRemove( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::ProvideTo( CBaseEntity *pEntity )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( !pEntity || !m_hOuter.Get() )
		return;

	IHasAttributes *pAttribInterface = GetAttribInterface( pEntity );

	if ( pAttribInterface )
	{
		pAttribInterface->GetAttributeManager()->AddProvider( m_hOuter.Get() );
	#ifndef CLIENT_DLL
		m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ATTRIB_REAPPLY_PARITY_MASK;
	#endif

		NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::StopProvidingTo( CBaseEntity *pEntity )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( !pEntity || !m_hOuter.Get() )
		return;

	IHasAttributes *pAttribInterface = GetAttribInterface( pEntity );

	if ( pAttribInterface )
	{
		pAttribInterface->GetAttributeManager()->RemoveProvider( m_hOuter.Get() );
	#ifndef CLIENT_DLL
		m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ATTRIB_REAPPLY_PARITY_MASK;
	#endif

		NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAttributeManager::IsProvidingTo( CBaseEntity *pEntity ) const
{
	IHasAttributes *pAttribInterface = GetAttribInterface( pEntity );
	if ( pAttribInterface )
	{
		if ( pAttribInterface->GetAttributeManager()->IsBeingProvidedToBy( m_hOuter.Get() ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAttributeManager::IsBeingProvidedToBy( CBaseEntity *pEntity ) const
{
	return ( m_AttributeProviders.Find( pEntity ) != m_AttributeProviders.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::ClearCache( void )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bParsingMyself )
		return;

	m_CachedAttribs.Purge();

	m_bParsingMyself = true;

	// Tell the things we are providing to that they have been invalidated
	FOR_EACH_VEC( m_AttributeReceivers, i )
	{
		IHasAttributes *pAttribInterface = GetAttribInterface( m_AttributeReceivers[i].Get() );
		if ( pAttribInterface )
		{
			pAttribInterface->GetAttributeManager()->ClearCache();
		}
	}

	// Also our owner
	IHasAttributes *pMyAttribInterface = GetAttribInterface( m_hOuter.Get() );
	if ( pMyAttribInterface )
	{
		pMyAttribInterface->GetAttributeManager()->ClearCache();
	}

	m_bParsingMyself = false;

#ifndef CLIENT_DLL
	m_iReapplyProvisionParity = ( m_iReapplyProvisionParity + 1 ) & ATTRIB_REAPPLY_PARITY_MASK;
#endif

	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::InitializeAttributes( CBaseEntity *pEntity )
{
	Assert( pEntity->GetHasAttributesInterfacePtr() != NULL );

	m_hOuter.Set( pEntity );
	m_bParsingMyself = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CAttributeManager::ApplyAttributeFloatWrapper( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	const int iGlobalCacheVersion = GetGlobalCacheVersion();
	if ( m_iCacheVersion != iGlobalCacheVersion )
	{
		ClearCache();
		m_iCacheVersion = iGlobalCacheVersion;
	}

	if ( pOutProviders == NULL )
	{
		FOR_EACH_VEC_BACK( m_CachedAttribs, i )
		{
			if ( m_CachedAttribs[i].iAttribName == strAttributeClass )
			{
				if ( flValue == m_CachedAttribs[i].in )
					return m_CachedAttribs[i].out;

				// We are looking for another attribute of the same name,
				// remove this cache so we can get a different value
				m_CachedAttribs.Remove( i );
				break;
			}
		}
	}

	// Uncached, loop our attribs now
	float result = ApplyAttributeFloat( flValue, pEntity, strAttributeClass, pOutProviders );

	if ( pOutProviders == NULL )
	{
		// Cache it out
		int nCache = m_CachedAttribs.AddToTail();
		m_CachedAttribs[nCache].iAttribName = strAttributeClass;
		m_CachedAttribs[nCache].in.fVal = flValue;
		m_CachedAttribs[nCache].out.fVal = result;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CAttributeManager::ApplyAttributeStringWrapper( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	// Have we requested a global attribute cache flush?
	const int iGlobalCacheVersion = GetGlobalCacheVersion();
	if ( m_iCacheVersion != iGlobalCacheVersion )
	{
		ClearCache();
		m_iCacheVersion = iGlobalCacheVersion;
	}

	if ( pOutProviders == NULL )
	{
		FOR_EACH_VEC_BACK( m_CachedAttribs, i )
		{
			if ( m_CachedAttribs[i].iAttribName == strAttributeClass )
			{
				if ( strValue == m_CachedAttribs[i].in )
					return m_CachedAttribs[i].out;

				// We are looking for another attribute of the same name,
				// remove this cache so we can get a different value
				m_CachedAttribs.Remove( i );
				break;
			}
		}
	}

	// Uncached, loop our attribs now
	string_t result = ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutProviders );

	if ( pOutProviders == NULL )
	{
		// Cache it out
		int nCache = m_CachedAttribs.AddToTail();
		m_CachedAttribs[nCache].iAttribName = strAttributeClass;
		m_CachedAttribs[nCache].in.iVal = strValue;
		m_CachedAttribs[nCache].out.iVal = result;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CAttributeManager::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return flValue;

	// Safeguard to prevent potential infinite loops.
	m_bParsingMyself = true;

	IHasAttributes *pBaseAttributes = GetAttribInterface( pEntity );
	Assert( pBaseAttributes );

	for ( int i = 0; i < m_AttributeProviders.Count(); i++ )
	{
		CBaseEntity *pProvider = m_AttributeProviders[i].Get();
		if ( !pProvider || pProvider == pEntity )
			continue;

		IHasAttributes *pAttributes = GetAttribInterface( pProvider );
		Assert( pAttributes );

		// Weapons can't provide to eachother
		if ( pAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON &&
				pBaseAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON )
		{
			continue;
		}

		flValue = pAttributes->GetAttributeManager()->ApplyAttributeFloat( flValue, pEntity, strAttributeClass, pOutProviders );
	}

	IHasAttributes *pAttributes = GetAttribInterface( m_hOuter.Get() );
	CBaseEntity *pOwner = pAttributes->GetAttributeOwner();

	if ( pOwner )
	{
		IHasAttributes *pOwnerAttrib = GetAttribInterface( pOwner );
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
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return strValue;

	// Safeguard to prevent potential infinite loops.
	m_bParsingMyself = true;

	IHasAttributes *pBaseAttributes = GetAttribInterface( pEntity );
	Assert( pBaseAttributes );

	for ( int i = 0; i < m_AttributeProviders.Count(); i++ )
	{
		CBaseEntity *pProvider = m_AttributeProviders[i].Get();
		if ( !pProvider || pProvider == pEntity )
			continue;

		IHasAttributes *pAttributes = GetAttribInterface( pProvider );
		Assert( pAttributes );

		// Weapons can't provide to eachother
		if ( pAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON &&
				pBaseAttributes->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON )
		{
			continue;
		}

		strValue = pAttributes->GetAttributeManager()->ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutProviders );
	}

	IHasAttributes *pAttributes = GetAttribInterface( m_hOuter.Get() );
	CBaseEntity *pOwner = pAttributes->GetAttributeOwner();

	if ( pOwner )
	{
		IHasAttributes *pOwnerAttrib = GetAttribInterface( pOwner );
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

BEGIN_DATADESC( CAttributeContainer )
	DEFINE_EMBEDDED( m_Item ),
END_DATADESC()

BEGIN_NETWORK_TABLE_NOBASE( CAttributeContainer, DT_AttributeContainer )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_ProviderType ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
	RecvPropDataTable( RECVINFO_DT( m_Item ), 0, &REFERENCE_RECV_TABLE( DT_ScriptCreatedItem ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_ProviderType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropDataTable( SENDINFO_DT( m_Item ), &REFERENCE_SEND_TABLE( DT_ScriptCreatedItem ) ),
#endif
END_NETWORK_TABLE();

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA_NO_BASE( CAttributeContainer )
	DEFINE_PRED_FIELD( m_iReapplyProvisionParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA();
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeContainer::InitializeAttributes( CBaseEntity *pEntity )
{
	BaseClass::InitializeAttributes( pEntity );
	m_Item.GetAttributeList()->SetManager( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CAttributeContainer::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return flValue;

	m_bParsingMyself = true;

	CAttributeIterator_ApplyAttributeFloat func( m_hOuter.Get(), strAttributeClass, flValue, pOutProviders );
	GetItem()->IterateAttributes( &func );

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeFloat( func, pEntity, strAttributeClass, pOutProviders );
}

//-----------------------------------------------------------------------------
// Purpose: Search for an attribute and apply its value.
//-----------------------------------------------------------------------------
string_t CAttributeContainer::ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bParsingMyself || m_hOuter.Get() == NULL )
		return strValue;

	m_bParsingMyself = true;

	CAttributeIterator_ApplyAttributeString func( m_hOuter.Get(), strAttributeClass, strValue, pOutProviders );
	GetItem()->IterateAttributes( &func );

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeString( func, pEntity, strAttributeClass, pOutProviders );
}

void CAttributeContainer::OnAttributesChanged( void )
{
	BaseClass::OnAttributesChanged();
	m_Item.OnAttributesChanged();
}


BEGIN_DATADESC( CAttributeContainerPlayer )
END_DATADESC()

BEGIN_NETWORK_TABLE_NOBASE( CAttributeContainerPlayer, DT_AttributeContainerPlayer )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOuter ) ),
	RecvPropInt( RECVINFO( m_ProviderType ) ),
	RecvPropInt( RECVINFO( m_iReapplyProvisionParity ) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
#else
	SendPropEHandle( SENDINFO( m_hOuter ) ),
	SendPropInt( SENDINFO( m_ProviderType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iReapplyProvisionParity ), ATTRIB_REAPPLY_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
#endif
END_NETWORK_TABLE();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CAttributeContainerPlayer::ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bParsingMyself || m_hPlayer.Get() == NULL )
		return flValue;

	m_bParsingMyself = true;

	CAttributeIterator_ApplyAttributeFloat func( m_hPlayer.Get(), strAttributeClass, flValue, pOutProviders );
	m_hPlayer->m_AttributeList.IterateAttributes( &func );

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeFloat( func, pEntity, strAttributeClass, pOutProviders );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CAttributeContainerPlayer::ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, ProviderVector *pOutProviders )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bParsingMyself || m_hPlayer.Get() == NULL )
		return strValue;

	m_bParsingMyself = true;

	CAttributeIterator_ApplyAttributeString func( m_hPlayer.Get(), strAttributeClass, strValue, pOutProviders );
	m_hPlayer->m_AttributeList.IterateAttributes( &func );

	m_bParsingMyself = false;

	return BaseClass::ApplyAttributeString( func, pEntity, strAttributeClass, pOutProviders );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeContainerPlayer::OnAttributesChanged( void )
{
	BaseClass::OnAttributesChanged();
	if( m_hPlayer )
		m_hPlayer->NetworkStateChanged();
}
