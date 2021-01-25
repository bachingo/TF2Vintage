//=========== Copyright © 2018, LFE-Team, Not All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BUFF_ITEM_H
#define TF_WEAPON_BUFF_ITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#ifdef CLIENT_DLL
#include "GameEventListener.h"
#endif

#ifdef CLIENT_DLL
#define CTFBuffItem C_TFBuffItem
class C_TFBuffBanner;
#endif

//=============================================================================
//
// BUFF item class.
//
#ifdef GAME_DLL
class CTFBuffItem : public CTFWeaponBaseMelee
#else
class C_TFBuffItem : public C_TFWeaponBaseMelee, public CGameEventListener
#endif
{
public:

	DECLARE_CLASS( CTFBuffItem, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFBuffItem();
	~CTFBuffItem();
	virtual int			GetWeaponID( void ) const	{ return TF_WEAPON_BUFF_ITEM; }

	virtual void		Precache( void );

	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void		WeaponReset( void );
	virtual void		PrimaryAttack( void );

	int					GetBuffType( void );
	void				BlowHorn( void );
	void				RaiseFlag( void );
	bool				IsFull( void ) 				{ return GetEffectBarProgress() >= 1.0f; }

	virtual bool		HasChargeBar( void )		{ return true; }
	virtual const char* GetEffectLabelText( void )	{ return "#TF_Rage"; }
	virtual float		GetEffectBarProgress( void );
	virtual bool		EffectMeterShouldFlash( void );

	virtual bool		CanReload( void ) { return false; }

	virtual bool		SendWeaponAnim( int iActivity );

#ifdef CLIENT_DLL
	void				CreateBanner( int iBuffType );
	void				DestroyBanner( void );

	virtual void		FireGameEvent( IGameEvent *event );
#endif

private:

	CTFBuffItem( const CTFBuffItem & ) {}

	CNetworkVar( bool, m_bBuffUsed );

#ifdef CLIENT_DLL
	CHandle<C_TFBuffBanner> m_hBanner;
	friend class C_TFBuffBanner;
#endif
};

//Parachute base.

#if defined CLIENT_DLL
#define CTFParachute C_TFParachute
#define CTFParachute_Primary C_TFParachute_Primary
#define CTFParachute_Secondary C_TFParachute_Secondary
#endif

class CTFParachute : public CTFBuffItem
{
public:

	DECLARE_CLASS( CTFParachute, CTFBuffItem )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_PARACHUTE; }
	virtual bool	HasChargeBar( void )			{ return false; }

	virtual const char	*GetWorldModel() const;
	virtual void		Precache();
	virtual void	DeployParachute(void);
	virtual void	RetractParachute(void);
	
	virtual bool	IsOpened(void)		{ return m_iDeployed == 1; }
private:
	CNetworkVar( int, m_iDeployed );
};

class CTFParachute_Primary : public CTFParachute
{
public:
	DECLARE_CLASS( CTFParachute_Primary, CTFParachute );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

class CTFParachute_Secondary : public CTFParachute
{
public:
	DECLARE_CLASS( CTFParachute_Secondary, CTFParachute );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

#endif // TF_WEAPON_BUFF_ITEM_H