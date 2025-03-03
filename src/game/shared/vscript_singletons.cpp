//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: This file contains brand new VScript singletons and singletons replicated from API
//			documentation in other games.
//
//			See vscript_funcs_shared.cpp for more information.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <vgui_controls/Controls.h> 
#include <vgui/ILocalize.h>
#include "ammodef.h"
#include "tier1/utlcommon.h"
#include "tier1/utlbuffer.h"

#ifndef CLIENT_DLL
#include "ai_squad.h"
#include "recipientfilter.h"
#endif // !CLIENT_DLL

#include "usermessages.h"
#include "filesystem.h"
#include "igameevents.h"
#include "engine/ivdebugoverlay.h"

#ifdef CLIENT_DLL
#include "IEffects.h"
#include "fx.h"
#include "itempents.h"
#include "c_te_legacytempents.h"
#include "iefx.h"
#include "dlight.h"
#endif

#include "vscript_singletons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IScriptManager *scriptmanager;
//CNetMsgScriptHelper *g_ScriptNetMsg = new CNetMsgScriptHelper();

//=============================================================================
// Localization Interface
// Unique to Mapbase
//=============================================================================
class CScriptLocalize
{
public:

	const char *GetTokenAsUTF8( const char *pszToken )
	{
		const char *pText = g_pVGuiLocalize->FindAsUTF8( pszToken );
		if ( pText )
		{
			return pText;
		}

		return NULL;
	}

	void AddStringAsUTF8( const char *pszToken, const char *pszString )
	{
		wchar_t wpszString[256];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszString, wpszString, sizeof(wpszString) );

		// TODO: This is a fake file name! Should "fileName" mean anything?
		g_pVGuiLocalize->AddString( pszToken, wpszString, "resource/vscript_localization.txt" );
	}

private:
} g_ScriptLocalize;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptLocalize, "CLocalize", SCRIPT_SINGLETON "Accesses functions related to localization strings." )

	DEFINE_SCRIPTFUNC( GetTokenAsUTF8, "Gets the current language's token as a UTF-8 string (not Unicode)." )

	DEFINE_SCRIPTFUNC( AddStringAsUTF8, "Adds a new localized token as a UTF-8 string (not Unicode)." )

END_SCRIPTDESC();

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static void FireGameEvent( const char* szEvent, HSCRIPT hTable )
{
	IGameEvent *event = gameeventmanager->CreateEvent( szEvent );
	if ( event )
	{
		ScriptVariant_t key, val;
		int nIterator = -1;
		while ( ( nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &key, &val ) ) != -1 )
		{
			switch ( val.GetType() )
			{
				case FIELD_FLOAT:   event->SetFloat ( key, val ); break;
				case FIELD_INTEGER: event->SetInt   ( key, val ); break;
				case FIELD_BOOLEAN: event->SetBool  ( key, val ); break;
				case FIELD_CSTRING: event->SetString( key, val ); break;
				case FIELD_UINT64:  event->SetUint64( key, val ); break;
			}

			g_pScriptVM->ReleaseValue(key);
			g_pScriptVM->ReleaseValue(val);
		}

#ifdef CLIENT_DLL
		gameeventmanager->FireEventClientSide(event);
#else
		gameeventmanager->FireEvent(event);
#endif
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Copy of FireGameEvent, server only with no broadcast to clients.
//-----------------------------------------------------------------------------
static void FireGameEventLocal( const char* szEvent, HSCRIPT hTable )
{
	IGameEvent *event = gameeventmanager->CreateEvent( szEvent );
	if ( event )
	{
		ScriptVariant_t key, val;
		int nIterator = -1;
		while ( ( nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &key, &val ) ) != -1 )
		{
			switch ( val.GetType() )
			{
				case FIELD_FLOAT:   event->SetFloat ( key, val ); break;
				case FIELD_INTEGER: event->SetInt   ( key, val ); break;
				case FIELD_BOOLEAN: event->SetBool  ( key, val ); break;
				case FIELD_CSTRING: event->SetString( key, val ); break;
				case FIELD_UINT64:  event->SetUint64( key, val ); break;
			}

			g_pScriptVM->ReleaseValue(key);
			g_pScriptVM->ReleaseValue(val);
		}

		gameeventmanager->FireEvent(event,true);
	}
}
#endif // !CLIENT_DLL

//=============================================================================
// Save/Restore Utility
// Based on L4D2 API
//=============================================================================
class CScriptSaveRestoreUtil : public CAutoGameSystem
{
public:
	static void SaveTable( const char *szId, HSCRIPT hTable );
	static void RestoreTable( const char *szId, HSCRIPT hTable );
	static void ClearSavedTable( const char *szId );

public: // IGameSystem

	void OnSave()
	{
		if ( g_pScriptVM )
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction( "OnSave" );
			if ( hFunc )
			{
				g_pScriptVM->Call( hFunc );
				g_pScriptVM->ReleaseScript( hFunc );
			}
		}
	}

#ifdef CLIENT_DLL
	// On the client, OnRestore() is called before VScript is actually restored, so this has to be called manually from VScript save/restore instead
	void OnVMRestore()
#else
	void OnRestore()
#endif
	{
		if ( g_pScriptVM )
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction( "OnRestore" );
			if ( hFunc )
			{
				g_pScriptVM->Call( hFunc );
				g_pScriptVM->ReleaseScript( hFunc );
			}
		}
	}

	void Shutdown()
	{
		FOR_EACH_MAP_FAST( m_Lookup, i )
			m_Lookup[i]->deleteThis();
		m_Lookup.Purge();
	}

private:
	static StringHashFunctor Hash;
	static CUtlMap< unsigned int, KeyValues* > m_Lookup;

} g_ScriptSaveRestoreUtil;

#ifdef CLIENT_DLL
void VScriptSaveRestoreUtil_OnVMRestore()
{
	g_ScriptSaveRestoreUtil.OnVMRestore();
}
#endif

CUtlMap< unsigned int, KeyValues* > CScriptSaveRestoreUtil::m_Lookup( DefLessFunc(unsigned int) );
StringHashFunctor CScriptSaveRestoreUtil::Hash;

//-----------------------------------------------------------------------------
// Store a table with primitive values that will persist across level transitions and save loads.
// Case sensitive
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::SaveTable( const char *szId, HSCRIPT hTable )
{
	KeyValues *pKV;
	unsigned int hash = Hash(szId);

	int idx = m_Lookup.Find( hash );
	if ( idx == m_Lookup.InvalidIndex() )
	{
		pKV = new KeyValues("ScriptSavedTable");
		m_Lookup.Insert( hash, pKV );
	}
	else
	{
		pKV = m_Lookup[idx];
		pKV->Clear();
	}

	ScriptVariant_t key, val;
	int nIterator = -1;
	while ( ( nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &key, &val ) ) != -1 )
	{
		switch ( val.GetType() )
		{
			case FIELD_FLOAT:   pKV->SetFloat ( key, val ); break;
			case FIELD_INTEGER: pKV->SetInt   ( key, val ); break;
			case FIELD_BOOLEAN: pKV->SetBool  ( key, val ); break;
			case FIELD_CSTRING: pKV->SetString( key, val ); break;
			case FIELD_UINT64:  pKV->SetUint64( key, val ); break;
		}

		g_pScriptVM->ReleaseValue(key);
		g_pScriptVM->ReleaseValue(val);
	}
}

//-----------------------------------------------------------------------------
// Retrieves a table from storage. Write into input table.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::RestoreTable( const char *szId, HSCRIPT hTable )
{
	int idx = m_Lookup.Find( Hash(szId) );
	if ( idx == m_Lookup.InvalidIndex() )
	{
		// DevWarning( 2, "RestoreTable could not find saved table with context '%s'\n", szId );
		return;
	}

	KeyValues *pKV = m_Lookup[idx];
	FOR_EACH_SUBKEY( pKV, key )
	{
		switch ( key->GetDataType() )
		{
			case KeyValues::TYPE_STRING: g_pScriptVM->SetValue( hTable, key->GetName(), key->GetString() ); break;
			case KeyValues::TYPE_INT:    g_pScriptVM->SetValue( hTable, key->GetName(), key->GetInt()    ); break;
			case KeyValues::TYPE_FLOAT:  g_pScriptVM->SetValue( hTable, key->GetName(), key->GetFloat()  ); break;
			case KeyValues::TYPE_UINT64: g_pScriptVM->SetValue( hTable, key->GetName(), key->GetUint64()  ); break;
		}
	}
}

//-----------------------------------------------------------------------------
// Remove a saved table.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::ClearSavedTable( const char *szId )
{
	int idx = m_Lookup.Find( Hash(szId) );
	if ( idx != m_Lookup.InvalidIndex() )
	{
		m_Lookup[idx]->deleteThis();
		m_Lookup.RemoveAt( idx );
	}
	else
	{
		// DevWarning( 2, "ClearSavedTable could not find saved table with context '%s'\n", szId );
	}
}

//=============================================================================
// Read/Write to File
// Based on L4D2/Source 2 API
//=============================================================================
#define SCRIPT_MAX_FILE_READ_SIZE  (16 * 1024)			// 16KB
#define SCRIPT_MAX_FILE_WRITE_SIZE (64 * 1024 * 1024)	// 64MB
#define SCRIPT_RW_PATH_ID "MOD"
#define SCRIPT_RW_FULL_PATH_FMT "vscript_io/%s"

class CScriptReadWriteFile : public CAutoGameSystem
{
	// A singleton class with all static members is used to be able to free the read string on level shutdown,
	// and register script funcs directly. Same reason applies to CScriptSaveRestoreUtil
public:
	static bool FileWrite( const char *szFile, const char *szInput );
	static const char *FileRead( const char *szFile );

	// NOTE: These two functions are new with Mapbase and have no Valve equivalent
	static bool KeyValuesWrite( const char *szFile, HSCRIPT hInput );
	static HSCRIPT KeyValuesRead( const char *szFile );

	void LevelShutdownPostEntity()
	{
		if ( m_pszReturnReadFile )
		{
			delete[] m_pszReturnReadFile;
			m_pszReturnReadFile = NULL;
		}
	}

private:
	static const char *m_pszReturnReadFile;

} g_ScriptReadWrite;

const char *CScriptReadWriteFile::m_pszReturnReadFile = NULL;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CScriptReadWriteFile::FileWrite( const char *szFile, const char *szInput )
{
	size_t len = strlen(szInput);
	if ( len > SCRIPT_MAX_FILE_WRITE_SIZE )
	{
		DevWarning( 2, "Input is too large for a ScriptFileWrite ( %s / %d MB )\n", V_pretifymem(len,2,true), (SCRIPT_MAX_FILE_WRITE_SIZE >> 20) );
		return false;
	}

	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		return false;
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	buf.PutString(szInput);

	int nSize = V_strlen(pszFullName) + 1;
	char *pszDir = (char*)stackalloc(nSize);
	V_memcpy( pszDir, pszFullName, nSize );
	V_StripFilename( pszDir );

	g_pFullFileSystem->CreateDirHierarchy( pszDir, SCRIPT_RW_PATH_ID );
	bool res = g_pFullFileSystem->WriteFile( pszFullName, SCRIPT_RW_PATH_ID, buf );
	buf.Purge();
	return res;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
const char *CScriptReadWriteFile::FileRead( const char *szFile )
{
	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		return NULL;
	}

	unsigned int size = g_pFullFileSystem->Size( pszFullName, SCRIPT_RW_PATH_ID );
	if ( size >= SCRIPT_MAX_FILE_READ_SIZE )
	{
		DevWarning( 2, "File '%s' (from '%s') is too large for a ScriptFileRead ( %s / %u bytes )\n", pszFullName, szFile, V_pretifymem(size,2,true), SCRIPT_MAX_FILE_READ_SIZE );
		return NULL;
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !g_pFullFileSystem->ReadFile( pszFullName, SCRIPT_RW_PATH_ID, buf, SCRIPT_MAX_FILE_READ_SIZE ) )
	{
		return NULL;
	}

	// first time calling, allocate
	if ( !m_pszReturnReadFile )
		m_pszReturnReadFile = new char[SCRIPT_MAX_FILE_READ_SIZE];

	V_strncpy( const_cast<char*>(m_pszReturnReadFile), (const char*)buf.Base(), buf.Size() );
	buf.Purge();
	return m_pszReturnReadFile;
}

//-----------------------------------------------------------------------------
// Get the checksum of any file. Can be used to check the existence or validity of a file.
// Returns unsigned int as hex string.
//-----------------------------------------------------------------------------
/*
const char *CScriptReadWriteFile::CRC32_Checksum( const char *szFilename )
{
	CUtlBuffer buf( 0, 0, CUtlBuffer::READ_ONLY );
	if ( !g_pFullFileSystem->ReadFile( szFilename, NULL, buf ) )
		return NULL;

	// first time calling, allocate
	if ( !m_pszReturnCRC32 )
		m_pszReturnCRC32 = new char[9]; // 'FFFFFFFF\0'

	V_snprintf( const_cast<char*>(m_pszReturnCRC32), 9, "%X", CRC32_ProcessSingleBuffer( buf.Base(), buf.Size()-1 ) );
	buf.Purge();

	return m_pszReturnCRC32;
}
*/

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CScriptReadWriteFile::KeyValuesWrite( const char *szFile, HSCRIPT hInput )
{
	KeyValues *pKV = HScriptToClass<CScriptKeyValues>(hInput)->GetKeyValues();
	if (!pKV)
	{
		return false;
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	pKV->RecursiveSaveToFile( buf, 0 );

	if ( buf.Size() > SCRIPT_MAX_FILE_WRITE_SIZE )
	{
		DevWarning( 2, "Input is too large for a ScriptKeyValuesWrite ( %s / %d MB )\n", V_pretifymem(buf.Size(),2,true), (SCRIPT_MAX_FILE_WRITE_SIZE >> 20) );
		buf.Purge();
		return false;
	}

	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		buf.Purge();
		return false;
	}

	int nSize = V_strlen(pszFullName) + 1;
	char *pszDir = (char*)stackalloc(nSize);
	V_memcpy( pszDir, pszFullName, nSize );
	V_StripFilename( pszDir );

	g_pFullFileSystem->CreateDirHierarchy( pszDir, SCRIPT_RW_PATH_ID );
	bool res = g_pFullFileSystem->WriteFile( pszFullName, SCRIPT_RW_PATH_ID, buf );
	buf.Purge();
	return res;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
HSCRIPT CScriptReadWriteFile::KeyValuesRead( const char *szFile )
{
	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		return NULL;
	}

	unsigned int size = g_pFullFileSystem->Size( pszFullName, SCRIPT_RW_PATH_ID );
	if ( size >= SCRIPT_MAX_FILE_READ_SIZE )
	{
		DevWarning( 2, "File '%s' (from '%s') is too large for a ScriptKeyValuesRead ( %s / %u bytes )\n", pszFullName, szFile, V_pretifymem(size,2,true), SCRIPT_MAX_FILE_READ_SIZE );
		return NULL;
	}

	KeyValues *pKV = new KeyValues( szFile );
	if ( !pKV->LoadFromFile( g_pFullFileSystem, pszFullName, SCRIPT_RW_PATH_ID ) )
	{
		pKV->deleteThis();
		return NULL;
	}

	return (HSCRIPT)new CScriptKeyValues( pKV );
}
#undef SCRIPT_MAX_FILE_READ_SIZE
#undef SCRIPT_MAX_FILE_WRITE_SIZE
#undef SCRIPT_RW_PATH_ID
#undef SCRIPT_RW_FULL_PATH_FMT


#define RETURN_IF_CANNOT_DRAW_OVERLAY\
	if (engine->IsPaused())\
		return;
class CDebugOverlayScriptHelper
{
public:

	void Box( const Vector &origin, const Vector &mins, const Vector &maxs, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddBoxOverlay(origin, mins, maxs, vec3_angle, r, g, b, a, flDuration);
	}
	void BoxDirection( const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &forward, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		QAngle f_angles = vec3_angle;
		f_angles.y = UTIL_VecToYaw(forward);

		debugoverlay->AddBoxOverlay(origin, mins, maxs, f_angles, r, g, b, a, flDuration);
	}
	void BoxAngles( const Vector &origin, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, r, g, b, a, flDuration);
	}
	void SweptBox( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle & angles, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddSweptBoxOverlay(start, end, mins, maxs, angles, r, g, b, a, flDuration);
	}
	void EntityBounds( HSCRIPT pEntity, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		CBaseEntity *pEnt = ToEnt(pEntity);
		if (!pEnt)
			return;

		const CCollisionProperty *pCollide = pEnt->CollisionProp();
		debugoverlay->AddBoxOverlay(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), pCollide->GetCollisionAngles(), r, g, b, a, flDuration);
	}
	void Line( const Vector &origin, const Vector &target, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddLineOverlay(origin, target, r, g, b, noDepthTest, flDuration);
	}
	void Triangle( const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddTriangleOverlay(p1, p2, p3, r, g, b, a, noDepthTest, duration);
	}
	void EntityText( int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddEntityTextOverlay(entityID, text_offset, flDuration,
				(int)clamp(r * 255.f, 0.f, 255.f), (int)clamp(g * 255.f, 0.f, 255.f), (int)clamp(b * 255.f, 0.f, 255.f),
				(int)clamp(a * 255.f, 0.f, 255.f), text);
	}
	void EntityTextAtPosition( const Vector &origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddTextOverlayRGB(origin, text_offset, flDuration, r, g, b, a, "%s", text);
	}
	void Grid( const Vector &vPosition )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddGridOverlay(vPosition);
	}
	void Text( const Vector &origin, const char *text, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddTextOverlay(origin, flDuration, "%s", text);
	}
	void ScreenText( float fXpos, float fYpos, const char *text, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddScreenTextOverlay(fXpos, fYpos, flDuration, r, g, b, a, text);
	}
	void Cross3D( const Vector &position, float size, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Line( position + Vector(size,0,0), position - Vector(size,0,0), r, g, b, noDepthTest, flDuration );
		Line( position + Vector(0,size,0), position - Vector(0,size,0), r, g, b, noDepthTest, flDuration );
		Line( position + Vector(0,0,size), position - Vector(0,0,size), r, g, b, noDepthTest, flDuration );
	}
	void Cross3DOriented( const Vector &position, const QAngle &angles, float size, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector forward, right, up;
		AngleVectors( angles, &forward, &right, &up );

		forward *= size;
		right *= size;
		up *= size;

		Line( position + right, position - right, r, g, b, noDepthTest, flDuration );
		Line( position + forward, position - forward, r, g, b, noDepthTest, flDuration );
		Line( position + up, position - up, r, g, b, noDepthTest, flDuration );
	}
	void DrawTickMarkedLine( const Vector &startPos, const Vector &endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector	lineDir = (endPos - startPos);
		float	lineDist = VectorNormalize(lineDir);
		int		numTicks = lineDist / tickDist;

		Vector  upVec = Vector(0,0,4);
		Vector	sideDir;
		Vector	tickPos = startPos;
		int		tickTextCnt = 0;

		CrossProduct(lineDir, upVec, sideDir);

		Line(startPos, endPos, r, g, b, noDepthTest, flDuration);

		for (int i = 0; i<numTicks + 1; i++)
		{
			Vector tickLeft = tickPos - sideDir;
			Vector tickRight = tickPos + sideDir;

			if (tickTextCnt == tickTextDist)
			{
				char text[25];
				Q_snprintf(text, sizeof(text), "%i", i);
				Vector textPos = tickLeft + Vector(0, 0, 8);
				Line(tickLeft, tickRight, 255, 255, 255, noDepthTest, flDuration);
				Text(textPos, text, flDuration);
				tickTextCnt = 0;
			}
			else
			{
				Line(tickLeft, tickRight, r, g, b, noDepthTest, flDuration);
			}

			tickTextCnt++;

			tickPos = tickPos + (tickDist * lineDir);
		}
	}
	void HorzArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector	lineDir		= (endPos - startPos);
		VectorNormalize( lineDir );
		Vector  upVec		= Vector( 0, 0, 1 );
		Vector	sideDir;
		float   radius		= width / 2.0;

		CrossProduct(lineDir, upVec, sideDir);

		Vector p1 =	startPos - sideDir * radius;
		Vector p2 = endPos - lineDir * width - sideDir * radius;
		Vector p3 = endPos - lineDir * width - sideDir * width;
		Vector p4 = endPos;
		Vector p5 = endPos - lineDir * width + sideDir * width;
		Vector p6 = endPos - lineDir * width + sideDir * radius;
		Vector p7 =	startPos + sideDir * radius;

		Line(p1, p2, r,g,b,noDepthTest,flDuration);
		Line(p2, p3, r,g,b,noDepthTest,flDuration);
		Line(p3, p4, r,g,b,noDepthTest,flDuration);
		Line(p4, p5, r,g,b,noDepthTest,flDuration);
		Line(p5, p6, r,g,b,noDepthTest,flDuration);
		Line(p6, p7, r,g,b,noDepthTest,flDuration);

		if ( a > 0 )
		{
			Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

			Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
		}
	}
	void YawArrow( const Vector &startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector forward = UTIL_YawToVector( yaw );
		HorzArrow( startPos, startPos + forward * length, width, r, g, b, a, noDepthTest, flDuration );
	}
	void VertArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector	lineDir		= (endPos - startPos);
		VectorNormalize( lineDir );
		Vector  upVec;
		Vector	sideDir;
		float   radius		= width / 2.0;

		VectorVectors( lineDir, sideDir, upVec );

		Vector p1 =	startPos - upVec * radius;
		Vector p2 = endPos - lineDir * width - upVec * radius;
		Vector p3 = endPos - lineDir * width - upVec * width;
		Vector p4 = endPos;
		Vector p5 = endPos - lineDir * width + upVec * width;
		Vector p6 = endPos - lineDir * width + upVec * radius;
		Vector p7 =	startPos + upVec * radius;

		Line(p1, p2, r,g,b,noDepthTest,flDuration);
		Line(p2, p3, r,g,b,noDepthTest,flDuration);
		Line(p3, p4, r,g,b,noDepthTest,flDuration);
		Line(p4, p5, r,g,b,noDepthTest,flDuration);
		Line(p5, p6, r,g,b,noDepthTest,flDuration);
		Line(p6, p7, r,g,b,noDepthTest,flDuration);

		if ( a > 0 )
		{
			Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

			Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
		}
	}
	void Axis( const Vector &position, const QAngle &angles, float size, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector xvec, yvec, zvec;
		AngleVectors( angles, &xvec, &yvec, &zvec );

		xvec = position + (size * xvec);
		yvec = position - (size * yvec);
		zvec = position + (size * zvec);

		Line( position, xvec, 255, 0, 0, noDepthTest, flDuration );
		Line( position, yvec, 0, 255, 0, noDepthTest, flDuration );
		Line( position, zvec, 0, 0, 255, noDepthTest, flDuration );
	}
	void Sphere( const Vector &center, float radius, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector edge, lastEdge;

		float axisSize = radius;
		Line( center + Vector( 0, 0, -axisSize ), center + Vector( 0, 0, axisSize ), r, g, b, noDepthTest, flDuration );
		Line( center + Vector( 0, -axisSize, 0 ), center + Vector( 0, axisSize, 0 ), r, g, b, noDepthTest, flDuration );
		Line( center + Vector( -axisSize, 0, 0 ), center + Vector( axisSize, 0, 0 ), r, g, b, noDepthTest, flDuration );

		lastEdge = Vector( radius + center.x, center.y, center.z );
		float angle;
		for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
		{
			edge.x = radius * cosf( angle / 180.0f * M_PI ) + center.x;
			edge.y = center.y;
			edge.z = radius * sinf( angle / 180.0f * M_PI ) + center.z;

			Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

			lastEdge = edge;
		}

		lastEdge = Vector( center.x, radius + center.y, center.z );
		for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
		{
			edge.x = center.x;
			edge.y = radius * cosf( angle / 180.0f * M_PI ) + center.y;
			edge.z = radius * sinf( angle / 180.0f * M_PI ) + center.z;

			Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

			lastEdge = edge;
		}

		lastEdge = Vector( center.x, radius + center.y, center.z );
		for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
		{
			edge.x = radius * cosf( angle / 180.0f * M_PI ) + center.x;
			edge.y = radius * sinf( angle / 180.0f * M_PI ) + center.y;
			edge.z = center.z;

			Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

			lastEdge = edge;
		}
	}
	void CircleOriented( const Vector &position, const QAngle &angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		matrix3x4_t xform;
		AngleMatrix(angles, position, xform);
		Vector xAxis, yAxis;
		MatrixGetColumn(xform, 2, xAxis);
		MatrixGetColumn(xform, 1, yAxis);
		Circle(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	void Circle( const Vector &position, const Vector &xAxis, const Vector &yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		const unsigned int nSegments = 16;
		const float flRadStep = (M_PI*2.0f) / (float) nSegments;

		Vector vecLastPosition;
		Vector vecStart = position + xAxis * radius;
		Vector vecPosition = vecStart;

		for ( int i = 1; i <= nSegments; i++ )
		{
			vecLastPosition = vecPosition;

			float flSin, flCos;
			SinCos( flRadStep*i, &flSin, &flCos );
			vecPosition = position + (xAxis * flCos * radius) + (yAxis * flSin * radius);

			Line( vecLastPosition, vecPosition, r, g, b, bNoDepthTest, flDuration );

			if ( a && i > 1 )
			{		
				debugoverlay->AddTriangleOverlay( vecStart, vecLastPosition, vecPosition, r, g, b, a, bNoDepthTest, flDuration );
			}
		}
	}
#ifndef CLIENT_DLL
	void SetDebugBits( HSCRIPT hEntity, int bit ) // DebugOverlayBits_t
	{
		CBaseEntity *pEnt = ToEnt(hEntity);
		if (!pEnt)
			return;

		if (pEnt->m_debugOverlays & bit)
		{
			pEnt->m_debugOverlays &= ~bit;
		}
		else
		{
			pEnt->m_debugOverlays |= bit;

#ifdef AI_MONITOR_FOR_OSCILLATION
			if (pEnt->IsNPC())
			{
				pEnt->MyNPCPointer()->m_ScheduleHistory.RemoveAll();
			}
#endif//AI_MONITOR_FOR_OSCILLATION
		}
	}
#endif
	void ClearAllOverlays()
	{
#ifndef CLIENT_DLL
		// Clear all entities of their debug overlays
		for (CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity; pEntity = gEntList.NextEnt(pEntity))
		{
			pEntity->m_debugOverlays = 0;
		}
#endif

		debugoverlay->ClearAllOverlays();
	}

private:
} g_ScriptDebugOverlay;

BEGIN_SCRIPTDESC_ROOT( CDebugOverlayScriptHelper, SCRIPT_SINGLETON "CDebugOverlayScriptHelper" )
	DEFINE_SCRIPTFUNC( Box, "Draws a world-space axis-aligned box. Specify bounds in world space." )
	DEFINE_SCRIPTFUNC( BoxDirection, "Draw box oriented to a Vector direction" )
	DEFINE_SCRIPTFUNC( BoxAngles, "Draws an oriented box at the origin. Specify bounds in local space." )
	DEFINE_SCRIPTFUNC( SweptBox, "Draws a swept box. Specify endpoints in world space and the bounds in local space." )
	DEFINE_SCRIPTFUNC( EntityBounds, "Draws bounds of an entity" )
	DEFINE_SCRIPTFUNC( Line, "Draws a line between two points" )
	DEFINE_SCRIPTFUNC( Triangle, "Draws a filled triangle. Specify vertices in world space." )
	DEFINE_SCRIPTFUNC( EntityText, "Draws text on an entity" )
	DEFINE_SCRIPTFUNC( EntityTextAtPosition, "Draw entity text overlay at a specific position" )
	DEFINE_SCRIPTFUNC( Grid, "Add grid overlay" )
	DEFINE_SCRIPTFUNC( Text, "Draws 2D text. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( ScreenText, "Draws 2D text. Specify coordinates in screen space." )
	DEFINE_SCRIPTFUNC( Cross3D, "Draws a world-aligned cross. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( Cross3DOriented, "Draws an oriented cross. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( DrawTickMarkedLine, "Draws a dashed line. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( HorzArrow, "Draws a horizontal arrow. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( YawArrow, "Draws a arrow associated with a specific yaw. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( VertArrow, "Draws a vertical arrow. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( Axis, "Draws an axis. Specify origin + orientation in world space." )
	DEFINE_SCRIPTFUNC( Sphere, "Draws a wireframe sphere. Specify center in world space." )
	DEFINE_SCRIPTFUNC( CircleOriented, "Draws a circle oriented. Specify center in world space." )
	DEFINE_SCRIPTFUNC( Circle, "Draws a circle. Specify center in world space." )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC( SetDebugBits, "Set debug bits on entity" )
#endif
	DEFINE_SCRIPTFUNC( ClearAllOverlays, "Clear all debug overlays at once" )
END_SCRIPTDESC();


//=============================================================================
// ConVars
//=============================================================================
#define VSCRIPT_CONVAR_ALLOWLIST_NAME "cfg/vscript_convar_allowlist.txt"

class CScriptConvarAccessor : public CAutoGameSystem
{
public:
	ScriptVariant_t GetBool( const char *cvar );
	ScriptVariant_t GetInt( const char *cvar );
	ScriptVariant_t GetFloat( const char *cvar );
	ScriptVariant_t GetStr( const char *cvar );
	const char *GetClientConvarValue( const char *cvar, int entindex );
	void SetValue( const char *cvar, ScriptVariant_t value );

	void LevelInitPreEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE;

	bool IsConVarOnAllowList( const char *cvar );

	CUtlSymbolTable m_AllowedConVars;
};
CScriptConvarAccessor g_ScriptConvars;

#define FCVAR_SCRIPT_NONO ( FCVAR_PROTECTED | FCVAR_SERVER_CANNOT_QUERY )

ScriptVariant_t CScriptConvarAccessor::GetBool( const char *cvar )
{
	if ( !cvar || !*cvar )
		return VARIANT_NULL;

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		return cref.GetBool();
	}
	else
	{
		return VARIANT_NULL;
	}
}

ScriptVariant_t CScriptConvarAccessor::GetInt( const char *cvar )
{
	if ( !cvar || !*cvar )
		return VARIANT_NULL;

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		return cref.GetInt();
	}
	else
	{
		return VARIANT_NULL;
	}
}

ScriptVariant_t CScriptConvarAccessor::GetFloat( const char *cvar )
{
	if ( !cvar || !*cvar )
		return VARIANT_NULL;

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		return cref.GetFloat();
	}
	else
	{
		return VARIANT_NULL;
	}
}

ScriptVariant_t CScriptConvarAccessor::GetStr( const char *cvar )
{
	if ( !cvar || !*cvar )
		return VARIANT_NULL;

	ConVarRef cref( cvar );
	if ( cref.IsValid() )
	{
		if ( cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
		{
			// the funny.
			return "hunter2";
		}
		return cref.GetString();
	}
	else
	{
		return VARIANT_NULL;
	}
}

const char *CScriptConvarAccessor::GetClientConvarValue( const char *cvar, int entindex )
{
	if ( !cvar || !*cvar )
		return "";
#ifndef CLIENT_DLL
	return engine->GetClientConVarValue( entindex, cvar );
#else
	return "";
#endif
}

void CScriptConvarAccessor::SetValue( const char *cvar, ScriptVariant_t value )
{
	if ( !cvar || !*cvar )
		return;

	if ( !IsConVarOnAllowList( cvar ) )
	{
		DevMsg( "Convar %s was not in " VSCRIPT_CONVAR_ALLOWLIST_NAME "\n", cvar );
		return;
	}

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		bool bSave = true;
		switch( value.GetType() )
		{
			case FIELD_BOOLEAN:
				cref.SetValue( (bool)value );
				break;
			case FIELD_INTEGER:
				cref.SetValue( (int)value );
				break;
			case FIELD_FLOAT:
				cref.SetValue( (float)value );
				break;
			case FIELD_CSTRING:
				cref.SetValue( (const char *)value );
				break;
			default:
				Warning( "%s.SetValue() unsupported value type %s\n", cvar, ScriptFieldTypeName( value.GetType() ) );
				bSave = false;
				break;
		}

		if ( bSave )
		{
			GameRules()->SaveConvar( cref );
		}
	}
}

void CScriptConvarAccessor::LevelInitPreEntity()
{
	m_AllowedConVars.RemoveAll();

	KeyValues *kv = new KeyValues( "vscript_convar_allowlist" );
	bool bLoaded = kv->LoadFromFile( g_pFullFileSystem, VSCRIPT_CONVAR_ALLOWLIST_NAME, "MOD" );
	if ( bLoaded )
	{
		for ( KeyValues *pCurItem = kv->GetFirstValue(); pCurItem; pCurItem = pCurItem->GetNextValue() )
		{
			const char *pName = pCurItem->GetName();
			const char *pValue = pCurItem->GetString();

			if ( !V_stricmp( pValue, "allowed" ) )
				m_AllowedConVars.AddString( pName );
		}
	}

	if ( !bLoaded )
		Warning( "Error loading " VSCRIPT_CONVAR_ALLOWLIST_NAME "\n" );
	kv->deleteThis();
}

void CScriptConvarAccessor::LevelShutdownPostEntity()
{
	m_AllowedConVars.RemoveAll();
}

bool CScriptConvarAccessor::IsConVarOnAllowList( const char *cvar )
{
	if ( !cvar || !*cvar )
		return false;

	return m_AllowedConVars.Find( cvar ) != UTL_INVAL_SYMBOL;
}

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvarAccessor, "Convars", SCRIPT_SINGLETON "Access to convar functions" )
	DEFINE_SCRIPTFUNC( GetBool, "GetBool(name) : returns the convar as a bool. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetInt, "GetInt(name) : returns the convar as an int. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetFloat, "GetFloat(name) : returns the convar as a float. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetStr, "GetStr(name) : returns the convar as a string. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetClientConvarValue, "GetClientConvarValue(name) : returns the convar value for the entindex as a string." )
	DEFINE_SCRIPTFUNC( SetValue, "SetValue(name, value) : sets the value of the convar. The convar must be in " VSCRIPT_CONVAR_ALLOWLIST_NAME " to be set. Supported types are bool, int, float, string." )
	DEFINE_SCRIPTFUNC( IsConVarOnAllowList, "IsConVarOnAllowList(name) : checks if the convar is allowed to be used and is in " VSCRIPT_CONVAR_ALLOWLIST_NAME ". Please be nice with this and use it for *compatibility* if you need check support and NOT to force server owners to allow hostname to be set... or else this will simply lie and return true in future. ;-) You have been warned!"  )
END_SCRIPTDESC()

//=============================================================================
// Effects
// (Unique to mapbase)
//
// At the moment only clientside until a filtering method on server is finalised.
//
// TEs most of the time call IEffects (g_pEffects) or ITempEnts (tempents) on client,
// but they also record for tools recording mode.
//
// On client no TE is suppressed.
// TE flags are found at tempent.h
//
// TODO:
//=============================================================================
#ifdef CLIENT_DLL

class CEffectsScriptHelper
{
private:
	C_RecipientFilter filter;

public:
	void DynamicLight( int index, const Vector& origin, int r, int g, int b, int exponent,
		float radius, float die, float decay, int style = 0, int flags = 0 )
	{
		//te->DynamicLight( filter, delay, &origin, r, g, b, exponent, radius, die, decay );
		dlight_t *dl = effects->CL_AllocDlight( index );
		dl->origin = origin;
		dl->color.r = r;
		dl->color.g = g;
		dl->color.b = b;
		dl->color.exponent = exponent;
		dl->radius = radius;
		dl->die = gpGlobals->curtime + die;
		dl->decay = decay;
		dl->style = style;
		dl->flags = flags;
	}

	void Explosion( const Vector& pos, float scale, int radius, int magnitude, int flags )
	{
		filter.AddAllPlayers();
		// framerate, modelindex, normal and materialtype are unused
		// radius for ragdolls
		extern short g_sModelIndexFireball;
		te->Explosion( filter, 0.0f, &pos, g_sModelIndexFireball, scale, 15, flags, radius, magnitude, &vec3_origin );
	}

//	void FXExplosion( const Vector& pos, const Vector& normal, int materialType = 'C' )
//	{
//		// just the particles
//		// materialtype only for debris. can be 'C','W' or anything else.
//		FX_Explosion( const_cast<Vector&>(pos), const_cast<Vector&>(normal), materialType );
//	}

//	void ConcussiveExplosion( const Vector& pos, const Vector& normal )
//	{
//		FX_ConcussiveExplosion( const_cast<Vector&>(pos), const_cast<Vector&>(normal) );
//	}

//	void MicroExplosion( const Vector& pos, const Vector& normal )
//	{
//		FX_MicroExplosion( const_cast<Vector&>(pos), const_cast<Vector&>(normal) );
//	}

//	void MuzzleFlash( int type, HSCRIPT hEntity, int attachment, bool firstPerson )
//	{
//		C_BaseEntity *p = ToEnt(hEntity);
//		ClientEntityHandle_t ent = p ? (ClientEntityList().EntIndexToHandle)( p->entindex() ) : NULL;;
//		tempents->MuzzleFlash( type, ent, attachment, firstPerson );
//	}

	void Sparks( const Vector& pos, int nMagnitude, int nTrailLength, const Vector& pDir )
	{
		//te->Sparks( filter, delay, &pos, nMagnitude, nTrailLength, &pDir );
		//g_pEffects->Sparks( pos, nMagnitude, nTrailLength, &pDir );
		FX_ElectricSpark( pos, nMagnitude, nTrailLength, &pDir );
	}

	void MetalSparks( const Vector& pos, const Vector& dir )
	{
		//g_pEffects->MetalSparks( pos, dir );
		FX_MetalSpark( pos, dir, dir );
	}

//	void Smoke( const Vector& pos, float scale, int framerate)
//	{
//		extern short g_sModelIndexSmoke;
//		//te->Smoke( filter, 0.0, &pos, g_sModelIndexSmoke, scale * 10.0f, framerate );
//		g_pEffects->Smoke( pos, g_sModelIndexSmoke, scale, framerate );
//	}

	void Dust( const Vector &pos, const Vector &dir, float size, float speed )
	{
		//te->Dust( filter, delay, pos, dir, size, speed );
		//g_pEffects->Dust( pos, dir, size, speed );
		FX_Dust( pos, dir, size, speed );
	}

	void Bubbles( const Vector &mins, const Vector &maxs, float height, int modelindex, int count, float speed )
	{
		//int bubbles = modelinfo->GetModelIndex( "sprites/bubble.vmt" );
		//te->Bubbles( filter, delay, &mins, &maxs, height, modelindex, count, speed );
		tempents->Bubbles( mins, maxs, height, modelindex, count, speed );
	}

//	void Fizz( const Vector& mins, const Vector& maxs, int modelIndex, int density, int current/*, int flags*/ )
//	{
//		//te->Fizz( filter, delay, ent, modelindex, density, current );
//		//tempents->FizzEffect( ToEnt(ent), modelindex, density, current );
//	}

	void Sprite( const Vector &pos, const Vector &dir, float scale, int modelIndex, int rendermode,
		int renderfx, int brightness, float life, int flags  )
	{
		//te->Sprite( filter, delay, &pos, modelindex, size, brightness );
		float a = (1.0 / 255.0) * brightness;
		tempents->TempSprite( pos, dir, scale, modelIndex, rendermode, renderfx, a, life, flags );
	}

//	void PhysicsProp( float delay, int modelindex, int skin, const Vector& pos, const QAngle &angles,
//		const Vector& vel, int flags, int effects )
//	{
//		//te->PhysicsProp( filter, delay, modelindex, skin, pos, angles, vel, flags, effects );
//		tempents->PhysicsProp( modelindex, skin, pos, angles, vel, flags, effects );
//	}

	void ClientProjectile( const Vector& vecOrigin, const Vector& vecVelocity, const Vector& vecAccel, int modelindex,
		int lifetime, HSCRIPT pOwner, const char *pszImpactEffect = NULL, const char *pszParticleEffect = NULL )
	{
		//te->ClientProjectile( filter, delay, &vecOrigin, &vecVelocity, modelindex, lifetime, ToEnt(pOwner) );
		if ( pszImpactEffect && !(*pszImpactEffect) )
			pszImpactEffect = NULL;
		if ( pszParticleEffect && !(*pszParticleEffect) )
			pszParticleEffect = NULL;
		tempents->ClientProjectile( vecOrigin, vecVelocity, vecAccel, modelindex, lifetime, ToEnt(pOwner), pszImpactEffect, pszParticleEffect );
	}

} g_ScriptEffectsHelper;

BEGIN_SCRIPTDESC_ROOT_NAMED( CEffectsScriptHelper, "CEffects", SCRIPT_SINGLETON "" )
	DEFINE_SCRIPTFUNC( DynamicLight, "" )
	DEFINE_SCRIPTFUNC( Explosion, "" )
	DEFINE_SCRIPTFUNC( Sparks, "" )
	DEFINE_SCRIPTFUNC( MetalSparks, "" )
	DEFINE_SCRIPTFUNC( Dust, "" )
	DEFINE_SCRIPTFUNC( Bubbles, "" )
	DEFINE_SCRIPTFUNC( Sprite, "" )
	DEFINE_SCRIPTFUNC( ClientProjectile, "" )
END_SCRIPTDESC();
#endif


void RegisterScriptSingletons()
{
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::SaveTable, "SaveTable", "Store a table with primitive values that will persist across level transitions and save loads." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::RestoreTable, "RestoreTable", "Retrieves a table from storage. Write into input table." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::ClearSavedTable, "ClearSavedTable", "Removes the table with the given context." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::FileWrite, "StringToFile", "Stores the string into the file" );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::FileRead, "FileToString", "Returns the string from the file, null if no file or file is too big." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::KeyValuesWrite, "KeyValuesToFile", "Stores the CScriptKeyValues into the file" );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::KeyValuesRead, "FileToKeyValues", "Returns the CScriptKeyValues from the file, null if no file or file is too big." );
	 
	ScriptRegisterFunction( g_pScriptVM, FireGameEvent, "Fire a game event." );
#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, FireGameEventLocal, "Fire a game event without broadcasting to the client." );
#endif

	g_pScriptVM->RegisterInstance( &g_ScriptLocalize, "Localize" );
	g_pScriptVM->RegisterInstance( &g_ScriptDebugOverlay, "debugoverlay" );
	g_pScriptVM->RegisterInstance( &g_ScriptConvars, "Convars" );
#ifdef CLIENT_DLL
	g_pScriptVM->RegisterInstance( &g_ScriptEffectsHelper, "effects" );
#endif

	// Singletons not unique to VScript (not declared or defined here)
	g_pScriptVM->RegisterInstance( GameRules(), "GameRules" );
	g_pScriptVM->RegisterInstance( GetAmmoDef(), "AmmoDef" );
}
