//=============================================================================//
//
// csm_autospawn.cpp
//
// Automatically spawns Fake-CSM entities from the map's light_environment
// without requiring any Hammer work.
//
// The CSM system is enabled/disabled by the CLIENT-SIDE r_lighting_overhaul
// convar (defined in lighting_overhaul.cpp). This file reads it via ConVarRef.
//
// To disable CSM for an entire game session, pass -csm_disable on the
// command line. This locks the system off regardless of r_lighting_overhaul.
//
// COLOR AUTO-SYNC:
//   Reads the sun color from CEnvLight::GetLightColor() and pushes it into
//   the csm_color_* convars used by c_env_cascade_light.cpp.
//
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "lights.h"
#include "tier1/convar.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar csm_autospawn_quiet(
    "csm_autospawn_quiet", "1", FCVAR_NONE,
    "Suppress non-developer log output from the CSM auto-spawn system." );

//-----------------------------------------------------------------------------
// CAutoCSMSystem
// NOTE: class must be fully defined before the global instance declaration.
//-----------------------------------------------------------------------------
class CAutoCSMSystem : public CAutoGameSystem
{
public:
    CAutoCSMSystem()
        : CAutoGameSystem( "CAutoCSMSystem" )
        , m_hCascadeLight( NULL )
        , m_bAutoSpawned( false )
        , m_bLaunchDisabled( false )
        , m_refOverhaul( "r_lighting_overhaul" )
    {}

    // -------------------------------------------------------------------------
    virtual bool Init() OVERRIDE
    {
        if ( CommandLine()->FindParm( "-csm_disable" ) )
        {
            m_bLaunchDisabled = true;
            Msg( "[AutoCSM] Disabled for this session via -csm_disable.\n" );
        }
        return true;
    }

    // -------------------------------------------------------------------------
    virtual void LevelInitPostEntity() OVERRIDE
    {
        m_hCascadeLight = NULL;
        m_bAutoSpawned  = false;

        if ( m_bLaunchDisabled )
            return;

        // r_lighting_overhaul is a client-side cvar; on a dedicated server
        // the ref will be invalid, and we skip auto-spawn (mapper must place
        // env_cascade_light manually on dedicated servers).
        if ( m_refOverhaul.IsValid() && m_refOverhaul.GetInt() == 0 )
            return;

        // If a mapper-placed env_cascade_light already exists, skip.
        if ( gEntList.FindEntityByClassname( NULL, "env_cascade_light" ) )
        {
            DevMsg( "[AutoCSM] Mapper-placed env_cascade_light found – skipping auto-spawn.\n" );
            return;
        }

        CBaseEntity *pBase = gEntList.FindEntityByClassname( NULL, "light_environment" );
        if ( !pBase )
        {
            DevMsg( "[AutoCSM] No light_environment found – CSM not spawned.\n" );
            return;
        }

        CEnvLight *pEnvLight = dynamic_cast<CEnvLight *>( pBase );
        if ( !pEnvLight )
        {
            DevMsg( "[AutoCSM] light_environment is not CEnvLight – CSM not spawned.\n" );
            return;
        }

        SpawnCSMFromLightEnv( pEnvLight );
    }

    // -------------------------------------------------------------------------
    virtual void LevelShutdownPostEntity() OVERRIDE
    {
        m_hCascadeLight = NULL;
        m_bAutoSpawned  = false;
    }

    // -------------------------------------------------------------------------
    // Public: used by csm_respawn console command.
    // -------------------------------------------------------------------------
    void RespawnCSM()
    {
        static const char *k_szClasses[] =
        {
            "env_cascade_light", "csm_origin", "csm_second", "csm_third", NULL
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

    void SpawnCSMFromLightEnv( CEnvLight *pEnvLight )
    {
        QAngle angSun;
        angSun.x = (float)( -pEnvLight->m_iPitch );
        angSun.y = pEnvLight->GetAbsAngles().y;
        angSun.z = 0.0f;

        Vector4D col = pEnvLight->GetLightColor();
        ConVarRef( "csm_color_r" ).SetValue( (int)col.x );
        ConVarRef( "csm_color_g" ).SetValue( (int)col.y );
        ConVarRef( "csm_color_b" ).SetValue( (int)col.z );
        ConVarRef( "csm_color_a" ).SetValue( (int)col.w );

        CBaseEntity *pCSMEnt = CreateEntityByName( "env_cascade_light" );
        if ( !pCSMEnt )
        {
            Warning( "[AutoCSM] Failed to CreateEntityByName( \"env_cascade_light\" )!\n" );
            return;
        }

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

    EHANDLE  m_hCascadeLight;
    bool     m_bAutoSpawned;
    bool     m_bLaunchDisabled;
    ConVarRef m_refOverhaul;
};

//-----------------------------------------------------------------------------
static CAutoCSMSystem s_AutoCSMSystem;

CON_COMMAND( csm_respawn,
    "Tear down and re-create all auto-spawned Fake-CSM entities from the "
    "current map's light_environment. No map reload required." )
{
    s_AutoCSMSystem.RespawnCSM();
}
