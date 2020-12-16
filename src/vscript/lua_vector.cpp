#include "mathlib/mathlib.h"
#include "fmtstr.h"

#include "lauxlib.h"

#include "lua_vector.h"

Vector lua_tovec3byvalue( lua_State *L, int narg )
{
	// support vector = vector + 15.0
	if ( lua_isnumber( L, narg ) )
	{
		float value = lua_tonumber( L, narg );
		return Vector( value );
	}

	return *(Vector *)luaL_checkvec3( L, narg );
}

Vector *lua_tovec3( lua_State *L, int idx )
{
	Vector *v = (Vector *)luaL_checkudata(L, idx, "Vec3");
	return v;
}

void lua_pushvec3( lua_State *L, Vector const *v )
{
	Vector *pVec = (Vector *)lua_newuserdata(L, sizeof(Vector) );
	luaL_getmetatable(L, "Vec3");
	lua_setmetatable(L, -2);
	*pVec = *v;
}

void lua_pushvec3( lua_State *L, Vector const &v )
{
	lua_pushvec3( L, &v );
}

Vector *luaL_checkvec3( lua_State *L, int narg )
{
	Vector *d = lua_tovec3(L, narg);
	if (d == NULL)
		luaL_typerror(L, narg, "Vec3");
	return d;
}


int vec3_new( lua_State *L )
{
	lua_settop( L, 3 );

	Vector *pVector = (Vector *)lua_newuserdata( L, sizeof Vector );

	luaL_getmetatable( L, "Vec3" );
	lua_setmetatable( L, -2 );

	pVector->x = luaL_optnumber( L, 1, 0.0 );
	pVector->y = luaL_optnumber( L, 2, 0.0 );
	pVector->z = luaL_optnumber( L, 3, 0.0 );

	return 1;
}

int vec3_index( lua_State *L )
{
	// how are we accessing this index?
	char const *pAccessor = luaL_checkstring( L, 2 );

	// are we accessing it correctly?
	if ( !pAccessor || pAccessor[1] != '\0' )
		return 1;

	Vector *pVector = (Vector *)luaL_checkvec3( L, 1 );
	switch ( pAccessor[0] )
	{
		case '1': case 'x':
		{
			lua_pushnumber( L, pVector->x );
			return 1;
		}
		case '2': case 'y':
		{
			lua_pushnumber( L, pVector->y );
			return 1;
		}
		case '3': case 'z':
		{
			lua_pushnumber( L, pVector->z );
			return 1;
		}
		default:
		{
			luaL_getmetatable( L, "Vec3" );
			lua_pushstring( L, pAccessor );
			lua_rawget( L, -2 );
			return 1;
		}
	}
}

int vec3_newindex( lua_State *L )
{
	// how are we accessing this index?
	char const *pAccessor = luaL_checkstring( L, 2 );
	
	// are we accessing it correctly?
	if ( !pAccessor || pAccessor[1] != '\0' )
		return 1;

	Vector *pVector = (Vector *)luaL_checkvec3( L, 1 );
	switch ( pAccessor[0] )
	{
		case '1': case 'x':
		{
			pVector->x = luaL_checknumber( L, 3 );
			return 1;
		}
		case '2': case 'y':
		{
			pVector->y = luaL_checknumber( L, 3 );
			return 1;
		}
		case '3': case 'z':
		{
			pVector->x = luaL_checknumber( L, 3 );
			return 1;
		}
		default:
			return 1;
	}
}

int vec3_tostring( lua_State *L )
{
	Vector *pVector = (Vector *)luaL_checkvec3( L, 1 );
	lua_pushstring( L, CFmtStr( "Vec3 %p", pVector ) );

	return 1;
}

int vec3_equal( lua_State *L )
{
	Vector A = lua_tovec3byvalue( L, 1 ), B = lua_tovec3byvalue( L, 2 );

	return A == B;
}

int vec3_add( lua_State *L )
{
	Vector A = lua_tovec3byvalue( L, 1 ), B = lua_tovec3byvalue( L, 2 );

	lua_pushvec3( L, A + B );

	return 1;
}

int vec3_subtract( lua_State *L )
{
	Vector A = lua_tovec3byvalue( L, 1 ), B = lua_tovec3byvalue( L, 2 );

	lua_pushvec3( L, A - B );

	return 1;
}

int vec3_multiply( lua_State *L )
{
	Vector A = lua_tovec3byvalue( L, 1 ), B = lua_tovec3byvalue( L, 2 );

	lua_pushvec3( L, A * B );

	return 1;
}

int vec3_divide( lua_State *L )
{
	Vector A = lua_tovec3byvalue( L, 1 ), B = lua_tovec3byvalue( L, 2 );

	lua_pushvec3( L, A / B );

	return 1;
}

int vec3_negate( lua_State *L )
{
	Vector *pVector = (Vector *)luaL_checkvec3( L, 1 );
	VectorNegate( *pVector );

	return 1;
}

int vec3_length( lua_State *L )
{
	Vector *pVec = luaL_checkvec3( L, 1 );

	lua_pushnumber( L, pVec->Length() );

	return 1;
}

int vec3_dot( lua_State *L )
{
	Vector A = lua_tovec3byvalue( L, 1 ), B = lua_tovec3byvalue( L, 2 );

	lua_pushnumber( L, A.Dot( B ) );

	return 1;
}

int vec3_cross( lua_State *L )
{
	Vector A = lua_tovec3byvalue( L, 1 ), B = lua_tovec3byvalue( L, 2 );

	lua_pushvec3( L, A.Cross( B ) );

	return 1;
}


static const luaL_Reg g_VectorFuncs[] ={
	{"__index",		vec3_index},
	{"__newindex",	vec3_newindex},
	{"__tostring",	vec3_tostring},
	{"__eq",		vec3_equal},
	{"__add",		vec3_add},
	{"__sub",		vec3_subtract},
	{"__mul",		vec3_multiply},
	{"__div",		vec3_divide},
	{"__unm",		vec3_negate},
	{"__len",		vec3_length},
	{"dot",			vec3_dot},
	{"cross",		vec3_cross},
	{NULL, NULL}
};

int lua_openvec3( lua_State *L )
{
	// register the type name
	luaL_newmetatable( L, "Vec3" );
	luaL_register( L, NULL, g_VectorFuncs );

	// add constructor
	lua_pushcfunction( L, vec3_new );

	// add to type registry
	lua_setfield( L, LUA_REGISTRYINDEX, "Vec3" );

	return 1;
}
