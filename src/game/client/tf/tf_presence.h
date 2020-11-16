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

namespace discord
{
	class Core;
	class Activity;
	class User;
	enum class LogLevel;
	enum class Result;
}
#define DISCORD_FIELD_MAXLEN 128

class CTFDiscordPresence : public CAutoGameSystemPerFrame, public CGameEventListener
{
	DECLARE_CLASS_GAMEROOT( CTFDiscordPresence, CAutoGameSystemPerFrame );
public:

	CTFDiscordPresence();
	virtual ~CTFDiscordPresence() {};

	virtual void FireGameEvent( IGameEvent *event );

	virtual bool		Init( void );
	virtual void		Shutdown( void );
	virtual void		Update( float frametime );
	virtual void		LevelInitPostEntity( void );
	virtual void		LevelShutdownPreEntity( void );

	void				ResetPresence( void );
	void				UpdatePresence( bool bForce = false, bool bIsDead = false );
	void				SetLevelName( char const *pMapName ) { Q_strncpy( m_szMapName, pMapName, MAX_MAP_NAME ); }

private:
	char m_szMapName[ MAX_MAP_NAME ];
	char m_szHostName[ DISCORD_FIELD_MAXLEN ];
	char m_szServerInfo[ DISCORD_FIELD_MAXLEN ];
	char m_szSteamID[ DISCORD_FIELD_MAXLEN ];
	char m_szGameType[ DISCORD_FIELD_MAXLEN ];
	char m_szGameState[ DISCORD_FIELD_MAXLEN ];
	char m_szClassName[ DISCORD_FIELD_MAXLEN ];

	RealTimeCountdownTimer m_updateThrottle;
	long m_iCreationTimestamp;

	static discord::Core *m_pCore;
	static discord::Activity m_Activity;
	static discord::User m_CurrentUser;

	static void OnReady();
	static void OnJoinedGame( char const *joinSecret );
	static void OnSpectateGame( char const *joinSecret );
	static void OnJoinRequested( discord::User const &joinRequester );
	static void OnLogMessage( discord::LogLevel logLevel, char const *pszMessage );
	static void OnActivityUpdate( discord::Result result );
};

extern CTFDiscordPresence *rpc;

#endif // TF_PRESENCE_H
