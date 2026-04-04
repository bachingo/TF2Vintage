//========= TF2VR, All rights reserved. ============//
//
// OVR Lip Sync — hooks ISteamUser::DecompressVoice to capture remote
// player voice PCM.  Uses per-frame VoiceStatus snapshots to correlate
// each DecompressVoice call with the correct player entity.
//
//=============================================================================//
#include "cbase.h"
#include "vr_lipsync.h"
#include "voice_status.h"
#include "cdll_int.h"

#ifdef _WIN32
#include <windows.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar vr_lipsync_enable( "vr_lipsync_enable", "1", FCVAR_ARCHIVE, "Enable OVR Lip Sync for voice chat" );
ConVar vr_lipsync_talking_timeout( "vr_lipsync_talking_timeout", "0.15", FCVAR_ARCHIVE, "Seconds to keep visemes after voice stops" );

// OVR viseme index -> Source Engine phoneme code (for VFE lookup)
const int g_OVRVisemeToSourcePhoneme[OVR_VISEME_COUNT] =
{
	'_',    // 0  sil
	'p',    // 1  PP
	'f',    // 2  FF
	0x03b8, // 3  TH
	'd',    // 4  DD
	'k',    // 5  kk
	0x02a7, // 6  CH
	's',    // 7  SS
	'n',    // 8  nn
	0x0279, // 9  RR
	0x0251, // 10 aa
	0x025b, // 11 E
	0x026a, // 12 ih
	'o',    // 13 oh
	'u',    // 14 ou
};

pfnDecompressVoice_t CVRLipSync::s_pfnOriginalDecompressVoice = NULL;

//-----------------------------------------------------------------------------
CVRLipSync &CVRLipSync::Instance()
{
	static CVRLipSync s_instance;
	return s_instance;
}

CVRLipSync::CVRLipSync()
	: m_bInitialized( false )
	, m_bHookInstalled( false )
	, m_nTalkingCount( 0 )
	, m_nDecompressCallIndex( 0 )
	, m_nEngineSampleRate( 0 )
{
	Q_memset( m_players, 0, sizeof( m_players ) );
	Q_memset( m_talkingSlots, 0, sizeof( m_talkingSlots ) );
}

CVRLipSync::~CVRLipSync()
{
	Shutdown();
}

//-----------------------------------------------------------------------------
bool CVRLipSync::Init()
{
	if ( m_bInitialized )
		return true;

	// Use 22050 as a reasonable default; per-context creation will use
	// the actual engine sample rate once we observe it in the hook.
	ovrLipSyncResult rc = ovrLipSync_Initialize( 22050, 1024 );
	if ( rc != ovrLipSyncSuccess )
	{
		Warning( "[VRLipSync] Failed to initialize OVRLipSync (error %d). "
				 "Make sure OVRLipSync.dll is next to the game executable.\n", rc );
		return false;
	}

	Q_memset( m_players, 0, sizeof( m_players ) );
	m_bInitialized = true;

	DevMsg( "[VRLipSync] Initialized\n" );

	InstallDecompressVoiceHook();
	return true;
}

//-----------------------------------------------------------------------------
void CVRLipSync::Shutdown()
{
	if ( !m_bInitialized )
		return;

	RemoveDecompressVoiceHook();

	for ( int i = 0; i < VR_LIPSYNC_MAX_PLAYERS; i++ )
		DestroyPlayerContext( i );

	ovrLipSync_Shutdown();
	m_bInitialized = false;
	DevMsg( "[VRLipSync] Shut down\n" );
}

//=============================================================================
// DecompressVoice vtable hook
//=============================================================================

bool CVRLipSync::InstallDecompressVoiceHook()
{
#ifdef _WIN32
	ISteamUser *pSteamUser = SteamUser();
	if ( !pSteamUser )
	{
		Warning( "[VRLipSync] SteamUser() unavailable — cannot install voice hook\n" );
		return false;
	}

	void **vtable = *reinterpret_cast<void ***>( pSteamUser );
	void *pOriginal = vtable[STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX];
	s_pfnOriginalDecompressVoice = reinterpret_cast<pfnDecompressVoice_t>( pOriginal );

	DWORD oldProtect;
	if ( !VirtualProtect(
			&vtable[STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX],
			sizeof( void * ), PAGE_READWRITE, &oldProtect ) )
	{
		Warning( "[VRLipSync] VirtualProtect failed — cannot install voice hook\n" );
		s_pfnOriginalDecompressVoice = NULL;
		return false;
	}

	vtable[STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX] =
		reinterpret_cast<void *>( &Hook_DecompressVoice );

	VirtualProtect(
		&vtable[STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX],
		sizeof( void * ), oldProtect, &oldProtect );

	m_bHookInstalled = true;
	DevMsg( "[VRLipSync] DecompressVoice hook installed\n" );
	return true;
#else
	return false;
#endif
}

void CVRLipSync::RemoveDecompressVoiceHook()
{
#ifdef _WIN32
	if ( !m_bHookInstalled || !s_pfnOriginalDecompressVoice )
		return;

	ISteamUser *pSteamUser = SteamUser();
	if ( !pSteamUser )
		return;

	void **vtable = *reinterpret_cast<void ***>( pSteamUser );

	DWORD oldProtect;
	if ( VirtualProtect(
			&vtable[STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX],
			sizeof( void * ), PAGE_READWRITE, &oldProtect ) )
	{
		vtable[STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX] =
			reinterpret_cast<void *>( s_pfnOriginalDecompressVoice );

		VirtualProtect(
			&vtable[STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX],
			sizeof( void * ), oldProtect, &oldProtect );
	}

	s_pfnOriginalDecompressVoice = NULL;
	m_bHookInstalled = false;
	DevMsg( "[VRLipSync] DecompressVoice hook removed\n" );
#endif
}

//-----------------------------------------------------------------------------
// Static hook — called by the engine in place of ISteamUser::DecompressVoice
//-----------------------------------------------------------------------------
#ifdef _WIN64
EVoiceResult CVRLipSync::Hook_DecompressVoice(
	ISteamUser *pThis,
	const void *pCompressed, uint32 cbCompressed,
	void *pDestBuffer, uint32 cbDestBufferSize,
	uint32 *nBytesWritten, uint32 nDesiredSampleRate )
{
	EVoiceResult result = s_pfnOriginalDecompressVoice(
		pThis, pCompressed, cbCompressed,
		pDestBuffer, cbDestBufferSize,
		nBytesWritten, nDesiredSampleRate );
#else
EVoiceResult __fastcall CVRLipSync::Hook_DecompressVoice(
	ISteamUser *pThis, void *edx,
	const void *pCompressed, uint32 cbCompressed,
	void *pDestBuffer, uint32 cbDestBufferSize,
	uint32 *nBytesWritten, uint32 nDesiredSampleRate )
{
	EVoiceResult result = s_pfnOriginalDecompressVoice(
		pThis, edx, pCompressed, cbCompressed,
		pDestBuffer, cbDestBufferSize,
		nBytesWritten, nDesiredSampleRate );
#endif

	CVRLipSync &ls = CVRLipSync::Instance();
	if ( !ls.m_bInitialized || !vr_lipsync_enable.GetBool() )
		return result;

	if ( result == k_EVoiceResultOK && nBytesWritten && *nBytesWritten > 0 )
	{
		if ( ls.m_nEngineSampleRate == 0 )
			ls.m_nEngineSampleRate = (int)nDesiredSampleRate;

		int entindex = ls.ResolveCurrentDecompressPlayer();

		if ( entindex > 0 && entindex < VR_LIPSYNC_MAX_PLAYERS )
		{
			int sampleCount = (int)( *nBytesWritten / sizeof( int16_t ) );
			ls.ProcessAudioForPlayer(
				entindex,
				reinterpret_cast<const int16_t *>( pDestBuffer ),
				sampleCount );
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
int CVRLipSync::ResolveCurrentDecompressPlayer()
{
	long idx = InterlockedIncrement( &m_nDecompressCallIndex ) - 1;

	if ( idx >= 0 && idx < m_nTalkingCount )
		return m_talkingSlots[idx];

	return -1;
}

//=============================================================================
// Per-player OVR context management
//=============================================================================

bool CVRLipSync::EnsurePlayerContext( int entindex )
{
	if ( entindex < 0 || entindex >= VR_LIPSYNC_MAX_PLAYERS )
		return false;

	PlayerLipSyncData &data = m_players[entindex];
	if ( data.bContextValid )
		return true;

	int sampleRate = m_nEngineSampleRate > 0 ? m_nEngineSampleRate : 22050;

	ovrLipSyncContext ctx = 0;
	ovrLipSyncResult rc = ovrLipSync_CreateContextEx(
		&ctx, ovrLipSyncContextProvider_Enhanced, sampleRate, true );

	if ( rc != ovrLipSyncSuccess )
	{
		Warning( "[VRLipSync] Failed to create context for player %d (error %d)\n", entindex, rc );
		return false;
	}

	data.context = ctx;
	data.bContextValid = true;
	Q_memset( data.visemes, 0, sizeof( data.visemes ) );

	data.frame.visemes = data.visemes;
	data.frame.visemesLength = OVR_VISEME_COUNT;
	data.frame.laughterCategories = NULL;
	data.frame.laughterCategoriesLength = 0;

	return true;
}

void CVRLipSync::DestroyPlayerContext( int entindex )
{
	if ( entindex < 0 || entindex >= VR_LIPSYNC_MAX_PLAYERS )
		return;

	PlayerLipSyncData &data = m_players[entindex];
	if ( data.bContextValid )
	{
		ovrLipSync_DestroyContext( data.context );
		data.bContextValid = false;
	}
	Q_memset( &data, 0, sizeof( data ) );
}

void CVRLipSync::ProcessAudioForPlayer( int entindex, const int16_t *pcm, int sampleCount )
{
	if ( !EnsurePlayerContext( entindex ) )
		return;

	PlayerLipSyncData &data = m_players[entindex];
	ovrLipSyncResult rc = ovrLipSync_ProcessFrameEx(
		data.context, pcm, sampleCount,
		ovrLipSyncAudioDataType_S16_Mono, &data.frame );

	if ( rc == ovrLipSyncSuccess )
	{
		data.flLastAudioTime = gpGlobals->curtime;
	}
}

//=============================================================================
// Public interface
//=============================================================================

bool CVRLipSync::GetVisemeWeights( int entindex, float *outWeights ) const
{
	if ( !m_bInitialized || !vr_lipsync_enable.GetBool() )
		return false;
	if ( entindex < 0 || entindex >= VR_LIPSYNC_MAX_PLAYERS )
		return false;

	const PlayerLipSyncData &data = m_players[entindex];
	if ( !data.bContextValid )
		return false;

	float timeSinceAudio = gpGlobals->curtime - data.flLastAudioTime;
	if ( timeSinceAudio > vr_lipsync_talking_timeout.GetFloat() )
		return false;

	Q_memcpy( outWeights, data.visemes, sizeof( float ) * OVR_VISEME_COUNT );
	return true;
}

void CVRLipSync::SetPlayerTalking( int entindex, bool bTalking )
{
	if ( entindex < 0 || entindex >= VR_LIPSYNC_MAX_PLAYERS )
		return;

	m_players[entindex].bTalking = bTalking;
}

bool CVRLipSync::IsPlayerTalking( int entindex ) const
{
	if ( entindex < 0 || entindex >= VR_LIPSYNC_MAX_PLAYERS )
		return false;

	return m_players[entindex].bTalking;
}

//=============================================================================
// Per-frame update — build the talking-player snapshot and reset call counter
//=============================================================================
void CVRLipSync::Update()
{
	if ( !m_bInitialized || !vr_lipsync_enable.GetBool() )
		return;

	// ---- Snapshot: which players are currently talking (sorted asc) ----
	m_nTalkingCount = 0;

	CVoiceStatus *pVoiceMgr = GetClientVoiceMgr();
	if ( pVoiceMgr )
	{
		for ( int i = 1; i <= gpGlobals->maxClients && m_nTalkingCount < VR_LIPSYNC_MAX_PLAYERS; i++ )
		{
			bool bSpeaking = pVoiceMgr->IsPlayerSpeaking( i );
			SetPlayerTalking( i, bSpeaking );

			if ( bSpeaking )
			{
				C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
				if ( pLocal && pLocal->entindex() == i )
					continue; // local player's voice isn't decompressed locally

				m_talkingSlots[m_nTalkingCount++] = i;
			}
		}
	}

	// Reset per-frame call counter (DecompressVoice calls that follow
	// will index into m_talkingSlots in order).
	InterlockedExchange( &m_nDecompressCallIndex, 0 );

	// Garbage-collect contexts for players who stopped talking
	for ( int i = 0; i < VR_LIPSYNC_MAX_PLAYERS; i++ )
	{
		PlayerLipSyncData &data = m_players[i];
		if ( data.bContextValid && !data.bTalking )
		{
			float timeSince = gpGlobals->curtime - data.flLastAudioTime;
			if ( timeSince > 5.0f )
			{
				DestroyPlayerContext( i );
			}
		}
	}
}
