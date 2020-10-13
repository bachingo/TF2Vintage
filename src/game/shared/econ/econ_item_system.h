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
	friend class CEconSchemaParser; friend class CTFInventory;
	char const *const items_game = "scripts/items/items_game.txt";
public:
	CEconItemSchema();
	~CEconItemSchema();

	bool Init( void );
	void InitAttributeTypes( void );
	bool LoadFromFile( void );
	bool LoadFromBuffer( CUtlBuffer &buf );
	bool SaveToBuffer( CUtlBuffer &buf );
	void Precache( void );
	void Reset( void );

	CEconItemDefinition* GetItemDefinition( int id );
	CEconItemDefinition* GetItemDefinitionByName( const char* name );
	CEconAttributeDefinition *GetAttributeDefinition( int id );
	CEconAttributeDefinition *GetAttributeDefinitionByName( const char* name );
	CEconAttributeDefinition *GetAttributeDefinitionByClass( const char* name );
	ISchemaAttributeType *GetAttributeType( const char *type ) const;


	KeyValues *GetSchemaKeyValues( void ) const { return m_pSchema; }
	uint GetResetCount( void ) const { return m_unSchemaResetCount; }

protected:
	CUtlDict< int, unsigned short >					m_GameInfo;
	CUtlDict< EconQuality, unsigned short >			m_Qualities;
	CUtlDict< EconColor, unsigned short >			m_Colors;
	CUtlDict< KeyValues *, unsigned short >			m_PrefabsValues;
	CUtlMap< uint32, CEconItemDefinition * >			m_Items;
	CUtlMap< uint32, CEconAttributeDefinition * >		m_Attributes;
	CUtlVector< attr_type_t >						m_AttributeTypes;

private:
	KeyValues *m_pSchema;
	bool m_bInited;
	uint m_unSchemaResetCount;

	void ParseSchema( KeyValues *pKVData );
};

CEconItemSchema *GetItemSchema();

template<class T>
class CSchemaFieldHandle
{
public:
	EXPLICIT CSchemaFieldHandle( char const *name ) : m_pszName( name )
	{
		m_pHandle = GetHandleRef();
		m_nSchemaVersion = GetItemSchema()->GetResetCount();
	}

	operator T const *( ) const
	{
		uint nSchemaVersion = GetItemSchema()->GetResetCount();
		if ( nSchemaVersion != m_nSchemaVersion )
		{
			m_nSchemaVersion = nSchemaVersion;
			m_pHandle = GetHandleRef();
		}
		return m_pHandle;
	}
	T const *operator->() { return static_cast<const T *>( *this ); }

private:
	T const *GetHandleRef( void ) const;

	char const *m_pszName;
	mutable T const *m_pHandle;
	mutable uint m_nSchemaVersion;
};

typedef CSchemaFieldHandle<CEconAttributeDefinition>	CSchemaAttributeHandle;
typedef CSchemaFieldHandle<CEconItemDefinition>			CSchemaItemHandle;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const CEconAttributeDefinition *CSchemaFieldHandle<CEconAttributeDefinition>::GetHandleRef( void ) const
{
	return GetItemSchema()->GetAttributeDefinitionByName( m_pszName );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const CEconItemDefinition *CSchemaFieldHandle<CEconItemDefinition>::GetHandleRef( void ) const
{
	return GetItemSchema()->GetItemDefinitionByName( m_pszName );
}

#endif // ECON_ITEM_SYSTEM_H
