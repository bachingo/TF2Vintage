/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#if defined(VSCRIPT_DLL_EXPORT)
#include "tier0/memdbgon.h"
#endif
#ifndef SQ_EXCLUDE_DEFAULT_MEMFUNCTIONS
void *sq_vm_malloc(SQUnsignedInteger size){ return calloc(size, 1); }

void *sq_vm_realloc(void *p, SQUnsignedInteger SQ_UNUSED_ARG(oldsize), SQUnsignedInteger size){ return realloc(p, size); }

void sq_vm_free(void *p, SQUnsignedInteger SQ_UNUSED_ARG(size)){ free(p); }
#endif
