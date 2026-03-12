#include "cbase.h"
#include "particles/particles.h"
#include "const.h"
#include "tier2/renderutils.h"
#include "bspflags.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_INIT_AssignTargetCP : public CParticleOperatorInstance
{
public:
	DECLARE_PARTICLE_OPERATOR( C_INIT_AssignTargetCP );

	uint32 GetReadAttributes( void ) const OVERRIDE
	{
		return 0;
	}

	uint32 GetWrittenAttributes( void ) const OVERRIDE
	{
		return 0x200000; // ??
	}

	uint64 GetReadControlPointMask( void ) const OVERRIDE
	{
		uint64 nMask = 0;

		return nMask;
	}

	bool IsScrubSafe( void ) OVERRIDE
	{
		return true;
	}

	virtual void InitNewParticlesScalar( CParticleCollection *pParticles, int start_p,
										 int nParticleCount, int nAttributeWriteMask,
										 void *pContext) const;

	int m_nStartCP;
	int m_nEndCP;
};

DEFINE_PARTICLE_OPERATOR( C_INIT_AssignTargetCP, "Assign target CP", OPERATOR_GENERIC );

BEGIN_PARTICLE_OPERATOR_UNPACK( C_INIT_AssignTargetCP )
	DMXELEMENT_UNPACK_FIELD( "starting control point", "0", int, m_nStartCP )
	DMXELEMENT_UNPACK_FIELD( "maximum end control point", "10", int, m_nEndCP )
END_PARTICLE_OPERATOR_UNPACK( C_INIT_AssignTargetCP );

void C_INIT_AssignTargetCP::InitNewParticlesScalar(
	CParticleCollection *pParticles, int start_p,
	int nParticleCount, int nAttributeWriteMask, void *pContext ) const
{
	for ( ; nParticleCount--; start_p++ )
	{

	}
}

class C_INIT_ControlPointLifetime : public CParticleOperatorInstance
{
public:
	DECLARE_PARTICLE_OPERATOR( C_INIT_ControlPointLifetime );

	uint32 GetReadAttributes( void ) const OVERRIDE
	{
		return 0x200000; // ??
	}

	uint32 GetWrittenAttributes( void ) const OVERRIDE
	{
		return PARTICLE_ATTRIBUTE_LIFE_DURATION_MASK;
	}

	uint64 GetReadControlPointMask( void ) const OVERRIDE
	{
		uint64 nMask = 0;

		return nMask;
	}

	bool IsScrubSafe( void ) OVERRIDE
	{
		return true;
	}

	bool InitMultipleOverride( void ) OVERRIDE
	{
		return true;
	}

	virtual void InitNewParticlesScalar( CParticleCollection *pParticles, int start_p,
										 int nParticleCount, int nAttributeWriteMask,
										 void *pContext ) const;

	int m_nStartCP;
	int m_nEndCP;
};

DEFINE_PARTICLE_OPERATOR( C_INIT_ControlPointLifetime, "Lifetime From Control Point Life Time", OPERATOR_GENERIC );

BEGIN_PARTICLE_OPERATOR_UNPACK( C_INIT_ControlPointLifetime )
	DMXELEMENT_UNPACK_FIELD( "starting control point", "0", int, m_nStartCP )
	DMXELEMENT_UNPACK_FIELD( "maximum end control point", "10", int, m_nEndCP )
END_PARTICLE_OPERATOR_UNPACK( C_INIT_ControlPointLifetime );

void C_INIT_ControlPointLifetime::InitNewParticlesScalar(
	CParticleCollection *pParticles, int start_p,
	int nParticleCount, int nAttributeWriteMask, void *pContext ) const
{
	for ( ; nParticleCount--; start_p++ )
	{
		
	}
}

class C_INIT_GenericCylinder : public CParticleOperatorInstance
{
public:
	DECLARE_PARTICLE_OPERATOR( C_INIT_GenericCylinder );

	uint32 GetReadAttributes( void ) const OVERRIDE
	{
		return PARTICLE_ATTRIBUTE_CREATION_TIME_MASK;
	}

	uint32 GetWrittenAttributes( void ) const OVERRIDE
	{
		return PARTICLE_ATTRIBUTE_XYZ_MASK | PARTICLE_ATTRIBUTE_PREV_XYZ_MASK;
	}

	uint64 GetReadControlPointMask( void ) const OVERRIDE
	{
		uint nMask = 0;

		return nMask;
	}

	bool IsScrubSafe( void ) OVERRIDE
	{
		return true;
	}

	virtual void InitNewParticlesScalar( CParticleCollection *pParticles, int start_p,
										 int nParticleCount, int nAttributeWriteMask,
										 void *pContext) const;

	int m_nStartCP;
	int m_nEndCP;
	float m_flVelocityScalarMin;
	float m_flVelocityScalarMax;
	float m_flParticleScalarMin;
	float m_flParticleScalarMax;
};

DEFINE_PARTICLE_OPERATOR( C_INIT_GenericCylinder, "Random position within a curved cylinder", OPERATOR_GENERIC );

BEGIN_PARTICLE_OPERATOR_UNPACK( C_INIT_GenericCylinder )
	DMXELEMENT_UNPACK_FIELD( "starting control point for cylinder", "0", int, m_nStartCP )
	DMXELEMENT_UNPACK_FIELD( "maximum end control point for cylinder", "10", int, m_nEndCP )
	DMXELEMENT_UNPACK_FIELD( "min scale factor for mapping cp velocity to particle velocity", "0", float, m_flVelocityScalarMin )
	DMXELEMENT_UNPACK_FIELD( "max scale factor for mapping cp velocity to particle velocity", "0", float, m_flVelocityScalarMax )
	DMXELEMENT_UNPACK_FIELD( "min scale factor for particle velocity along path", "0", float, m_flParticleScalarMin )
	DMXELEMENT_UNPACK_FIELD( "max scale factor for particle velocity along path", "0", float, m_flParticleScalarMax )
END_PARTICLE_OPERATOR_UNPACK( C_INIT_GenericCylinder );

void C_INIT_GenericCylinder::InitNewParticlesScalar(
	CParticleCollection *pParticles, int start_p,
	int nParticleCount, int nAttributeWriteMask, void *pContext ) const
{
	const int nControlPoints = pParticles->GetHighestControlPoint();
	for ( int i=0; i < nControlPoints; ++i )
	{
		
	}
}


class C_OP_DistanceToCPToVector : public CParticleOperatorInstance
{
public:
	DECLARE_PARTICLE_OPERATOR( C_OP_DistanceToCPToVector );

	uint32 GetReadInitialAttributes( void ) const OVERRIDE
	{
		return ( 1 << m_nFieldOutput );
	}
	uint32 GetWrittenAttributes( void ) const OVERRIDE
	{
		return ( 1 << m_nFieldOutput ) | PARTICLE_ATTRIBUTE_PREV_XYZ_MASK;
	}

	uint32 GetReadAttributes( void ) const OVERRIDE
	{
		return PARTICLE_ATTRIBUTE_XYZ_MASK;
	}

	uint64 GetReadControlPointMask( void ) const OVERRIDE
	{
		return ( 1ULL << m_nInputCP );
	}

	void InitParams( CParticleSystemDefinition *pDef, CDmxElement *pElement ) OVERRIDE
	{
		m_nInputCP = clamp( m_nInputCP, 0, MAX_PARTICLE_CONTROL_POINTS );
	}

	void Operate( CParticleCollection *pParticles, float flStrength,  void *pContext ) const OVERRIDE;

	int m_nFieldOutput;
	float m_flDistanceMin;
	float m_flDistanceMax;
	Vector m_vecOutputMin;
	Vector m_vecOutputMax;
	int m_nInputCP;
	bool m_bOnlyActiveWithinDistance;
	int m_nLocalCP;
};

DEFINE_PARTICLE_OPERATOR( C_OP_DistanceToCPToVector, "Remap Distance to Control Point to Vector", OPERATOR_GENERIC );

BEGIN_PARTICLE_OPERATOR_UNPACK( C_OP_DistanceToCPToVector )
	DMXELEMENT_UNPACK_FIELD( "distance minimum", "0", float, m_flDistanceMin )
	DMXELEMENT_UNPACK_FIELD( "distance maximum", "128", float, m_flDistanceMin )
	DMXELEMENT_UNPACK_FIELD_USERDATA( "output field", "3", int, m_nFieldOutput, "intchoice particlefield_vector" )
	DMXELEMENT_UNPACK_FIELD( "output minimum", "0 0 0", Vector, m_vecOutputMin )
	DMXELEMENT_UNPACK_FIELD( "output maximum", "0 0 0", Vector, m_vecOutputMax )
	DMXELEMENT_UNPACK_FIELD( "control point", "0", int, m_nInputCP )
	DMXELEMENT_UNPACK_FIELD( "only active within specified distance", "0", bool, m_bOnlyActiveWithinDistance )
	DMXELEMENT_UNPACK_FIELD( "local space CP", "-1", int, m_nLocalCP )
END_PARTICLE_OPERATOR_UNPACK( C_OP_DistanceToCPToVector );

void C_OP_DistanceToCPToVector::Operate( CParticleCollection *pParticles, float flStrength, void *pContext ) const
{
	Vector vecControlPoint = pParticles->GetControlPointAtCurrentTime( m_nInputCP );

	Vector vecMin = m_vecOutputMin;
	Vector vecMax = m_vecOutputMax;
	if ( m_nLocalCP != -1 )
	{
		matrix3x4_t matrix;
		pParticles->GetControlPointTransformAtCurrentTime( m_nLocalCP, &matrix );

		VectorRotate( m_vecOutputMin, matrix, vecMin );
		VectorRotate( m_vecOutputMax, matrix, vecMax );
	}

	const float *pXYZ;
	for ( int i = 0; i < pParticles->m_nActiveParticles; ++i )
	{
		pXYZ = pParticles->GetFloatAttributePtr( PARTICLE_ATTRIBUTE_XYZ, i );

		Vector vecParticle( pXYZ[0], pXYZ[1], pXYZ[2] );
		Vector vecToCP = vecParticle - vecControlPoint;

		float flLength = vecToCP.Length();

		if ( m_bOnlyActiveWithinDistance && ( flLength < m_flDistanceMin || flLength > m_flDistanceMax ) )
			continue;

		Vector vecOutput;
		vecOutput.x = RemapValClamped( flLength, m_flDistanceMin, m_flDistanceMax, vecMin.x, vecMax.x );
		vecOutput.y = RemapValClamped( flLength, m_flDistanceMin, m_flDistanceMax, vecMin.y, vecMax.y );
		vecOutput.z = RemapValClamped( flLength, m_flDistanceMin, m_flDistanceMax, vecMin.z, vecMax.z );

		float *pOutput = pParticles->GetFloatAttributePtrForWrite( m_nFieldOutput, i );
		
		Vector vecInput;
		SetVectorFromAttribute( vecInput, pOutput );

		Vector vecLerpLocal = vec3_origin;
		VectorLerp( vecInput, vecOutput, flStrength, vecLerpLocal );
		vecOutput = vecLerpLocal;

		if ( m_nFieldOutput == PARTICLE_ATTRIBUTE_TINT_RGB )
		{
			ColorClampTruncate( vecOutput );
			SetVectorAttribute( pOutput, vecOutput );
		}
		else
		{
			SetVectorAttribute( pOutput, vecOutput );
		}
	}
}

class C_OP_MovementFollowCP : public CParticleOperatorInstance
{
public:
	DECLARE_PARTICLE_OPERATOR( C_OP_MovementFollowCP );

	uint32 GetWrittenAttributes( void ) const OVERRIDE
	{
		uint nAttributes = PARTICLE_ATTRIBUTE_PREV_XYZ_MASK | PARTICLE_ATTRIBUTE_XYZ_MASK;
		if ( m_flLerpSpeed > 0 )
			nAttributes |= PARTICLE_ATTRIBUTE_RADIUS_MASK;
		if ( m_bUpdateParticleLife )
			nAttributes |= PARTICLE_ATTRIBUTE_LIFE_DURATION_MASK;

		return nAttributes;
	}

	uint32 GetReadAttributes( void ) const OVERRIDE
	{
		uint nAttributes = PARTICLE_ATTRIBUTE_PREV_XYZ_MASK | PARTICLE_ATTRIBUTE_XYZ_MASK | 0x200000;
		if ( m_flLerpSpeed > 0 )
			nAttributes |= PARTICLE_ATTRIBUTE_RADIUS_MASK;
		if ( m_bUpdateParticleLife )
			nAttributes |= PARTICLE_ATTRIBUTE_LIFE_DURATION_MASK;

		return nAttributes;
	}

	uint64 GetReadControlPointMask( void ) const OVERRIDE
	{
		uint nMask = 0;
		

		return nMask;
	}

	void Operate( CParticleCollection *pParticles, float flStrength, void *pContext ) const OVERRIDE;

	int m_nStartCP;
	int m_nMaximumEndCP;
	float m_flCatchUpSpeed;
	float m_flLerpSpeed;
	bool m_bUpdateParticleLife;
};

DEFINE_PARTICLE_OPERATOR( C_OP_MovementFollowCP, "Movement Follow CP", OPERATOR_GENERIC );

BEGIN_PARTICLE_OPERATOR_UNPACK( C_OP_MovementFollowCP )
	DMXELEMENT_UNPACK_FIELD( "starting control point", "0", int, m_nStartCP )
	DMXELEMENT_UNPACK_FIELD( "maximum end control point", "10", int, m_nMaximumEndCP )
	DMXELEMENT_UNPACK_FIELD( "catch up speed", "0", float, m_flCatchUpSpeed )
	DMXELEMENT_UNPACK_FIELD( "lerp to CP radius speed", "0", float, m_flLerpSpeed )
	DMXELEMENT_UNPACK_FIELD( "update particle lifetime", "0", bool, m_bUpdateParticleLife )
END_PARTICLE_OPERATOR_UNPACK( C_OP_MovementFollowCP );

void C_OP_MovementFollowCP::Operate( CParticleCollection *pParticles, float flStrength, void *pContext ) const
{

}


class C_OP_LockToSavedSequentialPath : public CParticleOperatorInstance
{
public:
	DECLARE_PARTICLE_OPERATOR( C_OP_LockToSavedSequentialPath );

	uint32 GetWrittenAttributes( void ) const OVERRIDE
	{
		return PARTICLE_ATTRIBUTE_HITBOX_RELATIVE_XYZ_MASK | PARTICLE_ATTRIBUTE_HITBOX_INDEX_MASK | PARTICLE_ATTRIBUTE_CREATION_TIME_MASK;
	}

	uint32 GetReadAttributes( void ) const OVERRIDE
	{
		return PARTICLE_ATTRIBUTE_XYZ_MASK | PARTICLE_ATTRIBUTE_PREV_XYZ_MASK;
	}

	uint64 GetReadControlPointMask( void ) const OVERRIDE
	{
		uint64 nStartMask = ( 1ULL << m_PathParams.m_nStartControlPointNumber ) - 1;
		uint64 nEndMask = ( 1ULL << ( m_PathParams.m_nEndControlPointNumber + 1 ) ) - 1;

		return nEndMask & ( ~nStartMask );
	}

	void InitParams( CParticleSystemDefinition *pDef, CDmxElement *pElement ) OVERRIDE
	{
		m_PathParams.ClampControlPointIndices();
	}

	void InitializeContextData( CParticleCollection *pParticle, void *pContext ) const OVERRIDE
	{

	}

	size_t GetRequiredContextBytes( void ) const OVERRIDE
	{
		return 40;
	}

	void Operate( CParticleCollection *pParticles, float flStrength, void *pContext ) const OVERRIDE;

	bool m_bUseSequentialCPPairs;
	CPathParameters m_PathParams;
};

DEFINE_PARTICLE_OPERATOR( C_OP_LockToSavedSequentialPath, "Movement Lock to Saved Position Along Path", OPERATOR_GENERIC );

BEGIN_PARTICLE_OPERATOR_UNPACK( C_OP_LockToSavedSequentialPath )
	DMXELEMENT_UNPACK_FIELD( "bulge", "0", float, m_PathParams.m_flBulge )
	DMXELEMENT_UNPACK_FIELD( "start control point number", "0", int, m_PathParams.m_nStartControlPointNumber )
	DMXELEMENT_UNPACK_FIELD( "end control point number", "1", int, m_PathParams.m_nEndControlPointNumber )
	DMXELEMENT_UNPACK_FIELD( "bulge control 0=random 1=orientation of start pnt 2=orientation of end point", "0", int, m_PathParams.m_nBulgeControl )
	DMXELEMENT_UNPACK_FIELD( "mid point position", "0.5", float, m_PathParams.m_flMidPoint )
	DMXELEMENT_UNPACK_FIELD( "Use sequential CP pairs between start and end point", "0", bool, m_bUseSequentialCPPairs )
END_PARTICLE_OPERATOR_UNPACK( C_OP_LockToSavedSequentialPath );

void C_OP_LockToSavedSequentialPath::Operate( CParticleCollection *pParticles, float flStrength, void *pContext ) const
{
}


void AddCustomParticleOps( void )
{
	REGISTER_PARTICLE_OPERATOR( FUNCTION_OPERATOR, C_OP_DistanceToCPToVector );
	REGISTER_PARTICLE_OPERATOR( FUNCTION_OPERATOR, C_OP_MovementFollowCP );
	REGISTER_PARTICLE_OPERATOR( FUNCTION_OPERATOR, C_OP_LockToSavedSequentialPath );
	REGISTER_PARTICLE_OPERATOR( FUNCTION_INITIALIZER, C_INIT_AssignTargetCP );
	REGISTER_PARTICLE_OPERATOR( FUNCTION_INITIALIZER, C_INIT_ControlPointLifetime );
	REGISTER_PARTICLE_OPERATOR( FUNCTION_INITIALIZER, C_INIT_GenericCylinder );
}
