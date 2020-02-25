#ifndef ECON_ITEM_SYSTEM_H
#define ECON_ITEM_SYSTEM_H

#ifdef _WIN32
#pragma once
#endif


class ISchemaAttributeType;
class CEconSchemaParser;
class CEconItemDefinition;
class CEconAttributeDefinition;
struct EconQuality;
struct EconColor;

enum
{
	ATTRTYPE_INVALID = -1,
	ATTRTYPE_INT,
	ATTRTYPE_UINT64,
	ATTRTYPE_FLOAT,
	ATTRTYPE_STRING
};

typedef struct
{
	CUtlConstString szName;
	ISchemaAttributeType *pType;
} attr_type_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconItemSchema
{
	friend class CEconSchemaParser;
	friend class CTFInventory;
public:
	CEconItemSchema();
	~CEconItemSchema();

	bool Init( void );
	void InitAttributeTypes( void );
	void Precache( void );

	CEconItemDefinition* GetItemDefinition( int id );
	CEconItemDefinition* GetItemDefinitionByName( const char* name );
	CEconAttributeDefinition *GetAttributeDefinition( int id );
	CEconAttributeDefinition *GetAttributeDefinitionByName( const char* name );
	CEconAttributeDefinition *GetAttributeDefinitionByClass( const char* name );
	int GetAttributeIndex( const char *classname );
	ISchemaAttributeType *GetAttributeType( const char *type ) const;

	KeyValues *GetSchemaKeyValues( void ) const { return m_pSchema; }

protected:
	CUtlDict< int, unsigned short >					m_GameInfo;
	CUtlDict< EconQuality, unsigned short >			m_Qualities;
	CUtlDict< EconColor, unsigned short >			m_Colors;
	CUtlDict< KeyValues *, unsigned short >			m_PrefabsValues;
	CUtlMap< int, CEconItemDefinition * >			m_Items;
	CUtlMap< int, CEconAttributeDefinition * >		m_Attributes;
	CUtlVector< attr_type_t >						m_AttributeTypes;

private:
	KeyValues *m_pSchema;
	bool m_bInited;
};

CEconItemSchema *GetItemSchema();

template<class T>
class CSchemaFieldHandle
{
public:
	EXPLICIT CSchemaFieldHandle( char const *name );

	operator T const *();

private:
	char const *m_pName;
	T *m_pHandle;
	KeyValues *const m_pSchema;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CSchemaFieldHandle<CEconAttributeDefinition>::CSchemaFieldHandle( char const *name )
	: m_pName( name ), m_pSchema( GetItemSchema()->GetSchemaKeyValues() )
{
	m_pHandle = GetItemSchema()->GetAttributeDefinitionByName( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CSchemaFieldHandle<CEconAttributeDefinition>::operator const CEconAttributeDefinition *( )
{
	return m_pHandle;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CSchemaFieldHandle<CEconItemDefinition>::CSchemaFieldHandle( char const *name )
	: m_pName( name ), m_pSchema( GetItemSchema()->GetSchemaKeyValues() )
{
	m_pHandle = GetItemSchema()->GetItemDefinitionByName( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CSchemaFieldHandle<CEconItemDefinition>::operator const CEconItemDefinition *( )
{
	return m_pHandle;
}

#endif // ECON_ITEM_SYSTEM_H
