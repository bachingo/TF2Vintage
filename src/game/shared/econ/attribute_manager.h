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


typedef enum
{
	PROVIDER_ANY,
	PROVIDER_WEAPON
} provider_type_t;

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
	int				GetProviderType( void ) const { return m_ProviderType; }
	void			SetProvidrType( int type ) { m_ProviderType = type; }
	virtual void	InitializeAttributes( CBaseEntity *pEntity );
	virtual float	ApplyAttributeFloat( float flValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );
	virtual string_t ApplyAttributeString( string_t strValue, const CBaseEntity *pEntity, string_t strAttributeClass, CUtlVector<EHANDLE> *pOutProviders );

	virtual void	OnAttributesChanged( void )
	{
		NetworkStateChanged();
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
