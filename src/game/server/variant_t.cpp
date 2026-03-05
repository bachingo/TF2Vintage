//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "variant_t.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void variant_t::SetEntity( CBaseEntity *val ) 
{ 
	eVal = val;
	fieldType = FIELD_EHANDLE; 
}

void variant_t::SetScriptVariant( ScriptVariant_t &var )
{
	switch ( FieldType() )
	{
		case FIELD_VOID:		var = VARIANT_NULL; break;
		case FIELD_INTEGER:		var = iVal; break;
		case FIELD_FLOAT:		var = flVal; break;
		case FIELD_STRING:		var = STRING( iszVal ); break;
		case FIELD_POSITION_VECTOR:
		case FIELD_VECTOR:		var = reinterpret_cast<Vector *>( &flVal ); break; // HACKHACK
		case FIELD_BOOLEAN:		var = bVal; break;
		case FIELD_EHANDLE:		var = ToHScript( eVal ); break;
		case FIELD_CLASSPTR:	var = ToHScript( eVal ); break;
		case FIELD_SHORT:		var = (short)iVal; break;
		case FIELD_CHARACTER:	var = (char)iVal; break;
		case FIELD_COLOR32:
		{
			Color *clr = new Color( rgbaVal.r, rgbaVal.g, rgbaVal.b, rgbaVal.a );
			var = g_pScriptVM->RegisterInstance( clr );
			break;
		}
		default:				var = ToString(); break;
	}
}
