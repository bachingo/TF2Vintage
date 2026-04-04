//=============================================================================
// TF2VR - VR World UI Queue
// Centralized distance-sorted rendering for all VR world-space UI elements
//=============================================================================

#include "cbase.h"
#include "vr_world_ui_queue.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instance
CVRWorldUIQueue* g_pVRWorldUIQueue = nullptr;

bool CVRWorldUIQueue::s_bInsideFlush = false;

//=============================================================================
// ConVars
//=============================================================================

ConVar tfvr_world_ui_queue_enabled("tfvr_world_ui_queue_enabled", "1", FCVAR_ARCHIVE,
    "Enable distance-sorted rendering for VR world UI elements");

ConVar tfvr_world_ui_queue_debug("tfvr_world_ui_queue_debug", "0", FCVAR_NONE,
    "Debug output for VR world UI queue");

//=============================================================================
// Implementation
//=============================================================================

CVRWorldUIQueue::CVRWorldUIQueue()
    : m_bInitialized(false)
    , m_headPos(0, 0, 0)
{
}

CVRWorldUIQueue::~CVRWorldUIQueue()
{
    Shutdown();
}

bool CVRWorldUIQueue::Initialize()
{
    if (m_bInitialized)
        return true;
    
    m_renderQueue.Purge();
    m_bInitialized = true;
    
    DevMsg("VR World UI Queue: Initialized\n");
    return true;
}

void CVRWorldUIQueue::Shutdown()
{
    if (!m_bInitialized)
        return;
    
    m_renderQueue.Purge();
    m_bInitialized = false;
    
    DevMsg("VR World UI Queue: Shutdown\n");
}

void CVRWorldUIQueue::BeginFrame(const Vector& headPos)
{
    m_headPos = headPos;
    m_renderQueue.RemoveAll();
}

void CVRWorldUIQueue::QueuePanel(vgui::Panel* pPanel, const VMatrix& transform,
                                  int pixelWidth, int pixelHeight,
                                  float worldWidth, float worldHeight,
                                  int priority,
                                  bool bRestoreVisibility, bool bWasVisible)
{
    if (!m_bInitialized || !pPanel || pixelWidth <= 0 || pixelHeight <= 0)
        return;
    
    if (!tfvr_world_ui_queue_enabled.GetBool())
    {
        // If queue is disabled, render immediately (legacy behavior)
        if (bRestoreVisibility)
        {
            pPanel->SetVisible(true);
        }
        
        g_pMatSystemSurface->DisableClipping(true);
        
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            pPanel->GetVPanel(),
            transform,
            pixelWidth,
            pixelHeight,
            worldWidth,
            worldHeight
        );
        
        g_pMatSystemSurface->DisableClipping(false);
        
        if (bRestoreVisibility)
        {
            pPanel->SetVisible(bWasVisible);
        }
        return;
    }
    
    VRWorldUIRenderItem item;
    item.pPanel = pPanel;
    item.transform = transform;
    item.pixelWidth = pixelWidth;
    item.pixelHeight = pixelHeight;
    item.worldWidth = worldWidth;
    item.worldHeight = worldHeight;
    item.bRestoreVisibility = bRestoreVisibility;
    item.bWasVisible = bWasVisible;
    item.priority = priority;
    
    // Calculate distance from head to panel center
    Vector panelPos = transform.GetTranslation();
    item.distanceFromHead = (panelPos - m_headPos).Length();
    
    m_renderQueue.AddToTail(item);
    
    if (tfvr_world_ui_queue_debug.GetBool())
    {
        DevMsg("VR World UI Queue: Queued panel %s at distance %.1f, priority %d\n",
               pPanel->GetName(), item.distanceFromHead, priority);
    }
}

// Comparison function for sorting render items
// Sort by distance (back-to-front), then by priority (lower priority first)
static int CompareWorldUIRenderItems(const VRWorldUIRenderItem* a, const VRWorldUIRenderItem* b)
{
    // First sort by distance (farther items first = back-to-front)
    if (a->distanceFromHead > b->distanceFromHead + 0.1f)
        return -1;
    if (a->distanceFromHead < b->distanceFromHead - 0.1f)
        return 1;
    
    // At same distance, sort by priority (lower priority rendered first = behind)
    if (a->priority < b->priority)
        return -1;
    if (a->priority > b->priority)
        return 1;
    
    return 0;
}

void CVRWorldUIQueue::FlushRenderQueue()
{
    VPROF("VRWorldUIQueue_FlushRenderQueue");
    
    if (!m_bInitialized || m_renderQueue.Count() == 0)
        return;
    
    if (!tfvr_world_ui_queue_enabled.GetBool())
    {
        // Already rendered immediately in QueuePanel
        m_renderQueue.RemoveAll();
        return;
    }
    
    // Sort by distance (back-to-front) and priority
    m_renderQueue.Sort(CompareWorldUIRenderItems);
    
    if (tfvr_world_ui_queue_debug.GetBool())
    {
        DevMsg("VR World UI Queue: Flushing %d panels\n", m_renderQueue.Count());
    }
    
    // Render all queued panels
    g_pMatSystemSurface->DisableClipping(true);
    
    s_bInsideFlush = true;
    
    for (int i = 0; i < m_renderQueue.Count(); i++)
    {
        VRWorldUIRenderItem& item = m_renderQueue[i];
        
        // Make panel visible for rendering if needed
        if (item.bRestoreVisibility)
        {
            item.pPanel->SetVisible(true);
        }
        
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            item.pPanel->GetVPanel(),
            item.transform,
            item.pixelWidth,
            item.pixelHeight,
            item.worldWidth,
            item.worldHeight
        );
        
        // Restore visibility
        if (item.bRestoreVisibility)
        {
            item.pPanel->SetVisible(item.bWasVisible);
        }
    }
    
    s_bInsideFlush = false;
    
    g_pMatSystemSurface->DisableClipping(false);
    
    // Clear the queue
    m_renderQueue.RemoveAll();
}
