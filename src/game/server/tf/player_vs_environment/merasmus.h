//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_H
#define MERASMUS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_halloween_boss.h"
#include "GameEventListener.h"
#include "NextBotBehavior.h"
#include "Path/NextBotPath.h"
#include "merasmus_components.h"

class CTFPlayer;
class CMerasmus;
class CMonsterResource;
class CWheelOfDoom;
class CTFWeaponBaseMerasmusGrenade;

static char const *gs_pszDisguiseProps[] ={
	"models/props_halloween/pumpkin_02.mdl",
	"models/props_halloween/pumpkin_03.mdl",
	"models/egypt/palm_tree/palm_tree.mdl",
	"models/props_spytech/control_room_console01.mdl",
	"models/props_spytech/work_table001.mdl",
	"models/props_coalmines/boulder1.mdl",
	"models/props_coalmines/boulder2.mdl",
	"models/props_farm/concrete_block001.mdl",
	"models/props_farm/welding_machine01.mdl",
	"models/props_medieval/medieval_resupply.mdl",
	"models/props_medieval/target/target.mdl",
	"models/props_swamp/picnic_table.mdl",
	"models/props_manor/baby_grand_01.mdl",
	"models/props_manor/bookcase_132_02.mdl",
	"models/props_manor/chair_01.mdl",
	"models/props_manor/couch_01.mdl",
	"models/props_manor/grandfather_clock_01.mdl",
	"models/props_viaduct_event/coffin_simpl",
	"models/props_2fort/miningcrate001.mdl",
	"models/props_gameplay/resupply_locker.mdl",
	"models/props_2fort/oildrum.mdl",
	"models/props_lakeside/wood_crate_01.mdl",
	"models/props_well/hand_truck01.mdl",
	"models/props_vehicles/mining_car_metal.mdl",
	"models/props_2fort/tire002.mdl",
	"models/props_well/computer_cart01.mdl",
	"models/egypt/palm_tree/palm_tree.mdl"
};

class CMerasmusPathCost : public IPathCost
{
public:
	CMerasmusPathCost( CMerasmus *actor )
		: m_Actor( actor )
	{
	}

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const OVERRIDE;

private:
	CMerasmus *m_Actor;
};


class CMerasmus : public CHalloweenBaseBoss, public CGameEventListener
{
	DECLARE_CLASS( CMerasmus, CHalloweenBaseBoss )
public:
	DECLARE_INTENTION_INTERFACE( CMerasmus )

	CMerasmus();
	virtual ~CMerasmus();

	static CTFWeaponBaseMerasmusGrenade *CreateMerasmusGrenade( Vector const &vecOrigin, Vector const &vecVelocity, CBaseCombatCharacter *pOwner, float flModelScale );
	static bool							Zap( CBaseCombatCharacter *pCaster, char const *szAttachment, float fRadius, float fMinDamage, float fMaxDamage, int nMaxTargets, int iTargetTeam );

	DECLARE_SERVERCLASS();

	virtual void			Spawn( void );
	virtual void			Precache( void );
	virtual void			UpdateOnRemove( void );
	virtual int				OnTakeDamage_Alive( CTakeDamageInfo const &info );

	virtual void			FireGameEvent( IGameEvent *event ) OVERRIDE;

	virtual void			Update( void ) OVERRIDE;

	virtual IBody			*GetBodyInterface( void ) const OVERRIDE { return m_body; }
	virtual ILocomotion		*GetLocomotionInterface( void ) const OVERRIDE;

	void					PushPlayer( CTFPlayer *pTarget, float flStrength );

	void					PlayHighPrioritySound( char const *szSoundName );
	void					PlayLowPrioritySound( IRecipientFilter &filter, char const *szSoundName );

	void					TriggerLogicRelay( char const *szRelayName, bool bTeleportTo );

	bool					IsNextKilledPropMerasmus( void );
	void					RemoveAllFakeProps( void );

	bool					ShouldDisguise( void );
	bool					ShouldReveal( void );
	bool					ShouldLeave( void );

	void					OnDisguise( void );
	void					OnRevealed( bool bFound );
	void					OnBeginStun( void );
	void					OnEndStun( void );
	void					OnLeaveWhileInDisguise( void );

	void					AddStun( CTFPlayer *pStunner );
	bool					IsStunned( void ) const               { return !m_stunDuration.IsElapsed(); }
	int						GetNumberTimesStunned( void ) const   { return m_nStunCount; }
	void					ResetStunCount( void )                { m_nStunCount = 0; }

	void					AddFakeProp( CBaseEntity *pProp )     { m_hTrickProps.AddToTail( pProp ); }
	void					NotifyFound( CTFPlayer *pPlayer )     { m_hHideNSeekWinner = pPlayer; }

	virtual int				GetBossType( void ) const OVERRIDE    { return MERASMUS; }
	virtual int				GetLevel( void ) const OVERRIDE       { return m_level; }

	void					StartRespawnTimer( void );

	CNetworkVar( bool, m_bRevealed );
	CNetworkVar( bool, m_bDoingAOEAttack );
	CNetworkVar( bool, m_bStunned );


	CountdownTimer m_lifeTimeDuration;
	float m_flTimeLeftAlive;

	Vector m_vecHome;

	static int m_level;

private:
	void PrecacheMerasmus( void );

	CHandle<CMonsterResource> m_hBossResource;
	CHandle<CWheelOfDoom> m_hWheelOfFate;
	CHandle<CTFPlayer> m_hHideNSeekWinner;

	CUtlVector<EHANDLE> m_hTrickProps;
	int m_nHidingSpotIndex;

	int m_nDisguiseLastHealth;

	int m_nStunCount;
	CountdownTimer m_stunDuration;

	int m_iPlayerKillCombo;
	int m_iZapKillCombo;
	int m_iGrenadeKillCombo;

	CSoundPatch *m_pFloatSound;

	ILocomotion *m_pGroundLoco;
	ILocomotion *m_pFlyingLoco;
	IBody *m_body;
};

void BombHeadForTeam( int iTeam, float flDuration );
void RemoveAllBombHeadFromPlayers( void );

#endif