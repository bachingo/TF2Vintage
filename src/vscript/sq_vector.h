#ifndef SQUIRREL_VECTOR_H
#define SQUIRREL_VECTOR_H

#ifdef _WIN32
#pragma once
#endif

#define VECTOR_TYPE_TAG		((SQUserPointer)0xDEDBEAF)

SQRESULT RegisterVector( HSQUIRRELVM pVM );
extern SQRegFunction g_VectorFuncs[];

SQInteger VectorConstruct( HSQUIRRELVM pVM );
SQInteger VectorRelease( SQUserPointer up, SQInteger size );

#endif