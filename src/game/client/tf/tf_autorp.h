//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: AutoRP remake.
//
//==============================================================================//
#ifndef TF_AUTORP
#define TF_AUTORP
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "c_tf_player.h"

struct wordreplacement_t
{
	int chance;
	int prepend_count;
	char prev[256];

	// These vectors should store the associated IDs of the
	// strings in the Symbol Table of the main class
	CUtlVector<CUtlSymbol> m_hWords;
	CUtlVector<CUtlSymbol> m_hWordsPlural;
	CUtlVector<CUtlSymbol> m_hReplacements;
	CUtlVector<CUtlSymbol> m_hReplacementsPlural;
	CUtlVector<CUtlSymbol> m_hReplacementPrepends;
};

struct replacementcheck_t
{
	// ???
};

class CTFAutoRP : CAutoGameSystem
{
public:
	
	CTFAutoRP();
	~CTFAutoRP();

	void ParseDataFile( void );
	void ApplyRPTo( char *pBuf, int iBufSize );
	void ModifySpeech( /*const char a1, */char *pBuf, unsigned int iBufSize, bool bAllowPrepend/*, bool b5*/ );

	bool WordMatches( wordreplacement_t *, replacementcheck_t * );
	bool ReplaceWord( replacementcheck_t *, char *, int, bool, bool );
	bool PerformReplacement( char const*, replacementcheck_t *, char *, int, char *, int );

private:
	
	// Word banks
	CUtlSymbolTable s_Words; // words we're looking to filter
	CUtlSymbolTable s_WordsPlural; // plural words we're looking to filter
	CUtlSymbolTable s_Replacements; // replacements for anything we filter

	CUtlVector<wordreplacement_t *> m_hWordReplacements;
	CUtlVector<CUtlSymbol> m_hPrependedWords;
	CUtlVector<CUtlSymbol> m_hAppendedWords;
};

extern CTFAutoRP *g_pAutoRP;

inline CTFAutoRP *AutoRP( void )
{
	if ( !g_pAutoRP )
	{
		g_pAutoRP = new CTFAutoRP;
		g_pAutoRP->ParseDataFile();
	}
	
	return g_pAutoRP;
}

#endif //TF_AUTORP