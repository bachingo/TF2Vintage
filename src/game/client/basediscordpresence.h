#ifndef BASEDISCORDPRESENCE_H
#define BASEDISCORDPRESENCE_H
#ifdef _WIN32
#pragma once
#endif

#include "irichpresenceclient.h"

// The maximum length for string fields in Discord's API
#define DISCORD_FIELD_MAXLEN 128

#ifndef POSIX
class CBaseDiscordPresence : public CAutoGameSystemPerFrame, public IRichPresenceClient
{
public:
	CBaseDiscordPresence();
	virtual ~CBaseDiscordPresence();

	virtual bool Init();
	virtual void Shutdown();
	virtual void Update( float frametime );

	virtual bool InitPresence( void )					{ return true; }
	virtual void ResetPresence( void )					{ }
	virtual void UpdatePresence( void )					{ }
	virtual void SetLevelName( char const *pMapName )	{ Q_strncpy( m_szMapName, pMapName, MAX_MAP_NAME ); }
	char const *GetLevelName( void ) const				{ return m_szMapName; }
	virtual char const *GetMatchSecret( void ) const	{ return NULL; }
	virtual char const *GetJoinSecret( void ) const		{ return NULL; }
	virtual char const *GetSpectateSecret( void ) const { return NULL; }
	virtual unsigned char const *GetEncryptionKey( void ) const	{ return (unsigned char *)""; }

private:
	char m_szMapName[ MAX_MAP_NAME ];
};

namespace discord { class Core; }
extern discord::Core *g_pDiscord;
#endif // !POSIX

#endif // BASEDISCORDPRESENCE_H