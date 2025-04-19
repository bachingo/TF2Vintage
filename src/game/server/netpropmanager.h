//========= Copyright Valve Corporation, All rights reserved. ==============================//
//
// Purpose: Gets and sets SendTable/DataMap networked properties and caches results.
//
// Code contributions by and used with the permission of L4D2 modders:
// Neil Rao (neilrao42@gmail.com)
// Raymond Nondorf (rayman1103@aol.com)
//==========================================================================================//

#ifndef NETPROPMANAGER_H
#define NETPROPMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "dt_send.h"
#include "datamap.h"

// Gets and sets SendTable/DataMap netprops and caches results
class CNetPropManager
{
public:
	~CNetPropManager();

private:

	// This class manages both SendProps and DataMap props,
	// so we will need an enum to hold type info
	enum NetPropType
	{
		Type_Int = 0,
		Type_Float,
		Type_Vector,
		Type_VectorXY,
		Type_String,
		Type_Array,
		Type_DataTable,

	#ifdef SUPPORTS_INT64
		Type_Int64,
	#endif

		Type_String_t,
		Type_InvalidOrMax
	};

	// Holds prop information that is valid throughout the life of the engine
	struct PropInfo_t
	{
	public:
		int m_nOffset;			/**< A SendProp holds the offset from the "this" pointer of an entity */
		int m_nBitCount;		/**< Usually the number of bits to transmit. */
		int m_nTransFlags;		/**< Entity transmission flags */
		NetPropType m_eType;	/**< The type of prop this offset belongs to */
		int m_nElements;		/**< The number of elements */
		bool m_bIsSendProp;		/**< Is this a SendProp (if false, it is a DataMap) */
		bool m_IsPropValid;		/**< Is the prop data in the struct valid? */
		int m_nPropLen;			/**< The length of the prop (applies to strings) */
		int m_nProps;			/**< The number of props in an array */
	};

	// Searches the specified SendTable and returns the SendProp or NULL if it DNE
	SendProp *SearchSendTable( SendTable *pSendTable, const char *pstrProperty ) const;

	// Searches the data map and returns the offset
	typedescription_t *SearchDataMap( datamap_t *pMap, const char *pstrProperty ) const;

	// Searches a ServerClass's SendTable and datamap and returns pertinent prop info
	PropInfo_t GetEntityPropInfo( CBaseEntity *pBaseEntity, const char *pstrProperty, int element );


private:

	// Prop/offset dictionary
	typedef CUtlDict< PropInfo_t, int > PropInfoDict_t;
	CUtlDict< PropInfoDict_t*, int > m_PropCache;

public:
#define GetPropFunc(name, varType) \
			/* Gets an #varType netprop value for the provided entity */       \
			varType GetProp##name( HSCRIPT hEnt, char const *pszProperty )     \
			{                                                                  \
				return GetProp##name##Array( hEnt, pszProperty, 0 );           \
			}                                                                  \
			/* Gets an #varType netprop array value for the provided entity */ \
			varType GetProp##name##Array( HSCRIPT hEnt, char const *pszProperty, int element )
#define SetPropFunc(name, varType) \
			/* Sets an #varType netprop value for the provided entity */               \
			void SetProp##name( HSCRIPT hEnt, char const *pszProperty, varType value ) \
			{                                                                          \
				SetProp##name##Array( hEnt, pszProperty, value, 0 );                   \
			}                                                                          \
			/* Sets an #varType netprop value for the provided entity */               \
			void SetProp##name##Array( HSCRIPT hEnt, char const *pszProperty, varType value, int element )
	
	GetPropFunc( Int, int );
	SetPropFunc( Int, int );
	GetPropFunc( Float, float );
	SetPropFunc( Float, float );
	GetPropFunc( Vector, Vector );
	SetPropFunc( Vector, Vector );
	GetPropFunc( Entity, HSCRIPT );
	SetPropFunc( Entity, HSCRIPT );
	GetPropFunc( String, char const* );
	SetPropFunc( String, char const* );

	// Gets the size of a netprop array value for the provided entity
	int GetPropArraySize( HSCRIPT hEnt, const char *pszProperty )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEnt );
		if (!pBaseEntity)
			return -1;

		PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, 0 );

		if ( !propInfo.m_IsPropValid )
			return -1;

		return propInfo.m_nProps;
	}

	// Returns true if the netprop exists for the provided entity
	bool HasProp( HSCRIPT hEnt, const char *pszProperty )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEnt );
		if ( !pBaseEntity )
			return false;

		PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, 0 );

		return propInfo.m_IsPropValid;
	}
	
	// Gets a string of the type of netprop value for the provided entity
	const char *GetPropType( HSCRIPT hEnt, const char *pszProperty )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEnt );
		if ( !pBaseEntity )
			return NULL;

		PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, 0 );

		if ( !propInfo.m_IsPropValid )
			return NULL;

		switch ( propInfo.m_eType )
		{
			case Type_Int:
				return "integer";
			case Type_Float:
				return "float";
			case Type_Vector:
				return "vector";
			case Type_VectorXY:
				return "vector2d";
			case Type_String:
			case Type_String_t:
				return "string";
			case Type_Array:
				return "array";
			case Type_DataTable:
				return "table";
		#ifdef SUPPORTS_INT64
			case Type_Int64:
				return "integer64";
		#endif
		}

		return NULL;
	}
};


#endif // NETPROPMANAGER_H