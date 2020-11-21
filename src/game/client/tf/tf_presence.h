//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
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
#include "basediscordpresence.h"
#include "hl2orange.spa.h"

//-----------------------------------------------------------------------------
// Purpose: TF implementation for setting user contexts and properties.
//-----------------------------------------------------------------------------
class CTFPresence : public CBasePresence, public CGameEventListener
{
	DECLARE_CLASS_GAMEROOT( CTFPresence, CBasePresence );
public:
	// IGameEventListener Interface
	virtual void	FireGameEvent( IGameEvent * event );

	// CBaseGameSystemPerFrame overrides
	virtual bool		Init( void );
	virtual void		LevelInitPreEntity( void );

	// IPresence interface
	virtual void		SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties );
	virtual uint		GetPresenceID( const char *pIDName );
	virtual const char *GetPropertyIdString( const uint id );
	virtual void		GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes );
	virtual void		UploadStats();

private:
	bool	m_bIsInCommentary;

#if defined( _X360 )
	XSESSION_VIEW_PROPERTIES		m_ViewProperties[3];
#endif

};


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

	bool				InitPresence( void );
	void				ResetPresence( void );
	void				UpdatePresence( void ) { UpdatePresence( false, false ); }
	char const*			GetMatchSecret( void ) const;
	char const*			GetJoinSecret( void ) const;
	char const*			GetSpectateSecret( void ) const;

private:
	void				UpdatePresence( bool bForce, bool bIsDead );
	char const*			GetEncryptionKey( void ) const { return "XwRJxjCc"; }

	char m_szHostName[ DISCORD_FIELD_MAXLEN ];
	char m_szServerInfo[ DISCORD_FIELD_MAXLEN ];
	char m_szSteamID[ DISCORD_FIELD_MAXLEN ];
	char m_szGameType[ DISCORD_FIELD_MAXLEN ];
	char m_szGameState[ DISCORD_FIELD_MAXLEN ];
	char m_szClassName[ DISCORD_FIELD_MAXLEN ];

	RealTimeCountdownTimer m_updateThrottle;
	long m_iCreationTimestamp;

	static discord::Activity m_Activity;
	static discord::User m_CurrentUser;

	static void OnReady();
	static void OnJoinedGame( char const *joinSecret );
	static void OnSpectateGame( char const *joinSecret );
	static void OnJoinRequested( discord::User const &joinRequester );
	static void OnLogMessage( discord::LogLevel logLevel, char const *pszMessage );
	static void OnActivityUpdate( discord::Result result );
};

#endif // TF_PRESENCE_H
