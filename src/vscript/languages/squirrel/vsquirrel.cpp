//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#define _HAS_EXCEPTIONS 0
#include <exception>

#include "tier0/platform.h"
#include "tier1/convar.h"
#include "tier1/utlvector.h"
#include "tier1/utlhash.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "tier1/utlsymbol.h"
#include "basehandle.h"

#include "squirrel.h"
#include "sqstdaux.h"
#include "sqstdstring.h"
#include "sqstdmath.h"
#include "sqstdtime.h"
#include "sqstdblob.h"
#include "sqrdbg.h"
#include "sqobject.h"
#include "sqstate.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqclass.h"
#include "sqstring.h"
#include "squtils.h"
#include "sqarray.h"
#ifdef _WIN32
#include "sqdbgserver.h"
#endif

#include "vscript/ivscript.h"
#include "vscript_init_nut.h"

#include "vsquirrel_math.h"
#include "sq_vmstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef RegisterClass
#undef RegisterClass
#endif

// we don't want bad actors being malicious
extern "C" 
{
	SQRESULT sqstd_loadfile(HSQUIRRELVM,const SQChar*,SQBool)
	{
		return SQ_ERROR;
	}

	SQRESULT sqstd_register_iolib(HSQUIRRELVM)
	{
		return SQ_ERROR;
	}
}

static SQObjectPtr const _null_;

typedef struct
{
	ScriptClassDesc_t *m_pClassDesc;
	void *m_pInstance;
	SQObjectPtr m_instanceUniqueId;
} ScriptInstance_t;

class StackProtector
{
public:
	StackProtector(HSQUIRRELVM v) : _v(v) { _top = sq_gettop( _v ); }
	~StackProtector() { if ( _top != sq_gettop( _v ) ) { Assert( false ); sq_settop( _v, _top ); } }

private:
	SQInteger _top;
	HSQUIRRELVM _v;
};


static SQObject const INVALID_HSQOBJECT = { (SQObjectType)-1, {(SQTable *)-1} };
inline bool operator==( SQObject const &lhs, SQObject const &rhs ) { return lhs._type == rhs._type && _table( lhs ) == _table( rhs ); }
inline bool operator!=( SQObject const &lhs, SQObject const &rhs ) { return lhs._type != rhs._type || _table( lhs ) != _table( rhs ); }

//-----------------------------------------------------------------------------
// Purpose: Squirrel scripting engine implementation
//-----------------------------------------------------------------------------
class CSquirrelVM : public IScriptVM
{
	friend struct SQVM;
public:
	CSquirrelVM( void );

	bool				Init( void );
	void				Shutdown( void );

	bool				Frame( float simTime );

	ScriptLanguage_t	GetLanguage()           { return SL_SQUIRREL; }
	char const			*GetLanguageName()      { return "Squirrel"; }

	ScriptStatus_t		Run( const char *pszScript, bool bWait = true );

	void				AddSearchPath( const char *pszSearchPath ) {}

	void				RemoveOrphanInstances() {}

	HSCRIPT				CompileScript( const char *pszScript, const char *pszId = NULL );
	ScriptStatus_t		Run( HSCRIPT hScript, HSCRIPT hScope = NULL, bool bWait = true );
	ScriptStatus_t		Run( HSCRIPT hScript, bool bWait );
	void				ReleaseScript( HSCRIPT hScript );

	HSCRIPT				CreateScope( const char *pszScope, HSCRIPT hParent = NULL );
	HSCRIPT				ReferenceScope( HSCRIPT hScope );
	void				ReleaseScope( HSCRIPT hScript );

	HSCRIPT				LookupFunction( const char *pszFunction, HSCRIPT hScope = NULL, bool bNoDelegation = false );
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
	int					GetKeyValue2( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue );
	bool				GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue );
	void				ReleaseValue( ScriptVariant_t &value );
	bool				ClearValue( HSCRIPT hScope, const char *pszKey );
	int					GetNumTableEntries( HSCRIPT hScope );

	enum
	{
		SAVE_VERSION = 3
	};
	void				WriteState( CUtlBuffer *pBuffer );
	void				ReadState( CUtlBuffer *pBuffer );
	void				DumpState();

	bool				ConnectDebugger();
	void				DisconnectDebugger();
	void				SetOutputCallback( ScriptOutputFunc_t pFunc ) { m_OutputFunc = pFunc; }
	void				SetErrorCallback( ScriptErrorFunc_t pFunc ) { m_ErrorFunc = pFunc; }
	bool				RaiseException( const char *pszExceptionText );

private:

	CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get( HSCRIPT &hParentObject, const char *pszSlotName, ISquirrelMetamethodDelegate *pDelegate, bool bDeleteDelegateWhenIAmDeleted );
	void DestroySquirrelMetamethod_Get( CSquirrelMetamethodDelegateImpl *pMetaMethodImpl );

	HSQUIRRELVM GetVM( void )   { return m_hVM; }

	static void					ConvertToVariant( HSQUIRRELVM pVM, SQObject const &pValue, ScriptVariant_t *pVariant );
	static void					PushVariant( HSQUIRRELVM pVM, ScriptVariant_t const &pVariant );
	static void					VariantToString( ScriptVariant_t const &Variant, char (&pszString)[512] );

	HSQOBJECT					CreateClass( ScriptClassDesc_t *pClassDesc );
	bool						CreateInstance( ScriptClassDesc_t *pClassDesc, ScriptInstance_t *pInstance, SQRELEASEHOOK fnRelease );

	enum
	{
		MAX_FUNCTION_PARAMS = 14
	};
	void						RegisterFunctionGuts( ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc = NULL );
	void						RegisterDocumentation( ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc = NULL );
	void						RegisterDocumentation( ScriptClassDesc_t *pClassDesc );
	void						RegisterDocumentation( ScriptHook_t *pHook, ScriptClassDesc_t *pClassDesc );
	void						RegisterDocumentation( ScriptEnumDesc_t *pEnumDesc );
	void						RegisterDocumentation( ScriptConstantBinding_t *pConstant );

	HSQOBJECT					LookupObject( char const *szName, HSCRIPT hScope = NULL, bool bRefCount = true );

	//---------------------------------------------------------------------------------------------
	// Squirrel Function Callbacks
	//---------------------------------------------------------------------------------------------

	static SQInteger			CallConstructor( HSQUIRRELVM pVM );
	static SQInteger			ReleaseHook( SQUserPointer data, SQInteger size );
	static SQInteger			ExternalReleaseHook( SQUserPointer data, SQInteger size );
	static SQInteger			InstanceToString( HSQUIRRELVM pVM );
	static SQInteger			InstanceIsValid( HSQUIRRELVM pVM );
	static SQInteger			InstanceGetStub( HSQUIRRELVM pVM );
	static SQInteger			InstanceSetStub( HSQUIRRELVM pVM );
	static SQInteger			TranslateCall( HSQUIRRELVM pVM );
	static SQInteger			GetDeveloper( HSQUIRRELVM pVM );
	static SQInteger			GetFunctionSignature( HSQUIRRELVM pVM );
	static SQInteger			IsWeakRef( HSQUIRRELVM pVM );
	static SQInteger			DumpObject( HSQUIRRELVM pVM );
	static void					PrintFunc( HSQUIRRELVM, const SQChar *, ... );
	static void					ErrorFunc( HSQUIRRELVM, const SQChar *, ... );
	static int					QueryContinue( HSQUIRRELVM );

	//---------------------------------------------------------------------------------------------

	HSQUIRRELVM m_hVM;
	HSQREMOTEDBG m_hDbgSrv;

	CUtlHashFast<SQClass *, CUtlHashFastGenericHash> m_ScriptClasses;

	// A reference to our Vector type to compare to
	HSQOBJECT m_VectorClass;
	HSQOBJECT m_QAngleClass;
	HSQOBJECT m_QuaternionClass;
	HSQOBJECT m_MatrixClass;

	HSQOBJECT m_CreateScopeClosure;
	HSQOBJECT m_ReleaseScopeClosure;

	SQObjectPtr m_ErrorString;

	ConVarRef developer;

	long long m_nUniqueKeySerial;
	float m_flTimeStartedCall;

	ScriptOutputFunc_t m_OutputFunc;
	ScriptErrorFunc_t m_ErrorFunc;

	static SQRegFunction s_ScriptClassDelegates[];

	friend class CSquirrelMetamethodDelegateImpl;
};

inline CSquirrelVM *GetVScript( HSQUIRRELVM pVM )
{
	return static_cast<CSquirrelVM *>( sq_getsharedforeignptr(pVM) );
}

SQRegFunction CSquirrelVM::s_ScriptClassDelegates[] ={
	{ _SC( "constructor" ), CSquirrelVM::CallConstructor,	0, NULL },
	{MM_GET,				CSquirrelVM::InstanceGetStub,	2, ".s"},
	{MM_SET,				CSquirrelVM::InstanceSetStub,	3, ".s."},
	{MM_TOSTRING,			CSquirrelVM::InstanceToString,	1, "."},
	{_SC( "IsValid" ),		CSquirrelVM::InstanceIsValid,	1, "."},
	{NULL,					NULL}
};


CSquirrelVM::CSquirrelVM( void )
	: m_hVM(NULL), developer("developer"), m_nUniqueKeySerial(0), m_hDbgSrv(NULL), m_ErrorFunc(NULL), m_OutputFunc(NULL)
{
	m_VectorClass = _null_;
	m_QuaternionClass = _null_;
	m_MatrixClass = _null_;
	m_CreateScopeClosure = _null_;
	m_ReleaseScopeClosure = _null_;
	m_ErrorString = _null_;
}

bool CSquirrelVM::Init( void )
{
	m_hVM = sq_open( 1024 );
	StackProtector sa( GetVM() );
	
	sq_setsharedforeignptr( GetVM(), this );
	m_hVM->SetQuerySuspendFn( &CSquirrelVM::QueryContinue );
	
	sq_setprintfunc( GetVM(), &CSquirrelVM::PrintFunc, &CSquirrelVM::ErrorFunc );

	if ( IsDebug() || developer.GetInt() )
		sq_enabledebuginfo( GetVM(), SQTrue );

	// Add Constants index for enum and constant registration
	sq_pushconsttable( GetVM() );
	sq_pushstring( GetVM(), "Constants", 9 );
	sq_newtable( GetVM() );
	sq_newslot( GetVM(), -3, SQFalse );
	sq_pop( GetVM(), 1 );

	{
		// register libraries
		sq_pushroottable( GetVM() );

		sqstd_register_stringlib( GetVM() );
		sqstd_register_mathlib( GetVM() );
		sqstd_register_timelib( GetVM() );
		sqstd_register_bloblib( GetVM() );

		sqstd_seterrorhandlers( GetVM() );

		// register root functions
		sq_pushstring( GetVM(), "developer", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::GetDeveloper, 0 );
		sq_setnativeclosurename( GetVM(), -1, "developer" );
		sq_createslot( GetVM(), -3 );
		sq_pushstring( GetVM(), "GetFunctionSignature", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::GetFunctionSignature, 0 );
		sq_setnativeclosurename( GetVM(), -1, "GetFunctionSignature" );
		sq_createslot( GetVM(), -3 );
		sq_pushstring( GetVM(), "IsWeakRef", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::IsWeakRef, 0 );
		sq_setnativeclosurename( GetVM(), -1, "IsWeakRef" );
		sq_createslot( GetVM(), -3 );
		sq_pushstring( GetVM(), "DumpObject", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::DumpObject, 0 );
		sq_setnativeclosurename( GetVM(), -1, "DumpObject" );
		sq_createslot( GetVM(), -3 );

		// pop off root table
		sq_pop( GetVM(), 1 );
	}

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2, false );
	RegisterMathBindings( GetVM() );

	// store a reference to our classes for instancing
	m_VectorClass = LookupObject( "Vector" );
	m_QAngleClass = LookupObject( "QAngle" );
	m_QuaternionClass = LookupObject( "Quaternion" );
	m_MatrixClass = LookupObject( "matrix3x4_t" );

	m_ScriptClasses.Init( 256 );

	Run( (char *)g_Script_init );

	// store a reference to the scope utilities from the init script
	m_CreateScopeClosure = LookupObject( "VSquirrel_OnCreateScope" );
	m_ReleaseScopeClosure = LookupObject( "VSquirrel_OnReleaseScope" );

	return true;
}

void CSquirrelVM::Shutdown( void )
{
	if ( GetVM() )
	{
		sq_collectgarbage( GetVM() );

		// free the root table reference
		sq_pushnull( GetVM() );
		sq_setroottable( GetVM() );

		sq_close( m_hVM );
		m_hVM = NULL;
	}

	DisconnectDebugger();
	m_ScriptClasses.Purge();
}

bool CSquirrelVM::Frame( float simTime )
{
	if ( m_hDbgSrv )
	{
		// process outgoing messages
		sq_rdbg_update( m_hDbgSrv );

		if ( !sq_rdbg_connected( m_hDbgSrv ) )
			DisconnectDebugger();
	}

	return true;
}

ScriptStatus_t CSquirrelVM::Run( const char *pszScript, bool bWait )
{
	HSQOBJECT pObject;
	if ( SQ_FAILED( sq_compilebuffer( GetVM(), pszScript, V_strlen( pszScript ), "unnamed", SQ_CALL_RAISE_ERROR ) ) )
		return SCRIPT_ERROR;

	// a closure is pushed on success
	sq_getstackobj( GetVM(), -1, &pObject );
	sq_addref( GetVM(), &pObject );

	// pop it off
	sq_pop( GetVM(), 1 );

	ScriptStatus_t result = ExecuteFunction( (HSCRIPT)&pObject, NULL, 0, NULL, NULL, bWait );

	sq_release( GetVM(), &pObject );

	return result;
}

ScriptStatus_t CSquirrelVM::Run( HSCRIPT hScript, HSCRIPT hScope, bool bWait )
{
	return ExecuteFunction( hScript, NULL, 0, NULL, hScope, bWait );
}

ScriptStatus_t CSquirrelVM::Run( HSCRIPT hScript, bool bWait )
{
	return ExecuteFunction( hScript, NULL, 0, NULL, NULL, bWait  );
}

HSCRIPT CSquirrelVM::CompileScript( const char *pszScript, const char *pszId )
{
	if ( !pszScript || !pszScript[0] )
		return NULL;

	HSQOBJECT *pObject = NULL;
	if ( SQ_SUCCEEDED( sq_compilebuffer( GetVM(), pszScript, V_strlen( pszScript ), pszId ? pszId : "unnamed", SQ_CALL_RAISE_ERROR ) ) )
	{
		pObject = new SQObject;

		sq_getstackobj( GetVM(), -1, pObject );
		sq_addref( GetVM(), pObject );

		// a result is pushed on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	return (HSCRIPT)pObject;
}

void CSquirrelVM::ReleaseScript( HSCRIPT hScript )
{
	if ( hScript && hScript != INVALID_HSCRIPT )
	{
		sq_release( GetVM(), (HSQOBJECT *)hScript );
		delete (HSQOBJECT *)hScript;
	}
}

HSCRIPT CSquirrelVM::CreateScope( const char *pszScope, HSCRIPT hParent )
{
	// call the utility create function
	sq_pushobject( GetVM(), m_CreateScopeClosure );
	// push parameters
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pszScope, -1 );
	if ( hParent )
	{
		HSQOBJECT &pTable = *(HSQOBJECT *)hParent;
		Assert( hParent != INVALID_HSCRIPT && sq_istable( pTable ) );
		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	// this pops off the parameters automatically
	HSQOBJECT hScope = _null_;
	if ( SQ_SUCCEEDED( sq_call( GetVM(), 3, SQTrue, SQ_CALL_RAISE_ERROR ) ) )
	{
		sq_getstackobj( GetVM(), -1, &hScope );

		// a result is pushed on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	// pop off the closure
	sq_pop( GetVM(), 1 );
	
	// valid return?
	if ( sq_isnull( hScope ) )
		return NULL;
	
	sq_addref( GetVM(), &hScope );

	HSQOBJECT *pObject = new HSQOBJECT;
	pObject->_type = hScope._type;
	pObject->_unVal = hScope._unVal;

	return (HSCRIPT)pObject;
}

HSCRIPT CSquirrelVM::ReferenceScope( HSCRIPT hScope )
{
	if ( hScope )
	{
		sq_addref( GetVM(), (HSQOBJECT *)hScope );

		HSQOBJECT *pObject = new HSQOBJECT;
		pObject->_type = ((HSQOBJECT *)hScope)->_type;
		pObject->_unVal = ((HSQOBJECT *)hScope)->_unVal;

		return (HSCRIPT)pObject;
	}

	return NULL;
}

void CSquirrelVM::ReleaseScope( HSCRIPT hScript )
{
	if ( hScript && hScript != INVALID_HSCRIPT )
	{
		HSQOBJECT *pObject = (HSQOBJECT *)hScript;

		// call the utility release function
		sq_pushobject( GetVM(), m_ReleaseScopeClosure );

		// push parameters
		sq_pushroottable( GetVM() );
		sq_pushobject( GetVM(), *pObject );

		// this pops off the parameters automatically
		sq_call( GetVM(), 2, SQFalse, SQ_CALL_RAISE_ERROR );

		// pop off the closure
		sq_pop( GetVM(), 1 );

		sq_release( GetVM(), pObject );
		delete (HSQOBJECT *)hScript;
	}
}

HSCRIPT CSquirrelVM::LookupFunction( const char *pszFunction, HSCRIPT hScope, bool bNoDelegation )
{
	HSQOBJECT pFunc = LookupObject( pszFunction, hScope );
	// did we find it?
	if ( sq_isnull( pFunc ) )
		return NULL;
	// is it even a function?
	if ( !sq_isclosure( pFunc ) )
	{
		sq_release( GetVM(), &pFunc );
		return NULL;
	}
	
	HSQOBJECT *pObject = new SQObject;
	pObject->_type = OT_CLOSURE;
	pObject->_unVal.pClosure = _closure( pFunc );

	return (HSCRIPT)pObject;
}

void CSquirrelVM::ReleaseFunction( HSCRIPT hScript )
{
	if ( hScript && hScript != INVALID_HSCRIPT )
	{
		sq_release( GetVM(), (HSQOBJECT *)hScript );
		delete (HSQOBJECT *)hScript;
	}
}

ScriptStatus_t CSquirrelVM::ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait )
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

	if ( m_hDbgSrv )
	{
		if ( g_bSqDebugBreak )
		{
			DisconnectDebugger();
			g_bSqDebugBreak = false;
		}
	}

	SQInteger initialTop = sq_gettop( GetVM() );
	HSQOBJECT &pClosure = *(HSQOBJECT *)hFunction;
	sq_pushobject( GetVM(), pClosure );

	// push the parent table to call from
	if ( hScope )
	{
		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
		{
			sq_pop( GetVM(), 1 );
			return SCRIPT_ERROR;
		}

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	if ( pArgs )
	{
		for ( int i=0; i < nArgs; ++i )
			PushVariant( GetVM(), pArgs[i] );
	}

	m_flTimeStartedCall = Plat_FloatTime();
	if ( SQ_FAILED( sq_call( GetVM(), nArgs + 1, pReturn != NULL, SQ_CALL_RAISE_ERROR ) ) )
	{
		// pop off the closure
		sq_pop( GetVM(), 1 );

		if ( pReturn )
			pReturn->m_type = FIELD_VOID;

		m_flTimeStartedCall = 0.0f;

		return SCRIPT_ERROR;
	}

	m_flTimeStartedCall = 0.0f;

	if ( pReturn )
	{
		HSQOBJECT _return;
		sq_getstackobj( GetVM(), -1, &_return );
		ConvertToVariant( GetVM(), _return, pReturn );

		// sq_call pushes a result on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	// pop off the closure
	sq_pop( GetVM(), 1 );

	if ( sq_gettop( GetVM() ) != initialTop )
	{
		Warning( "Callstack mismatch in VScript/Squirrel!\n" );
		Assert( sq_gettop( GetVM() ) == initialTop );
	}

	if ( !sq_isnull( m_ErrorString ) )
	{
		sq_pushobject( GetVM(), m_ErrorString );
		m_ErrorString = _null_;
		sq_throwobject( GetVM() );
		return SCRIPT_ERROR;
	}

	return SCRIPT_DONE;
}

void CSquirrelVM::RegisterFunction( ScriptFunctionBinding_t *pScriptFunction )
{
	StackProtector sa( GetVM() );

	sq_pushroottable( GetVM() );

	RegisterFunctionGuts( pScriptFunction );

	sq_pop( GetVM(), 1 );
}

bool CSquirrelVM::RegisterClass( ScriptClassDesc_t *pClassDesc )
{
	StackProtector sa( GetVM() );

	UtlHashFastHandle_t hndl = m_ScriptClasses.Find( (intp)pClassDesc );
	if ( hndl != m_ScriptClasses.InvalidHandle() )
		return true;

	if ( pClassDesc->m_pBaseDesc )
	{
		RegisterClass( pClassDesc->m_pBaseDesc );
	}

	HSQOBJECT hObject = CreateClass( pClassDesc );
	if ( hObject == INVALID_HSQOBJECT )
		return false;

	sq_pushobject( GetVM(), hObject );

	SQRegFunction *pReg = s_ScriptClassDelegates;
	// register our constructor if we have one
	if ( pClassDesc->m_pfnConstruct )
	{
		sq_pushstring( GetVM(), pReg->name, -1 );
		sq_newclosure( GetVM(), pReg->f, 0 );
		sq_createslot( GetVM(), -3 );
	}

	while ( ( ++pReg)->name != NULL )
	{
		sq_pushstring( GetVM(), pReg->name, -1 );
		sq_newclosure( GetVM(), pReg->f, 0 );
		sq_setnativeclosurename( GetVM(), -1, pReg->name );
		sq_createslot( GetVM(), -3 );
	}

	// register member functions
	FOR_EACH_VEC( pClassDesc->m_FunctionBindings, i )
	{
		RegisterFunctionGuts( &pClassDesc->m_FunctionBindings[i], pClassDesc );
	}

	sq_pop( GetVM(), 1 );

	RegisterDocumentation( pClassDesc );

	m_ScriptClasses.FastInsert( (intp)pClassDesc, _class( hObject ) );
	return true;
}

void CSquirrelVM::RegisterConstant( ScriptConstantBinding_t *pScriptConstant )
{
	// register to the const table so users can't change it
	sq_pushconsttable( GetVM() );
	sq_pushstring( GetVM(), pScriptConstant->m_pszScriptName, -1 );
	PushVariant( GetVM(), pScriptConstant->m_data );
	// add to consts
	sq_newslot( GetVM(), -3, SQFalse );
	// pop off const table
	sq_pop( GetVM(), 1 );

	RegisterDocumentation( pScriptConstant );
}

void CSquirrelVM::RegisterEnum( ScriptEnumDesc_t *pEnumDesc )
{
	// register to the const table
	sq_pushconsttable( GetVM() );
	sq_pushstring( GetVM(), "Constants", 9 );

	if ( SQ_FAILED( sq_get( GetVM(), -2 ) ) )
	{
		sq_pushstring( GetVM(), "Constants", 9 );
		sq_newtable( GetVM() );
		sq_newslot( GetVM(), -3, SQTrue );

		sq_pushstring( GetVM(), "Constants", 9 );
		Verify( SQ_SUCCEEDED( sq_get( GetVM(), -2 ) ) );
	}

	sq_pushstring( GetVM(), pEnumDesc->m_pszScriptName, -1 );
	// Check if name is already taken
	if ( SQ_FAILED( sq_get( GetVM(), -2 ) ) )
	{
		// create a new table to hold the values
		sq_pushstring( GetVM(), pEnumDesc->m_pszScriptName, -1 );
		sq_newtableex( GetVM(), pEnumDesc->m_ConstantBindings.Count() );

		FOR_EACH_VEC( pEnumDesc->m_ConstantBindings, i )
		{
			ScriptConstantBinding_t &constant = pEnumDesc->m_ConstantBindings[i];

			sq_pushstring( GetVM(), constant.m_pszScriptName, -1 );
			PushVariant( GetVM(), constant.m_data );
			// add to table
			sq_newslot( GetVM(), -3, SQFalse );
		}

		// add to consts
		sq_newslot( GetVM(), -3, SQTrue );
	}

	// pop off const tables
	sq_pop( GetVM(), 2 );

	RegisterDocumentation( pEnumDesc );
}

HSCRIPT CSquirrelVM::RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance )
{
	StackProtector sa( GetVM() );
	if ( !RegisterClass( pDesc ) )
		return NULL;

	ScriptInstance_t *pScriptInstance = new ScriptInstance_t;
	pScriptInstance->m_pClassDesc = pDesc;
	pScriptInstance->m_pInstance = pInstance;

	if ( !CreateInstance( pDesc, pScriptInstance, &CSquirrelVM::ExternalReleaseHook ) )
	{
		delete pScriptInstance;
		return NULL;
	}

	HSQOBJECT *pObject = new SQObject;
	sq_getstackobj( GetVM(), -1, pObject );
	sq_addref( GetVM(), pObject );

	sq_pop( GetVM(), 1 );

	return (HSCRIPT)pObject;
}

void CSquirrelVM::SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId )
{
	if ( hInstance == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return;
	}

	HSQOBJECT &pObject = *(HSQOBJECT *)hInstance;
	if ( !sq_isinstance( pObject ) )
		return;

	ScriptInstance_t *pInstance = (ScriptInstance_t *)_instance( pObject )->_userpointer;
	pInstance->m_instanceUniqueId = SQString::Create( _ss( GetVM() ), pszId, V_strlen( pszId ) );
}

void CSquirrelVM::RemoveInstance( HSCRIPT hScript )
{
	if ( hScript == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return;
	}

	HSQOBJECT &pObject = *(HSQOBJECT *)hScript;
	if ( sq_isinstance( pObject ) )
		_instance( pObject )->_userpointer = NULL;

	sq_release( GetVM(), &pObject );
	delete (HSQOBJECT *)hScript;
}

void *CSquirrelVM::GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType )
{
	if ( hInstance == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return NULL;
	}

	HSQOBJECT &pObject = *(HSQOBJECT *)hInstance;
	if ( !sq_isinstance( pObject ) )
		return NULL;

	ScriptInstance_t *pInstance = (ScriptInstance_t *)_instance( pObject )->_userpointer;

	ScriptClassDesc_t *pDescription = pInstance->m_pClassDesc;
	while ( pDescription )
	{
		if ( pDescription == pExpectedType )
			return pInstance->m_pInstance;

		pDescription = pDescription->m_pBaseDesc;
	}

	return NULL;
}

bool CSquirrelVM::GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize )
{
	if( Q_strlen(pszRoot) + 41 > nBufSize )
	{
		Error( "GenerateUniqueKey: buffer too small\n" );
		if ( nBufSize != 0 )
			*pBuf = '\0';

		return false;
	}

	V_snprintf( pBuf, nBufSize, "%x%x%llx_%s", RandomInt(0, 4095), Plat_MSTime(), ++m_nUniqueKeySerial, pszRoot );

	return true;
}

bool CSquirrelVM::ValueExists( HSCRIPT hScope, const char *pszKey )
{
	return !sq_isnull( LookupObject( pszKey, hScope, false ) );
}

bool CSquirrelVM::SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue )
{
	StackProtector sa( GetVM() );
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return false;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), pszKey, -1 );
	sq_pushstring( GetVM(), pszValue, -1 );
	sq_createslot( GetVM(), -3 );

	sq_pop( GetVM(), 1 );

	return true;
}

bool CSquirrelVM::SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value )
{
	StackProtector sa( GetVM() );
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return false;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), pszKey, -1 );

	if ( value.m_type == FIELD_HSCRIPT && value.m_hScript )
	{
		HSQOBJECT &pObject = *(HSQOBJECT *)value.m_hScript;
		if ( sq_isinstance( pObject ) && _instance( pObject )->_class->_typetag != VECTOR_TYPE_TAG )
		{
			ScriptInstance_t *pInstance = (ScriptInstance_t *)_instance( pObject )->_userpointer;
			if ( sq_isnull( pInstance->m_instanceUniqueId ) )
			{
				// if we haven't been given a unique ID, we'll be assigned the key name
				sq_getstackobj( GetVM(), -1, &pInstance->m_instanceUniqueId );
			}
		}
	}

	PushVariant( GetVM(), value );
	sq_createslot( GetVM(), -3 );

	sq_pop( GetVM(), 1 );

	return true;
}

void CSquirrelVM::CreateTable( ScriptVariant_t &Table )
{
	HSQOBJECT hObject = INVALID_HSQOBJECT;
	sq_newtable( GetVM() );

	sq_getstackobj( GetVM(), -1, &hObject );
	sq_addref( GetVM(), &hObject );

	ConvertToVariant( GetVM(), hObject, &Table );

	sq_pop( GetVM(), 1 );
}

int CSquirrelVM::GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue )
{
	StackProtector sa( GetVM() );

	HSQOBJECT pKeyObj, pValueObj;
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return -1;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return -1;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushinteger( GetVM(), nIterator );
	if ( SQ_FAILED( sq_next( GetVM(), -2 ) ) )
	{
		sq_pop( GetVM(), 2 );
		return -1;
	}

	sq_getstackobj( GetVM(), -2, &pKeyObj );
	sq_getstackobj( GetVM(), -1, &pValueObj );
	sq_addref( GetVM(), &pKeyObj );
	sq_addref( GetVM(), &pValueObj );

	// sq_next pushes 2 objects onto the stack, so pop them off too
	sq_pop( GetVM(), 2 );

	ConvertToVariant( GetVM(), pKeyObj, pKey );
	ConvertToVariant( GetVM(), pValueObj, pValue );

	// The next index is set by reference in sq_next, so retrieve it here
	SQInteger nNexti = -1;
	sq_getinteger( GetVM(), -1, &nNexti );

	// Pop index and table
	sq_pop( GetVM(), 2 );

	return nNexti;
}

int CSquirrelVM::GetKeyValue2( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue )
{
	StackProtector sa( GetVM() );

	HSQOBJECT pKeyObj, pValueObj;
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return -1;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return -1;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushinteger( GetVM(), nIterator );
	if ( SQ_FAILED( sq_next( GetVM(), -2 ) ) )
	{
		sq_pop( GetVM(), 2 );
		return -1;
	}

	sq_getstackobj( GetVM(), -2, &pKeyObj );
	sq_getstackobj( GetVM(), -1, &pValueObj );
	sq_addref( GetVM(), &pKeyObj );
	sq_addref( GetVM(), &pValueObj );

	// sq_next pushes 2 objects onto the stack, so pop them off too
	sq_pop( GetVM(), 2 );

	ConvertToVariant( GetVM(), pKeyObj, pKey );
	ConvertToVariant( GetVM(), pValueObj, pValue );

	// The next index is set by reference in sq_next, so retrieve it here
	SQInteger nNexti = -1;
	sq_getinteger( GetVM(), -1, &nNexti );

	// Pop index and table
	sq_pop( GetVM(), 2 );

	return nNexti;
}

bool CSquirrelVM::GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue )
{
	HSQOBJECT pObject = LookupObject( pszKey, hScope );
	ConvertToVariant( GetVM(), pObject, pValue );

	return !sq_isnull( pObject );
}

void CSquirrelVM::ReleaseValue( ScriptVariant_t &value )
{
	if( value.m_type == FIELD_HSCRIPT )
		sq_release( GetVM(), (HSQOBJECT *)value.m_hScript );
	
	value.Free();
	value.m_type = FIELD_VOID;
}

bool CSquirrelVM::ClearValue( HSCRIPT hScope, const char *pszKey )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return false;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), pszKey, -1 );
	sq_deleteslot( GetVM(), -2, SQFalse );

	sq_pop( GetVM(), 1 );

	return true;
}

int CSquirrelVM::GetNumTableEntries( HSCRIPT hScope )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return 0;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return 0;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	const int nEntries = sq_getsize( GetVM(), -1 );

	sq_pop( GetVM(), 1 );

	return nEntries;
}

void CSquirrelVM::WriteState( CUtlBuffer *pBuffer )
{
#ifndef VSQUIRREL_TEST
	pBuffer->PutInt( SAVE_VERSION );
	pBuffer->PutInt64( m_nUniqueKeySerial );

	WriteSquirrelState( GetVM(), pBuffer );
#endif
}

void CSquirrelVM::ReadState( CUtlBuffer *pBuffer )
{
#ifndef VSQUIRREL_TEST
	if ( pBuffer->GetInt() != SAVE_VERSION )
	{
		DevMsg( "Incompatible script version\n" );
		return;
	}

	int64 serial = pBuffer->GetInt64();
	m_nUniqueKeySerial = Max( m_nUniqueKeySerial, serial );

	ReadSquirrelState( GetVM(), pBuffer );
#endif
}

void CSquirrelVM::DumpState()
{
#ifndef VSQUIRREL_TEST
	DumpSquirrelState( GetVM() );
#endif
}

bool CSquirrelVM::ConnectDebugger()
{
#ifdef _WIN32
	if( developer.GetInt() == 0 )
		return false;

	if ( m_hDbgSrv == NULL )
	{
		const int serverPort = 1234;
		m_hDbgSrv = sq_rdbg_init( GetVM(), serverPort, SQTrue );
	}

	if ( m_hDbgSrv == NULL )
		return false;

	// !WARN!
	// This will hold up the main thread until connection is established
	return SQ_SUCCEEDED( sq_rdbg_waitforconnections( m_hDbgSrv ) );
#else
	return false;
#endif
}

void CSquirrelVM::DisconnectDebugger()
{
#ifdef _WIN32
	if ( m_hDbgSrv )
	{
		sq_rdbg_shutdown( m_hDbgSrv );
		m_hDbgSrv = NULL;
	}
#endif
}

bool CSquirrelVM::RaiseException( const char *pszExceptionText )
{
	m_ErrorString = SQString::Create( _ss( GetVM() ), pszExceptionText );

	return true;
}

class CSquirrelMetamethodDelegateImpl
{
public:
	CSquirrelMetamethodDelegateImpl(CSquirrelVM *pVM, HSQOBJECT hParentObject, CUtlSymbol const &symSlotName, IScriptVM::ISquirrelMetamethodDelegate *pDelegate, bool bDeleteDelegateWhenIAmDeleted) : m_pVM(pVM), m_hParentObject(hParentObject), m_symSlotName(symSlotName), m_pDelegate(pDelegate), m_bDeleteDelegateWhenIAmDeleted(bDeleteDelegateWhenIAmDeleted)
	{
		sq_addref( m_pVM->GetVM(), &hParentObject);
	}

	~CSquirrelMetamethodDelegateImpl()
	{
		sq_pushobject( m_pVM->GetVM(), m_hParentObject );
		sq_pushstring( m_pVM->GetVM(), m_symSlotName.String(), -1 );
		if ( SQ_SUCCEEDED( sq_get( m_pVM->GetVM(), -2 ) ) )
		{
			sq_pushnull( m_pVM->GetVM() );
			sq_setdelegate( m_pVM->GetVM(), -2 );
		}
		sq_poptop( m_pVM->GetVM() );
		sq_pushstring( m_pVM->GetVM(), m_symSlotName.String(), -1 );
		sq_pushnull( m_pVM->GetVM() );
		sq_set( m_pVM->GetVM(), -3 );
		sq_pop( m_pVM->GetVM(), 1 );

		if ( m_bDeleteDelegateWhenIAmDeleted )
		{
			if ( m_pDelegate )
				delete m_pDelegate;
		}

		sq_release( m_pVM->GetVM(), &m_hParentObject );
	}

	static SQInteger SqGetMetamethodThunk( HSQUIRRELVM pVM )
	{
		SQUserPointer p = NULL;
		if ( SQ_FAILED( sq_getuserpointer( pVM, 3, &p ) ) )
			return sq_throwerror( pVM, "Bad user pointer passed to a ISquirrelMetamethodDelegate _get()" );

		const SQChar *s = NULL;
		if ( SQ_FAILED( sq_getstring( pVM, 2, &s ) ) )
			return sq_throwerror( pVM, "Bad key string passed to a ISquirrelMetamethodDelegate _get()" );

		CSquirrelMetamethodDelegateImpl *pDelegate = (CSquirrelMetamethodDelegateImpl *)p;
		if ( pVM != pDelegate->m_pVM->GetVM() )
			return sq_throwerror( pVM, "key not found in _get()" );

		ScriptVariant_t variant;
		if ( !pDelegate->m_pVM->GetValue( (HSCRIPT)&pDelegate->m_hParentObject, s, &variant ) )
			return sq_throwerror( pVM, "key not found in _get()" );

		CSquirrelVM::PushVariant( pVM, variant );
		return SQ_OK;
	}

private:
	CSquirrelVM *m_pVM;
	HSQOBJECT m_hParentObject;
	CUtlSymbol m_symSlotName;
	IScriptVM::ISquirrelMetamethodDelegate *m_pDelegate;
	bool m_bDeleteDelegateWhenIAmDeleted;
};

CSquirrelMetamethodDelegateImpl *CSquirrelVM::MakeSquirrelMetamethod_Get( HSCRIPT &hParentObject, const char *pszSlotName, ISquirrelMetamethodDelegate *pDelegate, bool bDeleteDelegateWhenIAmDeleted )
{
	StackProtector sa( GetVM() );

	sq_pushobject( GetVM(), *(HSQOBJECT *)hParentObject );
	sq_pushstring( GetVM(), pszSlotName, -1 );
	if ( SQ_FAILED( sq_get( GetVM(), -2 ) ) )
		return nullptr;

	CSquirrelMetamethodDelegateImpl *pMetamethodDelegate = new CSquirrelMetamethodDelegateImpl( this, *(HSQOBJECT *)hParentObject, pszSlotName, pDelegate, bDeleteDelegateWhenIAmDeleted );

	sq_newtable( GetVM() );
	sq_pushstring( GetVM(), "_get", -1 );
	sq_pushuserpointer( GetVM(), pMetamethodDelegate );
	sq_newclosure( GetVM(), &CSquirrelMetamethodDelegateImpl::SqGetMetamethodThunk, 1 );
	if ( SQ_FAILED( sq_newslot( GetVM(), -3, SQFalse ) ) )
	{
		delete pMetamethodDelegate;
		return nullptr;
	}

	sq_pushstring( GetVM(), "cppdelegate", -1 );
	sq_pushuserpointer( GetVM(), pMetamethodDelegate );
	if ( SQ_FAILED( sq_newslot( GetVM(), -3, SQFalse ) ) )
	{
		delete pMetamethodDelegate;
		return nullptr;
	}
	sq_setdelegate( GetVM(), -2 );
	sq_pop( GetVM(), 2 );

	return pMetamethodDelegate;
}

void CSquirrelVM::DestroySquirrelMetamethod_Get( CSquirrelMetamethodDelegateImpl *pMetaMethodImpl )
{
	if ( pMetaMethodImpl )
		delete pMetaMethodImpl;
}

void CSquirrelVM::ConvertToVariant( HSQUIRRELVM pVM, HSQOBJECT const &pValue, ScriptVariant_t *pVariant )
{
	switch ( sq_type( pValue ) )
	{
		case OT_INTEGER:
		{
			*pVariant = (uint64)_integer( pValue );
			break;
		}
		case OT_FLOAT:
		{
			*pVariant = _float( pValue );
			break;
		}
		case OT_BOOL:
		{
			*pVariant = _integer( pValue ) != 0;
			break;
		}
		case OT_STRING:
		{
			const int nLength = _string( pValue )->_len + 1;
			char *pString = new char[ nLength ];

			*pVariant = pString;
			V_memcpy( (void *)pVariant->m_pszString, _stringval( pValue ), nLength );
			pVariant->m_flags |= SV_FREE;

			break;
		}
		case OT_EHANDLE:
		{
			*pVariant = _ehandle( pValue );
			break;
		}
		case OT_NULL:
		{
			pVariant->m_type = FIELD_VOID;
			break;
		}
		case OT_INSTANCE:
		{
			sq_pushobject( pVM, pValue );

			SQUserPointer pInstance = NULL;

			SQRESULT nResult = sq_getinstanceup( pVM, -1, &pInstance, VECTOR_TYPE_TAG, SQFalse );
			if ( nResult == SQ_OK )
			{
				*pVariant = new Vector();
				V_memcpy( pVariant->m_pData, pInstance, sizeof( Vector ) );
				pVariant->m_flags |= SV_FREE;

				sq_pop( pVM, 1 );
				break;
			}

			nResult = sq_getinstanceup( pVM, -1, &pInstance, QANGLE_TYPE_TAG, SQFalse );
			if ( nResult == SQ_OK )
			{
				*pVariant = new QAngle();
				V_memcpy( pVariant->m_pData, pInstance, sizeof( QAngle ) );
				pVariant->m_flags |= SV_FREE;

				sq_pop( pVM, 1 );
				break;
			}

			nResult = sq_getinstanceup( pVM, -1, &pInstance, QUATERNION_TYPE_TAG, SQFalse );
			if ( nResult == SQ_OK )
			{
				*pVariant = new Quaternion();
				V_memcpy( pVariant->m_pData, pInstance, sizeof( Quaternion ) );
				pVariant->m_flags |= SV_FREE;

				sq_pop( pVM, 1 );
				break;
			}

			nResult = sq_getinstanceup( pVM, -1, &pInstance, QUATERNION_TYPE_TAG, SQFalse );
			if ( nResult == SQ_OK )
			{
				*pVariant = new matrix3x4_t();
				V_memcpy( pVariant->m_pData, pInstance, sizeof( matrix3x4_t ) );
				pVariant->m_flags |= SV_FREE;

				sq_pop( pVM, 1 );
				break;
			}

			// Fall through here
		}
		default:
		{
			HSQOBJECT *pObject = new SQObject;
			pObject->_type = pValue._type;
			pObject->_unVal = pValue._unVal;

			*pVariant = (HSCRIPT)pObject;
			pVariant->m_flags |= SV_FREE;

			break;
		}
	}
}

void CSquirrelVM::PushVariant( HSQUIRRELVM pVM, ScriptVariant_t const &Variant )
{
	switch ( Variant.m_type )
	{
		case FIELD_INTEGER:
		{
			sq_pushinteger( pVM, Variant.m_int );
			break;
		}
		case FIELD_FLOAT:
		{
			sq_pushfloat( pVM, Variant.m_float );
			break;
		}
		case FIELD_BOOLEAN:
		{
			sq_pushbool( pVM, Variant.m_bool );
			break;
		}
		case FIELD_CHARACTER:
		{
			sq_pushinteger( pVM, Variant.m_char );
			break;
		}
		case FIELD_CSTRING:
		{
			char const *szString = Variant.m_pszString ? Variant.m_pszString : "";
			sq_pushstring( pVM, szString, V_strlen( szString ) );

			break;
		}
		case FIELD_EHANDLE:
		{
			sq_pushehandle( pVM, CBaseHandle::UnsafeFromIndex(Variant.m_hEntity) );
			break;
		}
		case FIELD_HSCRIPT:
		{
			if ( Variant.m_hScript )
				sq_pushobject( pVM, *(HSQOBJECT *)Variant.m_hScript );
			else
				sq_pushnull( pVM );

			break;
		}
		case FIELD_VECTOR:
		{
			sq_pushobject( pVM, GetVScript( pVM )->m_VectorClass );
			sq_createinstance( pVM, -1 );

			Vector *pVector = NULL;
			sq_getinstanceup( pVM, -1, (SQUserPointer *)&pVector, NULL, SQFalse );
			V_memcpy( pVector, Variant.m_pVector, sizeof(Vector) );

			// Remove the class object from stack so we are aligned
			sq_remove( pVM, -2 );

			break;
		}
		case FIELD_QANGLE:
		{
			sq_pushobject( pVM, GetVScript( pVM )->m_QAngleClass );
			sq_createinstance( pVM, -1 );

			QAngle *pAngle = NULL;
			sq_getinstanceup( pVM, -1, (SQUserPointer *)&pAngle, NULL, SQFalse );
			V_memcpy( pAngle, Variant.m_pData, sizeof(QAngle) );

			// Remove the class object from stack so we are aligned
			sq_remove( pVM, -2 );

			break;
		}
		case FIELD_QUATERNION:
		{
			sq_pushobject( pVM, GetVScript( pVM )->m_QuaternionClass );
			sq_createinstance( pVM, -1 );

			Quaternion *pQuat = NULL;
			sq_getinstanceup( pVM, -1, (SQUserPointer *)&pQuat, NULL, SQFalse );
			V_memcpy( pQuat, Variant.m_pData, sizeof(Quaternion) );

			// Remove the class object from stack so we are aligned
			sq_remove( pVM, -2 );

			break;
		}
		case FIELD_MATRIX3X4:
		{
			sq_pushobject( pVM, GetVScript( pVM )->m_MatrixClass );
			sq_createinstance( pVM, -1 );

			matrix3x4_t *pMatrix = NULL;
			sq_getinstanceup( pVM, -1, (SQUserPointer *)&pMatrix, NULL, SQFalse );
			V_memcpy( pMatrix, Variant.m_pData, sizeof(matrix3x4_t) );

			// Remove the class object from stack so we are aligned
			sq_remove( pVM, -2 );

			break;
		}
		default:
		{
			sq_pushnull( pVM );
			break;
		}
	}
}

void CSquirrelVM::VariantToString( ScriptVariant_t const &Variant, char( &szValue )[512] )
{
	switch ( Variant.m_type )
	{
		case FIELD_VOID:
			V_strncpy( szValue, "null", sizeof( szValue ) );
			break;
		case FIELD_FLOAT:
			V_snprintf( szValue, sizeof( szValue ), "%f", Variant.m_float );
			break;
		case FIELD_CSTRING:
			V_snprintf( szValue, sizeof( szValue ), "\"%s\"", Variant.m_pszString );
			break;
		case FIELD_VECTOR:
			V_snprintf( szValue, sizeof( szValue ), "Vector( %f, %f, %f )", Variant.m_pVector->x, Variant.m_pVector->y, Variant.m_pVector->z );
			break;
		case FIELD_QANGLE:
			QAngle m_pAngle;
			Variant.AssignTo( &m_pAngle );
			V_snprintf( szValue, sizeof( szValue ), "QAngle( %f, %f, %f )", m_pAngle.x, m_pAngle.y, m_pAngle.z );
			break;
		case FIELD_QUATERNION:
			Quaternion m_pQuat;
			Variant.AssignTo( &m_pQuat );
			V_snprintf( szValue, sizeof( szValue ), "Quaternion( %f, %f, %f, %f )", m_pQuat.x, m_pQuat.y, m_pQuat.z, m_pQuat.w );
			break;
		case FIELD_MATRIX3X4:
			matrix3x4_t m_pMatrix;
			Variant.AssignTo( &m_pMatrix );
			V_snprintf( szValue, sizeof( szValue ), "matrix3x4_t( %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f )", 
						m_pMatrix[0][0], m_pMatrix[0][1], m_pMatrix[0][2], m_pMatrix[0][3], 
						m_pMatrix[1][0], m_pMatrix[1][1], m_pMatrix[1][2], m_pMatrix[1][3], 
						m_pMatrix[2][0], m_pMatrix[2][1], m_pMatrix[2][2], m_pMatrix[2][3] );
			break;
		case FIELD_INTEGER:
			V_snprintf( szValue, sizeof( szValue ), "%i", Variant.m_int );
			break;
		case FIELD_BOOLEAN:
			V_snprintf( szValue, sizeof( szValue ), "%d", Variant.m_bool );
			break;
		case FIELD_CHARACTER:
			//char buf[2] = { value.m_char, 0 };
			V_snprintf( szValue, sizeof( szValue ), "%c", Variant.m_char );
			break;
	}
}

HSQOBJECT CSquirrelVM::CreateClass( ScriptClassDesc_t *pClassDesc )
{
	int nArgs = sq_gettop( GetVM() );

	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pClassDesc->m_pszScriptName, -1 );

	bool bHasBase = false;

	ScriptClassDesc_t *pBase = pClassDesc->m_pBaseDesc;
	if ( pBase )
	{
		sq_pushstring( GetVM(), pBase->m_pszScriptName, -1 );
		if ( SQ_FAILED( sq_get( GetVM(), -3 ) ) )
		{
			sq_settop( GetVM(), nArgs );
			return INVALID_HSQOBJECT;
		}

		bHasBase = true;
	}

	if ( SQ_FAILED( sq_newclass( GetVM(), bHasBase ) ) )
	{
		sq_settop( GetVM(), nArgs );
		return INVALID_HSQOBJECT;
	}

	sq_settypetag( GetVM(), -1, pClassDesc );

	HSQOBJECT pObject = _null_;
	sq_getstackobj( GetVM(), -1, &pObject );
	sq_addref( GetVM(), &pObject );
	
	sq_createslot( GetVM(), -3 );
	sq_pop( GetVM(), 1 );

	return pObject;
}

bool CSquirrelVM::CreateInstance( ScriptClassDesc_t *pClassDesc, ScriptInstance_t *pInstance, SQRELEASEHOOK fnRelease )
{
	UtlHashFastHandle_t nIndex = m_ScriptClasses.Find( (intp)pClassDesc );
	if ( nIndex == m_ScriptClasses.InvalidHandle() )
		return false;

	SQObjectPtr hClass( m_ScriptClasses[ nIndex ] );
	sq_pushobject( GetVM(), hClass );
	if ( SQ_FAILED( sq_createinstance( GetVM(), -1 ) ) )
	{
		sq_pop( GetVM(), 1 );
		return false;
	}

	// remove class object to align stack
	sq_remove( GetVM(), -2 );

	if ( SQ_FAILED( sq_setinstanceup( GetVM(), -1, pInstance ) ) )
		return false;

	sq_setreleasehook( GetVM(), -1, fnRelease );

	return true;
}

void CSquirrelVM::RegisterFunctionGuts( ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc )
{
	StackProtector sa( GetVM() );
	if ( pFunction->m_desc.m_Parameters.Count() > MAX_FUNCTION_PARAMS )
	{
		AssertMsg( 0, "Too many agruments provided for script function %s\n", pFunction->m_desc.m_pszFunction );
		return;
	}

	char szParamCheck[ MAX_FUNCTION_PARAMS+1 ]{0};
	szParamCheck[0] = '.';

	char *pCurrent = &szParamCheck[1];
	for ( int i=0; i < pFunction->m_desc.m_Parameters.Count(); ++i )
	{
		switch ( pFunction->m_desc.m_Parameters[i] )
		{
			case FIELD_INTEGER:
			case FIELD_FLOAT:
			{
				*pCurrent++ = 'n';
				break;
			}
			case FIELD_BOOLEAN:
			{
				*pCurrent++ = 'b';
				break;
			}
			case FIELD_VECTOR:
			case FIELD_QANGLE:
			case FIELD_QUATERNION:
			case FIELD_MATRIX3X4:
			{
				*pCurrent++ = 'x';
				break;
			}
			case FIELD_CSTRING:
			{
				*pCurrent++ = 's';
				break;
			}
			case FIELD_HSCRIPT:
			{
				*pCurrent++ = '.';
				break;
			}
			case FIELD_VARIANT:
			{
				*pCurrent++ = 'V';
				break;
			}
			case FIELD_EHANDLE:
			{
				*pCurrent++ = 'h';
				break;
			}
			default:
			{
				AssertMsg( 0 , "Unsupported type" );
				return;
			}
		}
	}

	// null terminate
	*pCurrent++ = '\0';
	
	sq_pushstring( GetVM(), pFunction->m_desc.m_pszScriptName, -1 );
	sq_pushuserpointer( GetVM(), pFunction );
	sq_newclosure( GetVM(), &CSquirrelVM::TranslateCall, 1 );
	sq_setnativeclosurename( GetVM(), -1, pFunction->m_desc.m_pszScriptName );
	sq_setparamscheck( GetVM(), pFunction->m_desc.m_Parameters.Count() + 1, szParamCheck );

	sq_createslot( GetVM(), -3 );

	RegisterDocumentation( pFunction, pClassDesc );
}

void CSquirrelVM::RegisterDocumentation( ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc )
{
	StackProtector sa( GetVM() );
	if ( pFunction->m_desc.m_pszDescription && *pFunction->m_desc.m_pszDescription == *SCRIPT_HIDE )
		return;

	char szName[128]{};
	if ( pClassDesc )
	{
		V_strcat_safe( szName, pClassDesc->m_pszScriptName );
		V_strcat_safe( szName, "::" );
	}
	V_strcat_safe( szName, pFunction->m_desc.m_pszScriptName );

	char szSignature[512]{};
	V_strcat_safe( szSignature, ScriptFieldTypeName( pFunction->m_desc.m_ReturnType ) );
	V_strcat_safe( szSignature, " " );
	V_strcat_safe( szSignature, szName );
	V_strcat_safe( szSignature, "(" );
	FOR_EACH_VEC( pFunction->m_desc.m_Parameters, i )
	{
		if ( i != 0 )
			V_strcat_safe( szSignature, ", " );

		V_strcat_safe( szSignature, ScriptFieldTypeName( pFunction->m_desc.m_Parameters[i] ) );
	}
	V_strcat_safe( szSignature, ")" );

	HSQOBJECT pRegisterDocumentation = LookupObject( "RegisterFunctionDocumentation", NULL, false );
	sq_pushobject( GetVM(), pRegisterDocumentation );
	// push our parameters
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), szName, -1 );
	sq_pushstring( GetVM(), szSignature, -1 );
	sq_pushstring( GetVM(), pFunction->m_desc.m_pszDescription, -1 );
	// call the function and pop the parameters
	sq_call( GetVM(), 4, SQFalse, SQ_CALL_RAISE_ERROR );
	// pop off closure
	sq_pop( GetVM(), 1 );
}

void CSquirrelVM::RegisterDocumentation( ScriptClassDesc_t *pClassDesc )
{
	StackProtector sa( GetVM() );
	char szBaseClass[512] = "";
	if ( pClassDesc->m_pBaseDesc )
		V_strcpy_safe( szBaseClass, pClassDesc->m_pBaseDesc->m_pszScriptName );

	sq_newtableex( GetVM(), pClassDesc->m_MemberBindings.Count() );
	FOR_EACH_VEC( pClassDesc->m_MemberBindings, i )
	{
		char szMemberName[512] = "";
		V_strcat_safe( szMemberName, pClassDesc->m_pszScriptName );
		V_strcat_safe( szMemberName, "::" );
		V_strcat_safe( szMemberName, pClassDesc->m_MemberBindings[i].m_pszScriptName );
		sq_pushstring( GetVM(), szMemberName, -1 );
		sq_newarray( GetVM(), 2 );

		char szMemberSignature[512];
		V_sprintf_safe( szMemberSignature, "%s %s%s;", 
						ScriptFieldTypeName( pClassDesc->m_MemberBindings[i].m_nMemberType ),
						pClassDesc->m_MemberBindings[i].m_nMemberType == FIELD_HSCRIPT ? "@" : "",
						pClassDesc->m_MemberBindings[i].m_pszScriptName );
		sq_pushstring( GetVM(), szMemberSignature, -1 );
		sq_arrayinsert( GetVM(), -2, 0 );

		sq_pushstring( GetVM(), pClassDesc->m_MemberBindings[i].m_pszDescription, -1 );
		sq_arrayinsert( GetVM(), -2, 1 );

		sq_createslot( GetVM(), -3 );
	}

	HSQOBJECT hTable = _null_;
	sq_getstackobj( GetVM(), -1, &hTable );
	sq_addref( GetVM(), &hTable );

	sq_pop( GetVM(), 1 );

	FOR_EACH_VEC( pClassDesc->m_Hooks, i )
	{
		RegisterDocumentation( &pClassDesc->m_Hooks[i], pClassDesc );
	}

	sq_pushobject( GetVM(), LookupObject( "RegisterClassDocumentation", NULL, false ) );
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pClassDesc->m_pszScriptName, -1 );
	sq_pushstring( GetVM(), szBaseClass, -1 );
	sq_pushobject( GetVM(), hTable );
	sq_pushstring( GetVM(), pClassDesc->m_pszDescription, -1 );
	sq_call( GetVM(), 5, SQFalse, SQ_CALL_RAISE_ERROR );

	sq_pop( GetVM(), 1 );
}

void CSquirrelVM::RegisterDocumentation( ScriptHook_t *pHook, ScriptClassDesc_t *pClassDesc )
{
	StackProtector sa( GetVM() );
	if ( pHook->m_func.m_desc.m_pszDescription && *pHook->m_func.m_desc.m_pszDescription == *SCRIPT_HIDE )
		return;

	char szName[128] = "";
	if ( pClassDesc )
	{
		V_strcat_safe( szName, pClassDesc->m_pszScriptName );
		V_strcat_safe( szName, "::" );
	}
	V_strcat_safe( szName, pHook->m_func.m_desc.m_pszScriptName );

	char szSignature[512] = "";
	V_strcat_safe( szSignature, ScriptFieldTypeName( pHook->m_func.m_desc.m_ReturnType ) );
	V_strcat_safe( szSignature, " " );
	V_strcat_safe( szSignature, szName );
	V_strcat_safe( szSignature, "(" );
	FOR_EACH_VEC( pHook->m_func.m_desc.m_Parameters, i )
	{
		if ( i != 0 )
			V_strcat_safe( szSignature, ", " );

		V_strcat_safe( szSignature, ScriptFieldTypeName( pHook->m_func.m_desc.m_Parameters[i] ) );
	}
	V_strcat_safe( szSignature, ")" );

	sq_pushobject( GetVM(), LookupObject( "RegisterHookDocumentation", NULL, false ) );
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), szName, -1 );
	sq_pushstring( GetVM(), szSignature, -1 );
	sq_pushstring( GetVM(), pHook->m_func.m_desc.m_pszDescription, -1 );
	sq_call( GetVM(), 4, SQFalse, SQ_CALL_RAISE_ERROR );
	
	sq_pop( GetVM(), 1 );
}

void CSquirrelVM::RegisterDocumentation( ScriptEnumDesc_t *pEnumDesc )
{
	StackProtector sa( GetVM() );
	if ( pEnumDesc->m_pszDescription && *pEnumDesc->m_pszDescription == *SCRIPT_HIDE )
		return;

	sq_newtableex( GetVM(), pEnumDesc->m_ConstantBindings.Count() );
	FOR_EACH_VEC( pEnumDesc->m_ConstantBindings, i )
	{
		char szName[512];
		V_sprintf_safe( szName, "%s", pEnumDesc->m_ConstantBindings[i].m_pszScriptName );
		sq_pushstring( GetVM(), szName, -1 );
		sq_newarray( GetVM(), 2 );
		
		char szValue[512];
		VariantToString( pEnumDesc->m_ConstantBindings[i].m_data, szValue );
		sq_pushstring( GetVM(), szValue, -1 );
		sq_arrayinsert( GetVM(), -2, 0 );

		sq_pushstring( GetVM(), pEnumDesc->m_ConstantBindings[i].m_pszDescription, -1 );
		sq_arrayinsert( GetVM(), -2, 1 );

		sq_createslot( GetVM(), -3 );
	}
	
	HSQOBJECT hTable = _null_;
	sq_getstackobj( GetVM(), -1, &hTable );
	sq_addref( GetVM(), &hTable );

	sq_pop( GetVM(), 1 );

	sq_pushobject( GetVM(), LookupObject( "RegisterEnumDocumentation", NULL, false ) );
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pEnumDesc->m_pszScriptName, -1 );
	sq_pushobject( GetVM(), hTable );
	sq_pushstring( GetVM(), pEnumDesc->m_pszDescription, -1 );
	sq_call( GetVM(), 4, SQFalse, SQ_CALL_RAISE_ERROR );
	
	sq_pop( GetVM(), 1 );
}

void CSquirrelVM::RegisterDocumentation( ScriptConstantBinding_t *pConstDesc )
{
	StackProtector sa( GetVM() );
	if ( pConstDesc->m_pszDescription && pConstDesc->m_pszDescription[0] == SCRIPT_HIDE[0] )
		return;

	char szValue[512];
	VariantToString( pConstDesc->m_data, szValue );

	sq_pushobject( GetVM(), LookupObject( "RegisterConstDocumentation", NULL, false ) );
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pConstDesc->m_pszScriptName, -1 );
	sq_pushstring( GetVM(), ScriptFieldTypeName( pConstDesc->m_data.m_type ), -1 );
	sq_pushstring( GetVM(), szValue, -1 );
	sq_pushstring( GetVM(), pConstDesc->m_pszDescription, -1 );
	sq_call( GetVM(), 5, SQFalse, SQ_CALL_RAISE_ERROR );
	
	sq_pop( GetVM(), 1 );
}

HSQOBJECT CSquirrelVM::LookupObject( char const *szName, HSCRIPT hScope, bool bRefCount )
{
	StackProtector sa( GetVM() );

	HSQOBJECT pObject = _null_;
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return _null_;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return _null_;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), szName, -1 );
	if ( SQ_SUCCEEDED( sq_get( GetVM(), -2 ) ) )
	{
		sq_getstackobj( GetVM(), -1, &pObject );
		if ( bRefCount )
			sq_addref( GetVM(), &pObject );

		sq_pop( GetVM(), 1 );
	}

	sq_pop( GetVM(), 1 );

	return pObject;
}

SQInteger CSquirrelVM::GetDeveloper( HSQUIRRELVM pVM )
{
	sq_pushinteger( pVM, GetVScript( pVM )->developer.GetInt() );
	return 1;
}

SQInteger CSquirrelVM::GetFunctionSignature( HSQUIRRELVM pVM )
{
	static char szSignature[512];

	if ( sq_gettop( pVM ) != 3 )
		return 0;

	HSQOBJECT pObject = _null_;
	sq_getstackobj( pVM, 2, &pObject );
	if ( !sq_isclosure( pObject ) )
		return 0;

	V_memset( szSignature, 0, sizeof szSignature );

	char const *pszName = NULL;
	sq_getstring( pVM, 1, &pszName );
	SQClosure *pClosure = _closure( pObject );
	SQFunctionProto *pPrototype = pClosure->_function;

	char const *pszFuncName;
	if ( pszName && *pszName )
	{
		pszFuncName = pszName;
	}
	else
	{
		pszFuncName = sq_isstring( pPrototype->_name ) ? _stringval( pPrototype->_name ) : "<unnamed>";
	}

	V_strncat( szSignature, "function ", sizeof szSignature );
	V_strncat( szSignature, pszFuncName, sizeof szSignature );
	V_strncat( szSignature, "(", sizeof szSignature );

	if ( pPrototype->_nparameters >= 2 )
	{
		for ( int i=1; i < pPrototype->_nparameters; ++i )
		{
			if ( i != 1 )
				V_strncat( szSignature, ", ", sizeof szSignature );

			char const *pszArgument = sq_isstring( pPrototype->_parameters[i] ) ? _stringval( pPrototype->_parameters[i] ) : "arg";
			V_strncat( szSignature, pszArgument, sizeof szSignature );
		}
	}

	V_strncat( szSignature, ")", sizeof szSignature );

	sq_pushstring( pVM, szSignature, -1 );
	return 1;
}

SQInteger CSquirrelVM::IsWeakRef( HSQUIRRELVM pVM )
{
	int nTop = sq_gettop( pVM );
	if ( nTop != 3 )
		return 0;

	HSQOBJECT pObject;
	sq_resetobject( &pObject );
	sq_getstackobj( pVM, 2, &pObject );

	HSQOBJECT pKey;
	sq_resetobject( &pKey );
	sq_getstackobj( pVM, 3, &pKey );

	SQObjectPtr ref;
	if ( sq_type( pObject ) == OT_TABLE )
	{
		_table( pObject )->GetIncludingWeakref( pKey, ref );
	}
	else if ( sq_type( pObject ) == OT_ARRAY && sq_type( pKey ) == OT_INTEGER )
	{
		_array( pObject )->Get( _integer( pKey ), ref );
	}

	sq_pushbool( pVM, sq_type( ref ) == OT_WEAKREF );
	return 1;
}

SQInteger CSquirrelVM::DumpObject( HSQUIRRELVM pVM )
{
	int nTop = sq_gettop( pVM );
	if ( nTop != 2 )
		return 0;

	SQObjectPtr pObject;
	sq_resetobject( &pObject );
	sq_getstackobj( pVM, 2, &pObject );

	CSQStateIterator iter( pVM );
	iter.Value( pObject );

	iter.BeginContained();
	IterateObject( &iter, sq_type( pObject ), pObject );
	iter.EndContained();

	return 1;
}

SQInteger CSquirrelVM::TranslateCall( HSQUIRRELVM pVM )
{
	CUtlVectorFixed<ScriptVariant_t, MAX_FUNCTION_PARAMS> parameters;

	ScriptFunctionBinding_t *pFuncBinding = NULL;
	sq_getuserpointer( pVM, sq_gettop( pVM ), (SQUserPointer *)&pFuncBinding );
	auto const &fnParams = pFuncBinding->m_desc.m_Parameters;

	parameters.SetCount( fnParams.Count() );

	const int nArguments = min<int>( fnParams.Count(), sq_gettop( pVM ) );
	for ( int i=0; i < nArguments; ++i )
	{
		switch ( fnParams.Element( i ) )
		{
			case FIELD_INTEGER:
			{
				SQInteger n = 0;
				if ( SQ_FAILED( sq_getinteger( pVM, i+2, &n ) ) )
					return sqstd_throwerrorf( pVM, "Integer argument expected at argument %d", i );

				parameters[i] = (int)n;
				break;
			}
			case FIELD_FLOAT:
			{
				SQFloat f = 0.0;
				if ( SQ_FAILED( sq_getfloat( pVM, i+2, &f ) ) )
					return sqstd_throwerrorf( pVM, "Float argument expected at argument %d", i );

				parameters[i] = f;
				break;
			}
			case FIELD_BOOLEAN:
			{
				SQBool b = SQFalse;
				if ( SQ_FAILED( sq_getbool( pVM, i+2, &b ) ) )
					return sqstd_throwerrorf( pVM, "Bool argument expected at argument %d", i );

				parameters[i] = b == SQTrue;
				break;
			}
			case FIELD_CHARACTER:
			{
				SQInteger n = 0;
				if ( SQ_FAILED( sq_getinteger( pVM, i+2, &n ) ) )
					return sqstd_throwerrorf( pVM, "Integer argument expected at argument %d", i );

				parameters[i] = (char)n;
				break;
			}
			case FIELD_CSTRING:
			{
				char const *pszString = NULL;
				if ( SQ_FAILED( sq_getstring( pVM, i+2, &pszString ) ) )
					return sqstd_throwerrorf( pVM, "String argument expected at argument %d", i );

				parameters[i] = pszString;
				break;
			}
			case FIELD_VECTOR:
			{
				SQUserPointer pInstance = NULL;
				sq_getinstanceup( pVM, i+2, &pInstance, VECTOR_TYPE_TAG, SQFalse );
				if ( pInstance == NULL )
					return sqstd_throwerrorf( pVM, "Vector argument expected at argument %d", i );

				parameters[i] = (Vector *)pInstance;
				break;
			}
			case FIELD_QANGLE:
			{
				SQUserPointer pInstance = NULL;
				sq_getinstanceup( pVM, i+2, &pInstance, QANGLE_TYPE_TAG, SQFalse );
				if ( pInstance == NULL )
					return sqstd_throwerrorf( pVM, "QAngle argument expected at argument %d", i );

				parameters[i] = (QAngle *)pInstance;
				break;
			}
			case FIELD_QUATERNION:
			{
				SQUserPointer pInstance = NULL;
				sq_getinstanceup( pVM, i+2, &pInstance, QUATERNION_TYPE_TAG, SQFalse );
				if ( pInstance == NULL )
					return sqstd_throwerrorf( pVM, "Quaternion argument expected at argument %d", i );

				parameters[i] = (Quaternion *)pInstance;
				break;
			}
			case FIELD_MATRIX3X4:
			{
				SQUserPointer pInstance = NULL;
				sq_getinstanceup( pVM, i+2, &pInstance, MATRIX_TYPE_TAG, SQFalse );
				if ( pInstance == NULL )
					return sqstd_throwerrorf( pVM, "Matrix argument expected at argument %d", i );

				parameters[i] = (matrix3x4_t *)pInstance;
				break;
			}
			case FIELD_EHANDLE:
			{
				CBaseHandle h;
				if ( SQ_FAILED( sq_getehandle( pVM, i + 2, &h ) ) )
					return sqstd_throwerrorf( pVM, "EHandle argument expected at argument %d", i );

				parameters[i] = h;
				break;
			}
			case FIELD_HSCRIPT:
			{
				HSQOBJECT pObject = _null_;
				if ( SQ_FAILED( sq_getstackobj( pVM, i+2, &pObject ) ) )
					return sqstd_throwerrorf( pVM, "Handle argument expected at argument %d", i );

				if ( sq_isnull( pObject ) )
				{
					parameters[i] = (HSCRIPT)NULL;
				}
				else
				{
					HSQOBJECT *pScript = new SQObject;
					pScript->_type = pObject._type;
					pScript->_unVal = pObject._unVal;

					parameters[i] = (HSCRIPT)pScript;
					parameters[i].m_flags |= SV_FREE;
				}

				break;
			}
			case FIELD_VARIANT:
			{
				HSQOBJECT pObject = _null_;
				sq_getstackobj( pVM, i+2, &pObject );

				ConvertToVariant( pVM, pObject, &parameters[i] );
				break;
			}
			default:
			{
				AssertMsg( 0, "Unsupported type" );
				return false;
			}
		}
	}

	SQUserPointer pContext = NULL;
	if ( pFuncBinding->m_flags & SF_MEMBER_FUNC )
	{
		ScriptInstance_t *pInstance = NULL;
		sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL, SQFalse );
		if ( pInstance == NULL || pInstance->m_pInstance == NULL )
			return sq_throwerror( pVM, "Accessed null instance" );

		IScriptInstanceHelper *pHelper = pInstance->m_pClassDesc->pHelper;
		if ( pHelper )
		{
			pContext = pHelper->GetProxied( pInstance->m_pInstance, pFuncBinding );
			if ( pContext == NULL )
				return sq_throwerror( pVM, "Accessed null instance" );
		}
		else
		{
			pContext = pInstance->m_pInstance;
		}
	}

	const bool bHasReturn = pFuncBinding->m_desc.m_ReturnType != FIELD_VOID;

	ScriptVariant_t returnValue;
	pFuncBinding->m_pfnBinding( pFuncBinding->m_pFunction, 
								pContext, 
								parameters.Base(), 
								parameters.Count(), 
								bHasReturn ? &returnValue : NULL );

	for ( int i=0; i < parameters.Count(); ++i )
	{
		parameters[i].Free();
	}

	HSQOBJECT &hErrorString = GetVScript( pVM )->m_ErrorString;
	if ( !sq_isnull( hErrorString ) )
	{
		sq_pushobject( pVM, hErrorString );
		GetVScript( pVM )->m_ErrorString = _null_;
		return sq_throwobject( pVM );
	}

	if ( bHasReturn )
	{
		PushVariant( pVM, returnValue );
	}

	return (SQBool)bHasReturn;
}

SQInteger CSquirrelVM::CallConstructor( HSQUIRRELVM pVM )
{
	ScriptClassDesc_t *pClassDesc = NULL;
	sq_gettypetag( pVM, 1, (SQUserPointer *)&pClassDesc );
	if ( !pClassDesc->m_pfnConstruct )
	{
		return sqstd_throwerrorf( pVM, "Unable to construct instances of %s.", pClassDesc->m_pszScriptName );
	}

	void *pvInstance = pClassDesc->m_pfnConstruct();

	HSQOBJECT &hErrorString = GetVScript( pVM )->m_ErrorString;
	if ( !sq_isnull( hErrorString ) )
	{
		sq_pushobject( pVM, hErrorString );
		GetVScript( pVM )->m_ErrorString = _null_;
		return sq_throwobject( pVM );
	}

	ScriptInstance_t *pInstance = new ScriptInstance_t;
	pInstance->m_pClassDesc = pClassDesc;
	pInstance->m_pInstance = pvInstance;

	sq_setinstanceup( pVM, 1, pInstance );
	sq_setreleasehook( pVM, 1, &CSquirrelVM::ReleaseHook );

	return SQ_OK;
}

SQInteger CSquirrelVM::ReleaseHook( SQUserPointer data, SQInteger size )
{
	ScriptInstance_t *pObject = (ScriptInstance_t *)data;
	if ( pObject->m_pClassDesc->m_pfnDestruct )
	{
		pObject->m_pClassDesc->m_pfnDestruct( pObject->m_pInstance );
		delete pObject;
	}

	return SQ_OK;
}

SQInteger CSquirrelVM::ExternalReleaseHook( SQUserPointer data, SQInteger size )
{
	delete (ScriptInstance_t *)data;
	return SQ_OK;
}

SQInteger CSquirrelVM::InstanceToString( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL, SQFalse );
	if ( pInstance && pInstance->m_pInstance )
	{
		IScriptInstanceHelper *pHelper = pInstance->m_pClassDesc->pHelper;
		if ( pHelper )
		{
			char szInstance[128] = "";
			if ( pHelper->ToString( pInstance->m_pInstance, szInstance, sizeof( szInstance ) ) )
			{
				sq_pushstring( pVM, szInstance, -1 );
				return 1;
			}
		}

		sqstd_pushstringf( pVM, "(%s : 0x%p)", pInstance->m_pClassDesc->m_pszScriptName, pInstance->m_pInstance );
		return 1;
	}

	SQObjectPtr hObject;
	sq_getstackobj( pVM, 1, &hObject );
	sqstd_pushstringf( pVM, "(%s : 0x%p)", GetTypeName( hObject ), _instance( hObject ) );
	return 1;
}

SQInteger CSquirrelVM::InstanceIsValid( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL, SQFalse );
	sq_pushbool( pVM, pInstance && pInstance->m_pInstance );
	return 1;
}

SQInteger CSquirrelVM::InstanceGetStub( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL, SQFalse );

	const SQChar *pString = NULL;
	if ( SQ_FAILED( sq_getstring( pVM, 2, &pString ) ) )
		return sq_throwerror( pVM, "Expected _get( string )" );

	auto const &members = pInstance->m_pClassDesc->m_MemberBindings;
	auto pvInstance = pInstance->m_pInstance;
	if( pvInstance == NULL )
		return sq_throwerror( pVM, "Null instance." );

	FOR_EACH_VEC( members, i )
	{
		if ( V_strcmp( members[i].m_pszScriptName, pString ) == 0 )
		{
			ptrdiff_t const nOffset = members[i].m_unMemberOffs;
			uint32 const nSize = members[i].m_unMemberSize;
			switch ( members[i].m_nMemberType )
			{
				case FIELD_INTEGER:
				{
					sq_pushinteger( pVM, *(int *)( (uintp)pvInstance + nOffset ) );
					return 1;
				}
				case FIELD_FLOAT:
				{
					sq_pushfloat( pVM, *(float *)( (uintp)pvInstance + nOffset ) );
					return 1;
				}
				case FIELD_BOOLEAN:
				{
					sq_pushbool( pVM, *(bool *)( (uintp)pvInstance + nOffset ) );
					return 1;
				}
				case FIELD_CSTRING:
				{
					SQChar sString[1024];
					V_memcpy( sString, (void *)( (uintp)pvInstance + nOffset ), nSize );
					sq_pushstring( pVM, sString, sizeof( sString ) );
					return 1;
				}
				case FIELD_VECTOR:
				{
					sq_pushobject( pVM, GetVScript( pVM )->m_VectorClass );
					sq_createinstance( pVM, -1 );

					Vector *pVector = NULL;
					sq_getinstanceup( pVM, -1, (SQUserPointer *)&pVector, NULL, SQFalse );
					V_memcpy( pVector, (void *)( (uintp)pvInstance + nOffset ), nSize );

					// Remove the class object from stack so we are aligned
					sq_remove( pVM, -2 );
					return 1;
				}
				case FIELD_QANGLE:
				{
					sq_pushobject( pVM, GetVScript( pVM )->m_QAngleClass );
					sq_createinstance( pVM, -1 );

					QAngle *pAngle = NULL;
					sq_getinstanceup( pVM, -1, (SQUserPointer *)&pAngle, NULL, SQFalse );
					V_memcpy( pAngle, (void *)( (uintp)pvInstance + nOffset ), nSize );

					// Remove the class object from stack so we are aligned
					sq_remove( pVM, -2 );
					return 1;
				}
				case FIELD_QUATERNION:
				{
					sq_pushobject( pVM, GetVScript( pVM )->m_QuaternionClass );
					sq_createinstance( pVM, -1 );

					Quaternion *pQuat = NULL;
					sq_getinstanceup( pVM, -1, (SQUserPointer *)&pQuat, NULL, SQFalse );
					V_memcpy( pQuat, (void *)( (uintp)pvInstance + nOffset ), nSize );

					// Remove the class object from stack so we are aligned
					sq_remove( pVM, -2 );
					return 1;
				}
				case FIELD_MATRIX3X4:
				{
					sq_pushobject( pVM, GetVScript( pVM )->m_MatrixClass );
					sq_createinstance( pVM, -1 );

					matrix3x4_t *pMatrix = NULL;
					sq_getinstanceup( pVM, -1, (SQUserPointer *)&pMatrix, NULL, SQFalse );
					V_memcpy( pMatrix, (void *)( (uintp)pvInstance + nOffset ), nSize );

					// Remove the class object from stack so we are aligned
					sq_remove( pVM, -2 );
					return 1;
				}
				default:
					return sqstd_throwerrorf( pVM, "Unsupported data type (%s).", ScriptFieldTypeName( members[i].m_nMemberType ) );
			}
		}
	}

	return 0;
}

SQInteger CSquirrelVM::InstanceSetStub( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL, SQFalse );

	const SQChar *pString = NULL;
	if ( SQ_FAILED( sq_getstring( pVM, 2, &pString ) ) )
		return sq_throwerror( pVM, "Expected _set( string, value )" );

	auto const &members = pInstance->m_pClassDesc->m_MemberBindings;
	auto pvInstance = pInstance->m_pInstance;
	if ( pvInstance == NULL )
		return sq_throwerror( pVM, "Null instance." );

	FOR_EACH_VEC( members, i )
	{
		if ( V_strcmp( members[i].m_pszScriptName, pString ) == 0 )
		{
			ptrdiff_t nOffset = members[i].m_unMemberOffs;
			size_t nSize = members[i].m_unMemberSize;
			switch ( members[i].m_nMemberType )
			{
				case FIELD_INTEGER:
				{
					SQInteger n = 0;
					if ( SQ_FAILED( sq_getinteger( pVM, 3, &n ) ) )
						return sq_throwerror( pVM, "Expected _set( string, integer )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &n, nSize );
					break;
				}
				case FIELD_FLOAT:
				{
					SQFloat f = 0.0;
					if ( SQ_FAILED( sq_getfloat( pVM, 3, &f ) ) )
						return sq_throwerror( pVM, "Expected _set( string, float )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &f, nSize );
					break;
				}
				case FIELD_BOOLEAN:
				{
					SQBool b = SQFalse;
					if ( SQ_FAILED( sq_getbool( pVM, 3, &b ) ) )
						return sq_throwerror( pVM, "Expected _set( string, boolean )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &b, nSize );
					break;
				}
				case FIELD_CHARACTER:
				{
					SQInteger n = 0;
					if ( SQ_FAILED( sq_getinteger( pVM, 3, &n ) ) )
						return sq_throwerror( pVM, "Expected _set( string, integer )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &n, nSize );
					break;
				}
				case FIELD_CSTRING:
				{
					char const *pszString = "";
					if ( SQ_FAILED( sq_getstring( pVM, 3, &pszString ) ) )
						return sq_throwerror( pVM, "Expected _set( string, string )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), pszString, nSize );
					break;
				}
				case FIELD_VECTOR:
				{
					SQUserPointer p = NULL;
					sq_getinstanceup( pVM, 3, &p, VECTOR_TYPE_TAG, SQFalse );
					if ( p == NULL )
						return sq_throwerror( pVM, "Expected _set( string, Vector )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), p, nSize );
					break;
				}
				case FIELD_QANGLE:
				{
					SQUserPointer p = NULL;
					sq_getinstanceup( pVM, 3, &p, QANGLE_TYPE_TAG, SQFalse );
					if ( p == NULL )
						return sq_throwerror( pVM, "Expected _set( string, QAngle )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), p, nSize );
					break;
				}
				case FIELD_QUATERNION:
				{
					SQUserPointer p = NULL;
					sq_getinstanceup( pVM, 3, &p, QUATERNION_TYPE_TAG, SQFalse );
					if ( p == NULL )
						return sq_throwerror( pVM, "Expected _set( string, Quaternion )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), p, nSize );
					break;
				}
				case FIELD_MATRIX3X4:
				{
					SQUserPointer p = NULL;
					sq_getinstanceup( pVM, 3, &p, MATRIX_TYPE_TAG, SQFalse );
					if ( p == NULL )
						return sq_throwerror( pVM, "Expected _set( string, matrix3x4_t )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), p, nSize );
					break;
				}
				case FIELD_HSCRIPT:
				{
					HSQOBJECT hObject = _null_;
					if ( SQ_FAILED( sq_getstackobj( pVM, 3, &hObject ) ) )
						return sq_throwerror( pVM, "Expected _set( string, Handle )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &_rawval( hObject ), nSize );
					break;
				}
				default:
					return sqstd_throwerrorf( pVM, "Unsupported data type (%s).", ScriptFieldTypeName( members[i].m_nMemberType ) );
			}
		}
	}

	return SQ_OK;
}

void CSquirrelVM::PrintFunc( HSQUIRRELVM pVM, const SQChar *fmt, ... )
{
	static char szMessage[2048]{};

	va_list va;
	va_start( va, fmt );
	V_vsprintf_safe( szMessage, fmt, va );
	va_end( va );

	ScriptOutputFunc_t fnOutput = GetVScript( pVM )->m_OutputFunc;
	if ( fnOutput )
	{
		fnOutput( szMessage );
	}
	else
	{
		Msg( "%s\n", szMessage );
	}
}

void CSquirrelVM::ErrorFunc( HSQUIRRELVM pVM, const SQChar *fmt, ... )
{
	static char szMessage[2048]{};

	va_list va;
	va_start( va, fmt );
	V_vsprintf_safe( szMessage, fmt, va );
	va_end( va );

	ScriptErrorFunc_t fnError = GetVScript( pVM )->m_ErrorFunc;
	if ( fnError )
	{
		fnError( SCRIPT_LEVEL_ERROR, szMessage );
	}
	else
	{
		Warning( "%s\n", szMessage );
	}
}

int CSquirrelVM::QueryContinue( HSQUIRRELVM pVM )
{
	CSquirrelVM *pVScript = GetVScript( pVM );
	const float flStartTime = pVScript->m_flTimeStartedCall;
	if ( !pVScript->m_hDbgSrv && flStartTime != 0.0f )
	{
		const float flTimeDelta = Plat_FloatTime() - flStartTime;
		if ( flTimeDelta > 0.03f )
		{
			scprintf(_SC("Script running too long, terminating\n"));
			return SQ_QUERY_BREAK;
		}
	}
	return SQ_QUERY_CONTINUE;
}


//-----------------------------------------------------------------------------
// Purpose: Return a new instace of CSquirrelVM
//-----------------------------------------------------------------------------
IScriptVM *CreateSquirrelVM( void )
{
	return new CSquirrelVM();
}

//-----------------------------------------------------------------------------
// Purpose: Delete CSquirrelVM isntance
//-----------------------------------------------------------------------------
void DestroySquirrelVM( IScriptVM *pVM )
{
	if( pVM ) delete assert_cast<CSquirrelVM *>( pVM );
}