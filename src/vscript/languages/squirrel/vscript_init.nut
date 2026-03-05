//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose: Script initially run after squirrel VM is initialized
//
//=============================================================================

//-----------------------------------------------------------------------------
// General
//-----------------------------------------------------------------------------

realPrint <- print;
print_indent <- 0;

getconsttable().Constants.Server <- {
	ConstantNamingConvention = "Constants are named as follows: F -> flags, E -> enums, (nothing) -> random values/constants",
	DIST_EPSILON = 0.03125,
	MAX_PLAYERS = 33,
	MAX_EDICTS = 2048
};

getconsttable().Constants.Math <- {
	Zero = 0,
	Epsilon = 1.19209e-07,
	GoldenRatio = 1.61803,
	One = 1,
	Sqrt2 = 1.41421,
	Sqrt3 = 1.73205,
	E = 2.71828,
	Pi = 3.14159,
	Tau = 6.28319
};

function print( text )
{
	for( local i = print_indent; i > 0; --i )
	{
		realPrint( "  " );
	}
	realPrint( text );
}

function printl( text )
{
	return print( text + "\n" );
}

function Msg( text )
{
	return print( text );
}

function Warning( text )
{
	return error( text );
}

function Assert( b, msg = null )
{
	if ( b )
		return;
		
	if ( msg != null )
	{
		throw "Assertion failed: " + msg;
	}
	else
	{
		throw "Assertion failed";
	}
}

function clamp(val,min,max)
{
	if ( max < min )
		return max;
	else if( val < min )
		return min;
	else if( val > max )
		return max;
	else
		return val;
}

function max(a,b) return a > b ? a : b

function min(a,b) return a < b ? a : b

function RemapVal(val, A, B, C, D)
{
	if ( A == B )
		return val >= B ? D : C;
	return C + (D - C) * (val - A) / (B - A);
}

function RemapValClamped(val, A, B, C, D)
{
	if ( A == B )
		return val >= B ? D : C;
	local cVal = (val - A) / (B - A);
	cVal = (cVal < 0.0) ? 0.0 : (1.0 < cVal) ? 1.0 : cVal;
	return C + (D - C) * cVal;
}

function Approach( target, value, speed )
{
	local delta = target - value

	if( delta > speed )
		value += speed
	else if( delta < (-speed) )
		value -= speed
	else
		value = target

	return value
}

function AngleDistance( next, cur )
{
	local delta = next - cur

	if ( delta < (-180.0) )
		delta += 360.0
	else if ( delta > 180.0 )
		delta -= 360.0

	return delta
}

function FLerp( f1, f2, i1, i2, x )
{
	return f1+(f2-f1)*(x-i1)/(i2-i1);
}

function Lerp( f, A, B )
{
	return A + ( B - A ) * f
}

function SimpleSpline( f )
{
	local ff = f * f;
	return 3.0 * ff - 2.0 * ff * f;
}

//-----------------------------------------------------------------------------

function FindCircularReference( target )
{
	local visits = {}
	local result = false;

	local function RecursiveSearch( current )
	{
		if( current in visits )
		{
			return;
		}
		visits[current] <- true;

		foreach( key, val in current )
		{
			if( val == target && !IsWeakRef( target, key ) )
			{
				printl( "    Circular reference to " + target.tostring() + " in key " + key.tostring() + " slot " + val.tostring() + " of object " + current.tostring() );
				result = true;
			}
			else if ( typeof(val) == "table" || typeof(val) == "array" || typeof(val) == "instance" )
			{
				if( !IsWeakRef( target, key ) )
				{
					RecursiveSearch( val );
				}
			}
		}
	}

	if( typeof( target ) == "table" || typeof( target ) == "array" || typeof(target) == "instance" )
	{
		RecursiveSearch( target );
	}

	return result;
}

function FindCircularReferences( resurrecteds )
{
	printl( "Circular references:" );

	if( resurrecteds == null )
	{
		printl( "    None" );
		return;
	}

	if( typeof(resurrecteds) != "array" )
	{
		throw "Bad input to FindCircularReference";
	}

	foreach( val in resurrecteds )
	{
		FindCircularReference( val );
	}

	print( "Resurrected objects: " );
	DumpObject( resurrecteds );
}

//-----------------------------------------------------------------------------

function DebugDrawCircle( position, angles, radius, r, g, b, a, bNoDepthTest, flDuration )
{
	local xform = AngleMatrix( angles );
	local xAxis = MatrixGetColumn( xform, 2 );
	local yAxis = MatrixGetColumn( xform, 1 );

	const nSegments = 16;
	local flRadStep = getconsttable().Constants.Math.Tau / nSegments.tofloat();

	local vecStart = position + xAxis * radius;
	local vecPosition = vecStart;
	local vecLastPosition = vecPosition;

	for ( local i = 1; i <= nSegments; i++ )
	{
		// Store off our last position
		vecLastPosition = vecPosition;

		// Calculate the new one
		local flCos = cos( flRadStep * i );
		local flSin = sin( flRadStep * i );
		vecPosition = position + (xAxis * flCos * radius) + (yAxis * flSin * radius);

		// Draw the line
		DebugDrawLine( vecLastPosition, vecPosition, r, g, b, bNoDepthTest, flDuration );

		// If we have an alpha value, then draw the fan
		if ( a > 0 && i > 1 )
		{		
			DebugDrawTriangle( vecStart, vecLastPosition, vecPosition, r, g, b, a, bNoDepthTest, flDuration );
		}
	}
}

function DebugDrawSphere( position, angles, radius, r, g, b, a, bNoDepthTest, flDuration )
{
	local xform = AngleMatrix( angles );
	local xAxis = MatrixGetColumn( xform, 0, xAxis );
	local yAxis = MatrixGetColumn( xform, 1, yAxis );
	local zAxis = MatrixGetColumn( xform, 2, zAxis );

	DebugDrawCircle( position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration );	// xy plane
	DebugDrawCircle( position, yAxis, zAxis, radius, r, g, b, a, bNoDepthTest, flDuration );	// yz plane
	DebugDrawCircle( position, xAxis, zAxis, radius, r, g, b, a, bNoDepthTest, flDuration );	// xz plane
}

function DebugDrawBox( origin, mins, maxs, r, g, b, a, duration )
{
	DebugDrawBoxAngles( origin, mins, maxs, QAngle(0, 0, 0), r, g, b, a, duration );
}

function DebugDrawBoxDirection( origin, mins, maxs, orientation, r, g, b, a, duration )
{
	local angles = VectorAngles( orientation );
	angles.x = angles.z = 0.0;

	DebugDrawBoxAngles( origin, mins, maxs, angles, r, g, b, a, duration );
}

function DebugDrawCross3D( position, size, r, g, b, noDepthTest, flDuration )
{
	DebugDrawLine( position + Vector(size,0,0), position - Vector(size,0,0), r, g, b, noDepthTest, flDuration );
	DebugDrawLine( position + Vector(0,size,0), position - Vector(0,size,0), r, g, b, noDepthTest, flDuration );
	DebugDrawLine( position + Vector(0,0,size), position - Vector(0,0,size), r, g, b, noDepthTest, flDuration );
}

function DebugDrawCross3DOriented( position, angles, size, r, g, b, noDepthTest, flDuration )
{
	local forward = angles.Forward() * size;
	local right = angles.Left() * size;
	local up = angles.Up() * size;

	DebugDrawLine( position + right, position - right, r, g, b, noDepthTest, flDuration );
	DebugDrawLine( position + forward, position - forward, r, g, b, noDepthTest, flDuration );
	DebugDrawLine( position + up, position - up, r, g, b, noDepthTest, flDuration );
}

function DebugDrawAxis(position, angles, size, noDepthTest, flDuration)
{
	local xvec = AngleVectors( angles );
	local yvec = xvec.Right();
	local zvec = xvec.Up();
	
	xvec = position + (xvec * size);
	yvec = position - (yvec * size); // Left is positive
	zvec = position + (zvec * size);

	DebugDrawLine( position, xvec, 255, 0, 0, noDepthTest, flDuration );
	DebugDrawLine( position, yvec, 0, 255, 0, noDepthTest, flDuration );
	DebugDrawLine( position, zvec, 0, 0, 255, noDepthTest, flDuration );
}

function DebugDrawHorzArrow( startPos, endPos, width, r, g, b, a, noDepthTest, flDuration )
{
	local lineDir = (endPos - startPos);
	lineDir.Norm();
	local radius = width / 2.0;

	local sideDir = lineDir.Cross( Vector( 0, 0, 1 ) );

	local p1 =	startPos - sideDir * radius;
	local p2 = endPos - lineDir * width - sideDir * radius;
	local p3 = endPos - lineDir * width - sideDir * width;
	local p4 = endPos;
	local p5 = endPos - lineDir * width + sideDir * width;
	local p6 = endPos - lineDir * width + sideDir * radius;
	local p7 =	startPos + sideDir * radius;

	// Outline the arrow
	DebugDrawLine( p1, p2, r, g, b, noDepthTest, flDuration );
	DebugDrawLine( p2, p3, r, g, b, noDepthTest, flDuration );
	DebugDrawLine( p3, p4, r, g, b, noDepthTest, flDuration );
	DebugDrawLine( p4, p5, r, g, b, noDepthTest, flDuration );
	DebugDrawLine( p5, p6, r, g, b, noDepthTest, flDuration );
	DebugDrawLine( p6, p7, r, g, b, noDepthTest, flDuration );

	if ( a > 0 )
	{
		// Fill us in with triangles
		DebugDrawTriangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration ); // Tip
		DebugDrawTriangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration ); // Shaft
		DebugDrawTriangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

		// And backfaces
		DebugDrawTriangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration ); // Tip
		DebugDrawTriangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration ); // Shaft
		DebugDrawTriangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
	}
}

function DebugDrawVertArrow( startPos, endPos, width, r, g, b, a, noDepthTest, flDuration )
{
	local lineDir = (endPos - startPos);
	lineDir.Norm();
	local radius = width / 2.0;

	local upVec = lineDir.Up();

	local p1 =	startPos - upVec * radius;
	local p2 = endPos - lineDir * width - upVec * radius;
	local p3 = endPos - lineDir * width - upVec * width;
	local p4 = endPos;
	local p5 = endPos - lineDir * width + upVec * width;
	local p6 = endPos - lineDir * width + upVec * radius;
	local p7 =	startPos + upVec * radius;

	// Outline the arrow
	DebugDrawLine(p1, p2, r,g,b,noDepthTest,flDuration);
	DebugDrawLine(p2, p3, r,g,b,noDepthTest,flDuration);
	DebugDrawLine(p3, p4, r,g,b,noDepthTest,flDuration);
	DebugDrawLine(p4, p5, r,g,b,noDepthTest,flDuration);
	DebugDrawLine(p5, p6, r,g,b,noDepthTest,flDuration);
	DebugDrawLine(p6, p7, r,g,b,noDepthTest,flDuration);

	if ( a > 0 )
	{
		// Fill us in with triangles
		DebugDrawTriangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration ); // Tip
		DebugDrawTriangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration ); // Shaft
		DebugDrawTriangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

		// And backfaces
		DebugDrawTriangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration ); // Tip
		DebugDrawTriangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration ); // Shaft
		DebugDrawTriangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
	}
}

function ScriptDebugDumpKeys( name, table = null )
{
	if( table == null )
	{
		table = getroottable();
	}

	if( name == "" )
	{
		printl( table.tostring() + "\n{" )
	}
	else
	{
		printl( "Find \"" + name +"\"\n{" );
	}

	local function PrintKey( keyPath, key, value )
	{
		printl( "    " + keyPath + " = " + value );
	}
	ScriptDebugIterateKeys( name, PrintKey, table );

	printl( "}" );
}

function ScriptDebugIterateKeys( name, callback, table = null )
{
	local visits = {};
	local pattern;

	local function MatchRegexp( keyPath )
	{
		return pattern.match( keyPath );
	}
	local function MatchSubstring( keyPath )
	{
		return keyPath.find( name ) != null;
	}
	local function MatchAll( keyPath )
	{
		return true;
	}

	local matchFunc;

	if( table == null )
	{
		table = getroottable();
	}

	if( name == "" )
	{
		matchFunc = MatchAll;
	}
	else if ( name[0] == "#" ) // exact
	{
		pattern = regexp( "^" + name + "$" )
		matchFunc = MatchRegexp;
	}
	else if ( name[0] == "@" ) // regexp
	{
		pattern = regexp( name.slice( 1 ) );
		matchFunc = MatchRegexp;
	}
	else //general
	{
		matchFunc = MatchSubstring;
	}

	ScriptDebugIterateKeysRecursive( matchFunc, null, table, visits, callback );
}

function ScriptDebugIterateKeysRecursive( matchFunc, path, current, visits, callback )
{
	if( !(current in visits) )
	{
		visits[current] <- true;

		foreach( key, val in current )
		{
			if( typeof(key) == "string" )
			{
				local keyPath = path ? path + "." + key : key;
				if( matchFunc(keyPath) )
				{
					callback( keyPath, key, value );
				}
			}

			if( typeof(value) == "table" )
			{
				ScriptDebugIterateKeysRecursive( matchFunc, keyPath, value, visits, callback );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Documentation table
//-----------------------------------------------------------------------------

local Documentation = {
	classes = {}
	functions = {}
	hooks = {}
	enums = {}
	constants = {}
}


function RetrieveNativeSignature( nativeFunction )
{
	if ( nativeFunction in NativeFunctionSignatures )
	{
		return NativeFunctionSignatures[nativeFunction]
	}
	return "<unnamed>"
}
	
function RegisterFunctionDocumentation( name, signature, description )
{
	if ( description.len() )
	{
		local b = ( description[0] == '#' );
		if ( description[0] == '#' )
		{
			local colon = description.find( ":" );
			if ( colon == null )
			{
				colon = description.len();
			}
			local alias = description.slice( 1, colon );
			description = description.slice( colon + 1 );
			name = alias;
			signature = "#";
		}
	}
	Documentation.functions[name] <- [ signature, description ]
}

function RegisterClassDocumentation( name, baseclass, members, description )
{
	Documentation.classes[name] <- [ baseclass, members, description ]
}

function RegisterHookDocumentation( name, signature, description )
{
	if ( description.len() )
	{
		local b = ( description[0] == '#' );
		if ( description[0] == '#' )
		{
			local colon = description.find( ":" );
			if ( colon == null )
			{
				colon = description.len();
			}
			local alias = description.slice( 1, colon );
			description = description.slice( colon + 1 );
			name = alias;
			signature = "#";
		}
	}
	Documentation.hooks[name] <- [ signature, description ]
}

function RegisterEnumDocumentation( name, constants, description )
{
	Documentation.enums[name] <- [ constants, description ]
}

function RegisterConstantDocumentation( name, type, value, description )
{
	if ( description.len() )
	{
		local b = ( description[0] == '#' );
		if ( description[0] == '#' )
		{
			local colon = description.find( ":" );
			if ( colon == null )
			{
				colon = description.len();
			}
			local alias = description.slice( 1, colon );
			description = description.slice( colon + 1 );
			name = alias;
		}
	}
	Documentation.constants[name] <- [ type, value, description ]
}

function Document( symbolOrTable, itemIfSymbol = null, descriptionIfSymbol = null )
{
	if ( typeof( symbolOrTable ) == "table" )
	{
		foreach( symbol, itemDescription in symbolOrTable )
		{
			Assert( typeof(symbol) == "string" )
				
			Document( symbol, itemDescription[0], itemDescription[1] );
		}
	}
	else
	{
		printl( symbolOrTable + ":" + itemIfSymbol.tostring() + "/" + descriptionIfSymbol );
	}
}

local function printdoc( text )
{
	return ::printc(200,224,255,text);
}

local function printdocl( text )
{
	return printdoc(text + "\n");
}

local function PrintFunc(name, doc)
{
	local text = "Function:    " + name + "\n"

	if (doc[0] == "#")
	{
		// Is an aliased function
		text += ("Signature:   function " + name + "(");
		foreach(k,v in this[name].getinfos().parameters)
		{
			if (k == 0 && v == "this") continue;
			if (k > 1) text += (", ");
			text += (v);
		}
		text += (")\n");
	}
	else
	{
		text += ("Signature:   " + doc[0] + "\n");
	}
	if (doc[1].len())
		text += ("Description: " + doc[1] + "\n");
	printdocl(text);
}

local function PrintHook(name, doc)
{
	local text = ("Hook:        " + name + "\n");
	if (doc[0] == "#")
	{
		// Is an aliased function
		text += ("Signature:   function " + name + "(");
		foreach(k,v in this[name].getinfos().parameters)
		{
			if (k == 0 && v == "this") continue;
			if (k > 1) text += (", ");
			text += (v);
		}
		text += (")\n");
	}
	else
	{
		text += ("Signature:   " + doc[0] + "\n");
	}
	if (doc[1].len())
		text += ("Description: " + doc[1] + "\n");
	printdocl(text);
}

local function PrintMember(name, doc)
{
	local text = ("Member:      " + name + "\n");
	text += ("Signature:   " + doc[0] + "\n");
	if (doc[1].len())
		text += ("Description: " + doc[1] + "\n");
	printdocl(text);
}

local function PrintClass(name, doc)
{
	local text = "=====================================\n";
	text += ("Class:       " + name + "\n");
	text += ("Base:        " + doc[0] + "\n");
	foreach( k,v in doc[1] ) {
		PrintMember( k, v );
	}
	if (doc[2].len())
		text += ("Description: " + doc[2] + "\n");
	text += "=====================================\n\n";

	printdoc(text);
}

local function PrintConst(name, doc)
{
	local text = ("Constant:    " + name + "\n");
	text +=      ("Type:        " + doc[0] + "\n");
	if (doc[1] == null)
	{
		text += ("Value:       null\n");
	}
	else
	{
		text += ("Value:       " + doc[1] + "\n");
	}
	if (doc[2].len())
		text += ("Description: " + doc[2] + "\n");
	printdocl(text);
}

local function PrintEnum(name, doc)
{
	local text = "=====================================\n";
	text += ("Enum:        " + name + "\n");
	foreach( k,v in doc[0] ) {
		text += ("  Name:        " + k + "\n");
		text += ("  Value:       " + v[0] + "\n");
		text += ("  Description: " + v[1] + "\n");
	}
	if (doc[1].len())
		text += ("Description: " + doc[1] + "\n");
	text += "=====================================\n\n";

	printdoc(text);
}
	
function PrintHelp( string = "*", exact = false )
{
	local matches = []
		
	if ( string == "*" || !exact )
	{
		local AppendMatches = function( doclist ) {
			foreach( name, doc in doclist )
			{
				if ( string != "*" && name.tolower().find( string.tolower() ) == null )
				{
					continue;
				}
				
				matches.append( name );
			}
		}

		AppendMatches( Documentation.functions );
		AppendMatches( Documentation.hooks );
		AppendMatches( Documentation.classes );
		AppendMatches( Documentation.enums );
		AppendMatches( Documentation.constants );
	} 
	else if ( exact )
	{
		if ( string in Documentation.functions )
			matches.append( string );

		if ( string in Documentation.hooks )
			matches.append( string );

		if ( string in Documentation.classes )
			matches.append( string );

		if ( string in Documentation.enums )
			matches.append( string );

		if ( string in Documentation.constants )
			matches.append( string );
	}
		
	if ( matches.len() == 0 )
	{
		printl( "Symbol " + string + " not found" );
		return;
	}
		
	matches.sort();
		
	foreach( name in matches )
	{
		if ( name in Documentation.functions )
			PrintFunc( name, Documentation.functions[name] );

		if ( name in Documentation.hooks )
			PrintHook( name, Documentation.hooks[name] );

		if ( name in Documentation.classes )
			PrintClass( name, Documentation.classes[name] );

		if ( name in Documentation.enums )
			PrintEnum( name, Documentation.enums[name] );

		if ( name in Documentation.constants )
			PrintConst( name, Documentations.constants[name] );
	}
}

//-----------------------------------------------------------------------------
// VSquirrel support functions
//-----------------------------------------------------------------------------

function VSquirrel_OnCreateScope( name, outer )
{
	local result;
	if ( !(name in outer) )
	{
		result = outer[name] <- { __vname=name, __vrefs = 1 };
		result.setdelegate( outer );
	}
	else
	{
		result = outer[name];
		result.__vrefs += 1;
	}
	return result;
}

function VSquirrel_OnReleaseScope( scope )
{
	scope.__vrefs -= 1;
	if ( scope.__vrefs < 0 )
	{
		throw "Bad reference counting on scope " + scope.__vname;
	}
	else if ( scope.__vrefs == 0 )
	{
		delete scope.parent[scope.__vname];
		scope.__vname = null;
		scope.setdelegate( null );
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CCallChainer
{
	constructor( prefixString, scopeForThis = null )
	{
		prefix = prefixString;
		if ( scopeForThis != null )
			scope = scopeForThis;
		else
			scope = ::getroottable();
		chains = {};
		
		// Expose a bound global function to dispatch to this object
		scope[ "Dispatch" + prefixString ] <- Call.bindenv( this );
	}
	
	function PostScriptExecute() 
	{
		foreach( key, value in scope )
		{
			if ( typeof( value ) == "function" ) 
			{
				if ( key.find( prefix ) == 0 )
				{
					key = key.slice( prefix.len() );
					
					if ( !(key in chains) )
					{
						//::print( "Creating new call chain " + key + "\n");
						chains[key] <- [];
					}
					
					local chain = chains[key];
					
					if ( !chain.len() || chain.top() != value )
					{
						chain.push( value );
						//::print( "Added " + value + " to call chain " + key + "\n" );
					}
				}
			}
		}
	}
	
	function Call( event, ... )
	{
		if ( event in chains )
		{
			local chain = chains[event];
			if ( chain.len() )
			{
				local i;
				local args = [];
				if ( vargv.len() > 0 )
				{
					args.push( scope );
					for ( i = 0; i < vegv.len(); i++ )
					{
						args.push( vargv[i] );
					}
				}
				for ( i = chain.len() - 1; i >= 0; i -= 1 )
				{
					local func = chain[i];
					local result;
					if ( !args.len() )
					{
						result = func();
					}
					else
					{
						result = func.acall( args ); 
					}
					if ( result != null && !result )
						return false;
				}
			}
		}
		
		return true;
	}
	
	scope = null;
	prefix = null;
	chains = null;
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CSimpleCallChainer
{
	constructor( prefixString, scopeForThis = null, exactNameMatch = false )
	{
		prefix = prefixString;
		if ( scopeForThis != null )
			scope = scopeForThis;
		else
			scope = ::getroottable();
		chain = [];
		
		// Expose a bound global function to dispatch to this object
		scope[ "Dispatch" + prefixString ] <- Call.bindenv( this );
		
		exactMatch = exactNameMatch
	}
	
	function PostScriptExecute() 
	{
		foreach( key, value in scope )
		{
			if ( typeof( value ) == "function" ) 
			{
				local foundMatch = false;
				if ( exactMatch )
				{
					foundMatch = ( prefix == key );
				}
				else
				{
					foundMatch = ( key.find( prefix ) == 0 )
				}
						
				if ( foundMatch )
				{
					if ( !exactMatch )
						key = key.slice( prefix.len() );
					
					if ( !(chain) )
					{
						//::print( "Creating new call simple chain\n");
						chain <- [];
					}
					
					if ( !chain.len() || chain != value )
					{
						chain.push( value );
						//::print( "Added " + value + " to call chain.\n" );
					}
				}
			}
		}
	}
	
	function Call( ... )
	{
		if ( chain.len() )
		{
			local i;
			local args = [];
			if ( vargv.len() > 0 )
			{
				args.push( scope );
				for ( i = 0; i < vargv.len(); i++ )
				{
					args.push( vargv[i] );
				}
			}
			for ( i = chain.len() - 1; i >= 0; i -= 1 )
			{
				local func = chain[i];
				local result;
				if ( args.len() == 0 )
				{
					result = func.pcall( scope );
				}
				else
				{
					result = func.pacall( scope, args ); 
				}
				if ( result != null && !result )
					return false;
			}
		}
		
		return true;
	}
	
	exactMatch = false
	scope = null;
	prefix = null;
	chain = null;
};

//-----------------------------------------------------------------------------
// Late binding: allows a table to refer to parts of itself, it's children,
// it's owner, and then have the references fixed up after it's fully parsed
//
// Usage:
//    lateBinder <- LateBinder();
//    lateBinder.Begin( this );
//    
//    Test1 <-
//    {   
// 	   Foo=1
//    }   
//    
//    Test2 <-
//    {   
// 	   FooFoo = "I'm foo foo"
// 	   BarBar="@Test1.Foo"
// 	   SubTable = { boo=[bah, "@Test2.FooFoo", "@Test1.Foo"], booboo2={one=bah, two="@Test2.FooFoo", three="@Test1.Foo"} }
// 	   booboo=[bah, "@Test2.FooFoo", "@Test1.Foo"]
// 	   booboo2={one=bah, two="@Test2.FooFoo", three="@Test1.Foo"}
// 	   bah=wha
//    }   
//    
//    lateBinder.End();
//    delete lateBinder;
//
// When End() is called, all of the unresolved symbols in the tables and arrays will be resolved,
// any left unresolved will become a string prepended with '~', which later code can deal with
//-----------------------------------------------------------------------------

class LateBinder
{
	// public:
	function Begin( target, log = false )
	{
		m_log = log;
		
		HookRootMetamethod( "_get", function( key ) { return "^" + key; } );
		HookRootMetamethod( "_newslot", function( key, value ) { if ( typeof value == "table" ) { m_fixupSet.push( [ key, value ] ); this.rawset( key, value ); };  }.bindenv(this) );
		m_targetTable = target;
		
		Log( "Begin late bind on table " + m_targetTable );
	}
	
	function End()
	{
		UnhookRootMetamethod( "_get" );
		UnhookRootMetamethod( "_newslot" );

		Log( "End late bind on table " + m_targetTable );
		
		foreach( subTablePair in m_fixupSet )
		{
			EstablishDelegation( m_targetTable, subTablePair[1] );
		}

		Log( "Begin resolution... " )
		m_logIndent++;
		
		local found = true;
		
		while ( found )
		{
			foreach( subTablePair in m_fixupSet )
			{
				Log( subTablePair[0] + " = " );
				Log( "{" );
				if ( !Resolve( subTablePair[1], subTablePair[1], false ) )
				{
					found = false;
				}
				Log( "}" );
			}
		}
			
		m_logIndent--;
		
		foreach( subTablePair in m_fixupSet )
		{
			RemoveDelegation( subTablePair[1] );
		}
		
		Log( "...end resolution" );
	}
		
	// private:
	function HookRootMetamethod( name, value )
	{
		local saved = null;
		local roottable = getroottable();
		if ( name in roottable )
		{
			saved = roottable[name];
		}
		roottable[name] <- value;
		roottable["__saved" + name] <- saved;
	}

	function UnhookRootMetamethod( name )
	{
		local saveSlot = "__saved" + name;
		local roottable = getroottable();
		local saved = roottable[saveSlot];
		if ( saved != null )
		{
			roottable[name] = saved;
		}
		else
		{
			delete roottable[name];
		}
		delete roottable[saveSlot];
	}

	function EstablishDelegation( parentTable, childTable )
	{
		childTable.setdelegate( parentTable );
		
		foreach( key, value in childTable )
		{
			local type = typeof value;
			if ( type == "table" )
			{
				EstablishDelegation( childTable, value );
			}
		}
	}
	
	function RemoveDelegation( childTable )
	{
		childTable.setdelegate( null );
		
		foreach( key, value in childTable )
		{
			local type = typeof value;
			if ( type == "table" )
			{
				RemoveDelegation( value );
			}
		}
	}

	function Resolve( lookupTable, subTableOrArray, throwException = false )
	{
		m_logIndent++;
		local found = false;
	
		foreach( key, value in subTableOrArray )
		{
			local type = typeof value;
			if ( type == "string" )
			{
				if ( value.len() )
				{
					local unresolvedId = null;
					local controlChar = value[0]
					if ( controlChar == '^' )
					{
						found = true;
						value = value.slice( 1 );
						if ( value in lookupTable )
						{
							subTableOrArray[key] = lookupTable[value];
							Log( key + " = " + lookupTable[value] + " <-- " + value );
						}
						else
						{
							subTableOrArray[key] = "~" + value;
							unresolvedId = value;
							Log( key + " = \"" + "~" + value + "\" (unresolved)" );
						}
					}
					else if ( controlChar == '@' )
					{
						found = true;
						local identifiers = [];
						local iLast = 1;
						local iNext;
						while ( ( iNext = value.find( ".", iLast ) ) != null )
						{
							identifiers.push( value.slice( iLast, iNext ) );
							iLast = iNext + 1;
						}
						identifiers.push( value.slice( iLast ) );
						
						local depthSuccess = 0;
						local result = lookupTable;
						foreach( identifier in identifiers )
						{
							if ( identifier in result )
							{
								depthSuccess++;
								result = result[identifier];
							}
							else
							{
								break;
							}
						}
						if ( depthSuccess == identifiers.len() )
						{
							subTableOrArray[key] = result;
							Log( key + " = " + result + " <-- " + value );
						}
						else
						{
							subTableOrArray[key] = "~" + value.slice( 1 );
							unresolvedId = value;
							Log( key + " = \"" + "~" + value + "\" (unresolved)" );
						}
					}
					
					if ( unresolvedId != null )
					{
						if ( throwException )
						{
							local exception = "Unresolved symbol: " + bind + " in ";
							foreach ( entry in m_bindNamesStack )
							{
								exception += entry;
								exception += "."
							}
							exception += unresolvedId;
							
							throw exception; 
						}
					}
				}
			}
		}

		foreach( key, value in subTableOrArray )
		{
			local type = typeof value;
			local isTable = ( type == "table" );
			local isArray = ( type == "array" )
			if ( isTable || isArray )
			{
				Log( key + " =" );
				Log( isTable ? "{" : "[" );
				
				m_bindNamesStack.push( key );
				if ( Resolve( ( isTable ) ? value : lookupTable, value, throwException ) )
				{
					found = true;
				}
				m_bindNamesStack.pop();
				
				Log( isTable ? "}" : "]" );
			}
		}
		m_logIndent--;
		return found;
	}
	
	function Log( string )
	{
		if ( m_log )
		{
			for ( local i = 0; i < m_logIndent; i++ )
			{
				print( "  " );
			}
			
			printl( string );
		}
	}

	m_targetTable = null;
	m_fixupSet = [];
	m_bindNamesStack = [];
	m_log = false;
	m_logIndent = 0;
}