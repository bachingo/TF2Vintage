//====== Copyright Valve Corporation, All rights reserved. =======//
//
// Purpose: Payload pushable physics ported from live TF2
//
//=============================================================================//

#include "cbase.h"
#include "pushentity.h"
#include "tf_gamerules.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTFPhysicsPushEntities : CPhysicsPushedEntities
{
protected:
	// Speculatively checks to see if all entities in this list can be pushed
	virtual bool SpeculativelyCheckRotPush(const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot);

	// Speculatively checks to see if all entities in this list can be pushed
	virtual bool	SpeculativelyCheckLinearPush(const Vector &vecAbsPush);

	// Some fixup for objects pushed by rotating objects
	virtual void	FinishRotPushedEntity(CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove);

	// Called by SpeculativelyCheckRotPush 
	bool RotationPushTFPlayer(CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, CPhysicsPushedEntities::RotatingPushMove_t const&, bool);

	// Called by SpeculativelyCheckLinearPush 
	bool LinearPushTFPlayer(CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, bool);

	// Called by RotationPushTFPlayer
	bool RotationCheckPush(CPhysicsPushedEntities::PhysicsPushedInfo_t &);

	// Called by LinearPushTFPlayer
	bool LinearCheckPush(CPhysicsPushedEntities::PhysicsPushedInfo_t &);

	// Called by LinearPushTFPlayer and RotationPushTFPlayer
	void MovePlayer(CBaseEntity *pPlayer, CPhysicsPushedEntities::PhysicsPushedInfo_t &info, float flAmount, bool);
	
	// Check whether or not the bounding boxes of player and pusher intersect
	// Called by RotationCheckPush (twice?)
	bool IsPlayerAABBIntersetingPusherOBB(CBaseEntity *pPlayer, CBaseEntity *pPusher);
};

bool CTFPhysicsPushEntities::SpeculativelyCheckRotPush(const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot)
{
	if (TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT)
	{
		Vector vecAbsPush;
		m_nBlocker = -1;
		for (int i = m_rgMoved.Count(); --i >= 0;)
		{
			if (m_rgMoved[i].m_pEntity && m_rgMoved[i].m_pEntity->IsPlayer())
			{
				Vector vecTemp;
				this->RotationPushTFPlayer(m_rgMoved[i], vecTemp, rotPushMove, false);
			}
			else
			{
				ComputeRotationalPushDirection(m_rgMoved[i].m_pEntity, rotPushMove, &vecAbsPush, pRoot);
				if (!SpeculativelyCheckPush(m_rgMoved[i], vecAbsPush, true))
				{
					m_nBlocker = i;
					return false;
				}
			}
		}

		return true;
	}
	else
	{
		return CPhysicsPushedEntities::SpeculativelyCheckRotPush(rotPushMove, pRoot);
	}
}

bool CTFPhysicsPushEntities::SpeculativelyCheckLinearPush(const Vector &vecAbsPush)
{
	if (TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT)
	{
		m_nBlocker = -1;
		for (int i = m_rgMoved.Count(); --i >= 0;)
		{
			if (m_rgMoved[i].m_pEntity && m_rgMoved[i].m_pEntity->IsPlayer())
			{
				this->LinearPushTFPlayer(m_rgMoved[i], vecAbsPush, false);
			}
			else
			{
				if (!SpeculativelyCheckPush(m_rgMoved[i], vecAbsPush, false))
				{
					m_nBlocker = i;
					return false;
				}
			}
		}

		return true;
	}
	else
	{
		return CPhysicsPushedEntities::SpeculativelyCheckLinearPush(vecAbsPush);
	}
}

void CTFPhysicsPushEntities::FinishRotPushedEntity( CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove )
{
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		if ( !pPushedEntity->IsPlayer() )
		{
			QAngle angles = pPushedEntity->GetAbsAngles();

			angles.y += rotPushMove.amove.y;
			pPushedEntity->SetAbsAngles( angles );
		}
	}
	else
	{
		CPhysicsPushedEntities::FinishRotPushedEntity(pPushedEntity, rotPushMove);
	}
}

bool CTFPhysicsPushEntities::RotationPushTFPlayer( PhysicsPushedInfo_t &info , Vector const&, const RotatingPushMove_t &rotPushMove, bool)
{
	info.m_bPusherIsGround = false;

	CBaseEntity *pBlocker = info.m_pEntity;
	if ( pBlocker && pBlocker->IsPlayer() )
	{
		info.m_vecStartAbsOrigin = pBlocker->GetAbsOrigin();

		// Not really sure if this part is correct
		CBaseEntity *pRoot = pBlocker->GetRootMoveParent();
		Vector vecOrigin = pBlocker->CollisionProp()->GetCollisionOrigin(), vecRootOrigin = pRoot->CollisionProp()->GetCollisionOrigin();

		if ( pRoot && IsOBBIntersectingOBB(
			vecOrigin, pBlocker->CollisionProp()->GetCollisionAngles(), pBlocker->CollisionProp()->OBBMins(), pBlocker->CollisionProp()->OBBMaxs(),
			vecRootOrigin, pRoot->CollisionProp()->GetCollisionAngles(), pRoot->CollisionProp()->OBBMins(), pRoot->CollisionProp()->OBBMaxs(), 0.0f ) )
		{
			// This doesn't make much sense
			//bool bBlockerBlocked = pBlocker->IsPointSized(), bRootBlocked = pRoot->IsPointSized();

			vecOrigin = vecOrigin - vecRootOrigin;
			vecOrigin.x = vecOrigin.x - vecRootOrigin.x;
			vecOrigin.y = vecOrigin.y - vecRootOrigin.y;

			if ( pBlocker->GetGroundEntity() == pRoot )
			{
				// Not sure at all about this
				m_rootPusherStartLocalOrigin.Init( 0, 0, 1 );
				if ( !rotPushMove.amove[0] )
				{
					m_rootPusherStartLocalAngles[0] = 0;
				}
				else
				{
					//v31 = COERCE_DOUBLE(COERCE_UNSIGNED_INT64(v39.m128_f32[0] * tan((v30 * *(&loc_1D8D8F + 10489809)))) & xmmword_C097D0);
					//m_rootPusherStartLocalAngles[0] = v31 * 1.1
				}
			}
			else
			{
			}
			RotationCheckPush( info );
			return true;
		}
	}
	return false;
}

bool CTFPhysicsPushEntities::LinearPushTFPlayer( PhysicsPushedInfo_t &info , Vector const&, bool)
{
	info.m_bPusherIsGround = false;

	CBaseEntity *pBlocker = info.m_pEntity;
	if ( pBlocker && pBlocker->IsPlayer() )
	{
		info.m_vecStartAbsOrigin = pBlocker->GetAbsOrigin();
	}
	return false;
}

bool CTFPhysicsPushEntities::RotationCheckPush( PhysicsPushedInfo_t &info )
{
	return false;
}

bool CTFPhysicsPushEntities::LinearCheckPush( PhysicsPushedInfo_t &info )
{
	//CBaseEntity *pBlocker = info.m_pEntity;
	return false;
}

void CTFPhysicsPushEntities::MovePlayer(CBaseEntity *, PhysicsPushedInfo_t &info, float, bool)
{

}

bool CTFPhysicsPushEntities::IsPlayerAABBIntersetingPusherOBB(CBaseEntity *pPlayer, CBaseEntity *pPusher)
{
	if ( !pPlayer || !pPlayer->IsPlayer() )
		return false;

	return IsOBBIntersectingOBB(
		pPlayer->CollisionProp()->GetCollisionOrigin(), pPlayer->CollisionProp()->GetCollisionAngles(), pPlayer->CollisionProp()->OBBMins(), pPlayer->CollisionProp()->OBBMaxs(),
		pPusher->CollisionProp()->GetCollisionOrigin(), pPusher->CollisionProp()->GetCollisionAngles(), pPusher->CollisionProp()->OBBMins(), pPusher->CollisionProp()->OBBMaxs(),
		0.0f);
}
