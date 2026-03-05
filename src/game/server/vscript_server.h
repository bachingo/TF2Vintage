//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_SERVER_H
#define VSCRIPT_SERVER_H

#include "vscript/ivscript.h"
#include "tier1/KeyValues.h"
#include "vscript_shared.h"
#include "tier1/utlsymbol.h"
#include "GameEventListener.h"

#if defined( _WIN32 )
#pragma once
#endif

class ISaveRestoreBlockHandler;

bool VScriptServerReplaceClosures( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing = false );
ISaveRestoreBlockHandler *GetVScriptSaveRestoreBlockHandler();


class CBaseEntityScriptInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize );
	void *BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId );
};

extern CBaseEntityScriptInstanceHelper g_BaseEntityScriptInstanceHelper;

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );


bool ScriptHooksEnabled( void );
bool ScriptHookEnabled( const char *pszName );
bool RunScriptHook( const char *pszHookName, HSCRIPT params );

#endif // VSCRIPT_SERVER_H
