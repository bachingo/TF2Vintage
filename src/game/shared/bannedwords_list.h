//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2V plain-text word/phrase censor.
//          Reads one phrase per line from cfg/bannedlist.txt (MOD path).
//          Lines beginning with '//' are treated as comments.
//          Matched phrases are replaced with asterisks of the same length.
//
//=============================================================================//

#ifndef BANNEDWORDS_LIST_H
#define BANNEDWORDS_LIST_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "utlstring.h"

class CBannedWordList
{
public:
	CBannedWordList() : m_bInitialized( false ) {}
	~CBannedWordList() {}

	// Loads phrases from the given MOD-relative file path.
	// Returns true if the file existed and at least one phrase was loaded.
	bool InitFromFile( const char *pszFilename );

	bool BInitialized() const { return m_bInitialized; }

	// Censors banned phrases in-place, replacing each matched character with '*'.
	// Returns number of replacements made.
	int  CensorBannedWordsInplace( char *sz ) const;

private:
	bool			m_bInitialized;
	CUtlVector<CUtlString> m_phrases; // all lower-cased for case-insensitive matching
};

// Global singleton — server-side only (shared/ compilation unit included by server)
extern CBannedWordList g_BannedWordList;

#endif // BANNEDWORDS_LIST_H
