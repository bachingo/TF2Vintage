#include "mathlib/vector.h"
#include "tier1/fmtstr.h"
#include "tier1/strtools.h"

#include "sqplus.h"
#include "sqobject.h"
#include "sq_vector.h"
using namespace SqPlus;


#define sq_checkvector(vm, vector) \
	if ( vector == nullptr ) { sq_throwerror( vm, "Null vector" ); return SQ_ERROR; }

#define sq_pushvector(vm, vector) \
	sq_getclass( vm, -1 ); \
	sq_createinstance( vm, -1 ); \
	sq_setinstanceup( vm, -1, vector ); \
	sq_setreleasehook( vm, -1, &VectorRelease ); \
	sq_remove( vm, -2 );

Vector GetVectorByValue( HSQUIRRELVM pVM, int nIndex )
{
	StackHandler hndl( pVM );
	// support vector = vector + 15
	if ( hndl.GetType( nIndex ) & SQOBJECT_NUMERIC )
	{
		float flValue = hndl.GetFloat( nIndex );
		return Vector( flValue );
	}

	Vector *pVector = (Vector *)hndl.GetInstanceUp( nIndex, 0 );
	if ( pVector == nullptr )
	{
		hndl.ThrowError( "Null vector" );
		return Vector();
	}

	return *pVector;
}

SQInteger VectorConstruct( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = new Vector;

	int i;
	for ( i=0; i < hndl.GetParamCount() - 1 && i < 3; ++i )
	{
		float flValue = hndl.GetFloat( i+2 );
		(*pVector)[ i ] = flValue;
	}

	if ( i < 3 )
	{
		for( i; i<3; ++i )
			( *pVector )[ i ] = 0;
	}

	PostConstructSimple( pVM, pVector, VectorRelease );

	return SQ_OK;
}

SQInteger VectorRelease( SQUserPointer up, SQInteger size )
{
	SQ_DELETE_CLASS( Vector );
}

SQInteger VectorGet( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	const SQChar *pString = hndl.GetString( 2 );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return SQ_ERROR;

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return SQ_ERROR;

	// Accessing x, y or z
	if ( pString[0] - 'x' < 3 )
		hndl.Return( ( *pVector )[ pString[0] - 'x' ] );

	return 1;
}

SQInteger VectorSet( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	const SQChar *pString = hndl.GetString( 2 );
	// Are we using the table accessor correctly?
	if ( pString == NULL || *pString == '\0' )
		return SQ_ERROR;

	// Error on using additional characters
	if ( pString[1] != '\0' )
		return SQ_ERROR;

	// Accessing x, y or z
	if ( pString[0] - 'x' < 3 )
	{
		float flValue = hndl.GetFloat( 3 );

		( *pVector )[ pString[0] - 'x' ] = flValue;
		hndl.Return( flValue );
	}

	return 0;
}

SQInteger VectorToString( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	return hndl.Return( CFmtStr("(vector : (%f, %f, %f))", VectorExpand( *pVector ) ));
}

SQInteger VectorTypeInfo( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	return hndl.Return( "Vector" );
}

SQInteger VectorEquals( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );

	Vector *pLHS = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pLHS );

	Vector *pRHS = (Vector *)hndl.GetInstanceUp( 2, 0 );
	sq_checkvector( pVM, pRHS );

	return hndl.Return( VectorsAreEqual( *pLHS, *pRHS, 0.01 ) );
}

SQInteger VectorIterate( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	if ( hndl.GetParamCount() < 2 )
		return SQ_ERROR;

	char const *szAccessor;
	if ( hndl.GetType( 2 ) == OT_NULL )
	{
		szAccessor = "w";
	}
	else
	{
		szAccessor = hndl.GetString( 2 );
		if ( !szAccessor || !*szAccessor )
			return SQ_ERROR;
	}

	if ( szAccessor[1] != '\0' )
		return SQ_ERROR;

	static char const *const results[] ={
		"x",
		"y",
		"z"
	};

	// Accessing x, y or z
	if ( szAccessor[0] - 'x' < 3 )
		hndl.Return( results[ szAccessor[0] - 'x' ] );
	else
		sq_pushnull( pVM );

	return 1;
}

SQInteger VectorAdd( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	Vector *pNewVector = new Vector;
	*pNewVector = LHS + RHS;

	sq_pushvector( pVM, pNewVector );

	return 1;
}

SQInteger VectorSubtract( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	Vector *pNewVector = new Vector;
	*pNewVector = LHS - RHS;

	sq_pushvector( pVM, pNewVector );

	return 1;
}

SQInteger VectorMultiply( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	Vector *pNewVector = new Vector;
	*pNewVector = LHS * RHS;

	sq_pushvector( pVM, pNewVector );

	return 1;
}

SQInteger VectorDivide( HSQUIRRELVM pVM )
{
	Vector LHS = GetVectorByValue( pVM, 1 );
	Vector RHS = GetVectorByValue( pVM, 2 );

	// Create a new vector so we can keep the values of the other
	Vector *pNewVector = new Vector;
	*pNewVector = LHS / RHS;

	sq_pushvector( pVM, pNewVector );

	return 1;
}

SQInteger VectorToKeyValue( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	return hndl.Return( CFmtStr( "(vector : (%f, %f, %f))", VectorExpand( *pVector ) ) );
}

SQInteger VectorLength( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	return hndl.Return( pVector->Length() );
}

SQInteger VectorLengthSqr( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	return hndl.Return( pVector->LengthSqr() );
}

SQInteger VectorLength2D( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	return hndl.Return( pVector->Length2D() );
}

SQInteger VectorLength2DSqr( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	return hndl.Return( pVector->Length2DSqr() );
}

SQInteger VectorDotProduct( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	
	Vector *pLHS = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pLHS );

	Vector *pRHS = (Vector *)hndl.GetInstanceUp( 2, 0 );
	sq_checkvector( pVM, pRHS );

	return hndl.Return( pLHS->Dot( *pRHS ) );
}

SQInteger VectorCrossProduct( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );

	Vector *pLHS = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pLHS );

	Vector *pRHS = (Vector *)hndl.GetInstanceUp( 2, 0 );
	sq_checkvector( pVM, pRHS );

	// Create a new vector so we can keep the values of the other
	Vector *pNewVector = new Vector;
	*pNewVector = pLHS->Cross( *pRHS );

	sq_pushvector( pVM, pNewVector );

	return 1;
}

SQInteger VectorNormalize( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	Vector *pVector = (Vector *)hndl.GetInstanceUp( 1, 0 );
	sq_checkvector( pVM, pVector );

	const float flLength = VectorNormalize( *pVector );

	return hndl.Return( flLength );
}


SQRegFunction g_VectorFuncs[] ={
	{_SC( "constructor" ),		VectorConstruct				},
	{MM_GET,					VectorGet					},
	{MM_SET,					VectorSet,			2,		_SC( ".." )},
	{MM_TOSTRING,				VectorToString,		3,		_SC( "..n" )},
	{MM_TYPEOF,					VectorTypeInfo				},
	{MM_CMP,					VectorEquals,		2,		0},
	{MM_NEXTI,					VectorIterate				},
	{MM_ADD, 					VectorAdd,			2,		0},
	{MM_SUB,					VectorSubtract,		2,		0},
	{MM_MUL,					VectorMultiply,		2,		0},
	{MM_DIV,					VectorDivide,		2,		0},
	{_SC( "Length" ),			VectorLength				},
	{_SC( "LengthSqr" ),		VectorLengthSqr				},
	{_SC( "Length2D" ),			VectorLength2D				},
	{_SC( "Length2DSqr" ),		VectorLength2DSqr			},
	{_SC( "Dot" ),				VectorDotProduct,	2,		0},
	{_SC( "Cross" ),			VectorCrossProduct,	2,		0},
	{_SC( "Norm" ),				VectorNormalize				},
	{_SC( "ToKVString" ),		VectorToKeyValue			}
};

SQRESULT RegisterVector( HSQUIRRELVM pVM )
{
	int nArgs = sq_gettop( pVM );

	// Register a new class of name Vector
	sq_pushroottable( pVM );
	sq_pushstring( pVM, "Vector", -1 );

	// Something went wrong, bail and reset
	if ( SQ_FAILED( sq_newclass( pVM, SQFalse ) ) )
	{
		sq_settop( pVM, nArgs );
		return SQ_ERROR;
	}

	HSQOBJECT pTable{};

	// Setup class table
	sq_getstackobj( pVM, -1, &pTable );
	sq_settypetag( pVM, -1, VECTOR_TYPE_TAG );

	// Add to VM
	sq_createslot( pVM, -3 );

	// Prepare table for insert
	sq_pushobject( pVM, pTable );

	for ( int i = 1; i < ARRAYSIZE( g_VectorFuncs ); ++i )
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

		// Add to table
		sq_createslot( pVM, -3 );
	}

	// Reset vm
	sq_pop( pVM, 1 );
	sq_settop( pVM, nArgs );

	return SQ_OK;
}
