#include "cbase.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#if defined(GAME_DLL)
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif
#include "tf_viewmodel.h"
#include "tf_weapon_scripted.h"
#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_melee.h"

LINK_ENTITY_TO_CLASS( tf_weapon_scripted, CTFScriptedWeapon );

IMPLEMENT_NETWORKCLASS_ALIASED( TFScriptedWeapon, DT_TFWeaponScripted )
BEGIN_NETWORK_TABLE( CTFScriptedWeapon, DT_TFWeaponScripted )
END_NETWORK_TABLE();

BEGIN_PREDICTION_DATA( CTFScriptedWeapon )
END_PREDICTION_DATA();

#define DEFINE_SIMPLE_WEAPON_HOOK(hook, returnType, description)	DEFINE_SIMPLE_SCRIPTHOOK(hook, #hook, returnType, description)
#define BEGIN_WEAPON_HOOK(hook, returnType, description)			BEGIN_SCRIPTHOOK(hook, #hook, returnType, description)

BEGIN_ENT_SCRIPTDESC( CTFScriptedWeapon, CBaseScriptedWeapon, "" )
#if defined(GAME_DLL)
	BEGIN_WEAPON_HOOK( ApplyOnHitAttributes, FIELD_VOID, "" )
		DEFINE_SCRIPTHOOK_PARAM( victim, FIELD_HSCRIPT )
		DEFINE_SCRIPTHOOK_PARAM( attacker, FIELD_HSCRIPT )
		DEFINE_SCRIPTHOOK_PARAM( dmgInfo, FIELD_VARIANT )
	END_SCRIPTHOOK()
	BEGIN_WEAPON_HOOK( ApplyPostOnHitAttributes, FIELD_VOID, "" )
		DEFINE_SCRIPTHOOK_PARAM( dmgInfo, FIELD_VARIANT )
		DEFINE_SCRIPTHOOK_PARAM( victim, FIELD_HSCRIPT )
	END_SCRIPTHOOK()
#endif
END_SCRIPTDESC()

bool DamageInfoToVariant( ScriptVariant_t &variant, CTakeDamageInfo const &info )
{
	Assert( variant.Get<HSCRIPT>() == NULL );

	g_pScriptVM->CreateTable( variant );
	if ( variant.Get<HSCRIPT>() == NULL )
		return false;

	g_pScriptVM->SetValue( variant, "attacker", ToHScript( info.GetAttacker() ) );
	g_pScriptVM->SetValue( variant, "inflictor", ToHScript( info.GetInflictor() ) );
	g_pScriptVM->SetValue( variant, "weapon", ToHScript( info.GetWeapon() ) );
	g_pScriptVM->SetValue( variant, "damage", info.GetDamage() );
	g_pScriptVM->SetValue( variant, "maxDamage", info.GetMaxDamage() );
	g_pScriptVM->SetValue( variant, "baseDamage", info.GetBaseDamage() );
	g_pScriptVM->SetValue( variant, "damageBonus", info.GetDamageBonus() );
	g_pScriptVM->SetValue( variant, "damageBonusProvider", ToHScript( info.GetDamageBonusProvider() ) );
	g_pScriptVM->SetValue( variant, "damageForce", info.GetDamageForce() );
	g_pScriptVM->SetValue( variant, "damagePosition", info.GetDamagePosition() );
	g_pScriptVM->SetValue( variant, "reportedPosition", info.GetReportedPosition() );
	g_pScriptVM->SetValue( variant, "damageType", info.GetDamageType() );
	g_pScriptVM->SetValue( variant, "damageCustom", info.GetDamageCustom() );
	g_pScriptVM->SetValue( variant, "ammoType", info.GetAmmoType() );
	g_pScriptVM->SetValue( variant, "ammoName", info.GetAmmoName() );
	g_pScriptVM->SetValue( variant, "playerPenetrationCount", info.GetPlayerPenetrationCount() );
	g_pScriptVM->SetValue( variant, "damagedOtherPlayers", info.GetDamagedOtherPlayers() );

	return true;
}


CTFScriptedWeapon::CTFScriptedWeapon()
{
}

CTFScriptedWeapon::~CTFScriptedWeapon()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SIMPLE_VOID_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && (bool)retVal == false) \
		return;

#define SIMPLE_BOOL_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.m_type == FIELD_BOOLEAN) \
		return retVal;

#define SIMPLE_FLOAT_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.m_type == FIELD_FLOAT) \
		return retVal;

#define SIMPLE_INT_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.m_type == FIELD_INTEGER) \
		return retVal;

#define SIMPLE_VECTOR_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.m_type == FIELD_VECTOR) \
		return retVal;

#define SIMPLE_VECTOR_REF_OVERRIDE( name ) \
	ScriptVariant_t retVal; \
	ScriptStatus_t result = m_ScriptScope.CallFunc( #name, &retVal ); \
	if (result != SCRIPT_ERROR && retVal.m_type == FIELD_VECTOR) \
	{ \
		static Vector vec = *retVal.m_pVector; \
		return vec; \
	}

#if defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScriptedWeapon::ApplyOnHitAttributes( CBaseEntity *pVictim, CTFPlayer *pAttacker, const CTakeDamageInfo &info )
{
	ScriptVariant_t dmgInfo = g_pScriptVM->RegisterInstance( const_cast<CTakeDamageInfo *>( &info ) );
	m_ScriptScope.PushArg( ToHScript( pVictim ), ToHScript( pAttacker ), dmgInfo );
	SIMPLE_VOID_OVERRIDE( ApplyOnHitAttributes );

	BaseClass::ApplyOnHitAttributes( pVictim, pAttacker, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScriptedWeapon::ApplyPostHitEffects( CTakeDamageInfo const &info, CTFPlayer *pVictim )
{
	ScriptVariant_t dmgInfo = g_pScriptVM->RegisterInstance( const_cast<CTakeDamageInfo *>( &info ) );
	m_ScriptScope.PushArg( dmgInfo, ToHScript( pVictim ) );
	SIMPLE_VOID_OVERRIDE( ApplyPostHitEffects );

	BaseClass::ApplyPostHitEffects( info, pVictim );
}
#endif // GAME_DLL
