#ifndef __TF_NAV_AREA_H__
#define __TF_NAV_AREA_H__

#include "nav_area.h"

enum TFNavAttributeType
{
	BLOCKED                     = 0x00000001,

	RED_SPAWN_ROOM              = 0x00000002,
	BLUE_SPAWN_ROOM             = 0x00000004,
	SPAWN_ROOM_EXIT             = 0x00000008,

	AMMO                        = 0x00000010,
	HEALTH                      = 0x00000020,

	CONTROL_POINT               = 0x00000040,

	BLUE_SENTRY                 = 0x00000080,
	RED_SENTRY                  = 0x00000100,

	/* bit  9: unused */
	/* bit 10: unused */

	BLUE_SETUP_GATE             = 0x00000800,
	RED_SETUP_GATE              = 0x00001000,

	BLOCKED_AFTER_POINT_CAPTURE = 0x00002000,
	BLOCKED_UNTIL_POINT_CAPTURE = 0x00004000,

	BLUE_ONE_WAY_DOOR           = 0x00008000,
	RED_ONE_WAY_DOOR            = 0x00010000,

	WITH_SECOND_POINT           = 0x00020000,
	WITH_THIRD_POINT            = 0x00040000,
	WITH_FOURTH_POINT           = 0x00080000,
	WITH_FIFTH_POINT            = 0x00100000,

	SNIPER_SPOT                 = 0x00200000,
	SENTRY_SPOT                 = 0x00400000,

	/* bit 23: unused */
	/* bit 24: unused */

	NO_SPAWNING                 = 0x02000000,
	RESCUE_CLOSET               = 0x04000000,
	BOMB_DROP                   = 0x08000000,
	DOOR_NEVER_BLOCKS           = 0x10000000,
	DOOR_ALWAYS_BLOCKS          = 0x20000000,
	UNBLOCKABLE                 = 0x40000000,

	/* bit 31: unused */
};

#define TF_ATTRIBUTE_RESET BLOCKED|RED_SPAWN_ROOM|BLUE_SPAWN_ROOM|SPAWN_ROOM_EXIT|AMMO|HEALTH|CONTROL_POINT|BLUE_SENTRY|RED_SENTRY

class CTFNavArea : public CNavArea
{
public:
	CTFNavArea();
	virtual ~CTFNavArea();

	virtual void OnServerActivate() override;
	virtual void OnRoundRestart() override;

	virtual void Save( CUtlBuffer& fileBuffer, unsigned int version ) const override;
	virtual NavErrorType Load( CUtlBuffer& fileBuffer, unsigned int version, unsigned int subVersion ) override;

	virtual void UpdateBlocked( bool force = false, int teamID = TEAM_ANY ) override;
	virtual bool IsBlocked( int teamID, bool ignoreNavBlockers = false ) const override;

	virtual void Draw() const override;

	virtual void CustomAnalysis( bool isIncremental = false ) override;

	//virtual bool IsPotentiallyVisibleToTeam( int team ) const override;

	void CollectNextIncursionAreas( int teamNum, CUtlVector<CTFNavArea *> *areas );
	void CollectPriorIncursionAreas( int teamNum, CUtlVector<CTFNavArea *> *areas );
	CTFNavArea *GetNextIncursionArea( int teamNum ) const;

	void ComputeInvasionAreaVectors();
	bool IsAwayFromInvasionAreas( int teamNum, float radius ) const;
	CUtlVector<CTFNavArea *> *GetInvasionAreasForTeam( int teamNum );

	//void AddPotentiallyVisibleActor( CBaseCombatCharacter *actor );

	float GetCombatIntensity() const;
	bool IsInCombat() const;
	void OnCombat();

	void ResetTFMarker();
	void MakeNewTFMarker();
	bool IsTFMarked() const;
	void TFMark();

	bool IsValidForWanderingPopulation() const;

	void SetIncursionDistance( int teamnum, float distance );
	float GetIncursionDistance( int teamnum ) const;

	void AddTFAttributes( int bits );
	int GetTFAttributes( void ) const;
	bool HasTFAttributes( int bits ) const;
	void RemoveTFAttributes( int bits );

	void SetBombTargetDistance( float distance );
	float GetBombTargetDistance( void ) const;

	static int m_masterTFMark;

private:
	float m_aIncursionDistances[4];
	CUtlVector<CTFNavArea *> m_InvasionAreas[4];

public:
	int m_TFSearchMarker;

private:
	int m_nAttributes;

	//CUtlVector< CHandle<CBaseCombatCharacter> > m_PVNPCs[4];

	float m_fCombatIntensity;
	IntervalTimer m_combatTimer;

	float m_flBombTargetDistance;

	int m_TFMarker;
};

inline CUtlVector<CTFNavArea *> *CTFNavArea::GetInvasionAreasForTeam( int teamNum )
{
	Assert( teamNum > -1 && teamNum < 4 );
	return &m_InvasionAreas[teamNum];
}

inline void CTFNavArea::SetIncursionDistance( int teamNum, float distance )
{
	Assert( teamNum > -1 && teamNum < 4 );
	if ( teamNum < 4 )
		m_aIncursionDistances[teamNum] = distance;
}

inline float CTFNavArea::GetIncursionDistance( int teamNum ) const
{
	Assert( teamNum > -1 && teamNum < 4 );
	if ( teamNum < 4 )
		return m_aIncursionDistances[teamNum];

	return -1.0f;
}

inline void CTFNavArea::AddTFAttributes( int bits )
{
	m_nAttributes |= bits;
}

inline int CTFNavArea::GetTFAttributes( void ) const
{
	return m_nAttributes;
}

inline bool CTFNavArea::HasTFAttributes( int bits ) const
{
	return ( m_nAttributes & bits ) != 0;
}

inline void CTFNavArea::RemoveTFAttributes( int bits )
{
	m_nAttributes &= ~bits;
}

inline void CTFNavArea::SetBombTargetDistance( float distance )
{
	m_flBombTargetDistance = distance;
}

inline float CTFNavArea::GetBombTargetDistance( void ) const
{
	return m_flBombTargetDistance;
}

#endif