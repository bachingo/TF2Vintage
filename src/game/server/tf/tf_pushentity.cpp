#include "cbase.h"
#include "tf_pushentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Hook our push logic
CTFPhysicsPushEntities s_TFPushedEntities;
CPhysicsPushedEntities *g_pPushedEntities = &s_TFPushedEntities; 

ConVar tf_debug_push( "tf_debug_push", "0", FCVAR_CHEAT );

//---------------------------------------------------------------------------------
// Purpose: Speculatively checks to see if all entities in this list can be pushed
//---------------------------------------------------------------------------------
bool CTFPhysicsPushEntities::SpeculativelyCheckRotPush( const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot )
{
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		Vector vecAbsPush;
		m_nBlocker = -1;
		for ( int i = m_rgMoved.Count(); --i >= 0; )
		{
			if ( m_rgMoved[i].m_pEntity && m_rgMoved[i].m_pEntity->IsPlayer() )
			{
				RotationPushTFPlayer( m_rgMoved[i], vecAbsPush, rotPushMove, false );
			}
			else
			{
				ComputeRotationalPushDirection(m_rgMoved[i].m_pEntity, rotPushMove, &vecAbsPush, pRoot );
				if ( !SpeculativelyCheckPush(m_rgMoved[i], vecAbsPush, true ) )
				{
					m_nBlocker = i;
					return false;
				}
			}
			// Zero out the vector after each iteration
			vecAbsPush.Zero();
		}
		return true;
	}
	else
	{
		return BaseClass::SpeculativelyCheckRotPush( rotPushMove, pRoot );
	}
}

//---------------------------------------------------------------------------------
// Purpose: Speculatively checks to see if all entities in this list can be pushed
//---------------------------------------------------------------------------------
bool CTFPhysicsPushEntities::SpeculativelyCheckLinearPush( const Vector &vecAbsPush )
{
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		m_nBlocker = -1;
		for ( int i = m_rgMoved.Count(); --i >= 0;)
		{
			if ( m_rgMoved[i].m_pEntity && m_rgMoved[i].m_pEntity->IsPlayer() )
			{
				LinearPushTFPlayer( m_rgMoved[i], vecAbsPush, false );
			}
			else
			{
				if ( !SpeculativelyCheckPush( m_rgMoved[i], vecAbsPush, false ) )
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
		return BaseClass::SpeculativelyCheckLinearPush( vecAbsPush );
	}
}

//---------------------------------------------------------------------------------
// Purpose: Some fixup for objects pushed by rotating objects
//---------------------------------------------------------------------------------
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

//---------------------------------------------------------------------------------
// Purpose: Prepares the necessary information for the push
//---------------------------------------------------------------------------------
bool CTFPhysicsPushEntities::RotationPushTFPlayer( PhysicsPushedInfo_t &info , const Vector &vecAbsPush, const RotatingPushMove_t &rotPushMove, bool)
{
	info.m_Trace.m_pEnt = 0;
	CBaseEntity *pBlocker = info.m_pEntity;

	if ( pBlocker && pBlocker->IsPlayer() && pBlocker->CollisionProp() )
	{
		info.m_vecStartAbsOrigin = pBlocker->GetAbsOrigin();
		CBaseEntity *pRoot = m_rgPusher[0].m_pEntity->GetRootMoveParent();

		if ( pRoot && pRoot->CollisionProp() && IsPlayerAABBIntersetingPusherOBB( pBlocker, pRoot ) )
		{
			float flBlockerRadius = pBlocker->CollisionProp()->BoundingRadius(), flRootRadius = pRoot->CollisionProp()->BoundingRadius();
			Vector vecResult = pBlocker->CollisionProp()->GetCollisionOrigin() - pRoot->CollisionProp()->GetCollisionOrigin();

			// Player is on top of the cart
			if ( pBlocker->GetGroundEntity() == pRoot )
			{
				m_vecPush = Vector( 0.0f, 0.0f, 1.0f );

				if ( !rotPushMove.amove.x )
				{
					m_flOffset = 0;
				}
				else
				{
					// Note: 0.017453292 approx. =  pi / 180
					m_flOffset = fabs( tan( rotPushMove.amove.x * 0.017453292 ) * flRootRadius ) * 1.1;
				}
			}
			else
			{ 
				m_vecPush = vecResult;
				VectorNormalizeFast( m_vecPush );

				flRootRadius = flRootRadius + flBlockerRadius;
				m_flOffset = fabs( flRootRadius - vecResult.Length() ) * 1.1;
			}
			return RotationCheckPush( info );
		}
	}
	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Prepares the necessary information for the push
//---------------------------------------------------------------------------------
bool CTFPhysicsPushEntities::LinearPushTFPlayer( PhysicsPushedInfo_t &info , const Vector &vecAbsPush, bool )
{
	info.m_Trace.m_pEnt = 0;
	CBaseEntity *pBlocker = info.m_pEntity;

	if ( pBlocker && pBlocker->IsPlayer() && pBlocker->CollisionProp() )
	{
		info.m_vecStartAbsOrigin = pBlocker->GetAbsOrigin();

		CBaseEntity *pRoot = m_rgPusher[0].m_pEntity->GetRootMoveParent();

		if ( pRoot && pRoot->CollisionProp() && IsPlayerAABBIntersetingPusherOBB( pBlocker, pRoot ) )
		{
			m_vecPush = vecAbsPush;
			m_flOffset = VectorNormalize( m_vecPush );

			// Player is not on top of the cart
			if ( pBlocker->GetGroundEntity() != pRoot )
			{
				m_vecPush.z = 0;
				VectorNormalizeFast( m_vecPush );
			}
			return LinearCheckPush( info );
		}
	}
	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Pushes the player
//---------------------------------------------------------------------------------
bool CTFPhysicsPushEntities::RotationCheckPush( PhysicsPushedInfo_t &info )
{
	CBaseEntity *pBlocker = info.m_pEntity, *pRoot = m_rgPusher[0].m_pEntity->GetRootMoveParent();
	if ( pBlocker && pRoot )
	{
		int *pPusherHandles = (int*)stackalloc( m_rgPusher.Count() * sizeof(int) );
		UnlinkPusherList( pPusherHandles );

		// if the player is blocking the train try nudging him around to fix accumulated error
		for ( int checkCount = 0; checkCount < 3; checkCount++ )
		{
			MovePlayer( pBlocker, info, 0.34999999, pRoot->GetMoveParent() ? true : false );
			if ( !IsPlayerAABBIntersetingPusherOBB ( pBlocker, pRoot ) )
			{
				break;
			}
		}

		RelinkPusherList( pPusherHandles );

		info.m_bPusherIsGround = false;
		if ( pBlocker->GetGroundEntity() && pBlocker->GetGroundEntity()->GetRootMoveParent() == m_rgPusher[0].m_pEntity )
		{
			info.m_bPusherIsGround = true;
		}

		if ( IsPlayerAABBIntersetingPusherOBB( pBlocker, pRoot ) )
		{
			 UnlinkPusherList( pPusherHandles );
			 MovePlayer( pBlocker, info, 1.0, pRoot->GetMoveParent() ? true : false );
			 RelinkPusherList( pPusherHandles );
		}

		info.m_bBlocked = false;
	}
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Pushes the player
//---------------------------------------------------------------------------------
bool CTFPhysicsPushEntities::LinearCheckPush( PhysicsPushedInfo_t &info )
{
	CBaseEntity *pBlocker = info.m_pEntity, *pRoot = m_rgPusher[0].m_pEntity->GetRootMoveParent();
	if ( pBlocker && pRoot )
	{
		int *pPusherHandles = (int*)stackalloc( m_rgPusher.Count() * sizeof(int) );

		// Nudge the player
		UnlinkPusherList( pPusherHandles );
		MovePlayer( pBlocker, info, 1.0f, pRoot->GetMoveParent() ? true : false  );
		RelinkPusherList( pPusherHandles );

		info.m_bPusherIsGround = false;
		if ( pBlocker->GetGroundEntity() && pBlocker->GetGroundEntity()->GetRootMoveParent() == m_rgPusher[0].m_pEntity )
		{
			info.m_bPusherIsGround = true;
		}

		if ( !info.m_bPusherIsGround && !IsPushedPositionValid( pBlocker ) )
		{
			// Nudge the player some more
			UnlinkPusherList( pPusherHandles );
			MovePlayer( pBlocker, info, 1.0f, pRoot->GetMoveParent() ? true : false  );
			RelinkPusherList( pPusherHandles );
		}

		info.m_bBlocked = false;
	}
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Performs the push itself
//---------------------------------------------------------------------------------
void CTFPhysicsPushEntities::MovePlayer(CBaseEntity *pPlayer, PhysicsPushedInfo_t &info, float flOffset, bool bRootMoveParent )
{
	// We need to do this so we can kill the player if necessary
	CBasePlayer *pBasePlayer = dynamic_cast< CBasePlayer * >( pPlayer );
	if ( !pBasePlayer )
		return;

	float flTotalOffset = flOffset * m_flOffset;
	Vector vecAbsOrigin = pPlayer->GetAbsOrigin();

	for ( int i = 0; i < 4; i++ )
	{
		Vector vecEndPos = m_vecPush * flTotalOffset + vecAbsOrigin;
		vecAbsOrigin.z += 4;
		UTIL_TraceEntity( pPlayer, vecAbsOrigin, vecEndPos, MASK_PLAYERSOLID, 0, COLLISION_GROUP_PLAYER_MOVEMENT, &info.m_Trace );

		if ( tf_debug_push.GetBool() )
		{
			NDebugOverlay::Line( vecAbsOrigin, info.m_Trace.endpos, 0, 255, 0, true, 0.25f );
		}

		if ( bRootMoveParent )
		{
			if ( PointsCrossRespawnRoomVisualizer( vecAbsOrigin, info.m_Trace.endpos, pBasePlayer->GetTeamNumber() ) )
			{
				break;
			}
		}
		float flTraceFraction = info.m_Trace.fraction;
		Vector vecPlaneNormal = info.m_Trace.plane.normal;

		if ( flTraceFraction > 0.0 )
		{
			pPlayer->SetAbsOrigin( info.m_Trace.endpos );
		}

		if ( flTraceFraction != 1.0 && info.m_Trace.m_pEnt )
		{
			vec_t vDot = DotProductAbs( m_vecPush, vecPlaneNormal );

			flOffset = ( 2.0 - vDot ) * ( 1.0 - flTraceFraction ) * flOffset;

			m_vecPush = m_vecPush - ( vDot * vecPlaneNormal );
			vDot = DotProduct( m_vecPush, vecPlaneNormal );
			if ( vDot < 0.0 )
			{
				m_vecPush -= vecPlaneNormal * vDot;
			}
		}
		else
		{
			// Nothing else to do
			//return;
			break;
		}
	}
	// Enabling this causes players to suicide when stuck in walls
	//pBasePlayer->CommitSuicide( true, true );
}

//---------------------------------------------------------------------------------
// Purpose: Check whether or not the bounding boxes of player and pusher intersect
//---------------------------------------------------------------------------------
bool CTFPhysicsPushEntities::IsPlayerAABBIntersetingPusherOBB( CBaseEntity *pPlayer, CBaseEntity *pPusher )
{
	if ( !pPlayer || !pPlayer->IsPlayer() )
		return false;

	return IsOBBIntersectingOBB(
		pPlayer->CollisionProp()->GetCollisionOrigin(), pPlayer->CollisionProp()->GetCollisionAngles(), pPlayer->CollisionProp()->OBBMins(), pPlayer->CollisionProp()->OBBMaxs(),
		pPusher->CollisionProp()->GetCollisionOrigin(), pPusher->CollisionProp()->GetCollisionAngles(), pPusher->CollisionProp()->OBBMins(), pPusher->CollisionProp()->OBBMaxs(),
		0.0f );
}
