#include "mathlib/vector.h"
#include "mathlib/vmatrix.h"
#include "tier1/fmtstr.h"
#include "tier1/strtools.h"

#include "squirrel.h"
#include "sqobject.h"
#include "sqstdstring.h"
#include "sqstdaux.h"

#include "vscript/ivscript.h"
#include "vsquirrel_math.h"


//=============================================================================
//
// Vector
// 
//=============================================================================

#define sq_checkvector(vm, vector) \
	if ( vector == nullptr ) { return sq_throwerror( vm, "Null vector" ); }

#define sq_pushvector(vm, vector) \
	sq_getclass( vm, -2 ); \
	sq_createinstance( vm, -1 ); \
	SQUserPointer p; \
	sq_getinstanceup( vm, -1, &p, 0, SQTrue ); \
	new( p ) Vector( vector ); \
	sq_remove( vm, -2 );

Vector GetVectorByValue( HSQUIRRELVM pVM, int nIndex )
{
	// support vector = vector + 15
	if ( sq_gettype( pVM, nIndex ) & SQOBJECT_NUMERIC )
	{
		SQFloat flValue = 0;
		sq_getfloat( pVM, nIndex, &flValue );
		return Vector( flValue );
	}

	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, nIndex, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	if ( pVector == nullptr )
	{
		sq_throwerror( pVM, "Null vector" );
		return Vector();
	}

	return *pVector;
}

SQInteger VectorConstruct( HSQUIRRELVM pVM )
{
	Vector vector;
	for ( int i=0; i < 3; ++i )
	{
		sq_getfloat( pVM, i + 2, &vector[i] );
	}

	SQUserPointer up;
	sq_getinstanceup( pVM, 1, &up, NULL, SQTrue );
	new( up ) Vector( vector );

	return 0;
}

SQInteger VectorGet( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	const SQChar *pString = NULL;
	sq_getstring( pVM, 2, &pString );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return sq_throwerror( pVM, "Bad Vector table access: Null key." );

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return sq_throwerror( pVM, "Bad Vector table access: Malformed key." );

	// Accessing x, y or z
	if ( pString[0] - 'x' < 3 )
	{
		sq_pushfloat( pVM, ( *pVector )[pString[0] - 'x'] );
		return 1;
	}

	return sqstd_throwerrorf( pVM, "Index out of range in Vector table access. Expected ('x', 'y', 'z') got '%s'", pString[0] );
}

SQInteger VectorSet( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	const SQChar *pString = NULL;
	sq_getstring( pVM, 2, &pString );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return sq_throwerror( pVM, "Bad Vector table access: Null key." );

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return sq_throwerror( pVM, "Bad Vector table access: Malformed key." );

	// Accessing x, y or z
	if ( pString[0] - 'x' < 3 )
	{
		SQFloat flValue = 0;
		sq_getfloat( pVM, 3, &flValue );

		(*pVector)[ pString[0] - 'x' ] = flValue;
		sq_pushfloat( pVM, flValue );
		return 1;
	}

	return sqstd_throwerrorf( pVM, "Index out of range in Vector table access. Expected ('x', 'y', 'z') got '%s'", pString[0] );
}

SQInteger VectorToString( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	sqstd_pushstringf( pVM, "(vector 0x%p : (%f, %f, %f))", (void *)pVector,
					   pVector->x, pVector->y, pVector->z );
	return 1;
}

SQInteger VectorTypeInfo( HSQUIRRELVM pVM )
{
	sq_pushstring( pVM, "Vector", -1 );
	return 1;
}

SQInteger VectorEquals( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pLHS = (Vector *)up;
	sq_checkvector( pVM, pLHS );

	sq_getinstanceup( pVM, 2, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pRHS = (Vector *)up;
	sq_checkvector( pVM, pRHS );

	sq_pushbool( pVM, VectorsAreEqual( *pLHS, *pRHS, 0.01 ) );
	return 1;
}

SQInteger VectorIterate( HSQUIRRELVM pVM )
{
	if ( sq_gettop( pVM ) < 2 )
		return SQ_ERROR;

	SQChar const *szAccessor = NULL;
	if ( sq_gettype( pVM, 2 ) == OT_NULL )
	{
		szAccessor = "w";
	}
	else
	{
		sq_getstring( pVM, 2, &szAccessor );
		if ( !szAccessor || !*szAccessor )
			return sq_throwerror( pVM, "Bad Vector table access: Null key." );
	}

	if ( szAccessor[1] != '\0' )
		return sq_throwerror( pVM, "Bad Vector table access: Malformed key." );

	static char const *const results[] ={
		"x",
		"y",
		"z"
	};

	// Accessing x, y or z
	if ( szAccessor[0] - 'w' < 3 )
		sq_pushstring( pVM, results[(szAccessor[0] - 'w')], 1 );
	else
		sq_pushnull( pVM );

	return 1;
}

SQInteger VectorAdd( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	sq_pushvector( pVM, LHS + RHS );

	return 1;
}

SQInteger VectorSubtract( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	sq_pushvector( pVM, LHS - RHS );

	return 1;
}

SQInteger VectorMultiply( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	sq_pushvector( pVM, LHS * RHS );

	return 1;
}

SQInteger VectorDivide( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	sq_pushvector( pVM, LHS / RHS );

	return 1;
}

SQInteger VectorNegate( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	pVector->Negate();
	return 0;
}

SQInteger VectorToKeyValue( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	sqstd_pushstringf( pVM, "%f %f %f", pVector->x, pVector->y, pVector->z );
	return 1;
}

SQInteger VectorFromKeyValue( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	SQChar const *pInput;
	if ( SQ_FAILED( sq_getstring( pVM, 2, &pInput ) ) )
		return sq_throwerror( pVM, "Expected a string input" );

	float x, y, z;
	if ( sscanf( pInput, "%f %f %f", &x, &y, &z ) < 3 )
		return sq_throwerror( pVM, "Expected format: 'float float float'" );

	pVector->Init( x, y, z );

	return 0;
}

SQInteger VectorLength( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	sq_pushfloat( pVM, pVector->Length() );
	return 1;
}

SQInteger VectorLengthSqr( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	sq_pushfloat( pVM, pVector->LengthSqr() );
	return 1;
}

SQInteger VectorLength2D( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	sq_pushfloat( pVM, pVector->Length2D() );
	return 1;
}

SQInteger VectorLength2DSqr( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	sq_pushfloat( pVM, pVector->Length2DSqr() );
	return 1;
}

SQInteger VectorDotProduct( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pLHS = (Vector *)up;
	sq_checkvector( pVM, pLHS );

	sq_getinstanceup( pVM, 2, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pRHS = (Vector *)up;
	sq_checkvector( pVM, pRHS );

	sq_pushfloat( pVM, pLHS->Dot( *pRHS ) );
	return 1;
}

SQInteger VectorCrossProduct( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pLHS = (Vector *)up;
	sq_checkvector( pVM, pLHS );

	sq_getinstanceup( pVM, 2, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pRHS = (Vector *)up;
	sq_checkvector( pVM, pRHS );

	// Create a new vector so we can keep the values of the other
	sq_pushvector( pVM, pLHS->Cross( *pRHS ) );

	return 1;
}

SQInteger VectorNormalize( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Vector *pVector = (Vector *)up;
	sq_checkvector( pVM, pVector );

	sq_pushfloat( pVM, pVector->NormalizeInPlace() );
	return 1;
}

SQInteger VectorRight( HSQUIRRELVM pVM )
{
	Vector *pVector = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pVector, VECTOR_TYPE_TAG, SQFalse );
	sq_checkvector( pVM, pVector );

	Vector right, up;
	VectorVectors( *pVector, right, up );
	sq_pushvector( pVM, right );
	return 1;
}

SQInteger VectorUp( HSQUIRRELVM pVM )
{
	Vector *pVector = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pVector, VECTOR_TYPE_TAG, SQFalse );
	sq_checkvector( pVM, pVector );

	Vector right, up;
	VectorVectors( *pVector, right, up );
	sq_pushvector( pVM, up );
	return 1;
}


SQRegFunction g_VectorFuncs[] ={
	{_SC( "constructor" ),		VectorConstruct				},
	{MM_GET,					VectorGet,			2,		_SC( ".s" )},
	{MM_SET,					VectorSet,			3,		_SC( ".sn" )},
	{MM_TOSTRING,				VectorToString				},
	{MM_TYPEOF,					VectorTypeInfo				},
	{MM_CMP,					VectorEquals,		2,		_SC( ".." )},
	{MM_NEXTI,					VectorIterate				},
	{MM_ADD, 					VectorAdd,			2,		0},
	{MM_SUB,					VectorSubtract,		2,		0},
	{MM_MUL,					VectorMultiply,		2,		0},
	{MM_DIV,					VectorDivide,		2,		0},
	{MM_UNM,					VectorNegate,		1,		0},
	{_SC( "Length" ),			VectorLength				},
	{_SC( "LengthSqr" ),		VectorLengthSqr				},
	{_SC( "Length2D" ),			VectorLength2D				},
	{_SC( "Length2DSqr" ),		VectorLength2DSqr			},
	{_SC( "Dot" ),				VectorDotProduct,	2,		0},
	{_SC( "Cross" ),			VectorCrossProduct,	2,		0},
	{_SC( "Norm" ),				VectorNormalize				},
	{_SC( "ToKVString" ),		VectorToKeyValue			},
	{_SC( "FromKVString" ),		VectorFromKeyValue			},
	{_SC( "Right" ),			VectorRight					},
	{_SC( "Up" ),				VectorUp					}
};

SQRESULT RegisterVector( HSQUIRRELVM pVM )
{
	int nArgs = sq_gettop( pVM );

	// Register a new class of name Vector
	sq_pushroottable( pVM );
	sq_pushstring( pVM, _SC("Vector"), -1 );
	if ( SQ_FAILED( sq_newclass( pVM, SQFalse ) ) )
	{
		// Something went wrong, bail and reset
		sq_settop( pVM, nArgs );
		return sq_throwerror( pVM, "Unable to create Vector class" );;
	}

	// Setup class table
	sq_settypetag( pVM, -1, VECTOR_TYPE_TAG );
	sq_setclassudsize( pVM, -1, sizeof(Vector) );

	for ( int i = 0; i < ARRAYSIZE( g_VectorFuncs ); ++i )
	{
		SQRegFunction *reg = &g_VectorFuncs[i];

		// Register function
		sq_pushstring( pVM, reg->name, -1 );
		sq_newclosure( pVM, reg->f, 0 );

		// Setup param enforcement if available
		if ( reg->nparamscheck != 0 )
			sq_setparamscheck( pVM, reg->nparamscheck, reg->typemask );

		// for debugging
		sq_setnativeclosurename( pVM, -1, reg->name );

		// Add to class
		sq_newslot( pVM, -3, SQFalse );
	}

	// Add to VM
	sq_newslot( pVM, -3, SQFalse );

	// Pop off roottable
	sq_pop( pVM, 1 );
	return SQ_OK;
}

//=============================================================================
//
// QAngle
// 
//=============================================================================

#define sq_checkangle(vm, angle) \
	if( angle == nullptr ) { return sq_throwerror( vm, "Null angle " ); }

#define sq_pushangle(vm, angle) \
	sq_getclass( vm, -2 ); \
	sq_createinstance( vm, -1 ); \
	SQUserPointer p; \
	sq_getinstanceup( vm, -1, &p, 0, SQTrue ); \
	new( p ) QAngle( angle ); \
	sq_remove( vm, -2 );

SQInteger QAngleConstruct( HSQUIRRELVM pVM )
{
	QAngle angles;
	for ( int i=0; i < 3; ++i )
	{
		sq_getfloat( pVM, i + 2, &angles[i] );
	}

	SQUserPointer up;
	sq_getinstanceup( pVM, 1, &up, NULL, SQTrue );
	new( up ) QAngle( angles );

	return 0;
}

SQInteger QAngleGet( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	const SQChar *pString = NULL;
	sq_getstring( pVM, 2, &pString );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return sq_throwerror( pVM, "Bad QAngle table access: Null key." );

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return sq_throwerror( pVM, "Bad QAngle table access: Malformed key." );

	// Accessing x, y or z
	if ( pString[0] - 'x' < 3 )
	{
		sq_pushfloat( pVM, ( *pAngle )[pString[0] - 'x'] );
		return 1;
	}

	return sqstd_throwerrorf( pVM, "Index out of range in QAngle table access. Expected ('x', 'y', 'z') got '%s'", pString[0] );
}

SQInteger QAngleSet( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	const SQChar *pString = NULL;
	sq_getstring( pVM, 2, &pString );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return sq_throwerror( pVM, "Bad QAngle table access: Null key." );

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return sq_throwerror( pVM, "Bad QAngle table access: Malformed key." );

	// Accessing x, y or z
	if ( pString[0] - 'x' < 3 )
	{
		SQFloat flValue = 0;
		sq_getfloat( pVM, 3, &flValue );

		(*pAngle)[ pString[0] - 'x' ] = flValue;
		sq_pushfloat( pVM, flValue );
		return 1;
	}

	return sqstd_throwerrorf( pVM, "Index out of range in QAngle table access. Expected ('x', 'y', 'z') got '%s'", pString[0] );
}

SQInteger QAngleToString( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	sqstd_pushstringf( pVM, "(qangle 0x%p : (%f, %f, %f))", (void *)pAngle,
					   pAngle->x, pAngle->y, pAngle->z );
	return 1;
}

SQInteger QAngleTypeInfo( HSQUIRRELVM pVM )
{
	sq_pushstring( pVM, "QAngle", -1 );

	return 1;
}

SQInteger QAngleIterate( HSQUIRRELVM pVM )
{
	if ( sq_gettop( pVM ) < 2 )
		return SQ_ERROR;

	SQChar const *szAccessor = NULL;
	if ( sq_gettype( pVM, 2 ) == OT_NULL )
	{
		szAccessor = "w";
	}
	else
	{
		sq_getstring( pVM, 2, &szAccessor );
		if ( !szAccessor || !*szAccessor )
			return sq_throwerror( pVM, "Bad Vector table access: Null key." );
	}

	if ( szAccessor[1] != '\0' )
		return sq_throwerror( pVM, "Bad Vector table access: Malformed key." );

	static char const *const results[] ={
		"x",
		"y",
		"z"
	};

	// Accessing x, y or z
	if ( szAccessor[0] - 'w' < 3 )
		sq_pushstring( pVM, results[(szAccessor[0] - 'w')], 1 );
	else
		sq_pushnull( pVM );

	return 1;
}

SQInteger QAngleAdd( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pLHS = (QAngle *)up;
	sq_checkangle( pVM, pLHS );

	sq_getinstanceup( pVM, 2, &up, QANGLE_TYPE_TAG, SQFalse );
	QAngle *pRHS = (QAngle *)up;
	sq_checkangle( pVM, pRHS );

	QAngle result = *pLHS + *pRHS;
	sq_pushangle( pVM, result );

	return 1;
}

SQInteger QAngleSubtract( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pLHS = (QAngle *)up;
	sq_checkangle( pVM, pLHS );

	sq_getinstanceup( pVM, 2, &up, QANGLE_TYPE_TAG, SQFalse );
	QAngle *pRHS = (QAngle *)up;
	sq_checkangle( pVM, pRHS );

	QAngle result = *pLHS - *pRHS;
	sq_pushangle( pVM, result );

	return 1;
}

SQInteger QAngleScale( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pLHS = (QAngle *)up;
	sq_checkangle( pVM, pLHS );

	sq_getinstanceup( pVM, 2, &up, QANGLE_TYPE_TAG, SQFalse );
	QAngle *pRHS = (QAngle *)up;
	sq_checkangle( pVM, pRHS );

	QAngle result ={pLHS->x * pRHS->x, pLHS->y * pRHS->y, pLHS->z * pRHS->z};
	sq_pushangle( pVM, result );

	return 1;
}

SQInteger QAngleToKeyValue( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	sqstd_pushstringf( pVM, "%f %f %f", pAngle->x, pAngle->y, pAngle->z );
	return 1;
}

SQInteger QAngleFromKeyValue( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	SQChar const *pInput;
	if ( SQ_FAILED( sq_getstring( pVM, 2, &pInput ) ) )
		return sq_throwerror( pVM, "Expected a string input" );

	float x, y, z;
	if ( sscanf( pInput, "%f %f %f", &x, &y, &z ) < 3 )
		return sq_throwerror( pVM, "Expected format: 'float float float'" );

	pAngle->Init( x, y, z );

	return 0;
}

SQInteger QAnglePitch( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	sq_pushfloat( pVM, pAngle->x );
	return 1;
}

SQInteger QAngleYaw( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	sq_pushfloat( pVM, pAngle->y );
	return 1;
}

SQInteger QAngleRoll( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	sq_pushfloat( pVM, pAngle->z );
	return 1;
}

SQInteger QAngleForward( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	Vector vecFwd;
	AngleVectors( *pAngle, &vecFwd );

	sq_pushroottable( pVM );
	sq_pushstring( pVM, "Vector", -1 );
	// Get the class delegate
	sq_get( pVM, -2 );
	// Remove root table
	sq_remove( pVM, -2 );

	sq_createinstance( pVM, -1 );
	sq_getinstanceup( pVM, -1, &up, VECTOR_TYPE_TAG, SQTrue );
	new( up ) Vector( vecFwd );
	// Remove class delegate
	sq_remove( pVM, -2 );

	return 1;
}

SQInteger QAngleLeft( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	Vector vecFwd, vecRight, vecUp;
	AngleVectors( *pAngle, &vecFwd, &vecRight, &vecUp );

	sq_pushroottable( pVM );
	sq_pushstring( pVM, "Vector", -1 );
	// Get the class delegate
	sq_get( pVM, -2 );
	// Remove root table
	sq_remove( pVM, -2 );

	sq_createinstance( pVM, -1 );
	sq_getinstanceup( pVM, -1, &up, VECTOR_TYPE_TAG, SQTrue );
	new( up ) Vector( vecRight );
	// Remove class delegate
	sq_remove( pVM, -2 );

	return 1;
}

SQInteger QAngleUp( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	Vector vecFwd, vecRight, vecUp;
	AngleVectors( *pAngle, &vecFwd, &vecRight, &vecUp );

	sq_pushroottable( pVM );
	sq_pushstring( pVM, "Vector", -1 );
	// Get the class delegate
	sq_get( pVM, -2 );
	// Remove root table
	sq_remove( pVM, -2 );

	sq_createinstance( pVM, -1 );
	sq_getinstanceup( pVM, -1, &up, VECTOR_TYPE_TAG, SQTrue );
	new( up ) Vector( vecUp );
	// Remove class delegate
	sq_remove( pVM, -2 );

	return 1;
}

SQInteger QAngleToQuat( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	QAngle *pAngle = (QAngle *)up;
	sq_checkangle( pVM, pAngle );

	Quaternion quat;
	AngleQuaternion( *pAngle, quat );

	sq_pushroottable( pVM );
	sq_pushstring( pVM, "Quaternion", -1 );
	// Get the class delegate
	sq_get( pVM, -2 );
	// Remove root table
	sq_remove( pVM, -2 );

	sq_createinstance( pVM, -1 );
	sq_getinstanceup( pVM, -1, &up, QUATERNION_TYPE_TAG, SQTrue );
	new( up ) Quaternion( quat );
	// Remove class delegate
	sq_remove( pVM, -2 );

	return 1;
}

SQRegFunction g_QAngleFuncs[] ={
	{_SC( "constructor" ),		QAngleConstruct		},
	{MM_GET,					QAngleGet,			2,		_SC( ".s" )},
	{MM_SET,					QAngleSet,			3,		_SC( ".sn" )},
	{MM_TOSTRING,				QAngleToString,		},
	{MM_TYPEOF,					QAngleTypeInfo		},
	{MM_NEXTI,					QAngleIterate		},
	{MM_ADD,					QAngleAdd,			2,		_SC( ".." )},
	{MM_SUB,					QAngleSubtract,		2,		_SC( ".." )},
	{MM_MUL,					QAngleScale,		2,		_SC( ".." )},
	{_SC( "ToKVString" ),		QAngleToKeyValue	},
	{_SC( "FromKVString" ),		QAngleFromKeyValue	},
	{_SC( "Pitch" ),			QAnglePitch			},
	{_SC( "Yaw" ),				QAngleYaw			},
	{_SC( "Roll" ),				QAngleRoll			},
	{_SC( "Forward" ),			QAngleForward		},
	{_SC( "Left" ),				QAngleLeft			},
	{_SC( "Up" ),				QAngleUp			},
	{_SC( "ToQuat" ),			QAngleToQuat		}
};

SQRESULT RegisterQAngle( HSQUIRRELVM pVM )
{
	int nArgs = sq_gettop( pVM );

	// Register a new class
	sq_pushroottable( pVM );
	sq_pushstring( pVM, _SC("QAngle"), -1 );
	if ( SQ_FAILED( sq_newclass( pVM, SQFalse ) ) )
	{
		// Something went wrong, bail and reset
		sq_settop( pVM, nArgs );
		return sq_throwerror( pVM, "Unable to create QAngle class" );;
	}

	// Setup class table
	sq_settypetag( pVM, -1, QANGLE_TYPE_TAG );
	sq_setclassudsize( pVM, -1, sizeof(Quaternion) );

	for ( int i = 0; i < ARRAYSIZE( g_QAngleFuncs ); ++i )
	{
		SQRegFunction *reg = &g_QAngleFuncs[i];

		// Register function
		sq_pushstring( pVM, reg->name, -1 );
		sq_newclosure( pVM, reg->f, 0 );

		// Setup param enforcement if available
		if ( reg->nparamscheck != 0 )
			sq_setparamscheck( pVM, reg->nparamscheck, reg->typemask );

		// for debugging
		sq_setnativeclosurename( pVM, -1, reg->name );

		// Add to class
		sq_newslot( pVM, -3, SQFalse );
	}

	// Add to VM
	sq_newslot( pVM, -3, SQFalse );

	// Pop off roottable
	sq_pop( pVM, 1 );
	return SQ_OK;
}

//=============================================================================
//
// Quaternion
// 
//=============================================================================

#define sq_checkquaternion(vm, quat) \
	if ( quat == nullptr ) { return sq_throwerror( vm, "Null quaternion" ); }

#define sq_pushquaternion(vm, quat) \
	sq_getclass( vm, -1 ); \
	sq_createinstance( vm, -1 ); \
	sq_getinstanceup( vm, -1, &up, 0, SQTrue ); \
	new( up ) Quaternion( quat ); \
	sq_remove( vm, -2 );

SQInteger QuaternionConstruct( HSQUIRRELVM pVM )
{
	int top = sq_gettop( pVM );
	if ( top > 1 && top != 5 )
		return sqstd_throwerrorf( pVM, "Bad arguments passed to Quaternion constructor, expected 4, got %d", (top - 1) );

	Quaternion quat;
	for ( int i=0; i < 4; ++i )
	{
		sq_getfloat( pVM, i + 2, &quat[i] );
	}

	SQUserPointer up;
	sq_getinstanceup( pVM, 1, &up, NULL, SQTrue );
	new( up ) Quaternion( quat );

	return 0;
}

SQInteger QuaternionGet( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, QUATERNION_TYPE_TAG, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	const SQChar *pString = NULL;
	sq_getstring( pVM, 2, &pString );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return sq_throwerror( pVM, "Bad Quaternion table access: Null string." );

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return sq_throwerror( pVM, "Bad Quaternion table access: Malformed string." );

	// Accessing w, x, y or z
	switch( pString[0] )
	{
		case 'w':
			sq_pushfloat( pVM, pQuat->w );
			break;
		case 'x':
			sq_pushfloat( pVM, pQuat->x );
			break;
		case 'y':
			sq_pushfloat( pVM, pQuat->y );
			break;
		case 'z':
			sq_pushfloat( pVM, pQuat->z );
			break;
		default:
			return sqstd_throwerrorf( pVM, "Index out of range in Quaternion table access. Expected ('x', 'y', 'z', 'w') got '%s'", pString[0] );
	}

	return 1;
}

SQInteger QuaternionSet( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, QUATERNION_TYPE_TAG, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	const SQChar *pString = NULL;
	sq_getstring( pVM, 2, &pString );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return sq_throwerror( pVM, "Bad Quaternion table access: Null string." );

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return sq_throwerror( pVM, "Bad Quaternion table access: Malformed string." );

	// Accessing w, x, y or z
	SQFloat flValue = 0;
	sq_getfloat( pVM, 3, &flValue );
	switch( pString[0] )
	{
		case 'w':
			pQuat->w = flValue;
			break;
		case 'x':
			pQuat->x = flValue;
			break;
		case 'y':
			pQuat->y = flValue;
			break;
		case 'z':
			pQuat->z = flValue;
			break;
		default:
			return sqstd_throwerrorf( pVM, "Index out of range in Quaternion table access. Expected ('x', 'y', 'z', 'w') got '%s'", pString[0] );
	}

	sq_pushfloat( pVM, flValue );
	return 1;
}

SQInteger QuaternionToString( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, QUATERNION_TYPE_TAG, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	sqstd_pushstringf( pVM, "(quaternion 0x%p : (%f, %f, %f, %f))", (void *)pQuat, 
								 pQuat->x, pQuat->y, pQuat->z, pQuat->w );
	return 1;
}

SQInteger QuaternionTypeInfo( HSQUIRRELVM pVM )
{
	sq_pushstring( pVM, "Quaternion", -1 );
	return 1;
}

SQInteger QuaternionEquals( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, QUATERNION_TYPE_TAG, SQFalse );
	Quaternion *pLHS = (Quaternion *)up;
	sq_checkquaternion( pVM, pLHS );

	sq_getinstanceup( pVM, 2, &up, QUATERNION_TYPE_TAG, SQFalse );
	Quaternion *pRHS = (Quaternion *)up;
	sq_checkquaternion( pVM, pRHS );

	sq_pushbool( pVM, *pLHS == *pRHS );
	return 1;
}

SQInteger QuaternionIterate( HSQUIRRELVM pVM )
{
	if ( sq_gettop( pVM ) < 2 )
		return SQ_ERROR;

	SQChar const *szAccessor = NULL;
	if ( sq_gettype( pVM, 2 ) == OT_NULL )
	{
		sq_pushstring( pVM, "x", 1 );
		return 1;
	}
	else
	{
		sq_getstring( pVM, 2, &szAccessor );
		if ( !szAccessor || !*szAccessor )
			return sq_throwerror( pVM, "Bad Quaternion table access: Null string." );
	}

	if ( szAccessor[1] != '\0' )
		return sq_throwerror( pVM, "Bad Quaternion table access: Malformed string." );

	static char const *const results[] ={
		"x",
		"y",
		"z",
		"w",
	};

	// Accessing w, x, y or z
	if ( szAccessor[0] != 'w' && szAccessor[0] - 'w' < 4 )
		sq_pushstring( pVM, results[(szAccessor[0] - 'w')], 1 );
	else
		sq_pushnull( pVM );

	return 1;
}

SQInteger QuaternionAdd( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *p = (Quaternion *)up;
	sq_checkquaternion( pVM, p );

	sq_getinstanceup( pVM, 2, &up, NULL, SQFalse );
	Quaternion *q = (Quaternion *)up;
	sq_checkquaternion( pVM, q );

	Quaternion result;
	QuaternionAdd( *p, *q, result );
	sq_pushquaternion( pVM, result );

	return 1;
}

SQInteger QuaternionSubtract( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *p = (Quaternion *)up;
	sq_checkquaternion( pVM, p );

	sq_getinstanceup( pVM, 2, &up, NULL, SQFalse );
	Quaternion *q = (Quaternion *)up;
	sq_checkquaternion( pVM, q );

	Quaternion result;
	sq_pushquaternion( pVM, result );

	return 1;
}

SQInteger QuaternionScale( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *p = (Quaternion *)up;
	sq_checkquaternion( pVM, p );

	SQFloat f = 0;
	sq_getfloat( pVM, 2, &f );

	Quaternion result;
	QuaternionScale( *p, f, result );
	sq_pushquaternion( pVM, result );

	return 1;
}

SQInteger QuaternionToKeyValue( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	sqstd_pushstringf( pVM, "%f %f %f %f", pQuat->x, pQuat->y, pQuat->z, pQuat->w );
	return 1;
}

SQInteger QuaternionFromKeyValue( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	SQChar const *pInput;
	if ( SQ_FAILED( sq_getstring( pVM, 2, &pInput ) ) )
		return sq_throwerror( pVM, "Expected a string input" );

	float x, y, z, w;
	if ( sscanf( pInput, "%f %f %f %f", &x, &y, &z, &w ) < 4 )
		return sq_throwerror( pVM, "Expected format: 'float float float float'" );

	pQuat->Init( x, y, z, w );

	return 0;
}

SQInteger QuaternionToQAngle( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	QAngle angles;
	QuaternionAngles( *pQuat, angles );

	sq_pushroottable( pVM );
	sq_pushstring( pVM, "QAngle", -1 );
	// Get the class delegate
	sq_get( pVM, -2 );
	// Remove root table
	sq_remove( pVM, -2 );

	sq_createinstance( pVM, -1 );
	sq_getinstanceup( pVM, -1, &up, QANGLE_TYPE_TAG, SQTrue );
	V_memcpy( up, &angles, sizeof(QAngle) );
	// Remove class delegate
	sq_remove( pVM, -2 );

	return 1;
}

SQInteger QuaternionDot( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;

	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *p = (Quaternion *)up;
	sq_checkquaternion( pVM, p );

	sq_getinstanceup( pVM, 2, &up, NULL, SQFalse );
	Quaternion *q = (Quaternion *)up;
	sq_checkquaternion( pVM, q );

	float flDot = QuaternionDotProduct( *p, *q );

	sq_pushfloat( pVM, flDot );
	return 1;
}

SQInteger QuaternionNorm( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	float flNormal = QuaternionNormalize( *pQuat );

	sq_pushfloat( pVM, flNormal );
	return 1;
}

SQInteger QuaternionInvert( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, NULL, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	Quaternion inv;
	QuaternionInvert( *pQuat, inv );
	sq_pushquaternion( pVM, inv );

	return 1;
}

SQInteger QuaternionSetPitchYawRoll( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, VECTOR_TYPE_TAG, SQFalse );
	Quaternion *pQuat = (Quaternion *)up;
	sq_checkquaternion( pVM, pQuat );

	QAngle angles;
	for ( int i = 0; i < 3; ++i )
		sq_getfloat( pVM, i + 2, &angles[i] );

	AngleQuaternion( angles, *pQuat );

	sq_pushnull( pVM );
	return 1;
}

SQRegFunction g_QuaternionFuncs[] ={
	{_SC( "constructor" ),		QuaternionConstruct				},
	{MM_GET,					QuaternionGet,			2,		_SC( ".s" )},
	{MM_SET,					QuaternionSet,			3,		_SC( ".sn" )},
	{MM_ADD,					QuaternionAdd					},
	{MM_SUB,					QuaternionSubtract				},
	{MM_MUL,					QuaternionScale					},
	{MM_TOSTRING,				QuaternionToString				},
	{MM_TYPEOF,					QuaternionTypeInfo				},
	{MM_CMP,					QuaternionEquals,		2,		_SC( ".." )},
	{MM_NEXTI,					QuaternionIterate				},
	{_SC( "ToKVString" ),		QuaternionToKeyValue			},
	{_SC( "FromKVString" ),		QuaternionFromKeyValue			},
	{_SC( "ToQAngle" ),			QuaternionToQAngle				},
	{_SC( "Dot" ),				QuaternionDot,			2,		_SC( ".." )},
	{_SC( "Norm" ),				QuaternionNorm					},
	{_SC( "Invert" ),			QuaternionInvert				},
	{_SC( "SetPitchYawRoll" ),	QuaternionSetPitchYawRoll, 4,	_SC( ".nnn" )}
};

SQRESULT RegisterQuaternion( HSQUIRRELVM pVM )
{
	int nArgs = sq_gettop( pVM );

	// Register a new class
	sq_pushroottable( pVM );
	sq_pushstring( pVM, _SC("Quaternion"), -1 );
	if ( SQ_FAILED( sq_newclass( pVM, SQFalse ) ) )
	{
		// Something went wrong, bail and reset
		sq_settop( pVM, nArgs );
		return sq_throwerror( pVM, "Unable to create Quaternion class" );;
	}

	// Setup class table
	sq_settypetag( pVM, -1, QUATERNION_TYPE_TAG );
	sq_setclassudsize( pVM, -1, sizeof(Quaternion) );

	for ( int i = 0; i < ARRAYSIZE( g_QuaternionFuncs ); ++i )
	{
		SQRegFunction *reg = &g_QuaternionFuncs[i];

		// Register function
		sq_pushstring( pVM, reg->name, -1 );
		sq_newclosure( pVM, reg->f, 0 );

		// Setup param enforcement if available
		if ( reg->nparamscheck != 0 )
			sq_setparamscheck( pVM, reg->nparamscheck, reg->typemask );

		// for debugging
		sq_setnativeclosurename( pVM, -1, reg->name );

		// Add to class
		sq_newslot( pVM, -3, SQFalse );
	}

	// Add to VM
	sq_newslot( pVM, -3, SQFalse );

	// Pop off roottable
	sq_pop( pVM, 1 );
	return SQ_OK;
}

//=============================================================================
//
// matrix3x4_t
// 
//=============================================================================

SQInteger MatrixConstruct( HSQUIRRELVM pVM )
{
	int top = sq_gettop( pVM );
	if ( top > 1 && top != 13 )
		return sqstd_throwerrorf( pVM, "Bad arguments passed to matrix3x4_t constructor, expected 12, got %d", (top - 1) );

	matrix3x4_t matrix;
	for ( int i=0; i < 12; ++i )
	{
		sq_getfloat( pVM, i + 2, &matrix[i / 4][i % 4] );
	}

	SQUserPointer up;
	sq_getinstanceup( pVM, 1, &up, NULL, SQTrue );
	V_memcpy( up, &matrix, sizeof(matrix3x4_t) );

	return SQ_OK;
}

SQInteger MatrixTypeInfo( HSQUIRRELVM pVM )
{
	sq_pushstring( pVM, "matrix3x4_t", -1 );
	return 1;
}

SQInteger MatrixToString( HSQUIRRELVM pVM )
{
	SQUserPointer up = NULL;
	sq_getinstanceup( pVM, 1, &up, MATRIX_TYPE_TAG, SQTrue );
	matrix3x4_t &matrix = *(matrix3x4_t *)up;

	sqstd_pushstringf( pVM, "(matrix 0x%p : [(%f, %f, %f, %f), (%f, %f, %f, %f), (%f, %f, %f, %f)])",
								 (void *)&matrix,
								 matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
								 matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
								 matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3] );
	return 1;
}

SQRESULT RegisterMatrix( HSQUIRRELVM pVM )
{
	int nArgs = sq_gettop( pVM );

	// Register a new class
	sq_pushroottable( pVM );
	sq_pushstring( pVM, _SC("matrix3x4_t"), -1 );
	if ( SQ_FAILED( sq_newclass( pVM, SQFalse ) ) )
	{	
		// Something went wrong, bail and reset
		sq_settop( pVM, nArgs );
		return sq_throwerror( pVM, "Unable to create matrix3x4_t class" );;
	}

	sq_settypetag( pVM, -1, MATRIX_TYPE_TAG );
	sq_setclassudsize( pVM, -1, sizeof(matrix3x4_t) );

	sq_pushstring( pVM, _SC( "constructor" ), -1);
	sq_newclosure( pVM, MatrixConstruct, 0 );
	sq_setnativeclosurename( pVM, -1, _SC( "constructor" ) );
	sq_newslot( pVM, -3, SQFalse );

	sq_pushstring( pVM, MM_TOSTRING, -1 );
	sq_newclosure( pVM, MatrixToString, 0 );
	sq_setnativeclosurename( pVM, -1, MM_TOSTRING );
	sq_newslot( pVM, -3, SQFalse );

	sq_pushstring( pVM, MM_TYPEOF, -1 );
	sq_newclosure( pVM, MatrixTypeInfo, 0 );
	sq_setnativeclosurename( pVM, -1, MM_TYPEOF );
	sq_newslot( pVM, -3, SQFalse );

	// Add to VM
	sq_newslot( pVM, -3, SQFalse );

	// Pop off roottable
	sq_pop( pVM, 1 );
	return SQ_OK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SQRESULT RegisterMathBindings( HSQUIRRELVM pVM )
{
	if( SQ_FAILED( RegisterVector( pVM ) ) )
		return SQ_ERROR;

	if( SQ_FAILED( RegisterQAngle( pVM ) ) )
		return SQ_ERROR;
	
	if ( SQ_FAILED( RegisterQuaternion( pVM ) ) )
		return SQ_ERROR;

	if ( SQ_FAILED( RegisterMatrix( pVM ) ) )
		return SQ_ERROR;

	return SQ_OK;
}
