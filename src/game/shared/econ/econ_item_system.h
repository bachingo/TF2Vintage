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
	void Precache( void );

	CEconItemDefinition* GetItemDefinition( int id );
	CEconItemDefinition* GetItemDefinitionByName( const char* name );
	CEconAttributeDefinition *GetAttributeDefinition( int id );
	CEconAttributeDefinition *GetAttributeDefinitionByName( const char* name );
	CEconAttributeDefinition *GetAttributeDefinitionByClass( const char* name );
	int GetAttributeIndex( const char *classname );
	int GetAttributeType( const char *type ) const;

	KeyValues *m_pSchema;

protected:
	CUtlDict< int, unsigned short >					m_GameInfo;
	CUtlDict< EconQuality, unsigned short >			m_Qualities;
	CUtlDict< EconColor, unsigned short >			m_Colors;
	CUtlDict< KeyValues *, unsigned short >			m_PrefabsValues;
	CUtlMap< int, CEconItemDefinition * >			m_Items;
	CUtlMap< int, CEconAttributeDefinition * >		m_Attributes;

private:
	bool m_bInited;
};

template<class T>
class CSchemaFieldHandle
{
public:
	EXPLICIT CSchemaFieldHandle( char const *name );

	operator T const *();

private:
	char const *m_pName;
	T *m_pHandle;
	KeyValues *m_pSchema;
};

inline CSchemaFieldHandle<CEconItemDefinition>::operator const CEconItemDefinition *( )
{
	return m_pHandle;
}

CEconItemSchema *GetItemSchema();

#endif // ECON_ITEM_SYSTEM_H
