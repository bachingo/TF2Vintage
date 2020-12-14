#ifndef ECON_ITEM_INTERFACE_H
#define ECON_ITEM_INTERFACE_H

#ifdef _WIN32
#pragma once
#endif

class CEconAttributeDefinition;
class CAttribute_String;

class IEconAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned int ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, float ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, CAttribute_String const & ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, uint64 const & ) = 0;
};

class ISchemaAttributeType
{
public:
	virtual ~ISchemaAttributeType() {}
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool b1 = true ) const = 0;
	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t& value, char *pString ) const = 0;
	virtual void InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const = 0;
	virtual void UnloadEconAttributeValue( attrib_data_union_t *pValue ) const = 0;
	virtual bool OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t& value ) const = 0;
};

class CEconItemSpecificAttributeIterator : public IEconAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned int )					{ return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, float )							{ return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, CAttribute_String const & )		{ return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, uint64 const & )				{ return true; }
};


template<typename TArg, typename TOut=TArg>
class CAttributeIterator_GetSpecificAttribute : public IEconAttributeIterator
{
public:
	CAttributeIterator_GetSpecificAttribute( CEconAttributeDefinition const *pAttribute, TOut *outValue )
		: m_pAttribute( pAttribute ), m_pOut( outValue )
	{
		m_bFound = false;
	}

	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *pAttrDef, unsigned int value ) OVERRIDE
	{ 
		return OnIterateAttributeValueOfType( pAttrDef, value );
	}
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *pAttrDef, float value ) OVERRIDE
	{ 
		return OnIterateAttributeValueOfType( pAttrDef, value );
	}
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *pAttrDef, CAttribute_String const &value ) OVERRIDE
	{ 
		return OnIterateAttributeValueOfType( pAttrDef, value );
	}
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *pAttrDef, uint64 const &value ) OVERRIDE
	{ 
		return OnIterateAttributeValueOfType( pAttrDef, value );
	}

	bool OnIterateAttributeValueOfType( CEconAttributeDefinition const *pDefinition, TArg const &value )
	{
		COMPILE_TIME_ASSERT( sizeof( TArg ) == sizeof( TOut ) );

		if ( m_pAttribute == pDefinition )
		{
			m_bFound = true;
			*m_pOut = *reinterpret_cast<const TOut *>( &value );
		}
		return !m_bFound;
	}

	bool Found( void ) const { return m_bFound; }

private:
	// Catch type mismatch between our iterator functions and the types we are looking for
	template<typename TOther>
	bool OnIterateAttributeValueOfType( CEconAttributeDefinition const *pDefinition, TOther const &value )
	{
		// If you hit this assert, you've got a bad attribute value type resolution
		Assert( pDefinition && pDefinition != m_pAttribute );
		return true;
	}

	CEconAttributeDefinition const *m_pAttribute;
	bool m_bFound;
	TOut *m_pOut;
};

// Allow conversion between CAttribute_String to char pointer
template<>
inline bool CAttributeIterator_GetSpecificAttribute<CAttribute_String, char const *>::OnIterateAttributeValueOfType( CEconAttributeDefinition const *pDefinition, CAttribute_String const &value )
{
	if ( m_pAttribute == pDefinition )
	{
		m_bFound = true;
		*m_pOut = value.Get();
	}
	return !m_bFound;
}

class IEconUntypedAttributeIterator : public IEconAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( const CEconAttributeDefinition *pAttrDef, unsigned ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}

	virtual bool OnIterateAttributeValue( const CEconAttributeDefinition *pAttrDef, float ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}

	virtual bool OnIterateAttributeValue( const CEconAttributeDefinition *pAttrDef, const uint64& ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}

	virtual bool OnIterateAttributeValue( const CEconAttributeDefinition *pAttrDef, const CAttribute_String& ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}


private:
	virtual bool OnIterateAttributeValueUntyped( const CEconAttributeDefinition *pAttrDef ) = 0;
};

class CAttributeIterator_HasAttribute : public IEconUntypedAttributeIterator
{
public:
	CAttributeIterator_HasAttribute( CEconAttributeDefinition const *pAttribute )
		: m_pDefinition( pAttribute ), m_bFound( false ) {}

	bool Found( void ) const { return m_bFound; }

private:
	virtual bool OnIterateAttributeValueUntyped( CEconAttributeDefinition const *pDefinition ) OVERRIDE
	{
		if ( pDefinition == m_pDefinition )
		{
			m_bFound = true;
		}

		return !m_bFound;
	}


	CEconAttributeDefinition const *m_pDefinition;
	bool m_bFound;
};


template <typename TIterator>
bool FindAttribute( TIterator const *pAttributeIterator, CEconAttributeDefinition const *pAttrDef )
{
	if ( pAttrDef == NULL )
		return false;

	CAttributeIterator_HasAttribute func( pAttrDef );
	pAttributeIterator->IterateAttributes( &func );

	return func.Found();
}

template <typename TArg, typename TIterator, typename TOut>
bool FindAttribute( TIterator const *pAttributeIterator, CEconAttributeDefinition const *pAttrDef, TOut *pOutput )
{
	if ( pAttrDef == NULL )
		return false;

	CAttributeIterator_GetSpecificAttribute<TArg, TOut> func( pAttrDef, pOutput );
	pAttributeIterator->IterateAttributes( &func );

	return func.Found();
}

#endif
