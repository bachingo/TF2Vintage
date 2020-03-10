#ifndef ATTRIBUTE_TYPES_H
#define ATTRIBUTE_TYPES_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_item_schema.h"
#include "fmtstr.h"
#include <string>

class ISchemaAttributeType
{
public:
	virtual ~ISchemaAttributeType() {}
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool b1 = true ) const = 0;
	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t& value, std::string *pString ) const = 0;
	virtual void InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const = 0;
	virtual void UnloadEconAttributeValue( attrib_data_union_t *pValue ) const = 0;
	virtual bool OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t& value ) const = 0;
};

template<typename T>
class ISchemaAttributeTypeBase : public ISchemaAttributeType
{
public:
	virtual void ConvertTypedValueToByteStream(const T& value, std::string *pString) const = 0;
	virtual void ConvertByteStreamToTypedValue(const std::string& string, T *pValue) const = 0;
};

template<typename T>
class CSchemaAttributeTypeBase : public ISchemaAttributeTypeBase<T>
{
public:
	virtual void InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const {}
	virtual void UnloadEconAttributeValue( attrib_data_union_t *pValue ) const {}
	virtual bool OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value ) const { return true; }
};

inline void CSchemaAttributeTypeBase<unsigned int>::InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const
{
	pValue->iVal = 0;
}

inline void CSchemaAttributeTypeBase<unsigned int>::UnloadEconAttributeValue( attrib_data_union_t *pValue ) const
{
}

inline bool CSchemaAttributeTypeBase<unsigned int>::OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value ) const
{
	return pIterator->OnIterateAttributeValue( pAttrDef, value.iVal );
}

inline void CSchemaAttributeTypeBase<unsigned long long>::InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const
{
	pValue->lVal = new unsigned long long;
}

inline void CSchemaAttributeTypeBase<unsigned long long>::UnloadEconAttributeValue( attrib_data_union_t *pValue ) const
{
	if ( pValue->lVal )
		delete pValue->lVal;
}

inline bool CSchemaAttributeTypeBase<unsigned long long>::OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value ) const
{
	return pIterator->OnIterateAttributeValue( pAttrDef, *(value.lVal) );
}

inline void CSchemaAttributeTypeBase<float>::InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const
{
	pValue->iVal = 0;
}

inline void CSchemaAttributeTypeBase<float>::UnloadEconAttributeValue( attrib_data_union_t *pValue ) const
{
}

inline bool CSchemaAttributeTypeBase<float>::OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value ) const
{
	return pIterator->OnIterateAttributeValue( pAttrDef, value.flVal );
}

inline void CSchemaAttributeTypeBase<CAttribute_String>::InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const
{
	pValue->sVal = new CAttribute_String;
}

inline void CSchemaAttributeTypeBase<CAttribute_String>::UnloadEconAttributeValue( attrib_data_union_t *pValue ) const
{
	if ( pValue->sVal )
		delete pValue->sVal;
}

inline bool CSchemaAttributeTypeBase<CAttribute_String>::OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value ) const
{
	return pIterator->OnIterateAttributeValue( pAttrDef, *(value.sVal) );
}

class CSchemaAttributeType_Default : public CSchemaAttributeTypeBase<unsigned int>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		if ( bUnk )
		{
			double val = 0.0;
			if ( pString && pString[0] )
				val = strtod( pString, NULL );

			pValue->flVal = val;
		}
		else if ( pAttrDef->stored_as_integer )
		{
			pValue->iVal = V_atoi64( pString );
		}
		else
		{
			pValue->flVal = V_atof( pString );
		}

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, std::string *pString ) const
	{
		if ( pAttrDef->stored_as_integer )
		{
			pString->assign( CFmtStr( "%u", value.iVal ) );
		}
		else
		{
			pString->assign( CFmtStr( "%f", value.flVal ) );
		}
	}

	virtual void ConvertTypedValueToByteStream( const unsigned int &value, std::string *pString ) const
	{
		pString->assign( CFmtStr( "%u", value ) );
	}

	virtual void ConvertByteStreamToTypedValue( const std::string &string, unsigned int *pValue ) const
	{
		const_cast<std::string &>( string ).resize( sizeof( unsigned int ) );
		*pValue = *reinterpret_cast<const unsigned int *>( string.data() );
	}
};

class CSchemaAttributeType_UInt64 : public CSchemaAttributeTypeBase<unsigned long long>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		*(pValue->lVal) = V_atoui64( pString );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, std::string *pString ) const
	{
		pString->assign( CFmtStr( "%llu", *(value.lVal) ) );
	}

	virtual void ConvertTypedValueToByteStream( const unsigned long long &value, std::string *pString ) const
	{
		pString->assign( CFmtStr( "%llu", value ) );
	}

	virtual void ConvertByteStreamToTypedValue( const std::string &string, unsigned long long *pValue ) const
	{
		const_cast<std::string &>( string ).resize( sizeof( unsigned long long ) );
		*pValue = *reinterpret_cast<const unsigned long long *>( string.data() );
	}
};

class CSchemaAttributeType_Float : public CSchemaAttributeTypeBase<float>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		pValue->flVal = V_atof( pString );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, std::string *pString ) const
	{
		pString->assign( CFmtStr( "%f", value.flVal ) );
	}

	virtual void ConvertTypedValueToByteStream( const float &value, std::string *pString ) const
	{
		pString->assign( CFmtStr( "%f", value ) );
	}

	virtual void ConvertByteStreamToTypedValue( const std::string &string, float *pValue ) const
	{
		const_cast<std::string &>( string ).resize( sizeof( float ) );
		*pValue = *reinterpret_cast<const vec_t *>( string.data() );
	}
};

class CSchemaAttributeType_String : public CSchemaAttributeTypeBase<CAttribute_String>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		CAttribute_String tmp;
		tmp.Assign( pString );

		*(pValue->sVal) = tmp;

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, std::string *pString ) const
	{
		if ( pAttrDef->string_attribute )
		{
			pString->assign( *(value.sVal) );
		}
	}

	virtual void ConvertTypedValueToByteStream( const CAttribute_String &value, std::string *pString ) const
	{
		pString->assign( value );
	}

	virtual void ConvertByteStreamToTypedValue( const std::string &string, CAttribute_String *pValue ) const
	{
		CAttribute_String tmp;
		tmp.Assign( string.data() );

		*pValue = tmp;
	}
};


#endif
