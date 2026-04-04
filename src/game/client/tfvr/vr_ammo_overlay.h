#ifndef VR_AMMO_OVERLAY_H
#define VR_AMMO_OVERLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "openxr_hand_tracking.h"

// Forward declarations
class CTFHudWeaponAmmo;
class IMaterial;
class ITexture;
class C_TFPlayer;

//-----------------------------------------------------------------------------
// Purpose: Manages rendering of ammo display as a 3D quad attached to player's main shooting hand
//-----------------------------------------------------------------------------
class CVRAmmoOverlay
{
public:
    CVRAmmoOverlay();
    ~CVRAmmoOverlay();

    // Initialize the overlay system
    bool Initialize();
    
    // Shutdown and cleanup
    void Shutdown();
    
    // Update each frame - checks for ammo changes and updates position
    void Update();
    
    // Render the ammo quad in 3D space
    void RenderAmmoQuad();
    
    // Set which hand the overlay is attached to (0=left, 1=right) - should be main shooting hand
    void SetHandAttachment(int hand);
    
    // Enable/disable the overlay
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Set position offset relative to hand
    void SetQuadOffset(const Vector& offset) { m_vQuadOffset = offset; }
    void SetQuadRotation(const QAngle& rotation) { m_angQuadRotation = rotation; }
    
    // Reset overlay state (called on map change to clear stale data)
    void ResetOverlayState();

private:
	// Calculate the quad transform matrix based on hand position
	bool CalculateQuadTransform(VMatrix& quadTransform);
	
	// Calculate transform using hand tracking instead of controller
	bool CalculateHandTrackingTransform(VMatrix& quadTransform);
	
	// Calculate transform using weapon's weapon_bone
	bool CalculateWeaponBoneTransform(VMatrix& quadTransform);
	
	// Check if ammo display should be shown based on game state
	bool ShouldDisplayAmmo();

private:
    bool        m_bInitialized;
    bool        m_bEnabled;
    int         m_nAttachedHand;          // 0=left, 1=right (should be main shooting hand)
    float       m_flLastUpdateTime;
    
    // Positioning
    Vector      m_vQuadOffset;            // Offset from hand position
    QAngle      m_angQuadRotation;        // Rotation relative to hand
    
    // Reference to the main ammo panel (we don't modify it!)
    CTFHudWeaponAmmo* m_pMainAmmoPanel;
};

// Global instance
extern CVRAmmoOverlay* g_pVRAmmoOverlay;

#endif // VR_AMMO_OVERLAY_H
