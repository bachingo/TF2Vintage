#include "cbase.h"
#include "npcevent.h"
#include "ammodef.h"
#include "weapon_parse.h"
#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	#include "tf_weaponbase.h"
#endif
#include "basescriptedweapon.h"

LINK_ENTITY_TO_CLASS( weapon_scripted, CBaseScriptedWeapon );

BEGIN_DATADESC( CBaseScriptedWeapon )
	DEFINE_AUTO_ARRAY( m_szWeaponScriptName, FIELD_CHARACTER )
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( BaseScriptedWeapon, DT_BaseScriptedWeapon )

BEGIN_PREDICTION_DATA( CBaseScriptedWeapon )
END_PREDICTION_DATA()

BEGIN_NETWORK_TABLE( CBaseScriptedWeapon, DT_BaseScriptedWeapon )
#if defined(CLIENT_DLL)
	RecvPropString( RECVINFO( m_szWeaponScriptName ) ),
#else
	SendPropString( SENDINFO( m_szWeaponScriptName ) ),
#endif
END_NETWORK_TABLE()

#define DEFINE_SIMPLE_WEAPON_HOOK(hook, returnType, description)	DEFINE_SIMPLE_SCRIPTHOOK(hook, #hook, returnType, description)
#define BEGIN_WEAPON_HOOK(hook, returnType, description)			BEGIN_SCRIPTHOOK(hook, #hook, returnType, description)

BEGIN_ENT_SCRIPTDESC( CBaseScriptedWeapon, CBaseCombatWeapon, "The base for all custom scripted weapons" )
	DEFINE_SIMPLE_WEAPON_HOOK( HasAnyAmmo, FIELD_BOOLEAN, "Should return true if weapon has ammo" )
	DEFINE_SIMPLE_WEAPON_HOOK( HasPrimaryAmmo, FIELD_BOOLEAN, "Should return true if weapon has primary ammo" )
	DEFINE_SIMPLE_WEAPON_HOOK( HasSecondaryAmmo, FIELD_BOOLEAN, "Should return true if weapon has secondary ammo" )

	DEFINE_SIMPLE_WEAPON_HOOK( CanHolster, FIELD_BOOLEAN, "Should return true if weapon can be holstered" )
	DEFINE_SIMPLE_WEAPON_HOOK( CanDeploy, FIELD_BOOLEAN, "Should return true if weapon can be deployed" )
	DEFINE_SIMPLE_WEAPON_HOOK( Deploy, FIELD_BOOLEAN, "Called when weapon is being deployed" )
	BEGIN_WEAPON_HOOK( Holster, FIELD_BOOLEAN, "Called when weapon is being holstered" )
		DEFINE_SCRIPTHOOK_PARAM( switchingto, FIELD_HSCRIPT )
	END_SCRIPTHOOK()

	DEFINE_SIMPLE_WEAPON_HOOK( ItemPreFrame, FIELD_VOID, "Called each frame by the player PreThink" )
	DEFINE_SIMPLE_WEAPON_HOOK( ItemPostFrame, FIELD_VOID, "Called each frame by the player PostThink" )
	DEFINE_SIMPLE_WEAPON_HOOK( ItemBusyFrame, FIELD_VOID, "Called each frame by the player PostThink, if the player's not ready to attack yet" )
	DEFINE_SIMPLE_WEAPON_HOOK( ItemHolsterFrame, FIELD_VOID, "Called each frame by the player PreThink, if the weapon is holstered" )
	DEFINE_SIMPLE_WEAPON_HOOK( WeaponIdle, FIELD_VOID, "Called when no buttons pressed" )
	DEFINE_SIMPLE_WEAPON_HOOK( HandleFireOnEmpty, FIELD_VOID, "Called when they have the attack button down but they are out of ammo. The default implementation either reloads, switches weapons, or plays an empty sound." )

	DEFINE_SIMPLE_WEAPON_HOOK( CheckReload, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( FinishReload, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( AbortReload, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( Reload, FIELD_BOOLEAN, "" )

	DEFINE_SIMPLE_WEAPON_HOOK( PrimaryAttack, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( SecondaryAttack, FIELD_VOID, "" )

	DEFINE_SIMPLE_WEAPON_HOOK( GetPrimaryAttackActivity, FIELD_VARIANT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetSecondaryAttackActivity, FIELD_VARIANT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetDrawActivity, FIELD_VARIANT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetDefaultAnimSpeed, FIELD_FLOAT, "" )

	DEFINE_SIMPLE_WEAPON_HOOK( AddViewKick, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( ActivityList, FIELD_HSCRIPT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( ActivityListCount, FIELD_INTEGER, "" )
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

#if defined( GAME_DLL )
	// Setup our script ID
	ValidateScriptScope();

	m_iszVScripts = AllocPooledString( GetWeaponScriptName() );

	m_ScriptScope.Init( STRING( m_iszScriptId ) );
	VScriptRunScript( STRING( m_iszVScripts ), m_ScriptScope, true );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Spawn()
{
#if defined( USES_ECON_ITEMS )
	InitializeAttributes();
#endif

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	m_ScriptScope.SetValue( "owner", ToHScript( pOwner ) );

	m_ScriptScope.CallFunc( "Equip" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Detach()
{
	BaseClass::Detach();

	m_ScriptScope.CallFunc( "Detach" );

	m_ScriptScope.SetValue( "owner", INVALID_HSCRIPT );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SIMPLE_VOID_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.Get<bool>() == false) \
		return;

#define SIMPLE_BOOL_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.GetType() == FIELD_BOOLEAN) \
		return retVal;

#define SIMPLE_FLOAT_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.GetType() == FIELD_FLOAT) \
		return retVal;

#define SIMPLE_INT_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.GetType() == FIELD_INTEGER) \
		return retVal;

#define SIMPLE_VECTOR_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.GetType() == FIELD_VECTOR) \
		return retVal;

#define SIMPLE_VECTOR_REF_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.GetType() == FIELD_VECTOR) \
	{ \
		static Vector vec = retVal; \
		return vec; \
	}

bool CBaseScriptedWeapon::HasAnyAmmo( void )
{
	SIMPLE_BOOL_OVERRIDE( HasAnyAmmo );

	return BaseClass::HasAnyAmmo();
}

bool CBaseScriptedWeapon::HasPrimaryAmmo( void )
{
	SIMPLE_BOOL_OVERRIDE( HasPrimaryAmmo );

	return BaseClass::HasPrimaryAmmo();
}

bool CBaseScriptedWeapon::HasSecondaryAmmo( void )
{
	SIMPLE_BOOL_OVERRIDE( HasSecondaryAmmo );

	return BaseClass::HasSecondaryAmmo();
}

bool CBaseScriptedWeapon::CanHolster( void ) const
{
	SIMPLE_BOOL_OVERRIDE( CanHolster );

	return BaseClass::CanHolster();
}

bool CBaseScriptedWeapon::CanDeploy( void )
{
	SIMPLE_BOOL_OVERRIDE( CanDeploy );

	return BaseClass::CanDeploy();
}

bool CBaseScriptedWeapon::Deploy( void )
{
	SIMPLE_BOOL_OVERRIDE( Deploy );

	return BaseClass::Deploy();
}

bool CBaseScriptedWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_ScriptScope.PushArg( ToHScript( pSwitchingTo ) );
	SIMPLE_BOOL_OVERRIDE( Holster );

	return BaseClass::Holster( pSwitchingTo );
}

void CBaseScriptedWeapon::ItemPreFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemPreFrame );

	BaseClass::ItemPostFrame();
}

void CBaseScriptedWeapon::ItemPostFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemPostFrame );

	BaseClass::ItemPostFrame();
}

void CBaseScriptedWeapon::ItemBusyFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemBusyFrame );

	BaseClass::ItemBusyFrame();
}

void CBaseScriptedWeapon::ItemHolsterFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemHolsterFrame );

	BaseClass::ItemHolsterFrame();
}

void CBaseScriptedWeapon::WeaponIdle( void )
{
	SIMPLE_VOID_OVERRIDE( WeaponIdle );

	BaseClass::WeaponIdle();
}

void CBaseScriptedWeapon::HandleFireOnEmpty( void )
{
	SIMPLE_VOID_OVERRIDE( HandleFireOnEmpty );

	BaseClass::HandleFireOnEmpty();
}

void CBaseScriptedWeapon::CheckReload( void )
{
	SIMPLE_VOID_OVERRIDE( CheckReload );

	BaseClass::CheckReload();
}

void CBaseScriptedWeapon::FinishReload( void )
{
	SIMPLE_VOID_OVERRIDE( FinishReload );

	BaseClass::FinishReload();
}

void CBaseScriptedWeapon::AbortReload( void )
{
	SIMPLE_VOID_OVERRIDE( AbortReload );

	BaseClass::AbortReload();
}

bool CBaseScriptedWeapon::Reload( void )
{
	SIMPLE_BOOL_OVERRIDE( Reload );

	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::PrimaryAttack( void )
{
	SIMPLE_VOID_OVERRIDE( PrimaryAttack );

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::SecondaryAttack( void )
{
	SIMPLE_VOID_OVERRIDE( SecondaryAttack );

	BaseClass::SecondaryAttack();
}

#define ACTIVITY_FUNC_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR) \
	{ \
		if (retVal.GetType() == FIELD_INTEGER) \
		{ \
			Activity activity = (Activity)retVal.Get<int>(); \
			if (activity != ACT_INVALID) \
				return activity; \
		} \
		else if (retVal.GetType() == FIELD_CSTRING) \
		{ \
			Activity activity = (Activity)LookupActivity( retVal ); \
			if (activity != ACT_INVALID) \
				return activity; \
		} \
	}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CBaseScriptedWeapon::GetPrimaryAttackActivity( void )
{
	ACTIVITY_FUNC_OVERRIDE( GetPrimaryAttackActivity );

	return BaseClass::GetPrimaryAttackActivity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CBaseScriptedWeapon::GetSecondaryAttackActivity( void )
{
	ACTIVITY_FUNC_OVERRIDE( GetSecondaryAttackActivity );

	return BaseClass::GetSecondaryAttackActivity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CBaseScriptedWeapon::GetDrawActivity( void )
{
	ACTIVITY_FUNC_OVERRIDE( GetDrawActivity );

	return BaseClass::GetDrawActivity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseScriptedWeapon::GetDefaultAnimSpeed( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetDefaultAnimSpeed );

	return BaseClass::GetDefaultAnimSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector &CBaseScriptedWeapon::GetBulletSpread( void )
{
	SIMPLE_VECTOR_REF_OVERRIDE( GetBulletSpread );

	return BaseClass::GetBulletSpread();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseScriptedWeapon::GetFireRate( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetFireRate );

	return BaseClass::GetFireRate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseScriptedWeapon::GetMinBurst( void )
{
	SIMPLE_INT_OVERRIDE( GetMinBurst );

	return BaseClass::GetMinBurst();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseScriptedWeapon::GetMaxBurst( void )
{
	SIMPLE_INT_OVERRIDE( GetMaxBurst );

	return BaseClass::GetMaxBurst();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseScriptedWeapon::GetMinRestTime( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetMinRestTime );

	return BaseClass::GetMinRestTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseScriptedWeapon::GetMaxRestTime( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetMaxRestTime );

	return BaseClass::GetMaxRestTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::AddViewKick( void )
{
	SIMPLE_VOID_OVERRIDE( AddViewKick );

	BaseClass::AddViewKick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CBaseScriptedWeapon::GetWeaponScriptName()
{
#if defined( USES_ECON_ITEMS )
	// If we were Econ generated then setup our weapon name using it,
	// else rely on the mapper to name their entities correctly
	CEconItemDefinition *pItemDef = GetAttributeContainer()->GetItem()->GetStaticData();
	if ( pItemDef )
	{
		if ( pItemDef->GetVScriptName() && pItemDef->GetVScriptName()[0] )
			return pItemDef->GetVScriptName();
	}
#endif
	if ( m_szWeaponScriptName[0] != '\0' )
		return m_szWeaponScriptName;

	return GetClassname();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
acttable_t *CBaseScriptedWeapon::ActivityList( int &iActivityCount )
{
	// TODO
	iActivityCount = ActivityListCount();
	return BaseClass::ActivityList( iActivityCount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseScriptedWeapon::ActivityListCount( void ) const
{
	// TODO

	return 0;
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
}
#endif

