#ifndef BASESCRIPTEDWEAPON_H
#define BASESCRIPTEDWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "basecombatweapon_shared.h"
#include "vscript/ivscript.h"
#include "tier1/utlhashtable.h"


#if defined( CLIENT_DLL )
#define CBaseScriptedWeapon C_BaseScriptedWeapon
#endif

#if defined(TF_VINTAGE) || defined(TF_VINTAGE_CLIENT)
#include "tf_weaponbase.h"
#define SCRIPTED_WEAPON_DERIVE_FROM CTFWeaponBase
#else
#define SCRIPTED_WEAPON_DERIVE_FROM CBaseCombatWeapon
#endif


class CScriptedWeaponScope : public CScriptScope
{
	typedef CUtlVectorFixed<ScriptVariant_t, 14> CScriptArgArray;
	typedef CUtlHashtable<char const *, CScriptFuncHolder> CScriptFuncMap;
public:
	~CScriptedWeaponScope() {
		FOR_EACH_VEC_BACK( m_vecPushedArgs, i )
		{
			m_vecPushedArgs[i].Free();
		}
		m_vecPushedArgs.Purge();

		Term();
	}

	IScriptVM *GetVM() {
		return CDefScriptScopeBase::GetVM();
	}

	template<class Arg>
	void PushArg( Arg const &arg )
	{
		// This is not at all the right method to do this, but pushing a
		// "void" type as a argument is an error anyway, c'est la vie
		COMPILE_TIME_ASSERT( ScriptDeduceType( Arg ) != FIELD_VOID );
		m_vecPushedArgs.AddToTail( arg );
	}
	template<class Arg, typename ...Rest>
	void PushArg( Arg const &arg, Rest const &...args )
	{
		COMPILE_TIME_ASSERT( ScriptDeduceType( Arg ) != FIELD_VOID );
		m_vecPushedArgs.AddToTail( arg );

		PushArg( args... );
	}

	template<typename T=int32>
	ScriptStatus_t CallFunc( char const *pszFuncName, T *pReturn = NULL )
	{
		// See if we used this function previously...
		uint nIndex = m_FuncMap.Find( pszFuncName );
		if ( nIndex == m_FuncMap.InvalidHandle() )
		{	// ...and cache it if not
			CScriptFuncHolder holder;
			HSCRIPT hFunction = GetVM()->LookupFunction( pszFuncName, m_hScope );
			if ( hFunction != INVALID_HSCRIPT )
			{
				holder.hFunction = hFunction;
				m_FuncHandles.AddToTail( &holder.hFunction );
				nIndex = m_FuncMap.Insert( pszFuncName, holder );
			}
		}
		
		ScriptVariant_t returnVal;
		ScriptStatus_t result = GetVM()->ExecuteFunction( m_FuncMap[nIndex].hFunction, 
														  m_vecPushedArgs.Base(), 
														  m_vecPushedArgs.Count(), 
														  pReturn ? &returnVal : NULL, 
														  m_hScope, true );
		if ( pReturn && result != SCRIPT_ERROR )
		{
			returnVal.AssignTo( pReturn );
		}

		returnVal.Free();
		m_vecPushedArgs.RemoveAll();

		return result;
	}

private:
	CScriptArgArray m_vecPushedArgs;
	CScriptFuncMap m_FuncMap;
};


class CBaseScriptedWeapon : public SCRIPTED_WEAPON_DERIVE_FROM
{
	DECLARE_CLASS( CBaseScriptedWeapon, SCRIPTED_WEAPON_DERIVE_FROM );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();
	DECLARE_ACTTABLE();

	CBaseScriptedWeapon();
	~CBaseScriptedWeapon();

	virtual void	Precache( void );
	virtual void	Spawn( void );

	virtual void	Equip( CBaseCombatCharacter *pOwner );
	virtual void	Detach( void );

	// Weapon selection
	bool			HasAnyAmmo( void ) OVERRIDE;						// Returns true is weapon has ammo
	bool			HasPrimaryAmmo( void ) OVERRIDE;					// Returns true is weapon has ammo
	bool			HasSecondaryAmmo( void ) OVERRIDE;				// Returns true is weapon has ammo

	bool			CanHolster( void ) const OVERRIDE;		// returns true if the weapon can be holstered
	bool			CanDeploy( void ) OVERRIDE;			// return true if the weapon's allowed to deploy
	bool			Deploy( void ) OVERRIDE;								// returns true is deploy was successful
	bool			Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;

	// Weapon behaviour
	void			ItemPreFrame( void ) OVERRIDE;					// called each frame by the player PreThink
	void			ItemPostFrame( void ) OVERRIDE;					// called each frame by the player PostThink
	void			ItemBusyFrame( void ) OVERRIDE;					// called each frame by the player PostThink, if the player's not ready to attack yet
	void			ItemHolsterFrame( void ) OVERRIDE;			// called each frame by the player PreThink, if the weapon is holstered
	void			WeaponIdle( void ) OVERRIDE;						// called when no buttons pressed
	void			HandleFireOnEmpty() OVERRIDE;					// Called when they have the attack button down

	// Reloading
	void			CheckReload( void ) OVERRIDE;
	void			FinishReload( void ) OVERRIDE;
	void			AbortReload( void ) OVERRIDE;
	bool			Reload( void ) OVERRIDE;

	// Weapon firing
	void			PrimaryAttack( void ) OVERRIDE;				// do "+ATTACK"
	void			SecondaryAttack( void ) OVERRIDE;			// do "+ATTACK2"

	// Firing animations
	Activity		GetPrimaryAttackActivity( void ) OVERRIDE;
	Activity		GetSecondaryAttackActivity( void ) OVERRIDE;
	Activity		GetDrawActivity( void ) OVERRIDE;
	float			GetDefaultAnimSpeed( void ) OVERRIDE;
	int				ActivityListCount( void ) const;

	// Bullet launch information
	const Vector&	GetBulletSpread( void ) OVERRIDE;
	float			GetFireRate( void ) OVERRIDE;
	int				GetMinBurst() OVERRIDE;
	int				GetMaxBurst() OVERRIDE;
	float			GetMinRestTime() OVERRIDE;
	float			GetMaxRestTime() OVERRIDE;

	void			AddViewKick( void ) OVERRIDE;

	char const*		GetWeaponScriptName( void ) OVERRIDE;

#if defined(CLIENT_DLL)
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
#endif

protected:
	CNetworkString( m_szWeaponScriptName, 64 );
	mutable CScriptedWeaponScope m_ScriptScope;
};

#endif
