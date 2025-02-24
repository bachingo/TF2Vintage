//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: VScript constants and enums shared between the server and client.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basecombatweapon_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( WeaponSound, "Weapon sounds." )

	DEFINE_ENUMCONST( EMPTY, "" )
	DEFINE_ENUMCONST( SINGLE, "" )
	DEFINE_ENUMCONST( SINGLE_NPC, "" )
	DEFINE_ENUMCONST( WPN_DOUBLE, "" )
	DEFINE_ENUMCONST( DOUBLE_NPC, "" )
	DEFINE_ENUMCONST( BURST, "" )
	DEFINE_ENUMCONST( RELOAD, "" )
	DEFINE_ENUMCONST( RELOAD_NPC, "" )
	DEFINE_ENUMCONST( MELEE_MISS, "" )
	DEFINE_ENUMCONST( MELEE_HIT, "" )
	DEFINE_ENUMCONST( MELEE_HIT_WORLD, "" )
	DEFINE_ENUMCONST( SPECIAL1, "" )
	DEFINE_ENUMCONST( SPECIAL2, "" )
	DEFINE_ENUMCONST( SPECIAL3, "" )
	DEFINE_ENUMCONST( TAUNT, "" )
	DEFINE_ENUMCONST( DEPLOY, "" )

	DEFINE_ENUMCONST( NUM_SHOOT_SOUND_TYPES, "" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FFireBulletsFlags, "" )

	DEFINE_ENUMCONST( FIRE_BULLETS_FIRST_SHOT_ACCURATE, "" )
	DEFINE_ENUMCONST( FIRE_BULLETS_DONT_HIT_UNDERWATER, "" )
	DEFINE_ENUMCONST( FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS, "" )
	DEFINE_ENUMCONST( FIRE_BULLETS_TEMPORARY_DANGER_SOUND, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

void RegisterWeaponScriptConstants()
{
	//
	// Vector cones
	//
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_PRECALCULATED, "This is just a zero vector, but it adds some context indicating that the person writing the code is not allowing "
		"FireBullets() to modify the direction of the shot because the shot direction "
		"being passed into the function has already been modified by another piece of "
		"code and should be fired as specified." );

	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_1DEGREES, "1-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_2DEGREES, "2-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_3DEGREES, "3-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_4DEGREES, "4-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_5DEGREES, "5-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_6DEGREES, "6-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_7DEGREES, "7-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_8DEGREES, "8-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_9DEGREES, "9-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_10DEGREES, "10-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_15DEGREES, "15-degree weapon vector cone." );
	ScriptRegisterConstantFromTemp( g_pScriptVM, VECTOR_CONE_20DEGREES, "20-degree weapon vector cone." );

	// 
	// Weapon proficiency
	// 
	ScriptRegisterConstant( g_pScriptVM, WEAPON_PROFICIENCY_INVALID, "Invalid weapon proficiency." );
	ScriptRegisterConstant( g_pScriptVM, WEAPON_PROFICIENCY_POOR, "Poor weapon proficiency. Causes low accuracy." );
	ScriptRegisterConstant( g_pScriptVM, WEAPON_PROFICIENCY_AVERAGE, "Average weapon proficiency. Causes average accuracy." );
	ScriptRegisterConstant( g_pScriptVM, WEAPON_PROFICIENCY_GOOD, "Good weapon proficiency. Causes good accuracy." );
	ScriptRegisterConstant( g_pScriptVM, WEAPON_PROFICIENCY_VERY_GOOD, "Very good weapon proficiency. Causes very good accuracy." );
	ScriptRegisterConstant( g_pScriptVM, WEAPON_PROFICIENCY_PERFECT, "Perfect weapon proficiency. Causes perfect accuracy." );

	// 
	// Autoaim
	// 
	ScriptRegisterConstant( g_pScriptVM, AUTOAIM_2DEGREES, "2-degree autoaim cone." );
	ScriptRegisterConstant( g_pScriptVM, AUTOAIM_5DEGREES, "5-degree autoaim cone." );
	ScriptRegisterConstant( g_pScriptVM, AUTOAIM_8DEGREES, "8-degree autoaim cone." );
	ScriptRegisterConstant( g_pScriptVM, AUTOAIM_10DEGREES, "10-degree autoaim cone." );
	ScriptRegisterConstant( g_pScriptVM, AUTOAIM_20DEGREES, "20-degree autoaim cone." );
	ScriptRegisterConstant( g_pScriptVM, AUTOAIM_SCALE_DEFAULT, "Indicates default auto aim scale." );
	ScriptRegisterConstant( g_pScriptVM, AUTOAIM_SCALE_DIRECT_ONLY, "Indicates auto aim should not be used except for direct hits." );
}
