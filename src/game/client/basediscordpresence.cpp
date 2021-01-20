#include "cbase.h"
#include "tier1/utlstring.h"
#include "basediscordpresence.h"

#ifdef _WIN32
#include "discord.h"
discord::Core *g_pDiscord = NULL;
#endif

ConVar cl_discord_appid( "cl_discord_appid", "451227888230858752", FCVAR_DEVELOPMENTONLY | FCVAR_PROTECTED, "This is for your Client ID for Discord Applications and is unique per sourcemod." );
ConVar cl_discord_presence_enabled( "cl_discord_presence_enabled", "1", FCVAR_ARCHIVE | FCVAR_NOT_CONNECTED );

static CBaseDiscordPresence s_presence;
IRichPresenceClient *rpc = NULL;


CBaseDiscordPresence::CBaseDiscordPresence()
	: CAutoGameSystemPerFrame( "Discord RPC" )
{
	m_szMapName[0] = '\0';
}

CBaseDiscordPresence::~CBaseDiscordPresence()
{
#ifdef _WIN32
	if ( g_pDiscord )
		delete g_pDiscord;
#endif
}

bool CBaseDiscordPresence::Init()
{
	if ( rpc == NULL )
		rpc = this;

	if ( !cl_discord_presence_enabled.GetBool() )
		return true;

#ifdef _WIN32
	if( g_pDiscord == NULL )
	{
		auto result = discord::Core::Create( V_atoi64( cl_discord_appid.GetString() ), DiscordCreateFlags_Default, &g_pDiscord );
		if ( result != discord::Result::Ok )
			return true;
	}
#endif

	return InitPresence();
}

void CBaseDiscordPresence::Shutdown()
{
	ResetPresence();

#ifdef _WIN32
	if ( g_pDiscord )
		delete g_pDiscord;
#endif

	if ( rpc == this )
		rpc = NULL;
}

void CBaseDiscordPresence::Update( float frametime )
{
	UpdatePresence();

#ifdef _WIN32
	if ( gpGlobals->tickcount % 2 )
		g_pDiscord->RunCallbacks();
#endif
}
