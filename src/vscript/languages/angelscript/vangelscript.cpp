#include "tier0/platform.h"

#include "angelscript.h"
#include "scriptbuilder/scriptbuilder.h"
#include "contextmgr/contextmgr.h"
#include "scriptmath/scriptmath.h"
#include "scripthandle/scripthandle.h"
#include "scriptarray/scriptarray.h"
#include "scriptany/scriptany.h"
#include "scriptdictionary/scriptdictionary.h"
#include "scriptstdstring/scriptstdstring.h"
#include "scripthelper/scripthelper.h"

#include "vscript/ivscript.h"
#include "as_vector.h"
#include "as_jit.h"

#include "tier1/utlstack.h"
#include "tier1/utlhash.h"
#include "tier1/utlbuffer.h"
#include "tier1/mempool.h"
#include "tier1/fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define VSCRIPT_TAG		(asPWORD)"VScriptVM"

struct ScriptScope_t
{
	char szScopeName[96];
	asIScriptModule *pModule;
	ScriptScope_t *pParent;
	CScriptDictionary *pTable;
};

struct ScriptContext_t
{
	ScriptFunctionBinding_t *pFuncBinding;
	ScriptClassDesc_t *pClassDesc;
};

class CAngelScriptVM : public IScriptVM
{
public:
	CAngelScriptVM();

	bool				Init( void );
	void				Shutdown( void );

	bool				Frame( float simTime );

	ScriptLanguage_t	GetLanguage() { return SL_ANGELSCRIPT; }
	char const			*GetLanguageName() { return "AngelScript"; }

	ScriptStatus_t		Run( const char *pszScript, bool bWait = true );

	void				AddSearchPath( const char *pszSearchPath ) {}

	void				RemoveOrphanInstances();

	HSCRIPT				CompileScript( const char *pszScript, const char *pszId = NULL );
	ScriptStatus_t		Run( HSCRIPT hScript, HSCRIPT hScope = NULL, bool bWait = true );
	ScriptStatus_t		Run( HSCRIPT hScript, bool bWait );
	void				ReleaseScript( HSCRIPT hScript );

	HSCRIPT				CreateScope( const char *pszScope, HSCRIPT hParent = NULL );
	HSCRIPT				ReferenceScope( HSCRIPT hScope );
	void				ReleaseScope( HSCRIPT hScript );

	HSCRIPT				LookupFunction( const char *pszFunction, HSCRIPT hScope = NULL );
	void				ReleaseFunction( HSCRIPT hScript );
	ScriptStatus_t		ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait );

	void				RegisterFunction( ScriptFunctionBinding_t *pScriptFunction );
	bool				RegisterClass( ScriptClassDesc_t *pClassDesc );
	void				RegisterConstant( ScriptConstantBinding_t *pScriptConstant );
	void				RegisterEnum( ScriptEnumDesc_t *pEnumDesc );

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

	ScriptClassDesc_t*	GetClassDescForType( asITypeInfo *pInfo );

private:
	asIScriptEngine *GetVM( void ) { return m_pEngine; }

	enum
	{
		MAX_FUNCTION_PARAMS = 14
	};

	static asUINT GetTime( void );
	static void *Alloc( size_t size );
	static void Free( void *p );
	static void PrintAny( CScriptAny *pAny );
	static void PrintString( const std::string &str );
	static int OnInclude( char const *pszFromFile, char const *pszToFile, CScriptBuilder *pBuilder, void *pvParam );
	static asIScriptContext *OnRequestContext( asIScriptEngine *engine, void *pvParam );
	static void OnReturnContext( asIScriptEngine *engine, asIScriptContext *pContext, void *pvParam );
	static void TranslateCall( asIScriptGeneric *gen );

	void PrintFunc( asSMessageInfo *msg );
	void OnException( asIScriptContext *ctx );
	void OnLineCue( asIScriptContext *ctx );
	bool RegisterFunctionGuts( ScriptFunctionBinding_t *pFuncBinding, ScriptClassDesc_t *pClassDesc );
	bool RegisterClassInheritance( ScriptClassDesc_t *pClassDesc );
	bool ConvertToVariant( void *pData, int iTypeID, ScriptVariant_t *pVariant );

	asIScriptEngine*				m_pEngine;
	CContextMgr						m_ContextMgr;
	asCJITCompiler					m_JIT;
	CUtlStack<asIScriptContext *>	m_ContextPool;
	CObjectPool<ScriptScope_t, 8>	m_ScopePool;
	CUtlVector<ScriptContext_t *>	m_ScriptContexts;
	CUtlHashFast<ScriptClassDesc_t *, CUtlHashFastGenericHash> m_ClassTypes;
	long long						m_nUniqueKeySerial;
	int								m_nStringTypeID;
	int								m_nVectorTypeID;
	int								m_nQuaternionTypeID;
	int								m_nMatrixTypeID;

	// Inherited via IScriptVM
	HSCRIPT LookupFunction(const char *pszFunction, HSCRIPT hScope, bool bNoDelegation) override
	{
		return HSCRIPT();
	}
	CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get(HSCRIPT &hParentObject, const char *pszSlotName, ISquirrelMetamethodDelegate *pDelegate, bool bDeleteDelegateWhenIAmDeleted) override
	{
		return nullptr;
	}
	void DestroySquirrelMetamethod_Get(CSquirrelMetamethodDelegateImpl *pMetaMethodImpl) override
	{
	}
	int GetKeyValue2(HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue) override
	{
		return 0;
	}
};


class CScriptClass
{
#pragma push_macro("new")
#undef new
	DECLARE_FIXEDSIZE_ALLOCATOR( CScriptClass );
#pragma pop_macro("new")
public:
	CScriptClass( ScriptClassDesc_t *pClassDesc )
		: m_pClassDesc( pClassDesc ), m_pInstance( NULL )
	{
		m_unRefCount = 1;

		if ( m_pClassDesc->m_pfnConstruct )
			m_pInstance = m_pClassDesc->m_pfnConstruct();
	}

	CScriptClass( void *pInstance, ScriptClassDesc_t *pClassDesc )
		: m_pClassDesc( pClassDesc ), m_pInstance( pInstance )
	{
		m_unRefCount = 1;
	}

	static void Construct( asIScriptGeneric *gen )
	{
		CAngelScriptVM *pVM = (CAngelScriptVM *)gen->GetEngine()->GetUserData( VSCRIPT_TAG );
		asITypeInfo *pInfo = gen->GetEngine()->GetTypeInfoById( gen->GetFunction()->GetReturnTypeId() );
		ScriptClassDesc_t *pClassDesc = pVM->GetClassDescForType( pInfo );

		*(CScriptClass **)gen->GetAddressOfReturnLocation() = new CScriptClass( pClassDesc );
	}

	int AddRef() const
	{
		return ThreadInterlockedIncrement( &m_unRefCount );
	}
	void Release() const
	{
		Assert( m_unRefCount >= 0 );
		if ( ThreadInterlockedDecrement( &m_unRefCount ) == 0 )
		{
			if ( m_pClassDesc->m_pfnDestruct && m_pInstance )
				m_pClassDesc->m_pfnDestruct( m_pInstance );

			delete this;
		}
	}

	void *GetInstance() const { return m_pInstance; }
	ScriptClassDesc_t *GetDesc() const { return m_pClassDesc; }

private:
	friend class CAngelScriptVM;
	ScriptClassDesc_t *m_pClassDesc;
	void *m_pInstance;
	char m_szUniqueId[256];
	mutable volatile uint32 m_unRefCount;
};
DEFINE_FIXEDSIZE_ALLOCATOR( CScriptClass, 1, UTLMEMORYPOOL_GROW_FAST );


CAngelScriptVM::CAngelScriptVM()
	: m_JIT( JIT_ALLOC_SIMPLE|JIT_SYSCALL_NO_ERRORS )
{
	m_pEngine = NULL;
	m_nUniqueKeySerial = 0;

	m_ClassTypes.Init( 32 );
}

bool CAngelScriptVM::Init( void )
{
	asSetGlobalMemoryFunctions( &Alloc, &Free );
	CScriptArray::SetMemoryFunctions( &Alloc, &Free );

	m_pEngine = asCreateScriptEngine();
	m_pEngine->SetMessageCallback( asMETHOD( CAngelScriptVM, PrintFunc ), this, asCALL_THISCALL );
	m_pEngine->SetContextCallbacks( &OnRequestContext, &OnReturnContext, this );

	RegisterStdString( m_pEngine );
	RegisterScriptArray( m_pEngine, true );
	RegisterScriptDictionary( m_pEngine );
	RegisterScriptAny( m_pEngine );
	RegisterStdStringUtils( m_pEngine );
	RegisterScriptHandle( m_pEngine );
	RegisterScriptMath( m_pEngine );
	RegisterValveScriptMath( m_pEngine );

	m_ContextMgr.SetGetTimeCallback( &GetTime );
	m_ContextMgr.RegisterCoRoutineSupport( m_pEngine );

	m_pEngine->SetJITCompiler( &m_JIT );
	m_pEngine->SetUserData( this, VSCRIPT_TAG );

	m_pEngine->SetEngineProperty( asEP_INCLUDE_JIT_INSTRUCTIONS, 1 );
	m_pEngine->SetEngineProperty( asEP_BUILD_WITHOUT_LINE_CUES, 1 );
	m_pEngine->SetEngineProperty( asEP_ALLOW_MULTILINE_STRINGS, 1 );

	Verify( m_pEngine->RegisterGlobalFunction( "void print(const string &in)", asFUNCTION( PrintString ), asCALL_CDECL ) >= 0 );
	Verify( m_pEngine->RegisterGlobalFunction( "void print(const any@)", asFUNCTION( PrintAny ), asCALL_CDECL ) >= 0 );
	Verify( m_pEngine->RegisterGlobalFunction( "uint time()", asFUNCTION( GetTime ), asCALL_CDECL ) >= 0 );

	m_nStringTypeID = m_pEngine->GetTypeIdByDecl( "string" );
	m_nVectorTypeID = m_pEngine->GetTypeIdByDecl( "Vector3" );
	m_nQuaternionTypeID = m_pEngine->GetTypeIdByDecl( "Quaternion" );
	m_nMatrixTypeID = m_pEngine->GetTypeIdByDecl( "Matrix" );

	return true;
}

void CAngelScriptVM::Shutdown( void )
{
	m_ContextMgr.AbortAll();
	m_pEngine->ShutDownAndRelease();

	m_ScriptContexts.PurgeAndDeleteElements();
	m_ScopePool.Purge();
	m_ContextPool.Purge();
	m_ClassTypes.Purge();
}

bool CAngelScriptVM::Frame( float simTime )
{
	return m_ContextMgr.ExecuteScripts() == 0;
}

ScriptStatus_t CAngelScriptVM::Run( const char *pszScript, bool bWait )
{
	int r = ExecuteString( m_pEngine, pszScript );
	return r >= 0 ? SCRIPT_DONE : SCRIPT_ERROR;
}

HSCRIPT CAngelScriptVM::CompileScript( const char *pszScript, const char *pszId )
{
	CScriptBuilder builder;
	builder.SetIncludeCallback( &OnInclude, this );

	if ( builder.StartNewModule( m_pEngine, pszId ? pszId : "CompileScript" ) < 0 )
	{
		AssertMsg( false, "Compilation of AngelScript code failed!" );
		Warning( "Compilation of AngelScript code failed!\n" );
		return NULL;
	}

	if ( builder.AddSectionFromMemory( "CompileScript", pszScript, V_strlen( pszScript ) ) < 0 )
	{
		AssertMsg( false, "Compilation of AngelScript code failed!" );
		Warning( "Compilation of AngelScript code failed!\n" );
		return NULL;
	}

	if ( builder.BuildModule() < 0 )
	{
		AssertMsg( false, "Compilation of AngelScript code failed!" );
		Warning( "Compilation of AngelScript code failed!\n" );
		return NULL;
	}

	return (HSCRIPT)builder.GetModule();
}

ScriptStatus_t CAngelScriptVM::Run( HSCRIPT hScript, HSCRIPT hScope, bool bWait )
{
	if ( hScope == INVALID_HSCRIPT )
		return SCRIPT_ERROR;

	((ScriptScope_t *)hScope)->pModule = (asIScriptModule *)hScript;

	return SCRIPT_DONE;
}

ScriptStatus_t CAngelScriptVM::Run( HSCRIPT hScript, bool bWait )
{
	Assert( 0 );
	return SCRIPT_ERROR;
}

void CAngelScriptVM::RemoveOrphanInstances()
{
	m_pEngine->GarbageCollect( asGC_FULL_CYCLE );
}

void CAngelScriptVM::ReleaseScript( HSCRIPT hScript )
{
	if ( hScript )
	{
		((asIScriptModule *)hScript)->Discard();
	}
}

HSCRIPT CAngelScriptVM::CreateScope( const char *pszScope, HSCRIPT hParent )
{
	ScriptScope_t *pScope = m_ScopePool.GetObject();
	if ( !pScope )
		return INVALID_HSCRIPT;

	V_strcpy_safe( pScope->szScopeName, pszScope );
	pScope->pModule = NULL;

	if ( hParent )
	{
		if ( hParent == INVALID_HSCRIPT )
		{
			m_ScopePool.PutObject( pScope );
			return INVALID_HSCRIPT;
		}
		pScope->pParent = (ScriptScope_t *)hParent;
	}

	return (HSCRIPT)pScope;
}

HSCRIPT CAngelScriptVM::ReferenceScope( HSCRIPT hScope )
{
	if ( hScope )
	{
		ScriptScope_t *pScope = m_ScopePool.GetObject();
		if ( !pScope )
			return INVALID_HSCRIPT;

		V_strcpy_safe( pScope->szScopeName, ((ScriptScope_t *)hScope)->szScopeName );
		pScope->pParent = ((ScriptScope_t *)hScope)->pParent;
		pScope->pModule = ((ScriptScope_t *)hScope)->pModule;

		return (HSCRIPT)pScope;
	}
	
	return INVALID_HSCRIPT;
}

void CAngelScriptVM::ReleaseScope( HSCRIPT hScript )
{
	if ( hScript )
	{
		V_memset( hScript, 0, sizeof( ScriptScope_t ) );
		m_ScopePool.PutObject( (ScriptScope_t *)hScript );
	}
}

HSCRIPT CAngelScriptVM::LookupFunction( const char *pszFunction, HSCRIPT hScope )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
		{
			Warning( "Invalid scope passed to LookupFunction!\n" );
			return NULL;
		}

		ScriptScope_t *pScope = (ScriptScope_t *)hScope;

		while ( pScope )
		{
			if ( pScope->pModule )
			{
				asIScriptFunction *pFunc = pScope->pModule->GetFunctionByName( pszFunction );
				if ( pFunc )
				{
					pFunc->AddRef();
					return (HSCRIPT)pFunc;
				}
			}

			pScope = pScope->pParent;
		}
	}

	asUINT numFuncs = m_pEngine->GetGlobalFunctionCount();
	for ( asUINT i = 0; i < numFuncs; ++i )
	{
		asIScriptFunction *pFunc = m_pEngine->GetGlobalFunctionByIndex( i );
		if ( !V_strcmp( pFunc->GetName(), pszFunction ) )
		{
			pFunc->AddRef();
			return (HSCRIPT)pFunc; 
		}
	}

	return NULL;
}

void CAngelScriptVM::ReleaseFunction( HSCRIPT hScript )
{
	if ( hScript )
	{
		((asIScriptFunction *)hScript)->Release();
	}
}

ScriptStatus_t CAngelScriptVM::ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait )
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

	asIScriptFunction *pFunction = reinterpret_cast<asIScriptFunction *>(hFunction);
	asIScriptContext *pCtx = m_ContextMgr.AddContext( m_pEngine, pFunction );
	pCtx->SetLineCallback( asMETHOD( CAngelScriptVM, OnLineCue ), this, asCALL_THISCALL );

	for ( int i = 0; i < nArgs; ++i )
	{
		switch ( pArgs[i].m_type )
		{
			case FIELD_BOOLEAN:
			case FIELD_CHARACTER:
				pCtx->SetArgByte( i, pArgs[i].m_char );
				break;
			case FIELD_FLOAT:
				pCtx->SetArgFloat( i, pArgs[i].m_float );
				break;
			case FIELD_INTEGER:
				pCtx->SetArgDWord( i, pArgs[i].m_int );
				break;
			case FIELD_CSTRING:
				AssertMsg( false, "Need to handle conversion from c-string in function execution" );
				pCtx->SetArgVarType( i, (void *)pArgs[i].m_pszString, m_nStringTypeID );
				break;
			case FIELD_VECTOR:
				pCtx->SetArgVarType( i, (void *)pArgs[i].m_pVector, m_nVectorTypeID );
				break;
			case FIELD_MATRIX3X4:
				pCtx->SetArgVarType( i, pArgs[i].m_pData, m_nMatrixTypeID );
				break;
			case FIELD_QUATERNION:
				pCtx->SetArgVarType( i, pArgs[i].m_pData, m_nQuaternionTypeID );
				break;
			case FIELD_HSCRIPT:
				pCtx->SetArgAddress( i, pArgs[i].m_hScript );
				break;
			default:
				AssertMsg( false, "Unhandled data type(%d) encountered executing function %s", pArgs[i].m_type, pFunction->GetName() );
				break;
		}
	}

	if ( m_ContextMgr.ExecuteScripts() > 0 )
	{
		if ( bWait )
		{
			for ( ;; )
			{
				if ( m_ContextMgr.ExecuteScripts() == 0 )
					break;
			}

			return SCRIPT_DONE;
		}

		return SCRIPT_RUNNING;
	}

	return SCRIPT_DONE;
}

void CAngelScriptVM::RegisterFunction( ScriptFunctionBinding_t *pScriptFunction )
{
	RegisterFunctionGuts( pScriptFunction, NULL );
}

bool CAngelScriptVM::RegisterClass( ScriptClassDesc_t *pClassDesc )
{
	char const *pszClassName = pClassDesc->m_pszScriptName;

	if ( pClassDesc->m_pBaseDesc )
	{
		RegisterClass( pClassDesc->m_pBaseDesc );
	}

	asDWORD flags = asOBJ_REF;
	if ( pClassDesc->m_pszDescription[0] == *SCRIPT_SINGLETON )
		flags |= asOBJ_NOHANDLE;
	int r = m_pEngine->RegisterObjectType( pszClassName, sizeof( CScriptClass ), flags );
	if ( r < 0 )
	{
		if ( r == asALREADY_REGISTERED )
			return true;

		Assert( false );
		return false;
	}

	asITypeInfo *pInfo = m_pEngine->GetTypeInfoById( r );
	m_ClassTypes.FastInsert( (uintptr_t)pInfo, pClassDesc );

	if ( pClassDesc->m_pfnConstruct )
	{
		r = m_pEngine->RegisterObjectBehaviour( pszClassName, asBEHAVE_FACTORY, CFmtStr( "%s@ f()", pszClassName ), asFUNCTION( CScriptClass::Construct ), asCALL_GENERIC );
		if ( r < 0 )
		{
			Assert( false );
			return false;
		}
	}

	if ( !( flags & asOBJ_NOHANDLE ) )
	{
		r = m_pEngine->RegisterObjectBehaviour( pszClassName, asBEHAVE_ADDREF, "void f()", asMETHOD( CScriptClass, AddRef ), asCALL_THISCALL );
		if ( r < 0 )
		{
			Assert( false );
			return false;
		}

		r = m_pEngine->RegisterObjectBehaviour( pszClassName, asBEHAVE_RELEASE, "void f()", asMETHOD( CScriptClass, Release ), asCALL_THISCALL );
		if ( r < 0 )
		{
			Assert( false );
			return false;
		}
	}

	if ( !RegisterClassInheritance( pClassDesc ) )
		return false;

	FOR_EACH_VEC( pClassDesc->m_FunctionBindings, i )
	{
		RegisterFunctionGuts( &pClassDesc->m_FunctionBindings[i], pClassDesc );
	}

	FOR_EACH_VEC( pClassDesc->m_MemberBindings, i )
	{
		ScriptMemberBinding_t &binding = pClassDesc->m_MemberBindings[i];

		char szDecleration[128] = "";
		switch ( binding.m_nMemberType )
		{
			case FIELD_VOID:
				V_strcpy_safe( szDecleration, "void " );
				break;
			case FIELD_CHARACTER:
				V_strcpy_safe( szDecleration, "int8 " );
				break;
			case FIELD_BOOLEAN:
				V_strcpy_safe( szDecleration, "bool " );
				break;
			case FIELD_INTEGER:
				V_strcpy_safe( szDecleration, "int " );
				break;
			case FIELD_FLOAT:
				V_strcpy_safe( szDecleration, "float " );
				break;
			case FIELD_CSTRING:
				AssertMsg( false, "Need to handle conversion from c-string in member declaration" );
				V_strcpy_safe( szDecleration, "string " );
				break;
			case FIELD_VECTOR:
				V_strcpy_safe( szDecleration, "Vector3 " );
				break;
			case FIELD_QUATERNION:
				V_strcpy_safe( szDecleration, "Quaternion " );
				break;
			case FIELD_MATRIX3X4:
				V_strcpy_safe( szDecleration, "Matrix " );
				break;
			case FIELD_HSCRIPT:
				V_strcpy_safe( szDecleration, "ref@ " );
				break;
		}

		V_strcat_safe( szDecleration, binding.m_pszScriptName );
		r = m_pEngine->RegisterObjectProperty( pszClassName, szDecleration, binding.m_unMemberOffs, asOFFSET( CScriptClass, m_pInstance ), true );
		if ( r < 0 )
		{
			Assert( false );
			return false;
		}
	}

	return true;
}

void CAngelScriptVM::RegisterConstant( ScriptConstantBinding_t *pScriptConstant )
{
	// HACKHACK: RegisterProperty fails if the data value is 0, as it needs a valid address
	// to point to in order to reference it, store a dummy reference and give it that
	if ( pScriptConstant->m_data.m_hScript == NULL )
	{
		static int CONST_NULL = 0;
		pScriptConstant->m_data.m_hScript = (HSCRIPT)&CONST_NULL;
	}

	char szDecleration[256] = "const ";
	switch ( pScriptConstant->m_data.m_type )
	{
		case FIELD_INTEGER:
			V_strcat_safe( szDecleration, "int " );
			break;
		case FIELD_BOOLEAN:
			V_strcat_safe( szDecleration, "bool " );
			break;
		case FIELD_CHARACTER:
			V_strcat_safe( szDecleration, "int8 " );
			break;
		case FIELD_FLOAT:
			V_strcat_safe( szDecleration, "float " );
			break;
		case FIELD_VECTOR:
			V_strcat_safe( szDecleration, "Vector3 " );
			break;
		case FIELD_QUATERNION:
			V_strcat_safe( szDecleration, "Quaternion " );
			break;
		case FIELD_MATRIX3X4:
			V_strcat_safe( szDecleration, "Matrix " );
			break;
		case FIELD_CSTRING:
			AssertMsg( false, "Need to handle c-strings in constant registration" );
			V_strcat_safe( szDecleration, "string " );
			break;
		default:
			AssertMsg1( false, "Unhandled data type(%d) passed in global constant registration.", pScriptConstant->m_data.m_type );
			return;
	}

	V_strcat_safe( szDecleration, pScriptConstant->m_pszScriptName );

	Verify( m_pEngine->RegisterGlobalProperty( szDecleration, &pScriptConstant->m_data.m_int ) >= 0 );
}

void CAngelScriptVM::RegisterEnum( ScriptEnumDesc_t *pEnumDesc )
{
	Verify( m_pEngine->RegisterEnum( pEnumDesc->m_pszScriptName ) >= 0 );
	FOR_EACH_VEC( pEnumDesc->m_ConstantBindings, i )
	{
		ScriptConstantBinding_t &constant = pEnumDesc->m_ConstantBindings[i];
		Verify( m_pEngine->RegisterEnumValue( pEnumDesc->m_pszScriptName, constant.m_pszScriptName, constant.m_data ) >= 0 );
	}
}

HSCRIPT CAngelScriptVM::RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance )
{
	if ( !RegisterClass( pDesc ) )
		return NULL;

	return (HSCRIPT)new CScriptClass( pInstance, pDesc );
}

void *CAngelScriptVM::GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType )
{
	if ( hInstance == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return NULL;
	}

	CScriptClass *pScriptInstance = (CScriptClass *)hInstance;
	ScriptClassDesc_t *pDescription = pScriptInstance->GetDesc();
	while ( pDescription )
	{
		if ( pDescription == pExpectedType )
			return pScriptInstance->GetInstance();

		pDescription = pDescription->m_pBaseDesc;
	}

	return NULL;
}

void CAngelScriptVM::RemoveInstance( HSCRIPT hScript )
{
	if ( hScript )
	{
		delete (CScriptClass *)hScript;
	}
}

void CAngelScriptVM::SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId )
{
	if ( hInstance == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return;
	}

	CScriptClass *pScriptInstance = (CScriptClass *)hInstance;
	V_strcpy_safe( pScriptInstance->m_szUniqueId, pszId );
}

bool CAngelScriptVM::GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize )
{
	if ( Q_strlen( pszRoot ) + 41 > nBufSize )
	{
		Error( "GenerateUniqueKey: buffer too small\n" );
		if ( nBufSize != 0 )
			*pBuf = '\0';

		return false;
	}

	V_snprintf( pBuf, nBufSize, "%x%x%llx_%s", RandomInt( 0, 4095 ), Plat_MSTime(), ++m_nUniqueKeySerial, pszRoot );

	return true;
}

bool CAngelScriptVM::ValueExists( HSCRIPT hScope, const char *pszKey )
{
	if ( hScope == NULL )
		return m_pEngine->GetGlobalPropertyIndexByName( pszKey ) >= 0;

	ScriptScope_t *pScope = (ScriptScope_t *)hScope;
	while ( pScope )
	{
		if ( pScope->pModule )
			return pScope->pModule->GetGlobalVarIndexByName( pszKey ) >= 0;

		if ( pScope->pTable )
			return pScope->pTable->Exists( pszKey );

		pScope = pScope->pParent;
	}

	return true;
}

bool CAngelScriptVM::SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue )
{
	Assert( false );
	return false;
}

bool CAngelScriptVM::SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value )
{
	ScriptScope_t *pScope = (ScriptScope_t *)hScope;
	if ( pScope )
	{
		if ( pScope->pTable )
		{
			switch ( value.m_type )
			{
				case FIELD_BOOLEAN:
					pScope->pTable->Set( pszKey, (bool *)&value.m_bool, asTYPEID_BOOL );
				case FIELD_CHARACTER:
					pScope->pTable->Set( pszKey, (char *)&value.m_char, asTYPEID_INT8 );
					return true;
				case FIELD_INTEGER:
					pScope->pTable->Set( pszKey, (int *)&value.m_int, asTYPEID_INT32 );
					return true;
				case FIELD_FLOAT:
					pScope->pTable->Set( pszKey, (float *)&value.m_float, asTYPEID_FLOAT );
					return true;
				case FIELD_VECTOR:
					pScope->pTable->Set( pszKey, (void *)value.m_pVector, m_nVectorTypeID );
					return true;
				case FIELD_QUATERNION:
					pScope->pTable->Set( pszKey, value.m_pData, m_nQuaternionTypeID );
					return true;
				case FIELD_MATRIX3X4:
					pScope->pTable->Set( pszKey, value.m_pData, m_nMatrixTypeID );
					return true;
				case FIELD_CSTRING:
				{
					std::string str;
					str.assign( value.m_pszString );
					pScope->pTable->Set( pszKey, &str, m_nStringTypeID );
					return true;
				}
				case FIELD_HSCRIPT:
				{
					CScriptClass *pClass = (CScriptClass *)value.m_hScript;
					if ( pClass && pClass->GetDesc() )
					{
						int iTypeID = m_pEngine->GetTypeIdByDecl( pClass->GetDesc()->m_pszScriptName );
						if ( iTypeID > 0 )
						{
							pScope->pTable->Set( pszKey, pClass, iTypeID );
							return true;
						}
					}
				}
			}
		}

		if ( pScope->pModule )
		{
			char szDecleration[128] = "";
			switch ( value.m_type )
			{
				case FIELD_VOID:
					V_strcpy_safe( szDecleration, "void " );
					break;
				case FIELD_CHARACTER:
					V_strcpy_safe( szDecleration, "int8 " );
					break;
				case FIELD_BOOLEAN:
					V_strcpy_safe( szDecleration, "bool " );
					break;
				case FIELD_INTEGER:
					V_strcpy_safe( szDecleration, "int " );
					break;
				case FIELD_FLOAT:
					V_strcpy_safe( szDecleration, "float " );
					break;
				case FIELD_CSTRING:
					AssertMsg( false, "Need to handle conversion to c-strings" );
					V_strcpy_safe( szDecleration, "string " );
					break;
				case FIELD_VECTOR:
					V_strcpy_safe( szDecleration, "Vector3 " );
					break;
				case FIELD_QUATERNION:
					V_strcpy_safe( szDecleration, "Quaternion " );
					break;
				case FIELD_MATRIX3X4:
					V_strcpy_safe( szDecleration, "Matrix " );
					break;
				case FIELD_HSCRIPT:
					CScriptClass *pClass = (CScriptClass *)value.m_hScript;
					if ( pClass && pClass->GetDesc() )
					{
						V_strcpy_safe( szDecleration, pClass->GetDesc()->m_pszScriptName );
						V_strcat_safe( szDecleration, " " );
					}
					break;
			}

			V_strcat_safe( szDecleration, pszKey );
			if ( pScope->pModule->CompileGlobalVar( "GlobalVars", szDecleration, 0 ) >= 0 )
			{
				int iGlobIndex = pScope->pModule->GetGlobalVarIndexByName( pszKey );
				if ( iGlobIndex >= 0 )
				{
					*(void **)pScope->pModule->GetAddressOfGlobalVar( iGlobIndex ) = value.m_hScript;

					return true;
				}
			}
		}
	}
	else
	{
		char szDecleration[128] = "";
		switch ( value.m_type )
		{
			case FIELD_VOID:
				V_strcpy_safe( szDecleration, "void " );
				break;
			case FIELD_CHARACTER:
				V_strcpy_safe( szDecleration, "int8 " );
				break;
			case FIELD_BOOLEAN:
				V_strcpy_safe( szDecleration, "bool " );
				break;
			case FIELD_INTEGER:
				V_strcpy_safe( szDecleration, "int " );
				break;
			case FIELD_FLOAT:
				V_strcpy_safe( szDecleration, "float " );
				break;
			case FIELD_CSTRING:
				AssertMsg( false, "Need to handle c-strings in constant registration" );
				V_strcpy_safe( szDecleration, "string " );
				break;
			case FIELD_VECTOR:
				V_strcpy_safe( szDecleration, "Vector3 " );
				break;
			case FIELD_QUATERNION:
				V_strcpy_safe( szDecleration, "Quaternion " );
				break;
			case FIELD_MATRIX3X4:
				V_strcpy_safe( szDecleration, "Matrix " );
				break;
			case FIELD_HSCRIPT:
				CScriptClass *pClass = (CScriptClass *)value.m_hScript;
				if ( pClass && pClass->GetDesc() )
				{
					V_strcpy_safe( szDecleration, pClass->GetDesc()->m_pszScriptName );
					V_strcat_safe( szDecleration, " " );
				}
				break;
		}

		V_strcat_safe( szDecleration, pszKey );
		if ( m_pEngine->RegisterGlobalProperty( szDecleration, value.m_hScript ) >= 0 )
			return true;
	}

	return false;
}

void CAngelScriptVM::CreateTable( ScriptVariant_t &Table )
{
	ScriptScope_t *pScope = m_ScopePool.GetObject();
	if ( pScope == NULL )
	{
		Table = (HSCRIPT)NULL;
		return;
	}

	pScope->pTable = CScriptDictionary::Create( m_pEngine );
	Table = (HSCRIPT)pScope;
}

int CAngelScriptVM::GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue )
{
	ScriptScope_t *pScope = (ScriptScope_t *)hScope;
	AssertMsg( pScope->pTable, "Iteration can only occur on tables!" );
	if ( pScope->pTable )
	{
		int nNexti = 0;
		for ( auto it = pScope->pTable->begin(); it != pScope->pTable->end(); ++it )
		{
			if ( nNexti++ == nIterator )
			{
				const int nLength = it.GetKey().length();
				char *pszString = new char[nLength + 1];
				V_strncpy( pszString, it.GetKey().c_str(), nLength );

				*pKey = pszString;
				pKey->m_flags |= SV_FREE;

				void *pData = NULL;
				if ( it.GetValue( &pData, it.GetTypeId() ) )
				{
					ConvertToVariant( pData, it.GetTypeId(), pValue );
					return nNexti;
				}
			}
		}
	}

	return -1;
}

bool CAngelScriptVM::GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue )
{
	if ( hScope )
	{
		ScriptScope_t *pScope = (ScriptScope_t *)hScope;
		if ( pScope->pTable )
		{
			int iTypeID = pScope->pTable->GetTypeId( pszKey );
			if ( iTypeID < 0 )
				return false;

			void *pData = NULL;
			if ( pScope->pTable->Get( pszKey, pData, iTypeID ) )
			{
				return ConvertToVariant( pData, iTypeID, pValue );
			}
		}

		if ( pScope->pModule )
		{
			int iGlobIndex = pScope->pModule->GetGlobalVarIndexByName( pszKey );
			if ( iGlobIndex >= 0 )
			{
				int iTypeID = 0;
				if ( pScope->pModule->GetGlobalVar( iGlobIndex, NULL, NULL, &iTypeID, NULL ) >= 0 )
				{
					void *pData = pScope->pModule->GetAddressOfGlobalVar( iGlobIndex );
					return ConvertToVariant( pData, iTypeID, pValue );
				}
			}
		}
	}
	else
	{
		int iGlobIndex = m_pEngine->GetGlobalPropertyIndexByName( pszKey );
		if ( iGlobIndex >= 0 )
		{
			int iTypeID = 0;
			void *pData = NULL;
			if ( m_pEngine->GetGlobalPropertyByIndex( iGlobIndex, NULL, NULL, &iTypeID, NULL, NULL, &pData, NULL ) >= 0 )
			{
				return ConvertToVariant( pData, iTypeID, pValue );
			}
		}
	}

	Assert( false );
	return false;
}

void CAngelScriptVM::ReleaseValue( ScriptVariant_t &value )
{
	if ( value.m_type == FIELD_HSCRIPT )
	{
		value.m_flags &= ~SV_FREE;

		((CScriptClass *)value.m_hScript)->Release();
	}

	value.Free();
	value.m_type = FIELD_VOID;
}

bool CAngelScriptVM::ClearValue( HSCRIPT hScope, const char *pszKey )
{
	if ( hScope )
	{
		ScriptScope_t *pScope = (ScriptScope_t *)hScope;
		if ( pScope->pTable )
		{
			return pScope->pTable->Delete( pszKey );
		}

		if ( pScope->pModule )
		{
			int nGlobVar = pScope->pModule->GetGlobalVarIndexByName( pszKey );
			if ( nGlobVar >= 0 )
			{
				return pScope->pModule->RemoveGlobalVar( nGlobVar ) >= 0;
			}
		}
	}

	// TODO: Global properties?
	Assert( false );
	return false;
}

int CAngelScriptVM::GetNumTableEntries( HSCRIPT hScope )
{
	ScriptScope_t *pScope = (ScriptScope_t *)hScope;
	if ( pScope->pTable )
		return pScope->pTable->GetSize();

	return 0;
}

bool CAngelScriptVM::RegisterFunctionGuts( ScriptFunctionBinding_t *pFuncBinding, ScriptClassDesc_t *pClassDesc )
{
	CUtlString pszDecleration = "";
	switch ( pFuncBinding->m_desc.m_ReturnType )
	{
		case FIELD_VOID:
			pszDecleration = "void ";
			break;
		case FIELD_CHARACTER:
			pszDecleration = "int8 ";
			break;
		case FIELD_BOOLEAN:
			pszDecleration = "bool ";
			break;
		case FIELD_INTEGER:
			pszDecleration = "int ";
			break;
		case FIELD_FLOAT:
			pszDecleration = "float ";
			break;
		case FIELD_CSTRING:
			pszDecleration = "string ";
			break;
		case FIELD_VECTOR:
			pszDecleration = "Vector3 ";
			break;
		case FIELD_QUATERNION:
			pszDecleration = "Quaternion ";
			break;
		case FIELD_MATRIX3X4:
			pszDecleration = "Matrix ";
			break;
		case FIELD_HSCRIPT:
			pszDecleration = "ref &";
			break;
		case FIELD_VARIANT:
			pszDecleration = "any @";
			break;
		default:
			AssertMsg1( false, "Unhandled data type(%d) passed in function registration.", pFuncBinding->m_desc.m_ReturnType );
			return false;
	}

	pszDecleration += pFuncBinding->m_desc.m_pszScriptName;
	pszDecleration += "(";

	int nNumParams = pFuncBinding->m_desc.m_Parameters.Count();
	for ( int i = 0; i < nNumParams; ++i )
	{
		ScriptDataType_t type = pFuncBinding->m_desc.m_Parameters[i];
		switch ( type )
		{
			case FIELD_VOID:
				pszDecleration += "void";
				break;
			case FIELD_CHARACTER:
				pszDecleration += "int8";
				break;
			case FIELD_BOOLEAN:
				pszDecleration += "bool";
				break;
			case FIELD_INTEGER:
				pszDecleration += "int";
				break;
			case FIELD_FLOAT:
				pszDecleration += "float";
				break;
			case FIELD_CSTRING:
				pszDecleration += "string";
				break;
			case FIELD_VECTOR:
				pszDecleration += "Vector3";
				break;
			case FIELD_QUATERNION:
				pszDecleration += "Quaternion";
				break;
			case FIELD_MATRIX3X4:
				pszDecleration += "Matrix";
				break;
			case FIELD_HSCRIPT:
				pszDecleration += "ref &in";
				break;
			case FIELD_VARIANT:
				pszDecleration += "any &in";
				break;
			default:
				AssertMsg1( false, "Unhandled data type(%d) passed in function registration.", type );
				return false;
		}

		if ( ( i + 1 ) != nNumParams )
			pszDecleration += ", ";
	}
	pszDecleration += ")";

	ScriptContext_t *pContext = new ScriptContext_t{pFuncBinding, pClassDesc};
	// Store for cleanup
	m_ScriptContexts.AddToTail( pContext );

	int r = 0;
	if ( pClassDesc )
	{
		r = m_pEngine->RegisterObjectMethod( pClassDesc->m_pszScriptName, pszDecleration, asFUNCTION( TranslateCall ), asCALL_GENERIC, pContext );
	}
	else
	{
		r = m_pEngine->RegisterGlobalFunction( pszDecleration, asFUNCTION( TranslateCall ), asCALL_GENERIC, pContext );
	}

	return r >= 0;
}

bool CAngelScriptVM::RegisterClassInheritance( ScriptClassDesc_t *pClassDesc )
{
	ScriptClassDesc_t *pBaseDesc = pClassDesc->m_pBaseDesc;
	while ( pBaseDesc )
	{
		FOR_EACH_VEC( pBaseDesc->m_FunctionBindings, i )
		{
			if ( !RegisterFunctionGuts( &pBaseDesc->m_FunctionBindings[i], pClassDesc ) )
				return false;
		}

		FOR_EACH_VEC( pBaseDesc->m_MemberBindings, i )
		{
			ScriptMemberBinding_t &binding = pBaseDesc->m_MemberBindings[i];

			char szDecleration[128] = "";
			switch ( binding.m_nMemberType )
			{
				case FIELD_VOID:
					V_strcpy_safe( szDecleration, "void " );
					break;
				case FIELD_CHARACTER:
					V_strcpy_safe( szDecleration, "int8 " );
					break;
				case FIELD_BOOLEAN:
					V_strcpy_safe( szDecleration, "bool " );
					break;
				case FIELD_INTEGER:
					V_strcpy_safe( szDecleration, "int " );
					break;
				case FIELD_FLOAT:
					V_strcpy_safe( szDecleration, "float " );
					break;
				case FIELD_CSTRING:
					AssertMsg( false, "Need to handle conversion from c-string in member declaration" );
					V_strcpy_safe( szDecleration, "string " );
					break;
				case FIELD_VECTOR:
					V_strcpy_safe( szDecleration, "Vector3 " );
					break;
				case FIELD_QUATERNION:
					V_strcpy_safe( szDecleration, "Quaternion " );
					break;
				case FIELD_MATRIX3X4:
					V_strcpy_safe( szDecleration, "Matrix " );
					break;
				case FIELD_HSCRIPT:
					V_strcpy_safe( szDecleration, "ref@ " );
					break;
			}

			V_strcat_safe( szDecleration, binding.m_pszScriptName );
			int r = m_pEngine->RegisterObjectProperty( pClassDesc->m_pszScriptName, szDecleration, binding.m_unMemberOffs, asOFFSET( CScriptClass, m_pInstance ), true );
			if ( r < 0 )
			{
				Assert( false );
				return false;
			}
		}

		pBaseDesc = pBaseDesc->m_pBaseDesc;
	}

	return true;
}

ScriptClassDesc_t *CAngelScriptVM::GetClassDescForType( asITypeInfo *pInfo )
{
	UtlHashFastHandle_t nIndex = m_ClassTypes.Find( (uintptr_t)pInfo );
	if( nIndex == m_ClassTypes.InvalidHandle() )
		return nullptr;

	return m_ClassTypes[ nIndex ];
}

asUINT CAngelScriptVM::GetTime( void )
{
	return Plat_MSTime();
}

void *CAngelScriptVM::Alloc( size_t s )
{
	return calloc( 1, s );
}

void CAngelScriptVM::Free( void *p )
{
	free( p );
}

int CAngelScriptVM::OnInclude( char const *pszFromFile, char const *pszSection, CScriptBuilder *pBuilder, void *pvParam )
{
	// HACK: This was the easiest method I could think of to be able to include scripts from the 'scripts/vscript' directory
	CAngelScriptVM *pVM = reinterpret_cast<CAngelScriptVM *>( pvParam );
	asIScriptFunction *pFunc = pVM->m_pEngine->GetGlobalFunctionByDecl( "bool DoIncludeScript(string, ref &in)" );
	if ( pFunc )
	{
		ScriptVariant_t arg = pszFromFile;
		return pVM->ExecuteFunction( (HSCRIPT)pFunc, &arg, 1, NULL, NULL, true );
	}

	return 1;
}

asIScriptContext *CAngelScriptVM::OnRequestContext( asIScriptEngine *engine, void *pvParam )
{
	CAngelScriptVM *pVM = reinterpret_cast<CAngelScriptVM *>( pvParam );

	asIScriptContext *pContext = NULL;
	if ( pVM->m_ContextPool.Count() > 0 )
	{
		pContext = pVM->m_ContextPool.Top();
		pVM->m_ContextPool.Pop();
		return pContext;
	}

	pContext = engine->CreateContext();
	return pContext;
}

void CAngelScriptVM::OnReturnContext( asIScriptEngine *engine, asIScriptContext *pContext, void *pvParam )
{
	CAngelScriptVM *pVM = reinterpret_cast<CAngelScriptVM *>( pvParam );

	if ( pContext )
	{
		pContext->Unprepare();
		pVM->m_ContextPool.Push( pContext );
	}
}

void CAngelScriptVM::TranslateCall( asIScriptGeneric *gen )
{
	CUtlVectorFixed<ScriptVariant_t, MAX_FUNCTION_PARAMS> parameters;

	asIScriptContext *ctx = asGetActiveContext();

	ScriptFunctionBinding_t *pFuncBinding = ( (ScriptContext_t *)gen->GetAuxiliary() )->pFuncBinding;
	auto const &fnParams = pFuncBinding->m_desc.m_Parameters;

	parameters.SetCount( fnParams.Count() );

	const int nArguments = Min( fnParams.Count(), gen->GetArgCount() );
	for ( int i=0; i < nArguments; ++i )
	{
		switch ( fnParams.Element( i ) )
		{
			case FIELD_INTEGER:
			{
				parameters[i] = int( gen->GetArgDWord( i ) );
				break;
			}
			case FIELD_FLOAT:
			{
				parameters[i] = gen->GetArgFloat( i );
				break;
			}
			case FIELD_BOOLEAN:
			{
				parameters[i] = gen->GetArgByte( i ) == 1 ? true : false;
				break;
			}
			case FIELD_CHARACTER:
			{
				parameters[i] = (char)gen->GetArgByte( i );
				break;
			}
			case FIELD_CSTRING:
			{
				std::string *str = (std::string *)gen->GetArgAddress( i );
				if ( str == NULL )
					parameters[i] = "";
				else
					parameters[i] = str->c_str();
				break;
			}
			case FIELD_VECTOR:
			{
				parameters[i] = (Vector *)gen->GetArgAddress( i );
				break;
			}
			case FIELD_QUATERNION:
			{
				parameters[i] = (Quaternion *)gen->GetArgAddress( i );
				break;
			}
			case FIELD_MATRIX3X4:
			{
				parameters[i] = (matrix3x4_t *)gen->GetArgAddress( i );
				break;
			}
			case FIELD_HSCRIPT:
			{
				parameters[i] = (HSCRIPT)gen->GetArgObject( i );
				break;
			}
			default:
			{
				AssertMsg( 0, "Unsupported type" );
				return;
			}
		}
	}

	void *pContext = NULL;
	if ( pFuncBinding->m_flags & SF_MEMBER_FUNC )
	{
		ScriptClassDesc_t *pClassDesc = ( (ScriptContext_t *)gen->GetAuxiliary() )->pClassDesc;
		if ( pClassDesc == NULL )
		{
			ctx->SetException( "Accessed null instance" );
			return;
		}

		CScriptClass *pObject = (CScriptClass *)gen->GetObject();

		IScriptInstanceHelper *pHelper = pClassDesc->pHelper;
		if ( pHelper )
		{
			pContext = pHelper->GetProxied( pObject->GetInstance(), pFuncBinding );
			if ( pContext == NULL )
			{
				ctx->SetException( "Accessed null instance" );
				return;
			}
		}
		else
		{
			pContext = pObject->GetInstance();
			if ( pContext == NULL )
			{
				ctx->SetException( "Accessed null instance" );
				return;
			}
		}
	}

	const bool bHasReturn = pFuncBinding->m_desc.m_ReturnType != FIELD_VOID;

	ScriptVariant_t returnValue;
	pFuncBinding->m_pfnBinding( pFuncBinding->m_pFunction,
								pContext,
								parameters.Base(),
								parameters.Count(),
								bHasReturn ? &returnValue : NULL );

	switch ( returnValue.m_type )
	{
		case FIELD_CHARACTER:
		case FIELD_BOOLEAN:
			gen->SetReturnByte( returnValue.m_char );
			break;
		case FIELD_FLOAT:
		case FIELD_INTEGER:
			gen->SetReturnDWord( returnValue.m_int );
			break;
		case FIELD_VOID:
			break;
		default:
			gen->SetReturnAddress( returnValue.m_hScript );
			break;
	}
}

bool CAngelScriptVM::ConvertToVariant( void *pData, int iTypeID, ScriptVariant_t *pVariant )
{
	switch ( iTypeID )
	{
		case asTYPEID_BOOL:
			*pVariant = *(bool *)pData;
			return true;
		case asTYPEID_FLOAT:
			*pVariant = *(float *)pData;
			return true;
		case asTYPEID_INT8:
			*pVariant = *(char *)pData;
			return true;
		case asTYPEID_INT32:
			*pVariant = *(int *)pData;
			return true;
	}

	if ( iTypeID & asTYPEID_APPOBJECT )
	{
		int nRawTypeID = iTypeID & ~asTYPEID_APPOBJECT;
		if ( nRawTypeID == m_nVectorTypeID )
		{
			Vector *pVector = new Vector( *(Vector *)pData );
			*pVariant = pVector;
			pVariant->m_flags |= SV_FREE;

			return true;
		}
		else if ( nRawTypeID == m_nQuaternionTypeID )
		{
			Quaternion *pQuat = new Quaternion( *(Quaternion *)pData );
			*pVariant = pQuat;
			pVariant->m_flags |= SV_FREE;

			return true;
		}
		else if ( nRawTypeID == m_nMatrixTypeID )
		{
			matrix3x4_t *pMatrix = new matrix3x4_t( *(matrix3x4_t *)pData );
			*pVariant = pMatrix;
			pVariant->m_flags |= SV_FREE;

			return true;
		}
		else if ( nRawTypeID == m_nStringTypeID )
		{
			std::string *str = (std::string *)pData;
			char *pszString = new char[ str->length() ];
			str->copy( pszString, str->length() );

			*pVariant = pszString;
			pVariant->m_flags |= SV_FREE;

			return true;
		}
		else
		{
			((CScriptClass *)pData)->AddRef();
			*pVariant = (HSCRIPT)pData;

			return true;
		}
	}

	return false;
}

void CAngelScriptVM::PrintAny( CScriptAny *pAny )
{
	int iTypeID = pAny->GetTypeId();
	char szValue[128] = "";

	if ( iTypeID == asTYPEID_DOUBLE )
	{
		double dValue = 0.0;
		pAny->Retrieve( dValue );
		V_sprintf_safe( szValue, "%lf", dValue );
	}
	else if ( iTypeID == asTYPEID_INT64 )
	{
		int64 llValue = 0;
		pAny->Retrieve( llValue );
		V_sprintf_safe( szValue, "%lld", llValue );
	}
	else if ( iTypeID == asTYPEID_BOOL )
	{
		bool bValue = false;
		pAny->Retrieve( &bValue, asTYPEID_BOOL );
		V_strcpy_safe( szValue, bValue ? "true" : "false" );
	}
	else
	{
		V_sprintf_safe( szValue, "Cannot coerce type(%d) into a string", iTypeID );
		asGetActiveContext()->SetException( szValue );
	}

	Msg( "%s\n", szValue );
	pAny->Release();
}

void CAngelScriptVM::PrintString( const std::string &str )
{
	Msg( "%s\n", str.c_str() );
}

void CAngelScriptVM::PrintFunc( asSMessageInfo *msg )
{
	const char *pszType = "";
	switch ( msg->type )
	{
		case asMSGTYPE_ERROR:
			pszType = "ERR ";
			break;
		case asMSGTYPE_WARNING:
			pszType = "WARN ";
			break;
		case asMSGTYPE_INFORMATION:
			pszType = "INFO ";
			break;
	}

	Msg( "%s[%d, %d] %s: %s\n", msg->section, msg->row, msg->col, pszType, msg->message );
}

void CAngelScriptVM::OnException( asIScriptContext *ctx )
{
	// TODO: Output
}

void CAngelScriptVM::OnLineCue( asIScriptContext *ctx )
{
	// TODO: Output if debug
}


IScriptVM *CreateAngelScriptVM( void )
{
	return new CAngelScriptVM();
}

void DestroyAngelScriptVM( IScriptVM *pVM )
{
	if ( pVM ) delete assert_cast<CAngelScriptVM *>( pVM );
}