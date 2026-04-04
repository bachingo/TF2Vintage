#include "cbase.h"
#include "vr_rendertargets.h"
#include "materialsystem\imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
#include "rendertexture.h"
#include "../public/materialsystem/itexture.h"
#include "hmdWrapper.h"
#include "openxr_manager.h"

extern ConVar tfvr_msaa;

#define TARGET_DIVISOR 1.3f	//This was 2, but 1 seems to work. The blur function seems to bleed out enough.
							//We may at some point want a value betweem 1 and 2

ITexture* CVrRenderTargets::CreateVGuiTexture(IMaterialSystem* pMaterialSystem)
{
	// Use RGBA8888 format for proper VGUI alpha blending
	// Note: sRGB correction will be handled in the VR compositor shader
	ImageFormat vguiFormat = IMAGE_FORMAT_RGBA8888;
	
	// Check if MSAA is enabled via mat_antialias setting
	const MaterialSystem_Config_t &config = pMaterialSystem->GetCurrentConfigForVideoCard();
	bool bMSAAEnabled = config.m_nAASamples > 1;
	
	// For VGUI, we want RGBA format for alpha blending, but we can still benefit from MSAA
	// Use back buffer format only if it supports alpha, otherwise stick with RGBA8888
	ImageFormat targetFormat = vguiFormat;
	if (bMSAAEnabled) {
		// Check if back buffer format has alpha support for UI rendering
		ImageFormat backBufferFormat = pMaterialSystem->GetBackBufferFormat();
		if (backBufferFormat == IMAGE_FORMAT_RGBA8888 || backBufferFormat == IMAGE_FORMAT_BGRA8888) {
			targetFormat = backBufferFormat;  // Use back buffer format to inherit MSAA
		}
		// Otherwise stick with RGBA8888 - MSAA inheritance happens through MATERIAL_RT_DEPTH_SHARED
	}
	
	uint32_t vguiW, vguiH;
	g_pOpenXRManager->GetSpectatorScreenDims(vguiW, vguiH);

	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_vgui",
		vguiW, vguiH,
		RT_SIZE_LITERAL,
		targetFormat,								// Use appropriate format with MSAA consideration
		MATERIAL_RT_DEPTH_SHARED,					// Use shared depth - this inherits MSAA from back buffer
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,	// Minimal flags like water refraction
		CREATERENDERTARGETFLAGS_HDR);				// HDR flag for proper alpha handling
}

ITexture* CVrRenderTargets::CreateVRTwoEyesHMDRenderTarget(IMaterialSystem* pMaterialSystem, int i)
{
	// Use the VR-specific MSAA setting (tfvr_msaa) instead of forcing it through DXVK
	// This allows UI render targets to use mat_antialias while VR eyes use tfvr_msaa
	int vrMSAA = tfvr_msaa.GetInt();
	dxvkSetRenderTextureSize(g_pOpenXRManager->GetBufferSize().x * 2, g_pOpenXRManager->GetBufferSize().y, vrMSAA);
    const char* name = backBufferNamePerIndex(i);
    return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
        name,
        g_pOpenXRManager->GetBufferSize().x * 2,
        g_pOpenXRManager->GetBufferSize().y,
        RT_SIZE_LITERAL, 
		IMAGE_FORMAT_BGRA8888,
        MATERIAL_RT_DEPTH_SEPARATE,		// VR eyes need separate depth for proper 3D rendering
        TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_SRGB,
        0);
}
								//No longer quater, but a pain in the ass to change
ITexture* CVrRenderTargets::CreateVROneEyeTextureQuarterSize(IMaterialSystem* pMaterialSystem)	
{
    return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
        "_rt_one_eye_quarter_size_1_VR",
		(int)floor(g_pOpenXRManager->GetBufferSize().x / TARGET_DIVISOR), (int)floor(g_pOpenXRManager->GetBufferSize().y / TARGET_DIVISOR),
        RT_SIZE_LITERAL,
        pMaterialSystem->GetBackBufferFormat(),
        MATERIAL_RT_DEPTH_SEPARATE,
        TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
        CREATERENDERTARGETFLAGS_HDR);
}

ITexture* CVrRenderTargets::CreateWaterReflectionTexture( IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_WaterReflection",
		iSize, iSize, RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_NONE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture* CVrRenderTargets::CreateVRWaterReflectionTexture( IMaterialSystem* pMaterialSystem, int iSize )
{
	// needed to work around a base game bug that's causing stereo mismatch in VR
	// see VRViewRender::CopyProperWaterReflectionForEye()
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_WaterReflectionVR",
		2*iSize, iSize, RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SEPARATE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture* CVrRenderTargets::CreateWaterRefractionTexture( IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_WaterRefraction",
		iSize, iSize, RT_SIZE_LITERAL,
		// This is different than reflection because it has to have alpha for fog factor.
		IMAGE_FORMAT_RGBA8888,
		MATERIAL_RT_DEPTH_SEPARATE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture* CVrRenderTargets::CreateVRScreenEffectTexture( IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_VRScreenEffect",
		iSize, iSize, RT_SIZE_LITERAL,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SEPARATE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture *CVrRenderTargets::GetVRRenderTarget(int i)
{
	return m_VRTwoEyesHMDRenderTargets[i];
}

ITexture *CVrRenderTargets::GetVRHandsRenderTarget()
{
	return m_VRHandsRenderTarget;
}

ITexture* CVrRenderTargets::CreateVRHandsRenderTarget(IMaterialSystem* pMaterialSystem)
{
	// Same size as one eye for per-eye VR hand rendering.
	// RGBA8888 with alpha for compositing (sniper scope, isolated hand rendering).
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_VRHands",
		g_pOpenXRManager->GetBufferSize().x,
		g_pOpenXRManager->GetBufferSize().y,
		RT_SIZE_LITERAL,
		IMAGE_FORMAT_RGBA8888,
		MATERIAL_RT_DEPTH_SEPARATE,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR);
}

static void GetDesiredFullFrameBufferDimensions(int &width, int &height)
{
	width = g_pOpenXRManager->GetBufferSize().x;
	height = g_pOpenXRManager->GetBufferSize().y;
}

void CVrRenderTargets::UpdateVRRenderTargets()
{
	int newMsaa = tfvr_msaa.GetInt();
	bool msaaChanged = newMsaa != m_currentMsaa;
	int desiredWidth = g_pOpenXRManager->GetBufferSize().x * 2;
	int desiredHeight = g_pOpenXRManager->GetBufferSize().y;
	int fullFrameWidth, fullFrameHeight;
	GetDesiredFullFrameBufferDimensions(fullFrameWidth, fullFrameHeight);
	
	ITexture *tex = m_VRTwoEyesHMDRenderTargets[0];
	if (tex == nullptr)
		return;

	uint32_t vguiDesiredW, vguiDesiredH;
	g_pOpenXRManager->GetSpectatorScreenDims(vguiDesiredW, vguiDesiredH);
	ITexture *vguiTex = m_VGuiTexture;
	bool vguiNeedsUpdate = vguiTex && ((uint32_t)vguiTex->GetActualWidth() != vguiDesiredW || (uint32_t)vguiTex->GetActualHeight() != vguiDesiredH);

	ITexture *fullFrameTex0 = materials->FindTexture("_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET, false);
	bool baseTexturesNeedUpdate = fullFrameTex0 && (fullFrameTex0->GetActualWidth() != fullFrameWidth || fullFrameTex0->GetActualHeight() != fullFrameHeight);

	if (tex->GetActualWidth() != desiredWidth || tex->GetActualHeight() != desiredHeight || msaaChanged || baseTexturesNeedUpdate || vguiNeedsUpdate)
	{
		ConVarRef mat_queue_mode("mat_queue_mode");
		if (mat_queue_mode.GetInt() != 0)
		{
			m_origThreadMode = mat_queue_mode.GetInt();
			m_changedThreadMode = true;
			mat_queue_mode.SetValue(0);
			return;
		}

		Log("Updating render textures...\n");

		m_currentMsaa = newMsaa;
		materials->BeginRenderTargetAllocation();
		for (int i = 0; i < VR_NUM_BUFFERS; ++i)
		{
			m_VRTwoEyesHMDRenderTargets[i].Init(CreateVRTwoEyesHMDRenderTarget(materials, i));
		}
		m_VROneEyeTextureQuarterSize.Init(CreateVROneEyeTextureQuarterSize(materials));
		m_VRHandsRenderTarget.Init(CreateVRHandsRenderTarget(materials));

		m_VGuiTexture.Init(CreateVGuiTexture(materials));

		UpdateBaseGameTextures(g_pMaterialSystem);

		materials->EndRenderTargetAllocation();

		if (m_changedThreadMode)
		{
			mat_queue_mode.SetValue(m_origThreadMode);
			m_changedThreadMode = false;
		}

		Log("Render textures updated.\n");
	}
}

// need to get access to the reference count of the textures
abstract_class ITextureInternal : public ITexture
{
public:
	virtual void Func0() = 0;
	virtual void Func1() = 0;
	virtual int GetReferenceCount() = 0;
};


void CVrRenderTargets::UpdateBaseGameRenderTexture(const char *name, int desiredWidth, int desiredHeight, int flags, bool requiresDepth, IMaterialSystem *pMaterialSystem)
{
	ITextureInternal *tex = (ITextureInternal*)pMaterialSystem->FindTexture(name, TEXTURE_GROUP_RENDER_TARGET, false);

	if (tex->GetActualWidth() != desiredWidth || tex->GetActualHeight() != desiredHeight)
	{
		char tempName[256];
		sprintf(tempName, "%s_temp", name);
		ITextureInternal *replacement = (ITextureInternal*)pMaterialSystem->CreateNamedRenderTargetTextureEx2(
			tempName,
			desiredWidth,
			desiredHeight,
			RT_SIZE_LITERAL,
			tex->GetImageFormat(),
			// FIXME: probably need separate depth textures?! otherwise it's using the window's depth buffer
			requiresDepth ? MATERIAL_RT_DEPTH_SEPARATE : MATERIAL_RT_DEPTH_NONE,
			TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
			flags);

		int origRefCount = tex->GetReferenceCount();

		replacement->SwapContents(tex);
		// swapping contents also swaps the reference counts, we need to fix that after the fact
		while (tex->GetReferenceCount() < origRefCount)
			tex->IncrementReferenceCount();
		while (tex->GetReferenceCount() > origRefCount)
			tex->DecrementReferenceCount();

		while (replacement->GetReferenceCount() > 0)
			replacement->DecrementReferenceCount();
		replacement->DeleteIfUnreferenced();

		Log("Replaced base game render target %s\n", name);
	}
}

void CVrRenderTargets::UpdateBaseGameTextures(IMaterialSystem* pMaterialSystem)
{
	int desiredWidth, desiredHeight;
	GetDesiredFullFrameBufferDimensions(desiredWidth, desiredHeight);
	int quarterWidth = desiredWidth / 4;
	int quarterHeight = desiredHeight / 4;

	UpdateBaseGameRenderTexture("_rt_FullFrameFB", desiredWidth, desiredHeight, CREATERENDERTARGETFLAGS_HDR, true, pMaterialSystem);
	UpdateBaseGameRenderTexture("_rt_FullFrameFB1", desiredWidth, desiredHeight, CREATERENDERTARGETFLAGS_HDR, true, pMaterialSystem);
	UpdateBaseGameRenderTexture("_rt_Fullscreen", desiredWidth, desiredHeight, 0, false, pMaterialSystem);
	UpdateBaseGameRenderTexture("_rt_SmallFB0", quarterWidth, quarterHeight, 0, false, pMaterialSystem);
	UpdateBaseGameRenderTexture("_rt_SmallFB1", quarterWidth, quarterHeight, 0, false, pMaterialSystem);
	UpdateBaseGameRenderTexture("_rt_PowerOfTwoFB", 1024, 1024, CREATERENDERTARGETFLAGS_HDR, true, pMaterialSystem);
}


extern ConVar vr_activate_default;

//-----------------------------------------------------------------------------
// Purpose: Called by the engine in material system init and shutdown.
//			Clients should override this in their inherited version, but the base
//			is to init all standard render targets for use.
// Input  : pMaterialSystem - the engine's material system (our singleton is not yet inited at the time this is called)
//			pHardwareConfig - the user hardware config, useful for conditional render target setup
//-----------------------------------------------------------------------------
void CVrRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	if (vr_activate_default.GetBool())
	{
		m_VGuiTexture.Init(CreateVGuiTexture(pMaterialSystem));

		m_currentMsaa = tfvr_msaa.GetInt();
		for (int i = 0; i < VR_NUM_BUFFERS; i++)
		{
			m_VRTwoEyesHMDRenderTargets[i].Init(
				CreateVRTwoEyesHMDRenderTarget(pMaterialSystem, i));
		}

		m_VROneEyeTextureQuarterSize.Init(CreateVROneEyeTextureQuarterSize(pMaterialSystem));

		UpdateBaseGameTextures(pMaterialSystem);

		m_origThreadMode = ConVarRef("mat_queue_mode").GetInt();
		m_changedThreadMode = false;
	}

	// VR hands render target (for sniper scope / isolated hand rendering)
	m_VRHandsRenderTarget.Init( CreateVRHandsRenderTarget( pMaterialSystem ) );

	// Item model panel render targets (war paints, festivized items, etc.)
	{
		extern const char *g_ItemModelPanelRenderTargetNames[];
		extern const char *g_pszModelImagePanelRTName;
		for ( int i = 0; i < ITEM_MODEL_IMAGE_CACHE_SIZE_VR; i++ )
		{
			m_ItemModelPanelRTs[i].Init( pMaterialSystem->CreateNamedRenderTargetTextureEx2(
				g_ItemModelPanelRenderTargetNames[i],
				256, 256, RT_SIZE_DEFAULT,
				pMaterialSystem->GetBackBufferFormat(),
				MATERIAL_RT_DEPTH_SHARED,
				TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
				0 ) );
		}
		m_ModelImagePanelRT.Init( pMaterialSystem->CreateNamedRenderTargetTextureEx2(
			g_pszModelImagePanelRTName,
			256, 256, RT_SIZE_DEFAULT,
			pMaterialSystem->GetBackBufferFormat(),
			MATERIAL_RT_DEPTH_SHARED,
			TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
			0 ) );
	}

	// Water effects
	m_WaterReflectionTexture.Init( CreateWaterReflectionTexture( pMaterialSystem ) );
	m_VRWaterReflectionTexture.Init( CreateVRWaterReflectionTexture( pMaterialSystem ) );
	m_WaterRefractionTexture.Init( CreateWaterRefractionTexture( pMaterialSystem ) );
	m_VRScreenEffectTexture.Init( CreateVRScreenEffectTexture( pMaterialSystem ) );

	// Monitors
	m_CameraTexture.Init( CreateCameraTexture( pMaterialSystem ) );
}

//-----------------------------------------------------------------------------
// Purpose: Shut down each CTextureReference we created in InitClientRenderTargets.
//			Called by the engine in material system shutdown.
// Input  :  - 
//-----------------------------------------------------------------------------
void CVrRenderTargets::ShutdownClientRenderTargets()
{ 
    m_VGuiTexture.Shutdown();
	m_VRHandsRenderTarget.Shutdown();
	m_VRWaterReflectionTexture.Shutdown();
	m_VRScreenEffectTexture.Shutdown();

	for (int i = 0; i < VR_NUM_BUFFERS; i++)
	{
		m_VRTwoEyesHMDRenderTargets[i].Shutdown();
	}

    m_VROneEyeTextureQuarterSize.Shutdown();

	for (int i = 0; i < ITEM_MODEL_IMAGE_CACHE_SIZE_VR; i++)
	{
		m_ItemModelPanelRTs[i].Shutdown();
	}
	m_ModelImagePanelRT.Shutdown();

    // Clean up standard HL2 RTs (camera and water) 
    BaseClass::ShutdownClientRenderTargets();
}
 
//add the interface!
static CVrRenderTargets g_pVrRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CVrRenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_pVrRenderTargets  );
CVrRenderTargets* vrRenderTargets = &g_pVrRenderTargets;