//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_shared.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "characterset.h"
#include "isaverestore.h"
#include "gamerules.h"

IScriptVM *g_pScriptVM;
extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );


// ----------------------------------------------------------------------------
class CScriptColorInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize )
	{
		Color *pClr = ( (Color *)p );
		V_snprintf( pBuf, bufSize, "(color: (%i, %i, %i, %i))", pClr->r(), pClr->g(), pClr->b(), pClr->a() );
		return true;
	}
} g_ColorScriptInstanceHelper;
DEFINE_SCRIPT_INSTANCE_HELPER( Color, &g_ColorScriptInstanceHelper )

BEGIN_SCRIPTDESC_ROOT( Color, "" )
	DEFINE_SCRIPT_CONSTRUCTOR()

	DEFINE_SCRIPTFUNC( SetColor, "Sets the color." )

	DEFINE_SCRIPTFUNC( SetRawColor, "Sets the raw color integer." )
	DEFINE_SCRIPTFUNC( GetRawColor, "Gets the raw color integer." )

	DEFINE_MEMBERVAR_NAMED( _color[0], FIELD_CHARACTER, "r", "Member variable for red.")
	DEFINE_MEMBERVAR_NAMED( _color[1], FIELD_CHARACTER, "g", "Member variable for green.")
	DEFINE_MEMBERVAR_NAMED( _color[2], FIELD_CHARACTER, "b", "Member variable for blue.")
	DEFINE_MEMBERVAR_NAMED( _color[3], FIELD_CHARACTER, "a", "Member variable for alpha. (transparency)")
END_SCRIPTDESC();


// ----------------------------------------------------------------------------
// KeyValues access - CBaseEntity::ScriptGetKeyFromModel returns root KeyValues
// ----------------------------------------------------------------------------
BEGIN_SCRIPTDESC_ROOT( CScriptKeyValues, "Wrapper class over KeyValues instance" )
	DEFINE_SCRIPT_CONSTRUCTOR()	
	DEFINE_SCRIPTFUNC_NAMED( ScriptFindKey, "FindKey", "Given a KeyValues object and a key name, find a KeyValues object associated with the key name" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFirstSubKey, "GetFirstSubKey", "Given a KeyValues object, return the first sub key object" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetNextKey, "GetNextKey", "Given a KeyValues object, return the next key object in a sub key group" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueInt, "GetKeyInt", "Given a KeyValues object and a key name, return associated integer value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueFloat, "GetKeyFloat", "Given a KeyValues object and a key name, return associated float value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueBool, "GetKeyBool", "Given a KeyValues object and a key name, return associated bool value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueString, "GetKeyString", "Given a KeyValues object and a key name, return associated string value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsKeyValueEmpty, "IsKeyEmpty", "Given a KeyValues object and a key name, return true if key name has no value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptReleaseKeyValues, "ReleaseKeyValues", "Given a root KeyValues object, release its contents" )

	DEFINE_SCRIPTFUNC( TableToSubKeys, "Converts a script table to KeyValues." )
	DEFINE_SCRIPTFUNC( SubKeysToTable, "Converts to script table." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptFindOrCreateKey, "FindOrCreateKey", "Given a KeyValues object and a key name, find or create a KeyValues object associated with the key name" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetName, "GetName", "Given a KeyValues object, return its name" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetInt, "GetInt", "Given a KeyValues object, return its own associated integer value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFloat, "GetFloat", "Given a KeyValues object, return its own associated float value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetString, "GetString", "Given a KeyValues object, return its own associated string value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBool, "GetBool", "Given a KeyValues object, return its own associated bool value" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueInt, "SetKeyInt", "Given a KeyValues object and a key name, set associated integer value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueFloat, "SetKeyFloat", "Given a KeyValues object and a key name, set associated float value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueBool, "SetKeyBool", "Given a KeyValues object and a key name, set associated bool value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueString, "SetKeyString", "Given a KeyValues object and a key name, set associated string value" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptSetName, "SetName", "Given a KeyValues object, set its name" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetInt, "SetInt", "Given a KeyValues object, set its own associated integer value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetFloat, "SetFloat", "Given a KeyValues object, set its own associated float value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetBool, "SetBool", "Given a KeyValues object, set its own associated bool value" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetString, "SetString", "Given a KeyValues object, set its own associated string value" )
END_SCRIPTDESC();

HSCRIPT CScriptKeyValues::ScriptFindKey( const char *pszName )
{
	KeyValues *pKeyValues = m_pKeyValues->FindKey(pszName);
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetFirstSubKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetFirstSubKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetNextKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetNextKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

int CScriptKeyValues::ScriptGetKeyValueInt( const char *pszName )
{
	int i = m_pKeyValues->GetInt( pszName );
	return i;
}

float CScriptKeyValues::ScriptGetKeyValueFloat( const char *pszName )
{
	float f = m_pKeyValues->GetFloat( pszName );
	return f;
}

const char *CScriptKeyValues::ScriptGetKeyValueString( const char *pszName )
{
	const char *psz = m_pKeyValues->GetString( pszName );
	return psz;
}

bool CScriptKeyValues::ScriptIsKeyValueEmpty( const char *pszName )
{
	bool b = m_pKeyValues->IsEmpty( pszName );
	return b;
}

bool CScriptKeyValues::ScriptGetKeyValueBool( const char *pszName )
{
	bool b = m_pKeyValues->GetBool( pszName );
	return b;
}

void CScriptKeyValues::ScriptReleaseKeyValues( )
{
	m_pKeyValues->deleteThis();
	m_pKeyValues = NULL;
}

void CScriptKeyValues::TableToSubKeys( HSCRIPT hTable )
{
	int nIterator = -1;
	ScriptVariant_t varKey, varValue;
	while ((nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &varKey, &varValue )) != -1)
	{
		switch (varValue.m_type)
		{
			case FIELD_CSTRING:		m_pKeyValues->SetString( varKey.m_pszString, varValue.m_pszString ); break;
			case FIELD_INTEGER:		m_pKeyValues->SetInt( varKey.m_pszString, varValue.m_int ); break;
			case FIELD_FLOAT:		m_pKeyValues->SetFloat( varKey.m_pszString, varValue.m_float ); break;
			case FIELD_BOOLEAN:		m_pKeyValues->SetBool( varKey.m_pszString, varValue.m_bool ); break;
			case FIELD_VECTOR:		m_pKeyValues->SetString( varKey.m_pszString, CFmtStr( "%f %f %f", varValue.m_pVector->x, varValue.m_pVector->y, varValue.m_pVector->z ) ); break;
		}

		g_pScriptVM->ReleaseValue( varKey );
		g_pScriptVM->ReleaseValue( varValue );
	}
}

void CScriptKeyValues::SubKeysToTable( HSCRIPT hTable )
{
	FOR_EACH_SUBKEY( m_pKeyValues, key )
	{
		switch ( key->GetDataType() )
		{
			case KeyValues::TYPE_STRING: g_pScriptVM->SetValue( hTable, key->GetName(), key->GetString() ); break;
			case KeyValues::TYPE_INT:    g_pScriptVM->SetValue( hTable, key->GetName(), key->GetInt()    ); break;
			case KeyValues::TYPE_FLOAT:  g_pScriptVM->SetValue( hTable, key->GetName(), key->GetFloat()  ); break;
		}
	}
}

HSCRIPT CScriptKeyValues::ScriptFindOrCreateKey( const char *pszName )
{
	KeyValues *pKeyValues = m_pKeyValues->FindKey(pszName, true);
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

const char *CScriptKeyValues::ScriptGetName()
{
	const char *psz = m_pKeyValues->GetName();
	return psz;
}

int CScriptKeyValues::ScriptGetInt()
{
	int i = m_pKeyValues->GetInt();
	return i;
}

float CScriptKeyValues::ScriptGetFloat()
{
	float f = m_pKeyValues->GetFloat();
	return f;
}

const char *CScriptKeyValues::ScriptGetString()
{
	const char *psz = m_pKeyValues->GetString();
	return psz;
}

bool CScriptKeyValues::ScriptGetBool()
{
	bool b = m_pKeyValues->GetBool();
	return b;
}


void CScriptKeyValues::ScriptSetKeyValueInt( const char *pszName, int iValue )
{
	m_pKeyValues->SetInt( pszName, iValue );
}

void CScriptKeyValues::ScriptSetKeyValueFloat( const char *pszName, float flValue )
{
	m_pKeyValues->SetFloat( pszName, flValue );
}

void CScriptKeyValues::ScriptSetKeyValueString( const char *pszName, const char *pszValue )
{
	m_pKeyValues->SetString( pszName, pszValue );
}

void CScriptKeyValues::ScriptSetKeyValueBool( const char *pszName, bool bValue )
{
	m_pKeyValues->SetBool( pszName, bValue );
}

void CScriptKeyValues::ScriptSetName( const char *pszValue )
{
	m_pKeyValues->SetName( pszValue );
}

void CScriptKeyValues::ScriptSetInt( int iValue )
{
	m_pKeyValues->SetInt( NULL, iValue );
}

void CScriptKeyValues::ScriptSetFloat( float flValue )
{
	m_pKeyValues->SetFloat( NULL, flValue );
}

void CScriptKeyValues::ScriptSetString( const char *pszValue )
{
	m_pKeyValues->SetString( NULL, pszValue );
}

void CScriptKeyValues::ScriptSetBool( bool bValue )
{
	m_pKeyValues->SetBool( NULL, bValue );
}


// constructors
CScriptKeyValues::CScriptKeyValues( KeyValues *pKeyValues = NULL )
{
	if (pKeyValues == NULL)
	{
		m_pKeyValues = new KeyValues("CScriptKeyValues");
	}
	else
	{
		m_pKeyValues = pKeyValues;
	}
}

// destructor
CScriptKeyValues::~CScriptKeyValues( )
{
	if (m_pKeyValues)
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = NULL;
}


// Shorten the string and return it
const char *VScriptCutDownString( const char* str )
{
	static char staticStr[MAX_PATH] = {0};
	Q_strncpy( staticStr, str, MAX_PATH );
	return staticStr;
}

HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return NULL;
	}

	static const char *pszExtensions[] =
	{
		"",		// SL_NONE
		".gm",	// SL_GAMEMONKEY
		".nut",	// SL_SQUIRREL
		".lua", // SL_LUA
		".py",  // SL_PYTHON
		".as"	// AS_ANGELSCRIPT
	};

	const char *pszVMExtension = pszExtensions[ g_pScriptVM->GetLanguage() ];
	const char *pszIncomingExtension = V_strrchr( pszScriptName , '.' );
	if ( pszIncomingExtension && V_strcmp( pszIncomingExtension, pszVMExtension ) != 0 )
	{
		Warning( "Script file type does not match VM type\n" );
		return NULL;
	}

	CFmtStr scriptPath;
	if ( pszIncomingExtension )
	{
		scriptPath.sprintf( "scripts/vscripts/%s", pszScriptName );
	}
	else
	{	
		scriptPath.sprintf( "scripts/vscripts/%s%s", pszScriptName,  pszVMExtension );
	}

	const char *pBase;
	CUtlBuffer bufferScript;

	if ( g_pScriptVM->GetLanguage() == SL_PYTHON )
	{
		// python auto-loads raw or precompiled modules - don't load data here
		pBase = NULL;
	}
	else
	{
		bool bResult = filesystem->ReadFile( scriptPath, "GAME", bufferScript );

		if( !bResult )
		{
			if( bWarnMissing )
			{
				Warning( "Script not found (%s) \n", scriptPath.Get() );
				Assert( "Error running script" );
			}
		}

		pBase = (const char *) bufferScript.Base();

		if ( !pBase || !*pBase )
		{
			return NULL;
		}
	}


	const char *pszFilename = V_strrchr( scriptPath, '/' );
	pszFilename++;
	HSCRIPT hScript = g_pScriptVM->CompileScript( pBase, pszFilename );
	if ( !hScript )
	{
		Warning( "FAILED to compile and execute script file named %s\n", scriptPath.Get() );
		Assert( "Error running script" );
	}
	return hScript;
}

static int g_ScriptServerRunScriptDepth;

bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	if ( !pszScriptName || !*pszScriptName )
	{
		Warning( "Cannot run script: NULL script name\n" );
		return false;
	}

	// Prevent infinite recursion in VM
	if ( g_ScriptServerRunScriptDepth > 16 )
	{
		Warning( "IncludeScript stack overflow\n" );
		return false;
	}

	g_ScriptServerRunScriptDepth++;
	HSCRIPT	hScript = VScriptCompileScript( pszScriptName, bWarnMissing );
	bool bSuccess = false;
	if ( hScript )
	{
#ifdef GAME_DLL
		if ( gpGlobals->maxClients == 1 )
		{
			CBaseEntity *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				g_pScriptVM->SetValue( "player", pPlayer->GetScriptInstance() );
			}
		}
#endif
		bSuccess = ( g_pScriptVM->Run( hScript, hScope ) != SCRIPT_ERROR );
		if ( !bSuccess )
		{
			Warning( "Error running script named %s\n", pszScriptName );
			Assert( "Error running script" );
		}
	}
	g_ScriptServerRunScriptDepth--;
	return bSuccess;
}

CON_COMMAND_SHARED( script, "Run the text as a script" )
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !*args[1] )
	{
		Warning( "No function name specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args.GetCommandString();

#ifdef CLIENT_DLL
	pszScript += 13;
#else
	pszScript += 6;
#endif
	
	while ( *pszScript == ' ' )
	{
		pszScript++;
	}

	if ( !*pszScript )
	{
		return;
	}

	if ( *pszScript != '\"' )
	{
		g_pScriptVM->Run( pszScript );
	}
	else
	{
		pszScript++;
		const char *pszEndQuote = pszScript;
		while ( *pszEndQuote !=  '\"' )
		{
			pszEndQuote++;
		}
		if ( !*pszEndQuote )
		{
			return;
		}
		*((char *)pszEndQuote) = 0;
		g_pScriptVM->Run( pszScript );
		*((char *)pszEndQuote) = '\"';
	}
}


CON_COMMAND_SHARED( script_execute, "Run a vscript file" )
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !*args[1] )
	{
		Warning( "No script specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	VScriptRunScript( args[1], true );
}

CON_COMMAND_SHARED( script_debug, "Connect the vscript VM to the script debugger" )
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->ConnectDebugger();
}

CON_COMMAND_SHARED( script_help, "Output help for script functions, optionally with a search string" )
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}
	const char *pszArg1 = "*";
	if ( *args[1] )
	{
		pszArg1 = args[1];
	}

	g_pScriptVM->Run( CFmtStr( "PrintHelp( \"%s\" );", pszArg1 ) );
}

CON_COMMAND_SHARED( script_dump_all, "Dump the state of the VM to the console" )
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->DumpState();
}
