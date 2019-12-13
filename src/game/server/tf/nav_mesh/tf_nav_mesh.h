#ifndef __TF_NAV_MESH_H__
#define __TF_NAV_MESH_H__

#include "nav_mesh.h"
#include "nav_colors.h"
#include "tf_nav_area.h"

#include "func_respawnroom.h"
#include "team_control_point_master.h"
#include "trigger_area_capture.h"

class CBaseObject;

class CTFNavMesh : public CNavMesh
{
public:
	CTFNavMesh();
	virtual ~CTFNavMesh();

	virtual void FireGameEvent( IGameEvent *event ) override;
	virtual CNavArea *CreateArea( void ) const override;
	virtual void Update( void ) override;
	virtual bool IsAuthoritative( void ) const override;
	virtual unsigned int GetSubVersionNumber( void ) const override;
	virtual void SaveCustomData( CUtlBuffer& fileBuffer ) const override;
	virtual void LoadCustomData( CUtlBuffer& fileBuffer, unsigned int subVersion ) override;
	virtual void OnServerActivate( void ) override;
	virtual void OnRoundRestart( void ) override;
	virtual unsigned int GetGenerationTraceMask( void ) const override;
	virtual void PostCustomAnalysis( void ) override;
	virtual void BeginCustomAnalysis( bool bIncremental ) override;
	virtual void EndCustomAnalysis( void ) override;

	void CollectAmbushAreas( CUtlVector<CTFNavArea *> *areas, CTFNavArea *startArea, int teamNum, float fMaxDist = -1.0f, float fIncursionDiff = 0.0f ) const;
	//void CollectAreasWithinBombTravelRange( CUtlVector<CTFNavArea *> *areas, float f1, float f2 ) const;
	void CollectBuiltObjects( CUtlVector<CBaseObject *> *objects, int teamNum );
	void CollectSpawnRoomThresholdAreas( CUtlVector<CTFNavArea *> *areas, int teamNum ) const;
	bool IsSentryGunHere( CTFNavArea *area ) const;

	const CUtlVector<CTFNavArea *> &GetControlPointAreas( int iPointIndex ) const
	{
		Assert( iPointIndex >= 0 && iPointIndex < MAX_CONTROL_POINTS );
		return m_CPAreas[iPointIndex];
	}
	CTFNavArea *GetMainControlPointArea( int iPointIndex )
	{
		Assert( iPointIndex >= 0 && iPointIndex < MAX_CONTROL_POINTS );
		return m_CPArea[iPointIndex];
	}

	const CUtlVector<CTFNavArea *> &GetSpawnRoomAreasForTeam( int iTeamNum ) const
	{
		Assert( iTeamNum == TF_TEAM_RED || iTeamNum == TF_TEAM_BLUE );
		if (iTeamNum == TF_TEAM_RED)
			return m_spawnAreasTeam1;

		return m_spawnAreasTeam2;
	}
	const CUtlVector<CTFNavArea *> &GetSpawnRoomExitsForTeam( int iTeamNum ) const
	{
		Assert( iTeamNum == TF_TEAM_RED || iTeamNum == TF_TEAM_BLUE );
		if (iTeamNum == TF_TEAM_RED)
			return m_spawnExitsTeam1;
		
		return m_spawnExitsTeam2;
	}

private:
	void CollectAndMarkSpawnRoomExits( CTFNavArea *area, CUtlVector<CTFNavArea *> *areas );
	void CollectControlPointAreas( void );
	void ComputeBlockedAreas( void );
	//void ComputeBombTargetDistance(void);
	void ComputeIncursionDistances( void );
	void ComputeIncursionDistances( CTFNavArea *area, int teamNum );
	void ComputeInvasionAreas( void );
	//void ComputeLegalBombDropAreas(void);
	void DecorateMesh( void );
	void OnObjectChanged( void );
	void RecomputeInternalData( void );
	void RemoveAllMeshDecoration( void );
	void ResetMeshAttributes( bool bFullReset );
	void UpdateDebugDisplay( void ) const;
	void OnBlockedAreasChanged( void );

	CountdownTimer m_recomputeTimer;

	enum
	{
		CP_STATE_RESET = 1,
		CP_STATE_OWNERSHIP_CHANGED = 2,
		CP_STATE_AWAITING_CAPTURE = 3
	};
	int m_pointState;
	int m_pointChangedIdx;

	CUtlVector<CTFNavArea *> m_sentryAreas;

	CUtlVector<CTFNavArea *> m_CPAreas[MAX_CONTROL_POINTS];
	CTFNavArea *m_CPArea[MAX_CONTROL_POINTS];

	CUtlVector<CTFNavArea *> m_spawnAreasTeam1;
	CUtlVector<CTFNavArea *> m_spawnAreasTeam2;

	CUtlVector<CTFNavArea *> m_spawnExitsTeam1;
	CUtlVector<CTFNavArea *> m_spawnExitsTeam2;

	int m_lastNPCCount;
};

inline CTFNavMesh *TFNavMesh( void )
{
	return assert_cast<CTFNavMesh *>( TheNavMesh );
}

#endif