//====== Copyright Valve Corporation, All rights reserved. =======//
//
// Purpose: Payload pushable physics ported from live TF2
//
//=============================================================================//

#ifndef TF_PUSHENTITY_H
#define TF_PUSHENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "pushentity.h"
#include "tf_gamerules.h"
#include "collisionutils.h"
#include "func_respawnroom.h"

class CTFPhysicsPushEntities : public CPhysicsPushedEntities
{

public:
	DECLARE_CLASS( CTFPhysicsPushEntities, CPhysicsPushedEntities );

protected:
	virtual bool	SpeculativelyCheckRotPush( const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot );
	virtual bool	SpeculativelyCheckLinearPush( const Vector &vecAbsPush );
	virtual void	FinishRotPushedEntity( CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove );

	bool			RotationPushTFPlayer( CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, CPhysicsPushedEntities::RotatingPushMove_t const&, bool );
	bool			LinearPushTFPlayer( CPhysicsPushedEntities::PhysicsPushedInfo_t &, Vector const&, bool );
	bool			RotationCheckPush( CPhysicsPushedEntities::PhysicsPushedInfo_t & );
	bool			LinearCheckPush( CPhysicsPushedEntities::PhysicsPushedInfo_t & );
	void			MovePlayer( CBaseEntity *pPlayer, CPhysicsPushedEntities::PhysicsPushedInfo_t &info, float flAmount, bool );
	bool			IsPlayerAABBIntersetingPusherOBB( CBaseEntity *pPlayer, CBaseEntity *pPusher );

	// Not sure if these names are any good
	float		m_flOffset;
	Vector		m_vecPush;
};
#endif  // TF_PUSHENTITY_H