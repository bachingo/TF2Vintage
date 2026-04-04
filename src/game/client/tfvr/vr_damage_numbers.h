// Purpose: VR Damage Numbers - Renders floating damage/healing numbers in 3D 
//          world space above victims, matching vanilla TF2 behavior.

#ifndef VR_DAMAGE_NUMBERS_H
#define VR_DAMAGE_NUMBERS_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "utlvector.h"
#include <vgui_controls/EditablePanel.h>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class C_BaseEntity;

//-----------------------------------------------------------------------------
// Purpose: Individual damage number entry (mirrors vanilla account_delta_t)
//-----------------------------------------------------------------------------
struct vr_damage_number_t
{
    enum eDamageType_t
    {
        DAMAGE_TYPE_DAMAGE,
        DAMAGE_TYPE_HEALING,
        DAMAGE_TYPE_CRIT,
    };

    int m_iAmount;              // Damage/healing value
    eDamageType_t m_eType;      // Type of damage for coloring
    
    float m_flSpawnTime;        // When this number was created
    float m_flDieTime;          // When to remove
    
    // World position
    float m_flWorldX;           // X position in world
    float m_flWorldY;           // Y position in world
    float m_flWorldZStart;      // Starting Z (height)
    float m_flWorldZEnd;        // Ending Z (floats up)
    
    int m_nSourceID;            // Entity index for batching
    float m_flBatchWindow;      // Time window for batching
    
    bool m_bLargeFont;          // Crit = larger text
};

//-----------------------------------------------------------------------------
// Purpose: Simple panel to render a damage number
//-----------------------------------------------------------------------------
class CVRDamageNumberPanel : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CVRDamageNumberPanel, vgui::EditablePanel);
    
public:
    CVRDamageNumberPanel(vgui::Panel* parent, const char* name);
    
    void SetNumber(int amount, bool bLargeFont, Color color);
    virtual void Paint() override;
    
private:
    int m_iAmount;
    bool m_bLargeFont;
    Color m_Color;
    
    vgui::HFont m_hFont;
    vgui::HFont m_hFontLarge;
};

//-----------------------------------------------------------------------------
// Purpose: Manages all VR damage numbers
//-----------------------------------------------------------------------------
class CVRDamageNumberManager
{
public:
    CVRDamageNumberManager();
    ~CVRDamageNumberManager();
    
    bool Initialize();
    void Shutdown();
    void ResetState();
    
    void Update(float deltaTime);
    void Render();
    
    // Add a new damage number (called from game events)
    void AddDamageNumber(C_BaseEntity* pVictim, int iAmount, bool bCrit, bool bHealing);
    
    // Static accessors
    static bool IsEnabled();
    static bool ShouldSuppressVanillaRendering();
    
private:
    // Calculate billboard transform facing the player
    bool CalculateBillboardTransform(const Vector& worldPos, VMatrix& transform);
    
    // Get color for damage type
    Color GetDamageColor(vr_damage_number_t::eDamageType_t type);
    
private:
    bool m_bInitialized;
    bool m_bEnabled;
    
    // Pool of damage numbers
    CUtlVector<vr_damage_number_t> m_DamageNumbers;
    
    // Rendering panel (reused for each number)
    CVRDamageNumberPanel* m_pRenderPanel;
    
    // Configuration (from ConVars, matching vanilla)
    float m_flLifetime;         // How long numbers last (vanilla: 2.0)
    float m_flFloatDistance;    // How far to float up (vanilla: 32 units)
    float m_flScale;            // World size scale
    int m_nRenderScale;         // Resolution multiplier
    
    // Batching
    bool m_bBatchingEnabled;
    float m_flBatchWindow;
};

// Global instance
extern CVRDamageNumberManager* g_pVRDamageNumberManager;

#endif // VR_DAMAGE_NUMBERS_H
