//========= TF2VR, All rights reserved. ============//
//
// OVR Lip Sync integration for voice chat visemes.
// Hooks ISteamUser::DecompressVoice to intercept remote player voice PCM
// and correlates calls with VoiceStatus to identify speakers.
//
//=============================================================================//
#ifndef VR_LIPSYNC_H
#define VR_LIPSYNC_H
#ifdef _WIN32
#pragma once
#endif

#include "OVRLipSync.h"
#include "steam/isteamuser.h"
#include "steam/steam_api.h"

#define VR_LIPSYNC_MAX_PLAYERS     34     // MAX_PLAYERS + 1 + padding
#define OVR_VISEME_COUNT           15

// ISteamUser vtable index for DecompressVoice (SteamUser023)
#define STEAMUSER_DECOMPRESS_VOICE_VTABLE_IDX  11

extern const int g_OVRVisemeToSourcePhoneme[OVR_VISEME_COUNT];

// Function pointer type matching ISteamUser::DecompressVoice.
// On x64 MSVC there is a single calling convention; on x86 virtual
// member functions use __thiscall (this in ECX).
#ifdef _WIN64
typedef EVoiceResult (*pfnDecompressVoice_t)(
	ISteamUser *pThis,
	const void *pCompressed, uint32 cbCompressed,
	void *pDestBuffer, uint32 cbDestBufferSize,
	uint32 *nBytesWritten, uint32 nDesiredSampleRate );
#else
typedef EVoiceResult (__fastcall *pfnDecompressVoice_t)(
	ISteamUser *pThis, void *edx,
	const void *pCompressed, uint32 cbCompressed,
	void *pDestBuffer, uint32 cbDestBufferSize,
	uint32 *nBytesWritten, uint32 nDesiredSampleRate );
#endif

class CVRLipSync
{
public:
	static CVRLipSync &Instance();

	bool Init();
	void Shutdown();
	void Update();

	bool GetVisemeWeights( int entindex, float *outWeights ) const;
	void SetPlayerTalking( int entindex, bool bTalking );
	bool IsPlayerTalking( int entindex ) const;
	bool IsInitialized() const { return m_bInitialized; }

private:
	CVRLipSync();
	~CVRLipSync();
	CVRLipSync( const CVRLipSync & );
	CVRLipSync &operator=( const CVRLipSync & );

	// vtable hook
	bool InstallDecompressVoiceHook();
	void RemoveDecompressVoiceHook();

	// Static hook entry point — must match vtable calling convention
#ifdef _WIN64
	static EVoiceResult Hook_DecompressVoice(
		ISteamUser *pThis,
		const void *pCompressed, uint32 cbCompressed,
		void *pDestBuffer, uint32 cbDestBufferSize,
		uint32 *nBytesWritten, uint32 nDesiredSampleRate );
#else
	static EVoiceResult __fastcall Hook_DecompressVoice(
		ISteamUser *pThis, void *edx,
		const void *pCompressed, uint32 cbCompressed,
		void *pDestBuffer, uint32 cbDestBufferSize,
		uint32 *nBytesWritten, uint32 nDesiredSampleRate );
#endif

	// Per-player OVR context management
	bool EnsurePlayerContext( int entindex );
	void DestroyPlayerContext( int entindex );
	void ProcessAudioForPlayer( int entindex, const int16_t *pcm, int sampleCount );

	// Frame-order correlation: resolve which talking player the Nth
	// DecompressVoice call belongs to.
	int  ResolveCurrentDecompressPlayer();

	struct PlayerLipSyncData
	{
		ovrLipSyncContext context;
		ovrLipSyncFrame   frame;
		float             visemes[OVR_VISEME_COUNT];
		bool              bContextValid;
		bool              bTalking;
		float             flLastAudioTime;
	};

	bool m_bInitialized;
	bool m_bHookInstalled;

	// Saved original vtable entry
	static pfnDecompressVoice_t s_pfnOriginalDecompressVoice;

	// Per-frame talking-player snapshot (sorted ascending by entindex)
	int  m_talkingSlots[VR_LIPSYNC_MAX_PLAYERS];
	int  m_nTalkingCount;
	volatile long m_nDecompressCallIndex;

	// Voice sample rate observed from the engine
	int  m_nEngineSampleRate;

	PlayerLipSyncData m_players[VR_LIPSYNC_MAX_PLAYERS];
};

extern ConVar vr_lipsync_enable;

#endif // VR_LIPSYNC_H
