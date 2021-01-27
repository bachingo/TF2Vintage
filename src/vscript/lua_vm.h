#ifndef LUA_VM_H
#define LUA_VM_H

#ifdef _WIN32
#pragma once
#endif



extern IScriptVM *CreateLuaVM( void );


extern void DestroyLuaVM( IScriptVM *pVM );


#endif