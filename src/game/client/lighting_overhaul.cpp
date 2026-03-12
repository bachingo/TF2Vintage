//=============================================================================//
//
// lighting_overhaul.cpp — TF2 Vintage Lighting Overhaul
//
// r_lighting_overhaul is the single on/off switch for all lighting
// improvements in this mod.  It defaults to 1 (enabled).
// Pass -csm_disable at launch to lock it to 0 for the whole session.
//
// ── TIER 1 ── ConVar quality baseline (applied when enabled) ──────────────
//   mat_specular 1          proper env_cubemap specular
//   mat_bumpmap 1           normal-map lighting
//   mat_filterlightmaps 1   smooth lightmaps
//   mat_filtertextures 1    bilinear textures
//   r_radiosity 4           full indirect light bounce
//   r_worldlightmin 0.002   raise shadow floor slightly
//   r_shadowrendertotexture 0   CSM replaces blob/RTT shadows
//   r_flashlightdepthres 4096   high-res shadow maps
//   mat_slopescaledepthbias_shadowmap 4
//   csm_filter 0.5          PCF soft shadow edges
//
// ── TIER 2 ── Game-DLL lighting features ──────────────────────────────────
//   CSM entity auto-spawn (csm_autospawn.cpp reads r_lighting_overhaul)
//   Per-player rim back-light: shadowless fill flashlight opposite the sun
//
// ── TIER 3 ── Screen-space and material-proxy effects ─────────────────────
//   Sun shafts / god rays (screen-space radial blur)
//   GGX specular proxy: "GGXSpecular"   — per-VMT microfacet highlights
//   Per-pixel rim proxy: "PixelRimLight" — per-VMT Fresnel rim tint
//   Cubemap tint proxy: "EnvMapTint"    — tints $envmaptint with TOD colour
//
// ConVar:  r_lighting_overhaul   (default 1)
// Command: r_lighting_overhaul_toggle   (key-bind friendly)
// Launch:  -csm_disable          (session lock — stays off even on toggle)
//
//=============================================================================//
#include "cbase.h"
#include "igamesystem.h"
#include "c_baseplayer.h"
#include "iclientshadowmgr.h"
#include "ScreenSpaceEffects.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "KeyValues.h"
#include "view_scene.h"
#include "rendertexture.h"
#include "mathlib/mathlib.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar csm_filter;  // defined in c_env_cascade_light.cpp

//-----------------------------------------------------------------------------
// Master toggle — default ON, archived so users can turn it off and have it
// stay off.  Session lock handled in Init() below.
//-----------------------------------------------------------------------------
ConVar r_lighting_overhaul(
    "r_lighting_overhaul", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "TF2 Vintage lighting overhaul master switch.\n"
    "  1 = On (default): CSM, PCF shadows, rim lighting, quality ConVars.\n"
    "  0 = Off: all ConVars restored to their pre-overhaul values.\n"
    "Pass -csm_disable at launch to lock this to 0 for the session." );

//-----------------------------------------------------------------------------
// Rim light tuning
//-----------------------------------------------------------------------------
static ConVar r_rimlight(
    "r_rimlight", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Enable per-player rim (back) lighting. Follows r_lighting_overhaul." );

static ConVar r_rimlight_intensity(
    "r_rimlight_intensity", "20.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Linear attenuation of the rim flashlight (lower = brighter over distance)." );

static ConVar r_rimlight_fov(
    "r_rimlight_fov", "90.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Cone angle in degrees of the rim flashlight." );

static ConVar r_rimlight_distance(
    "r_rimlight_distance", "384.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Far clip distance of the rim flashlight in world units." );

static ConVar r_rimlight_r( "r_rimlight_r", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Rim light red override (0-255). 0 = auto from light_environment." );
static ConVar r_rimlight_g( "r_rimlight_g", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Rim light green override." );
static ConVar r_rimlight_b( "r_rimlight_b", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Rim light blue override." );

//-----------------------------------------------------------------------------
// CLightingOverhaulSystem
//-----------------------------------------------------------------------------
#define MAX_RIMLIGHT_HANDLES ( MAX_PLAYERS + 1 )
static ClientShadowHandle_t s_hRimLights[ MAX_RIMLIGHT_HANDLES ];
static bool                 s_bRimInit = false;

class CLightingOverhaulSystem : public CAutoGameSystem
{
public:
    CLightingOverhaulSystem()
        : CAutoGameSystem( "CLightingOverhaulSystem" )
        , m_bActive( false )
        , m_bLastState( false )
        , m_bSessionLocked( false )
        , m_refSpecular(  "mat_specular"                      )
        , m_refBumpmap(   "mat_bumpmap"                       )
        , m_refFilterLM(  "mat_filterlightmaps"               )
        , m_refFilterTex( "mat_filtertextures"                )
        , m_refRadiosity( "r_radiosity"                       )
        , m_refWLMin(     "r_worldlightmin"                   )
        , m_refRTT(       "r_shadowrendertotexture"           )
        , m_refDepthRes(  "r_flashlightdepthres"              )
        , m_refSlopeBias( "mat_slopescaledepthbias_shadowmap" )
        , m_iOldSpecular(1), m_iOldBumpmap(1)
        , m_iOldFilterLM(1), m_iOldFilterTex(1), m_iOldRadiosity(0)
        , m_flOldWLMin(0.f), m_iOldRTT(1), m_iOldDepthRes(2048)
        , m_flOldSlopeBias(16.f), m_flOldCSMFilter(1.f)
        , m_rimR(1.f), m_rimG(1.f), m_rimB(1.f)
        , m_rimLinearAtten(20.f), m_rimFOV(90.f), m_rimFarZ(384.f)
        , m_flRimCacheTime(-1.f)    // forces RefreshRimCache() on first frame
    {}

    virtual bool Init() OVERRIDE
    {
        if ( !s_bRimInit )
        {
            for ( int i = 0; i < MAX_RIMLIGHT_HANDLES; ++i )
                s_hRimLights[i] = CLIENTSHADOW_INVALID_HANDLE;
            s_bRimInit = true;
        }

        if ( CommandLine()->FindParm( "-csm_disable" ) )
        {
            m_bSessionLocked = true;
            r_lighting_overhaul.SetValue( 0 );
            Msg( "[LightingOverhaul] Locked OFF for this session via -csm_disable.\n" );
        }
        return true;
    }

    virtual void LevelInitPostEntity() OVERRIDE
    {
        m_bLastState = !r_lighting_overhaul.GetBool();  // force re-apply
    }

    virtual void LevelShutdownPostEntity() OVERRIDE
    {
        KillAllRimLights();
    }

    virtual void FrameUpdatePostEntityThink() OVERRIDE
    {
        if ( m_bSessionLocked )
            return;

        bool bWant = r_lighting_overhaul.GetBool();
        if ( bWant != m_bLastState )
        {
            bWant ? EnableOverhaul() : DisableOverhaul();
            m_bLastState = bWant;
        }

        if ( m_bActive && r_rimlight.GetBool() )
            UpdateRimLights();
        else
            KillAllRimLights();
    }

private:

    //--------------------------------------------------------------------------
    void EnableOverhaul()
    {
        SaveAll();

        if ( m_refSpecular.IsValid()  ) m_refSpecular.SetValue( 1 );
        if ( m_refBumpmap.IsValid()   ) m_refBumpmap.SetValue( 1 );
        if ( m_refFilterLM.IsValid()  ) m_refFilterLM.SetValue( 1 );
        if ( m_refFilterTex.IsValid() ) m_refFilterTex.SetValue( 1 );
        if ( m_refRadiosity.IsValid() ) m_refRadiosity.SetValue( 4 );
        if ( m_refWLMin.IsValid()     ) m_refWLMin.SetValue( 0.002f );
        if ( m_refRTT.IsValid()       ) m_refRTT.SetValue( 0 );
        if ( m_refDepthRes.IsValid()  ) m_refDepthRes.SetValue( 4096 );
        if ( m_refSlopeBias.IsValid() ) m_refSlopeBias.SetValue( 4.0f );
        csm_filter.SetValue( 0.5f );

        r_rimlight.SetValue( 1 );

        if ( g_pScreenSpaceEffects )
            g_pScreenSpaceEffects->EnableScreenSpaceEffect( "sunshafts" );

        m_bActive = true;
        Msg( "[LightingOverhaul] Enabled.\n" );
    }

    void DisableOverhaul()
    {
        RestoreAll();
        r_rimlight.SetValue( 0 );
        KillAllRimLights();

        if ( g_pScreenSpaceEffects )
            g_pScreenSpaceEffects->DisableScreenSpaceEffect( "sunshafts" );

        m_bActive = false;
        Msg( "[LightingOverhaul] Disabled — ConVars restored.\n" );
    }

    //--------------------------------------------------------------------------
    void SaveAll()
    {
        m_iOldSpecular   = m_refSpecular.IsValid()  ? m_refSpecular.GetInt()    : 1;
        m_iOldBumpmap    = m_refBumpmap.IsValid()   ? m_refBumpmap.GetInt()     : 1;
        m_iOldFilterLM   = m_refFilterLM.IsValid()  ? m_refFilterLM.GetInt()   : 1;
        m_iOldFilterTex  = m_refFilterTex.IsValid() ? m_refFilterTex.GetInt()  : 1;
        m_iOldRadiosity  = m_refRadiosity.IsValid() ? m_refRadiosity.GetInt()  : 0;
        m_flOldWLMin     = m_refWLMin.IsValid()     ? m_refWLMin.GetFloat()     : 0.f;
        m_iOldRTT        = m_refRTT.IsValid()       ? m_refRTT.GetInt()         : 1;
        m_iOldDepthRes   = m_refDepthRes.IsValid()  ? m_refDepthRes.GetInt()    : 2048;
        m_flOldSlopeBias = m_refSlopeBias.IsValid() ? m_refSlopeBias.GetFloat() : 16.f;
        m_flOldCSMFilter = csm_filter.GetFloat();
    }

    void RestoreAll()
    {
        if ( m_refSpecular.IsValid()  ) m_refSpecular.SetValue( m_iOldSpecular );
        if ( m_refBumpmap.IsValid()   ) m_refBumpmap.SetValue( m_iOldBumpmap );
        if ( m_refFilterLM.IsValid()  ) m_refFilterLM.SetValue( m_iOldFilterLM );
        if ( m_refFilterTex.IsValid() ) m_refFilterTex.SetValue( m_iOldFilterTex );
        if ( m_refRadiosity.IsValid() ) m_refRadiosity.SetValue( m_iOldRadiosity );
        if ( m_refWLMin.IsValid()     ) m_refWLMin.SetValue( m_flOldWLMin );
        if ( m_refRTT.IsValid()       ) m_refRTT.SetValue( m_iOldRTT );
        if ( m_refDepthRes.IsValid()  ) m_refDepthRes.SetValue( m_iOldDepthRes );
        if ( m_refSlopeBias.IsValid() ) m_refSlopeBias.SetValue( m_flOldSlopeBias );
        csm_filter.SetValue( m_flOldCSMFilter );
    }

    //--------------------------------------------------------------------------
    void GetSunColor( Vector &vecColor )
    {
        // Delegate to the module-level cached version — no duplicate ConVarRef construction.
        GetSunColorFromCSM( vecColor );
    }

    // Sun colour and ConVar values are slow-changing — refresh at most once per second.
    // VectorVectors + BasisToQuaternion on a constant direction are also moved here.
    // Per-frame BuildRimState then only writes origin — everything else is cached.
    void RefreshRimCache()
    {
        Vector vecSunColor;
        GetSunColor( vecSunColor );
        float flR = r_rimlight_r.GetFloat(), flG = r_rimlight_g.GetFloat(), flB = r_rimlight_b.GetFloat();
        if ( flR+flG+flB < 1.f )
        {
            m_rimR = vecSunColor.x*0.85f + 0.15f;
            m_rimG = vecSunColor.y*0.85f + 0.15f;
            m_rimB = vecSunColor.z*0.85f + 0.15f;
        }
        else { m_rimR=flR/255.f; m_rimG=flG/255.f; m_rimB=flB/255.f; }

        m_rimLinearAtten = r_rimlight_intensity.GetFloat();
        m_rimFOV         = r_rimlight_fov.GetFloat();
        m_rimFarZ        = r_rimlight_distance.GetFloat();

        // Rim direction is constant — compute orientation quaternion once.
        Vector vecSunDir( 0.5f, 0.5f, -0.7f );
        VectorNormalize( vecSunDir );
        Vector vecRimDir = -vecSunDir;
        Vector vRight, vUp;
        VectorVectors( vecRimDir, vRight, vUp );
        BasisToQuaternion( vecRimDir, vRight, vUp, m_rimQuat );
        m_rimDir = vecRimDir;

        m_flRimCacheTime = gpGlobals->curtime;
    }

    void BuildRimState( C_BasePlayer *pPlayer, FlashlightState_t &st )
    {
        // Refresh slow-changing state at most once per second.
        if ( m_flRimCacheTime < 0.f || gpGlobals->curtime - m_flRimCacheTime > 1.f )
            RefreshRimCache();

        // Per-frame: only the player origin changes.
        Vector vecOrigin = pPlayer->GetAbsOrigin();
        vecOrigin.z += 64.f;
        st.m_vecLightOrigin  = vecOrigin - m_rimDir * 96.f;
        st.m_quatOrientation = m_rimQuat;

        st.m_Color[0] = m_rimR;
        st.m_Color[1] = m_rimG;
        st.m_Color[2] = m_rimB;
        st.m_Color[3] = 0.f;

        st.m_fConstantAtten        = 0.f;
        st.m_fLinearAtten          = m_rimLinearAtten;
        st.m_fQuadraticAtten       = 0.f;
        st.m_fHorizontalFOVDegrees = m_rimFOV;
        st.m_fVerticalFOVDegrees   = m_rimFOV;
        st.m_NearZ                 = 8.f;
        st.m_FarZ                  = m_rimFarZ;
        st.m_bEnableShadows        = false;

        static CTextureReference s_RimTex;
        if ( !s_RimTex )
            s_RimTex.Init( "effects/flashlight001", TEXTURE_GROUP_OTHER, true );
        st.m_pSpotlightTexture           = s_RimTex;
        st.m_nSpotlightTextureFrame      = 0;
        st.m_flShadowSlopeScaleDepthBias = 0.f;
        st.m_flShadowDepthBias           = 0.f;
        st.m_flShadowFilterSize          = 0.f;
        st.m_flShadowAtten               = 0.f;
    }

    void UpdateRimLights()
    {
        if ( !g_pClientShadowMgr ) return;
        for ( int i = 1; i <= gpGlobals->maxClients; ++i )
        {
            C_BasePlayer *pPlayer = UTIL_PlayerByIndex( i );
            if ( !pPlayer || !pPlayer->ShouldDraw() )
            {
                if ( s_hRimLights[i] != CLIENTSHADOW_INVALID_HANDLE )
                {
                    g_pClientShadowMgr->DestroyFlashlight( s_hRimLights[i] );
                    s_hRimLights[i] = CLIENTSHADOW_INVALID_HANDLE;
                }
                continue;
            }
            FlashlightState_t st;
            BuildRimState( pPlayer, st );
            if ( s_hRimLights[i] == CLIENTSHADOW_INVALID_HANDLE )
                s_hRimLights[i] = g_pClientShadowMgr->CreateFlashlight( st );
            else
            {
                g_pClientShadowMgr->UpdateFlashlightState( s_hRimLights[i], st );
                g_pClientShadowMgr->UpdateProjectedTexture( s_hRimLights[i], true );
            }
        }
    }

    void KillAllRimLights()
    {
        if ( !g_pClientShadowMgr || !s_bRimInit ) return;
        for ( int i = 0; i < MAX_RIMLIGHT_HANDLES; ++i )
        {
            if ( s_hRimLights[i] != CLIENTSHADOW_INVALID_HANDLE )
            {
                g_pClientShadowMgr->DestroyFlashlight( s_hRimLights[i] );
                s_hRimLights[i] = CLIENTSHADOW_INVALID_HANDLE;
            }
        }
    }

    //--------------------------------------------------------------------------
    bool m_bActive, m_bLastState, m_bSessionLocked;
    ConVarRef m_refSpecular, m_refBumpmap, m_refFilterLM, m_refFilterTex;
    ConVarRef m_refRadiosity, m_refWLMin, m_refRTT, m_refDepthRes, m_refSlopeBias;
    int   m_iOldSpecular, m_iOldBumpmap, m_iOldFilterLM, m_iOldFilterTex;
    int   m_iOldRadiosity, m_iOldRTT, m_iOldDepthRes;
    float m_flOldWLMin, m_flOldSlopeBias, m_flOldCSMFilter;

    // Rim light cache — refreshed at 1Hz rather than per-frame.
    float      m_rimR, m_rimG, m_rimB;      // light colour
    float      m_rimLinearAtten, m_rimFOV, m_rimFarZ;
    Quaternion m_rimQuat;                   // orientation (constant direction)
    Vector     m_rimDir;                    // rim direction vector
    float      m_flRimCacheTime;            // gpGlobals->curtime of last refresh
};

static CLightingOverhaulSystem s_LightingOverhaulSystem;

CON_COMMAND( r_lighting_overhaul_toggle,
    "Toggle r_lighting_overhaul on/off. Bind: bind F5 r_lighting_overhaul_toggle" )
{
    int iNew = r_lighting_overhaul.GetBool() ? 0 : 1;
    r_lighting_overhaul.SetValue( iNew );
    Msg( "[LightingOverhaul] %s\n", iNew ? "ON" : "OFF" );
}


//=============================================================================
//  Shared helper used by all three material proxies and the sun-shafts effect.
//  Reads the current sun colour from the csm_color_r/g/b ConVars that
//  sky_tod.cpp writes every frame.  Returns a normalised linear RGB Vector.
//=============================================================================
// Static ConVarRefs — constructed once on first call (one string hash lookup each),
// then the same objects are reused on every subsequent call with zero lookup cost.
static void GetSunColorFromCSM( Vector &vecColor )
{
    vecColor.Init( 1.f, 1.f, 1.f );
    static ConVarRef s_r( "csm_color_r" );
    static ConVarRef s_g( "csm_color_g" );
    static ConVarRef s_b( "csm_color_b" );
    if ( s_r.IsValid() && s_g.IsValid() && s_b.IsValid() )
    {
        float fr = s_r.GetFloat()/255.f, fg = s_g.GetFloat()/255.f, fb = s_b.GetFloat()/255.f;
        if ( fr+fg+fb > 0.01f )
            vecColor.Init( fr, fg, fb );
    }
}


//=============================================================================
//
//  SUNSHAFTS SCREEN-SPACE EFFECT
//
//=============================================================================

static ConVar r_sunshafts(
    "r_sunshafts", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Enable sun shaft / god ray screen-space effect. Requires r_lighting_overhaul 1." );

static ConVar r_sunshafts_strength(
    "r_sunshafts_strength", "0.12", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Brightness of the sun shaft overlay (0.0 - 1.0)." );

static ConVar r_sunshafts_width(
    "r_sunshafts_width", "0.6", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Radial blur width; higher = longer shafts." );

static ConVar r_sunshafts_samples(
    "r_sunshafts_samples", "16", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Number of radial blur samples (8-32). Higher = smoother, more expensive." );

class CSunShaftsEffect : public IScreenSpaceEffect
{
public:
    CSunShaftsEffect() : m_bEnabled(false) {}

    virtual void Init() OVERRIDE
    {
        // We drive this purely via DrawScreenSpaceRectangle with the
        // engine's built-in "engine/writez" and add materials.  No custom
        // shader needed — we use the existing _rt_FullFrameFB textures
        // and a tinted additive blit.
        m_matAdd.Init( "dev/add_model", TEXTURE_GROUP_OTHER );

        // If that material doesn't exist in this build, fall back to a
        // simpler unlitgeneric additive.  Either way we get a blend.
        if ( IsErrorMaterial( m_matAdd ) )
            m_matAdd.Init( "engine/occlusionproxy", TEXTURE_GROUP_OTHER );

        m_bEnabled = false;
    }

    virtual void Shutdown() OVERRIDE
    {
        m_matAdd.Shutdown();
    }

    virtual void SetParameters( KeyValues *params ) OVERRIDE {}

    virtual void Enable( bool bEnable ) OVERRIDE
    {
        m_bEnabled = bEnable && r_sunshafts.GetBool();
    }

    virtual bool IsEnabled() OVERRIDE
    {
        return m_bEnabled && r_sunshafts.GetBool() && r_lighting_overhaul.GetBool();
    }

    virtual void Render( int x, int y, int w, int h ) OVERRIDE
    {
        if ( !IsEnabled() )
            return;

        if ( !g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_b() )
            return;

        // We need the frame buffer captured
        Rect_t actualRect;
        UpdateScreenEffectTexture( 0, x, y, w, h, false, &actualRect );
        ITexture *pFBTex = GetFullFrameFrameBufferTexture( 0 );
        if ( !pFBTex )
            return;

        CMatRenderContextPtr pRenderContext( materials );

        // Compute sun screen position.  We use a simple hardcoded sun
        // direction (the same default in lighting_overhaul.cpp) projected
        // to screen space via the current view.
        Vector vecSunDir( 0.5f, 0.5f, -0.7f );
        VectorNormalize( vecSunDir );

        // Push the sun far out along its direction and project to NDC
        const float flSunDist = 100000.f;
        Vector vecSunWorld = MainViewOrigin() + vecSunDir * flSunDist;

        Vector vecSunScreen;
        bool bBehind = false;
        {
            const VMatrix &worldToScreen = engine->WorldToScreenMatrix();
            float w4 = worldToScreen[3][0]*vecSunWorld.x + worldToScreen[3][1]*vecSunWorld.y
                      + worldToScreen[3][2]*vecSunWorld.z + worldToScreen[3][3];
            if ( w4 < 0.001f )
                bBehind = true;
            else
            {
                float invW = 1.f / w4;
                vecSunScreen.x = (worldToScreen[0][0]*vecSunWorld.x + worldToScreen[0][1]*vecSunWorld.y
                                + worldToScreen[0][2]*vecSunWorld.z + worldToScreen[0][3]) * invW;
                vecSunScreen.y = (worldToScreen[1][0]*vecSunWorld.x + worldToScreen[1][1]*vecSunWorld.y
                                + worldToScreen[1][2]*vecSunWorld.z + worldToScreen[1][3]) * invW;
                vecSunScreen.z = 0.f;
            }
        }

        // If sun is behind the camera, fade out quickly
        if ( bBehind )
            return;

        // NDC (-1..1) to UV (0..1)
        float flSunU = vecSunScreen.x * 0.5f + 0.5f;
        float flSunV = 1.f - ( vecSunScreen.y * 0.5f + 0.5f );

        // Clamp contribution — fade when sun is near edge of screen
        float flEdgeFade = 1.f - 2.f * MAX( fabsf(flSunU - 0.5f), fabsf(flSunV - 0.5f) );
        flEdgeFade = clamp( flEdgeFade * 2.f, 0.f, 1.f );
        if ( flEdgeFade < 0.01f )
            return;

        // Get sun colour tint
        Vector vecSunColor;
        GetSunColorFromCSM( vecSunColor );

        float flStrength = r_sunshafts_strength.GetFloat() * flEdgeFade;
        int nSamples = clamp( r_sunshafts_samples.GetInt(), 8, 32 );
        float flBlurWidth = r_sunshafts_width.GetFloat() / (float)nSamples;

        // Multi-sample radial blur: step from current pixel toward sun centre,
        // accumulate, then blit additively.  We implement this as N sequential
        // additive DrawScreenSpaceRectangle calls with sub-pixel offsets,
        // which approximates the integral cheaply without a custom shader.
        // Each step shifts the source rect toward the sun by one step width.

        float flAlphaPerSample = flStrength / (float)nSamples;

        IMaterial *pAddMat = m_matAdd;
        if ( !pAddMat || IsErrorMaterial( pAddMat ) )
            return;

        // Set the material colour/alpha to tinted sun colour
        pAddMat->AlphaModulate( flAlphaPerSample );
        pAddMat->ColorModulate(
            vecSunColor.x * flStrength,
            vecSunColor.y * flStrength,
            vecSunColor.z * flStrength );

        pRenderContext->OverrideDepthEnable( true, false );

        for ( int s = 1; s <= nSamples; ++s )
        {
            float t = (float)s * flBlurWidth;
            // Offset the source rectangle toward the sun position
            float offU = ( flSunU - 0.5f ) * t * (float)w;
            float offV = ( flSunV - 0.5f ) * t * (float)h;

            float srcX0 = (float)actualRect.x      + offU;
            float srcX1 = (float)actualRect.x + (float)actualRect.width  - 1.f + offU;
            float srcY0 = (float)actualRect.y      + offV;
            float srcY1 = (float)actualRect.y + (float)actualRect.height - 1.f + offV;

            pRenderContext->DrawScreenSpaceRectangle(
                pAddMat, x, y, w, h,
                srcX0, srcY0, srcX1, srcY1,
                pFBTex->GetActualWidth(), pFBTex->GetActualHeight() );
        }

        pRenderContext->OverrideDepthEnable( false, true );

        // Reset modulation
        pAddMat->AlphaModulate( 1.f );
        pAddMat->ColorModulate( 1.f, 1.f, 1.f );
    }

private:
    bool             m_bEnabled;
    CMaterialReference m_matAdd;
};

ADD_SCREENSPACE_EFFECT( CSunShaftsEffect, sunshafts );


//=============================================================================
//
//  GGX SPECULAR MATERIAL PROXY
//
//  Drives $phongboost using a GGX NDF approximation so that VertexLitGeneric
//  materials with $phong 1 get soft microfacet highlights instead of Phong.
//
//  VMT usage:
//    "Proxies"
//    {
//        "GGXSpecular"
//        {
//            "$roughness"    "0.3"    // 0=mirror, 1=fully diffuse
//        }
//    }
//
//=============================================================================

static ConVar r_ggx_enable(
    "r_ggx_enable", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Enable GGX specular material proxy on materials that request it." );

static ConVar r_ggx_default_roughness(
    "r_ggx_default_roughness", "0.3", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Default GGX roughness if not specified in the VMT (0=mirror, 1=diffuse)." );

class CGGXSpecularProxy : public IMaterialProxy
{
public:
    CGGXSpecularProxy()
        : m_pPhongBoost(NULL)
        , m_pPhongExponent(NULL)
        , m_pPhongEnable(NULL)
        , m_flRoughness( 0.3f )
    {}

    virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) OVERRIDE
    {
        bool bFound;

        m_pPhongBoost    = pMaterial->FindVar( "$phongboost",    &bFound, false );
        m_pPhongExponent = pMaterial->FindVar( "$phongexponent", &bFound, false );
        m_pPhongEnable   = pMaterial->FindVar( "$phong",         &bFound, false );

        // Read per-material roughness override from VMT proxy block
        m_flRoughness = pKeyValues->GetFloat( "$roughness", r_ggx_default_roughness.GetFloat() );
        m_flRoughness = clamp( m_flRoughness, 0.02f, 1.f );

        // Pre-compute GGX->Phong mapping once — OnBind is called every frame
        // per material instance, so avoiding this math there is worthwhile.
        float alpha  = m_flRoughness * m_flRoughness;
        float alpha2 = alpha * alpha;
        m_flGGXPeak  = clamp( (1.f / ( M_PI_F * alpha2 )) * 0.05f, 0.5f, 8.f );
        m_flPhongExp = clamp( MAX( 2.f / alpha2 - 2.f, 1.f ), 1.f, 256.f );

        // Only apply on materials that actually use phong
        return ( m_pPhongBoost != NULL );
    }

    virtual void OnBind( void *pRenderable ) OVERRIDE
    {
        if ( !r_ggx_enable.GetBool() || !r_lighting_overhaul.GetBool() )
            return;

        if ( !m_pPhongBoost )
            return;

        // m_flGGXPeak and m_flPhongExp are pre-computed at Init() from the
        // material's roughness value, which never changes at runtime.
        // OnBind just pushes the cached results — zero floating-point math here.
        m_pPhongBoost->SetFloatValue( m_flGGXPeak );
        if ( m_pPhongExponent )
            m_pPhongExponent->SetFloatValue( m_flPhongExp );
    }

    virtual IMaterial *GetMaterial() OVERRIDE
    {
        return m_pPhongBoost ? m_pPhongBoost->GetOwningMaterial() : NULL;
    }

    virtual void Release() OVERRIDE { delete this; }

private:
    IMaterialVar *m_pPhongBoost;
    IMaterialVar *m_pPhongExponent;
    IMaterialVar *m_pPhongEnable;
    float         m_flRoughness;
    float         m_flGGXPeak;    // pre-computed from roughness at Init()
    float         m_flPhongExp;   // pre-computed from roughness at Init()
};

EXPOSE_INTERFACE( CGGXSpecularProxy, IMaterialProxy,
    "GGXSpecular" IMATERIAL_PROXY_INTERFACE_VERSION );


//=============================================================================
//
//  PER-PIXEL RIM LIGHT MATERIAL PROXY
//
//  Drives $rimlight, $rimlightexponent, $rimlightboost on VertexLitGeneric.
//  Tints the rim by the current sun colour so it matches the sky.
//
//  VMT usage:
//    "Proxies"
//    {
//        "PixelRimLight"
//        {
//            "$exponent"   "4.0"     // optional; overrides r_pixelrim_exponent
//            "$boost"      "2.0"     // optional; overrides r_pixelrim_boost
//        }
//    }
//
//=============================================================================

static ConVar r_pixelrim_enable(
    "r_pixelrim_enable", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Enable per-pixel rim light material proxy on materials that request it." );

static ConVar r_pixelrim_exponent(
    "r_pixelrim_exponent", "4.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Default Fresnel exponent for the pixel rim light proxy." );

static ConVar r_pixelrim_boost(
    "r_pixelrim_boost", "2.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Default brightness multiplier for the pixel rim light proxy." );

class CPixelRimLightProxy : public IMaterialProxy
{
public:
    CPixelRimLightProxy()
        : m_pRimLight(NULL)
        , m_pRimExponent(NULL)
        , m_pRimBoost(NULL)
        , m_pRimMask(NULL)
        , m_flExponentOverride( -1.f )
        , m_flBoostOverride( -1.f )
    {}

    virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) OVERRIDE
    {
        bool bFound;

        m_pRimLight    = pMaterial->FindVar( "$rimlight",          &bFound, false );
        m_pRimExponent = pMaterial->FindVar( "$rimlightexponent",  &bFound, false );
        m_pRimBoost    = pMaterial->FindVar( "$rimlightboost",     &bFound, false );
        m_pRimMask     = pMaterial->FindVar( "$rimmask",           &bFound, false );

        // Per-VMT overrides
        m_flExponentOverride = pKeyValues->GetFloat( "$exponent", -1.f );
        m_flBoostOverride    = pKeyValues->GetFloat( "$boost",    -1.f );

        // Enable $rimlight on the material unconditionally so we don't need
        // the artist to set it manually.
        if ( m_pRimLight )
            m_pRimLight->SetIntValue( 1 );

        return ( m_pRimLight != NULL );
    }

    virtual void OnBind( void *pRenderable ) OVERRIDE
    {
        if ( !r_pixelrim_enable.GetBool() || !r_lighting_overhaul.GetBool() )
        {
            if ( m_pRimLight ) m_pRimLight->SetIntValue( 0 );
            return;
        }

        if ( !m_pRimLight )
            return;

        m_pRimLight->SetIntValue( 1 );

        float flExp   = ( m_flExponentOverride > 0.f ) ? m_flExponentOverride : r_pixelrim_exponent.GetFloat();
        float flBoost = ( m_flBoostOverride    > 0.f ) ? m_flBoostOverride    : r_pixelrim_boost.GetFloat();

        // Tint boost by sun colour intensity so rim dims at dusk/dawn
        Vector vecSunColor;
        GetSunColorFromCSM( vecSunColor );
        float flTint = ( vecSunColor.x + vecSunColor.y + vecSunColor.z ) / 3.f;
        flTint = clamp( flTint, 0.3f, 1.f );   // keep a minimum so rim is never invisible
        flBoost *= flTint;

        if ( m_pRimExponent ) m_pRimExponent->SetFloatValue( flExp );
        if ( m_pRimBoost    ) m_pRimBoost->SetFloatValue( flBoost );
    }

    virtual IMaterial *GetMaterial() OVERRIDE
    {
        return m_pRimLight ? m_pRimLight->GetOwningMaterial() : NULL;
    }

    virtual void Release() OVERRIDE { delete this; }

private:
    IMaterialVar *m_pRimLight;
    IMaterialVar *m_pRimExponent;
    IMaterialVar *m_pRimBoost;
    IMaterialVar *m_pRimMask;
    float         m_flExponentOverride;
    float         m_flBoostOverride;
};

EXPOSE_INTERFACE( CPixelRimLightProxy, IMaterialProxy,
    "PixelRimLight" IMATERIAL_PROXY_INTERFACE_VERSION );

//=============================================================================
//
//  CUBEMAP TINT MATERIAL PROXY  ("EnvMapTint")
//
//  Colours the $envmaptint parameter to match the current time-of-day sun
//  colour written by sky_tod.cpp into the csm_color_r/g/b ConVars.
//
//  At solar noon the tint is neutral (white).  At dawn/dusk it shifts warm
//  orange-amber.  The strength of the tint is controlled by r_envmaptint_strength
//  (0 = always white, 1 = full sun colour).  A small minimum brightness is
//  kept so reflections never go fully black at the arc edges.
//
//  Source's env_cubemap textures are baked at compile time and cannot change
//  their geometry at runtime.  This proxy changes only the colour multiplier,
//  so reflections on metal surfaces will shift from cool blue-white at noon
//  to warm amber at sunset — reading as real reflected light even though the
//  underlying cubemap image is static.
//
//  VMT usage (VertexLitGeneric or LightmappedGeneric with $envmap):
//    "Proxies"
//    {
//        "EnvMapTint"
//        {
//            // Optional per-material strength override (0.0 - 1.0)
//            // If omitted, r_envmaptint_strength is used.
//            "$strength"   "0.7"
//        }
//    }
//
//  ConVars:
//    r_envmaptint_enable     — master enable (default 1)
//    r_envmaptint_strength   — global tint strength (default 0.55)
//    r_envmaptint_minbright  — minimum luminance kept even in darkness (default 0.15)
//
//=============================================================================

static ConVar r_envmaptint_enable(
    "r_envmaptint_enable", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Enable the EnvMapTint proxy that colours env_cubemap reflections with\n"
    "the current time-of-day sun colour. Requires r_lighting_overhaul 1." );

static ConVar r_envmaptint_strength(
    "r_envmaptint_strength", "0.55", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "How strongly the TOD sun colour tints $envmaptint.\n"
    "  0.0 = always neutral white (no TOD tint)\n"
    "  1.0 = full sun colour\n"
    "  Default 0.55 gives a subtle but visible warm/cool shift." );

static ConVar r_envmaptint_minbright(
    "r_envmaptint_minbright", "0.15", FCVAR_ARCHIVE | FCVAR_CLIENTDLL,
    "Minimum per-channel brightness of the envmaptint even at night/dusk.\n"
    "Prevents reflections going completely dark at the arc edges." );

class CEnvMapTintProxy : public IMaterialProxy
{
public:
    CEnvMapTintProxy()
        : m_pEnvMapTint( NULL )
        , m_flStrengthOverride( -1.f )
    {}

    virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) OVERRIDE
    {
        bool bFound;
        m_pEnvMapTint = pMaterial->FindVar( "$envmaptint", &bFound, false );

        // Per-VMT strength override
        m_flStrengthOverride = pKeyValues->GetFloat( "$strength", -1.f );

        // We can still operate even if $envmaptint wasn't pre-declared —
        // FindVar will create it.  Return true as long as we got the var.
        return ( m_pEnvMapTint != NULL );
    }

    virtual void OnBind( void *pRenderable ) OVERRIDE
    {
        if ( !m_pEnvMapTint )
            return;

        if ( !r_envmaptint_enable.GetBool() || !r_lighting_overhaul.GetBool() )
        {
            // Overhaul off — restore neutral white so the static cubemap
            // shows at its authored brightness.
            m_pEnvMapTint->SetVecValue( 1.f, 1.f, 1.f );
            return;
        }

        // Read sun colour (linear 0..1 per channel) from sky_tod convars.
        Vector vecSun( 1.f, 1.f, 1.f );
        GetSunColorFromCSM( vecSun );

        // Compute luminance to drive the brightness of the tint.
        // The sun is at peak brightness (1,1,1) at solar noon and dimmer/
        // more coloured at the horizon, so this naturally darkens reflections
        // as the match approaches dawn/dusk without any extra threshold logic.
        float flLuma = vecSun.x * 0.2126f + vecSun.y * 0.7152f + vecSun.z * 0.0722f;
        flLuma = clamp( flLuma, 0.f, 1.f );

        float flStrength = ( m_flStrengthOverride >= 0.f )
            ? m_flStrengthOverride
            : r_envmaptint_strength.GetFloat();
        flStrength = clamp( flStrength, 0.f, 1.f );

        float flMinBright = r_envmaptint_minbright.GetFloat();

        // Lerp from white toward the sun colour.
        // At flStrength=0 we get (1,1,1).  At flStrength=1 we get the raw sun colour.
        float r = Lerp( flStrength, 1.f, vecSun.x );
        float g = Lerp( flStrength, 1.f, vecSun.y );
        float b = Lerp( flStrength, 1.f, vecSun.z );

        // Scale down during low-light conditions (dawn/dusk/moon) but keep
        // the minimum brightness floor so reflections are always visible.
        float flScale = Lerp( flStrength, 1.f, flLuma );
        flScale = MAX( flScale, flMinBright );

        m_pEnvMapTint->SetVecValue(
            MAX( r * flScale, flMinBright ),
            MAX( g * flScale, flMinBright ),
            MAX( b * flScale, flMinBright ) );
    }

    virtual IMaterial *GetMaterial() OVERRIDE
    {
        return m_pEnvMapTint ? m_pEnvMapTint->GetOwningMaterial() : NULL;
    }

    virtual void Release() OVERRIDE { delete this; }

private:
    IMaterialVar *m_pEnvMapTint;
    float         m_flStrengthOverride;
};

EXPOSE_INTERFACE( CEnvMapTintProxy, IMaterialProxy,
    "EnvMapTint" IMATERIAL_PROXY_INTERFACE_VERSION );
