//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2V plain-text word/phrase censor.
//          See bannedwords_list.h for format documentation.
//
//=============================================================================//

#include "cbase.h"
#include "bannedwords_list.h"
#include "filesystem.h"
#include "utlbuffer.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

CBannedWordList g_BannedWordList;

bool CBannedWordList::InitFromFile( const char *pszFilename )
{
	m_phrases.RemoveAll();
	m_bInitialized = false;

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !g_pFullFileSystem->ReadFile( pszFilename, "MOD", buf ) )
	{
		// File absent — not an error; server just won't censor anything.
		DevMsg( "[BannedWordList] %s not found — chat censor disabled.\n", pszFilename );
		return false;
	}

	char szLine[256];
	while ( buf.IsValid() )
	{
		buf.GetLine( szLine, sizeof( szLine ) );
		if ( !buf.IsValid() && szLine[0] == '\0' )
			break;

		// Strip trailing CR/LF
		int nLen = Q_strlen( szLine );
		while ( nLen > 0 && ( szLine[nLen-1] == '\r' || szLine[nLen-1] == '\n' ) )
			szLine[--nLen] = '\0';

		// Skip empty lines and comments
		if ( nLen == 0 || ( szLine[0] == '/' && szLine[1] == '/' ) )
			continue;

		// Lower-case the phrase for case-insensitive matching
		Q_strlower( szLine );

		CUtlString s( szLine );
		m_phrases.AddToTail( s );
	}

	m_bInitialized = ( m_phrases.Count() > 0 );
	DevMsg( "[BannedWordList] Loaded %d phrase(s) from %s.\n", m_phrases.Count(), pszFilename );
	return m_bInitialized;
}

int CBannedWordList::CensorBannedWordsInplace( char *sz ) const
{
	if ( !m_bInitialized || !sz || !*sz )
		return 0;

	int nLen = Q_strlen( sz );

	// Work on a lower-cased copy so matching is case-insensitive but the
	// replacement asterisks land in the *original* buffer at correct offsets.
	char *pLower = (char *)stackalloc( nLen + 1 );
	Q_strncpy( pLower, sz, nLen + 1 );
	Q_strlower( pLower );

	int nReplacements = 0;

	FOR_EACH_VEC( m_phrases, i )
	{
		const char *pPhrase = m_phrases[i].Get();
		int nPhraseLen = Q_strlen( pPhrase );
		if ( nPhraseLen == 0 || nPhraseLen > nLen )
			continue;

		// Slide a window over the input looking for this phrase.
		const char *pSearch = pLower;
		while ( ( pSearch = Q_strstr( pSearch, pPhrase ) ) != nullptr )
		{
			// Replace matching characters in the original buffer with '*'
			int nOffset = (int)( pSearch - pLower );
			for ( int j = 0; j < nPhraseLen; ++j )
				sz[nOffset + j] = '*';

			nReplacements++;
			pSearch += nPhraseLen;
		}
	}

	return nReplacements;
}
