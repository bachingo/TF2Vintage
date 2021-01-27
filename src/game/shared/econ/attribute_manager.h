#ifndef ATTRIBUTE_MANAGER_H
#define ATTRIBUTE_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "gamestringpool.h"
#include "econ_item_interface.h"

typedef CUtlVector< CHandle<CBaseEntity> > ProviderVector;

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

#define CALL_ATTRIB_HOOK(type, caller, value, name, items) \
			value = CAttributeManager::AttribHookValue<type>(value, #name, caller, items)

#define CALL_ATTRIB_HOOK_INT(value, name)                       CALL_ATTRIB_HOOK(int, this, value, name, NULL)
#define CALL_ATTRIB_HOOK_INT_LIST(value, name, items)           CALL_ATTRIB_HOOK(int, this, value, name, items)
#define CALL_ATTRIB_HOOK_FLOAT(value, name)                     CALL_ATTRIB_HOOK(float, this, value, name, NULL)
#define CALL_ATTRIB_HOOK_FLOAT_LIST(value, name, items)         CALL_ATTRIB_HOOK(float, this, value, name, items)
#define CALL_ATTRIB_HOOK_STRING(value, name)                    CALL_ATTRIB_HOOK(string_t, this, value, name, NULL)
#define CALL_ATTRIB_HOOK_STRING_LIST(value, name, items)        CALL_ATTRIB_HOOK(string_t, this, value, name, items)
#define CALL_ATTRIB_HOOK_INT_ON_OTHER(ent, value, name)                 CALL_ATTRIB_HOOK(int, ent, value, name, NULL)
#define CALL_ATTRIB_HOOK_INT_ON_OTHER_LIST(ent, value, name, items)     CALL_ATTRIB_HOOK(int, ent, value, name, items)
#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(ent, value, name)               CALL_ATTRIB_HOOK(float, ent, value, name, NULL)
#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER_LIST(ent, value, name, items)   CALL_ATTRIB_HOOK(float, ent, value, name, items)
#define CALL_ATTRIB_HOOK_STRING_ON_OTHER(ent, value, name)              CALL_ATTRIB_HOOK(string_t, ent, value, name, NULL)
#define CALL_ATTRIB_HOOK_STRING_ON_OTHER_LIST(ent, value, name, items)  CALL_ATTRIB_HOOK(string_t, ent, value, name, items)

inline IHasAttributes *GetAttribInterface( CBaseEntity const *pEntity )
{
	if ( pEntity == nullptr )
		return nullptr;

	IHasAttributes *pInterface = pEntity->GetHasAttributesInterfacePtr();
	Assert( dynamic_cast<IHasAttributes const *>( pEntity ) == pInterface );
	return pInterface;
}

template<typename T>
inline T AttributeConvertFromFloat( float flValue )
{
	return (T)flValue;
}

template<>
inline float AttributeConvertFromFloat( float flValue )
{
	return flValue;
}

template<>
inline int AttributeConvertFromFloat( float flValue )
{
	return RoundFloatToInt( flValue );
}


typedef enum
{
	PROVIDER_ANY,
	PROVIDER_WEAPON
} provider_type_t;

class CAttributeManager
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();
	DECLARE_CLASS_NOBASE( CAttributeManager );

	CAttributeManager();
	virtual ~CAttributeManager();

	template <typename T>
	static T AttribHookValue( T inValue, const char* text, const CBaseEntity *pEntity, CUtlVector<EHANDLE> *pOutList = NULL )
	{
		IHasAttributes *pAttribInteface = GetAttribInterface( pEntity );
		AssertMsg( pAttribInteface, "What are you doing trying to get an attribute of something that doesn't have an interface?" );
		if ( pAttribInteface )
		{
			string_t strAttributeClass = AllocPooledString_StaticConstantStringPointer( text );
			
			T outValue;
			TypedAttribHookValue( outValue, inValue, strAttributeClass, pEntity, pAttribInteface, pOutList );
			return outValue;
		}

		return inValue;
	}

#ifdef CLIENT_DLL
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updatetype );
#endif
	void			AddProvider( CBaseEntity *pEntity );
	void			RemoveProvider( CBaseEntity *pEntity );
	void			AddReceiver( CBaseEntity *pEntity );
	void			RemoveReceiver( CBaseEntity *pEntity );
	void			ProvideTo( CBaseEntity *pEntity );
	void			StopProvidingTo( CBaseEntity *pEntity );
	int				GetProviderType( void ) const { return m_ProviderType; }
	void			SetProvidrType( int type ) { m_ProviderType = type; }
	bool			IsProvidingTo( CBaseEntity *pEntity ) const;
	bool			IsBeingProvidedToBy( CBaseEntity *pEntity ) const;
	virtual void	InitializeAttributes( CBaseEntity *pEntity );
	virtual float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );
	virtual string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );

	virtual void	OnAttributesChanged( void )
	{
		ClearCache();
	}

protected:
	CNetworkHandleForDerived( CBaseEntity, m_hOuter );
	bool m_bParsingMyself;

	CNetworkVarForDerived( int, m_iReapplyProvisionParity );
#ifdef CLIENT_DLL
	int m_iOldReapplyProvisionParity;
#endif
	CNetworkVarForDerived( int, m_ProviderType );

	CUtlVector<EHANDLE> m_AttributeProviders;
	CUtlVector<EHANDLE> m_AttributeReceivers;

	int m_iCacheVersion;

private:
	void	ClearCache();
	int		GetGlobalCacheVersion() const;

	virtual float	ApplyAttributeFloatWrapper( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );
	virtual string_t ApplyAttributeStringWrapper( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders = NULL );

	template<class T>
	static void TypedAttribHookValue( T &outPut, T const &value, string_t strAttributeClass, const CBaseEntity *pEntity, IHasAttributes *pAttribInteface, CUtlVector<EHANDLE> *pOutList )
	{
		float flResult = pAttribInteface->GetAttributeManager()->ApplyAttributeFloatWrapper( (float)value, pEntity, strAttributeClass, pOutList );
		outPut = AttributeConvertFromFloat<T>( flResult );
	}

	static void TypedAttribHookValue( string_t &outPut, string_t const &value, string_t strAttributeClass, const CBaseEntity *pEntity, IHasAttributes *pAttribInterface, CUtlVector<EHANDLE> *pOutList )
	{
		outPut = pAttribInterface->GetAttributeManager()->ApplyAttributeStringWrapper( value, pEntity, strAttributeClass, pOutList );
	}

	typedef union
	{
		string_t iVal;
		float fVal;

		operator string_t const &() const { return iVal; }
		operator float() const { return fVal; }
	} cached_attribute_value_t;
	struct cached_attribute_t
	{
		string_t	iAttribName;
		cached_attribute_value_t in;
		cached_attribute_value_t out;
	};
	CUtlVector<cached_attribute_t>	m_CachedAttribs;

	friend class CEconEntity;
};


class CAttributeContainer : public CAttributeManager
{
public:
	DECLARE_CLASS( CAttributeContainer, CAttributeManager );
#if defined( CLIENT_DLL )
	DECLARE_PREDICTABLE();
#endif
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();

	virtual ~CAttributeContainer() {};
	void	InitializeAttributes( CBaseEntity *pEntity );
	float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	void	OnAttributesChanged( void ) OVERRIDE;

	void SetItem( CEconItemView const &pItem ) { m_Item.CopyFrom( pItem ); }
	CEconItemView *GetItem( void ) { return &m_Item; }
	CEconItemView const *GetItem( void ) const { return &m_Item; }

protected:
	CNetworkVarEmbedded( CEconItemView, m_Item );
};


class CAttributeContainerPlayer : public CAttributeManager
{
public:
	DECLARE_CLASS( CAttributeContainerPlayer, CAttributeManager );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();

	float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	void	OnAttributesChanged( void ) OVERRIDE;

protected:
	CNetworkHandle( CBasePlayer, m_hPlayer );

#if defined( GAME_DLL )
	friend class CTFPlayer;
#endif
};

#endif // ATTRIBUTE_MANAGER_H
