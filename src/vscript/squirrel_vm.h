#ifndef SQUIRREL_VM_H
#define SQUIRREL_VM_H

#ifdef _WIN32
#pragma once
#endif


#include "ivscript.h"

extern IScriptVM *CreateSquirrelVM( void );


extern void DestroySquirrelVM( IScriptVM *pVM );


#endif
