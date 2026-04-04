// Purpose: VR Damage Numbers - Renders floating damage/healing numbers in 3D 
//          world space above victims, matching vanilla TF2 behavior.
//
// NOTE: This manager renders immediately rather than using the global UI queue
//       because it reuses a single panel for multiple damage numbers in a loop.
//       The panel data changes for each number, so queuing doesn't work.

#include "cbase.h"
#include "vr_damage_numbers.h"
#include "c_tf_player.h"
#include "c_basecombatcharacter.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "tier0/vprof.h"
#include "sourcevr/isourcevirtualreality.h"
#include "GameEventListener.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instance
CVRDamageNumberManager* g_pVRDamageNumberManager = nullptr;

//=============================================================================
// ConVars - reference vanilla ConVars for colors
//=============================================================================

// Reference the vanilla color ConVars
extern ConVar hud_combattext;
extern ConVar hud_combattext_batching;
extern ConVar hud_combattext_batching_window;
extern ConVar hud_combattext_red;
extern ConVar hud_combattext_green;
extern ConVar hud_combattext_blue;

// VR-specific ConVars
ConVar tfvr_damage_numbers_enabled("tfvr_damage_numbers_enabled", "1", FCVAR_ARCHIVE,
    "Enable VR world-space damage numbers");
ConVar tfvr_damage_numbers_scale("tfvr_damage_numbers_scale", "0.08", FCVAR_ARCHIVE,
    "Scale of damage numbers in world space");
ConVar tfvr_damage_numbers_render_scale("tfvr_damage_numbers_render_scale", "4", FCVAR_ARCHIVE,
    "Resolution multiplier for sharper text (1-8)");
ConVar tfvr_damage_numbers_lifetime("tfvr_damage_numbers_lifetime", "2.0", FCVAR_ARCHIVE,
    "How long damage numbers last (vanilla: 2.0)");
ConVar tfvr_damage_numbers_float_distance("tfvr_damage_numbers_float_distance", "32", FCVAR_ARCHIVE,
    "How far damage numbers float up in world units (vanilla: 32)");

//=============================================================================
// CVRDamageNumberPanel Implementation
//=============================================================================

DECLARE_BUILD_FACTORY(CVRDamageNumberPanel);

CVRDamageNumberPanel::CVRDamageNumberPanel(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    m_iAmount = 0;
    m_bLargeFont = false;
    m_Color = Color(255, 0, 0, 255);
    m_hFont = vgui::INVALID_FONT;
    m_hFontLarge = vgui::INVALID_FONT;
    
    SetPaintBackgroundEnabled(false);
}

void CVRDamageNumberPanel::SetNumber(int amount, bool bLargeFont, Color color)
{
    m_iAmount = amount;
    m_bLargeFont = bLargeFont;
    m_Color = color;
    
    // Get fonts from scheme if not loaded
    if (m_hFont == vgui::INVALID_FONT)
    {
        vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("ClientScheme"));
        if (pScheme)
        {
            m_hFont = pScheme->GetFont("HudFontMediumSmallBold", true);
            m_hFontLarge = pScheme->GetFont("HudFontMediumBold", true);
            
            // Fallback if fonts not found
            if (m_hFont == vgui::INVALID_FONT)
                m_hFont = pScheme->GetFont("Default", true);
            if (m_hFontLarge == vgui::INVALID_FONT)
                m_hFontLarge = pScheme->GetFont("DefaultBold", true);
        }
    }
}

void CVRDamageNumberPanel::Paint()
{
    int panelWidth, panelHeight;
    GetSize(panelWidth, panelHeight);
    
    // Format the number
    wchar_t wBuf[32];
    if (m_iAmount > 0)
    {
        V_swprintf_safe(wBuf, L"+%d", m_iAmount);
    }
    else
    {
        V_swprintf_safe(wBuf, L"%d", m_iAmount);
    }
    
    vgui::HFont font = m_bLargeFont ? m_hFontLarge : m_hFont;
    if (font == vgui::INVALID_FONT)
        return;
    
    vgui::surface()->DrawSetTextFont(font);
    
    // Get text size for centering
    int textWidth, textHeight;
    vgui::surface()->GetTextSize(font, wBuf, textWidth, textHeight);
    
    int xPos = (panelWidth - textWidth) / 2;
    int yPos = (panelHeight - textHeight) / 2;
    
    // Draw shadow
    vgui::surface()->DrawSetTextPos(xPos + 2, yPos + 2);
    vgui::surface()->DrawSetTextColor(Color(0, 0, 0, m_Color.a()));
    vgui::surface()->DrawPrintText(wBuf, wcslen(wBuf), vgui::FONT_DRAW_NONADDITIVE);
    
    // Draw text
    vgui::surface()->DrawSetTextPos(xPos, yPos);
    vgui::surface()->DrawSetTextColor(m_Color);
    vgui::surface()->DrawPrintText(wBuf, wcslen(wBuf), vgui::FONT_DRAW_NONADDITIVE);
}

//=============================================================================
// CVRDamageNumberManager Implementation
//=============================================================================

CVRDamageNumberManager::CVRDamageNumberManager()
{
    m_bInitialized = false;
    m_bEnabled = true;
    m_pRenderPanel = nullptr;
    
    m_flLifetime = 2.0f;
    m_flFloatDistance = 32.0f;
    m_flScale = 0.08f;
    m_nRenderScale = 4;
    
    m_bBatchingEnabled = false;
    m_flBatchWindow = 0.2f;
}

CVRDamageNumberManager::~CVRDamageNumberManager()
{
    Shutdown();
}

// Forward declarations for event registration
void VRDamageNumbers_RegisterEvents();
void VRDamageNumbers_UnregisterEvents();

bool CVRDamageNumberManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    // Create the render panel
    vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
    m_pRenderPanel = new CVRDamageNumberPanel(pViewport, "VRDamageNumberPanel");
    m_pRenderPanel->SetBounds(0, 0, 256, 64);
    m_pRenderPanel->SetVisible(false);
    
    // Register for game events
    VRDamageNumbers_RegisterEvents();
    
    m_bInitialized = true;
    DevMsg("VR Damage Number Manager: Initialized\n");
    
    return true;
}

void CVRDamageNumberManager::Shutdown()
{
    // Unregister from game events
    VRDamageNumbers_UnregisterEvents();
    
    if (m_pRenderPanel)
    {
        m_pRenderPanel->MarkForDeletion();
        m_pRenderPanel = nullptr;
    }
    
    m_DamageNumbers.RemoveAll();
    m_bInitialized = false;
}

void CVRDamageNumberManager::ResetState()
{
    m_DamageNumbers.RemoveAll();
}

bool CVRDamageNumberManager::IsEnabled()
{
    return g_pVRDamageNumberManager && 
           g_pVRDamageNumberManager->m_bInitialized && 
           g_pVRDamageNumberManager->m_bEnabled &&
           UseVR();
}

bool CVRDamageNumberManager::ShouldSuppressVanillaRendering()
{
    return IsEnabled();
}

void CVRDamageNumberManager::Update(float deltaTime)
{
    if (!m_bInitialized)
        return;
    
    // Update from ConVars
    m_bEnabled = tfvr_damage_numbers_enabled.GetBool() && hud_combattext.GetBool();
    m_flScale = tfvr_damage_numbers_scale.GetFloat();
    m_nRenderScale = clamp(tfvr_damage_numbers_render_scale.GetInt(), 1, 8);
    m_flLifetime = tfvr_damage_numbers_lifetime.GetFloat();
    m_flFloatDistance = tfvr_damage_numbers_float_distance.GetFloat();
    m_bBatchingEnabled = hud_combattext_batching.GetBool();
    m_flBatchWindow = hud_combattext_batching_window.GetFloat();
    
    // Remove expired numbers
    FOR_EACH_VEC_BACK(m_DamageNumbers, i)
    {
        if (m_DamageNumbers[i].m_flDieTime < gpGlobals->curtime)
        {
            m_DamageNumbers.Remove(i);
        }
    }
}

void CVRDamageNumberManager::AddDamageNumber(C_BaseEntity* pVictim, int iAmount, bool bCrit, bool bHealing)
{
    if (!m_bInitialized || !m_bEnabled || !pVictim)
        return;
    
    // Don't show damage on invisible targets
    trace_t tr;
    UTIL_TraceLine(pVictim->WorldSpaceCenter(), MainViewOrigin(), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);
    if (tr.fraction < 1.0f)
        return;
    
    // Calculate spawn position (above victim's head)
    Vector vecPos = pVictim->GetAbsOrigin();
    
    // Get victim's height
    C_BaseAnimating* pAnimating = pVictim->GetBaseAnimating();
    if (pAnimating)
    {
        vecPos.z += VEC_HULL_MAX_SCALED(pAnimating).z;
    }
    
    // Add height offset based on distance (like vanilla)
    C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (pLocalPlayer)
    {
        Vector vecDistance = vecPos - pLocalPlayer->GetAbsOrigin();
        float flHeightOffset = RemapValClamped(vecDistance.LengthSqr(), 0.0f, (200.0f * 200.0f), 1, 16);
        vecPos.z += flHeightOffset;
    }
    
    // Check for batching - merge with recent number from same source
    if (m_bBatchingEnabled)
    {
        int sourceID = pVictim->entindex();
        FOR_EACH_VEC_BACK(m_DamageNumbers, i)
        {
            if (m_DamageNumbers[i].m_nSourceID == sourceID &&
                (gpGlobals->curtime - m_DamageNumbers[i].m_flSpawnTime) < m_flBatchWindow)
            {
                // Merge into existing number
                m_DamageNumbers[i].m_iAmount += bHealing ? iAmount : -iAmount;
                m_DamageNumbers[i].m_bLargeFont = m_DamageNumbers[i].m_bLargeFont || bCrit;
                return;
            }
        }
    }
    
    // Create new damage number
    vr_damage_number_t newNumber;
    newNumber.m_iAmount = bHealing ? iAmount : -iAmount;
    newNumber.m_eType = bHealing ? vr_damage_number_t::DAMAGE_TYPE_HEALING : 
                       (bCrit ? vr_damage_number_t::DAMAGE_TYPE_CRIT : vr_damage_number_t::DAMAGE_TYPE_DAMAGE);
    
    newNumber.m_flSpawnTime = gpGlobals->curtime;
    newNumber.m_flDieTime = gpGlobals->curtime + m_flLifetime;
    
    newNumber.m_flWorldX = vecPos.x;
    newNumber.m_flWorldY = vecPos.y;
    newNumber.m_flWorldZStart = vecPos.z;
    newNumber.m_flWorldZEnd = vecPos.z + m_flFloatDistance;
    
    newNumber.m_nSourceID = pVictim->entindex();
    newNumber.m_flBatchWindow = m_bBatchingEnabled ? m_flBatchWindow : 0.0f;
    newNumber.m_bLargeFont = bCrit;
    
    m_DamageNumbers.AddToTail(newNumber);
}

Color CVRDamageNumberManager::GetDamageColor(vr_damage_number_t::eDamageType_t type)
{
    if (type == vr_damage_number_t::DAMAGE_TYPE_HEALING)
    {
        return Color(0, 255, 0, 255);  // Green for healing
    }
    else
    {
        // Use vanilla ConVar colors for damage
        return Color(
            hud_combattext_red.GetInt(),
            hud_combattext_green.GetInt(),
            hud_combattext_blue.GetInt(),
            255
        );
    }
}

bool CVRDamageNumberManager::CalculateBillboardTransform(const Vector& worldPos, VMatrix& transform)
{
    Vector vecEyePos = MainViewOrigin();
    
    // Calculate direction from number to player
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

void CVRDamageNumberManager::Render()
{
    VPROF("VRDamageNumberManager_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pRenderPanel)
        return;
    
    if (!UseVR())
        return;
    
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || !pPlayer->IsAlive())
        return;
    
    Vector eyePos = MainViewOrigin();
    
    // Base panel size
    int basePanelWidth = 64;
    int basePanelHeight = 32;
    int renderWidth = basePanelWidth * m_nRenderScale;
    int renderHeight = basePanelHeight * m_nRenderScale;
    
    m_pRenderPanel->SetSize(renderWidth, renderHeight);
    
    FOR_EACH_VEC(m_DamageNumbers, i)
    {
        vr_damage_number_t& num = m_DamageNumbers[i];
        
        // Calculate lifetime progress (0 to 1)
        float flLifetimePercent = (gpGlobals->curtime - num.m_flSpawnTime) / m_flLifetime;
        flLifetimePercent = clamp(flLifetimePercent, 0.0f, 1.0f);
        
        // Calculate alpha - fade out after 50% of lifetime (like vanilla)
        int alpha = 255;
        if (flLifetimePercent > 0.5f)
        {
            alpha = (int)(255.0f * ((1.0f - flLifetimePercent) / 0.5f));
        }
        
        if (alpha <= 0)
            continue;
        
        // Calculate current Z position (floats up)
        float currentZ = num.m_flWorldZStart + (flLifetimePercent * (num.m_flWorldZEnd - num.m_flWorldZStart));
        Vector worldPos(num.m_flWorldX, num.m_flWorldY, currentZ);
        
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
        Color color = GetDamageColor(num.m_eType);
        color[3] = alpha;
        m_pRenderPanel->SetNumber(num.m_iAmount, num.m_bLargeFont, color);
        
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

//=============================================================================
// Game Event Listener
//=============================================================================

class CVRDamageNumberEventListener : public CGameEventListener
{
public:
    CVRDamageNumberEventListener() : m_bRegistered(false) {}
    
    void RegisterEvents()
    {
        if (m_bRegistered)
            return;
            
        ListenForGameEvent("player_hurt");
        ListenForGameEvent("npc_hurt");
        ListenForGameEvent("player_healed");
        m_bRegistered = true;
    }
    
    void UnregisterEvents()
    {
        if (!m_bRegistered)
            return;
            
        StopListeningForAllEvents();
        m_bRegistered = false;
    }
    
    virtual void FireGameEvent(IGameEvent* event) override
    {
        if (!g_pVRDamageNumberManager || !CVRDamageNumberManager::IsEnabled())
            return;
        
        const char* eventName = event->GetName();
        
        C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
        if (!pLocalPlayer || !pLocalPlayer->IsAlive())
            return;
        
        if (FStrEq(eventName, "player_hurt") || FStrEq(eventName, "npc_hurt"))
        {
            int iDamage = event->GetInt("damageamount");
            int iAttacker = event->GetInt("attacker");
            int iVictim = event->GetInt(FStrEq(eventName, "player_hurt") ? "userid" : "entindex");
            bool bCrit = event->GetBool("crit");
            
            // Only show for local player's damage
            C_BasePlayer* pAttacker = UTIL_PlayerByUserId(iAttacker);
            if (pAttacker != pLocalPlayer)
            {
                // Also check if local medic is healing the attacker
                if (!pLocalPlayer->IsPlayerClass(TF_CLASS_MEDIC))
                    return;
                
                C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
                if (!pTFAttacker || pLocalPlayer->MedicGetHealTarget() != pTFAttacker)
                    return;
            }
            
            // Get victim entity
            C_BaseEntity* pVictim = nullptr;
            if (FStrEq(eventName, "player_hurt"))
            {
                pVictim = UTIL_PlayerByUserId(iVictim);
            }
            else
            {
                pVictim = ClientEntityList().GetBaseEntity(iVictim);
            }
            
            if (!pVictim || pVictim->IsDormant())
                return;
            
            // Don't show self-damage
            if (pVictim == pLocalPlayer)
                return;
            
            g_pVRDamageNumberManager->AddDamageNumber(pVictim, iDamage, bCrit, false);
        }
        else if (FStrEq(eventName, "player_healed"))
        {
            int iAmount = event->GetInt("amount");
            int iPatient = event->GetInt("patient");
            int iHealer = event->GetInt("healer");
            
            // Only show healing done by local player
            C_BasePlayer* pHealer = UTIL_PlayerByUserId(iHealer);
            if (pHealer != pLocalPlayer)
                return;
            
            C_BaseEntity* pPatient = UTIL_PlayerByUserId(iPatient);
            if (!pPatient || pPatient->IsDormant())
                return;
            
            g_pVRDamageNumberManager->AddDamageNumber(pPatient, iAmount, false, true);
        }
    }
    
private:
    bool m_bRegistered;
};

static CVRDamageNumberEventListener s_VRDamageNumberEventListener;

// Called from CVRDamageNumberManager::Initialize()
void VRDamageNumbers_RegisterEvents()
{
    s_VRDamageNumberEventListener.RegisterEvents();
}

// Called from CVRDamageNumberManager::Shutdown()
void VRDamageNumbers_UnregisterEvents()
{
    s_VRDamageNumberEventListener.UnregisterEvents();
}
