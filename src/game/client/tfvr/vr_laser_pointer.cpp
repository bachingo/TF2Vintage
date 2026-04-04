#include "cbase.h"
#include "vr_laser_pointer.h"
#include "openxr_manager.h"
#include "vr_menu_manager.h"
#include "hmdWrapper.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "engine/ivdebugoverlay.h"
#include "vgui_controls/Controls.h"
#include "convar.h"
#include "client_virtualreality.h"

// ConVars for laser pointer control
ConVar tfvr_laser_enabled("tfvr_laser_enabled", "1", FCVAR_ARCHIVE, "Enable VR laser pointer");
ConVar tfvr_laser_length("tfvr_laser_length", "100.0", FCVAR_ARCHIVE, "Length of the laser pointer in game units");
ConVar tfvr_laser_width("tfvr_laser_width", "0.1", FCVAR_ARCHIVE, "Width of the laser pointer cylinder");

ConVar tfvr_laser_color_r("tfvr_laser_color_r", "128", FCVAR_ARCHIVE, "Red component of laser color (0-255)");
ConVar tfvr_laser_color_g("tfvr_laser_color_g", "183", FCVAR_ARCHIVE, "Green component of laser color (0-255)");
ConVar tfvr_laser_color_b("tfvr_laser_color_b", "24", FCVAR_ARCHIVE, "Blue component of laser color (0-255)");
ConVar tfvr_laser_debug("tfvr_laser_debug", "0", FCVAR_ARCHIVE, "Show debug info for laser pointer");

// Global instance
CVRLaserPointer* g_pVRLaserPointer = nullptr;

CVRLaserPointer::CVRLaserPointer()
    : m_bLaserActive(false)
    , m_laserStart(vec3_origin)
    , m_laserEnd(vec3_origin)
    , m_laserLength(100.0f)
    , m_laserWidth(2.0f)
    , m_laserColor(255, 0, 0, 255) // Red laser
    , m_pLaserMaterial(nullptr)
    , m_bLaserMaterialCreated(false)
{
}

CVRLaserPointer::~CVRLaserPointer()
{
    Shutdown();
}

void CVRLaserPointer::Initialize()
{
    CreateLaserMaterial();
    
    // Sync laser settings to compositor immediately on initialization
    // This ensures the compositor has correct values before any frames are rendered
    SyncSettingsToCompositor();
}

void CVRLaserPointer::SyncSettingsToCompositor()
{
    // Convert game units to meters and sync all laser parameters to compositor
    float r = tfvr_laser_color_r.GetInt() / 255.0f;
    float g = tfvr_laser_color_g.GetInt() / 255.0f;
    float b = tfvr_laser_color_b.GetInt() / 255.0f;
    float lengthMeters = tfvr_laser_length.GetFloat() / 39.3701f;  // Game units to meters
    float widthMeters = tfvr_laser_width.GetFloat() / 39.3701f;
    
    dxvkSetLaserColor(r, g, b);
    dxvkSetLaserLength(lengthMeters);
    dxvkSetLaserWidth(widthMeters);
}

void CVRLaserPointer::Shutdown()
{
    // Materials are reference-counted and automatically destroyed
    m_pLaserMaterial = nullptr;
    m_bLaserMaterialCreated = false;
}

void CVRLaserPointer::CreateLaserMaterial()
{
    // Create a simple colored material using UnlitGeneric
    KeyValues *pVMTKeyValues = new KeyValues("UnlitGeneric");
    pVMTKeyValues->SetString("$basetexture", "vgui/white_additive");
    pVMTKeyValues->SetString("$vertexcolor", "1");
    pVMTKeyValues->SetString("$vertexalpha", "1");
    pVMTKeyValues->SetString("$translucent", "1");
    pVMTKeyValues->SetString("$ignorez", "1");
    
    // Create the material
    m_pLaserMaterial = materials->CreateMaterial("__vr_laser_material", pVMTKeyValues);
    
    if (m_pLaserMaterial && !m_pLaserMaterial->IsErrorMaterial())
    {
        m_bLaserMaterialCreated = true;
    }
    else
    {
        // Fallback to existing materials
        m_pLaserMaterial = materials->FindMaterial("sprites/white", TEXTURE_GROUP_OTHER);
        if (!m_pLaserMaterial || m_pLaserMaterial->IsErrorMaterial())
        {
            m_pLaserMaterial = materials->FindMaterial("engine/writez", TEXTURE_GROUP_OTHER);
        }
    }
}

void CVRLaserPointer::Update(float frametime)
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
        return;
    
    // Sync laser parameters to compositor (convert game units to meters)
    // Must happen BEFORE the enabled check so compositor gets correct values on startup
    static float lastSyncedR = -1, lastSyncedG = -1, lastSyncedB = -1;
    static float lastSyncedLength = -1, lastSyncedWidth = -1;
    static bool initialSyncDone = false;
    
    float r = tfvr_laser_color_r.GetInt() / 255.0f;
    float g = tfvr_laser_color_g.GetInt() / 255.0f;
    float b = tfvr_laser_color_b.GetInt() / 255.0f;
    float lengthMeters = tfvr_laser_length.GetFloat() / 39.3701f;  // Game units to meters
    float widthMeters = tfvr_laser_width.GetFloat() / 39.3701f;
    
    // Force sync on first update (ensure compositor gets cvar values on startup)
    // After that, only sync if changed (reduce bridge calls)
    bool forceSync = !initialSyncDone;
    if (forceSync || r != lastSyncedR || g != lastSyncedG || b != lastSyncedB) {
        dxvkSetLaserColor(r, g, b);
        lastSyncedR = r; lastSyncedG = g; lastSyncedB = b;
    }
    if (forceSync || lengthMeters != lastSyncedLength) {
        dxvkSetLaserLength(lengthMeters);
        lastSyncedLength = lengthMeters;
    }
    if (forceSync || widthMeters != lastSyncedWidth) {
        dxvkSetLaserWidth(widthMeters);
        lastSyncedWidth = widthMeters;
    }
    initialSyncDone = true;
    
    // Check if laser is enabled (after sync so compositor always has correct values)
    if (!tfvr_laser_enabled.GetBool())
        return;
    
    UpdateLaserPointer();
    // Note: Rendering is now done on-demand via RenderLaserOnTop()
    // No world collision detection - laser only interacts with menu plane
}

void CVRLaserPointer::UpdateLaserPointer()
{
    // Only show laser when menu is visible
    if (!g_pVRMenuManager || !g_pVRMenuManager->IsMenuVisible())
    {
        m_bLaserActive = false;
        return;
    }
    
    // Get raw XR pose for proper transformation (matches controller model approach)
    XrPosef xrPose;
    bool poseValid = false;
    int menuHand = g_pVRMenuManager->GetActiveMenuHand();
    
    if (menuHand == 0) // Left hand
    {
        if (g_pOpenXRManager && g_pOpenXRManager->IsLeftControllerPoseValid())
        {
            poseValid = g_pOpenXRManager->GetLeftControllerPoseXR(xrPose);
        }
    }
    else // Right hand
    {
        if (g_pOpenXRManager && g_pOpenXRManager->IsRightControllerPoseValid())
        {
            poseValid = g_pOpenXRManager->GetRightControllerPoseXR(xrPose);
        }
    }
    
    if (poseValid)
    {
        // Transform using the same approach as controller models:
        // 1. Work in OpenXR space first, then convert to Source
        
        float worldScale = g_pOpenXRManager->GetWorldScale();
        
        // Build rotation matrix from quaternion (OpenXR space)
        float qx = xrPose.orientation.x;
        float qy = xrPose.orientation.y;
        float qz = xrPose.orientation.z;
        float qw = xrPose.orientation.w;
        
        float xx = qx * qx, yy = qy * qy, zz = qz * qz;
        float xy = qx * qy, xz = qx * qz, yz = qy * qz;
        float wx = qw * qx, wy = qw * qy, wz = qw * qz;
        
        // In OpenXR, -Z is forward (away from user), +Z is backward (toward user)
        // Get the Z axis direction (backward in OpenXR space) - third column of rotation matrix
        // For quaternion (qx,qy,qz,qw), Z column is: (2(xz+wy), 2(yz-wx), 1-2(xx+yy))
        Vector zAxisXR;
        zAxisXR.x = 2.0f * (xz + wy);
        zAxisXR.y = 2.0f * (yz - wx);
        zAxisXR.z = 1.0f - 2.0f * (xx + yy);
        
        // Position in OpenXR space (meters)
        Vector posXR(xrPose.position.x, xrPose.position.y, xrPose.position.z);
        
        // Convert position from OpenXR to Source playspace and scale to game units
        Vector playspacePosSource;
        playspacePosSource.x = -posXR.z * worldScale;  // Source X = -OpenXR Z
        playspacePosSource.y = -posXR.x * worldScale;  // Source Y = -OpenXR X
        playspacePosSource.z = posXR.y * worldScale;   // Source Z = OpenXR Y
        
        // Transform position through head-relative to world (same as controller models)
        extern CClientVirtualReality g_ClientVirtualReality;
        VMatrix headInPlayspace = g_pOpenXRManager->GetMideyePose();
        VMatrix headInverse = headInPlayspace.InverseTR();
        VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
        
        Vector posRelativeToHead = headInverse.VMul4x3(playspacePosSource);
        Vector controllerPos = smoothedHeadWorld.VMul4x3(posRelativeToHead);
        
        // To get the correct world direction, transform a point along the forward direction
        // the same way we transform the position, then compute the difference
        Vector testPointXR = posXR + zAxisXR * (-0.1f);  // 10cm forward in OpenXR (negative Z)
        
        // Convert test point to Source playspace
        Vector testPointSource;
        testPointSource.x = -testPointXR.z;
        testPointSource.y = -testPointXR.x;
        testPointSource.z = testPointXR.y;
        testPointSource *= worldScale;
        
        // Transform test point through head-relative to world
        Vector testRelativeToHead = headInverse.VMul4x3(testPointSource);
        Vector testPointWorld = smoothedHeadWorld.VMul4x3(testRelativeToHead);
        
        // Direction is from controller position to test point
        Vector forwardSource = testPointWorld - controllerPos;
        forwardSource.NormalizeInPlace();
        
        // Set laser start and end points
        m_laserStart = controllerPos;
        m_laserLength = tfvr_laser_length.GetFloat();
        
        // Laser only interacts with menu plane - no world collision detection
        Vector menuIntersection = GetCursorWorldPosition(controllerPos, forwardSource);
        if (menuIntersection != vec3_origin)
        {
            // Laser ends at menu plane intersection
            m_laserEnd = menuIntersection;
        }
        else
        {
            // Menu not visible or no intersection - laser goes full length in controller direction
            m_laserEnd = controllerPos + forwardSource * m_laserLength;
        }
        
        // Note: Compositor calculates its own intersection for the compositor laser
        
        m_bLaserActive = true;
        
        // Update laser color from ConVars (fix color issue)
        m_laserColor.SetColor(
            clamp(tfvr_laser_color_r.GetInt(), 0, 255),
            clamp(tfvr_laser_color_g.GetInt(), 0, 255),
            clamp(tfvr_laser_color_b.GetInt(), 0, 255),
            255
        );
        
        // Optional debug output
        if (tfvr_laser_debug.GetBool())
        {
            DevMsg("Laser: hand=%d pos(%.2f, %.2f, %.2f) dir(%.2f, %.2f, %.2f) length=%.1f\n",
                   menuHand, controllerPos.x, controllerPos.y, controllerPos.z,
                   forwardSource.x, forwardSource.y, forwardSource.z, m_laserLength);
        }
    }
    else
    {
        m_bLaserActive = false;
    }
}



void CVRLaserPointer::RenderLaserPointer()
{
    if (!m_bLaserActive || !m_pLaserMaterial)
        return;
    
    CMatRenderContextPtr pRenderContext(materials);
    
    // Disable depth testing so laser renders on top of HUD
    pRenderContext->OverrideDepthEnable(true, false); // Enable depth read, disable depth write
    
    // Bind material and set up rendering
    pRenderContext->Bind(m_pLaserMaterial);
    
    // Prepare color for vertex colors (since our custom material supports them)
    float r = m_laserColor.r() / 255.0f;
    float g = m_laserColor.g() / 255.0f;
    float b = m_laserColor.b() / 255.0f;
    float a = m_laserColor.a() / 255.0f;
    
    // Get laser parameters
    float width = tfvr_laser_width.GetFloat();
    
    // Calculate direction and length
    Vector direction = m_laserEnd - m_laserStart;
    float length = direction.Length();
    if (length < 0.1f) 
    {
        pRenderContext->OverrideDepthEnable(false, true); // Restore normal depth
        return;
    }
    direction /= length;
    
    // Calculate perpendicular vectors for quad orientation
    Vector up;
    if (fabs(direction.z) < 0.9f)
        up = Vector(0, 0, 1);
    else
        up = Vector(1, 0, 0);
    
    Vector right = direction.Cross(up).Normalized();
    up = right.Cross(direction).Normalized();
    
    // Create box mesh (rectangular prism)
    IMesh* pMesh = pRenderContext->GetDynamicMesh();
    CMeshBuilder meshBuilder;
    
    // Box has 6 faces, each face is 2 triangles = 12 triangles total
    meshBuilder.Begin(pMesh, MATERIAL_TRIANGLES, 8, 36);
    
    // Calculate box dimensions (thin box)
    float halfWidth = width * 0.5f;
    Vector upOffset = up * halfWidth;
    Vector rightOffset = right * halfWidth;
    
    // Define 8 vertices of the box
    Vector verts[8];
    verts[0] = m_laserStart - upOffset - rightOffset; // Bottom-back-left
    verts[1] = m_laserStart - upOffset + rightOffset; // Bottom-back-right
    verts[2] = m_laserStart + upOffset + rightOffset; // Top-back-right
    verts[3] = m_laserStart + upOffset - rightOffset; // Top-back-left
    verts[4] = m_laserEnd - upOffset - rightOffset;   // Bottom-front-left
    verts[5] = m_laserEnd - upOffset + rightOffset;   // Bottom-front-right
    verts[6] = m_laserEnd + upOffset + rightOffset;   // Top-front-right
    verts[7] = m_laserEnd + upOffset - rightOffset;   // Top-front-left
    
    // Add vertices to mesh with vertex colors
    for (int i = 0; i < 8; i++)
    {
        meshBuilder.Position3fv(verts[i].Base());
        meshBuilder.Normal3fv(Vector(0, 0, 1).Base()); // Will set proper normals per face
        meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
        meshBuilder.Color4f(r, g, b, a);  // Apply color to each vertex
        meshBuilder.AdvanceVertex();
    }
    
    // Define faces (each face = 2 triangles = 6 indices)
    // Face indices for box faces
    int faces[6][6] = {
        {0, 1, 2, 0, 2, 3}, // Back face
        {4, 6, 5, 4, 7, 6}, // Front face  
        {0, 4, 5, 0, 5, 1}, // Bottom face
        {3, 2, 6, 3, 6, 7}, // Top face
        {0, 3, 7, 0, 7, 4}, // Left face
        {1, 5, 6, 1, 6, 2}  // Right face
    };
    
    // Add indices for all faces
    for (int face = 0; face < 6; face++)
    {
        for (int i = 0; i < 6; i++)
        {
            meshBuilder.Index(faces[face][i]);
            meshBuilder.AdvanceIndex();
        }
    }
    
    meshBuilder.End();
    pMesh->Draw();
    
    // Restore normal depth testing
    pRenderContext->OverrideDepthEnable(false, true);
}

bool CVRLaserPointer::GetLaserHitPoint(Vector& hitPoint, Vector& hitNormal, C_BaseEntity*& hitEntity)
{
    // Laser no longer does world collision detection - only interacts with menu plane
    return false;
}

void CVRLaserPointer::RenderLaserOnTop()
{
    // Only render if laser is active and enabled
    if (!m_bLaserActive || !tfvr_laser_enabled.GetBool())
        return;
    
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
        return;
    
    // Call the existing render method
    RenderLaserPointer();
}

Vector CVRLaserPointer::GetCursorWorldPosition(const Vector& controllerPos, const Vector& controllerForward)
{
    // Check if VR menu manager exists and is available
    if (!g_pVRMenuManager)
        return vec3_origin;
    
    // Use the VR menu manager's method to get the EXACT same intersection point
    // that ComputeCursorPosition uses - this guarantees we're using the same menu plane
    return g_pVRMenuManager->GetMenuPlaneIntersection(controllerPos, controllerForward);
}
