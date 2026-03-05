#include "tier1.h"
#include "ivscript.h"
#include "languages/lua/lua_vm.h"
#include "languages/squirrel/vsquirrel.h"
#include "languages/angelscript/vangelscript.h"
#include "vscript_misc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CScriptManager : public CTier1AppSystem<IScriptManager>
{
public:
	IScriptVM *CreateVM( ScriptLanguage_t language = SL_DEFAULT ) OVERRIDE;
	void DestroyVM( IScriptVM *pVM ) OVERRIDE;
};

EXPOSE_SINGLE_INTERFACE( CScriptManager, IScriptManager, VSCRIPT_INTERFACE_VERSION )


//-----------------------------------------------------------------------------
// Purpose: Create an isntance of our desired language's VM
//-----------------------------------------------------------------------------
IScriptVM *CScriptManager::CreateVM( ScriptLanguage_t language )
{
	IScriptVM *pVM = NULL;
	switch ( language )
	{
		case SL_SQUIRREL:
			pVM = CreateSquirrelVM();
			break;
		case SL_LUA:
			//pVM = CreateLuaVM();
			break;
		case SL_ANGELSCRIPT:
			//pVM = CreateAngelScriptVM();
			break;
		default:
			return NULL;
	}

	if ( pVM )
	{
		if ( !pVM->Init() )
		{
			delete pVM;
			return NULL;
		}

		RegisterBaseBindings( pVM );
	}

	return pVM;
}

//-----------------------------------------------------------------------------
// Purpose: shutdown and delete the VM instance
//-----------------------------------------------------------------------------
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
			case SL_LUA:
				//DestroyLuaVM( pVM );
				break;
			case SL_ANGELSCRIPT:
				//DestroyAngelScriptVM( pVM );
				break;
		}
	}
}
