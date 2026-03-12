//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: See vscript_singletons.cpp
//
// $NoKeywords: $
//=============================================================================

#ifndef VSCRIPT_SINGLETONS_H
#define VSCRIPT_SINGLETONS_H
#ifdef _WIN32
#pragma once
#endif

void RegisterScriptSingletons();

#ifdef CLIENT_DLL
void VScriptSaveRestoreUtil_OnVMRestore();
#endif

#endif
