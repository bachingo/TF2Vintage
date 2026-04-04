#ifndef VR_LASER_POINTER_H
#define VR_LASER_POINTER_H

#include "cbase.h"
#include "openxr_manager.h"

class CVRInput;

class CVRLaserPointer
{
public:
    CVRLaserPointer();
    ~CVRLaserPointer();

    void Initialize();
    void Shutdown();
    void Update(float frametime);

    // Laser pointer state
    bool IsLaserActive() const { return m_bLaserActive; }
    Vector GetLaserStart() const { return m_laserStart; }
    Vector GetLaserEnd() const { return m_laserEnd; }
    bool GetLaserHitPoint(Vector& hitPoint, Vector& hitNormal, C_BaseEntity*& hitEntity);
    
    // Rendering methods
    void RenderLaserOnTop(); // Render laser on top of HUD/menus

private:
    void UpdateLaserPointer();
    void RenderLaserPointer();
    
    // Helper methods
    Vector GetCursorWorldPosition(const Vector& controllerPos, const Vector& controllerForward);
    void CreateLaserMaterial();
    void SyncSettingsToCompositor();  // Push cvar values to compositor

    // Laser state
    bool m_bLaserActive;
    Vector m_laserStart;
    Vector m_laserEnd;
    
    // Laser properties
    float m_laserLength;
    float m_laserWidth;
    Color m_laserColor;
    
    // Rendering
    IMaterial* m_pLaserMaterial;
    bool m_bLaserMaterialCreated;
};

extern CVRLaserPointer* g_pVRLaserPointer;

#endif // VR_LASER_POINTER_H
