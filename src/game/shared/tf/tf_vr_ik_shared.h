//========= TF2VR, All rights reserved. ============//
//
// Shared VR arm IK solver -- used by both client and server.
//
//=============================================================================//

#ifndef TF_VR_IK_SHARED_H
#define TF_VR_IK_SHARED_H
#pragma once

#include "cbase.h"

void VRIK_AppendChildren_R( CUtlVector< const mstudiobone_t * > *pChildBones, const studiohdr_t *pHdr, int nBone );

bool VRIK_SolveTwoBoneIK(
	const Vector &shoulderPos,
	const Vector &targetPos,
	float upperLen,
	float foreLen,
	const Vector &poleVector,
	Vector &outElbowPos );

void VRIK_RotateBoneToDirection(
	const matrix3x4_t &oldBoneMat,
	const Vector &oldDir,
	const Vector &newDir,
	const Vector &newPos,
	matrix3x4_t &outMat );

void VRIK_CascadeBoneDelta(
	CBaseAnimating *pAnim,
	const studiohdr_t *pHdr,
	matrix3x4_t *pBones,
	int parentBoneIdx,
	const matrix3x4_t &delta,
	const int *skipBones,
	int numSkip );

void VRIK_ApplyArmIK(
	CBaseAnimating *pAnim,
	const studiohdr_t *pHdr,
	matrix3x4_t *pBones,
	int iCollar,
	int iUpperArm,
	int iLowerArm,
	int iHand,
	float collarLen,
	float upperLen,
	float foreLen,
	const Vector &targetPos,
	const QAngle &targetAng,
	const Vector &poleVector,
	float shoulderRange,
	float wristTwistShare,
	bool bDebug );

#endif // TF_VR_IK_SHARED_H
