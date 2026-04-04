// Purpose: VR Spectator Extras - Renders player names and health bars in 3D
//          world space above teammates/entities when glowing or spectating.
//
// NOTE: This manager renders immediately rather than using the global UI queue
//       because it reuses a single panel for multiple entities in a loop.
//       The panel data changes for each entity, so queuing doesn't work.

#include "cbase.h"
#include "vr_spectator_extras.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "c_baseobject.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "tf_hud_target_id.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui/ILocalize.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "tier0/vprof.h"
#include "sourcevr/isourcevirtualreality.h"
#include "vr_menu_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instance
CVRSpectatorExtrasManager* g_pVRSpectatorExtrasManager = nullptr;

// Reference vanilla ConVars
extern ConVar tf_spec_xray;
extern ConVar tf_spec_xray_disable;
extern ConVar tf_enable_glows_after_respawn;

// VR-specific ConVars
ConVar tfvr_spectator_extras_enabled("tfvr_spectator_extras_enabled", "1", FCVAR_ARCHIVE,
    "Enable VR world-space player names and health bars");
ConVar tfvr_spectator_extras_scale("tfvr_spectator_extras_scale", "0.06", FCVAR_ARCHIVE,
    "Scale of spectator extras in world space");
ConVar tfvr_spectator_extras_render_scale("tfvr_spectator_extras_render_scale", "4", FCVAR_ARCHIVE,
    "Resolution multiplier for sharper text (1-8)");
ConVar tfvr_spectator_extras_height_offset("tfvr_spectator_extras_height_offset", "10", FCVAR_ARCHIVE,
    "Additional height offset above entity");

//=============================================================================
// CVRSpectatorExtrasPanel Implementation
//=============================================================================

DECLARE_BUILD_FACTORY(CVRSpectatorExtrasPanel);

CVRSpectatorExtrasPanel::CVRSpectatorExtrasPanel(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    m_wszName[0] = L'\0';
    m_flHealth = 1.0f;
    m_Color = Color(255, 255, 255, 255);
    m_hFont = vgui::INVALID_FONT;
    
    SetPaintBackgroundEnabled(false);
}

void CVRSpectatorExtrasPanel::SetData(const wchar_t* wszName, float flHealth, Color color)
{
    V_wcsncpy(m_wszName, wszName, sizeof(m_wszName));
    m_flHealth = flHealth;
    m_Color = color;
    
    // Get font from scheme if not loaded
    if (m_hFont == vgui::INVALID_FONT)
    {
        vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("ClientScheme"));
        if (pScheme)
        {
            m_hFont = pScheme->GetFont("HudFontSmallBold", true);
            if (m_hFont == vgui::INVALID_FONT)
                m_hFont = pScheme->GetFont("DefaultSmall", true);
        }
    }
}

void CVRSpectatorExtrasPanel::Paint()
{
    int panelWidth, panelHeight;
    GetSize(panelWidth, panelHeight);
    
    if (m_hFont == vgui::INVALID_FONT)
        return;
    
    vgui::surface()->DrawSetTextFont(m_hFont);
    
    // Get text size for centering
    int textWidth, textHeight;
    vgui::surface()->GetTextSize(m_hFont, m_wszName, textWidth, textHeight);
    
    int xPos = (panelWidth - textWidth) / 2;
    int yPos = panelHeight / 4;  // Name in upper portion
    
    // Draw name shadow
    vgui::surface()->DrawSetTextPos(xPos + 1, yPos + 1);
    vgui::surface()->DrawSetTextColor(Color(0, 0, 0, m_Color.a()));
    vgui::surface()->DrawPrintText(m_wszName, wcslen(m_wszName), vgui::FONT_DRAW_NONADDITIVE);
    
    // Draw name
    vgui::surface()->DrawSetTextPos(xPos, yPos);
    vgui::surface()->DrawSetTextColor(m_Color);
    vgui::surface()->DrawPrintText(m_wszName, wcslen(m_wszName), vgui::FONT_DRAW_NONADDITIVE);
    
    // Draw health bar
    int healthBarWidth = panelWidth * 0.7f;
    int healthBarHeight = panelHeight * 0.15f;
    int healthBarX = (panelWidth - healthBarWidth) / 2;
    int healthBarY = yPos + textHeight + 4;
    
    // Background
    vgui::surface()->DrawSetColor(Color(40, 40, 40, 200));
    vgui::surface()->DrawFilledRect(healthBarX, healthBarY, healthBarX + healthBarWidth, healthBarY + healthBarHeight);
    
    // Health fill
    vgui::surface()->DrawSetColor(m_Color);
    vgui::surface()->DrawFilledRect(healthBarX, healthBarY, healthBarX + (int)(healthBarWidth * m_flHealth), healthBarY + healthBarHeight);
    
    // Border
    vgui::surface()->DrawSetColor(Color(0, 0, 0, 255));
    vgui::surface()->DrawOutlinedRect(healthBarX, healthBarY, healthBarX + healthBarWidth, healthBarY + healthBarHeight);
}

//=============================================================================
// CVRSpectatorExtrasManager Implementation
//=============================================================================

CVRSpectatorExtrasManager::CVRSpectatorExtrasManager()
{
    m_bInitialized = false;
    m_bEnabled = true;
    m_pRenderPanel = nullptr;
    
    m_flScale = 0.06f;
    m_nRenderScale = 4;
}

CVRSpectatorExtrasManager::~CVRSpectatorExtrasManager()
{
    Shutdown();
}

bool CVRSpectatorExtrasManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    // Create the render panel
    vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
    m_pRenderPanel = new CVRSpectatorExtrasPanel(pViewport, "VRSpectatorExtrasPanel");
    m_pRenderPanel->SetBounds(0, 0, 256, 64);
    m_pRenderPanel->SetVisible(false);
    
    m_bInitialized = true;
    DevMsg("VR Spectator Extras Manager: Initialized\n");
    
    return true;
}

void CVRSpectatorExtrasManager::Shutdown()
{
    if (m_pRenderPanel)
    {
        m_pRenderPanel->MarkForDeletion();
        m_pRenderPanel = nullptr;
    }
    
    m_vecEntitiesToDraw.Purge();
    m_bInitialized = false;
}

void CVRSpectatorExtrasManager::ResetState()
{
    m_vecEntitiesToDraw.Purge();
}

bool CVRSpectatorExtrasManager::IsEnabled()
{
    return g_pVRSpectatorExtrasManager && 
           g_pVRSpectatorExtrasManager->m_bInitialized && 
           g_pVRSpectatorExtrasManager->m_bEnabled &&
           UseVR();
}

bool CVRSpectatorExtrasManager::ShouldSuppressVanillaRendering()
{
    return IsEnabled();
}

void CVRSpectatorExtrasManager::Update(float deltaTime)
{
    if (!m_bInitialized)
        return;
    
    // Update from ConVars
    m_bEnabled = tfvr_spectator_extras_enabled.GetBool();
    m_flScale = tfvr_spectator_extras_scale.GetFloat();
    m_nRenderScale = clamp(tfvr_spectator_extras_render_scale.GetInt(), 1, 8);
    
    // Update entity list
    UpdateEntityList();
}

void CVRSpectatorExtrasManager::UpdateEntityList()
{
    m_vecEntitiesToDraw.Purge();
    
    if (!g_PR)
        return;
    
    if (TFGameRules() && TFGameRules()->ShowMatchSummary())
        return;
    
    C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pLocalPlayer)
        return;
    
    int nLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
    bool bIsHLTV = engine->IsHLTV();
    
    if (tf_spec_xray_disable.GetBool() || (!bIsHLTV && (nLocalPlayerTeam < TEAM_SPECTATOR)))
        return;
    
    // Check if we should be drawing glows
    if (nLocalPlayerTeam >= FIRST_GAME_TEAM)
    {
        if (pLocalPlayer->IsAlive() && !pLocalPlayer->m_Shared.InCond(TF_COND_TEAM_GLOWS))
            return;
    }
    
    bool bShouldDraw = bIsHLTV || 
        (tf_spec_xray.GetBool() && 
         ((nLocalPlayerTeam == TEAM_SPECTATOR) || 
          (pLocalPlayer->GetObserverMode() > OBS_MODE_FREEZECAM) || 
          (pLocalPlayer->m_Shared.InCond(TF_COND_TEAM_GLOWS) && tf_enable_glows_after_respawn.GetBool())));
    
    if (!bShouldDraw)
        return;
    
    bool bShowEveryone = bIsHLTV || 
        ((nLocalPlayerTeam == TEAM_SPECTATOR) && tf_spec_xray.GetBool()) ||
        ((nLocalPlayerTeam >= FIRST_GAME_TEAM) && (pLocalPlayer->GetObserverMode() > OBS_MODE_FREEZECAM) && (tf_spec_xray.GetInt() > 1));
    
    float flHeightOffset = tfvr_spectator_extras_height_offset.GetFloat();
    
    // Loop through players
    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        if (!g_PR->IsConnected(i))
            continue;
        
        CTFPlayer* pPlayer = ToTFPlayer(UTIL_PlayerByIndex(i));
        if (!pPlayer || pPlayer == pLocalPlayer)
            continue;
        
        int nPlayerTeamNumber = pPlayer->GetTeamNumber();
        
        // Skip players we shouldn't show
        if (pPlayer->IsDormant() ||
            (nPlayerTeamNumber < FIRST_GAME_TEAM) ||
            (!pPlayer->IsAlive()) ||
            (pPlayer->m_Shared.IsStealthed() && (nLocalPlayerTeam >= FIRST_GAME_TEAM) && (nPlayerTeamNumber != nLocalPlayerTeam)) ||
            (!bShowEveryone && !pPlayer->IsPlayerClass(TF_CLASS_SPY) && (nPlayerTeamNumber != nLocalPlayerTeam)) ||
            (!bShowEveryone && pPlayer->IsPlayerClass(TF_CLASS_SPY) && !pPlayer->m_Shared.InCond(TF_COND_DISGUISED) && (nPlayerTeamNumber != nLocalPlayerTeam)) ||
            (!bShowEveryone && pPlayer->IsPlayerClass(TF_CLASS_SPY) && pPlayer->m_Shared.InCond(TF_COND_DISGUISED) && (nPlayerTeamNumber != nLocalPlayerTeam) && (pPlayer->m_Shared.GetDisguiseTeam() != nLocalPlayerTeam)))
        {
            continue;
        }
        
        // Don't draw name if spectating this player
        bool bDrawName = true;
        if (!pLocalPlayer->IsAlive())
        {
            if (pLocalPlayer->GetObserverTarget() == pPlayer)
            {
                bDrawName = false;
                if (pLocalPlayer->GetObserverMode() == OBS_MODE_CHASE)
                    continue;
            }
        }
        
        if (!bDrawName)
            continue;
        
        vr_spec_extra_t entry;
        entry.m_nEntIndex = i;
        
        // Handle disguised spies
        C_TFPlayer* pDisguiseTarget = nullptr;
        if (!bIsHLTV && (nLocalPlayerTeam >= FIRST_GAME_TEAM))
        {
            if (pPlayer->IsPlayerClass(TF_CLASS_SPY) && pPlayer->m_Shared.InCond(TF_COND_DISGUISED) && (nPlayerTeamNumber != nLocalPlayerTeam))
            {
                pDisguiseTarget = pPlayer->m_Shared.GetDisguiseTarget();
            }
        }
        
        // Get name
        int nNameIndex = pDisguiseTarget ? pDisguiseTarget->entindex() : i;
        g_pVGuiLocalize->ConvertANSIToUnicode(g_PR->GetPlayerName(nNameIndex), entry.m_wszName, sizeof(entry.m_wszName));
        
        // Height offset
        entry.m_nOffset = VEC_HULL_MAX_SCALED(pPlayer).z + flHeightOffset;
        
        // Health (use disguise health if applicable)
        float flHealth = 1.0f;
        if (pDisguiseTarget)
        {
            flHealth = (float)(pPlayer->m_Shared.GetDisguiseHealth()) / (float)(pPlayer->m_Shared.GetDisguiseMaxHealth());
        }
        else
        {
            flHealth = (float)(pPlayer->GetHealth()) / (float)(pPlayer->GetMaxHealth());
        }
        entry.m_flHealth = clamp(flHealth, 0.0f, 1.0f);
        
        // Glow color
        float r, g, b;
        pPlayer->GetGlowEffectColor(&r, &g, &b);
        entry.m_clrGlowColor = Color(r * 255, g * 255, b * 255, 255);
        
        entry.m_bDrawName = true;
        
        m_vecEntitiesToDraw.AddToTail(entry);
    }
    
    // Loop through buildings
    for (int nCount = 0; nCount < IBaseObjectAutoList::AutoList().Count(); nCount++)
    {
        C_BaseObject* pObject = static_cast<C_BaseObject*>(IBaseObjectAutoList::AutoList()[nCount]);
        if (!pObject || pObject->IsDormant() || pObject->IsMapPlaced() || pObject->IsEffectActive(EF_NODRAW))
            continue;
        
        if (!bShowEveryone && !((nLocalPlayerTeam >= FIRST_GAME_TEAM) && (nLocalPlayerTeam == pObject->GetTeamNumber())))
            continue;
        
        // Don't draw if spectating this building
        if (pLocalPlayer->GetObserverTarget() == pObject)
            continue;
        
        vr_spec_extra_t entry;
        entry.m_nEntIndex = pObject->entindex();
        
        // Get building name
        pObject->GetTargetIDString(entry.m_wszName, sizeof(entry.m_wszName), true);
        
        // Height offset based on building type
        if (pObject->GetType() == OBJ_TELEPORTER)
            entry.m_nOffset = 30 + flHeightOffset;
        else if (pObject->GetType() == OBJ_DISPENSER)
            entry.m_nOffset = 70 + flHeightOffset;
        else
        {
            switch (pObject->GetUpgradeLevel())
            {
            case 1: entry.m_nOffset = 50 + flHeightOffset; break;
            case 2: entry.m_nOffset = 65 + flHeightOffset; break;
            default: entry.m_nOffset = 80 + flHeightOffset; break;
            }
        }
        
        // Health
        float flHealth = (float)(pObject->GetHealth()) / (float)(pObject->GetMaxHealth());
        entry.m_flHealth = clamp(flHealth, 0.0f, 1.0f);
        
        // Glow color
        float r, g, b;
        pObject->GetGlowEffectColor(&r, &g, &b);
        entry.m_clrGlowColor = Color(r * 255, g * 255, b * 255, 255);
        
        entry.m_bDrawName = true;
        
        m_vecEntitiesToDraw.AddToTail(entry);
    }
}

bool CVRSpectatorExtrasManager::CalculateBillboardTransform(const Vector& worldPos, VMatrix& transform)
{
    Vector vecEyePos = MainViewOrigin();
    
    // Calculate direction from panel to player
    Vector forward = vecEyePos - worldPos;
    forward.z = 0;  // Keep upright
    
    if (forward.LengthSqr() < 0.001f)
    {
        forward = Vector(1, 0, 0);
    }
    VectorNormalize(forward);
    
    // Build orientation
    Vector up(0, 0, 1);
    Vector right;
    CrossProduct(up, forward, right);
    VectorNormalize(right);
    
    CrossProduct(forward, right, up);
    VectorNormalize(up);
    
    Vector panelForward = -forward;
    
    // Build matrix
    transform.Identity();
    transform[0][0] = right.x;        transform[0][1] = up.x;        transform[0][2] = panelForward.x;
    transform[1][0] = right.y;        transform[1][1] = up.y;        transform[1][2] = panelForward.y;
    transform[2][0] = right.z;        transform[2][1] = up.z;        transform[2][2] = panelForward.z;
    transform.SetTranslation(worldPos);
    
    return true;
}

void CVRSpectatorExtrasManager::Render()
{
    VPROF("VRSpectatorExtrasManager_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pRenderPanel)
        return;
    
    if (!UseVR())
        return;
    
    if (g_pVRMenuManager && g_pVRMenuManager->IsMenuPanelOpen())
        return;
    
    if (m_vecEntitiesToDraw.Count() == 0)
        return;
    
    Vector eyePos = MainViewOrigin();
    
    // Base panel size
    int basePanelWidth = 128;
    int basePanelHeight = 48;
    int renderWidth = basePanelWidth * m_nRenderScale;
    int renderHeight = basePanelHeight * m_nRenderScale;
    
    m_pRenderPanel->SetSize(renderWidth, renderHeight);
    
    FOR_EACH_VEC(m_vecEntitiesToDraw, i)
    {
        const vr_spec_extra_t& entry = m_vecEntitiesToDraw[i];
        
        if (!entry.m_bDrawName)
            continue;
        
        // Get entity
        C_BaseEntity* pEnt = cl_entitylist->GetEnt(entry.m_nEntIndex);
        if (!pEnt)
            continue;
        
        // Calculate world position
        Vector worldPos = pEnt->GetAbsOrigin();
        worldPos.z += entry.m_nOffset;
        
        // Calculate distance for consistent sizing
        float distance = (worldPos - eyePos).Length();
        const float referenceDistance = 100.0f;
        float distanceScale = distance / referenceDistance;
        
        // World dimensions
        float aspectRatio = (float)basePanelWidth / (float)basePanelHeight;
        float worldHeight = m_flScale * 100.0f * distanceScale;
        float worldWidth = worldHeight * aspectRatio;
        
        // Calculate billboard transform
        VMatrix panelToWorld;
        if (!CalculateBillboardTransform(worldPos, panelToWorld))
            continue;
        
        // Offset to top-left (DrawPanelIn3DSpace renders from top-left)
        Vector panelRight(panelToWorld[0][0], panelToWorld[1][0], panelToWorld[2][0]);
        Vector panelUp(panelToWorld[0][1], panelToWorld[1][1], panelToWorld[2][1]);
        Vector currentPos = panelToWorld.GetTranslation();
        Vector topLeftPos = currentPos - panelRight * (worldWidth * 0.5f) + panelUp * (worldHeight * 0.5f);
        panelToWorld.SetTranslation(topLeftPos);
        
        // Set up panel for rendering
        m_pRenderPanel->SetData(entry.m_wszName, entry.m_flHealth, entry.m_clrGlowColor);
        
        // Render
        m_pRenderPanel->SetVisible(true);
        
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            m_pRenderPanel->GetVPanel(),
            panelToWorld,
            renderWidth,
            renderHeight,
            worldWidth,
            worldHeight
        );
        
        m_pRenderPanel->SetVisible(false);
    }
}
