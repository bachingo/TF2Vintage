#ifndef TF_ECON_ITEM_VIEW_H
#define TF_ECON_ITEM_VIEW_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_item_schema.h"

class CAttributeManager;
class IEconAttributeIterator;

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
	void IterateAttributes( IEconAttributeIterator *iter );

	bool SetRuntimeAttributeValue( const CEconAttributeDefinition *pAttrib, float flValue );
	bool RemoveAttribute( const CEconAttributeDefinition *pAttrib );
	bool RemoveAttribByIndex( int iIndex );

	void SetManager( CAttributeManager *pManager );
	void NotifyManagerOfAttributeValueChanges( void );

	CAttributeList &operator=( CAttributeList const &rhs );
	
private:
	CUtlVector<CEconItemAttribute> m_Attributes;
	CAttributeManager *m_pManager;
};


class CEconItemView
{
public:
	DECLARE_CLASS_NOBASE( CEconItemView );
	DECLARE_EMBEDDED_NETWORKVAR();

	CEconItemView();
	CEconItemView( CEconItemView const &other );
	CEconItemView( int iItemID );

	void Init( int iItemID );

	CEconItemDefinition *GetStaticData( void ) const;

	const char* GetWorldDisplayModel( int iClass = 0 ) const;
	const char* GetPlayerDisplayModel( int iClass = 0 ) const;
	const char* GetEntityName( void );
	bool IsCosmetic( void );
	int GetAnimationSlot( void );
	int GetItemSlot( void );
	Activity GetActivityOverride( int iTeamNumber, Activity actOriginalActivity );
	const char* GetActivityOverride( int iTeamNumber, const char *name );
	const char* GetSoundOverride( int iIndex, int iTeamNum = TEAM_UNASSIGNED ) const;
	unsigned int GetModifiedRGBValue( bool bAlternate );
	int GetSkin( int iTeamNum, bool bViewmodel ) const;
	bool HasCapability( const char* name );
	bool HasTag( const char* name );

	bool AddAttribute( CEconItemAttribute *pAttribute );
	void SkipBaseAttributes( bool bSkip );
	void IterateAttributes( IEconAttributeIterator *iter );
	CAttributeList *GetAttributeList( void ) { return &m_AttributeList; }

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
