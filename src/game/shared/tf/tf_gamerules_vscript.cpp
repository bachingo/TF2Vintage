//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "filesystem.h"
#include "tf_gamerules.h"
#include "vscript_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( GAME_DLL )
class CScriptConvars
{
public:
	ScriptVariant_t GetClientConvarValue( int clientIndex, const char *name )
	{
		const char *cvar = engine->GetClientConVarValue( clientIndex, name );
		if ( cvar )
		{
			return ScriptVariant_t( cvar, true );
		}
		return SCRIPT_VARIANT_NULL;
	}

	ScriptVariant_t GetStr( const char *name )
	{
		ConVarRef cvar( name );
		if ( cvar.IsValid() )
		{
			return ScriptVariant_t( cvar.GetString(), true );
		}
		return SCRIPT_VARIANT_NULL;
	}

	ScriptVariant_t GetFloat( const char *name )
	{
		ConVarRef cvar( name );
		if ( cvar.IsValid() )
		{
			return ScriptVariant_t( cvar.GetFloat() );
		}
		return SCRIPT_VARIANT_NULL;
	}

	void SetValue( const char *name, float value )
	{
		ConVarRef cvar( name );
		if ( !cvar.IsValid() )
		{
			return;
		}

		TFGameRules()->SaveConvar( cvar );

		cvar.SetValue( value );
	}

	void SetValueString( const char *name, const char *value )
	{
		ConVarRef cvar( name );
		if ( !cvar.IsValid() )
		{
			return;
		}

		TFGameRules()->SaveConvar( cvar );

		cvar.SetValue( value );
	}

	void ExecuteConCommand( const char *pszCommand )
	{
		CCommand cmd;
		cmd.Tokenize( pszCommand );

		ConCommand *command = cvar->FindCommand( cmd[0] );
		if ( command )
		{
			if ( !command->IsFlagSet( FCVAR_GAMEDLL ) )
				return;
			command->Dispatch( cmd );
		}
	}
} g_ConvarsVScript;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvars, "Convars", SCRIPT_SINGLETON "Provides an interface for getting and setting convars on the server." )
	DEFINE_SCRIPTFUNC( GetClientConvarValue, "Returns the convar value for the entindex as a string. Only works with client convars with the FCVAR_USERINFO flag." )
	DEFINE_SCRIPTFUNC( GetStr, "Returns the convar as a string. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetFloat, "Returns the convar as a float. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( SetValue, "Sets the value of the convar to a numeric value." )
	DEFINE_SCRIPTFUNC( SetValueString, "Sets the value of the convar to a string." )
	DEFINE_SCRIPTFUNC( ExecuteConCommand, "Executes the convar command." )
END_SCRIPTDESC();
#endif

static float AttribHookValueFloat( float value, char const *szName, HSCRIPT hEntity )
{
	CBaseEntity *pEntity = ToEnt( hEntity );
	if ( !pEntity )
		return value;

	IHasAttributes *pAttribInteface = pEntity->GetHasAttributesInterfacePtr();
	if ( pAttribInteface )
	{
		string_t strAttributeClass = AllocPooledString_StaticConstantStringPointer( szName );
		float flResult = pAttribInteface->GetAttributeManager()->ApplyAttributeFloat( value, pEntity, strAttributeClass, NULL );
		value = flResult;
	}

	return value;
}

static int AttribHookValueInt( int value, char const *szName, HSCRIPT hEntity )
{
	CBaseEntity *pEntity = ToEnt( hEntity );
	if ( !pEntity )
		return value;

	IHasAttributes *pAttribInteface = pEntity->GetHasAttributesInterfacePtr();
	if ( pAttribInteface )
	{
		string_t strAttributeClass = AllocPooledString_StaticConstantStringPointer( szName );
		float flResult = pAttribInteface->GetAttributeManager()->ApplyAttributeFloat( value, pEntity, strAttributeClass, NULL );
		value = RoundFloatToInt( flResult );
	}

	return value;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RegisterScriptFunctions( void )
{
	ScriptRegisterFunctionNamed( g_pScriptVM, AttribHookValueFloat, "GetAttribFloatValue", "Fetch an attribute that is assigned to the provided weapon" );
	ScriptRegisterFunctionNamed( g_pScriptVM, AttribHookValueInt, "GetAttribIntValue", "Fetch an attribute that is assigned to the provided weapon" );

#if defined( GAME_DLL )
	g_pScriptVM->RegisterInstance( &g_ConvarsVScript, "Convars" );
#endif

	char root[ MAX_PATH ]{};
	V_strcpy_safe( root, "scripts\\vscripts" );
	Q_FixSlashes( root );

	FileFindHandle_t fh;
	char const *path = g_pFullFileSystem->FindFirst( root, &fh );
	while ( path )
	{
		CScriptScope hTable{};
		if ( g_pFullFileSystem->FindIsDirectory( fh ) && path[0] != '.' )
			break;

		HSCRIPT hScript = VScriptCompileScript( path, true );

		char className[96]{};
		V_FileBase( path, className, sizeof className );
		if ( hTable.Init( className ) )
		{
			if ( hTable.Run( hScript ) == SCRIPT_ERROR )
			{
				Warning( "Error running script named %s\n", className );
				AssertMsg( 0, "Error running script" );
			}
		}
		else
		{
			Warning( "Unable to create script scope for %s\n", className );
		}

		g_pScriptVM->ReleaseScript( hScript );

		path = g_pFullFileSystem->FindNext( fh );
	}

	g_pFullFileSystem->FindClose( fh );
}
