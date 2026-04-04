#ifndef VRRENDERTARGETS_H_
#define VRRENDERTARGETS_H_
#ifdef _WIN32
#pragma once
#endif
 
#include "baseclientrendertargets.h" 
#include "tfvr/stCommon.h"
#include "materialsystem/MaterialSystemUtil.h"

#define ITEM_MODEL_IMAGE_CACHE_SIZE_VR 3

// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;
 
class CVrRenderTargets : public CBaseClientRenderTargets
{ 
	// no networked vars 
	DECLARE_CLASS_GAMEROOT( CVrRenderTargets, CBaseClientRenderTargets );
public: 
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig ) override;
	virtual void ShutdownClientRenderTargets() override;
 
	ITexture* CreateVGuiTexture(IMaterialSystem* pMaterialSystem);
	ITexture* CreateVRTwoEyesHMDRenderTarget(IMaterialSystem* pMaterialSystem, int i);
	ITexture* CreateVROneEyeTextureQuarterSize(IMaterialSystem* pMaterialSystem);

	void UpdateBaseGameRenderTexture(const char *name, int desiredWidth, int desiredHeight, int flags, bool requiresDepth, IMaterialSystem *pMaterialSystem);
	void UpdateBaseGameTextures(IMaterialSystem *pMaterialSystem);

	ITexture* GetVRRenderTarget(int i);
	ITexture* GetVRHandsRenderTarget();
	void UpdateVRRenderTargets();

private:
	CTextureReference		m_VGuiTexture;
	CTextureReference		m_VRTwoEyesHMDRenderTargets[VR_NUM_BUFFERS];
	CTextureReference		m_VROneEyeTextureQuarterSize;
	CTextureReference		m_VRWaterReflectionTexture;
	CTextureReference		m_VRScreenEffectTexture;
	CTextureReference		m_VRHandsRenderTarget;
	CTextureReference		m_ItemModelPanelRTs[ITEM_MODEL_IMAGE_CACHE_SIZE_VR];
	CTextureReference		m_ModelImagePanelRT;
	int						m_currentMsaa;

	int						m_origThreadMode;
	bool					m_changedThreadMode;

	ITexture* CreateWaterReflectionTexture( IMaterialSystem* pMaterialSystem, int iSize = 1024 );
	ITexture* CreateVRWaterReflectionTexture( IMaterialSystem* pMaterialSystem, int iSize = 1024 );
	ITexture* CreateWaterRefractionTexture( IMaterialSystem* pMaterialSystem, int iSize = 1024 );
	ITexture* CreateVRScreenEffectTexture( IMaterialSystem* pMaterialSystem, int iSize = 1024 );
	ITexture* CreateVRHandsRenderTarget( IMaterialSystem* pMaterialSystem );
};
 
extern CVrRenderTargets* vrRenderTargets;
 
#endif //VRRENDERTARGETS_H_