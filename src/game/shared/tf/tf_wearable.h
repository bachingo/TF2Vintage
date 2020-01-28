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

	virtual bool	IsViewModelWearable( void ) const { return false; }

#ifdef GAME_DLL
	virtual void	Equip( CBasePlayer *pPlayer );
	void			UpdateModelToClass( void );
	void			Break( void );
#else
	virtual int		InternalDrawModel( int flags );
	void			UpdateModelToClass(void);
#endif
};

class CTFWearableVM : public CTFWearable
{
	DECLARE_CLASS( CTFWearableVM, CTFWearable );
public:
	DECLARE_NETWORKCLASS();

	virtual bool	IsViewModelWearable( void ) const { return true; }
};

#endif // TF_WEARABLE_H
