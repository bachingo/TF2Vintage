//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VR Controller Model rendering for game-side display
//
//=============================================================================

#include "cbase.h"
#include "vr_controller_model.h"
#include "openxr_manager.h"
#include "vr_menu_manager.h"
#include "tf_shareddefs.h"
#include "c_tf_player.h"
#include "view.h"
#include "materialsystem/imaterialsystem.h"
#include "ienginevgui.h"
#include "client_virtualreality.h"

// cgltf for parsing GLB data
// Source Engine replaces strncpy with a macro, we need the real one for cgltf
#ifdef strncpy
#undef strncpy
#endif
// Provide strncpy implementation that cgltf needs
inline char* cgltf_strncpy(char* dest, const char* src, size_t n) {
	size_t i;
	for (i = 0; i < n && src[i] != '\0'; i++)
		dest[i] = src[i];
	for (; i < n; i++)
		dest[i] = '\0';
	return dest;
}
#define strncpy cgltf_strncpy
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#undef strncpy

// stb_image for decoding PNG/JPEG textures from GLB
// Must be included in a way that avoids Source Engine macro conflicts
#ifdef ARRAYSIZE
#define SAVED_ARRAYSIZE ARRAYSIZE
#undef ARRAYSIZE
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ASSERT(x) Assert(x)
#include "stb_image.h"

#ifdef SAVED_ARRAYSIZE
#define ARRAYSIZE SAVED_ARRAYSIZE
#undef SAVED_ARRAYSIZE
#endif

#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Procedural texture regenerator for controller textures
//-----------------------------------------------------------------------------
class CControllerTextureRegenerator : public ITextureRegenerator
{
public:
	CControllerTextureRegenerator(uint8_t* pData, int width, int height)
		: m_pData(pData), m_nWidth(width), m_nHeight(height) {}
	
	virtual void RegenerateTextureBits(ITexture* pTexture, IVTFTexture* pVTFTexture, Rect_t* pRect) override
	{
		if (!m_pData || !pVTFTexture)
			return;
		
		unsigned char* pImageData = pVTFTexture->ImageData(0, 0, 0);
		int width = pVTFTexture->Width();
		int height = pVTFTexture->Height();
		
		// Copy RGBA data (may need to flip or convert format)
		for (int y = 0; y < height && y < m_nHeight; y++)
		{
			for (int x = 0; x < width && x < m_nWidth; x++)
			{
				int srcIdx = (y * m_nWidth + x) * 4;
				int dstIdx = (y * width + x) * 4;
				
				// RGBA -> BGRA conversion for Source Engine
				pImageData[dstIdx + 0] = m_pData[srcIdx + 2]; // B
				pImageData[dstIdx + 1] = m_pData[srcIdx + 1]; // G
				pImageData[dstIdx + 2] = m_pData[srcIdx + 0]; // R
				pImageData[dstIdx + 3] = m_pData[srcIdx + 3]; // A
			}
		}
	}
	
	virtual void Release() override { delete this; }
	
private:
	uint8_t* m_pData;
	int m_nWidth;
	int m_nHeight;
};

CVRControllerModelManager* g_pVRControllerModelManager = nullptr;

static ConVar tfvr_controller_models("tfvr_controller_models", "1", FCVAR_ARCHIVE, "Enable VR controller model rendering during preamble/death");

//-----------------------------------------------------------------------------
// Helper: Identity matrix (column-major)
//-----------------------------------------------------------------------------
static void IdentityMatrix(float* m)
{
	memset(m, 0, 16 * sizeof(float));
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

//-----------------------------------------------------------------------------
// Helper: Quaternion to matrix (column-major)
//-----------------------------------------------------------------------------
static void QuaternionToMatrix(float qx, float qy, float qz, float qw, float* m)
{
	float xx = qx * qx, yy = qy * qy, zz = qz * qz;
	float xy = qx * qy, xz = qx * qz, yz = qy * qz;
	float wx = qw * qx, wy = qw * qy, wz = qw * qz;
	
	m[0] = 1 - 2*(yy + zz);  m[4] = 2*(xy - wz);      m[8]  = 2*(xz + wy);      m[12] = 0;
	m[1] = 2*(xy + wz);      m[5] = 1 - 2*(xx + zz);  m[9]  = 2*(yz - wx);      m[13] = 0;
	m[2] = 2*(xz - wy);      m[6] = 2*(yz + wx);      m[10] = 1 - 2*(xx + yy);  m[14] = 0;
	m[3] = 0;                m[7] = 0;                m[11] = 0;                m[15] = 1;
}

//-----------------------------------------------------------------------------
// Helper: Multiply matrices (column-major)
//-----------------------------------------------------------------------------
static void MultiplyMatrices(const float* a, const float* b, float* result)
{
	float temp[16];
	for (int col = 0; col < 4; col++) {
		for (int row = 0; row < 4; row++) {
			temp[col * 4 + row] = 
				a[0 * 4 + row] * b[col * 4 + 0] +
				a[1 * 4 + row] * b[col * 4 + 1] +
				a[2 * 4 + row] * b[col * 4 + 2] +
				a[3 * 4 + row] * b[col * 4 + 3];
		}
	}
	memcpy(result, temp, 16 * sizeof(float));
}

//-----------------------------------------------------------------------------
// GameControllerNode methods
//-----------------------------------------------------------------------------
void GameControllerNode::ComputeLocalMatrix()
{
	float T[16], R[16], S[16], TR[16];
	
	// Translation
	IdentityMatrix(T);
	T[12] = translation[0];
	T[13] = translation[1];
	T[14] = translation[2];
	
	// Rotation (quaternion)
	QuaternionToMatrix(rotation[0], rotation[1], rotation[2], rotation[3], R);
	
	// Scale
	IdentityMatrix(S);
	S[0] = scale[0];
	S[5] = scale[1];
	S[10] = scale[2];
	
	// localMatrix = T * R * S
	MultiplyMatrices(T, R, TR);
	MultiplyMatrices(TR, S, localMatrix);
}

void GameControllerNode::UpdateWorldMatrix(const float* parentWorld)
{
	if (parentWorld) {
		MultiplyMatrices(parentWorld, localMatrix, worldMatrix);
	} else {
		memcpy(worldMatrix, localMatrix, sizeof(float) * 16);
	}
}

//-----------------------------------------------------------------------------
// CVRControllerModelManager
//-----------------------------------------------------------------------------
CVRControllerModelManager::CVRControllerModelManager()
	: m_pOpenXRManager(nullptr)
	, m_bLoaded(false)
	, m_bInitialized(false)
	, m_bTriedLoading(false)
	, m_xrEnumerateInteractionRenderModelIds(nullptr)
	, m_xrCreateRenderModel(nullptr)
	, m_xrDestroyRenderModel(nullptr)
	, m_xrGetRenderModelProperties(nullptr)
	, m_xrGetRenderModelState(nullptr)
	, m_xrCreateRenderModelAsset(nullptr)
	, m_xrDestroyRenderModelAsset(nullptr)
	, m_xrGetRenderModelAssetData(nullptr)
	, m_xrGetRenderModelAssetProperties(nullptr)
	, m_xrEnumerateRenderModelSubactionPaths(nullptr)
	, m_xrCreateRenderModelSpace(nullptr)
	, m_pControllerMaterial(nullptr)
{
}

CVRControllerModelManager::~CVRControllerModelManager()
{
	Shutdown();
}

bool CVRControllerModelManager::Initialize(COpenXRManager* pOpenXRManager)
{
	if (m_bInitialized)
		return true;
	
	m_pOpenXRManager = pOpenXRManager;
	if (!m_pOpenXRManager)
		return false;
	
	XrInstance instance = m_pOpenXRManager->GetInstance();
	if (instance == XR_NULL_HANDLE)
		return false;
	
	// Get render model extension function pointers
	if (XR_FAILED(xrGetInstanceProcAddr(instance, "xrEnumerateInteractionRenderModelIdsEXT", (PFN_xrVoidFunction*)&m_xrEnumerateInteractionRenderModelIds)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrCreateRenderModelEXT", (PFN_xrVoidFunction*)&m_xrCreateRenderModel)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrDestroyRenderModelEXT", (PFN_xrVoidFunction*)&m_xrDestroyRenderModel)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrGetRenderModelPropertiesEXT", (PFN_xrVoidFunction*)&m_xrGetRenderModelProperties)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrGetRenderModelStateEXT", (PFN_xrVoidFunction*)&m_xrGetRenderModelState)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrCreateRenderModelAssetEXT", (PFN_xrVoidFunction*)&m_xrCreateRenderModelAsset)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrDestroyRenderModelAssetEXT", (PFN_xrVoidFunction*)&m_xrDestroyRenderModelAsset)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrGetRenderModelAssetDataEXT", (PFN_xrVoidFunction*)&m_xrGetRenderModelAssetData)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrGetRenderModelAssetPropertiesEXT", (PFN_xrVoidFunction*)&m_xrGetRenderModelAssetProperties)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrEnumerateRenderModelSubactionPathsEXT", (PFN_xrVoidFunction*)&m_xrEnumerateRenderModelSubactionPaths)) ||
		XR_FAILED(xrGetInstanceProcAddr(instance, "xrCreateRenderModelSpaceEXT", (PFN_xrVoidFunction*)&m_xrCreateRenderModelSpace)))
	{
		DevMsg("VRControllerModel: Render model extension not fully available\n");
		return false;
	}
	
	// Create unlit material for controller rendering (opaque)
	KeyValues* pVMTKeyValues = new KeyValues("UnlitGeneric");
	pVMTKeyValues->SetString("$basetexture", "vgui/white");
	pVMTKeyValues->SetInt("$vertexcolor", 1);
	pVMTKeyValues->SetInt("$no_fullbright", 1);
	pVMTKeyValues->SetInt("$ignorez", 0);
	m_pControllerMaterial = materials->CreateMaterial("__vr_controller_model", pVMTKeyValues);
	
	m_bInitialized = true;
	DevMsg("VRControllerModel: Initialized\n");
	
	return true;
}

void CVRControllerModelManager::Shutdown()
{
	CleanupModel(m_leftController);
	CleanupModel(m_rightController);
	
	if (m_pControllerMaterial)
	{
		m_pControllerMaterial->DecrementReferenceCount();
		m_pControllerMaterial = nullptr;
	}
	
	m_bLoaded = false;
	m_bInitialized = false;
	m_bTriedLoading = false;
	m_pOpenXRManager = nullptr;
}

void CVRControllerModelManager::CleanupModel(GameControllerModel& model)
{
	// Clean up per-material textures and materials (but not the fallback m_pControllerMaterial)
	for (int i = 0; i < model.materials.Count(); i++)
	{
		GameControllerMaterial& mat = model.materials[i];
		
		// Only release materials we created, not the shared fallback
		if (mat.pMaterial && mat.pMaterial != m_pControllerMaterial)
		{
			mat.pMaterial->Release();
			mat.pMaterial = nullptr;
		}
		
		if (mat.pTexture)
		{
			mat.pTexture->SetTextureRegenerator(nullptr);
			mat.pTexture->Release();
			mat.pTexture = nullptr;
		}
		
		mat.textureData.clear();
	}
	
	model.meshes.Purge();
	model.nodes.Purge();
	model.materials.Purge();
	model.rootNodes.Purge();
	model.nodeStates.Purge();
	model.animNodeToGltfNode.Purge();
	model.isLoaded = false;
	
	// Clean up OpenXR handles
	if (model.modelSpace != XR_NULL_HANDLE)
	{
		xrDestroySpace(model.modelSpace);
		model.modelSpace = XR_NULL_HANDLE;
	}
	if (model.assetHandle != XR_NULL_HANDLE && m_xrDestroyRenderModelAsset)
	{
		m_xrDestroyRenderModelAsset(model.assetHandle);
		model.assetHandle = XR_NULL_HANDLE;
	}
	if (model.renderModel != XR_NULL_HANDLE && m_xrDestroyRenderModel)
	{
		m_xrDestroyRenderModel(model.renderModel);
		model.renderModel = XR_NULL_HANDLE;
	}
}

bool CVRControllerModelManager::ShouldShowControllers() const
{
	if (!tfvr_controller_models.GetBool())
		return false;
	
	// Hide when SteamVR overlay is open (SteamVR renders its own controllers)
	if (m_pOpenXRManager && !m_pOpenXRManager->IsSessionFocused())
		return false;
	
	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pLocalPlayer)
		return false;
	
	// Show during preamble (no class selected)
	if (pLocalPlayer->GetPlayerClass() && 
		pLocalPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED)
	{
		return true;
	}
	
	// Show when dead
	if (pLocalPlayer->IsPlayerDead())
	{
		return true;
	}
	
	// Show when any VR menu is visible (includes class select, team select, pause menu, etc.)
	if (g_pVRMenuManager && g_pVRMenuManager->IsMenuVisible())
	{
		return true;
	}
	
	return false;
}

bool CVRControllerModelManager::LoadControllerModels()
{
	if (m_bLoaded || m_bTriedLoading)
		return m_bLoaded;
	
	m_bTriedLoading = true;
	
	if (!m_bInitialized || !m_pOpenXRManager)
		return false;
	
	XrSession session = m_pOpenXRManager->GetSession();
	if (session == XR_NULL_HANDLE)
		return false;
	
	// Enumerate render model IDs
	XrInteractionRenderModelIdsEnumerateInfoEXT enumInfo = {XR_TYPE_INTERACTION_RENDER_MODEL_IDS_ENUMERATE_INFO_EXT};
	
	uint32_t modelCount = 0;
	XrResult result = m_xrEnumerateInteractionRenderModelIds(session, &enumInfo, 0, &modelCount, nullptr);
	if (XR_FAILED(result) || modelCount == 0)
	{
		DevMsg("VRControllerModel: No render models available\n");
		m_bTriedLoading = false;  // Allow retry
		return false;
	}
	
	CUtlVector<XrRenderModelIdEXT> modelIds;
	modelIds.SetCount(modelCount);
	result = m_xrEnumerateInteractionRenderModelIds(session, &enumInfo, modelCount, &modelCount, modelIds.Base());
	if (XR_FAILED(result))
	{
		DevMsg("VRControllerModel: Failed to enumerate render models\n");
		return false;
	}
	
	DevMsg("VRControllerModel: Found %d render models\n", modelCount);
	
	XrInstance instance = m_pOpenXRManager->GetInstance();
	
	// Load each model
	for (uint32_t i = 0; i < modelCount && i < 4; i++)
	{
		// Create temporary render model to query hand
		XrRenderModelEXT tempRenderModel = XR_NULL_HANDLE;
		XrRenderModelCreateInfoEXT createInfo = {XR_TYPE_RENDER_MODEL_CREATE_INFO_EXT};
		createInfo.renderModelId = modelIds[i];
		createInfo.gltfExtensionCount = 0;
		createInfo.gltfExtensions = nullptr;
		
		result = m_xrCreateRenderModel(session, &createInfo, &tempRenderModel);
		if (XR_FAILED(result))
		{
			DevMsg("VRControllerModel: Failed to create render model %d\n", i);
			continue;
		}
		
		// Detect hand via subaction paths
		bool isLeftHand = false;
		bool isRightHand = false;
		
		XrInteractionRenderModelSubactionPathInfoEXT subactionInfo = {XR_TYPE_INTERACTION_RENDER_MODEL_SUBACTION_PATH_INFO_EXT};
		uint32_t pathCount = 0;
		
		if (XR_SUCCEEDED(m_xrEnumerateRenderModelSubactionPaths(tempRenderModel, &subactionInfo, 0, &pathCount, nullptr)) && pathCount > 0)
		{
			CUtlVector<XrPath> paths;
			paths.SetCount(pathCount);
			if (XR_SUCCEEDED(m_xrEnumerateRenderModelSubactionPaths(tempRenderModel, &subactionInfo, pathCount, &pathCount, paths.Base())))
			{
				for (uint32_t p = 0; p < pathCount; p++)
				{
					char pathStr[256] = {0};
					uint32_t pathLen = 0;
					if (XR_SUCCEEDED(xrPathToString(instance, paths[p], sizeof(pathStr), &pathLen, pathStr)))
					{
						if (V_strstr(pathStr, "/user/hand/left"))
							isLeftHand = true;
						else if (V_strstr(pathStr, "/user/hand/right"))
							isRightHand = true;
					}
				}
			}
		}
		
		// Fallback if hand detection failed
		if (!isLeftHand && !isRightHand)
		{
			if (!m_leftController.isLoaded)
				isLeftHand = true;
			else if (!m_rightController.isLoaded)
				isRightHand = true;
		}
		
		// Assign to controller
		GameControllerModel* targetModelPtr = nullptr;
		if (isLeftHand && !m_leftController.isLoaded)
			targetModelPtr = &m_leftController;
		else if (isRightHand && !m_rightController.isLoaded)
			targetModelPtr = &m_rightController;
		else
		{
			m_xrDestroyRenderModel(tempRenderModel);
			continue;
		}
		
		GameControllerModel& model = *targetModelPtr;
		model.renderModel = tempRenderModel;
		model.renderModelId = modelIds[i];
		
		// Get properties
		XrRenderModelPropertiesGetInfoEXT propsGetInfo = {XR_TYPE_RENDER_MODEL_PROPERTIES_GET_INFO_EXT};
		XrRenderModelPropertiesEXT props = {XR_TYPE_RENDER_MODEL_PROPERTIES_EXT};
		
		result = m_xrGetRenderModelProperties(model.renderModel, &propsGetInfo, &props);
		if (XR_FAILED(result))
		{
			DevMsg("VRControllerModel: Failed to get properties for model %d\n", i);
			continue;
		}
		
		model.cacheId = props.cacheId;
		model.animatableNodeCount = props.animatableNodeCount;
		model.nodeStates.SetCount(props.animatableNodeCount);
		
		// Create asset
		XrRenderModelAssetCreateInfoEXT assetCreateInfo = {XR_TYPE_RENDER_MODEL_ASSET_CREATE_INFO_EXT};
		assetCreateInfo.cacheId = props.cacheId;
		
		result = m_xrCreateRenderModelAsset(session, &assetCreateInfo, &model.assetHandle);
		if (XR_FAILED(result))
		{
			DevMsg("VRControllerModel: Failed to create asset for model %d\n", i);
			continue;
		}
		
		// Get GLB data size
		XrRenderModelAssetDataGetInfoEXT dataGetInfo = {XR_TYPE_RENDER_MODEL_ASSET_DATA_GET_INFO_EXT};
		XrRenderModelAssetDataEXT assetData = {XR_TYPE_RENDER_MODEL_ASSET_DATA_EXT};
		assetData.bufferCapacityInput = 0;
		assetData.buffer = nullptr;
		
		result = m_xrGetRenderModelAssetData(model.assetHandle, &dataGetInfo, &assetData);
		if (XR_FAILED(result))
		{
			DevMsg("VRControllerModel: Failed to get asset data size for model %d\n", i);
			continue;
		}
		
		// Get actual data
		CUtlVector<uint8_t> glbData;
		glbData.SetCount(assetData.bufferCountOutput);
		assetData.bufferCapacityInput = glbData.Count();
		assetData.buffer = glbData.Base();
		
		result = m_xrGetRenderModelAssetData(model.assetHandle, &dataGetInfo, &assetData);
		if (XR_FAILED(result))
		{
			DevMsg("VRControllerModel: Failed to get asset data for model %d\n", i);
			continue;
		}
		
		DevMsg("VRControllerModel: Got %d bytes of GLB data for model %d\n", assetData.bufferCountOutput, i);
		
		// Parse GLB
		if (!ParseGLBData(glbData.Base(), glbData.Count(), model))
		{
			DevMsg("VRControllerModel: Failed to parse GLB for model %d\n", i);
			continue;
		}
		
		// Build animation node mapping
		if (model.animatableNodeCount > 0)
		{
			CUtlVector<XrRenderModelAssetNodePropertiesEXT> nodeProps;
			nodeProps.SetCount(model.animatableNodeCount);
			
			XrRenderModelAssetPropertiesEXT assetProps = {XR_TYPE_RENDER_MODEL_ASSET_PROPERTIES_EXT};
			assetProps.nodePropertyCount = model.animatableNodeCount;
			assetProps.nodeProperties = nodeProps.Base();
			
			XrRenderModelAssetPropertiesGetInfoEXT assetPropsGetInfo = {XR_TYPE_RENDER_MODEL_ASSET_PROPERTIES_GET_INFO_EXT};
			
			if (XR_SUCCEEDED(m_xrGetRenderModelAssetProperties(model.assetHandle, &assetPropsGetInfo, &assetProps)))
			{
				model.animNodeToGltfNode.SetCount(model.animatableNodeCount);
				for (uint32_t n = 0; n < model.animatableNodeCount; n++)
					model.animNodeToGltfNode[n] = -1;
				
				for (uint32_t animIdx = 0; animIdx < assetProps.nodePropertyCount; animIdx++)
				{
					std::string animNodeName(nodeProps[animIdx].uniqueName);
					
					for (int gltfIdx = 0; gltfIdx < model.nodes.Count(); gltfIdx++)
					{
						if (model.nodes[gltfIdx].name == animNodeName)
						{
							model.animNodeToGltfNode[animIdx] = gltfIdx;
							break;
						}
					}
				}
			}
		}
		
		// Create a space for locating this render model
		XrRenderModelSpaceCreateInfoEXT spaceCreateInfo = {XR_TYPE_RENDER_MODEL_SPACE_CREATE_INFO_EXT};
		spaceCreateInfo.renderModel = model.renderModel;
		
		result = m_xrCreateRenderModelSpace(session, &spaceCreateInfo, &model.modelSpace);
		if (XR_FAILED(result))
		{
			DevMsg("VRControllerModel: Failed to create model space for model %d (non-fatal)\n", i);
			model.modelSpace = XR_NULL_HANDLE;
		}
		
		// Create Source Engine materials for each texture
		CreateMaterialsForModel(model, isLeftHand ? "left" : "right");
		
		model.isLoaded = true;
		DevMsg("VRControllerModel: Loaded %s controller with %d meshes, %d nodes, %d materials\n",
			isLeftHand ? "left" : "right", model.meshes.Count(), model.nodes.Count(), model.materials.Count());
	}
	
	m_bLoaded = m_leftController.isLoaded || m_rightController.isLoaded;
	return m_bLoaded;
}

void CVRControllerModelManager::CreateMaterialsForModel(GameControllerModel& model, const char* handName)
{
	for (int i = 0; i < model.materials.Count(); i++)
	{
		GameControllerMaterial& mat = model.materials[i];
		
		if (mat.hasTexture && mat.textureData.size() > 0)
		{
			// Create a unique texture name
			char textureName[128];
			V_snprintf(textureName, sizeof(textureName), "__vr_controller_%s_mat%d_tex", handName, i);
			
			// Create procedural texture
			mat.pTexture = materials->CreateProceduralTexture(
				textureName,
				TEXTURE_GROUP_VGUI,
				mat.textureWidth,
				mat.textureHeight,
				IMAGE_FORMAT_BGRA8888,
				TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD
			);
			
			if (mat.pTexture)
			{
				// Set up regenerator to copy our texture data
				CControllerTextureRegenerator* pRegen = new CControllerTextureRegenerator(
					mat.textureData.data(), mat.textureWidth, mat.textureHeight);
				mat.pTexture->SetTextureRegenerator(pRegen);
				mat.pTexture->Download();
				
				// Create material using this texture
				char materialName[128];
				V_snprintf(materialName, sizeof(materialName), "__vr_controller_%s_mat%d", handName, i);
				
				KeyValues* pVMTKeyValues = new KeyValues("UnlitGeneric");
				pVMTKeyValues->SetString("$basetexture", textureName);
				pVMTKeyValues->SetInt("$vertexcolor", 1);
				pVMTKeyValues->SetInt("$no_fullbright", 1);
				pVMTKeyValues->SetInt("$ignorez", 0);
				mat.pMaterial = materials->CreateMaterial(materialName, pVMTKeyValues);
				
				DevMsg("VRControllerModel: Created textured material %s (%dx%d)\n", 
					materialName, mat.textureWidth, mat.textureHeight);
			}
		}
		
		// If no texture or creation failed, material will use fallback
		if (!mat.pMaterial)
		{
			mat.pMaterial = m_pControllerMaterial;
		}
	}
}

bool CVRControllerModelManager::ParseGLBData(const uint8_t* data, size_t size, GameControllerModel& model)
{
	cgltf_options options = {};
	cgltf_data* gltf = nullptr;
	
	cgltf_result result = cgltf_parse(&options, data, size, &gltf);
	if (result != cgltf_result_success)
	{
		DevMsg("VRControllerModel: Failed to parse GLB\n");
		return false;
	}
	
	result = cgltf_load_buffers(&options, gltf, nullptr);
	if (result != cgltf_result_success)
	{
		cgltf_free(gltf);
		return false;
	}
	
	// Parse materials and extract textures
	for (size_t i = 0; i < gltf->materials_count; i++)
	{
		cgltf_material* mat = &gltf->materials[i];
		GameControllerMaterial matData;
		
		if (mat->has_pbr_metallic_roughness)
		{
			memcpy(matData.baseColorFactor, mat->pbr_metallic_roughness.base_color_factor, sizeof(float) * 4);
			
			// Extract base color texture if present
			cgltf_texture* tex = mat->pbr_metallic_roughness.base_color_texture.texture;
			if (tex && tex->image && tex->image->buffer_view)
			{
				cgltf_buffer_view* bufView = tex->image->buffer_view;
				const uint8_t* imageData = static_cast<const uint8_t*>(bufView->buffer->data) + bufView->offset;
				size_t imageSize = bufView->size;
				
				// Decode image using stb_image
				int width, height, channels;
				uint8_t* pixels = stbi_load_from_memory(imageData, (int)imageSize, &width, &height, &channels, 4);
				
				if (pixels)
				{
					matData.hasTexture = true;
					matData.textureWidth = width;
					matData.textureHeight = height;
					matData.textureData.resize(width * height * 4);
					memcpy(matData.textureData.data(), pixels, width * height * 4);
					stbi_image_free(pixels);
					
					DevMsg("VRControllerModel: Loaded texture %dx%d for material %d\n", width, height, (int)i);
				}
			}
		}
		
		memcpy(matData.emissiveFactor, mat->emissive_factor, sizeof(float) * 3);
		
		model.materials.AddToTail(matData);
	}
	
	// Parse nodes
	for (size_t i = 0; i < gltf->nodes_count; i++)
	{
		cgltf_node* node = &gltf->nodes[i];
		GameControllerNode nodeData;
		
		if (node->name)
			nodeData.name = node->name;
		nodeData.parent = -1;
		nodeData.meshIndex = node->mesh ? (int)(node->mesh - gltf->meshes) : -1;
		
		if (node->has_translation)
			memcpy(nodeData.translation, node->translation, sizeof(float) * 3);
		if (node->has_rotation)
			memcpy(nodeData.rotation, node->rotation, sizeof(float) * 4);
		if (node->has_scale)
			memcpy(nodeData.scale, node->scale, sizeof(float) * 3);
		
		if (node->has_matrix)
			memcpy(nodeData.localMatrix, node->matrix, sizeof(float) * 16);
		else
			nodeData.ComputeLocalMatrix();
		
		IdentityMatrix(nodeData.worldMatrix);
		
		model.nodes.AddToTail(nodeData);
	}
	
	// Parent-child relationships
	for (size_t i = 0; i < gltf->nodes_count; i++)
	{
		cgltf_node* node = &gltf->nodes[i];
		for (size_t c = 0; c < node->children_count; c++)
		{
			int childIdx = (int)(node->children[c] - gltf->nodes);
			model.nodes[i].children.push_back(childIdx);
			model.nodes[childIdx].parent = i;
		}
	}
	
	// Find root nodes
	for (int i = 0; i < model.nodes.Count(); i++)
	{
		if (model.nodes[i].parent < 0)
			model.rootNodes.AddToTail(i);
	}
	
	// Parse meshes
	for (size_t i = 0; i < gltf->meshes_count; i++)
	{
		cgltf_mesh* mesh = &gltf->meshes[i];
		
		for (size_t p = 0; p < mesh->primitives_count; p++)
		{
			cgltf_primitive* prim = &mesh->primitives[p];
			GameControllerMesh meshData;
			
			meshData.materialIndex = prim->material ? (int)(prim->material - gltf->materials) : -1;
			
			for (size_t a = 0; a < prim->attributes_count; a++)
			{
				cgltf_attribute* attr = &prim->attributes[a];
				cgltf_accessor* accessor = attr->data;
				
				if (attr->type == cgltf_attribute_type_position)
				{
					meshData.positions.resize(accessor->count);
					for (size_t v = 0; v < accessor->count; v++)
					{
						float pos[3];
						cgltf_accessor_read_float(accessor, v, pos, 3);
						meshData.positions[v].Init(pos[0], pos[1], pos[2]);
					}
				}
				else if (attr->type == cgltf_attribute_type_normal)
				{
					meshData.normals.resize(accessor->count);
					for (size_t v = 0; v < accessor->count; v++)
					{
						float norm[3];
						cgltf_accessor_read_float(accessor, v, norm, 3);
						meshData.normals[v].Init(norm[0], norm[1], norm[2]);
					}
				}
				else if (attr->type == cgltf_attribute_type_texcoord)
				{
					meshData.texCoords.resize(accessor->count);
					for (size_t v = 0; v < accessor->count; v++)
					{
						float uv[2];
						cgltf_accessor_read_float(accessor, v, uv, 2);
						meshData.texCoords[v].Init(uv[0], uv[1]);
					}
				}
			}
			
			if (prim->indices)
			{
				meshData.indices.resize(prim->indices->count);
				for (size_t idx = 0; idx < prim->indices->count; idx++)
					meshData.indices[idx] = (unsigned int)cgltf_accessor_read_index(prim->indices, idx);
			}
			
			model.meshes.AddToTail(meshData);
		}
	}
	
	cgltf_free(gltf);
	return true;
}

void CVRControllerModelManager::Update(float frametime)
{
	if (!m_bInitialized)
		return;
	
	// Try loading if not loaded
	if (!m_bLoaded)
		LoadControllerModels();
	
	if (!m_bLoaded)
		return;
	
	UpdateControllerPoses();
	UpdateAnimationState();
}

void CVRControllerModelManager::UpdateControllerPoses()
{
	if (!m_pOpenXRManager)
		return;
	
	// Get the display time from DXVK layer (same as input/hand tracking does)
	extern void dxvkGetPredictedDisplayTime(XrTime& time);
	XrTime displayTime = 0;
	dxvkGetPredictedDisplayTime(displayTime);
	
	if (displayTime == 0)
		return;  // No valid frame time available
	
	XrSpace refSpace = m_pOpenXRManager->GetReferenceSpace();
	
	auto updatePose = [&](GameControllerModel& model, bool isLeft) {
		// Try to get pose from model space
		if (model.modelSpace != XR_NULL_HANDLE && refSpace != XR_NULL_HANDLE)
		{
			XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};
			XrResult result = xrLocateSpace(model.modelSpace, refSpace, displayTime, &location);
			
			if (XR_SUCCEEDED(result) && (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
			{
				// Store the raw OpenXR pose - we'll apply it in the render function
				model.currentPoseXR = location.pose;
				model.hasRawXRPose = true;
				
				// Also compute Source world pose for compatibility
				float worldScale = m_pOpenXRManager->GetWorldScale();
				Vector sourcePos;
				sourcePos.x = -location.pose.position.z * worldScale;
				sourcePos.y = -location.pose.position.x * worldScale;
				sourcePos.z = location.pose.position.y * worldScale;
				
				Quaternion xrQuat(location.pose.orientation.x, location.pose.orientation.y, 
				                  location.pose.orientation.z, location.pose.orientation.w);
				Quaternion sourceQuat(-xrQuat.z, -xrQuat.x, xrQuat.y, xrQuat.w);
				
				matrix3x4_t mat;
				QuaternionMatrix(sourceQuat, sourcePos, mat);
				VMatrix poseInPlayspace;
				poseInPlayspace.CopyFrom3x4(mat);
				
				extern CClientVirtualReality g_ClientVirtualReality;
				C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
				if (pPlayer)
				{
					VMatrix headInPlayspace = m_pOpenXRManager->GetMideyePose();
					VMatrix poseRelativeToHead = headInPlayspace.InverseTR() * poseInPlayspace;
					VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
					model.currentPose = smoothedHeadWorld * poseRelativeToHead;
				}
				else
				{
					model.currentPose = poseInPlayspace;
				}
				
				model.poseValid = true;
				model.isVisible = (location.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) != 0;
				return;
			}
		}
		
		// Fallback: use grip pose when model space is not available
		// Note: In fallback mode, currentPoseXR is identity, so the render path
		// uses currentPose (pre-converted Source pose) instead
		if (isLeft)
			model.poseValid = m_pOpenXRManager->GetLeftControllerGripPose(model.currentPose);
		else
			model.poseValid = m_pOpenXRManager->GetRightControllerGripPose(model.currentPose);
		
		// Set identity for XR pose in fallback case (grip pose is already converted)
		model.currentPoseXR.orientation = {0, 0, 0, 1};
		model.currentPoseXR.position = {0, 0, 0};
		model.hasRawXRPose = false;
	};
	
	updatePose(m_leftController, true);
	updatePose(m_rightController, false);
}

void CVRControllerModelManager::UpdateAnimationState()
{
	if (!m_xrGetRenderModelState)
		return;
	
	XrTime displayTime = m_pOpenXRManager->GetFrameState().predictedDisplayTime;
	
	auto updateModel = [&](GameControllerModel& model) {
		if (!model.isLoaded || model.renderModel == XR_NULL_HANDLE)
			return;
		
		XrRenderModelStateGetInfoEXT stateGetInfo = {XR_TYPE_RENDER_MODEL_STATE_GET_INFO_EXT};
		stateGetInfo.displayTime = displayTime;
		
		XrRenderModelStateEXT state = {XR_TYPE_RENDER_MODEL_STATE_EXT};
		state.nodeStateCount = model.nodeStates.Count();
		state.nodeStates = model.nodeStates.Base();
		
		XrResult result = m_xrGetRenderModelState(model.renderModel, &stateGetInfo, &state);
		if (XR_SUCCEEDED(result))
		{
			for (uint32_t animIdx = 0; animIdx < state.nodeStateCount && animIdx < (uint32_t)model.animNodeToGltfNode.Count(); animIdx++)
			{
				int gltfIdx = model.animNodeToGltfNode[animIdx];
				if (gltfIdx < 0 || gltfIdx >= model.nodes.Count())
					continue;
				
				GameControllerNode& node = model.nodes[gltfIdx];
				const XrRenderModelNodeStateEXT& animState = model.nodeStates[animIdx];
				
				node.isVisible = animState.isVisible;
				
				const XrPosef& pose = animState.nodePose;
				QuaternionToMatrix(pose.orientation.x, pose.orientation.y, 
								   pose.orientation.z, pose.orientation.w, 
								   node.localMatrix);
				node.localMatrix[12] = pose.position.x;
				node.localMatrix[13] = pose.position.y;
				node.localMatrix[14] = pose.position.z;
			}
			
			// Update world matrices
			for (int r = 0; r < model.rootNodes.Count(); r++)
			{
				int rootIdx = model.rootNodes[r];
				model.nodes[rootIdx].UpdateWorldMatrix(nullptr);
				
				// Update children recursively
				CUtlVector<int> stack;
				stack.AddToTail(rootIdx);
				while (stack.Count() > 0)
				{
					int nodeIdx = stack[stack.Count() - 1];
					stack.Remove(stack.Count() - 1);
					
					for (size_t c = 0; c < model.nodes[nodeIdx].children.size(); c++)
					{
						int childIdx = model.nodes[nodeIdx].children[c];
						model.nodes[childIdx].UpdateWorldMatrix(model.nodes[nodeIdx].worldMatrix);
						stack.AddToTail(childIdx);
					}
				}
			}
		}
	};
	
	updateModel(m_leftController);
	updateModel(m_rightController);
}

void CVRControllerModelManager::Render()
{
	if (!ShouldShowControllers())
		return;
	
	if (!m_bLoaded || !m_pControllerMaterial)
		return;
	
	// Update poses right before rendering to minimize latency
	// This ensures the controller position matches the current frame's head position
	UpdateControllerPoses();
	
	if (m_leftController.isLoaded && m_leftController.poseValid)
		RenderController(m_leftController, true);
	
	if (m_rightController.isLoaded && m_rightController.poseValid)
		RenderController(m_rightController, false);
}

void CVRControllerModelManager::TransformControllerVertex(
	const Vector& localPosXR, const matrix3x4_t& nodeMatrix3x4, const GameControllerModel& model,
	float r00, float r01, float r02, float r10, float r11, float r12,
	float r20, float r21, float r22,
	const XrPosef& xrPose, float worldScale,
	const VMatrix& headInverse, const VMatrix& smoothedHeadWorld,
	Vector& worldPos)
{
	Vector modelPosXR;
	VectorTransform(localPosXR, nodeMatrix3x4, modelPosXR);
	
	if (model.hasRawXRPose)
	{
		Vector rotatedXR;
		rotatedXR.x = r00 * modelPosXR.x + r01 * modelPosXR.y + r02 * modelPosXR.z;
		rotatedXR.y = r10 * modelPosXR.x + r11 * modelPosXR.y + r12 * modelPosXR.z;
		rotatedXR.z = r20 * modelPosXR.x + r21 * modelPosXR.y + r22 * modelPosXR.z;
		
		Vector playspacePosXR;
		playspacePosXR.x = rotatedXR.x + xrPose.position.x;
		playspacePosXR.y = rotatedXR.y + xrPose.position.y;
		playspacePosXR.z = rotatedXR.z + xrPose.position.z;
		
		Vector playspacePosSource;
		playspacePosSource.x = -playspacePosXR.z;
		playspacePosSource.y = -playspacePosXR.x;
		playspacePosSource.z = playspacePosXR.y;
		
		playspacePosSource *= worldScale;
		
		Vector posRelativeToHead = headInverse.VMul4x3(playspacePosSource);
		worldPos = smoothedHeadWorld.VMul4x3(posRelativeToHead);
	}
	else
	{
		Vector modelPosSource;
		modelPosSource.x = -modelPosXR.z;
		modelPosSource.y = -modelPosXR.x;
		modelPosSource.z = modelPosXR.y;
		
		modelPosSource *= worldScale;
		
		worldPos = model.currentPose.VMul4x3(modelPosSource);
	}
}

void CVRControllerModelManager::RenderController(const GameControllerModel& model, bool isLeft)
{
	CMatRenderContextPtr pRenderContext(materials);
	
	// Get dynamic world scale (accounts for merc height differences)
	float worldScale = m_pOpenXRManager ? m_pOpenXRManager->GetWorldScale() : 48.0f;
	
	// Enable depth testing for proper self-occlusion
	pRenderContext->OverrideDepthEnable(false, true);
	
	// Pre-compute transforms that are constant for all vertices
	extern CClientVirtualReality g_ClientVirtualReality;
	VMatrix headInPlayspace = m_pOpenXRManager->GetMideyePose();
	VMatrix headInverse = headInPlayspace.InverseTR();
	VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
	
	// Build OpenXR pose rotation matrix (constant for all vertices)
	const XrPosef& xrPose = model.currentPoseXR;
	float qx = xrPose.orientation.x;
	float qy = xrPose.orientation.y;
	float qz = xrPose.orientation.z;
	float qw = xrPose.orientation.w;
	
	// Pre-compute quaternion rotation terms
	float xx = qx * qx, yy = qy * qy, zz = qz * qz;
	float xy = qx * qy, xz = qx * qz, yz = qy * qz;
	float wx = qw * qx, wy = qw * qy, wz = qw * qz;
	
	// Build 3x3 rotation matrix from quaternion
	float r00 = 1.0f - 2.0f * (yy + zz);
	float r01 = 2.0f * (xy - wz);
	float r02 = 2.0f * (xz + wy);
	float r10 = 2.0f * (xy + wz);
	float r11 = 1.0f - 2.0f * (xx + zz);
	float r12 = 2.0f * (yz - wx);
	float r20 = 2.0f * (xz - wy);
	float r21 = 2.0f * (yz + wx);
	float r22 = 1.0f - 2.0f * (xx + yy);
	
	// Dynamic vertex buffer limit is 32768; use non-indexed batches for large meshes
	static const int MAX_VERTS_PER_BATCH = 30000;
	static const int MAX_TRIS_PER_BATCH = MAX_VERTS_PER_BATCH / 3;
	
	for (int nodeIdx = 0; nodeIdx < model.nodes.Count(); nodeIdx++)
	{
		const GameControllerNode& node = model.nodes[nodeIdx];
		if (node.meshIndex < 0 || !node.isVisible)
			continue;
		
		const GameControllerMesh& mesh = model.meshes[node.meshIndex];
		if (mesh.positions.size() == 0 || mesh.indices.size() == 0)
			continue;
		
		// Get material and color
		IMaterial* pMaterialToUse = m_pControllerMaterial;
		float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
		
		if (mesh.materialIndex >= 0 && mesh.materialIndex < model.materials.Count())
		{
			const GameControllerMaterial& mat = model.materials[mesh.materialIndex];
			
			// Use per-material material if available (has texture)
			if (mat.pMaterial)
				pMaterialToUse = mat.pMaterial;
			
			// Apply base color factor (multiplied with texture or used directly)
			r = mat.baseColorFactor[0];
			g = mat.baseColorFactor[1];
			b = mat.baseColorFactor[2];
			a = mat.baseColorFactor[3];
		}
		
		// Build node transform matrix from the column-major worldMatrix (OpenXR/GLTF space)
		matrix3x4_t nodeMatrix3x4;
		nodeMatrix3x4[0][0] = node.worldMatrix[0];  nodeMatrix3x4[0][1] = node.worldMatrix[4];  nodeMatrix3x4[0][2] = node.worldMatrix[8];   nodeMatrix3x4[0][3] = node.worldMatrix[12];
		nodeMatrix3x4[1][0] = node.worldMatrix[1];  nodeMatrix3x4[1][1] = node.worldMatrix[5];  nodeMatrix3x4[1][2] = node.worldMatrix[9];   nodeMatrix3x4[1][3] = node.worldMatrix[13];
		nodeMatrix3x4[2][0] = node.worldMatrix[2];  nodeMatrix3x4[2][1] = node.worldMatrix[6];  nodeMatrix3x4[2][2] = node.worldMatrix[10];  nodeMatrix3x4[2][3] = node.worldMatrix[14];
		
		pRenderContext->Bind(pMaterialToUse);
		
		int numVerts = (int)mesh.positions.size();
		int numIndices = (int)mesh.indices.size();
		int totalTris = numIndices / 3;
		
		if (numVerts <= MAX_VERTS_PER_BATCH && numIndices <= INDEX_BUFFER_SIZE)
		{
			// Fast path: mesh fits in a single indexed batch
			IMesh* pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, pMaterialToUse);
			
			CMeshBuilder meshBuilder;
			meshBuilder.Begin(pMesh, MATERIAL_TRIANGLES, numVerts, numIndices);
			
			for (int vi = 0; vi < numVerts; vi++)
			{
				Vector worldPos;
				TransformControllerVertex(mesh.positions[vi], nodeMatrix3x4, model,
					r00, r01, r02, r10, r11, r12, r20, r21, r22,
					xrPose, worldScale, headInverse, smoothedHeadWorld, worldPos);
				
				meshBuilder.Position3fv(worldPos.Base());
				meshBuilder.Color4f(r, g, b, a);
				
				if (vi < (int)mesh.texCoords.size())
					meshBuilder.TexCoord2fv(0, &mesh.texCoords[vi].x);
				else
					meshBuilder.TexCoord2f(0, 0, 0);
				
				meshBuilder.AdvanceVertex();
			}
			
			for (int tri = 0; tri < totalTris; tri++)
			{
				meshBuilder.Index(mesh.indices[tri * 3 + 0]);
				meshBuilder.AdvanceIndex();
				meshBuilder.Index(mesh.indices[tri * 3 + 2]);
				meshBuilder.AdvanceIndex();
				meshBuilder.Index(mesh.indices[tri * 3 + 1]);
				meshBuilder.AdvanceIndex();
			}
			
			meshBuilder.End();
			pMesh->Draw();
		}
		else
		{
			// Batched path: mesh exceeds vertex buffer limit, render as
			// non-indexed triangles in batches that fit within the limit.
			int triOffset = 0;
			while (triOffset < totalTris)
			{
				int trisThisBatch = MIN(totalTris - triOffset, MAX_TRIS_PER_BATCH);
				int vertsThisBatch = trisThisBatch * 3;
				
				IMesh* pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, pMaterialToUse);
				
				CMeshBuilder meshBuilder;
				meshBuilder.Begin(pMesh, MATERIAL_TRIANGLES, vertsThisBatch);
				
				for (int tri = 0; tri < trisThisBatch; tri++)
				{
					int globalTri = triOffset + tri;
					// Reversed winding order for coordinate system flip
					int indices[3] = {
						(int)mesh.indices[globalTri * 3 + 0],
						(int)mesh.indices[globalTri * 3 + 2],
						(int)mesh.indices[globalTri * 3 + 1]
					};
					
					for (int v = 0; v < 3; v++)
					{
						int vi = indices[v];
						
						Vector worldPos;
						TransformControllerVertex(mesh.positions[vi], nodeMatrix3x4, model,
							r00, r01, r02, r10, r11, r12, r20, r21, r22,
							xrPose, worldScale, headInverse, smoothedHeadWorld, worldPos);
						
						meshBuilder.Position3fv(worldPos.Base());
						meshBuilder.Color4f(r, g, b, a);
						
						if (vi < (int)mesh.texCoords.size())
							meshBuilder.TexCoord2fv(0, &mesh.texCoords[vi].x);
						else
							meshBuilder.TexCoord2f(0, 0, 0);
						
						meshBuilder.AdvanceVertex();
					}
				}
				
				meshBuilder.End();
				pMesh->Draw();
				
				triOffset += trisThisBatch;
			}
		}
	}
	
	// Restore default depth test behavior
	pRenderContext->OverrideDepthEnable(false, true);
}