#ifndef SQUIRREL_VECTOR_H
#define SQUIRREL_VECTOR_H

#ifdef _WIN32
#pragma once
#endif

#define VECTOR_TYPE_TAG		0xFEEDFACE

SQRESULT RegisterVector( HSQUIRRELVM pVM );
extern SQRegFunction g_VectorFuncs[];

SQInteger VectorConstruct( HSQUIRRELVM pVM );
SQInteger VectorRelease( SQUserPointer data, SQInteger size );
SQInteger VectorGet( HSQUIRRELVM pVM );
SQInteger VectorSet( HSQUIRRELVM pVM );
SQInteger VectorToString( HSQUIRRELVM pVM );
SQInteger VectorTypeInfo( HSQUIRRELVM pVM );
SQInteger VectorIterate( HSQUIRRELVM pVM );
SQInteger VectorAdd( HSQUIRRELVM pVM );
SQInteger VectorSubtract( HSQUIRRELVM pVM );
SQInteger VectorMultiply( HSQUIRRELVM pVM );
SQInteger VectorToKeyValue( HSQUIRRELVM pVM );
SQInteger VectorLength( HSQUIRRELVM pVM );
SQInteger VectorLengthSqr( HSQUIRRELVM pVM );
SQInteger VectorLength2D( HSQUIRRELVM pVM );
SQInteger VectorLength2DSqr( HSQUIRRELVM pVM );
SQInteger VectorDotProduct( HSQUIRRELVM pVM );
SQInteger VectorCrossProduct( HSQUIRRELVM pVM );
SQInteger VectorNormalize( HSQUIRRELVM pVM );

#endif