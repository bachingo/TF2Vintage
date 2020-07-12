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
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned int ) { return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, float ) { return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, CAttribute_String const & ) { return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned long long const & ) { return true; }
};


template<typename TArg, typename TOut=TArg>
class CAttributeIterator_GetSpecificAttribute : public IEconAttributeIterator
{
public:
	CAttributeIterator_GetSpecificAttribute( CEconAttributeDefinition const *attribute, TOut *outValue )
		: m_pAttribute( attribute ), m_pOut( outValue )
	{
		m_bFound = false;
	}

	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned int ) { return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, float ) { return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, CAttribute_String const & ) { return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned long long const & ) { return true; }

	bool Found( void ) const { return m_bFound; }

private:
	CEconAttributeDefinition const *m_pAttribute;
	bool m_bFound;
	TOut *m_pOut;
};

#define DEFINE_ATTRIBUTE_ITERATOR( overrideParam, outputType ) \
	bool CAttributeIterator_GetSpecificAttribute<overrideParam, outputType>::OnIterateAttributeValue( CEconAttributeDefinition const *pDefinition, overrideParam value ) \
	{ \
		if ( m_pAttribute == pDefinition ) \
		{ \
			m_bFound = true; \
			*m_pOut = *reinterpret_cast<const overrideParam *>( &value ); \
		} \
		return !m_bFound; \
	}

#define DEFINE_ATTRIBUTE_ITERATOR_REF( overrideParam, outputType ) \
	bool CAttributeIterator_GetSpecificAttribute<overrideParam, outputType>::OnIterateAttributeValue( CEconAttributeDefinition const *pDefinition, overrideParam const &value ) \
	{ \
		if ( m_pAttribute == pDefinition ) \
		{ \
			m_bFound = true; \
			*m_pOut = *reinterpret_cast<const overrideParam *>( &value ); \
		} \
		return !m_bFound; \
	}

#define ATTRIBUTE_ITERATOR( paramType, outputType, overrideParam ) \
	bool CAttributeIterator_GetSpecificAttribute<paramType, outputType>::OnIterateAttributeValue( CEconAttributeDefinition const *pDefinition, overrideParam value )

DEFINE_ATTRIBUTE_ITERATOR( float, float )
DEFINE_ATTRIBUTE_ITERATOR( unsigned int, unsigned int )
DEFINE_ATTRIBUTE_ITERATOR( float, unsigned int )
DEFINE_ATTRIBUTE_ITERATOR( unsigned int, float )
DEFINE_ATTRIBUTE_ITERATOR_REF( CAttribute_String, CAttribute_String )
DEFINE_ATTRIBUTE_ITERATOR_REF( uint64, uint64 )

// Client specific.
#ifdef CLIENT_DLL
	EXTERN_RECV_TABLE( DT_AttributeManager );
	EXTERN_RECV_TABLE( DT_AttributeContainer );
	EXTERN_RECV_TABLE( DT_AttributeContainerPlayer );
// Server specific.
#else
	EXTERN_SEND_TABLE( DT_AttributeManager );
	EXTERN_SEND_TABLE( DT_AttributeContainer );
	EXTERN_SEND_TABLE( DT_AttributeContainerPlayer );
#endif

inline IHasAttributes *GetAttribInterface( CBaseEntity const *pEntity )
{
	if ( pEntity == nullptr )
		return nullptr;

	IHasAttributes *pInteface = pEntity->GetHasAttributesInterfacePtr();
	if( pInteface )
	{
		Assert( dynamic_cast<IHasAttributes *>( (CBaseEntity *)pEntity ) == pInteface );
		return pInteface;
	}

	return nullptr;
}

template<typename T>
inline T AttributeConvertFromFloat( float flValue )
{
	return flValue;
}

template<>
inline int AttributeConvertFromFloat( float flValue )
{
	return RoundFloatToInt( flValue );
}

FORCEINLINE void ApplyAttribute( CEconAttributeDefinition const *pDefinition, float *pOutput, float flValue )
{
	switch ( pDefinition->description_format )
	{
		case ATTRIB_FORMAT_ADDITIVE:
		case ATTRIB_FORMAT_ADDITIVE_PERCENTAGE:
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
		IHasAttributes *pAttribInteface = GetAttribInterface( pEntity );

		if ( pAttribInteface )
		{
			string_t strAttributeClass = AllocPooledString_StaticConstantStringPointer( text );
			float flResult = pAttribInteface->GetAttributeManager()->ApplyAttributeFloat( (float)value, pEntity, strAttributeClass, pOutList );
			value = AttributeConvertFromFloat<type>( flResult );
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
	IHasAttributes *pAttribInteface = GetAttribInterface( pEntity );

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
#if defined( CLIENT_DLL )
	DECLARE_PREDICTABLE();
#endif
	DECLARE_EMBEDDED_NETWORKVAR();

	virtual ~CAttributeContainer() {};
	void	InitializeAttributes( CBaseEntity *pEntity );
	float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	void	OnAttributesChanged( void );

	CEconItemView *GetItem( void ) const { return (CEconItemView *)&m_Item; }

protected:
	CNetworkVarEmbedded( CEconItemView, m_Item );

	friend class CEconEntity;
};


class CAttributeContainerPlayer : public CAttributeManager
{
public:
	DECLARE_CLASS( CAttributeContainerPlayer, CAttributeManager );
	DECLARE_EMBEDDED_NETWORKVAR();

	float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders ) override;
	string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders ) override;
	void	OnAttributesChanged( void ) override;

protected:
	CNetworkHandle( CBasePlayer, m_hPlayer );

#if defined( GAME_DLL )
	friend class CTFPlayer;
#endif
};

#endif // ATTRIBUTE_MANAGER_H
