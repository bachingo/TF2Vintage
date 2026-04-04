#ifndef VR_WORLD_UI_QUEUE_H
#define VR_WORLD_UI_QUEUE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "vgui_controls/Panel.h"
#include "utlvector.h"

//-----------------------------------------------------------------------------
// Purpose: Render item for distance-sorted 3D UI rendering
//-----------------------------------------------------------------------------
struct VRWorldUIRenderItem
{
    vgui::Panel* pPanel;        // The panel to render
    VMatrix transform;          // World transform for rendering
    int pixelWidth;             // Panel width in pixels
    int pixelHeight;            // Panel height in pixels
    float worldWidth;           // World width in units
    float worldHeight;          // World height in units
    float distanceFromHead;     // Distance from head (for sorting)
    bool bRestoreVisibility;    // Whether to restore visibility after render
    bool bWasVisible;           // Original visibility state
    int priority;               // Render priority (higher = rendered later/on top at same distance)
};

//-----------------------------------------------------------------------------
// Purpose: Global manager for distance-sorted VR world UI rendering.
//          All VR UI managers should queue their panels here instead of
//          rendering directly. At the end of the frame, call FlushRenderQueue()
//          to render all panels sorted by distance (back-to-front).
//-----------------------------------------------------------------------------
class CVRWorldUIQueue
{
public:
    CVRWorldUIQueue();
    ~CVRWorldUIQueue();
    
    // Initialize/shutdown
    bool Initialize();
    void Shutdown();
    
    // Call at the start of VR UI rendering to prepare for new frame
    void BeginFrame(const Vector& headPos);
    
    // Queue a panel for distance-sorted rendering
    // priority: Higher values rendered later (on top) at same distance
    void QueuePanel(vgui::Panel* pPanel, const VMatrix& transform,
                    int pixelWidth, int pixelHeight,
                    float worldWidth, float worldHeight,
                    int priority = 0,
                    bool bRestoreVisibility = false, bool bWasVisible = false);
    
    // Call at the end of VR UI rendering to flush all queued panels
    void FlushRenderQueue();
    
    // Get the cached head position for distance calculations
    const Vector& GetHeadPosition() const { return m_headPos; }
    
    // Check if initialized
    bool IsInitialized() const { return m_bInitialized; }
    
    // True while FlushRenderQueue is executing DrawPanelIn3DSpace calls.
    // Used by paint-suppression functions to allow painting during 3D capture.
    static bool s_bInsideFlush;
    
private:
    bool m_bInitialized;
    Vector m_headPos;
    CUtlVector<VRWorldUIRenderItem> m_renderQueue;
};

// Global instance
extern CVRWorldUIQueue* g_pVRWorldUIQueue;

#endif // VR_WORLD_UI_QUEUE_H
