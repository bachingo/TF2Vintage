//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: AutoRP.
//
//==============================================================================//
#include "cbase.h"
#include "tf_autorp.h"
#include "filesystem.h" 


CTFAutoRP *g_pAutoRP = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAutoRP::CTFAutoRP()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAutoRP::~CTFAutoRP()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutoRP::ParseDataFile( void )
{
	KeyValues *pKV = new KeyValues( "AutoRPFile" );
	if ( pKV->LoadFromFile( filesystem, "scripts/autorp.txt", "MOD" ) )
	{
		KeyValues *pPrepended = pKV->FindKey( "prepended_words" );
		if ( pPrepended )
		{
			for ( KeyValues *pSubData = pPrepended->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
			{
				if ( pSubData->GetName() )
				{
					m_hPrependedWords.AddToTail( s_Replacements.AddString( pSubData->GetName() ) );
				}
			}
		}

		KeyValues *pAppended = pKV->FindKey( "appended_words" );
		if ( pAppended )
		{
			for ( KeyValues *pSubData = pAppended->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
			{
				if ( pSubData->GetName() )
				{
					m_hAppendedWords.AddToTail( s_Replacements.AddString( pSubData->GetName() ) );
				}
			}
		}

		KeyValues *pReplacements = pKV->FindKey( "word_replacements" );
		if ( pReplacements )
		{
			for ( KeyValues *pWordReplacements = pReplacements->GetFirstSubKey(); pWordReplacements != NULL; pWordReplacements = pWordReplacements->GetNextKey() )
			{
				wordreplacement_t *word = new wordreplacement_t;

				for ( KeyValues *pSubData = pWordReplacements->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
				{
					if ( V_strcmp( pSubData->GetName(), "chance" ) == 0 )
					{
						word->chance = pSubData->GetInt();
					}					
					if ( V_strcmp( pSubData->GetName(), "prepend_count" ) == 0 )
					{
						word->prepend_count = pSubData->GetInt();
					}
					if ( V_strcmp( pSubData->GetName(), "prev" ) == 0 )
					{
						V_strcpy( word->prev, pSubData->GetName() );
					}
					if ( V_strcmp( pSubData->GetName(), "word" ) == 0 )
					{
						word->m_hWords.AddToTail( s_Words.AddString( pSubData->GetName() ) );
					}
					if ( V_strcmp( pSubData->GetName(), "word_plural" ) == 0 )
					{
						word->m_hWordsPlural.AddToTail( s_WordsPlural.AddString( pSubData->GetName() ) );
					}
					if ( V_strcmp( pSubData->GetName(), "replacement" ) == 0 )
					{
						word->m_hReplacements.AddToTail( s_Replacements.AddString( pSubData->GetName() ) );
					}
					if ( V_strcmp( pSubData->GetName(), "replacement_plural" ) == 0 )
					{
						word->m_hReplacementsPlural.AddToTail( s_Replacements.AddString( pSubData->GetName() ) );
					}
					if ( V_strcmp( pSubData->GetName(), "replacement_prepend" ) == 0 )
					{
						word->m_hReplacementPrepends.AddToTail( s_Replacements.AddString( pSubData->GetName() ) );
					}
				}
				m_hWordReplacements.AddToTail( word );
			}
		}
	}
	pKV->deleteThis();
}

void CTFAutoRP::ApplyRPTo( char *pBuf, int iBufSize ) 
{
	// None of this should be using strcpy

	// Ignore commands and text formatting
	/*if ( !pBuf[0] || pBuf[0] == '!' || pBuf[0] == '/' )
		return;

	if ( iBufSize > -2 )
		iBufSize = -1;

	char buf[128];
	bool bAllowPrepend = true;

	if ( pBuf[0] == '-' || pBuf[0] == '>' || pBuf[0] == '<' )
	{
		pBuf++;
		bAllowPrepend = false;
	}

	ModifySpeech( pBuf, iBufSize, bAllowPrepend );*/
}

void CTFAutoRP::ModifySpeech( char *pBuf, unsigned int iBufSize, bool bAllowPrepend )
{
	// TODO: All of this

	/*if ( bAllowPrepend )
	{
		// Only Prepend the sentence sometimes
		if ( RandomInt( 1, 4 ) == 1 )
		{
			int iWord = RandomInt( 0, m_hPrependedWords.Count() - 1 );
			char tmp[128];
			V_strcat( tmp, s_Replacements.String( m_hPrependedWords[iWord] ), 128 );

			if ( tmp[0] )
			{
				V_strcat( tmp, pBuf, 128 );
				V_strcpy( pBuf, tmp );
			}
		}

		// Only Append the sentence some other times
		if ( RandomInt( 1, 4 ) == 1 )
		{
			int iWord = RandomInt( 0, m_hAppendedWords.Count() - 1 );
			const char *iszAppended;
			iszAppended = s_Replacements.String( m_hAppendedWords[iWord] );

			if ( iszAppended[0] )
			{
				// Make sure there's a space between pBuf and the appended phrase
				V_strcat( pBuf, " ", iBufSize );
				V_strcat( pBuf, iszAppended, iBufSize );
			}
		}
	}
	Maybe do some sort of while loop checking for a-z/A-Z and grab that and then modify it and concatenate into new string*/
}