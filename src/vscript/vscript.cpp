#include "tier1.h"
#include "ivscript.h"
#include "squirrel_vm.h"
#include "lua_vm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CScriptManager : public CTier1AppSystem<IScriptManager>
{
public:
	IScriptVM *CreateVM( ScriptLanguage_t language = SL_DEFAULT );
	void DestroyVM( IScriptVM *pVM );
};

EXPOSE_SINGLE_INTERFACE( CScriptManager, IScriptManager, VSCRIPT_INTERFACE_VERSION )



IScriptVM *CScriptManager::CreateVM( ScriptLanguage_t language )
{
	IScriptVM *pVM = NULL;
	switch ( language )
	{
		case SL_SQUIRREL:
			pVM = CreateSquirrelVM();
			break;
		case SL_LUA:
			pVM = CreateLuaVM();
		default:
			return NULL;
	}

	if ( pVM )
	{
		if ( !pVM->Init() )
			return NULL;

		ScriptRegisterFunction( pVM, RandomFloat, "Generate a random floating point number within a range, inclusive" );
		ScriptRegisterFunction( pVM, RandomInt, "Generate a random integer within a range, inclusive" );
		ScriptRegisterFunction( pVM, RandomFloatExp, "Generate an exponential random floating point number within a range, exclusive" );
	}

	return pVM;
}

void CScriptManager::DestroyVM( IScriptVM *pVM )
{
	if ( pVM )
	{
		pVM->Shutdown();

		switch ( pVM->GetLanguage() )
		{
			case SL_SQUIRREL:
				DestroySquirrelVM( pVM );
				break;
			default:
				break;
		}
	}
}
