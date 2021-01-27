#ifndef ECON_ITEM_SYSTEM_H
#define ECON_ITEM_SYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlbuffer.h"

class ISchemaAttributeType;
class CEconItemDefinition;
class CEconAttributeDefinition;
struct EconQuality;
struct EconColor;


typedef struct
{
	CUtlConstString szName;
	ISchemaAttributeType *pType;
} attr_type_t;

typedef uint16 attrib_def_index_t;
typedef uint16 item_def_index_t;
#define INVALID_ATTRIBUTE_DEF_INDEX		((attrib_def_index_t)-1)
#define INVALID_ITEM_DEF_INDEX			((item_def_index_t)-1)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconItemSchema
{
	friend class CTFInventory;
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

	void ClientConnected( edict_t *pClient );
	void ClientDisconnected( edict_t *pClient );

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
	CUtlMap< char const *, KeyValues * >			m_PrefabsValues;
	CUtlMap< item_def_index_t, CEconItemDefinition * > m_Items;
	CUtlMap< attrib_def_index_t, CEconAttributeDefinition * > m_Attributes;
	CUtlVector< attr_type_t >						m_AttributeTypes;

private:
	KeyValues *m_pSchema;
	bool m_bInited;
	uint m_unSchemaResetCount;

	void ParseSchema( KeyValues *pKVData );
	void ParseGameInfo( KeyValues *pKVData );
	void ParseQualities( KeyValues *pKVData );
	void ParseColors( KeyValues *pKVData );
	void ParsePrefabs( KeyValues *pKVData );
	void ParseItems( KeyValues *pKVData );
	void ParseAttributes( KeyValues *pKVData );

	void MergeDefinitionPrefabs( KeyValues *pDefinition, KeyValues *pSchemeData );
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
template<>
inline const CEconAttributeDefinition *CSchemaFieldHandle<CEconAttributeDefinition>::GetHandleRef( void ) const
{
	return GetItemSchema()->GetAttributeDefinitionByName( m_pszName );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<>
inline const CEconItemDefinition *CSchemaFieldHandle<CEconItemDefinition>::GetHandleRef( void ) const
{
	return GetItemSchema()->GetItemDefinitionByName( m_pszName );
}

#endif // ECON_ITEM_SYSTEM_H
