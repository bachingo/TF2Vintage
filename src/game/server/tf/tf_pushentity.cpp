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

	// ???
	bool RotationPushTFPlayer(CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, CPhysicsPushedEntities::RotatingPushMove_t const&, bool);

	// ???
	bool LinearPushTFPlayer(CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, bool);

	// ???
	bool RotationCheckPush(CPhysicsPushedEntities::PhysicsPushedInfo_t &);

	// ???
	bool LinearCheckPush(CPhysicsPushedEntities::PhysicsPushedInfo_t &);

	// ???
	void MovePlayer(CBaseEntity *pPlayer, CPhysicsPushedEntities::PhysicsPushedInfo_t &info, float flAmount, bool);
	
	// Check whether or not the bounding boxes of player and pusher intersect
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

void CTFPhysicsPushEntities::FinishRotPushedEntity(CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove)
{
	if (TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT)
	{
		if (!pPushedEntity->IsPlayer())
		{
			QAngle angles = pPushedEntity->GetAbsAngles();

			angles.y += rotPushMove.amove.y;
			pPushedEntity->SetAbsAngles(angles);
		}
	}
	else
	{
		CPhysicsPushedEntities::FinishRotPushedEntity(pPushedEntity, rotPushMove);
	}
}

bool CTFPhysicsPushEntities::RotationPushTFPlayer(CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, CPhysicsPushedEntities::RotatingPushMove_t const&, bool)
{
	return false;
}

bool CTFPhysicsPushEntities::LinearPushTFPlayer(CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, bool)
{
	return false;
}

bool CTFPhysicsPushEntities::RotationCheckPush(CPhysicsPushedEntities::PhysicsPushedInfo_t &)
{
	return false;
}

bool CTFPhysicsPushEntities::LinearCheckPush(CPhysicsPushedEntities::PhysicsPushedInfo_t &)
{
	return false;
}

void CTFPhysicsPushEntities::MovePlayer(CBaseEntity *, CPhysicsPushedEntities::PhysicsPushedInfo_t &, float, bool)
{

}

bool CTFPhysicsPushEntities::IsPlayerAABBIntersetingPusherOBB(CBaseEntity *pPlayer, CBaseEntity *pPusher)
{
	if ( !pPlayer )
		return false;

	if ( !pPlayer->IsPlayer() )
		return false;

	return IsOBBIntersectingOBB(
		pPlayer->CollisionProp()->GetCollisionOrigin(), pPlayer->CollisionProp()->GetCollisionAngles(), pPlayer->CollisionProp()->OBBMins(), pPlayer->CollisionProp()->OBBMaxs(),
		pPusher->CollisionProp()->GetCollisionOrigin(), pPusher->CollisionProp()->GetCollisionAngles(), pPusher->CollisionProp()->OBBMins(), pPusher->CollisionProp()->OBBMaxs(),
		0.f);
}
