#ifndef VR_CONTROLLER_MODEL_H
#define VR_CONTROLLER_MODEL_H

#include "cbase.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "../public/openxr/openxr.h"
#include <vector>
#include <string>

// Forward declarations
class COpenXRManager;

// Mesh data for a single controller mesh primitive
struct GameControllerMesh
{
	std::vector<Vector> positions;
	std::vector<Vector> normals;
	std::vector<Vector2D> texCoords;
	std::vector<unsigned int> indices;
	int materialIndex;
	
	GameControllerMesh() : materialIndex(-1) {}
};

// Node in the controller model hierarchy
struct GameControllerNode
{
	std::string name;
	int parent;
	int meshIndex;  // -1 if no mesh
	float localMatrix[16];
	float worldMatrix[16];
	float translation[3];
	float rotation[4];  // quaternion xyzw
	float scale[3];
	bool isVisible;
	std::vector<int> children;
	
	GameControllerNode() : parent(-1), meshIndex(-1), isVisible(true) {
		translation[0] = translation[1] = translation[2] = 0.0f;
		rotation[0] = rotation[1] = rotation[2] = 0.0f;
		rotation[3] = 1.0f;
		scale[0] = scale[1] = scale[2] = 1.0f;
		for (int i = 0; i < 16; i++) {
			localMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
			worldMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
		}
	}
	
	void ComputeLocalMatrix();
	void UpdateWorldMatrix(const float* parentWorld);
};

// Material data
struct GameControllerMaterial
{
	float baseColorFactor[4];
	float emissiveFactor[3];
	bool hasTexture;
	
	// Texture data (decoded RGBA pixels)
	std::vector<uint8_t> textureData;
	int textureWidth;
	int textureHeight;
	
	// Source Engine material (created from texture)
	IMaterial* pMaterial;
	ITexture* pTexture;
	
	GameControllerMaterial() : hasTexture(false), textureWidth(0), textureHeight(0), pMaterial(nullptr), pTexture(nullptr) {
		baseColorFactor[0] = baseColorFactor[1] = baseColorFactor[2] = 0.8f;
		baseColorFactor[3] = 1.0f;
		emissiveFactor[0] = emissiveFactor[1] = emissiveFactor[2] = 0.0f;
	}
};

// Complete controller model
struct GameControllerModel
{
	bool isLoaded;
	bool isVisible;
	VMatrix currentPose;        // Source world-space pose (for compatibility)
	XrPosef currentPoseXR;      // Raw OpenXR pose in playspace
	bool poseValid;
	bool hasRawXRPose;          // True if currentPoseXR is valid (not using fallback)
	
	CUtlVector<GameControllerMesh> meshes;
	CUtlVector<GameControllerNode> nodes;
	CUtlVector<GameControllerMaterial> materials;
	CUtlVector<int> rootNodes;
	
	// OpenXR handles
	XrRenderModelEXT renderModel;
	XrRenderModelAssetEXT assetHandle;
	XrRenderModelIdEXT renderModelId;
	XrSpace modelSpace;  // Space for locating this render model
	XrUuidEXT cacheId;
	uint32_t animatableNodeCount;
	
	// Animation state
	CUtlVector<XrRenderModelNodeStateEXT> nodeStates;
	CUtlVector<int> animNodeToGltfNode;
	
	GameControllerModel() 
		: isLoaded(false), isVisible(true), poseValid(false), hasRawXRPose(false)
		, renderModel(XR_NULL_HANDLE), assetHandle(XR_NULL_HANDLE)
		, renderModelId(0), modelSpace(XR_NULL_HANDLE), animatableNodeCount(0) 
	{
		// Initialize XrPosef to identity
		currentPoseXR.orientation.x = 0;
		currentPoseXR.orientation.y = 0;
		currentPoseXR.orientation.z = 0;
		currentPoseXR.orientation.w = 1;
		currentPoseXR.position.x = 0;
		currentPoseXR.position.y = 0;
		currentPoseXR.position.z = 0;
		currentPose.Identity();
		memset(&cacheId, 0, sizeof(cacheId));
	}
};

// Manager for controller model rendering on game side
class CVRControllerModelManager
{
public:
	CVRControllerModelManager();
	~CVRControllerModelManager();
	
	bool Initialize(COpenXRManager* pOpenXRManager);
	void Shutdown();
	
	void Update(float frametime);
	void Render();
	
	bool IsLoaded() const { return m_bLoaded; }
	
	// Check if we should show controllers (preamble/death states)
	bool ShouldShowControllers() const;
	
private:
	bool LoadControllerModels();
	bool ParseGLBData(const uint8_t* data, size_t size, GameControllerModel& model);
	void CreateMaterialsForModel(GameControllerModel& model, const char* handName);
	void UpdateAnimationState();
	void UpdateControllerPoses();
	void RenderController(const GameControllerModel& model, bool isLeft);
	void TransformControllerVertex(
		const Vector& localPosXR, const matrix3x4_t& nodeMatrix3x4, const GameControllerModel& model,
		float r00, float r01, float r02, float r10, float r11, float r12,
		float r20, float r21, float r22,
		const XrPosef& xrPose, float worldScale,
		const VMatrix& headInverse, const VMatrix& smoothedHeadWorld,
		Vector& worldPos);
	void CleanupModel(GameControllerModel& model);
	
	COpenXRManager* m_pOpenXRManager;
	bool m_bLoaded;
	bool m_bInitialized;
	bool m_bTriedLoading;
	
	GameControllerModel m_leftController;
	GameControllerModel m_rightController;
	
	// Render model extension function pointers
	PFN_xrEnumerateInteractionRenderModelIdsEXT m_xrEnumerateInteractionRenderModelIds;
	PFN_xrCreateRenderModelEXT m_xrCreateRenderModel;
	PFN_xrDestroyRenderModelEXT m_xrDestroyRenderModel;
	PFN_xrGetRenderModelPropertiesEXT m_xrGetRenderModelProperties;
	PFN_xrGetRenderModelStateEXT m_xrGetRenderModelState;
	PFN_xrCreateRenderModelAssetEXT m_xrCreateRenderModelAsset;
	PFN_xrDestroyRenderModelAssetEXT m_xrDestroyRenderModelAsset;
	PFN_xrGetRenderModelAssetDataEXT m_xrGetRenderModelAssetData;
	PFN_xrGetRenderModelAssetPropertiesEXT m_xrGetRenderModelAssetProperties;
	PFN_xrEnumerateRenderModelSubactionPathsEXT m_xrEnumerateRenderModelSubactionPaths;
	PFN_xrCreateRenderModelSpaceEXT m_xrCreateRenderModelSpace;
	
	// Material for rendering
	IMaterial* m_pControllerMaterial;
};

extern CVRControllerModelManager* g_pVRControllerModelManager;

#endif // VR_CONTROLLER_MODEL_H
