//=============================================================================//
//
// tier3_lighting.cpp — TF2 Vintage Tier 3 Lighting Improvements
//
// All effects here are pure game-DLL; no shader source files are required.
// They work by driving existing compiled shader parameters via the material
// proxy and screen-space effect systems that Source already exposes.
//
// CONTENTS
//
// 1. SUNSHAFTS SCREEN-SPACE EFFECT
//    Registered as "sunshafts" via ADD_SCREENSPACE_EFFECT.
//    Uses two passes on _rt_FullFrameFB:
//      Pass 1 — radial blur outward from the projected sun position,
//               using engine/occlusionproxy or a tinted copy of the FB.
//      Pass 2 — additive blend of the blurred result back onto the scene.
//    Enabled/disabled by lighting_overhaul.cpp via g_pScreenSpaceEffects.
//    ConVars: r_sunshafts, r_sunshafts_strength, r_sunshafts_width
//
// 2. GGX SPECULAR MATERIAL PROXY  ("GGXSpecular")
//    Drop-in upgrade for VertexLitGeneric's $phong path.
//    Add to any VMT:   "Proxies" { "GGXSpecular" { "$roughness" "0.3" } }
//    The proxy computes a GGX lobe distribution coefficient each frame
//    and writes it into $phongboost, giving soft metallic highlights
//    instead of the harsh Ward/Phong default.
//    ConVars: r_ggx_enable, r_ggx_default_roughness
//
// 3. PER-PIXEL RIM LIGHT MATERIAL PROXY  ("PixelRimLight")
//    Drives $rimlight, $rimlightexponent, and $rimlightboost on any
//    VertexLitGeneric material, keyed to the current light_environment
//    colour stored in csm_color_r/g/b.
//    Add to any VMT:   "Proxies" { "PixelRimLight" { } }
//    ConVars: r_pixelrim_enable, r_pixelrim_exponent, r_pixelrim_boost
//
//=============================================================================//

#include "cbase.h"
#include "ScreenSpaceEffects.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "KeyValues.h"
#include "view_scene.h"
#include "rendertexture.h"
#include "c_baseplayer.h"
#include "tier0/icommandline.h"
#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Forward — the overhaul master switch
extern ConVar r_lighting_overhaul;

//=============================================================================
//  Shared helper: read the current sun direction from light_environment angles
//  stored by csm_autospawn into the csm_color_* convars.
//  Direction is approximated from a simple default; colour is read exactly.
//=============================================================================
static void GetSunColorFromCSM( Vector &vecColor )
{
    vecColor.Init( 1.f, 1.f, 1.f );
    ConVarRef r("csm_color_r"), g("csm_color_g"), b("csm_color_b");
    if ( r.IsValid() && g.IsValid() && b.IsValid() )
    {
        float fr = r.GetFloat()/255.f, fg = g.GetFloat()/255.f, fb = b.GetFloat()/255.f;
        if ( fr+fg+fb > 0.01f )
            vecColor.Init( fr, fg, fb );
    }
}


//=============================================================================
//
//  1. SUNSHAFTS SCREEN-SPACE EFFECT
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
//  2. GGX SPECULAR MATERIAL PROXY
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

        // Only apply on materials that actually use phong
        return ( m_pPhongBoost != NULL );
    }

    virtual void OnBind( void *pRenderable ) OVERRIDE
    {
        if ( !r_ggx_enable.GetBool() || !r_lighting_overhaul.GetBool() )
            return;

        if ( !m_pPhongBoost )
            return;

        // GGX NDF approximation for a directional specular peak.
        // We compute the GGX distribution D for a half-angle of ~15° (a
        // typical viewer-sun angle for outdoor TF2 maps) and map it to a
        // phong boost and exponent pair that approximates the GGX lobe shape.
        //
        //   D_GGX(alpha) = alpha^2 / (pi * (cos^2(theta) * (alpha^2 - 1) + 1)^2)
        //
        // At theta=0 (specular peak), D simplifies to: 1 / (pi * alpha^2)
        // We use alpha = roughness^2 (Disney remapping).

        float alpha  = m_flRoughness * m_flRoughness;   // Disney alpha
        float alpha2 = alpha * alpha;

        // Peak GGX value at theta=0; normalised to [0,1] range for phongboost
        float flGGXPeak = 1.f / ( M_PI_F * alpha2 );
        flGGXPeak = clamp( flGGXPeak * 0.05f, 0.5f, 8.f );  // scale to useful range

        // Convert roughness to an equivalent Phong exponent so the lobe
        // width matches the GGX distribution width:
        //   n_phong ≈ 2 / alpha^2 - 2  (Beckmann <-> Phong mapping)
        float flPhongExp = MAX( 2.f / alpha2 - 2.f, 1.f );
        flPhongExp = clamp( flPhongExp, 1.f, 256.f );

        m_pPhongBoost->SetFloatValue( flGGXPeak );
        if ( m_pPhongExponent )
            m_pPhongExponent->SetFloatValue( flPhongExp );
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
};

EXPOSE_INTERFACE( CGGXSpecularProxy, IMaterialProxy,
    "GGXSpecular" IMATERIAL_PROXY_INTERFACE_VERSION );


//=============================================================================
//
//  3. PER-PIXEL RIM LIGHT MATERIAL PROXY
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
