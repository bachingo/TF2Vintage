//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF implementation of the IPresence interface
//
//=============================================================================

#ifndef TF_PRESENCE_H
#define TF_PRESENCE_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "basepresence.h"
#include "hl2orange.spa.h"

#ifndef POSIX
#include "basediscordpresence.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: TF implementation for setting user contexts and properties.
//-----------------------------------------------------------------------------
class CTF_Presence : public CBasePresence, public CGameEventListener
{
public:
	// IGameEventListener Interface
	virtual void	FireGameEvent( IGameEvent * event );

	// CBaseGameSystemPerFrame overrides
	virtual bool		Init( void );
	virtual void		LevelInitPreEntity( void );

	// IPresence interface
	virtual void		SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties );
	virtual uint		GetPresenceID( const char *pIDName );
	virtual const char 	*GetPropertyIdString( const unsigned int id );
	virtual void		GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes );
	virtual void		UploadStats();

private:
	bool	m_bIsInCommentary;

#if defined( _X360 )
	XSESSION_VIEW_PROPERTIES		m_ViewProperties[3];
#endif

};

#ifndef POSIX
class CTFDiscordPresence : public CBaseDiscordPresence, public CGameEventListener
{
	DECLARE_CLASS_GAMEROOT( CTFDiscordPresence, CBaseDiscordPresence );
public:

	CTFDiscordPresence();
	virtual ~CTFDiscordPresence() {};

	virtual void FireGameEvent( IGameEvent *event );

	virtual bool		Init( void );
	virtual void		Shutdown( void );
	virtual void		LevelInitPostEntity( void );
	virtual void		LevelShutdownPreEntity( void );

	bool				InitPresence( void ) OVERRIDE;
	void				ResetPresence( void ) OVERRIDE;
	void				UpdatePresence( void ) OVERRIDE;
	char const*			GetMatchSecret( void ) const OVERRIDE;
	char const*			GetJoinSecret( void ) const OVERRIDE;
	char const*			GetSpectateSecret( void ) const OVERRIDE;

private:
	discord::User const&GetCurrentUser( void ) const { return m_CurrentUser; }
	void				SetCurrentUser( discord::User const &user ) { m_CurrentUser = user; }

	void				UpdatePresence( bool bIsDead );
	unsigned char const* GetEncryptionKey( void ) const OVERRIDE { return (unsigned char *)"XwRJxjCc"; }

	char m_szHostName[ DISCORD_FIELD_MAXLEN ];
	char m_szServerInfo[ DISCORD_FIELD_MAXLEN ];	// "ip:port"
	char m_szSteamID[ DISCORD_FIELD_MAXLEN ];
	char m_szTeamPref[ 16 ];						// "red", "blue", "" — updated on team change
	int  m_nSourceTVPort;							// read from server_spawn tvport field (default 27020)

	// Persistent buffers for secret getters — avoids returning dangling _alloca pointers.
	mutable char m_szMatchSecret[ DISCORD_FIELD_MAXLEN ];
	mutable char m_szJoinSecret[ DISCORD_FIELD_MAXLEN ];
	mutable char m_szSpectateSecret[ DISCORD_FIELD_MAXLEN ];

	long m_iCreationTimestamp;
	float m_flLastPlayerJoinTime;
	int m_nPlayerCount;

	// Builds the plaintext payload that gets ICE-encrypted into the join secret.
	// Format: "ip:port" or "ip:port/red" / "ip:port/blue"
	void BuildJoinPayload( char *pBuf, int nBufLen ) const;

	discord::Activity m_Activity;
	discord::User m_CurrentUser;

	static void OnReady();
	static void OnJoinedGame( char const *joinSecret );
	static void OnSpectateGame( char const *joinSecret );
	static void OnJoinRequested( discord::User const &joinRequester );
	static void OnJoinRequestReply( discord::Result result );
	static void OnLogMessage( discord::LogLevel logLevel, char const *pszMessage );
	static void OnActivityUpdate( discord::Result result );
};
#endif // POSIX

#endif // TF_PRESENCE_H