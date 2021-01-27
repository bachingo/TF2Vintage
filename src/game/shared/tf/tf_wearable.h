//=============================================================================
//
// Purpose: Stub class for compatibility with item schema
//
//=============================================================================
#ifndef TF_WEARABLE_H
#define TF_WEARABLE_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_wearable.h"

#ifdef CLIENT_DLL
#define CTFWearable C_TFWearable
#define CTFWearableVM C_TFWearableVM
#endif

class CTFWearable : public CEconWearable
{
public:
	DECLARE_CLASS( CTFWearable, CEconWearable );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	virtual void	Equip( CBasePlayer *pPlayer );
	void			UpdateModelToClass( void );
	virtual void	UpdatePlayerBodygroups( int bOnOff );
	void			Break( void );
#else
	virtual int		InternalDrawModel( int flags );
	virtual int		GetWorldModelIndex( void );
	virtual void	ValidateModelIndex( void );
	void			UpdateModelToClass( void );
	virtual bool 	ShouldDraw( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
#endif

	virtual void	ReapplyProvision( void ) OVERRIDE;

	virtual void	SetExtraWearable( bool bExtraWearable ) { m_bExtraWearable = bExtraWearable; }
	virtual bool	IsExtraWearable( void ) { return m_bExtraWearable; }
	void			SetDisguiseWearable( bool bDisguiseWearable )	{ m_bDisguiseWearable = bDisguiseWearable; }
	bool			IsDisguiseWearable( void ) const				{ return m_bDisguiseWearable; }
	void			SetWeaponAssociatedWith( CBaseEntity *pWeapon )	{ m_hWeaponAssociatedWith = pWeapon; }
	CBaseEntity*	GetWeaponAssociatedWith( void ) const			{ return m_hWeaponAssociatedWith.Get(); }

private:
	CNetworkVar( bool, m_bExtraWearable );
	CNetworkVar( bool, m_bDisguiseWearable );
	CNetworkHandle( CBaseEntity, m_hWeaponAssociatedWith );
	short		m_nWorldModelIndex;
};

class CTFWearableVM : public CTFWearable
{
	DECLARE_CLASS( CTFWearableVM, CTFWearable );
public:
	DECLARE_NETWORKCLASS();

	virtual bool	IsViewModelWearable( void ) const { return true; }
};

#endif // TF_WEARABLE_H
