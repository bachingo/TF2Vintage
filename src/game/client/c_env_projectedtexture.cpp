//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"
#include "c_env_projectedtexture.h"
#include "view_scene.h"
#include "viewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_slopescaledepthbias_shadowmap( "mat_slopescaledepthbias_shadowmap", "4", FCVAR_CHEAT );
static ConVar mat_depthbias_shadowmap( "mat_depthbias_shadowmap", "0.00001", FCVAR_CHEAT );

static ConVar volumetrics_fade_range( "volumetrics_fade_range", "128.0", FCVAR_CHEAT  );
ConVar volumetrics_enabled( "volumetrics_enabled", "1", FCVAR_ARCHIVE );

static void volumetrics_subdiv_Callback( IConVar *var, const char *, float )
{
	for ( C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity(); pEnt != NULL;
		pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
	{
		C_EnvProjectedTexture *pProjectedTexture = dynamic_cast< C_EnvProjectedTexture* >( pEnt );
		if ( pProjectedTexture != NULL )
		{
			pProjectedTexture->ClearVolumetricsMesh();
		}
	}
}
static ConVar volumetrics_subdiv( "volumetrics_subdiv", "0", 0, "", volumetrics_subdiv_Callback );

IMPLEMENT_CLIENTCLASS_DT( C_EnvProjectedTexture, DT_EnvProjectedTexture, CEnvProjectedTexture )
	RecvPropEHandle( RECVINFO( m_hTargetEntity ) ),
	RecvPropBool( RECVINFO( m_bState ) ),
	RecvPropFloat( RECVINFO( m_flLightFOV ) ),
	RecvPropBool( RECVINFO( m_bEnableShadows ) ),
	RecvPropBool( RECVINFO( m_bLightOnlyTarget ) ),
	RecvPropBool( RECVINFO( m_bLightWorld ) ),
	RecvPropBool( RECVINFO( m_bCameraSpace ) ),
	RecvPropVector( RECVINFO( m_LinearFloatLightColor ) ),
	RecvPropFloat( RECVINFO( m_flAmbient ) ),
	RecvPropString( RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt( RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat( RECVINFO( m_flNearZ ) ),
	RecvPropFloat( RECVINFO( m_flFarZ ) ),
	RecvPropInt( RECVINFO( m_nShadowQuality ) ),
	RecvPropFloat( RECVINFO( m_flShadowFilterSize ) ),

	RecvPropBool(	 RECVINFO( m_bEnableVolumetrics ) ),
	RecvPropBool(	 RECVINFO( m_bEnableVolumetricsLOD ) ),
	RecvPropFloat(	 RECVINFO( m_flVolumetricsFadeDistance ) ),
	RecvPropInt(	 RECVINFO( m_iVolumetricsQuality ) ),
	RecvPropFloat(	 RECVINFO( m_flVolumetricsQualityBias ) ),
	RecvPropFloat(	 RECVINFO( m_flVolumetricsMultiplier ) ),

	RecvPropBool( RECVINFO_NAME( m_UberlightState.m_bEnabled, m_bUberlight ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fNearEdge, m_fNearEdge ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fFarEdge, m_fFarEdge ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fCutOn, m_fCutOn ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fCutOff, m_fCutOff ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fShearx, m_fShearx ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fSheary, m_fSheary ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fWidth, m_fWidth ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fWedge, m_fWedge ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fHeight, m_fHeight ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fHedge, m_fHedge ) ),
	RecvPropFloat( RECVINFO_NAME( m_UberlightState.m_fRoundness, m_fRoundness ) ),
END_RECV_TABLE()


C_EnvProjectedTexture::C_EnvProjectedTexture( void )
	: m_bEnableVolumetrics( false )
	, m_flLastFOV( 0.0f )
	, m_iCurrentVolumetricsSubDiv( 1 )
	, m_flVolumetricsQualityBias( 1.0f )
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
}

C_EnvProjectedTexture::~C_EnvProjectedTexture( void )
{
	Assert( m_pVolmetricMesh == NULL );

	ShutDownLightHandle();
}

void C_EnvProjectedTexture::ShutDownLightHandle( void )
{
	// Clear out the light
	if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LightHandle );
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : updateType -
//-----------------------------------------------------------------------------
void C_EnvProjectedTexture::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_SpotlightTexture.Init( m_UberlightState.m_bEnabled ? "white" : m_SpotlightTextureName, TEXTURE_GROUP_OTHER, true );
	}

	if ( m_flLastFOV != m_flLightFOV )
	{
		m_flLastFOV = m_flLightFOV;
		ClearVolumetricsMesh();
	}

	UpdateLight( true );

	BaseClass::OnDataChanged( updateType );
}

void C_EnvProjectedTexture::UpdateLight(bool bForceUpdate)
{
	if (m_bState == false)
	{
		if (m_LightHandle != CLIENTSHADOW_INVALID_HANDLE)
		{
			ShutDownLightHandle();
		}

		ClearVolumetricsMesh();
		return;
	}

	Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
	if (m_hTargetEntity != NULL)
	{
		if (m_bCameraSpace)
		{
			const QAngle& angles = GetLocalAngles();

			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			if (pPlayer)
			{
				const QAngle playerAngles = pPlayer->GetAbsAngles();

				Vector vPlayerForward, vPlayerRight, vPlayerUp;
				AngleVectors(playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp);

				matrix3x4_t	mRotMatrix;
				AngleMatrix(angles, mRotMatrix);

				VectorITransform(vPlayerForward, mRotMatrix, vForward);
				VectorITransform(vPlayerRight, mRotMatrix, vRight);
				VectorITransform(vPlayerUp, mRotMatrix, vUp);

				float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
				vPos = m_hTargetEntity->GetAbsOrigin() - vForward * dist;

				VectorNormalize(vForward);
				VectorNormalize(vRight);
				VectorNormalize(vUp);
			}
		}
		else
		{
			// VXP: Fixing targeting
			Vector vecToTarget;
			QAngle vecAngles;
			if (m_hTargetEntity == NULL)
			{
				vecAngles = GetAbsAngles();
			}
			else
			{
				vecToTarget = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
				VectorAngles(vecToTarget, vecAngles);
			}
			AngleVectors(vecAngles, &vForward, &vRight, &vUp);
		}
	}
	else
	{
		AngleVectors(GetAbsAngles(), &vForward, &vRight, &vUp);
	}


	m_FlashlightState.m_fHorizontalFOVDegrees = m_flLightFOV;
	m_FlashlightState.m_fVerticalFOVDegrees = m_flLightFOV;

	m_FlashlightState.m_vecLightOrigin = vPos;
	BasisToQuaternion(vForward, vRight, vUp, m_FlashlightState.m_quatOrientation);

	m_FlashlightState.m_fQuadraticAtten = 0.0;
	m_FlashlightState.m_fLinearAtten = 100;
	m_FlashlightState.m_fConstantAtten = 0.0f;
	m_FlashlightState.m_Color[0] = m_LinearFloatLightColor.x;
	m_FlashlightState.m_Color[1] = m_LinearFloatLightColor.y;
	m_FlashlightState.m_Color[2] = m_LinearFloatLightColor.z;
	m_FlashlightState.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
	m_FlashlightState.m_NearZ = m_flNearZ;
	m_FlashlightState.m_FarZ = m_flFarZ;
	m_FlashlightState.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
	m_FlashlightState.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();
	// Use per-entity shadow filter size if set, otherwise fall back to global csm_filter
	extern ConVar csm_filter;
	m_FlashlightState.m_flShadowFilterSize = ( m_flShadowFilterSize > 0.0f ) ? m_flShadowFilterSize : csm_filter.GetFloat();
	m_FlashlightState.m_bEnableShadows = m_bEnableShadows;
	m_FlashlightState.m_pSpotlightTexture = m_SpotlightTexture;
	m_FlashlightState.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

	//m_FlashlightState.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality

	if (m_LightHandle == CLIENTSHADOW_INVALID_HANDLE)
	{
		m_LightHandle = g_pClientShadowMgr->CreateFlashlight(m_FlashlightState);
	}
	else
	{
		//if ( m_hTargetEntity != NULL || bForceUpdate == true )
		{
			g_pClientShadowMgr->UpdateUberlightState(m_FlashlightState, m_UberlightState);
			g_pClientShadowMgr->UpdateFlashlightState(m_LightHandle, m_FlashlightState);
		}
	}

	if (m_bLightOnlyTarget)
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, m_hTargetEntity);
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, NULL);
	}

	g_pClientShadowMgr->SetFlashlightLightWorld(m_LightHandle, m_bLightWorld);

	g_pClientShadowMgr->UpdateProjectedTexture(m_LightHandle, true);

	if (m_bEnableVolumetrics && m_bEnableShadows)
	{
		CViewSetup setup;
		GetShadowViewSetup(setup);

		VMatrix world2View, view2Proj, world2Proj, world2Pixels;
		render->GetMatricesForView(setup, &world2View, &view2Proj, &world2Proj, &world2Pixels);
		VMatrix proj2world;
		MatrixInverseGeneral(world2Proj, proj2world);

		Vector fwd, right, up;
		AngleVectors(setup.angles, &fwd, &right, &up);

		Vector nearFarPlane[8];
		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, -1, 1), nearFarPlane[0]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, -1, 1), nearFarPlane[1]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, 1, 1), nearFarPlane[2]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, 1, 1), nearFarPlane[3]);

		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, -1, 0), nearFarPlane[4]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, -1, 0), nearFarPlane[5]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, 1, 0), nearFarPlane[6]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, 1, 0), nearFarPlane[7]);

		m_vecRenderBoundsMin.Init(MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT);
		m_vecRenderBoundsMax.Init(MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT);

		for (int i = 0; i < 8; i++)
		{
			m_vecRenderBoundsMin.x = Min(m_vecRenderBoundsMin.x, nearFarPlane[i].x);
			m_vecRenderBoundsMin.y = Min(m_vecRenderBoundsMin.y, nearFarPlane[i].y);
			m_vecRenderBoundsMin.z = Min(m_vecRenderBoundsMin.y, nearFarPlane[i].z);
			m_vecRenderBoundsMax.x = Max(m_vecRenderBoundsMax.x, nearFarPlane[i].x);
			m_vecRenderBoundsMax.y = Max(m_vecRenderBoundsMax.y, nearFarPlane[i].y);
			m_vecRenderBoundsMax.z = Max(m_vecRenderBoundsMax.y, nearFarPlane[i].z);
		}
	}
}

void C_EnvProjectedTexture::Simulate( void )
{
	UpdateLight( GetMoveParent() != NULL );

	BaseClass::Simulate();
}

int C_EnvProjectedTexture::DrawModel( int flags )
{
	if ( !m_bState ||
		m_LightHandle == CLIENTSHADOW_INVALID_HANDLE ||
		!m_bEnableVolumetrics ||
		!volumetrics_enabled.GetBool() ||
		CurrentViewID() != VIEW_MAIN )
	{
		return 0;
	}

	float flDistanceFade = 1.0f;
	if ( m_flVolumetricsFadeDistance > 0.0f )
	{
		Vector delta = CurrentViewOrigin() - GetLocalOrigin();
		flDistanceFade = RemapValClamped( delta.Length(),
			m_flVolumetricsFadeDistance + volumetrics_fade_range.GetFloat(),
			m_flVolumetricsFadeDistance, 0.0f, 1.0f );
	}

	if ( flDistanceFade <= 0.0f )
	{
		return 0;
	}

	ITexture *pDepthTexture = NULL;
	const ShadowHandle_t shadowHandle = g_pClientShadowMgr->GetShadowHandle( m_LightHandle );
	const int iNumActiveDepthTextures = g_pClientShadowMgr->GetNumShadowDepthtextures();
	for ( int i = 0; i < iNumActiveDepthTextures; i++)
	{
		if ( g_pClientShadowMgr->GetShadowDepthHandle( i ) == shadowHandle )
		{
			pDepthTexture = g_pClientShadowMgr->GetShadowDepthTex( i );
			break;
		}
	}

	if ( pDepthTexture == NULL )
	{
		return 0;
	}

	if ( m_pVolmetricMesh == NULL )
	{
		RebuildVolumetricMesh();
	}

	UpdateScreenEffectTexture();

	CViewSetup setup;
	GetShadowViewSetup( setup );

	VMatrix world2View, view2Proj, world2Proj, world2Pixels;
	render->GetMatricesForView( setup, &world2View, &view2Proj, &world2Proj, &world2Pixels );

	VMatrix tmp, shadowToUnit;
	MatrixBuildScale( tmp, 1.0f / 2, 1.0f / -2, 1.0f );
	tmp[0][3] = tmp[1][3] = 0.5f;
	MatrixMultiply( tmp, world2Proj, shadowToUnit );

	m_FlashlightState.m_Color[ 3 ] = m_flVolumetricsMultiplier / m_iCurrentVolumetricsSubDiv * flDistanceFade;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->SetFlashlightMode( true );
	pRenderContext->SetFlashlightStateEx( m_FlashlightState, shadowToUnit, pDepthTexture );

	matrix3x4_t flashlightTransform;
	AngleMatrix( setup.angles, setup.origin, flashlightTransform );

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadMatrix( flashlightTransform );

	pRenderContext->Bind( m_matVolumetricsMaterial );
	m_pVolmetricMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PopMatrix();
	pRenderContext->SetFlashlightMode( false );
	return 1;
}

void C_EnvProjectedTexture::ClearVolumetricsMesh()
{
	if ( m_pVolmetricMesh != NULL )
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->DestroyStaticMesh( m_pVolmetricMesh );
		m_pVolmetricMesh = NULL;
	}
}

void C_EnvProjectedTexture::RebuildVolumetricMesh()
{
	if ( !m_matVolumetricsMaterial.IsValid() )
	{
		m_matVolumetricsMaterial.Init( "engine/light_volumetrics", TEXTURE_GROUP_OTHER );
	}

	ClearVolumetricsMesh();

	CViewSetup setup;
	GetShadowViewSetup( setup );
	setup.origin = vec3_origin;
	setup.angles = vec3_angle;

	VMatrix world2View, view2Proj, world2Proj, world2Pixels;
	render->GetMatricesForView( setup, &world2View, &view2Proj, &world2Proj, &world2Pixels );
	VMatrix proj2world;
	MatrixInverseGeneral( world2Proj, proj2world );

	const Vector origin( vec3_origin );
	QAngle angles( vec3_angle );
	Vector fwd, right, up;
	AngleVectors( angles, &fwd, &right, &up );

	Vector nearPlane[4];
	Vector farPlane[4];
	Vector3DMultiplyPositionProjective( proj2world, Vector( -1, -1, 1 ), farPlane[ 0 ] );
	Vector3DMultiplyPositionProjective( proj2world, Vector( 1, -1, 1 ), farPlane[ 1 ] );
	Vector3DMultiplyPositionProjective( proj2world, Vector( 1, 1, 1 ), farPlane[ 2 ] );
	Vector3DMultiplyPositionProjective( proj2world, Vector( -1, 1, 1 ), farPlane[ 3 ] );

	Vector3DMultiplyPositionProjective( proj2world, Vector( -1, -1, 0 ), nearPlane[ 0 ] );
	Vector3DMultiplyPositionProjective( proj2world, Vector( 1, -1, 0 ), nearPlane[ 1 ] );
	Vector3DMultiplyPositionProjective( proj2world, Vector( 1, 1, 0 ), nearPlane[ 2 ] );
	Vector3DMultiplyPositionProjective( proj2world, Vector( -1, 1, 0 ), nearPlane[ 3 ] );

	DebugDrawLine( farPlane[ 0 ], farPlane[ 1 ], 0, 0, 255, true, -1 );
	DebugDrawLine( farPlane[ 1 ], farPlane[ 2 ], 0, 0, 255, true, -1 );
	DebugDrawLine( farPlane[ 2 ], farPlane[ 3 ], 0, 0, 255, true, -1 );
	DebugDrawLine( farPlane[ 3 ], farPlane[ 0 ], 0, 0, 255, true, -1 );
	DebugDrawLine( nearPlane[ 0 ], nearPlane[ 1 ], 0, 0, 255, true, -1 );
	DebugDrawLine( nearPlane[ 1 ], nearPlane[ 2 ], 0, 0, 255, true, -1 );
	DebugDrawLine( nearPlane[ 2 ], nearPlane[ 3 ], 0, 0, 255, true, -1 );
	DebugDrawLine( nearPlane[ 3 ], nearPlane[ 0 ], 0, 0, 255, true, -1 );
	DebugDrawLine( farPlane[ 0 ], nearPlane[ 0 ], 0, 0, 255, true, -1 );
	DebugDrawLine( farPlane[ 1 ], nearPlane[ 1 ], 0, 0, 255, true, -1 );
	DebugDrawLine( farPlane[ 2 ], nearPlane[ 2 ], 0, 0, 255, true, -1 );
	DebugDrawLine( farPlane[ 3 ], nearPlane[ 3 ], 0, 0, 255, true, -1 );

	const Vector vecDirections[3] = {
		( farPlane[ 0 ] - vec3_origin ),
		( farPlane[ 1 ] - farPlane[ 0 ] ),
		( farPlane[ 3 ] - farPlane[ 0 ] ),
	};

	const VertexFormat_t vertexFormat =  VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 );
	CMatRenderContextPtr pRenderContext( materials );
	m_pVolmetricMesh = pRenderContext->CreateStaticMesh( vertexFormat, TEXTURE_GROUP_OTHER, m_matVolumetricsMaterial );

	const int iCvarSubDiv = volumetrics_subdiv.GetInt();
	m_iCurrentVolumetricsSubDiv = ( iCvarSubDiv > 2 ) ? iCvarSubDiv : Max( m_iVolumetricsQuality, 3 );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( m_pVolmetricMesh, MATERIAL_TRIANGLES, m_iCurrentVolumetricsSubDiv * 6 - 4 );

	for ( int x = 1; x < m_iCurrentVolumetricsSubDiv * 2; x++ )
	{
		// never 0.0 or 1.0
		float flFracX = x / float( m_iCurrentVolumetricsSubDiv * 2 );
		//flFracX = powf( flFracX, 3.0f );
		flFracX = powf( flFracX, m_flVolumetricsQualityBias );

		Vector v00 = origin + vecDirections[ 0 ] * flFracX;
		Vector v10 = v00 + vecDirections[ 1 ] * flFracX;
		Vector v11 = v10 + vecDirections[ 2 ] * flFracX;
		Vector v01 = v00 + vecDirections[ 2 ] * flFracX;

		meshBuilder.Position3f( XYZ( v00 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v10 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v11 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v00 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v11 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v01 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();
	}

	for ( int x = 1; x < m_iCurrentVolumetricsSubDiv; x++ )
	{
		// never 0.0 or 1.0
		const float flFracX = x / float( m_iCurrentVolumetricsSubDiv );
		Vector v0 = origin + vecDirections[ 0 ] + vecDirections[ 1 ] * flFracX;
		Vector v1 = v0 + vecDirections[ 2 ];

		meshBuilder.Position3f( XYZ( origin ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v0 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v1 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();
	}

	for ( int x = 1; x < m_iCurrentVolumetricsSubDiv; x++ )
	{
		// never 0.0 or 1.0
		const float flFracX = x / float( m_iCurrentVolumetricsSubDiv );
		Vector v0 = origin + vecDirections[ 0 ] + vecDirections[ 2 ] * flFracX;
		Vector v1 = v0 + vecDirections[ 1 ];

		meshBuilder.Position3f( XYZ( origin ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v0 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( XYZ( v1 ) );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
}

void C_EnvProjectedTexture::GetShadowViewSetup( CViewSetup &setup )
{
	setup.origin = m_FlashlightState.m_vecLightOrigin;
	QuaternionAngles( m_FlashlightState.m_quatOrientation, setup.angles );
	setup.fov = m_flLightFOV;
	setup.zFar = m_flFarZ;
	setup.zNear = m_flNearZ;
	setup.m_bOrtho = false;
	setup.m_flAspectRatio = 1.0f;
	setup.x = setup.y = 0;
	setup.width = m_FlashlightState.m_pSpotlightTexture ? m_FlashlightState.m_pSpotlightTexture->GetActualWidth() : 512;
	setup.height = m_FlashlightState.m_pSpotlightTexture ? m_FlashlightState.m_pSpotlightTexture->GetActualHeight() : 512;
}
