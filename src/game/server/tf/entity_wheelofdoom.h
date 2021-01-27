//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef ENTITY_WHEEL_OF_DOOM_H
#define ENTITY_WHEEL_OF_DOOM_H
#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"
#include "GameEventListener.h"

class CTFPlayer;
class CMerasmus;
class CMerasmusDancer;

class CWheelOfDoom : public CBaseAnimating, public CGameEventListener
{
	DECLARE_CLASS( CWheelOfDoom, CBaseAnimating );
public:

	DECLARE_DATADESC()

	CWheelOfDoom();
	virtual ~CWheelOfDoom();

	typedef CUtlVector<CTFPlayer *> EffectData_t;
	struct WOD_BaseEffect
	{
		WOD_BaseEffect()
		{
			flDuration = 0.0f;
			szBroadcastSound = NULL;
			fFlags = 0;
			szEffectName = NULL;
		}

		virtual void InitEffect( float flDuration );
		virtual void ActivateEffect( EffectData_t &data ) {}
		virtual void UpdateEffect( EffectData_t &data ) {}
		virtual void DeactivateEffect( EffectData_t &data ) {}

		int nType;
		float flDuration;
		char const *szBroadcastSound;
		int fFlags;
		char const *szEffectName; // for debugging
	};

	static void SpeakMagicConceptToAllPlayers( char const *szConcept );

	static void ApplyAttributeToAllPlayers( char const *szAtribName, float flValue );
	static void ApplyAttributeToPlayer( CTFPlayer *pTarget, char const *szAtribName, float flValue );
	static void RemoveAttributeFromAllPlayers( char const *szAtribName );
	static void RemoveAttributeFromPlayer( CTFPlayer *pTarget, char const *szAtribName );

	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual void	FireGameEvent( IGameEvent *event );

	void			PlaySound( char const *szSound );
	void			SetSkin( int skin );
	void			SetScale( float scale );

	void			StartSpin( void );
	void			SpinThink( void );
	void			IdleThink( void );

	bool			IsDoneBroadcastingEffectSound( void ) const;

	void			InputSpin( inputdata_t &data );
	void			InputClearAllEffects( inputdata_t &data );

	void			RegisterEffect( WOD_BaseEffect *pEffect, int flags=0 );
	WOD_BaseEffect *GetRandomEffectWithFlags( void ) const;

	float			CalcNextTickTime( void ) const;
	float			CalcSpinCompletion( void ) const;

	CON_COMMAND_MEMBER_F( CWheelOfDoom, "tf_debug_wheel_of_doom", DBG_ApplyEffectByName, "Apply <effect name> to yourself. For testing", FCVAR_DEVELOPMENTONLY );
	void			DBG_ApplyEffectByName( char const *szEffectName );

	struct EffectManager
	{
		int AddEffect( WOD_BaseEffect *pEffect, float flDuration );
		void ClearEffects( void );
		void ApplyAllEffectsToPlayer( CTFPlayer *pTarget );
		void Precache( void );
		bool UpdateAndClearExpiredEffects( void );

		CUtlVector<WOD_BaseEffect *> m_Effects;
	}
	m_EffectManager;

private:
	CUtlVectorAutoPurge<WOD_BaseEffect *> m_Effects;

	CUtlVector<CWheelOfDoom *> m_Wheels;

	WOD_BaseEffect *m_pActiveEffect;
	CHandle<CBaseAnimating> m_hSpiral;
	bool			m_bHasSpiral;
	float			m_flDuration;
	float			m_flNextThinkTick;
	float			m_flSpinAnnounce;
	float			m_flSpinDuration;
	float			m_flEffectEndTime;

	COutputEvent	m_EffectApplied;
	COutputEvent	m_EffectExpired;

	class WOD_Burn : public WOD_BaseEffect
	{
		void InitEffect( float flDuration ) OVERRIDE;
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_Pee : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
		void UpdateEffect( EffectData_t &data ) OVERRIDE;

		CUtlVector<EHANDLE> m_SpawnPoints;
		float m_flNextSpawn;
	};
	class WOD_Ghosts : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
		void DeactivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_Dance : public WOD_BaseEffect
	{
		void InitEffect( float flDuration ) OVERRIDE;
		void UpdateEffect( EffectData_t &data ) OVERRIDE;
		void DeactivateEffect( EffectData_t &data ) OVERRIDE;
		int GetNumOfTeamDancing( int iTeam );
		inline void SlamPosAndAngles( CTFPlayer *pTarget, Vector const &pos, QAngle const &ang );

		int m_iSpawnSide;
		CHandle<CMerasmusDancer> m_hDancer;
		float m_flNextDance;

		typedef struct
		{
			Vector pos;
			QAngle ang;
			CHandle<CTFPlayer> hPlayer;
		} Dancer_t;
		CUtlVector<Dancer_t> m_Dancers;

		typedef struct MerasmusCreateInfo
		{
			MerasmusCreateInfo(Vector const &pos, QAngle const &ang)
				: pos( pos ), dir( ang ) {}

			Vector pos;
			QAngle dir;
		} MerasmusCreateInfo_t;
		CUtlVector<MerasmusCreateInfo_t> m_CreateInfos;
	};
	class WOD_UberEffect : public WOD_BaseEffect
	{
		void InitEffect( float flDuration ) OVERRIDE;
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_CritsEffect : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_SuperJumpEffect : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
		void DeactivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_SuperSpeedEffect : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
		void DeactivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_LowGravityEffect : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
		void DeactivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_SmallHeadEffect : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
		void DeactivateEffect( EffectData_t &data ) OVERRIDE;
	};
	class WOD_BigHeadEffect : public WOD_BaseEffect
	{
		void ActivateEffect( EffectData_t &data ) OVERRIDE;
		void DeactivateEffect( EffectData_t &data ) OVERRIDE;
	};
};

#endif
