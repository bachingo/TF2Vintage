//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "tier1.h"
#include "utlvector.h"
#include "utlhash.h"
#include "fmtstr.h"

#include "lauxlib.h"
#include "lualib.h"
#include "lstate.h"
#include "lua_vm.h"
#include "lua_vector.h"

COMPILE_TIME_ASSERT( sizeof( intp ) == sizeof( int ) );

typedef struct
{
	ScriptClassDesc_t *m_pClassDesc;
	void *m_pInstance;
	char m_szUniqueId[64];
} ScriptInstance_t;

typedef struct
{
	ScriptFunctionBinding_t *m_pFunctionBinding;
} ScriptFunction_t;

typedef struct
{
	ScriptClassDesc_t *m_pClasDescriptor;
} ScriptClass_t;

class CLuaVM : public IScriptVM
{
	typedef CLuaVM ThisClass;
	typedef IScriptVM BaseClass;
	friend struct lua_State;
public:
	CLuaVM( void );

	enum
	{
		MAX_FUNCTION_PARAMS = 14
	};

	bool				Init( void );
	void				Shutdown( void );

	bool				Frame( float simTime );

	ScriptLanguage_t	GetLanguage()			{ return SL_LUA; }
	char const			*GetLanguageName()		{ return "Lua"; }

	void				AddSearchPath( const char *pszSearchPath );

	void				RemoveOrphanInstances() {}

	HSCRIPT				CompileScript( const char *pszScript, const char *pszId = NULL );
	ScriptStatus_t		Run( const char *pszScript, bool bWait = true );
	ScriptStatus_t		Run( HSCRIPT hScript, HSCRIPT hScope = NULL, bool bWait = true );
	ScriptStatus_t		Run( HSCRIPT hScript, bool bWait );
	void				ReleaseScript( HSCRIPT hScript );

	HSCRIPT				CreateScope( const char *pszScope, HSCRIPT hParent = NULL );
	void				ReleaseScope( HSCRIPT hScript );

	HSCRIPT				LookupFunction( const char *pszFunction, HSCRIPT hScope = NULL );
	void				ReleaseFunction( HSCRIPT hScript );
	ScriptStatus_t		ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait );

	void				RegisterFunction( ScriptFunctionBinding_t *pScriptFunction );
	bool				RegisterClass( ScriptClassDesc_t *pClassDesc );
	HSCRIPT				RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance );
	void				*GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType = NULL );
	void				RemoveInstance( HSCRIPT hScript );

	void				SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId );
	bool				GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize );

	bool				ValueExists( HSCRIPT hScope, const char *pszKey );
	bool				SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue );
	bool				SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value );
	void				CreateTable( ScriptVariant_t &Table );
	int					GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue );
	bool				GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue );
	void				ReleaseValue( ScriptVariant_t &value );
	bool				ClearValue( HSCRIPT hScope, const char *pszKey );
	int					GetNumTableEntries( HSCRIPT hScope );

	void				WriteState( CUtlBuffer *pBuffer ) {}
	void				ReadState( CUtlBuffer *pBuffer ) {}
	void				DumpState() {}

	bool				ConnectDebugger() { return false; }
	void				DisconnectDebugger() {}
	void				SetOutputCallback( ScriptOutputFunc_t pFunc ) {}
	void				SetErrorCallback( ScriptErrorFunc_t pFunc ) {}
	bool				RaiseException( const char *pszExceptionText ) { return true; }

private:
	lua_State *GetVM( void ) const { return L; }

	static void					ConvertToVariant( int nIndex, lua_State *L, ScriptVariant_t *pVariant );
	static void					PushVariant( lua_State *L, ScriptVariant_t const &pVariant );
	static void					ReleaseVariant( lua_State *pState, ScriptVariant_t &value );

	void						RegisterFunctionGuts( ScriptFunctionBinding_t *pFunction, HSCRIPT hScope = NULL );

	int							GetStackSize( void ) const;

	//---------------------------------------------------------------------------------------------
	// LUA Function Callbacks
	//---------------------------------------------------------------------------------------------

	static int					ScriptIndex( lua_State *L );
	static int					TranslateCall( lua_State *L );
	static int					FatalErrorHandler( lua_State *L );
	static int					PrintFunc( lua_State *L );

	//---------------------------------------------------------------------------------------------

	lua_State *L;
	ConVarRef developer;
	long long m_nUniqueKeyQueries;
};

CLuaVM::CLuaVM( void )
	: L( NULL ), developer( "developer" )
{
}

bool CLuaVM::Init( void )
{
	L = lua_open();

	luaL_openlibs( L );
	lua_openvec3( L );

	lua_atpanic( L, &CLuaVM::FatalErrorHandler );

	lua_pushstring( GetVM(), "print" );
	lua_pushcfunction( GetVM(), &CLuaVM::PrintFunc );
	lua_settable( L, LUA_ENVIRONINDEX );

	return true;
}

void CLuaVM::Shutdown( void )
{
	if ( GetVM() )
	{
		lua_close( L );
		L = NULL;
	}
}

bool CLuaVM::Frame( float simTime )
{
	if ( GetVM() )
	{
		Msg( "Garbage collecting...\n" );
		lua_gc( L, LUA_GCCOLLECT, 0 );
	}

	return true;
}

void CLuaVM::AddSearchPath( const char *pszSearchPath )
{
	lua_getglobal( GetVM(), "package" );
	if ( !lua_istable( GetVM(), -1 ) )
	{
		Assert( 0 );
		lua_pop( GetVM(), 1 );
		return;
	}

	lua_getfield( GetVM(), -1, "path" );
	if ( !lua_isstring( GetVM(), -1 ) )
	{
		Assert( 0 );
		lua_pop( GetVM(), 1 );
		return;
	}

	lua_pushstring( GetVM(), CFmtStr( "%s;%s\\?.lua", lua_tostring( GetVM(), -1 ), pszSearchPath ) );
	lua_setfield( GetVM(), -3, "path" );

	lua_pop( GetVM(), 2 );
}

HSCRIPT CLuaVM::CompileScript( const char *pszScript, const char *pszId )
{
	if ( luaL_loadbuffer( GetVM(), pszScript, V_strlen( pszScript ), pszId ) == 0 )
	{
		return (HSCRIPT)lua_ref( GetVM(), true );
	}
	else
	{
		Warning( "%s", lua_tostring( GetVM(), -1 ) );
		return INVALID_HSCRIPT;
	}
}

ScriptStatus_t CLuaVM::Run( const char *pszScript, bool bWait )
{
	if ( luaL_dostring( GetVM(), pszScript ) )
		return SCRIPT_DONE;

	return SCRIPT_ERROR;
}

ScriptStatus_t CLuaVM::Run( HSCRIPT hScript, HSCRIPT hScope, bool bWait )
{
	return ExecuteFunction( hScript, NULL, 0, NULL, hScope, bWait );
}

ScriptStatus_t CLuaVM::Run( HSCRIPT hScript, bool bWait )
{
	return ExecuteFunction( hScript, NULL, 0, NULL, NULL, bWait );
}

void CLuaVM::ReleaseScript( HSCRIPT hScript )
{
	lua_unref( GetVM(), (intptr_t)hScript );
}

HSCRIPT CLuaVM::CreateScope( const char *pszScope, HSCRIPT hParent )
{
	if ( !luaL_newmetatable( GetVM(), pszScope ) )
	{
		lua_pop( GetVM(), 1 );
		return INVALID_HSCRIPT;
	}

	if ( hParent != INVALID_HSCRIPT )
	{
		lua_getref( GetVM(), (intptr_t)hParent );
		if ( lua_isnil( GetVM(), -1 ) || !lua_istable( GetVM(), -1 ) )
			return INVALID_HSCRIPT;

		lua_setmetatable( GetVM(), -1 );
	}

	return (HSCRIPT)lua_ref( GetVM(), true );
}

void CLuaVM::ReleaseScope( HSCRIPT hScript )
{
	lua_unref( GetVM(), (intptr_t)hScript );
}

HSCRIPT CLuaVM::LookupFunction( const char *pszFunction, HSCRIPT hScope )
{
	if ( hScope != INVALID_HSCRIPT )
	{
		lua_getref( GetVM(), (intptr_t)hScope );
		if ( lua_isnil( GetVM(), -1 ) || !lua_istable( GetVM(), -1 ) )
		{
			lua_pop( GetVM(), 1 );
			return NULL;
		}

		lua_getfield( GetVM(), -1, pszFunction );
		if ( lua_isnil( GetVM(), -1 ) || !lua_isfunction( GetVM(), -1 ) )
		{
			lua_pop( GetVM(), 2 );
			return NULL;
		}

		int nFunction = lua_ref( GetVM(), true );
		// pop off the scope reference
		lua_pop( GetVM(), 1 );

		return (HSCRIPT)nFunction;
	}
	else
	{
		lua_getglobal( GetVM(), pszFunction );
		if ( lua_isnil( GetVM(), -1 ) || !lua_isfunction( GetVM(), -1 ) )
		{
			lua_pop( GetVM(), 1 );
			return NULL;
		}

		return (HSCRIPT)lua_ref( GetVM(), true );
	}
}

void CLuaVM::ReleaseFunction( HSCRIPT hScript )
{
	lua_unref( GetVM(), (intptr_t)hScript );
}

ScriptStatus_t CLuaVM::ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait )
{
	if ( hScope == INVALID_HSCRIPT )
	{
		DevWarning( "Invalid scope handed to script VM\n" );
		return SCRIPT_ERROR;
	}

	if ( hFunction == NULL )
	{
		if ( pReturn )
			pReturn->m_type = FIELD_VOID;

		return SCRIPT_ERROR;
	}

	try
	{
		int nOldStack = GetStackSize();

		lua_getref( GetVM(), (intptr_t)hFunction );

		int nTop = lua_gettop( GetVM() );

		if ( hScope )
			lua_getref( GetVM(), (intptr_t)hScope );

		for ( int i = 0; i < nArgs; i++ )
			PushVariant( GetVM(), pArgs[i] );

		lua_call( GetVM(), lua_gettop( GetVM() ) - nTop, pReturn != NULL );
		if ( pReturn )
			ConvertToVariant( -1, GetVM(), pReturn );

		lua_pop( GetVM(), nOldStack - GetStackSize() );

		return SCRIPT_DONE;
	}
	catch( const char *pszString )
	{
		Warning( "%s", pszString );
	}

	return SCRIPT_ERROR;
}

void CLuaVM::RegisterFunction( ScriptFunctionBinding_t *pScriptFunction )
{
	RegisterFunctionGuts( pScriptFunction );
}

bool CLuaVM::RegisterClass( ScriptClassDesc_t *pClassDesc )
{
	if ( !luaL_newmetatable( GetVM(), pClassDesc->m_pszScriptName ) )
	{
		lua_pop( GetVM(), 1 );
		return false;
	}

	lua_pushcfunction( L, &CLuaVM::ScriptIndex );
	lua_setfield( GetVM(), -2, "__index" );

	HSCRIPT hTable = (HSCRIPT)lua_ref( L, true );

	// register member functions
	FOR_EACH_VEC( pClassDesc->m_FunctionBindings, i )
	{
		RegisterFunctionGuts( &pClassDesc->m_FunctionBindings[i], hTable );
	}

	if ( pClassDesc->m_pBaseDesc )
		RegisterClass( pClassDesc->m_pBaseDesc );

	// this pops the script reference for us
	lua_unref( L, (intptr_t)hTable );

	return true;
}

HSCRIPT CLuaVM::RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance )
{
	if ( !RegisterClass( pDesc ) )
		return INVALID_HSCRIPT;

	ScriptInstance_t *pScriptInstance = (ScriptInstance_t *)lua_newuserdata( GetVM(), sizeof ScriptInstance_t );
	pScriptInstance->m_pInstance = pInstance;
	pScriptInstance->m_pClassDesc = pDesc;

	luaL_getmetatable( GetVM(), pDesc->m_pszScriptName );
	lua_setmetatable( GetVM(), -2 );

	return (HSCRIPT)lua_ref( GetVM(), true );
}

void *CLuaVM::GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType )
{
	lua_getref( GetVM(), (intptr_t)hInstance );
	if( !lua_isuserdata( GetVM(), -1 ) )
		return NULL;

	ScriptInstance_t *pInstance = (ScriptInstance_t *)lua_touserdata( GetVM(), -1 );

	lua_pop( GetVM(), 1 );

	ScriptClassDesc_t *pDescription = pInstance->m_pClassDesc;
	while ( pDescription )
	{
		if ( pDescription == pExpectedType )
			return pInstance->m_pInstance;

		pDescription = pDescription->m_pBaseDesc;
	}

	return NULL;
}

void CLuaVM::RemoveInstance( HSCRIPT hScript )
{
	if ( hScript == INVALID_HSCRIPT )
		return;

	lua_getref( GetVM(), (intptr_t)hScript );
	ScriptInstance_t *pInstance = (ScriptInstance_t *)lua_touserdata( GetVM(), -1 );
	lua_pop( GetVM(), 1 );

	delete pInstance;
	lua_unref( GetVM(), (intptr_t)hScript );
}

void CLuaVM::SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId )
{
	if ( hInstance == INVALID_HSCRIPT )
		return;

	lua_getref( GetVM(), (intptr_t)hInstance );
	ScriptInstance_t *pInstance = (ScriptInstance_t *)lua_touserdata( GetVM(), -1 );
	V_strncpy( pInstance->m_szUniqueId, pszId, sizeof pInstance->m_szUniqueId );
	lua_pop( GetVM(), 1 );
}

bool CLuaVM::GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize )
{
	V_snprintf( pBuf, nBufSize, "_%x%I64x_%s", RandomInt(0, 4095), ++m_nUniqueKeyQueries, pszRoot );

	return true;
}

bool CLuaVM::ValueExists( HSCRIPT hScope, const char *pszKey )
{
	lua_getglobal( GetVM(), pszKey );
	int nType = lua_type( GetVM(), -1 );
	lua_pop( GetVM(), 1 );

	return nType != LUA_TNIL;
}

bool CLuaVM::SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		lua_getref( GetVM(), (intptr_t)hScope );
		lua_pushstring( GetVM(), pszKey );
		lua_pushstring( GetVM(), pszValue );
		lua_settable( GetVM(), -3 );

		lua_pop( GetVM(), 1 );
	}
	else
	{
		lua_pushstring( GetVM(), pszKey );
		lua_pushstring( GetVM(), pszValue );
		lua_settable( GetVM(), LUA_GLOBALSINDEX );
	}

	return true;
}

bool CLuaVM::SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		lua_getref( GetVM(), (intptr_t)hScope );
		lua_pushstring( GetVM(), pszKey );
		PushVariant( GetVM(), value );
		lua_settable( GetVM(), -3 );

		lua_pop( GetVM(), 1 );
	}
	else
	{
		lua_pushstring( GetVM(), pszKey );
		PushVariant( GetVM(), value );
		lua_settable( GetVM(), LUA_GLOBALSINDEX );
	}

	return true;
}

void CLuaVM::CreateTable( ScriptVariant_t &Table )
{
	lua_newtable( L );
	ConvertToVariant( -1, GetVM(), &Table );
}

int CLuaVM::GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue )
{
	const int nOldStack = GetStackSize();

	lua_getref( GetVM(), (intptr_t)hScope );
	lua_pushnil( GetVM() );

	int nCount = 0;
	while ( lua_next( GetVM(), -2 ) )
	{
		nCount++;
		lua_pop( GetVM(), 1 );
	}

	ConvertToVariant( -2, GetVM(), pKey );
	ConvertToVariant( -1, GetVM(), pValue );

	lua_pop( GetVM(), 3 );
	lua_pop( GetVM(), nOldStack - GetStackSize() );

	return nCount + 1;
}

bool CLuaVM::GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue )
{
	const int nOldStack = GetStackSize();
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		lua_getref( GetVM(), (intptr_t)hScope );
		lua_getfield( GetVM(), -1, pszKey );
		if ( lua_isnil( GetVM(), -1 ) )
		{
			lua_pop( GetVM(), nOldStack - GetStackSize() );
			return false;
		}
	}
	else
	{
		lua_getglobal( GetVM(), pszKey );
		if ( lua_isnil( GetVM(), -1 ) )
		{
			lua_pop( GetVM(), nOldStack - GetStackSize() );
			return false;
		}
	}

	ConvertToVariant( -1, GetVM(), pValue );
	lua_pop( GetVM(), nOldStack - GetStackSize() );

	return true;
}

void CLuaVM::ReleaseValue( ScriptVariant_t &value )
{
	ReleaseVariant( GetVM(), value );
}

bool CLuaVM::ClearValue( HSCRIPT hScope, const char *pszKey )
{
	return false;
}

int CLuaVM::GetNumTableEntries( HSCRIPT hScope )
{
	if( hScope == INVALID_HSCRIPT )
		return 0;

	int nEntries = 0;
	lua_getref( GetVM(), (intptr_t)hScope );

	// push first key
	lua_pushnil( GetVM() );
	while ( lua_next( GetVM(), -2 ) )
	{
		lua_pop( GetVM(), 1 );
		++nEntries;
	}

	lua_pop( GetVM(), 1 );

	return nEntries;
}

void CLuaVM::RegisterFunctionGuts( ScriptFunctionBinding_t *pFunction, HSCRIPT hScope )
{
	static char szSignature[512];
	if ( pFunction->m_desc.m_Parameters.Count() > MAX_FUNCTION_PARAMS )
		return;

	int nIndex = LUA_GLOBALSINDEX;
	if ( hScope )
	{
		lua_getref( GetVM(), (intptr_t)hScope );
		if ( lua_type( GetVM(), -1 ) != LUA_TNIL )
			nIndex = lua_gettop( GetVM() );
	}

	lua_pushstring( GetVM(), pFunction->m_desc.m_pszScriptName );
	lua_pushlightuserdata( GetVM(), pFunction );
	lua_pushcclosure( GetVM(), &CLuaVM::TranslateCall, 1 );
	lua_settable( GetVM(), nIndex );

	if ( hScope )
		lua_pop( GetVM(), 1 );
}

FORCEINLINE int CLuaVM::GetStackSize( void ) const
{
	return ( (char *)GetVM()->stack_last - (char *)GetVM()->top ) / sizeof(TValue);
}

void CLuaVM::ConvertToVariant( int nIndex, lua_State *L, ScriptVariant_t *pVariant )
{
	switch ( lua_type( L, nIndex ) )
	{
		case LUA_TNIL:
		{
			pVariant->m_type = FIELD_VOID;
			break;
		}
		case LUA_TBOOLEAN:
		{
			*pVariant = lua_toboolean( L, nIndex ) != FALSE;
			break;
		}
		case LUA_TNUMBER:
		{
			*pVariant = lua_tonumber( L, nIndex );
			break;
		}
		case LUA_TSTRING:
		{
			const int nLength = V_strlen( lua_tostring( L, nIndex ) );
			char *pString = new char[ nLength+1 ];
			V_memcpy( pString, lua_tostring( L, nIndex ), nLength+1 );

			*pVariant = pString;
			pVariant->m_flags |= SV_FREE;

			break;
		}
		default:
		{
			*pVariant = (HSCRIPT)lua_ref( L, true );
			break;
		}
	}
}

void CLuaVM::PushVariant( lua_State *L, ScriptVariant_t const &pVariant )
{
	switch ( pVariant.m_type )
	{
		case FIELD_VOID:
		{
			lua_pushnil( L );
			break;
		}
		case FIELD_INTEGER:
		{
			lua_pushinteger( L, pVariant.m_int );
			break;
		}
		case FIELD_FLOAT:
		{
			lua_pushnumber( L, pVariant.m_float );
			break;
		}
		case FIELD_BOOLEAN:
		{
			lua_pushboolean( L, pVariant.m_bool );
			break;
		}
		case FIELD_CHARACTER:
		{
			lua_pushstring( L, &pVariant.m_char );
			break;
		}
		case FIELD_CSTRING:
		{
			char const *szString = pVariant.m_pszString ? pVariant : "";
			lua_pushstring( L, szString );

			break;
		}
		case FIELD_HSCRIPT:
		{
			if ( pVariant.m_hScript )
				lua_getref( L, (intptr_t)pVariant.m_hScript );
			else
				lua_pushnil( L );

			break;
		}
		case FIELD_VECTOR:
		{
			lua_pushvec3( L, pVariant.m_pVector );
			break;
		}
	}
}

void CLuaVM::ReleaseVariant( lua_State *pState, ScriptVariant_t &value )
{
	if ( value.m_type == FIELD_HSCRIPT )
	{
		lua_unref( pState, (intptr_t)value.m_hScript );
	}
	else
	{
		value.Free();
	}

	value.m_type = FIELD_VOID;
}

int CLuaVM::ScriptIndex( lua_State *L )
{
	ScriptInstance_t *pInstance = (ScriptInstance_t *)lua_touserdata( L, 1 );
	char const *pAccessor = luaL_checkstring( L, 2 );

	ScriptClassDesc_t *pClass = pInstance->m_pClassDesc;
	while ( pClass )
	{
		luaL_getmetatable( L, pClass->m_pszScriptName );
		if ( lua_type( L, -1 ) == LUA_TNIL )
			break;

		lua_pushstring( L, pAccessor );
		lua_rawget( L, -2 );
		// we found what we wanted to, stop iterating
		if ( lua_type( L, -1 ) != LUA_TNIL )
			break;

		lua_pop( L, 2 );
		pClass = pClass->m_pBaseDesc;
	}

	return 1;
}

int CLuaVM::TranslateCall( lua_State *L )
{
	CUtlVectorFixed<ScriptVariant_t, MAX_FUNCTION_PARAMS> parameters;

	ScriptFunctionBinding_t *pFuncBinding = (ScriptFunctionBinding_t *)lua_touserdata( L, lua_upvalueindex( 1 ) );
	CUtlVector<ScriptDataType_t> const &fnParams = pFuncBinding->m_desc.m_Parameters;

	parameters.SetCount( fnParams.Count() );

	int nArguments = Max( fnParams.Count(), lua_gettop( L ) );
	void *pContext = NULL; int nOffset = 1;
	if ( pFuncBinding->m_flags & SF_MEMBER_FUNC )
	{
		ScriptInstance_t *pInstance = (ScriptInstance_t *)lua_touserdata( L, nOffset );
		if ( pInstance == NULL || pInstance->m_pInstance == NULL )
			return SCRIPT_ERROR;

		IScriptInstanceHelper *pHelper = pInstance->m_pClassDesc->pHelper;
		if ( pHelper )
		{
			pContext = pHelper->GetProxied( pInstance->m_pInstance );
			if ( pContext == NULL )
				return SCRIPT_ERROR;
		}
		else
		{
			pContext = pInstance->m_pInstance;
		}

		nOffset++;
		nArguments--;
	}

	for ( int i=0; i < nArguments; ++i )
	{
		switch ( fnParams.Element( i ) )
		{
			case FIELD_INTEGER:
			{
				parameters[ i ] = lua_tointeger( L, i+2 );
				break;
			}
			case FIELD_FLOAT:
			{
				parameters[ i ] = lua_tonumber( L, i+2 );
				break;
			}
			case FIELD_BOOLEAN:
			{
				parameters[ i ] = lua_toboolean( L, i+2 ) != FALSE;
				break;
			}
			case FIELD_CHARACTER:
			{
				char const *pChar = lua_tostring( L, i+2 );
				if ( pChar == NULL )
					pChar = "\0";

				parameters[ i ] = *pChar;
				break;
			}
			case FIELD_CSTRING:
			{
				parameters[ i ] = lua_tostring( L, i+2 );
				break;
			}
			case FIELD_VECTOR:
			{
				parameters[ i ] = lua_tovec3( L, i+2 );
				break;
			}
			case FIELD_HSCRIPT:
			{
				lua_pushvalue( L, i+2 );
				parameters[ i ] = (HSCRIPT)lua_ref( L, true );
				break;
			}
			default:
				break;
		}
	}

	const bool bHasReturn = pFuncBinding->m_desc.m_ReturnType != FIELD_VOID;

	ScriptVariant_t returnValue;
	pFuncBinding->m_pfnBinding( pFuncBinding->m_pFunction, 
								pContext, 
								parameters.Base(), 
								parameters.Count(), 
								bHasReturn ? &returnValue : NULL );

	if ( bHasReturn )
		PushVariant( L, returnValue );

	for ( int i=0; i < parameters.Count(); ++i )
		ReleaseVariant( L, parameters[i] );

	return pFuncBinding->m_desc.m_ReturnType != FIELD_VOID;
}

int CLuaVM::FatalErrorHandler( lua_State *L )
{
	throw lua_tostring( L, 1 );
	return 0;
}

int CLuaVM::PrintFunc( lua_State *L )
{
	CUtlString string;
	int nArgs = lua_gettop( L );

	lua_getfield( L, LUA_REGISTRYINDEX, "tostring" );
	for ( int i=0; i < nArgs; ++i )
	{
		lua_pushvalue( L, -1 );
		lua_pushvalue( L, i );
		lua_call( L, 1, 1 );

		const char *pString = lua_tostring( L, -1 );
		if ( pString == NULL )
			return luaL_error( L, LUA_QL( "tostring" ) " must return a string to " LUA_QL( "print" ) );

		if ( i > 1 )
			string += "\t";

		string += pString;

		lua_settop( L, -2 );
	}
	
	Msg( "%s", string.Get() );

	return 0;
}


IScriptVM *CreateLuaVM( void )
{
	return new CLuaVM();
}

void DestroyLuaVM( IScriptVM *pVM )
{
	if( pVM )
	{
		pVM->Shutdown();
		delete pVM;
	}
}
