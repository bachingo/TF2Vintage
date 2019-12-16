#ifndef ATTRIBUTE_MANAGER_H
#define ATTRIBUTE_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "gamestringpool.h"

typedef CUtlVector< CHandle<CBaseEntity> > ProviderVector;

class CEconItemSpecificAttributeIterator : public IEconAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( EconAttributeDefinition const *, unsigned int ) { return true; }
	virtual bool OnIterateAttributeValue( EconAttributeDefinition const *, float ) { return true; }
	virtual bool OnIterateAttributeValue( EconAttributeDefinition const *, string_t const & ) { return true; }
};

extern ConVar tf2v_attrib_mult;
class CEconItemAttributeIterator_ApplyAttributeFloat : public CEconItemSpecificAttributeIterator
{
public:
	CEconItemAttributeIterator_ApplyAttributeFloat( EHANDLE hOwner, string_t iName, float *outValue, ProviderVector *outVector )
		: m_hOwner( hOwner ), m_iName( iName ), m_flOut( outValue ), m_pOutProviders( outVector ) {}

	virtual bool OnIterateAttributeValue( EconAttributeDefinition const *pDefinition, unsigned int value )
	{
		string_t name = pDefinition->m_iAttributeClass;
		if ( !name && pDefinition->attribute_class[ 0 ] )
		{
			name = AllocPooledString( pDefinition->attribute_class );
			const_cast<EconAttributeDefinition *>( pDefinition )->m_iAttributeClass = name;
		}

		// Pointer comparison, bad
		if ( m_iName == name )
		{
			if ( m_pOutProviders )
			{
				if ( m_pOutProviders->Find( m_hOwner ) == m_pOutProviders->InvalidIndex() )
					m_pOutProviders->AddToTail( m_hOwner );
			}

			switch ( pDefinition->description_format )
			{
				case ATTRIB_FORMAT_ADDITIVE:
				case ATTRIB_FORMAT_ADDITIVE_PERCENTAGE:
				{
					*m_flOut += BitsToFloat( value );
					break;
				}
				case ATTRIB_FORMAT_PERCENTAGE:
				case ATTRIB_FORMAT_INVERTED_PERCENTAGE:
				{
					*m_flOut *= BitsToFloat( value );
					break;
				}
				case ATTRIB_FORMAT_OR:
				{
					// Oh, man...
					int iValue = FloatBits( *m_flOut );
					iValue |= value;
					*m_flOut = BitsToFloat( iValue );
					break;
				}
				default:
				{
					*m_flOut = value;
					break;
				}
			}
		}
		*m_flOut *=	tf2v_attrib_mult.GetFloat();
		return true;
	}

private:
	EHANDLE m_hOwner;
	string_t m_iName;
	float *m_flOut;
	ProviderVector *m_pOutProviders;
};

class CEconItemAttributeIterator_ApplyAttributeString : public CEconItemSpecificAttributeIterator
{
public:
	CEconItemAttributeIterator_ApplyAttributeString( EHANDLE hOwner, string_t iName, string_t *outValue, ProviderVector *outVector )
		: m_hOwner( hOwner ), m_iName( iName ), m_pOut( outValue ), m_pOutProviders( outVector ) {}

	virtual bool OnIterateAttributeValue( EconAttributeDefinition const *pDefinition, unsigned int value )
	{
		attrib_data_union_t convert;
		convert.iVal = value;

		string_t name = pDefinition->m_iAttributeClass;
		if ( !name && pDefinition->attribute_class[ 0 ] )
		{
			name = AllocPooledString( pDefinition->attribute_class );
			const_cast<EconAttributeDefinition *>( pDefinition )->m_iAttributeClass = name;
		}

		// Pointer comparison, bad
		if ( m_iName == name )
		{
			if ( m_pOutProviders )
			{
				if ( m_pOutProviders->Find( m_hOwner ) == m_pOutProviders->InvalidIndex() )
					m_pOutProviders->AddToTail( m_hOwner );
			}

			*m_pOut = convert.sVal;
		}

		return true;
	}

private:
	EHANDLE m_hOwner;
	string_t m_iName;
	string_t *m_pOut;
	ProviderVector *m_pOutProviders;
};

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_AttributeManager );
EXTERN_RECV_TABLE( DT_AttributeContainer );
// Server specific.
#else
EXTERN_SEND_TABLE( DT_AttributeManager );
EXTERN_SEND_TABLE( DT_AttributeContainer );
#endif

class CAttributeManager
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CAttributeManager );

	CAttributeManager();
	virtual ~CAttributeManager();

	template <typename type>
	static type AttribHookValue( type value, const char* text, const CBaseEntity *pEntity, CUtlVector<EHANDLE> *pOutList = NULL )
	{
		if ( !pEntity )
			return value;

		IHasAttributes *pAttribInteface = pEntity->GetHasAttributesInterfacePtr();

		if ( pAttribInteface )
		{
			string_t strAttributeClass = AllocPooledString_StaticConstantStringPointer( text );
			float flResult = pAttribInteface->GetAttributeManager()->ApplyAttributeFloat( (float)value, pEntity, strAttributeClass, pOutList );
			value = (type)flResult;
		}

		return value;
	}

#ifdef CLIENT_DLL
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updatetype );
#endif
	void			AddProvider( CBaseEntity *pEntity );
	void			RemoveProvider( CBaseEntity *pEntity );
	void			ProvideTo( CBaseEntity *pEntity );
	void			StopProvidingTo( CBaseEntity *pEntity );
	virtual void	InitializeAttributes( CBaseEntity *pEntity );
	virtual float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	virtual string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	virtual void	OnAttributesChanged( void ) {}

protected:
	CNetworkHandle( CBaseEntity, m_hOuter );
	bool m_bParsingMyself;

	CNetworkVar( int, m_iReapplyProvisionParity );
#ifdef CLIENT_DLL
	int m_iOldReapplyProvisionParity;
#endif

	CUtlVector<EHANDLE> m_AttributeProviders;

	friend class CEconEntity;
};

template<>
inline string_t CAttributeManager::AttribHookValue<string_t>( string_t strValue, const char *text, const CBaseEntity *pEntity, CUtlVector<EHANDLE> *pOutList )
{
	if ( !pEntity )
		return strValue;

	IHasAttributes *pAttribInteface = pEntity->GetHasAttributesInterfacePtr();

	if ( pAttribInteface )
	{
		string_t strAttributeClass = AllocPooledString_StaticConstantStringPointer( text );
		strValue = pAttribInteface->GetAttributeManager()->ApplyAttributeString( strValue, pEntity, strAttributeClass, pOutList );
	}

	return strValue;
}


class CAttributeContainer : public CAttributeManager
{
public:
	DECLARE_CLASS( CAttributeContainer, CAttributeManager );
#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif
	DECLARE_EMBEDDED_NETWORKVAR();

	virtual ~CAttributeContainer() {};
	void	InitializeAttributes( CBaseEntity *pEntity );
	float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	void	OnAttributesChanged( void );

	CEconItemView *GetItem( void ) { return &m_Item; }

protected:
	CNetworkVarEmbedded( CEconItemView, m_Item );

	friend class CEconEntity;
};

#endif // ATTRIBUTE_MANAGER_H
