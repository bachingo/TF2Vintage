#ifndef ECON_ITEM_INTERFACE_H
#define ECON_ITEM_INTERFACE_H

#ifdef _WIN32
#pragma once
#endif

class ISchemaAttributeType;
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
	CAttributeIterator_GetSpecificAttribute( CEconAttributeDefinition const *attribute, TOut *outValue )
		: m_pAttribute( attribute ), m_pOut( outValue )
	{
		m_bFound = false;
	}

	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned int )					{ return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, float )							{ return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, CAttribute_String const & )		{ return true; }
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, uint64 const & )				{ return true; }

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

template<typename T>
class CAttributeIterator_HasAttribute : public IEconUntypedAttributeIterator
{
	DECLARE_CLASS( CAttributeIterator_HasAttribute, IEconUntypedAttributeIterator );
public:
	CAttributeIterator_HasAttribute(CEconAttributeDefinition *pAttribute)
		: m_pDefinition(pAttribute), m_bFound(false) {}

	bool Found( void ) const { return m_bFound; }

private:
	virtual bool OnIterateAttributeValueUntyped( CEconAttributeDefinition const *pDefinition )
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

#endif
