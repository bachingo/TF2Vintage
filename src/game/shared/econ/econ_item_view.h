#ifndef TF_ECON_ITEM_VIEW_H
#define TF_ECON_ITEM_VIEW_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_item_schema.h"

class CAttributeManager;
class IEconAttributeIterator;

class CEconItemAttribute
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CEconItemAttribute );

	CEconAttributeDefinition *GetStaticData( void );
	CEconAttributeDefinition const *GetStaticData( void ) const;

	CEconItemAttribute()
	{
		Init( -1, 0.0f );
	}
	CEconItemAttribute( int iIndex, float flValue )
	{
		Init( iIndex, flValue );
	}
	CEconItemAttribute( int iIndex, uint32 unValue )
	{
		Init( iIndex, unValue );
	}
	CEconItemAttribute( CEconItemAttribute const &src );

	void Init( int iIndex, float flValue );
	void Init( int iIndex, uint32 unValue );

	CEconItemAttribute &operator=( CEconItemAttribute const &src );

public:
	CNetworkVar( uint16, m_iAttributeDefinitionIndex );
	CNetworkVar( float, m_flValue );
	CNetworkVar( int, m_nRefundableCurrency );
};

class CAttributeList
{
	DECLARE_CLASS_NOBASE( CAttributeList );
	DECLARE_EMBEDDED_NETWORKVAR();
public:
	DECLARE_DATADESC();
	CAttributeList();

	void Init( void );

	CEconItemAttribute const *GetAttribByID( int iNum );
	CEconItemAttribute const *GetAttribByName( char const *szName );
	void IterateAttributes( IEconAttributeIterator *iter ) const;

	bool SetRuntimeAttributeValue( const CEconAttributeDefinition *pAttrib, float flValue );
	bool RemoveAttribute( const CEconAttributeDefinition *pAttrib );
	bool RemoveAttribByIndex( int iIndex );
	void RemoveAllAttributes( void );

	void SetRuntimeAttributeRefundableCurrency( const CEconAttributeDefinition *pAttrib, int nRefundableCurrency );
	int	GetRuntimeAttributeRefundableCurrency( const CEconAttributeDefinition *pAttrib ) const;

	void SetManager( CAttributeManager *pManager );
	void NotifyManagerOfAttributeValueChanges( void );

	void operator=( CAttributeList const &rhs );
	
private:
	CUtlVector<CEconItemAttribute> m_Attributes;
	CAttributeManager *m_pManager;
};


class CEconItemView
{
public:
	DECLARE_CLASS_NOBASE( CEconItemView );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();

	CEconItemView();
	CEconItemView( CEconItemView const &other );
	CEconItemView( int iItemID );

	void Init( int iItemID );

	CEconItemDefinition *GetStaticData( void ) const;

	void OnAttributesChanged()
	{
		NetworkStateChanged();
	}

	const char* GetWorldDisplayModel( int iClass = 0 ) const;
	const char* GetPlayerDisplayModel( int iClass = 0 ) const;
	const char* GetEntityName( void );
	bool IsCosmetic( void );
	int GetAnimationSlot( void );
	int GetItemSlot( void );
	Activity GetActivityOverride( int iTeamNumber, Activity actOriginalActivity );
	const char* GetActivityOverride( int iTeamNumber, const char *name );
	const char* GetSoundOverride( int iIndex, int iTeamNum = TEAM_UNASSIGNED ) const;
	const char* GetCustomSound( int iIndex, int iTeamNum = TEAM_UNASSIGNED ) const;
	unsigned int GetModifiedRGBValue( bool bAlternate );
	int GetSkin( int iTeamNum, bool bViewmodel ) const;
	bool HasCapability( const char* name );
	bool HasTag( const char* name );

	bool AddAttribute( CEconItemAttribute *pAttribute );
	void SkipBaseAttributes( bool bSkip );
	void IterateAttributes( IEconAttributeIterator *iter ) const;
	CAttributeList *GetAttributeList( void ) { return &m_AttributeList; }
	CAttributeList const *GetAttributeList( void ) const { return &m_AttributeList; }

	void SetItemDefIndex( int iItemID );
	int GetItemDefIndex( void ) const;
	void SetItemQuality( int iQuality );
	int GetItemQuality( void ) const;
	void SetItemLevel( int nLevel );
	int GetItemLevel( void ) const;

	const char*	GetExtraWearableModel(void) const;

	CEconItemView &operator=( CEconItemView const &rhs );

private:
	CNetworkVar( short, m_iItemDefinitionIndex );

	CNetworkVar( int, m_iEntityQuality );
	CNetworkVar( int, m_iEntityLevel );

	CNetworkVar( int, m_iTeamNumber );
	
	CNetworkVar( bool, m_bOnlyIterateItemViewAttributes );

	CNetworkVarEmbedded( CAttributeList, m_AttributeList );

#ifdef GAME_DLL
public:

	void	SetItemClassNumber( int iClassNum ) { m_iClassNumber = iClassNum; }
	int		GetItemClassNumber() { return m_iClassNumber; }

private:

	// Randomizer needs class values for item view to give correct max ammo values
	int m_iClassNumber;
#endif
};

#endif // TF_ECON_ITEM_VIEW_H
