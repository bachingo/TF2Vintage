//=============================================================================//
//
// csm_autospawn.cpp
//
// Automatically spawns Fake-CSM entities from the map's light_environment
// without requiring any Hammer work.
//
// MODES (csm_mode convar):
//
//   csm_mode 0  - Disabled entirely
//   csm_mode 1  - CSM active alongside normal RTT shadows
//   csm_mode 2  - CSM active, normal RTT/blob shadows suppressed, and the
//                 two quality convars forced to their recommended values.
//                 THIS IS THE DEFAULT.
//
// DISABLING AT LAUNCH:
//   Pass -csm_disable on the command line to force csm_mode 0 for the entire
//   session and prevent it being changed at runtime. Also un-forces the two
//   quality convars so they revert to the engine/user-set values.
//
// COLOR AUTO-SYNC:
//   Reads the sun color from CEnvLight::GetLightColor() (parsed from the BSP
//   "_light" key and stored in the protected m_vecLightColor member) and
//   pushes it into the csm_color_* convars automatically.
//
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "lights.h"
#include "tier1/convar.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Forward declaration of the global system instance so CON_COMMAND can reach it
//-----------------------------------------------------------------------------
class CAutoCSMSystem;
static CAutoCSMSystem s_AutoCSMSystem;

//-----------------------------------------------------------------------------
// ConVars
//-----------------------------------------------------------------------------
static ConVar csm_mode(
    "csm_mode", "2", FCVAR_ARCHIVE,
    "Fake-CSM auto-spawn mode.\n"
    "  0 = Off (normal shadows only)\n"
    "  1 = CSM alongside normal shadows\n"
    "  2 = Pure CSM: normal shadows suppressed, r_flashlightdepthres and\n"
    "      mat_slopescaledepthbias_shadowmap forced to recommended values (default)\n"
    "Pass -csm_disable at launch to lock this to 0 for the whole session.",
    true, 0, true, 2 );

static ConVar csm_autospawn_quiet(
    "csm_autospawn_quiet", "1", FCVAR_NONE,
    "Suppress non-developer log output from the auto-CSM system." );

//-----------------------------------------------------------------------------
// Quality values forced in mode 2
//-----------------------------------------------------------------------------
static const int   k_iForcedDepthRes   = 4096;
static const float k_flForcedSlopeBias = 4.0f;

//-----------------------------------------------------------------------------
// CAutoCSMSystem
//-----------------------------------------------------------------------------
class CAutoCSMSystem : public CAutoGameSystem
{
public:
    CAutoCSMSystem() : CAutoGameSystem( "CAutoCSMSystem" ),
        m_hCascadeLight( NULL ),
        m_bAutoSpawned( false ),
        m_bShadowsSuppressed( false ),
        m_bQualityForced( false ),
        m_bLaunchDisabled( false ),
        m_iOldDepthRes( 2048 ),
        m_flOldSlopeBias( 1.0f ),
        m_iOldRTT( 1 ),
        m_iOldGameControl( -1 )
    {}

    // -------------------------------------------------------------------------
    // Init: check for -csm_disable once at startup.
    // -------------------------------------------------------------------------
    virtual bool Init() OVERRIDE
    {
        if ( CommandLine()->FindParm( "-csm_disable" ) )
        {
            m_bLaunchDisabled = true;
            csm_mode.SetValue( 0 );
            Msg( "[AutoCSM] Disabled for this session via -csm_disable.\n" );
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // LevelInitPostEntity: called after every entity in the map has spawned.
    // -------------------------------------------------------------------------
    virtual void LevelInitPostEntity() OVERRIDE
    {
        m_hCascadeLight      = NULL;
        m_bAutoSpawned       = false;
        m_bShadowsSuppressed = false;
        m_bQualityForced     = false;

        if ( m_bLaunchDisabled || csm_mode.GetInt() == 0 )
            return;

        // If a mapper-placed env_cascade_light exists, leave it alone but
        // still apply the mode-2 quality/shadow settings.
        if ( gEntList.FindEntityByClassname( NULL, "env_cascade_light" ) )
        {
            DevMsg( "[AutoCSM] Mapper-placed env_cascade_light found – skipping auto-spawn.\n" );
            ApplyMode( csm_mode.GetInt() );
            return;
        }

        // Find the light_environment.
        CBaseEntity *pBase = gEntList.FindEntityByClassname( NULL, "light_environment" );
        if ( !pBase )
        {
            DevMsg( "[AutoCSM] No light_environment – CSM not spawned.\n" );
            return;
        }

        CEnvLight *pEnvLight = dynamic_cast<CEnvLight *>( pBase );
        if ( !pEnvLight )
        {
            DevMsg( "[AutoCSM] light_environment is not CEnvLight – CSM not spawned.\n" );
            return;
        }

        SpawnCSMFromLightEnv( pEnvLight );
        ApplyMode( csm_mode.GetInt() );
    }

    // -------------------------------------------------------------------------
    // LevelShutdownPostEntity: restore all overridden cvars.
    // -------------------------------------------------------------------------
    virtual void LevelShutdownPostEntity() OVERRIDE
    {
        RestoreMode();
        m_hCascadeLight = NULL;
        m_bAutoSpawned  = false;
    }

    // -------------------------------------------------------------------------
    // FrameUpdatePostEntityThink: react to csm_mode changes at runtime.
    // -------------------------------------------------------------------------
    virtual void FrameUpdatePostEntityThink() OVERRIDE
    {
        if ( m_bLaunchDisabled )
            return;

        int  wantMode      = csm_mode.GetInt();
        bool wantSuppressed = ( wantMode == 2 );
        bool wantQuality    = ( wantMode == 2 );

        if ( wantSuppressed != m_bShadowsSuppressed )
            SetShadowSuppression( wantSuppressed );
        if ( wantQuality != m_bQualityForced )
            SetQualityForced( wantQuality );
    }

    // -------------------------------------------------------------------------
    // Public: used by csm_respawn console command.
    // -------------------------------------------------------------------------
    void RespawnCSM()
    {
        static const char *k_szClasses[] =
        {
            "env_cascade_light",
            "csm_origin",
            "csm_second",
            "csm_third",
            NULL
        };

        for ( int i = 0; k_szClasses[i]; ++i )
        {
            CBaseEntity *pEnt = NULL;
            while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, k_szClasses[i] ) ) != NULL )
                UTIL_Remove( pEnt );
        }

        LevelInitPostEntity();
    }

private:

    // -------------------------------------------------------------------------
    // Core spawn: create env_cascade_light seeded from pEnvLight.
    // -------------------------------------------------------------------------
    void SpawnCSMFromLightEnv( CEnvLight *pEnvLight )
    {
        // --- Sun direction ---
        // m_iPitch: degrees below the horizon (public in Fake-CSM lights.h).
        // Yaw: from the entity's abs angles (set by Hammer "angles" key).
        QAngle angSun;
        angSun.x = (float)( -pEnvLight->m_iPitch );
        angSun.y = pEnvLight->GetAbsAngles().y;
        angSun.z = 0.0f;

        // --- Sun color ---
        // GetLightColor() returns raw 0-255 RGBA parsed from the BSP "_light"
        // key, stored in the new protected m_vecLightColor member.
        Vector4D col = pEnvLight->GetLightColor();
        ConVarRef( "csm_color_r" ).SetValue( (int)col.x );
        ConVarRef( "csm_color_g" ).SetValue( (int)col.y );
        ConVarRef( "csm_color_b" ).SetValue( (int)col.z );
        ConVarRef( "csm_color_a" ).SetValue( (int)col.w ); // Hammer brightness scalar

        // --- Create entity ---
        CBaseEntity *pCSMEnt = CreateEntityByName( "env_cascade_light" );
        if ( !pCSMEnt )
        {
            Warning( "[AutoCSM] Failed to CreateEntityByName( \"env_cascade_light\" )!\n" );
            return;
        }

        // uselightenvangles=1 tells CLightOrigin::Spawn() to find
        // light_environment and re-derive pitch+yaw from it directly,
        // which is more accurate than anything we can pass through angles.
        pCSMEnt->KeyValue( "uselightenvangles", "1" );
        pCSMEnt->KeyValue( "enablethird",       "1" );
        pCSMEnt->KeyValue( "enableshadows",     "1" );
        pCSMEnt->KeyValue( "lightworld",        "1" );
        pCSMEnt->KeyValue( "brightnessscale",   "1.0" );

        pCSMEnt->SetAbsOrigin( vec3_origin );
        pCSMEnt->SetAbsAngles( angSun );

        DispatchSpawn( pCSMEnt );
        pCSMEnt->Activate();

        m_hCascadeLight = pCSMEnt;
        m_bAutoSpawned  = true;

        if ( !csm_autospawn_quiet.GetBool() )
        {
            Msg( "[AutoCSM] Spawned from light_environment "
                 "(pitch=%d  yaw=%.1f  color=%.0f,%.0f,%.0f  brightness=%.0f).\n",
                 pEnvLight->m_iPitch, angSun.y,
                 col.x, col.y, col.z, col.w );
        }
        else
        {
            DevMsg( "[AutoCSM] Spawned from light_environment "
                    "(pitch=%d  yaw=%.1f  color=%.0f,%.0f,%.0f  brightness=%.0f).\n",
                    pEnvLight->m_iPitch, angSun.y,
                    col.x, col.y, col.z, col.w );
        }
    }

    // -------------------------------------------------------------------------
    // Apply/restore all state for a given mode.
    // -------------------------------------------------------------------------
    void ApplyMode( int iMode )
    {
        SetShadowSuppression( iMode == 2 );
        SetQualityForced    ( iMode == 2 );
    }

    void RestoreMode()
    {
        if ( m_bShadowsSuppressed ) SetShadowSuppression( false );
        if ( m_bQualityForced     ) SetQualityForced    ( false );
    }

    // -------------------------------------------------------------------------
    // Toggle normal shadow rendering.
    // -------------------------------------------------------------------------
    void SetShadowSuppression( bool bSuppress )
    {
        ConVarRef rtt ( "r_shadowrendertotexture" );
        ConVarRef ctrl( "r_shadows_gamecontrol"  );

        if ( bSuppress )
        {
            m_iOldRTT         = rtt.IsValid()  ? rtt.GetInt()  : 1;
            m_iOldGameControl = ctrl.IsValid() ? ctrl.GetInt() : -1;

            if ( rtt.IsValid()  ) rtt.SetValue( 0 );
            if ( ctrl.IsValid() ) ctrl.SetValue( 0 );

            m_bShadowsSuppressed = true;
            DevMsg( "[AutoCSM] Normal shadows suppressed (mode 2).\n" );
        }
        else
        {
            if ( rtt.IsValid()  ) rtt.SetValue( m_iOldRTT );
            if ( ctrl.IsValid() ) ctrl.SetValue( m_iOldGameControl );

            m_bShadowsSuppressed = false;
            DevMsg( "[AutoCSM] Normal shadows restored.\n" );
        }
    }

    // -------------------------------------------------------------------------
    // Force / un-force the two Fake-CSM quality convars.
    // -------------------------------------------------------------------------
    void SetQualityForced( bool bForce )
    {
        ConVarRef depthRes ( "r_flashlightdepthres"             );
        ConVarRef slopeBias( "mat_slopescaledepthbias_shadowmap" );

        if ( bForce )
        {
            m_iOldDepthRes   = depthRes.IsValid()  ? depthRes.GetInt()   : 2048;
            m_flOldSlopeBias = slopeBias.IsValid() ? slopeBias.GetFloat(): 1.0f;

            if ( depthRes.IsValid()  ) depthRes.SetValue( k_iForcedDepthRes );
            if ( slopeBias.IsValid() ) slopeBias.SetValue( k_flForcedSlopeBias );

            m_bQualityForced = true;
            DevMsg( "[AutoCSM] Quality forced: r_flashlightdepthres=%d  "
                    "mat_slopescaledepthbias_shadowmap=%.1f\n",
                    k_iForcedDepthRes, k_flForcedSlopeBias );
        }
        else
        {
            if ( depthRes.IsValid()  ) depthRes.SetValue( m_iOldDepthRes );
            if ( slopeBias.IsValid() ) slopeBias.SetValue( m_flOldSlopeBias );

            m_bQualityForced = false;
            DevMsg( "[AutoCSM] Quality settings restored.\n" );
        }
    }

    // ---- state ----
    EHANDLE m_hCascadeLight;
    bool    m_bAutoSpawned;
    bool    m_bShadowsSuppressed;
    bool    m_bQualityForced;
    bool    m_bLaunchDisabled;

    int     m_iOldDepthRes;
    float   m_flOldSlopeBias;
    int     m_iOldRTT;
    int     m_iOldGameControl;
};

//-----------------------------------------------------------------------------
// Console command
//-----------------------------------------------------------------------------
CON_COMMAND( csm_respawn,
    "Tear down and re-create all auto-spawned Fake-CSM entities from the "
    "current map's light_environment. No map reload required." )
{
    s_AutoCSMSystem.RespawnCSM();
}
