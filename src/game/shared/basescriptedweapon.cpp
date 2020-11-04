#include "cbase.h"
#include "npcevent.h"
#include "basescriptedweapon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( BaseScriptedWeapon, DT_BaseScriptedWeapon )

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CBaseScriptedWeapon )
END_PREDICTION_DATA()
#endif

BEGIN_NETWORK_TABLE( CBaseScriptedWeapon, DT_BaseScriptedWeapon )
END_NETWORK_TABLE()

BEGIN_ENT_SCRIPTDESC( CBaseScriptedWeapon, CBaseCombatWeapon, "The base for all custom scripted weapons" )
	DEFINE_SCRIPTFUNC( IsWeaponVisible, "" )
#if defined( GAME_DLL )
	DEFINE_SCRIPTFUNC( SetWeaponVisible, "" )
#endif
END_SCRIPTDESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseScriptedWeapon::CBaseScriptedWeapon()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseScriptedWeapon::~CBaseScriptedWeapon()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Precache()
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Spawn()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::PrimaryAttack()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::SecondaryAttack()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::CanDeploy( void )
{
	bool bResult = true;
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::Deploy( void )
{
	bool bResult = BaseClass::Deploy();
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::CanHolster( void )
{
	bool bResult = true;
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	bool bResult = BaseClass::Holster( pSwitchingTo );
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::OnActiveStateChanged( int iOldState )
{
	BaseClass::OnActiveStateChanged( iOldState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Detach()
{
	BaseClass::Detach();
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::HandleAnimEvent( pEvent );
}
#endif