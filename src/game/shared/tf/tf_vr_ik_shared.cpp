//========= TF2VR, All rights reserved. ============//
//
// Shared VR arm IK solver -- used by both client and server
// for VR player arm positioning.
//
//=============================================================================//

#include "cbase.h"
#include "tf_vr_ik_shared.h"

#ifdef CLIENT_DLL
#include "engine/ivdebugoverlay.h"
#endif

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Recursively collect all child bones of the given bone index.
//-----------------------------------------------------------------------------
void VRIK_AppendChildren_R( CUtlVector< const mstudiobone_t * > *pChildBones, const studiohdr_t *pHdr, int nBone )
{
	if ( !pChildBones || !pHdr )
		return;

	for ( int i = nBone + 1; i < pHdr->numbones; ++i )
	{
		const mstudiobone_t *pBone = pHdr->pBone( i );
		if ( pBone->parent == nBone )
		{
			pChildBones->AddToTail( pBone );
			VRIK_AppendChildren_R( pChildBones, pHdr, i );
		}
	}
}

//-----------------------------------------------------------------------------
// 2-bone IK solver: finds elbow position given shoulder, hand target,
// and arm segment lengths. Returns false if inputs are degenerate.
//-----------------------------------------------------------------------------
bool VRIK_SolveTwoBoneIK(
	const Vector &shoulderPos,
	const Vector &targetPos,
	float upperLen,
	float foreLen,
	const Vector &poleVector,
	Vector &outElbowPos )
{
	Vector toTarget = targetPos - shoulderPos;
	float dist = toTarget.Length();

	if ( dist < 0.001f )
		return false;

	float minDist = fabsf( upperLen - foreLen ) + 0.1f;
	float maxDist = upperLen + foreLen - 0.1f;
	dist = clamp( dist, minDist, maxDist );

	float cosA = ( upperLen * upperLen + dist * dist - foreLen * foreLen ) / ( 2.0f * upperLen * dist );
	cosA = clamp( cosA, -1.0f, 1.0f );
	float sinA = sqrtf( 1.0f - cosA * cosA );

	Vector dir = toTarget;
	dir.NormalizeInPlace();

	float projLen = upperLen * cosA;
	float elbowHeight = upperLen * sinA;

	Vector elbowCenter = shoulderPos + dir * projLen;

	Vector toPole = poleVector - shoulderPos;
	Vector perpDir = toPole - dir * DotProduct( toPole, dir );
	float perpLen = perpDir.Length();
	if ( perpLen < 0.001f )
	{
		Vector arbitrary = ( fabsf( dir.z ) < 0.9f ) ? Vector( 0, 0, 1 ) : Vector( 1, 0, 0 );
		perpDir = CrossProduct( dir, arbitrary );
		perpLen = perpDir.Length();
	}
	perpDir /= perpLen;

	outElbowPos = elbowCenter + perpDir * elbowHeight;
	return true;
}

//-----------------------------------------------------------------------------
// Rotate a bone matrix so its primary axis points in a new direction,
// preserving the original twist/roll as much as possible.
//-----------------------------------------------------------------------------
void VRIK_RotateBoneToDirection(
	const matrix3x4_t &oldBoneMat,
	const Vector &oldDir,
	const Vector &newDir,
	const Vector &newPos,
	matrix3x4_t &outMat )
{
	Vector axis = CrossProduct( oldDir, newDir );
	float axisLen = axis.Length();
	float dot = DotProduct( oldDir, newDir );

	Quaternion rotQ;
	if ( axisLen < 0.0001f )
	{
		if ( dot > 0.0f )
		{
			MatrixCopy( oldBoneMat, outMat );
			MatrixSetTranslation( newPos, outMat );
			return;
		}
		Vector perp = ( fabsf( oldDir.z ) < 0.9f ) ? Vector( 0, 0, 1 ) : Vector( 1, 0, 0 );
		axis = CrossProduct( oldDir, perp );
		axis.NormalizeInPlace();
		AxisAngleQuaternion( axis, 180.0f, rotQ );
	}
	else
	{
		axis /= axisLen;
		float angle = RAD2DEG( atan2f( axisLen, dot ) );
		AxisAngleQuaternion( axis, angle, rotQ );
	}

	matrix3x4_t rotMat;
	QuaternionMatrix( rotQ, rotMat );

	matrix3x4_t oldNoTranslate;
	MatrixCopy( oldBoneMat, oldNoTranslate );
	MatrixSetTranslation( vec3_origin, oldNoTranslate );

	matrix3x4_t rotated;
	ConcatTransforms( rotMat, oldNoTranslate, rotated );
	MatrixSetTranslation( newPos, rotated );

	MatrixCopy( rotated, outMat );
}

//-----------------------------------------------------------------------------
// Apply a delta transform to all descendant bones of a given bone,
// skipping bones in the skipSet.
// Operates on the raw pBones buffer, using pAnim only for LookupBone.
//-----------------------------------------------------------------------------
void VRIK_CascadeBoneDelta(
	CBaseAnimating *pAnim,
	const studiohdr_t *pHdr,
	matrix3x4_t *pBones,
	int parentBoneIdx,
	const matrix3x4_t &delta,
	const int *skipBones,
	int numSkip )
{
	CUtlVector< const mstudiobone_t * > children;
	VRIK_AppendChildren_R( &children, pHdr, parentBoneIdx );

	for ( int i = 0; i < children.Count(); i++ )
	{
		int childIdx = pAnim->LookupBone( children[i]->pszName() );
		if ( childIdx == -1 )
			continue;

		bool bSkip = false;
		for ( int s = 0; s < numSkip; s++ )
		{
			if ( childIdx == skipBones[s] )
			{
				bSkip = true;
				break;
			}
		}
		if ( bSkip )
			continue;

		matrix3x4_t newChild;
		ConcatTransforms( delta, pBones[childIdx], newChild );
		MatrixCopy( newChild, pBones[childIdx] );
	}
}

//-----------------------------------------------------------------------------
// Full arm IK: collar shoulder movement, two-bone solve, wrist twist
// distribution, and hand orientation. Works on a raw bone buffer.
//-----------------------------------------------------------------------------
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
	bool bDebug )
{
	if ( iUpperArm == -1 || iLowerArm == -1 || iHand == -1 )
		return;

	// --- Collar/clavicle shoulder movement ---
	if ( iCollar != -1 && collarLen > 0.1f && shoulderRange > 0.0f )
	{
		Vector collarPos;
		MatrixPosition( pBones[iCollar], collarPos );
		Vector animShoulderPos;
		MatrixPosition( pBones[iUpperArm], animShoulderPos );

		Vector animCollarDir = ( animShoulderPos - collarPos );
		animCollarDir.NormalizeInPlace();

		Vector reachDir = ( targetPos - collarPos );
		reachDir.NormalizeInPlace();

		float cosAngle = clamp( DotProduct( animCollarDir, reachDir ), -1.0f, 1.0f );
		float angleDeg = RAD2DEG( acosf( cosAngle ) );

		if ( angleDeg > 1.0f )
		{
			float clampedAngle = MIN( angleDeg, shoulderRange );
			float frac = clampedAngle / angleDeg;

			Vector blendedDir = animCollarDir * ( 1.0f - frac ) + reachDir * frac;
			blendedDir.NormalizeInPlace();

			matrix3x4_t oldCollar;
			MatrixCopy( pBones[iCollar], oldCollar );

			VRIK_RotateBoneToDirection( oldCollar, animCollarDir, blendedDir, collarPos, pBones[iCollar] );

			matrix3x4_t collarDelta, collarOldInv;
			MatrixInvert( oldCollar, collarOldInv );
			ConcatTransforms( pBones[iCollar], collarOldInv, collarDelta );

			int skipCollar[] = { iCollar };
			VRIK_CascadeBoneDelta( pAnim, pHdr, pBones, iCollar, collarDelta, skipCollar, 1 );

#ifdef CLIENT_DLL
			if ( bDebug )
			{
				Vector newShoulderPos;
				MatrixPosition( pBones[iUpperArm], newShoulderPos );
				debugoverlay->AddLineOverlay( collarPos, newShoulderPos, 255, 255, 0, true, 0.0f );
			}
#endif
		}
	}

	// --- Two-bone IK ---
	Vector animShoulderPos, animElbowPos, animHandPos;
	MatrixPosition( pBones[iUpperArm], animShoulderPos );
	MatrixPosition( pBones[iLowerArm], animElbowPos );
	MatrixPosition( pBones[iHand], animHandPos );

	matrix3x4_t oldUpperArm, oldLowerArm;
	MatrixCopy( pBones[iUpperArm], oldUpperArm );
	MatrixCopy( pBones[iLowerArm], oldLowerArm );

	Vector elbowPos;
	if ( !VRIK_SolveTwoBoneIK( animShoulderPos, targetPos, upperLen, foreLen, poleVector, elbowPos ) )
		return;

	// Upper arm
	Vector animUpperDir = ( animElbowPos - animShoulderPos );
	animUpperDir.NormalizeInPlace();
	Vector ikUpperDir = ( elbowPos - animShoulderPos );
	ikUpperDir.NormalizeInPlace();

	VRIK_RotateBoneToDirection( oldUpperArm, animUpperDir, ikUpperDir, animShoulderPos, pBones[iUpperArm] );

	matrix3x4_t upperDelta, upperOldInv;
	MatrixInvert( oldUpperArm, upperOldInv );
	ConcatTransforms( pBones[iUpperArm], upperOldInv, upperDelta );

	int skipUpper[] = { iUpperArm };
	VRIK_CascadeBoneDelta( pAnim, pHdr, pBones, iUpperArm, upperDelta, skipUpper, 1 );

	// Forearm
	matrix3x4_t currentLowerArm;
	MatrixCopy( pBones[iLowerArm], currentLowerArm );

	Vector animForeDir = ( animHandPos - animElbowPos );
	animForeDir.NormalizeInPlace();
	Vector rotatedForeDir;
	VectorRotate( animForeDir, upperDelta, rotatedForeDir );
	rotatedForeDir.NormalizeInPlace();

	Vector ikForeDir = ( targetPos - elbowPos );
	ikForeDir.NormalizeInPlace();

	VRIK_RotateBoneToDirection( currentLowerArm, rotatedForeDir, ikForeDir, elbowPos, pBones[iLowerArm] );

	// --- Wrist twist distribution ---
	if ( wristTwistShare > 0.001f )
	{
		Vector foreUp;
		MatrixGetColumn( pBones[iLowerArm], 1, foreUp );

		Vector handTargetUp;
		{
			matrix3x4_t handTargetMat;
			AngleMatrix( targetAng, vec3_origin, handTargetMat );
			MatrixGetColumn( handTargetMat, 1, handTargetUp );
		}

		Vector foreUpPerp = foreUp - ikForeDir * DotProduct( foreUp, ikForeDir );
		Vector handUpPerp = handTargetUp - ikForeDir * DotProduct( handTargetUp, ikForeDir );
		float foreUpLen = foreUpPerp.Length();
		float handUpLen = handUpPerp.Length();

		if ( foreUpLen > 0.001f && handUpLen > 0.001f )
		{
			foreUpPerp /= foreUpLen;
			handUpPerp /= handUpLen;

			float cosTwist = clamp( DotProduct( foreUpPerp, handUpPerp ), -1.0f, 1.0f );
			Vector twistAxis = CrossProduct( foreUpPerp, handUpPerp );
			float twistSign = ( DotProduct( twistAxis, ikForeDir ) >= 0.0f ) ? 1.0f : -1.0f;
			float twistAngle = twistSign * RAD2DEG( acosf( cosTwist ) ) * clamp( wristTwistShare, 0.0f, 1.0f );

			if ( fabsf( twistAngle ) > 0.5f )
			{
				Quaternion twistQ;
				AxisAngleQuaternion( ikForeDir, twistAngle, twistQ );

				matrix3x4_t twistMat;
				QuaternionMatrix( twistQ, twistMat );

				matrix3x4_t noTranslate;
				MatrixCopy( pBones[iLowerArm], noTranslate );
				MatrixSetTranslation( vec3_origin, noTranslate );

				matrix3x4_t rotated;
				ConcatTransforms( twistMat, noTranslate, rotated );
				MatrixSetTranslation( elbowPos, rotated );
				MatrixCopy( rotated, pBones[iLowerArm] );
			}
		}
	}

	// Cascade lower arm delta to descendants
	matrix3x4_t lowerDelta, lowerOldInv;
	MatrixInvert( currentLowerArm, lowerOldInv );
	ConcatTransforms( pBones[iLowerArm], lowerOldInv, lowerDelta );

	int skipLower[] = { iLowerArm };
	VRIK_CascadeBoneDelta( pAnim, pHdr, pBones, iLowerArm, lowerDelta, skipLower, 1 );

	// Hand: set rotation from VR angles, position from IK chain
	Vector handWorldPos;
	MatrixPosition( pBones[iHand], handWorldPos );

	matrix3x4_t oldHandAfterCascade;
	MatrixCopy( pBones[iHand], oldHandAfterCascade );

	AngleMatrix( targetAng, handWorldPos, pBones[iHand] );

	matrix3x4_t handDelta, handOldInv;
	MatrixInvert( oldHandAfterCascade, handOldInv );
	ConcatTransforms( pBones[iHand], handOldInv, handDelta );

	int skipHand[] = { iHand };
	VRIK_CascadeBoneDelta( pAnim, pHdr, pBones, iHand, handDelta, skipHand, 1 );

#ifdef CLIENT_DLL
	if ( bDebug )
	{
		debugoverlay->AddLineOverlay( animShoulderPos, elbowPos, 0, 255, 0, true, 0.0f );
		debugoverlay->AddLineOverlay( elbowPos, targetPos, 0, 128, 255, true, 0.0f );
		debugoverlay->AddBoxOverlay( animShoulderPos, Vector(-1,-1,-1), Vector(1,1,1), vec3_angle, 255, 0, 0, 128, 0.0f );
		debugoverlay->AddBoxOverlay( elbowPos, Vector(-1,-1,-1), Vector(1,1,1), vec3_angle, 0, 255, 0, 128, 0.0f );
		debugoverlay->AddBoxOverlay( targetPos, Vector(-1,-1,-1), Vector(1,1,1), vec3_angle, 0, 0, 255, 128, 0.0f );
	}
#endif
}
