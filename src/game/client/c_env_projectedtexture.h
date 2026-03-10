//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_ENVPROJECTED_TEXTURE_H
#define C_ENVPROJECTED_TEXTURE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "basetypes.h"
#include "../../materialsystem/stdshaders/IShaderExtension.h"
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_EnvProjectedTexture : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvProjectedTexture, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	void	ShutDownLightHandle( void );

	virtual void Simulate();

	void	UpdateLight( bool bForceUpdate );

// GSTRINGMIGRATION
	virtual void UpdateOnRemove()
	{
		if ( m_pVolmetricMesh != NULL )
		{
			CMatRenderContextPtr pRenderContext( materials );
			pRenderContext->DestroyStaticMesh( m_pVolmetricMesh );
			m_pVolmetricMesh = NULL;
		}
		BaseClass::UpdateOnRemove();
	}

	virtual bool					IsTransparent() { return true; }
	virtual bool					IsTwoPass() { return false; }

	virtual void GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
	{
		if ( m_bEnableVolumetrics )
		{
			mins = m_vecRenderBoundsMin;
			maxs = m_vecRenderBoundsMax;
		}
		else
		{
			BaseClass::GetRenderBoundsWorldspace( mins, maxs );
		}
	}
	virtual bool ShouldDraw() { return true; }
	virtual int DrawModel( int flags );

	virtual bool ShouldReceiveProjectedTextures( int flags ) { return false; }

	void ClearVolumetricsMesh();

	C_EnvProjectedTexture();
	~C_EnvProjectedTexture();

private:

	void RebuildVolumetricMesh();
	void GetShadowViewSetup( CViewSetup &setup );

	CTextureReference m_SpotlightTexture;

	IMesh	*m_pVolmetricMesh;
	CMaterialReference m_matVolumetricsMaterial;

	ClientShadowHandle_t m_LightHandle;

	EHANDLE	m_hTargetEntity;

	bool	m_bState;
	float	m_flLightFOV;
	bool	m_bEnableShadows;
	bool	m_bLightOnlyTarget;
	bool	m_bLightWorld;
	bool	m_bCameraSpace;
	
	Vector	m_LinearFloatLightColor;
	float	m_flAmbient;
	float	m_flNearZ;
	float	m_flFarZ;
	char	m_SpotlightTextureName[MAX_PATH];
	int		m_nSpotlightTextureFrame;
	int		m_nShadowQuality;
	float		m_flShadowFilterSize;

	FlashlightState_t	m_FlashlightState;
	UberlightState_t	m_UberlightState;
	Vector m_vecRenderBoundsMin, m_vecRenderBoundsMax;

	bool m_bEnableVolumetrics;
	bool m_bEnableVolumetricsLOD;
	float m_flVolumetricsFadeDistance;
	int m_iVolumetricsQuality;
	float m_flVolumetricsMultiplier;
	float m_flVolumetricsQualityBias;

	float m_flLastFOV;
	int m_iCurrentVolumetricsSubDiv;
};

#endif // C_ENV_PROJECTED_TEXTURE_H