//=============================================================================//
//
// sky_tod.cpp — TF2 Vintage Fake Time of Day  (rev 2)
//
// WHAT IT DOES
//   Drives the sun's position and colour across the sky over the course of a
//   match by writing into CEnvLight's networked members (m_angSunAngles,
//   m_vecLight, m_vecAmbient).  viewrender.cpp reads those fields every frame
//   through g_pCSMEnvLight for the CSM shadow pass, so shadow direction, rim
//   lights, and sun shafts all follow automatically on every client.
//
// OPTIMIZATIONS vs rev 1
//   • tod_enable 0 = zero cost. FrameUpdatePostEntityThink() returns on the
//     very first line — no trig, no network, no ConVar reads.
//   • r_lighting_overhaul is a CLIENT cvar; the server cannot read it.
//     tod_enable is the server-side master switch that mirrors its intent.
//     When tod_enable is 0, tod is completely inert.
//   • Per-frame trig is skipped unless the hour has changed by > 0.001
//     (~3.6 seconds of movement at default speed).
//   • Angle network writes are additionally throttled to > 0.05° change,
//     preventing NetworkStateChanged() spam at slow sun movement rates.
//   • csm_color_* ConVarRefs are cached at LevelInit — no string hash
//     lookup per frame.
//
// TIME MODES  (tod_time_mode)
//   0 = MATCH    Sun moves proportionally to match time (mp_timelimit).
//                tod_match_hours controls how many in-game hours span the match.
//   1 = REALTIME Sun moves at real wall-clock speed. 1 real second = 1 sun
//                second. Good for long/unlimited time-limit servers.
//   2 = MANUAL   Sun is frozen. Use tod_set_hour to position it.
//
// TOD_START_HOUR special values
//   -1   Auto-detect from map's light_environment baked angles (default)
//   -2   Use server's real local clock  (e.g. 14.5 = 2:30 PM)
//   0-24 Explicit starting hour
//
// ROUND RESET AUTO-DETECTION  (tod_reset_on_round -1)
//   mp_maxrounds > 0  or  mp_winlimit > 0  →  reset ON  (round-limited)
//   only mp_timelimit > 0                  →  reset OFF (time-limited)
//   all zero (pickup)                      →  reset OFF
//
// CONVARS
//   tod_enable           1/0  master switch  (default 1)
//   tod_time_mode        0=match 1=realtime 2=manual  (default 0)
//   tod_match_hours      in-game hours per mp_timelimit  (default 14)
//   tod_start_hour       -1=auto -2=server clock 0-24=explicit
//   tod_round_start_hour first clock hour the match arc covers  (-1 = auto sunrise)
//   tod_round_end_hour   last  clock hour the match arc covers  (-1 = auto sunset)
//   tod_latitude         latitude for elevation math  (default 36.79 = NM Badlands)
//   tod_longitude        observer longitude in degrees W, negative  (default -108.25)
//   tod_tz_meridian      standard meridian of your timezone  (default -105.0 = Mountain)
//   tod_day_of_year      day of year for seasonal declination (1-365, -1 = auto from server date)
//   tod_reset_on_round   -1=auto 0=never 1=always  (default -1)
//   tod_moon_enable      moon below horizon  (default 1)
//   tod_yaw_offset       fixed yaw offset  (default 0)
//   tod_ambient_scale    ambient/direct ratio  (default 0.4)
//   tod_debug            print state every second  (default 0)
//
// COMMANDS
//   tod_set_hour <h>     jump to hour, switch to MANUAL
//   tod_print_state      print current state + server clock
//   tod_round_reset      manually fire round-start reset
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "SkyCamera.h"
#include "igamesystem.h"
#include "lights.h"
#include "tier1/convar.h"
#include "tier0/icommandline.h"
#include "tier0/platform.h"
#include "mathlib/mathlib.h"
#include "teamplayroundbased_gamerules.h"
#include "fogcontroller.h"
#include <time.h>
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// ConVars
//=============================================================================

static ConVar tod_enable(
    "tod_enable", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Enable the fake time-of-day system.\n"
    "Pass -tod_disable at launch to lock off for the entire session." );

static ConVar tod_time_mode(
    "tod_time_mode", "0", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "How the sun advances:\n"
    "  0 = MATCH    — spans tod_match_hours over the full mp_timelimit.\n"
    "  1 = REALTIME — 1 real second = 1 in-game second (wall-clock speed).\n"
    "  2 = MANUAL   — frozen; use tod_set_hour to move it.",
    true, 0, true, 2 );

static ConVar tod_match_hours(
    "tod_match_hours", "14.0", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "MATCH mode: in-game hours spanned over the full mp_timelimit.\n"
    "If mp_timelimit is 0 the system assumes a 30-minute match." );

static ConVar tod_start_hour(
    "tod_start_hour", "-1.0", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Hour of day at match/round start.\n"
    "  -1  = auto-detect from the map's light_environment baked angles.\n"
    "  -2  = use the server's real local clock.\n"
    "  0-24 = explicit hour (e.g. 6.5 = 6:30 AM)." );

static ConVar tod_round_start_hour(
    "tod_round_start_hour", "-1.0", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Clock hour at which the match time arc begins.\n"
    "  -1 = auto: use the real calculated sunrise for the current date and location.\n"
    "  e.g. 10.0 = match always starts at 10:00 AM regardless of season.\n"
    "Must be less than tod_round_end_hour." );

static ConVar tod_round_end_hour(
    "tod_round_end_hour", "-1.0", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Clock hour at which the match time arc ends.\n"
    "  -1 = auto: use the real calculated sunset for the current date and location.\n"
    "  e.g. 16.0 = match always ends at 4:00 PM.\n"
    "Set both to narrow the window: tod_round_start_hour 10 + tod_round_end_hour 16\n"
    "means the full match spans only 10 AM to 4 PM, with a higher sun throughout." );

static ConVar tod_latitude(
    "tod_latitude", "36.79", FCVAR_ARCHIVE,
    "Observer latitude in degrees N.  Default 36.79 = Badlands, NM.\n"
    "Higher latitudes = lower peak sun arc (further from equator)." );

static ConVar tod_longitude(
    "tod_longitude", "-108.25", FCVAR_ARCHIVE,
    "Observer longitude in degrees (negative = West).  Default -108.25 = Badlands, NM.\n"
    "Combined with tod_tz_meridian this corrects solar noon to the right clock hour." );

static ConVar tod_tz_meridian(
    "tod_tz_meridian", "-105.0", FCVAR_ARCHIVE,
    "Standard meridian of your timezone.  Default -105.0 = Mountain Time (UTC-7).\n"
    "Each 15° from UTC corresponds to one hour: Pacific=-120, Mountain=-105, Central=-90, Eastern=-75." );

static ConVar tod_day_of_year(
    "tod_day_of_year", "-1", FCVAR_ARCHIVE,
    "Day of year (1-365) for seasonal sun declination, or -1 to auto-read\n"
    "from the server's real-world date (default -1).\n"
    "  1 = Jan 1   79 = Spring equinox   172 = Summer solstice\n"
    "  265 = Fall equinox   355 = Winter solstice\n"
    "At -1 a July game has a high intense sun; a January game has a low winter arc." );

static ConVar tod_reset_on_round(
    "tod_reset_on_round", "-1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Reset time of day at the start of each round.\n"
    "  -1 = auto: ON for round-limited servers (mp_maxrounds/mp_winlimit),\n"
    "             OFF for time-limited servers (mp_timelimit only).\n"
    "   0 = never.   1 = always.",
    true, -1, true, 1 );

static ConVar tod_moon_enable(
    "tod_moon_enable", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Show a moon when the sun is below the horizon." );

static ConVar tod_yaw_offset(
    "tod_yaw_offset", "0.0", FCVAR_ARCHIVE,
    "Fixed yaw added to the sun direction. Align with map geometry if needed." );

static ConVar tod_ambient_scale(
    "tod_ambient_scale", "0.4", FCVAR_ARCHIVE,
    "Ambient/direct ratio. 0.4 = ambient is 40%% of direct." );

static ConVar tod_debug(
    "tod_debug", "0", FCVAR_NONE,
    "Print sun state to console every second." );

// ---------------------------------------------------------------------------
// Sky colour convars
// ---------------------------------------------------------------------------

static ConVar tod_sky_fog_enable(
    "tod_sky_fog_enable", "1", FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Drive the 3D skybox and world fog colours to match the time of day.\n"
    "Shifts the horizon haze from blue (noon) through orange (dawn/dusk) to\n"
    "dark navy (night).  Requires a sky_camera and/or env_fog_controller in the map." );

static ConVar tod_sky_fog_start(
    "tod_sky_fog_start", "256.0", FCVAR_ARCHIVE,
    "Skybox fog start distance (world units). Fog tint begins here." );

static ConVar tod_sky_fog_end(
    "tod_sky_fog_end", "2048.0", FCVAR_ARCHIVE,
    "Skybox fog end distance (world units). Full fog density at this distance." );

static ConVar tod_sky_fog_blend(
    "tod_sky_fog_blend", "1", FCVAR_ARCHIVE,
    "1 = blend skybox fog colour based on viewing direction (warm horizon on sun side).\n"
    "0 = uniform fog colour in all directions." );

static ConVar tod_sky_fog_density(
    "tod_sky_fog_density", "0.4", FCVAR_ARCHIVE,
    "Max fog density used during dawn/dusk transitions (0-1, default 0.4).\n"
    "0 = no fog change.  1 = completely opaque horizon." );

static ConVar tod_skyname_day(
    "tod_skyname_day", "", FCVAR_ARCHIVE,
    "sv_skyname to use during daylight hours (leave blank to keep map default).\n"
    "Swapped in when the sun is above the horizon." );

static ConVar tod_skyname_dusk(
    "tod_skyname_dusk", "", FCVAR_ARCHIVE,
    "sv_skyname to use during the 30 minutes around sunrise and sunset.\n"
    "Leave blank to skip this transition." );

static ConVar tod_skyname_night(
    "tod_skyname_night", "", FCVAR_ARCHIVE,
    "sv_skyname to use when the sun is below the horizon (moon mode).\n"
    "Leave blank to skip night sky swap." );

// ---------------------------------------------------------------------------
// Replicated convars — server broadcasts resolved location to all clients.
// Clients can run tod_print_state to see what location the server is using.
// Set automatically by ResolveGeo(); do not set manually.
// ---------------------------------------------------------------------------
static ConVar tod_resolved_lat(
    "tod_resolved_lat", "36.79", FCVAR_REPLICATED | FCVAR_NOTIFY,
    "Effective latitude the server resolved for the current map (replicated)." );
static ConVar tod_resolved_lon(
    "tod_resolved_lon", "-108.25", FCVAR_REPLICATED | FCVAR_NOTIFY,
    "Effective longitude the server resolved for the current map (replicated)." );
static ConVar tod_resolved_tz(
    "tod_resolved_tz", "-105.0", FCVAR_REPLICATED | FCVAR_NOTIFY,
    "Effective timezone meridian the server resolved (replicated)." );

//=============================================================================
// Map TOD cfg file loader (Priority A — highest priority)
//
// Looks for  maps/<mapname>_tod.cfg  in the GAME search path.
// Format: three lines — latitude, longitude, tz_meridian (decimal, one per line).
// Lines starting with // are comments and are skipped.
//=============================================================================
// Map geographic location — file-based (Priority B)
//
// At LevelInit, sky_tod looks for:
//   maps/<mapname>_tod.cfg   (in the MOD search path)
//
// File format — three lines, each a plain decimal number:
//   <latitude>     decimal degrees, N positive  (e.g.  36.79)
//   <longitude>    decimal degrees, E positive  (e.g. -108.25)
//   <tz_meridian>  standard meridian of the map's timezone (e.g. -105.0)
//   # optional comment line — ignored
//
// Example — maps/pl_borneo_tod.cfg:
//   2.30
//   113.50
//   105.0
//   # Sarawak, Borneo, Malaysia
//
// CFGs ship bundled with the mod in the maps/ folder for all known TF2
// maps.  For custom or community maps that don't have a cfg, the system
// falls back to the server ConVars (Priority C):
//   tod_latitude / tod_longitude / tod_tz_meridian
//
// Lookup priority:
//   1. maps/<mapname>_tod.cfg  (Priority B — file, zero recompile to add maps)
//   2. Server ConVars          (Priority C — tod_latitude / longitude / tz_meridian)
//=============================================================================

// Attempt to parse a _tod.cfg file for the current map.
// Returns true and fills out params on success; false if no file or parse error.
static bool LoadMapGeoCfg( const char *pszMapName,
                            float &flOutLat, float &flOutLon, float &flOutTZ,
                            char *pszOutLocation = NULL, int nLocationBuf = 0 )
{
    char szPath[MAX_PATH];
    Q_snprintf( szPath, sizeof(szPath), "maps/%s_tod.cfg", pszMapName );

    if ( !filesystem->FileExists( szPath, "MOD" ) )
        return false;

    FileHandle_t fh = filesystem->Open( szPath, "r", "MOD" );
    if ( fh == FILESYSTEM_INVALID_HANDLE )
        return false;

    // Read up to 3 non-comment, non-blank lines; each must be a bare float.
    float vals[3] = { 0.f, 0.f, 0.f };
    int   nRead   = 0;
    char  szLine[128];

    while ( nRead < 3 && filesystem->ReadLine( szLine, sizeof(szLine), fh ) )
    {
        // Strip leading whitespace and \r\n
        char *p = szLine;
        while ( *p == ' ' || *p == '\t' ) ++p;
        // Strip trailing whitespace / CR LF
        int len = Q_strlen( p );
        while ( len > 0 && ( p[len-1] == '\r' || p[len-1] == '\n' || p[len-1] == ' ' ) )
            p[--len] = '\0';

        if ( *p == '\0' || *p == '#' || *p == '/' )
            continue;   // skip blank lines and comments

        char *pEnd = NULL;
        float fVal = (float)strtod( p, &pEnd );
        if ( pEnd == p )
        {
            // Not a number — malformed cfg; bail out rather than use garbage.
            filesystem->Close( fh );
            Warning( "[SkyTOD] Malformed _tod.cfg '%s' at value %d — ignoring.\n",
                     szPath, nRead );
            return false;
        }
        vals[nRead++] = fVal;
    }

    filesystem->Close( fh );

    if ( nRead < 3 )
    {
        Warning( "[SkyTOD] _tod.cfg '%s' has fewer than 3 values — ignoring.\n", szPath );
        return false;
    }

    // Sanity-check ranges
    if ( vals[0] < -90.f || vals[0] > 90.f )
    {
        Warning( "[SkyTOD] _tod.cfg '%s': latitude %.2f out of range [-90,90].\n",
                 szPath, vals[0] );
        return false;
    }
    if ( vals[1] < -180.f || vals[1] > 180.f )
    {
        Warning( "[SkyTOD] _tod.cfg '%s': longitude %.2f out of range [-180,180].\n",
                 szPath, vals[1] );
        return false;
    }
    if ( vals[2] < -180.f || vals[2] > 180.f )
    {
        Warning( "[SkyTOD] _tod.cfg '%s': tz_meridian %.2f out of range [-180,180].\n",
                 szPath, vals[2] );
        return false;
    }

    flOutLat = vals[0];
    flOutLon = vals[1];
    flOutTZ  = vals[2];

    // Optional 4th value line: location name string.
    // Read the next non-comment, non-blank line verbatim.
    if ( pszOutLocation && nLocationBuf > 0 )
    {
        pszOutLocation[0] = '\0';
        // Re-open to read line 4 — ReadLine cursor is past line 3
        // We left the handle open above; re-open to get the next line.
        // (The handle was already closed above; re-open for line 4.)
        FileHandle_t fh4 = filesystem->Open( szPath, "r", "MOD" );
        if ( fh4 != FILESYSTEM_INVALID_HANDLE )
        {
            int nSkipped = 0;
            char szL[128];
            while ( nSkipped < 3 && filesystem->ReadLine( szL, sizeof(szL), fh4 ) )
            {
                char *p4 = szL;
                while ( *p4 == ' ' || *p4 == '\t' ) ++p4;
                if ( *p4 == '\0' || *p4 == '#' || *p4 == '/' ) continue;
                ++nSkipped;
            }
            // nSkipped==3 means we consumed the 3 data lines; next ReadLine = line 4
            if ( filesystem->ReadLine( szL, sizeof(szL), fh4 ) )
            {
                char *p4 = szL;
                while ( *p4 == ' ' || *p4 == '\t' ) ++p4;
                // Strip trailing whitespace/newlines
                int l4 = Q_strlen(p4);
                while ( l4 > 0 && (p4[l4-1]=='\r'||p4[l4-1]=='\n'||p4[l4-1]==' ') )
                    p4[--l4] = '\0';
                if ( *p4 && *p4 != '#' && *p4 != '/' )
                    Q_strncpy( pszOutLocation, p4, nLocationBuf );
            }
            filesystem->Close( fh4 );
        }
    }
    return true;
}

//=============================================================================
// Map geographic location table (Priority B-table)
//
// Covers all known TF2 and TF2V maps with real-world coordinates.
// Checked only when no maps/<mapname>_tod.cfg is found (Priority A).
// Server ConVars (tod_latitude / tod_longitude / tod_tz_meridian) are the
// final fallback (Priority C) for any map not in this table.
//
// Priority at LevelInit:
//   A. maps/<mapname>_tod.cfg  (file — zero recompile, mapmaker/server drops it)
//   B. This table             (covers full official + TF2V pool)
//   C. Server ConVars          (tod_latitude / tod_longitude / tod_tz_meridian)
//
// To add a new map without recompiling: drop a maps/<mapname>_tod.cfg next
// to the BSP.  The file is checked first, so it always wins over this table.
//
// Lat/Lon: decimal degrees, N/E positive.  TZ: standard meridian (UTC*15).
//=============================================================================
struct MapGeoEntry_t
{
    const char *pszPrefix;     // matched as exact name or prefix_variant
    float        flLatitude;   // decimal degrees, N positive
    float        flLongitude;  // decimal degrees, E positive (W = negative)
    float        flTZMeridian; // standard meridian for timezone (e.g. -105 = MT)
    const char  *pszLocation;  // human-readable, printed at LevelInit
};

static const MapGeoEntry_t s_MapGeoTable[] =
{
    // ── New Mexico / Arizona / American Southwest ───────────────────
    { "arena_afterlife",                  36.79f,  -108.25f,  -105.0f, "Halloween afterlife" },
    { "arena_badlands",                   36.79f,  -108.25f,  -105.0f, "Badlands, NM" },
    { "arena_generator_final",            36.79f,  -108.25f,  -105.0f, "Southwest generator" },
    { "arena_nucleus",                    36.79f,  -108.25f,  -105.0f, "New Mexico (underground)" },
    { "arena_offblast_final",             36.79f,  -108.25f,  -105.0f, "Southwest missile silo" },
    { "arena_perks",                      36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "arena_ravine",                     45.00f,  -110.50f,  -105.0f, "Rocky Mountain ravine" },
    { "arena_watchtower",                 36.79f,  -108.25f,  -105.0f, "Southwest watchtower" },
    { "arena_well",                       36.79f,  -108.25f,  -105.0f, "New Mexico" },
    { "background01",                     36.79f,  -108.25f,  -105.0f, "Menu background (NM)" },
    { "cp_alloy_rc3",                     36.79f,  -108.25f,  -105.0f, "Southwest industrial" },
    { "cp_amaranth_event_rc1",            36.79f,  -108.25f,  -105.0f, "Southwest Halloween" },
    { "cp_amaranth_rc1b",                 36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "cp_ambush_event",                  36.79f,  -108.25f,  -105.0f, "Halloween NM" },
    { "cp_ashworks",                      39.55f,  -106.50f,  -105.0f, "Colorado Rockies" },
    { "cp_badlands",                      36.79f,  -108.25f,  -105.0f, "Badlands, NM" },
    { "cp_boulder_v5",                    40.01f,  -105.27f,  -105.0f, "Boulder, CO" },
    { "cp_carrier",                       36.79f,  -108.25f,  -105.0f, "Southwest (aircraft carrier)" },
    { "cp_cloak",                         36.79f,  -108.25f,  -105.0f, "Southwest desert base" },
    { "cp_desertion_event",               33.50f,  -112.10f,  -105.0f, "Arizona desert Halloween" },
    { "cp_desertion_rc1",                 33.50f,  -112.10f,  -105.0f, "Arizona desert" },
    { "cp_dusk_event",                    36.79f,  -108.25f,  -105.0f, "Southwest desert Halloween" },
    { "cp_dusk_rc1",                      36.79f,  -108.25f,  -105.0f, "Southwest desert at dusk" },
    { "cp_fastlane",                      39.80f,  -104.90f,  -105.0f, "Colorado plains" },
    { "cp_freaky_fair",                   36.79f,  -108.25f,  -105.0f, "Halloween fairground" },
    { "cp_fulgur",                        36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "cp_furnace_rc",                    36.79f,  -108.25f,  -105.0f, "Southwest furnace works" },
    { "cp_generator_final",               36.79f,  -108.25f,  -105.0f, "Southwest generator" },
    { "cp_hybro_b10",                     36.79f,  -108.25f,  -105.0f, "Southwest industrial" },
    { "cp_metalworks",                    36.79f,  -108.25f,  -105.0f, "New Mexico" },
    { "cp_orange_x3",                     36.79f,  -108.25f,  -105.0f, "Generic test map" },
    { "cp_reckoner",                      39.80f,  -104.90f,  -105.0f, "Colorado industrial" },
    { "cp_spookeyridge",                  36.79f,  -108.25f,  -105.0f, "Halloween SW ridge" },
    { "cp_steel",                         35.50f,  -106.60f,  -105.0f, "New Mexico industrial" },
    { "cp_thundermountain",               33.50f,  -112.10f,  -105.0f, "Arizona canyon country" },
    { "cp_warpath2",                      36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "cp_warpath_a2",                    36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "cp_well",                          36.79f,  -108.25f,  -105.0f, "New Mexico (underground)" },
    { "ctf_2fort",                        36.79f,  -108.25f,  -105.0f, "San Juan County, NM" },
    { "ctf_2fort_invasion",               36.79f,  -108.25f,  -105.0f, "San Juan County, NM" },
    { "ctf_2fortified_v1",                36.79f,  -108.25f,  -105.0f, "New Mexico (2fort variant)" },
    { "ctf_applejack",                    35.50f,  -106.60f,  -105.0f, "New Mexico ranch" },
    { "ctf_convoy_v2",                    36.79f,  -108.25f,  -105.0f, "Southwest convoy" },
    { "ctf_crasher",                      36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "ctf_deceit_b1",                    36.79f,  -108.25f,  -105.0f, "Southwest desert base" },
    { "ctf_deceit_event_b",               36.79f,  -108.25f,  -105.0f, "Southwest desert Halloween" },
    { "ctf_hellfire",                     36.79f,  -108.25f,  -105.0f, "Southwest industrial" },
    { "ctf_helltrain_event",              36.79f,  -108.25f,  -105.0f, "Southwest Halloween train" },
    { "ctf_pelican_peak",                 36.10f,  -112.10f,  -105.0f, "Grand Canyon area, AZ" },
    { "ctf_push_a4",                      36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "ctf_sidewinder",                   33.50f,  -112.10f,  -105.0f, "Arizona desert" },
    { "ctf_thundermountain",              33.50f,  -112.10f,  -105.0f, "Arizona canyon country" },
    { "ctf_well",                         36.79f,  -108.25f,  -105.0f, "New Mexico (underground)" },
    { "ds_extractum",                     36.79f,  -108.25f,  -105.0f, "Southwest extraction" },
    { "itemtest",                         36.79f,  -108.25f,  -105.0f, "New Mexico (test map)" },
    { "koth_badlands",                    36.79f,  -108.25f,  -105.0f, "Badlands, NM" },
    { "koth_bagel_event",                 36.79f,  -108.25f,  -105.0f, "Halloween" },
    { "koth_blowout",                     38.50f,  -109.50f,  -105.0f, "Canyon country, UT" },
    { "koth_demolition",                  36.79f,  -108.25f,  -105.0f, "Southwest demolition" },
    { "koth_dusker",                      36.79f,  -108.25f,  -105.0f, "Southwest dusk" },
    { "koth_highpass",                    40.50f,  -106.80f,  -105.0f, "Colorado high country" },
    { "koth_lakeside_event",              35.70f,  -106.30f,  -105.0f, "Jemez Mountains, NM" },
    { "koth_lakeside_final",              35.70f,  -106.30f,  -105.0f, "Jemez Mountains, NM" },
    { "koth_lazarus",                     36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "koth_mannhole",                    36.79f,  -108.25f,  -105.0f, "Southwest underground" },
    { "koth_megalo",                      33.50f,  -112.10f,  -105.0f, "Arizona" },
    { "koth_megaton",                     36.79f,  -108.25f,  -105.0f, "New Mexico" },
    { "koth_nucleus",                     36.79f,  -108.25f,  -105.0f, "New Mexico (underground reactor)" },
    { "koth_product",                     44.50f,  -110.50f,  -105.0f, "Wyoming alpine" },
    { "koth_rotunda",                     39.55f,  -106.50f,  -105.0f, "Colorado Rockies" },
    { "koth_slasher",                     36.79f,  -108.25f,  -105.0f, "Halloween Southwest" },
    { "koth_slaughter_event",             36.79f,  -108.25f,  -105.0f, "Halloween" },
    { "koth_slime",                       36.79f,  -108.25f,  -105.0f, "Halloween" },
    { "koth_synthetic_event",             36.79f,  -108.25f,  -105.0f, "Halloween" },
    { "koth_toxic",                       36.79f,  -108.25f,  -105.0f, "Southwest toxic waste" },
    { "koth_undergrove_event",            36.79f,  -108.25f,  -105.0f, "Halloween grove" },
    { "koth_vanguard_event_hhh",          36.79f,  -108.25f,  -105.0f, "Halloween vanguard" },
    { "koth_vanguard_rc1",                36.79f,  -108.25f,  -105.0f, "Southwest vanguard" },
    { "koth_viaduct",                     44.50f,  -110.50f,  -105.0f, "Yellowstone region, WY" },
    { "koth_viaduct_event",               44.50f,  -110.50f,  -105.0f, "Yellowstone region, WY" },
    { "mvm_bigrock",                      39.55f,  -106.50f,  -105.0f, "Colorado Rockies" },
    { "mvm_decoy",                        36.79f,  -108.25f,  -105.0f, "New Mexico (MvM camp)" },
    { "mvm_ghost_town",                   36.79f,  -108.25f,  -105.0f, "New Mexico ghost town" },
    { "pd_atom_smash",                    36.79f,  -108.25f,  -105.0f, "Southwest atomic facility" },
    { "pd_circus",                        36.79f,  -108.25f,  -105.0f, "Southwest circus" },
    { "pd_cursed_cove_event",             36.79f,  -108.25f,  -105.0f, "Halloween cove" },
    { "pd_monster_bash",                  36.79f,  -108.25f,  -105.0f, "Halloween bash" },
    { "pd_pit_of_death_event",            36.79f,  -108.25f,  -105.0f, "Halloween pit" },
    { "pl_aquarius",                      33.00f,  -112.00f,  -105.0f, "Arizona desert" },
    { "pl_barnblitz",                     39.80f,  -104.90f,  -105.0f, "Eastern Colorado plains" },
    { "pl_cactuscanyon",                  31.90f,  -110.90f,  -105.0f, "Sonoran Desert, AZ" },
    { "pl_corruption",                    36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "pl_crag_event_b",                  36.79f,  -108.25f,  -105.0f, "Southwest crags Halloween" },
    { "pl_emerge",                        36.79f,  -108.25f,  -105.0f, "Southwest desert" },
    { "pl_fifthcurve_event",              36.79f,  -108.25f,  -105.0f, "Halloween" },
    { "pl_hightower",                     38.50f,  -109.50f,  -105.0f, "Moab area, UT" },
    { "pl_hoodoo_final",                  37.60f,  -112.20f,  -105.0f, "Bryce Canyon area, UT" },
    { "pl_millstone_event",               36.79f,  -108.25f,  -105.0f, "Halloween mill" },
    { "pl_moonlit_b6",                    36.79f,  -108.25f,  -105.0f, "Southwest moonlit" },
    { "pl_odyssey",                       35.00f,  -106.60f,  -105.0f, "New Mexico desert" },
    { "pl_phoenix",                       33.45f,  -112.07f,  -105.0f, "Phoenix, AZ" },
    { "pl_precipice_event_final",         36.79f,  -108.25f,  -105.0f, "Halloween cliffs" },
    { "pl_rumble_event",                  36.79f,  -108.25f,  -105.0f, "Halloween Southwest" },
    { "pl_spineyard",                     36.79f,  -108.25f,  -105.0f, "Southwest boneyard" },
    { "pl_terror_event",                  36.79f,  -108.25f,  -105.0f, "Halloween Southwest" },
    { "pl_thundermountain",               33.50f,  -112.10f,  -105.0f, "Arizona canyon country" },
    { "pl_upward",                        39.55f,  -106.50f,  -105.0f, "Colorado Rockies" },
    { "pl_wutville_event",                36.79f,  -108.25f,  -105.0f, "Halloween" },
    { "plr_cutter",                       36.79f,  -108.25f,  -105.0f, "Southwest industrial" },
    { "plr_hacksaw",                      39.55f,  -106.50f,  -105.0f, "Colorado Rockies sawmill" },
    { "plr_hacksaw_event",                39.55f,  -106.50f,  -105.0f, "Colorado Rockies Halloween" },
    { "plr_hightower",                    38.50f,  -109.50f,  -105.0f, "Moab area, UT" },
    { "plr_hightower_event",              38.50f,  -109.50f,  -105.0f, "Moab area, UT" },
    { "plr_nightfall_final",              36.79f,  -108.25f,  -105.0f, "Southwest nightfall" },
    { "plr_panic_b2",                     36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "tc_meridian_rc3",                  36.79f,  -108.25f,  -105.0f, "American Southwest" },
    { "tow_dynamite",                     36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "tr_target",                        36.79f,  -108.25f,  -105.0f, "New Mexico (training)" },
    { "vsh_maul",                         36.79f,  -108.25f,  -105.0f, "Southwest arena" },
    { "vsh_nucleus",                      36.79f,  -108.25f,  -105.0f, "New Mexico (underground)" },
    { "vsh_outburst",                     36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "vsh_skirmish",                     36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "vsh_tinyrock",                     36.79f,  -108.25f,  -105.0f, "Southwest" },
    { "zi_devastation_final1",            36.79f,  -108.25f,  -105.0f, "Southwest zombie" },
    { "zi_sanitarium",                    36.79f,  -108.25f,  -105.0f, "Southwest sanitarium" },

    // ── California ──────────────────────────────────────────────────
    { "arena_backlot_event",              34.05f,  -118.25f,  -120.0f, "Los Angeles backlot, CA" },
    { "arena_backlot_rc2",                34.05f,  -118.25f,  -120.0f, "Los Angeles backlot, CA" },
    { "cp_brew",                          38.50f,  -122.80f,  -120.0f, "Northern California brewery" },
    { "cp_cargo",                         33.70f,  -118.20f,  -120.0f, "Port of Los Angeles, CA" },
    { "cp_dustbowl",                      35.10f,  -117.80f,  -120.0f, "Mojave Desert, CA" },
    { "cp_junction_final",                37.80f,  -122.20f,  -120.0f, "Bay Area, CA" },
    { "cp_mojave_b2",                     35.10f,  -117.80f,  -120.0f, "Mojave Desert, CA" },
    { "cp_mojave_event",                  35.10f,  -117.80f,  -120.0f, "Mojave Desert, CA" },
    { "cp_sunshine",                      34.10f,  -118.30f,  -120.0f, "Southern California" },
    { "cp_sunshine_event",                34.10f,  -118.30f,  -120.0f, "Southern California" },
    { "ctf_pressure",                     35.10f,  -117.80f,  -120.0f, "Mojave Desert, CA" },
    { "koth_harvest_event",               37.50f,  -120.80f,  -120.0f, "San Joaquin Valley, CA" },
    { "koth_harvest_final",               37.50f,  -120.80f,  -120.0f, "San Joaquin Valley, CA" },
    { "koth_probed",                      35.20f,  -116.90f,  -120.0f, "Mojave Desert, CA (Area 51)" },
    { "pl_badwater",                      36.20f,  -116.40f,  -120.0f, "Badwater Basin, Death Valley CA" },
    { "pl_cashworks",                     37.80f,  -122.20f,  -120.0f, "Bay Area, CA industrial" },
    { "pl_effigy_rc2",                    37.80f,  -119.50f,  -120.0f, "Sierra Nevada, CA" },
    { "pl_frontier_final",                36.20f,  -119.10f,  -120.0f, "Central Valley, CA" },
    { "pl_goldrush",                      37.90f,  -120.50f,  -120.0f, "Gold Country, CA" },
    { "tr_dustbowl",                      35.10f,  -117.80f,  -120.0f, "Mojave Desert, CA" },

    // ── Pacific Northwest ───────────────────────────────────────────
    { "arena_lumberyard",                 47.00f,  -123.80f,  -120.0f, "Pacific Northwest, WA" },
    { "arena_lumberyard_event",           47.00f,  -123.80f,  -120.0f, "Pacific Northwest, WA" },
    { "arena_sawmill",                    47.00f,  -123.80f,  -120.0f, "Olympic Peninsula, WA" },
    { "cp_5gorge",                        48.00f,  -121.50f,  -120.0f, "Cascade Mountains, WA" },
    { "cp_altitude",                      46.50f,  -121.70f,  -120.0f, "Mt Rainier area, WA" },
    { "cp_coldfront",                     64.00f,  -145.00f,  -135.0f, "Interior Alaska" },
    { "cp_conifer",                       45.00f,  -122.70f,  -120.0f, "Oregon conifer forests" },
    { "cp_freight_final1",                45.50f,  -122.70f,  -120.0f, "Portland OR rail yard" },
    { "cp_gorge",                         48.00f,  -121.50f,  -120.0f, "Cascade Mountains, WA" },
    { "cp_gorge_event",                   48.00f,  -121.50f,  -120.0f, "Cascade Mountains, WA" },
    { "cp_hardwood_final",                47.00f,  -123.80f,  -120.0f, "Olympic Peninsula, WA" },
    { "cp_landfall",                      46.00f,  -124.00f,  -120.0f, "Oregon Coast" },
    { "cp_mossrock",                      48.00f,  -121.50f,  -120.0f, "Pacific Northwest" },
    { "cp_overgrown",                     48.00f,  -121.50f,  -120.0f, "Pacific Northwest" },
    { "cp_snakewater_final1",             44.00f,  -123.10f,  -120.0f, "Willamette Valley, OR" },
    { "cp_tidal_v4",                      46.00f,  -124.00f,  -120.0f, "Oregon coast" },
    { "cp_yukon_final",                   60.70f,  -135.10f,  -135.0f, "Yukon Territory, Canada" },
    { "ctf_doublecross",                  48.70f,  -121.80f,  -120.0f, "North Cascades, WA" },
    { "ctf_doublecross_event",            48.70f,  -121.80f,  -120.0f, "North Cascades, WA" },
    { "ctf_doublecross_snowy",            48.70f,  -121.80f,  -120.0f, "North Cascades, WA" },
    { "ctf_gorge",                        48.00f,  -121.50f,  -120.0f, "Cascade Mountains, WA" },
    { "ctf_haarp",                        62.40f,  -145.10f,  -135.0f, "Interior Alaska (HAARP area)" },
    { "ctf_landfall",                     46.00f,  -124.00f,  -120.0f, "Oregon Coast" },
    { "ctf_mach4",                        47.50f,  -122.30f,  -120.0f, "Pacific Northwest facility" },
    { "ctf_sawmill",                      47.00f,  -123.80f,  -120.0f, "Olympic Peninsula, WA" },
    { "ctf_snowfall_final",               48.00f,  -121.50f,  -120.0f, "Cascade Mountains, WA" },
    { "ctf_turbine",                      47.50f,  -122.30f,  -120.0f, "Pacific Northwest, WA" },
    { "ctf_turbine_winter",               47.50f,  -122.30f,  -120.0f, "Pacific Northwest, WA" },
    { "ctf_upstream_a1",                  47.60f,  -122.30f,  -120.0f, "Pacific Northwest river" },
    { "koth_cascade",                     47.80f,  -121.90f,  -120.0f, "Cascade foothills, WA" },
    { "koth_hydraulic_rc1_5",             47.50f,  -122.30f,  -120.0f, "Pacific Northwest" },
    { "koth_maple_ridge_b6",              49.20f,  -122.70f,  -120.0f, "Maple Ridge, BC, Canada" },
    { "koth_maple_ridge_event",           49.20f,  -122.70f,  -120.0f, "Maple Ridge, BC, Canada" },
    { "koth_overcast_final",              47.50f,  -122.30f,  -120.0f, "Pacific Northwest overcast" },
    { "koth_sawmill",                     47.00f,  -123.80f,  -120.0f, "Olympic Peninsula, WA" },
    { "koth_sawmill_event",               47.00f,  -123.80f,  -120.0f, "Olympic Peninsula, WA" },
    { "mvm_mannworks",                    47.50f,  -122.30f,  -120.0f, "Pacific Northwest, WA" },
    { "pass_brickyard",                   44.00f,  -123.00f,  -120.0f, "Pacific Northwest" },
    { "pass_timbertown",                  47.00f,  -123.80f,  -120.0f, "Pacific Northwest logging, WA" },
    { "pl_eclipse_rc2",                   44.00f,  -121.50f,  -120.0f, "Oregon Cascades" },
    { "pl_pier",                          47.60f,  -122.30f,  -120.0f, "Seattle waterfront, WA" },
    { "zi_woods",                         44.00f,  -123.10f,  -120.0f, "Oregon woods (zombie)" },

    // ── Rocky Mountains / Colorado / Utah ───────────────────────────
    { "cp_mountainlab",                   46.50f,  -112.00f,  -105.0f, "Montana mountains" },
    { "cp_snowplow",                      46.50f,  -112.00f,  -105.0f, "Helena area, MT" },
    { "koth_snowtower",                   46.50f,  -112.00f,  -105.0f, "Montana snow tower" },
    { "koth_winter_ridge",                46.50f,  -112.00f,  -105.0f, "Montana winter ridge" },
    { "pd_snowville_event",               46.50f,  -112.00f,  -105.0f, "Winter mountain village" },
    { "pl_chilly",                        46.50f,  -112.00f,  -105.0f, "Montana winter" },
    { "plr_pipeline",                     46.00f,  -112.50f,  -105.0f, "Montana pipeline country" },

    // ── Great Plains / Midwest / East Coast ─────────────────────────
    { "arena_granary",                    37.70f,   -97.30f,   -90.0f, "Great Plains, KS" },
    { "cp_canaveral_5cp",                 28.60f,   -80.60f,   -75.0f, "Cape Canaveral, FL" },
    { "cp_darkmarsh",                     30.00f,   -90.00f,   -90.0f, "Louisiana bayou/swamp" },
    { "cp_foundry",                       42.30f,   -83.00f,   -75.0f, "Detroit area, MI" },
    { "cp_granary",                       37.70f,   -97.30f,   -90.0f, "Great Plains, KS" },
    { "cp_gullywash_final1",              38.90f,   -99.30f,   -90.0f, "Central Kansas" },
    { "cp_powerhouse",                    43.90f,   -75.70f,   -75.0f, "Upstate New York hydro" },
    { "cp_process_final",                 41.90f,   -87.70f,   -90.0f, "Chicago area, IL" },
    { "cp_standin_final",                 39.80f,   -97.00f,   -90.0f, "Kansas plains" },
    { "cp_vanguard",                      28.60f,   -80.60f,   -75.0f, "Cape Canaveral, FL" },
    { "ctf_aerospace_b4",                 28.60f,   -80.60f,   -75.0f, "Cape Canaveral area, FL" },
    { "ctf_foundry",                      42.30f,   -83.00f,   -75.0f, "Detroit area, MI" },
    { "ctf_moonwalk_v2",                  28.60f,   -80.60f,   -75.0f, "Cape Canaveral, FL" },
    { "ctf_npire_v3",                     40.71f,   -74.00f,   -75.0f, "New York area" },
    { "ctf_rocketcity",                   34.73f,   -86.59f,   -90.0f, "Huntsville, AL" },
    { "htf_marshlands",                   30.00f,   -90.00f,   -90.0f, "Louisiana marshlands" },
    { "koth_boardwalk",                   40.00f,   -74.00f,   -75.0f, "New Jersey boardwalk" },
    { "koth_granary",                     37.70f,   -97.30f,   -90.0f, "Great Plains, KS" },
    { "koth_king",                        38.50f,   -98.00f,   -90.0f, "Central Kansas" },
    { "koth_moonshine_event",             38.00f,   -86.00f,   -90.0f, "Appalachian Kentucky" },
    { "koth_oilfield",                    29.90f,   -93.90f,   -90.0f, "Gulf Coast oilfield" },
    { "mvm_coaltown",                     37.70f,   -97.30f,   -90.0f, "Kansas coal country" },
    { "mvm_mannhattan",                   40.71f,   -74.00f,   -75.0f, "New York City, NY" },
    { "pass_district",                    41.90f,   -87.70f,   -90.0f, "Chicago area, IL" },
    { "pd_farmageddon",                   30.50f,   -92.10f,   -90.0f, "Louisiana bayou" },
    { "pd_watergate",                     38.90f,   -77.05f,   -75.0f, "Washington DC waterfront" },
    { "pl_bloodwater",                    29.90f,   -93.90f,   -90.0f, "Gulf Coast, TX/LA bayou" },
    { "pl_breadspace",                    28.60f,   -80.60f,   -75.0f, "Cape Canaveral area, FL" },
    { "pl_coal_event",                    37.70f,   -97.30f,   -90.0f, "Kansas coal (Halloween)" },
    { "pl_rumford_event",                 44.55f,   -70.56f,   -75.0f, "Rumford, Maine" },
    { "pl_sludgepit_event",               30.00f,   -90.00f,   -90.0f, "Louisiana bayou Halloween" },
    { "pl_swiftwater_final1",             29.90f,   -93.90f,   -90.0f, "Gulf Coast, TX/LA" },
    { "rd_asteroid",                      28.60f,   -80.60f,   -75.0f, "Cape Canaveral area, FL" },
    { "sd_doomsday",                      28.60f,   -80.60f,   -75.0f, "Cape Canaveral, FL" },
    { "sd_doomsday_event",                28.60f,   -80.60f,   -75.0f, "Cape Canaveral, FL" },
    { "tc_hydro",                         43.90f,   -75.70f,   -75.0f, "Upstate New York hydro" },
    { "vsh_distillery",                   38.00f,   -86.00f,   -90.0f, "Kentucky whiskey distillery" },
    { "zi_blazehattan",                   40.71f,   -74.00f,   -75.0f, "New York City zombie" },
    { "zi_murky",                         30.00f,   -90.00f,   -90.0f, "Louisiana swamp zombie" },

    // ── Northern Rockies / Canada / Alaska ──────────────────────────
    { "ctf_frosty",                       50.00f,  -110.00f,  -105.0f, "Canadian prairies, winter" },

    // ── Pacific / Hawaii ────────────────────────────────────────────
    { "cp_hadal",                          0.00f,   142.00f,   135.0f, "Pacific deep sea context" },
    { "cp_hadal_b13a",                     0.00f,   142.00f,   135.0f, "Pacific deep sea context" },
    { "cp_lavapit_final",                 19.40f,  -155.30f,  -150.0f, "Hawaii Volcanoes, HI" },
    { "cp_sulfur",                        19.40f,  -155.30f,  -150.0f, "Volcanic Hawaii" },
    { "ctf_penguin_peak",                -77.85f,   166.67f,   180.0f, "Ross Island, Antarctica" },
    { "zi_atoll",                         10.00f,   165.00f,   180.0f, "Pacific atoll (Marshall Islands)" },

    // ── Europe / Middle East / Africa ───────────────────────────────
    { "2koth_abbey",                      51.50f,    -2.00f,     0.0f, "English countryside (abbey)" },
    { "arena_byre",                       56.00f,    -3.50f,     0.0f, "Scottish farmland" },
    { "cp_burghausen",                    48.15f,    12.83f,    15.0f, "Burghausen, Bavaria, Germany" },
    { "cp_cowerhouse",                    51.50f,    -2.00f,     0.0f, "English countryside" },
    { "cp_degrootkeep",                   52.00f,     5.30f,    15.0f, "Netherlands medieval castle" },
    { "cp_degrootkeep_rats",              52.00f,     5.30f,    15.0f, "Netherlands medieval castle" },
    { "cp_egypt_final",                   29.50f,    31.00f,    30.0f, "Cairo area, Egypt" },
    { "cp_fortezza",                      44.40f,     8.90f,    15.0f, "Ligurian coast, Italy" },
    { "cp_frontline_a1",                  51.00f,     3.00f,    15.0f, "Belgian/French WWI frontline" },
    { "cp_frostwatch",                    60.00f,     8.00f,    15.0f, "Scandinavian winter outpost" },
    { "cp_gothic_b3",                     48.20f,    16.37f,    15.0f, "Central Europe gothic" },
    { "cp_gravelpit",                     51.50f,    -0.10f,     0.0f, "England, UK" },
    { "cp_gravelpit_snowy",               51.50f,    -0.10f,     0.0f, "England, UK" },
    { "cp_manor_event",                   51.50f,    -2.00f,     0.0f, "English manor, Halloween" },
    { "cp_redfort_b6",                    28.66f,    77.24f,    82.5f, "Red Fort, Delhi, India" },
    { "cppl_gavle",                       60.67f,    17.10f,    15.0f, "Gavle, Sweden" },
    { "ctf_casbah",                       36.80f,     3.00f,    15.0f, "Algiers, Algeria" },
    { "ctf_casbah3_b4",                   36.80f,     3.00f,    15.0f, "Algiers, Algeria" },
    { "ctf_egypt",                        29.50f,    31.00f,    30.0f, "Cairo area, Egypt" },
    { "ds_mysticruins",                   29.50f,    31.00f,    30.0f, "Egyptian mystic ruins" },
    { "koth_chapel_a10",                  51.50f,    -2.00f,     0.0f, "English countryside chapel" },
    { "koth_chapel_event",                51.50f,    -2.00f,     0.0f, "English chapel Halloween" },
    { "koth_krampus",                     47.80f,    13.04f,    15.0f, "Alpine Austria (Krampus)" },
    { "mvm_rottenburg",                   47.80f,    13.04f,    15.0f, "Bavaria, Germany" },
    { "pd_galleria",                      45.43f,    12.33f,    15.0f, "Venice area, Italy" },
    { "pd_mannsylvania",                  47.50f,    18.50f,    15.0f, "Transylvania, Romania" },
    { "pd_nutcracker",                    47.80f,    13.04f,    15.0f, "Alpine Christmas setting" },
    { "pd_selbyen",                       69.30f,    16.00f,    15.0f, "Northern Norway (Lofoten area)" },
    { "pl_camber",                        50.90f,     0.60f,     0.0f, "Camber, East Sussex, England" },
    { "pl_citadel",                       55.75f,    37.62f,    45.0f, "Moscow area, Russia" },
    { "pl_frostcliff",                    60.50f,     8.00f,    15.0f, "Norwegian cliffs, winter" },
    { "pl_hasslecastle",                  51.50f,    -2.00f,     0.0f, "English castle" },
    { "pl_morrigan_alley_b3",             53.40f,    -2.98f,     0.0f, "Liverpool area, England" },
    { "pl_snowycoast",                    60.50f,     8.00f,    15.0f, "Norwegian mountains" },
    { "pl_venice",                        45.43f,    12.33f,    15.0f, "Venice, Italy" },
    { "plr_matterhorn",                   45.98f,     7.66f,    15.0f, "Matterhorn, Switzerland" },

    // ── Asia / Oceania ──────────────────────────────────────────────
    { "arena_arakawa_b3",                 35.69f,   139.70f,   135.0f, "Arakawa, Tokyo, Japan" },
    { "arena_arakawa_event",              35.69f,   139.70f,   135.0f, "Arakawa, Tokyo, Japan" },
    { "cp_mercenarypark",                  2.30f,   113.50f,   105.0f, "Sarawak, Borneo, Malaysia" },
    { "koth_kong_king",                   22.30f,   114.20f,   120.0f, "Hong Kong SAR" },
    { "koth_suijin",                      35.69f,   139.70f,   135.0f, "Tokyo area, Japan" },
    { "pl_borneo",                         2.30f,   113.50f,   105.0f, "Sarawak, Borneo, Malaysia" },
    { "pl_enclosure_final",                2.30f,   113.50f,   105.0f, "Sarawak, Borneo, Malaysia" },
    { "pl_redship_dc_rc3",                35.50f,   129.32f,   135.0f, "Ulsan, South Korea shipyard" },
    { "plr_bananabay",                     2.30f,   113.50f,   105.0f, "Borneo tropical bay" },

    // ── Southern Hemisphere / Tropics ───────────────────────────────
    { "koth_brazil",                      -3.10f,   -60.00f,   -60.0f, "Amazon Basin, Brazil" },
    { "koth_cachoeira",                   -3.10f,   -60.00f,   -60.0f, "Amazon Basin, Brazil" },
    { "koth_sharkbay",                   -25.90f,   113.60f,   120.0f, "Shark Bay, Western Australia" },
    { "pl_brazil",                        -3.10f,   -60.00f,   -60.0f, "Amazon Basin, Brazil" },
    { "pl_patagonia",                    -51.00f,   -73.00f,   -45.0f, "Patagonia, Argentina" },

    // ── Miscellaneous ─────────────────────────────────────────────────────
    { "koth_los_muertos",                 23.63f,  -102.55f,   -90.0f, "Mexico (Dia de los Muertos)" },
    { "pl_embargo",                       23.10f,   -82.40f,   -75.0f, "Havana, Cuba" },
};
static const int s_nMapGeoEntries = ARRAYSIZE( s_MapGeoTable );

// Returns true if pszMapName matches the table prefix (exact, or followed by
// '_' or a digit to avoid prefix collisions like cp_badlands vs cp_badlandsx).
static bool MapPrefixMatch( const char *pszMapName, const char *pszPrefix )
{
    int n = Q_strlen( pszPrefix );
    if ( Q_strnicmp( pszMapName, pszPrefix, n ) != 0 ) return false;
    char c = pszMapName[n];
    return ( c == '\0' || c == '_' || ( c >= '0' && c <= '9' ) );
}

// Look up a map by name in the table.  Returns true and fills out-params if found.
static bool LookupMapGeoTable( const char *pszMapName,
                                float &flOutLat, float &flOutLon, float &flOutTZ,
                                const char *&pszOutLocation )
{
    for ( int i = 0; i < s_nMapGeoEntries; ++i )
    {
        if ( MapPrefixMatch( pszMapName, s_MapGeoTable[i].pszPrefix ) )
        {
            flOutLat      = s_MapGeoTable[i].flLatitude;
            flOutLon      = s_MapGeoTable[i].flLongitude;
            flOutTZ       = s_MapGeoTable[i].flTZMeridian;
            pszOutLocation = s_MapGeoTable[i].pszLocation;
            return true;
        }
    }
    return false;
}

//=============================================================================
// Helpers
//=============================================================================

// Fills both the fractional clock hour (e.g. 14.5 = 2:30 PM) and the
// 1-based day of year (1-365) from the server's real local time in one call.
static void GetServerLocalTime( float &flOutClockHour, int &iOutDayOfYear )
{
    tm t;
    VCRHook_LocalTime( &t );
    flOutClockHour = (float)t.tm_hour + t.tm_min / 60.f + t.tm_sec / 3600.f;
    iOutDayOfYear  = t.tm_yday + 1;   // tm_yday is 0-based; we use 1-based
}

static float GetServerClockHour()
{
    float flHour; int iDay;
    GetServerLocalTime( flHour, iDay );
    return flHour;
}

static int GetServerDayOfYear()
{
    float flHour; int iDay;
    GetServerLocalTime( flHour, iDay );
    return iDay;
}

// Compute solar declination for a given day of year using Spencer's formula.
// Returns degrees.  Day 172 = Jun 21 (summer solstice, +23.5°).
static float SolarDeclination( int iDayOfYear )
{
    float B = 2.f * M_PI_F * ( iDayOfYear - 1 ) / 365.f;
    return ( 0.006918f
           - 0.399912f * cosf(B)   + 0.070257f * sinf(B)
           - 0.006758f * cosf(2*B) + 0.000907f * sinf(2*B)
           - 0.002697f * cosf(3*B) + 0.001480f * sinf(3*B) )
           * ( 180.f / M_PI_F );
}

// Convert civil clock hour to apparent solar hour at the given longitude.
// Mountain Time (UTC-7) is calibrated to the 105th meridian.  At -108.25° W
// (Badlands NM) the sun peaks 13 minutes after noon clock time.
// formula: solar_hour = clock_hour + (longitude - tz_meridian) / 15
static float ClockToSolarHour( float flClockHour, float flLongitude, float flTZMeridian )
{
    return flClockHour + ( flLongitude - flTZMeridian ) / 15.f;
}

// And the reverse for inverting map baked angles back to clock hour.
static float SolarToClockHour( float flSolarHour, float flLongitude, float flTZMeridian )
{
    return flSolarHour - ( flLongitude - flTZMeridian ) / 15.f;
}

static void HourToSunAngles( float flClockHour, float flLatitude,
                              float flLongitude, float flTZMeridian,
                              int iDayOfYear, float flYawOff,
                              float &flOutPitch, float &flOutYaw )
{
    const float kD2R = M_PI_F / 180.f;
    const float kR2D = 180.f / M_PI_F;

    // Convert clock hour → apparent solar hour at this longitude
    float flSolarHour = ClockToSolarHour( flClockHour, flLongitude, flTZMeridian );

    float flH    = ( flSolarHour - 12.f ) * ( M_PI_F / 12.f );
    float flDecl = SolarDeclination( iDayOfYear ) * kD2R;
    float flLat  = flLatitude * kD2R;

    float flSinAlt = sinf(flLat)*sinf(flDecl) + cosf(flLat)*cosf(flDecl)*cosf(flH);
    float flAltRad = asinf( clamp( flSinAlt, -1.f, 1.f ) );
    flOutPitch     = -( flAltRad * kR2D );   // negative = above horizon in Source

    float flCosAlt = cosf( flAltRad );
    float flAzimuth = 0.f;
    if ( flCosAlt > 0.001f )
    {
        float fC = ( sinf(flDecl) - sinf(flLat)*flSinAlt ) / ( cosf(flLat)*flCosAlt );
        fC        = clamp( fC, -1.f, 1.f );
        flAzimuth = acosf( fC ) * kR2D;
        if ( flSolarHour > 12.f ) flAzimuth = 360.f - flAzimuth;
    }

    flOutYaw = 90.f - flAzimuth + flYawOff;
    while ( flOutYaw >  180.f ) flOutYaw -= 360.f;
    while ( flOutYaw < -180.f ) flOutYaw += 360.f;
}

static float SunAnglesToHour( float flPitch, float flYaw, float flLatitude,
                               float flLongitude, float flTZMeridian, int iDayOfYear )
{
    const float kD2R = M_PI_F / 180.f;
    float flAltRad  = -flPitch * kD2R;
    float flDeclRad = SolarDeclination( iDayOfYear ) * kD2R;
    float flLatRad  = flLatitude * kD2R;

    float flCosH = ( sinf(flAltRad) - sinf(flLatRad)*sinf(flDeclRad) )
                 / ( cosf(flLatRad) * cosf(flDeclRad) );
    flCosH = clamp( flCosH, -1.f, 1.f );
    float flHAngle = acosf( flCosH );

    float flAzimuth = 90.f - flYaw;
    while ( flAzimuth <    0.f ) flAzimuth += 360.f;
    while ( flAzimuth >= 360.f ) flAzimuth -= 360.f;
    bool bPM = ( flAzimuth > 180.f );

    // This gives solar hour; convert back to clock hour
    float flSolarHour = 12.f + ( bPM ? 1.f : -1.f ) * ( flHAngle * 12.f / M_PI_F );
    float flClockHour = SolarToClockHour( flSolarHour, flLongitude, flTZMeridian );
    return clamp( flClockHour, 0.f, 24.f );
}

struct SunColor { float r, g, b, brightness; };

// Compute true sunrise and sunset clock hours from astronomy.
// Uses the standard solar hour-angle formula:
//   cos(HA) = -tan(lat) * tan(decl)
// Then corrects from solar time to clock time via longitude offset.
// Returns false (writes 0/24) on polar day; returns false (writes 12/12) on polar night.
static bool ComputeSunriseSunset( float flLatitude, float flLongitude, float flTZMeridian,
                                   int iDayOfYear,
                                   float &flOutSunrise, float &flOutSunset )
{
    const float kD2R = M_PI_F / 180.f;

    float flDecl = SolarDeclination( iDayOfYear ) * kD2R;
    float flLat  = flLatitude * kD2R;

    float flCosHA = -tanf(flLat) * tanf(flDecl);
    if ( flCosHA <= -1.f )   // polar day
    {
        flOutSunrise = 0.f;
        flOutSunset  = 24.f;
        return true;
    }
    if ( flCosHA >= 1.f )    // polar night
    {
        flOutSunrise = flOutSunset = 12.f;
        return false;
    }

    float flHA_deg = acosf( flCosHA ) * ( 180.f / M_PI_F );

    // Solar times (symmetric around solar noon = 12:00 solar)
    float flRiseSolar = 12.f - flHA_deg / 15.f;
    float flSetSolar  = 12.f + flHA_deg / 15.f;

    // Convert solar → clock via longitude correction
    float flCorrection = ( flLongitude - flTZMeridian ) / 15.f;
    flOutSunrise = flRiseSolar - flCorrection;
    flOutSunset  = flSetSolar  - flCorrection;
    return true;
}

// ---------------------------------------------------------------------------
// ComputeSkyFogColor
//
// Returns the skybox-fog tint colour (0-255 per channel) for a given solar
// hour, based on the actual sunrise/sunset times for the day.
//
// Colour arc (local solar time fractions):
//   pre-dawn    → deep indigo          (0.15, 0.15, 0.35)
//   dawn        → indigo-to-gold blend
//   civil twil  → soft orange-gold     (0.85, 0.55, 0.25)
//   sunrise+30m → warm peach           (0.90, 0.75, 0.55)
//   mid-morning → pale sky blue        (0.60, 0.75, 0.90)
//   solar noon  → bright blue-white    (0.70, 0.85, 1.00)
//   afternoon   → matches mid-morning
//   pre-sunset  → warm peach (mirror)
//   sunset-30m  → deep orange-red      (0.95, 0.45, 0.15)
//   civil twil  → soft gold (mirror)
//   post-dusk   → indigo               (0.15, 0.15, 0.35)
//
// Alpha is always 255 (fog colour; density is controlled separately).
// ---------------------------------------------------------------------------
static color32 ComputeSkyFogColor( float flHour, float flSunrise, float flSunset )
{
    // Normalise to 0..1 across the full day arc (pre-dawn through post-dusk).
    // Give 45-minute twilight wings on each side.
    const float kTwilight = 0.75f;                           // hours of civil twilight
    float flArcStart = flSunrise - kTwilight;
    float flArcEnd   = flSunset  + kTwilight;
    float t = clamp( (flHour - flArcStart) / MAX(flArcEnd - flArcStart, 0.01f), 0.f, 1.f );

    // Key colour stops along t=0..1
    // Each stop: { t, r, g, b }  (linear 0..1 floats, converted to 0-255 at end)
    struct ColorStop { float t, r, g, b; };
    static const ColorStop kStops[] =
    {
        { 0.00f,  0.10f, 0.10f, 0.28f },  // pre-dawn: deep indigo
        { 0.06f,  0.22f, 0.18f, 0.35f },  // late twilight
        { 0.12f,  0.75f, 0.42f, 0.18f },  // horizon gold at sunrise
        { 0.18f,  0.85f, 0.62f, 0.42f },  // warm peach after sunrise
        { 0.28f,  0.52f, 0.70f, 0.88f },  // mid-morning sky blue
        { 0.50f,  0.58f, 0.78f, 1.00f },  // solar noon, bright blue-white
        { 0.72f,  0.52f, 0.70f, 0.88f },  // mid-afternoon (mirror)
        { 0.82f,  0.85f, 0.62f, 0.42f },  // pre-dusk warm peach
        { 0.88f,  0.90f, 0.38f, 0.12f },  // sunset orange-red
        { 0.94f,  0.22f, 0.18f, 0.35f },  // post-dusk twilight
        { 1.00f,  0.10f, 0.10f, 0.28f },  // post-dusk: deep indigo
    };
    const int kNumStops = ARRAYSIZE( kStops );

    // Find bracketing stops and lerp
    float r = kStops[0].r, g = kStops[0].g, b = kStops[0].b;
    for ( int i = 0; i < kNumStops - 1; ++i )
    {
        if ( t >= kStops[i].t && t <= kStops[i+1].t )
        {
            float frac = (t - kStops[i].t) / MAX(kStops[i+1].t - kStops[i].t, 0.0001f);
            r = Lerp( frac, kStops[i].r, kStops[i+1].r );
            g = Lerp( frac, kStops[i].g, kStops[i+1].g );
            b = Lerp( frac, kStops[i].b, kStops[i+1].b );
            break;
        }
    }

    color32 col;
    col.r = (byte)clamp( (int)(r * 255.f + 0.5f), 0, 255 );
    col.g = (byte)clamp( (int)(g * 255.f + 0.5f), 0, 255 );
    col.b = (byte)clamp( (int)(b * 255.f + 0.5f), 0, 255 );
    col.a = 255;
    return col;
}

static SunColor ComputeSunColor( float flHour, float flArcStart, float flArcEnd, bool bMoon )
{
    SunColor col;
    if ( bMoon )
    {
        col.r = 0.55f; col.g = 0.60f; col.b = 0.80f;
        col.brightness = 0.18f;
        return col;
    }
    float t      = clamp( (flHour-flArcStart) / MAX(flArcEnd-flArcStart,0.01f), 0.f, 1.f );
    float flNoon = 1.f - 2.f * fabsf( t - 0.5f );
    float flH    = 1.f - flNoon;
    float bDusk  = (t > 0.5f) ? 1.f : 0.f;
    col.r        = Lerp( flH, 1.f, Lerp(bDusk, 0.85f, 1.00f) );
    col.g        = Lerp( flH, 1.f, Lerp(bDusk, 0.55f, 0.60f) );
    col.b        = Lerp( flH, 1.f, 0.30f );
    float flEdge = clamp( MIN(t,1.f-t) * 12.f, 0.f, 1.f );
    col.brightness = clamp( Lerp(flH*flEdge, 0.95f, 0.25f), 0.05f, 1.f );
    return col;
}

// Forward declaration — implemented as a member in CSkyTODSystem below.
// Left here so call sites outside the class don't need refactoring.
static bool ShouldResetOnRound()
{
    int s = tod_reset_on_round.GetInt();
    if ( s == 1 ) return true;
    if ( s == 0 ) return false;
    // Fallback path (pre-LevelInit): construct ConVarRef locally.
    // After LevelInit the class method with cached refs is used instead.
    ConVarRef mr("mp_maxrounds"), wl("mp_winlimit");
    return ( (mr.IsValid() && mr.GetInt() > 0) || (wl.IsValid() && wl.GetInt() > 0) );
}

//=============================================================================
// Sky colour helpers
//=============================================================================

// Sky colour for a given hour.  Returns an RGBA Color32 suitable for
// fog.colorPrimary.  Near sunrise/sunset the colour is warm orange-amber.
// At solar noon it's light sky-blue.  Below the horizon it's dark navy.
static color32 SkyFogColor( float flHour, float flArcStart, float flArcEnd, bool bMoon )
{
    if ( bMoon )
    {
        color32 col; col.r = 10; col.g = 15; col.b = 35; col.a = 255;
        return col;
    }
    // Delegate to the multi-stop physically-based colour function.
    // Pass the true sunrise/sunset times so the twilight wings are accurate
    // regardless of the playable window (arc start/end).
    return ComputeSkyFogColor( flHour, flArcStart, flArcEnd );
}

// Sky state enum for texture swap decisions
enum SkyPhase_t
{
    SKY_NIGHT = 0,   // moon above horizon
    SKY_DUSK,        // within 30 min of sunrise or sunset
    SKY_DAY,         // full daylight
};

static SkyPhase_t ClassifySkyPhase( float flHour, float flArcStart, float flArcEnd )
{
    if ( flHour < flArcStart || flHour > flArcEnd )
        return SKY_NIGHT;
    float flDuskWindow = 0.5f;   // 30 minutes on each side
    if ( flHour < flArcStart + flDuskWindow || flHour > flArcEnd - flDuskWindow )
        return SKY_DUSK;
    return SKY_DAY;
}

//=============================================================================
// CSkyTODSystem
class CSkyTODSystem : public CAutoGameSystem
{
public:
    CSkyTODSystem()
        : CAutoGameSystem( "CSkyTODSystem" )
        , m_pEnvLight       ( NULL  )
        , m_flCurrentHour        ( 12.f  )
        , m_iCachedDayOfYear     ( -1    )
        , m_flDayOfYearRefreshTime( -1.f )
        , m_flMatchStartTime( 0.f   )
        , m_flMatchDurationSec(0.f  )
        , m_flHourAtMatchStart(12.f )
        , m_flMapDefaultHour( 12.f  )
        , m_flArcStart          ( 6.f    )
        , m_flArcEnd            ( 20.f   )
        , m_flResolvedLat       ( 36.79f )
        , m_flResolvedLon       (-108.25f)
        , m_flResolvedTZ        (-105.0f )
        , m_pszResolvedLocation ( "NM Badlands (default)" )
        , m_bGeoFromTable       ( false  )
        , m_bSessionLocked      ( false  )
        , m_bMoonMode       ( false )
        , m_bLevelReady     ( false )
        , m_flLastDebugPrint( 0.f   )
        , m_crRef( "csm_color_r" )
        , m_cgRef( "csm_color_g" )
        , m_cbRef( "csm_color_b" )
        , m_refMaxRounds( "mp_maxrounds" )
        , m_refWinLimit( "mp_winlimit" )
        , m_refSvSkyname( "sv_skyname" )
        , m_pFogController( NULL )
        , m_pSkyCamera    ( NULL )
        , m_eLastSkyPhase ( SKY_DAY )
    {
        m_lastFogColor.r = m_lastFogColor.g = m_lastFogColor.b = m_lastFogColor.a = 0;
    }

    virtual bool Init() OVERRIDE
    {
        if ( CommandLine()->FindParm( "-tod_disable" ) )
        {
            m_bSessionLocked = true;
            tod_enable.SetValue( 0 );
            Msg( "[SkyTOD] Locked OFF for this session (-tod_disable).\n" );
        }
        return true;
    }

    virtual void LevelInitPostEntity() OVERRIDE
    {
        m_bLevelReady = false;
        m_pEnvLight   = NULL;
        // Re-cache ConVarRefs (they survive level changes but re-caching is cheap)
        m_crRef = ConVarRef( "csm_color_r" );
        m_cgRef = ConVarRef( "csm_color_g" );
        m_cbRef = ConVarRef( "csm_color_b" );

        // Cache fog controller and sky camera
        m_pFogController = FogSystem() ? FogSystem()->GetMasterFogController() : NULL;
        m_pSkyCamera     = GetCurrentSkyCamera();
        m_eLastSkyPhase  = SKY_DAY;   // force first sky-name check
        m_lastFogColor.r = m_lastFogColor.g = m_lastFogColor.b = m_lastFogColor.a = 0;

        if ( m_bSessionLocked || !tod_enable.GetBool() )
            return;

        // Resolve geographic location — must happen before any trig
        ResolveGeo();

        CBaseEntity *pBase = gEntList.FindEntityByClassname( NULL, "light_environment" );
        if ( !pBase )
        {
            Msg( "[SkyTOD] No light_environment found — TOD disabled for this map.\n" );
            return;
        }
        m_pEnvLight = dynamic_cast<CEnvLight *>( pBase );
        if ( !m_pEnvLight )
        {
            Msg( "[SkyTOD] light_environment is not CEnvLight — TOD disabled.\n" );
            return;
        }

        // Detect map's baked default time
        float flP = m_pEnvLight->GetAbsAngles().x;
        float flY = m_pEnvLight->GetAbsAngles().y;
        int iDayOfYear = ResolveDayOfYear();
        m_flMapDefaultHour = SunAnglesToHour(
            flP, flY,
            m_flResolvedLat, m_flResolvedLon,
            m_flResolvedTZ, iDayOfYear );
        // Log the resolved day and season for admin visibility
        {
            const char *pszSeason = "Spring";
            int d = iDayOfYear;
            if      ( d < 80  || d >= 355 ) pszSeason = "Winter";
            else if ( d < 172 )             pszSeason = "Spring";
            else if ( d < 265 )             pszSeason = "Summer";
            else                            pszSeason = "Fall";
            if ( tod_day_of_year.GetInt() < 1 )
                Msg( "[SkyTOD] Season: %s (day %d, auto from server date)\n", pszSeason, d );
            else
                Msg( "[SkyTOD] Season: %s (day %d, configured)\n", pszSeason, d );
        }
        Msg( "[SkyTOD] Map default sun time: %02d:%02d  (pitch=%.1f° yaw=%.1f°)\n",
             HH(m_flMapDefaultHour), MM(m_flMapDefaultHour), flP, flY );

        ResolveArcBounds( iDayOfYear, m_flArcStart, m_flArcEnd );

        float flStart = ResolveStartHour();
        flStart = clamp( flStart, m_flArcStart, m_flArcEnd );

        m_flCurrentHour       = flStart;
        m_flHourAtMatchStart  = flStart;
        m_flMatchStartTime    = gpGlobals->curtime;

        ConVarRef mpTL("mp_timelimit");
        m_flMatchDurationSec  = (mpTL.IsValid() && mpTL.GetInt() > 0)
            ? (float)mpTL.GetInt() * 60.f
            : 30.f * 60.f;

        const char *pModes[] = { "MATCH", "REALTIME", "MANUAL" };
        int iMode = clamp( tod_time_mode.GetInt(), 0, 2 );
        Msg( "[SkyTOD] Mode=%s  StartHour=%02d:%02d  RoundReset=%s\n",
             pModes[iMode], HH(flStart), MM(flStart),
             ShouldResetOnRoundCached() ? "ON (round-limited)" : "OFF (time-limited)" );

        m_bLevelReady = true;
        PushSunState( m_flCurrentHour, true );
    }

    virtual void LevelShutdownPostEntity() OVERRIDE
    {
        m_bLevelReady    = false;
        m_pEnvLight      = NULL;
        m_pFogController = NULL;
        m_pSkyCamera     = NULL;
    }

    virtual void FrameUpdatePostEntityThink() OVERRIDE
    {
        // ---- FAST PATH ----
        // tod_enable 0  →  one bool check, return. Zero cost.
        if ( !m_bLevelReady || m_bSessionLocked || !tod_enable.GetBool() )
            return;
        if ( !m_pEnvLight )
            return;

        float flNewHour = m_flCurrentHour;

        const int iTimeMode = tod_time_mode.GetInt();
        switch ( iTimeMode )
        {
        case 0: // MATCH — proportional to elapsed match time
            {
                float flElapsed   = MAX( gpGlobals->curtime - m_flMatchStartTime, 0.f );
                float flMatchHrs  = tod_match_hours.GetFloat();
                flNewHour = m_flHourAtMatchStart
                          + (m_flMatchDurationSec > 0.f
                             ? (flElapsed / m_flMatchDurationSec) * flMatchHrs
                             : 0.f);
            }
            break;

        case 1: // REALTIME — 1 real second = 1 in-game second
            {
                float flElapsed = MAX( gpGlobals->curtime - m_flMatchStartTime, 0.f );
                flNewHour = m_flHourAtMatchStart + flElapsed / 3600.f;
            }
            break;

        default: // MANUAL — frozen; tod_set_hour drives it
            break;
        }

        // Clamp to the arc window (sunrise→sunset or configured range)
        // In MATCH/REALTIME modes the sun stops at the end of the arc
        // rather than rolling into darkness unless moon mode is on.
        if ( iTimeMode != 2 )                  // not MANUAL
            flNewHour = clamp( flNewHour, m_flArcStart, m_flArcEnd + 4.f );  // +4h grace for moon
        // Wrap at 24 h for display
        flNewHour = fmodf( flNewHour, 24.f );
        if ( flNewHour < 0.f ) flNewHour += 24.f;

        // ---- THROTTLE ----
        // Only call PushSunState (which does trig + potential network writes)
        // when the hour has meaningfully changed.  0.001 h ≈ 3.6 s — invisible.
        if ( fabsf( flNewHour - m_flCurrentHour ) > 0.001f )
        {
            m_flCurrentHour = flNewHour;
            PushSunState( flNewHour, false );
        }

        if ( tod_debug.GetBool() && gpGlobals->curtime - m_flLastDebugPrint > 1.f )
        {
            m_flLastDebugPrint = gpGlobals->curtime;
            PrintState();
        }
    }

    void OnRoundStart()
    {
        if ( !m_bLevelReady || !ShouldResetOnRoundCached() )
            return;

        // Re-resolve geo in case server ConVars changed between rounds
        if ( !m_bGeoFromTable )
            ResolveGeo();

        int iDay = ResolveDayOfYear();
        ResolveArcBounds( iDay, m_flArcStart, m_flArcEnd );

        float flStart = ResolveStartHour();
        flStart = clamp( flStart, m_flArcStart, m_flArcEnd );

        m_flCurrentHour      = flStart;
        m_flHourAtMatchStart = flStart;
        m_flMatchStartTime   = gpGlobals->curtime;

        PushSunState( flStart, true );
        Msg( "[SkyTOD] Round reset → %02d:%02d\n", HH(flStart), MM(flStart) );
    }

    void SetHour( float flHour )
    {
        flHour = fmodf( clamp(flHour, 0.f, 24.f), 24.f );
        tod_time_mode.SetValue( 2 );   // switch to MANUAL so it sticks
        m_flCurrentHour      = flHour;
        m_flHourAtMatchStart = flHour;
        m_flMatchStartTime   = gpGlobals->curtime;
        if ( m_pEnvLight )
            PushSunState( flHour, true );
        Msg( "[SkyTOD] Time → %02d:%02d  [mode: MANUAL]\n", HH(flHour), MM(flHour) );
    }

    void PrintState()
    {
        const char *pModes[] = { "MATCH", "REALTIME", "MANUAL" };
        int iMode = clamp( tod_time_mode.GetInt(), 0, 2 );
        float flP = m_pEnvLight ? m_pEnvLight->GetAbsAngles().x : 0.f;
        float flY = m_pEnvLight ? m_pEnvLight->GetAbsAngles().y : 0.f;
        Msg( "[SkyTOD] %-8s  %s  %02d:%02d  pitch=%.1f°  yaw=%.1f°  reset=%s\n",
             pModes[iMode], m_bMoonMode ? "MOON" : "SUN ",
             HH(m_flCurrentHour), MM(m_flCurrentHour),
             flP, flY, ShouldResetOnRound() ? "on" : "off" );
        Msg( "[SkyTOD] Location: %s  (%.2f°N, %.2f°  TZ %.1f)  [%s]\n",
             m_pszResolvedLocation,
             m_flResolvedLat, m_flResolvedLon, m_flResolvedTZ,
             m_bGeoFromTable ? "table" : "ConVars" );
        Msg( "[SkyTOD] Arc: %02d:%02d → %02d:%02d\n",
             HH(m_flArcStart), MM(m_flArcStart), HH(m_flArcEnd), MM(m_flArcEnd) );
    }

    float GetCurrentHour()    const { return m_flCurrentHour;    }
    float GetMapDefaultHour() const { return m_flMapDefaultHour; }

private:

    // Convenience: hour → HH / MM components for Msg()
    static int HH( float h ) { return (int)floorf(h); }
    static int MM( float h ) { return (int)(fmodf(h,1.f)*60.f); }

    // Returns the effective day-of-year: reads the server's real date when
    // tod_day_of_year is -1 (default), otherwise uses the configured value.
    // Compute the effective start/end clock hours for the match arc.
    // If either cvar is -1, the real astronomical sunrise/sunset is used.
    // This is called once at LevelInit and OnRoundStart so the season
    // automatically updates between maps without any admin intervention.
    // Resolve geographic coordinates from map name table (Priority B)
    // or fall back to server ConVars (Priority C).  Must be called once at
    // LevelInitPostEntity, before ResolveArcBounds or any trig call.
    // Push resolved geo into FCVAR_REPLICATED convars so clients see it.
    void PushResolvedGeo()
    {
        ConVarRef rl( "tod_resolved_lat" );
        ConVarRef rn( "tod_resolved_lon" );
        ConVarRef rz( "tod_resolved_tz" );
        if ( rl.IsValid() ) rl.SetValue( m_flResolvedLat );
        if ( rn.IsValid() ) rn.SetValue( m_flResolvedLon );
        if ( rz.IsValid() ) rz.SetValue( m_flResolvedTZ );
    }

    void ResolveGeo()
    {
        const char *pszMap = STRING( gpGlobals->mapname );
        float flLat, flLon, flTZ;
        const char *pszTableLoc = NULL;

        // ── Priority A: maps/<mapname>_tod.cfg (file-based, highest priority) ──
        // Mapmakers or server admins drop a three-line cfg alongside the BSP.
        // An optional 4th line is a human-readable location name.
        m_szCfgLocationBuf[0] = '\0';
        if ( LoadMapGeoCfg( pszMap, flLat, flLon, flTZ,
                            m_szCfgLocationBuf, sizeof(m_szCfgLocationBuf) ) )
        {
            m_flResolvedLat       = flLat;
            m_flResolvedLon       = flLon;
            m_flResolvedTZ        = flTZ;
            m_pszResolvedLocation = m_szCfgLocationBuf[0]
                                    ? m_szCfgLocationBuf
                                    : "maps/_tod.cfg (no location name)";
            m_bGeoFromTable       = false;
            Msg( "[SkyTOD] Location (cfg): maps/%s_tod.cfg  %.2f\xc2\xb0, %.2f\xc2\xb0  TZ %.1f  [%s]\n",
                 pszMap, flLat, flLon, flTZ, m_pszResolvedLocation );
            PushResolvedGeo();
            return;
        }

        // ── Priority B: hardcoded table (full TF2 + TF2V pool) ──
        if ( LookupMapGeoTable( pszMap, flLat, flLon, flTZ, pszTableLoc ) )
        {
            m_flResolvedLat       = flLat;
            m_flResolvedLon       = flLon;
            m_flResolvedTZ        = flTZ;
            m_pszResolvedLocation = pszTableLoc;
            m_bGeoFromTable       = true;
            Msg( "[SkyTOD] Location (table): %s  %.2f\xc2\xb0, %.2f\xc2\xb0  TZ %.1f\n",
                 pszTableLoc, flLat, flLon, flTZ );
            PushResolvedGeo();
            return;
        }

        // ── Priority C: server ConVars (tod_latitude / tod_longitude / tod_tz_meridian) ──
        m_flResolvedLat       = tod_latitude.GetFloat();
        m_flResolvedLon       = tod_longitude.GetFloat();
        m_flResolvedTZ        = tod_tz_meridian.GetFloat();
        m_pszResolvedLocation = "server ConVars (tod_latitude/longitude/tz_meridian)";
        m_bGeoFromTable       = false;
        Msg( "[SkyTOD] Location (ConVar fallback): %.2f\xc2\xb0, %.2f\xc2\xb0  TZ %.1f\n",
             m_flResolvedLat, m_flResolvedLon, m_flResolvedTZ );
        PushResolvedGeo();
    }

        void ResolveArcBounds( int iDayOfYear, float &flOutStart, float &flOutEnd )
    {
        float flSunrise, flSunset;
        ComputeSunriseSunset(
            m_flResolvedLat, m_flResolvedLon,
            m_flResolvedTZ, iDayOfYear,
            flSunrise, flSunset );

        float flCfgStart = tod_round_start_hour.GetFloat();
        float flCfgEnd   = tod_round_end_hour.GetFloat();

        flOutStart = ( flCfgStart < 0.f ) ? flSunrise : flCfgStart;
        flOutEnd   = ( flCfgEnd   < 0.f ) ? flSunset  : flCfgEnd;

        // Sanity: ensure end > start with at least a 30-minute window
        if ( flOutEnd <= flOutStart + 0.5f )
            flOutEnd = flOutStart + 0.5f;

        Msg( "[SkyTOD] Arc: %02d:%02d → %02d:%02d  (%s/%s)\n",
             HH(flOutStart), MM(flOutStart),
             HH(flOutEnd),   MM(flOutEnd),
             (flCfgStart < 0.f) ? "auto sunrise" : "configured",
             (flCfgEnd   < 0.f) ? "auto sunset"  : "configured" );
    }

    int ResolveDayOfYear()
    {
        int iCfg = tod_day_of_year.GetInt();
        if ( iCfg >= 1 )
            return clamp( iCfg, 1, 365 );   // explicit — always use directly

        // Auto mode: cache the syscall result and refresh once every 6 real hours.
        // A match won't span 6 hours, so the cached value is always correct in play.
        const float kRefreshInterval = 6.f * 3600.f;
        if ( m_iCachedDayOfYear < 1 ||
             gpGlobals->realtime - m_flDayOfYearRefreshTime > kRefreshInterval )
        {
            m_iCachedDayOfYear      = GetServerDayOfYear();
            m_flDayOfYearRefreshTime = gpGlobals->realtime;
        }
        return m_iCachedDayOfYear;
    }

    float ResolveStartHour()
    {
        float flRaw = tod_start_hour.GetFloat();
        if ( flRaw < -1.5f )
        {
            float flC = GetServerClockHour();
            Msg( "[SkyTOD] Start: server clock %02d:%02d\n", HH(flC), MM(flC) );
            return flC;
        }
        if ( flRaw < 0.f )
        {
            Msg( "[SkyTOD] Start: map default %02d:%02d\n",
                 HH(m_flMapDefaultHour), MM(m_flMapDefaultHour) );
            return m_flMapDefaultHour;
        }
        Msg( "[SkyTOD] Start: configured %02d:%02d\n", HH(flRaw), MM(flRaw) );
        return flRaw;
    }

    // Cached version — uses ConVarRefs built at LevelInit, no string hash per call.
    bool ShouldResetOnRoundCached() const
    {
        int s = tod_reset_on_round.GetInt();
        if ( s == 1 ) return true;
        if ( s == 0 ) return false;
        return ( (m_refMaxRounds.IsValid() && m_refMaxRounds.GetInt() > 0) ||
                 (m_refWinLimit.IsValid()   && m_refWinLimit.GetInt()   > 0) );
    }

    // bForce = true bypasses the 0.05° angle-change guard (used on init / reset).
    void PushSunState( float flHour, bool bForce )
    {
        if ( !m_pEnvLight )
            return;

        float flPitch, flYaw;
        int iDayOfYear = ResolveDayOfYear();
        HourToSunAngles(
            flHour,
            m_flResolvedLat, m_flResolvedLon,
            m_flResolvedTZ, iDayOfYear,
            tod_yaw_offset.GetFloat(),
            flPitch, flYaw );

        bool bBelowHorizon = ( flPitch > 0.f );
        bool bMoon         = ( bBelowHorizon && tod_moon_enable.GetBool() );
        m_bMoonMode        = bMoon;

        if ( bBelowHorizon && !tod_moon_enable.GetBool() )
            return;

        if ( bMoon )
        {
            flPitch = -12.f;
            flYaw  += 180.f;
            while ( flYaw >  180.f ) flYaw -= 360.f;
            while ( flYaw < -180.f ) flYaw += 360.f;
        }

        // ---- ANGLE-CHANGE THROTTLE ----
        // Only write + fire NetworkStateChanged when the angle moved > 0.05°.
        // This prevents useless network traffic at slow sun-movement rates.
        QAngle angNew( flPitch, flYaw, 0.f );
        if ( bForce || !m_pEnvLight )
        {
            m_pEnvLight->SetAbsAngles( angNew );
            m_pEnvLight->m_angSunAngles = angNew;
        }
        else
        {
            const QAngle &angOld = m_pEnvLight->m_angSunAngles.Get();
            if ( fabsf(angNew.x - angOld.x) > 0.05f ||
                 fabsf(angNew.y - angOld.y) > 0.05f )
            {
                m_pEnvLight->SetAbsAngles( angNew );
                m_pEnvLight->m_angSunAngles = angNew;
            }
        }

        // Colour — always update for smooth transitions
        SunColor col = ComputeSunColor( flHour,
            m_flArcStart, m_flArcEnd, bMoon );

        float flAmb = clamp( tod_ambient_scale.GetFloat(), 0.f, 1.f );
        Vector vD( col.r*col.brightness, col.g*col.brightness, col.b*col.brightness );
        Vector vA = vD * flAmb;
        if ( bMoon )
        {
            vA.x = MAX(vA.x, 0.04f);
            vA.y = MAX(vA.y, 0.04f);
            vA.z = MAX(vA.z, 0.06f);
        }
        m_pEnvLight->m_vecLight   = vD;
        m_pEnvLight->m_vecAmbient = vA;
        m_pEnvLight->m_bCascadedShadowMappingEnabled = true;

        // Push csm_color_* — cached ConVarRefs, no hash lookup
        if ( m_crRef.IsValid() )
        {
            m_crRef.SetValue( (int)clamp(col.r*col.brightness*255.f, 0.f, 255.f) );
            m_cgRef.SetValue( (int)clamp(col.g*col.brightness*255.f, 0.f, 255.f) );
            m_cbRef.SetValue( (int)clamp(col.b*col.brightness*255.f, 0.f, 255.f) );
        }

        // Drive sky fog and sv_skyname to match
        PushSkyState( flHour, flYaw, bMoon );
    }

    //--------------------------------------------------------------------------
    // Push sky fog colour and optionally swap sv_skyname when the phase changes.
    //--------------------------------------------------------------------------
    void PushSkyState( float flHour, float flSunYaw, bool bMoon )
    {
        if ( !tod_sky_fog_enable.GetBool() )
            return;

        // ── Sky fog colour ──────────────────────────────────────────────────
        color32 fogCol = SkyFogColor( flHour, m_flArcStart, m_flArcEnd, bMoon );

        // Sun direction as a horizontal vector (ignores elevation — fog blend is
        // only meaningful in the horizontal plane for skybox horizon tinting).
        float flYawRad = flSunYaw * ( M_PI_F / 180.f );
        Vector vSunDir( cosf(flYawRad), sinf(flYawRad), 0.f );

        bool bColChanged = ( abs((int)fogCol.r - (int)m_lastFogColor.r) > 2 ||
                             abs((int)fogCol.g - (int)m_lastFogColor.g) > 2 ||
                             abs((int)fogCol.b - (int)m_lastFogColor.b) > 2 );

        if ( bColChanged )
        {
            m_lastFogColor = fogCol;

            // Fog density: full at horizon (dawn/dusk), zero at noon
            float flArcLen = MAX( m_flArcEnd - m_flArcStart, 0.01f );
            float t        = clamp( (flHour - m_flArcStart) / flArcLen, 0.f, 1.f );
            float flNoon   = 1.f - 2.f * fabsf( t - 0.5f );
            const float flFogDensityMax = tod_sky_fog_density.GetFloat();
            float flDensity = Lerp( flNoon, flFogDensityMax, 0.0f );
            if ( bMoon ) flDensity = flFogDensityMax * 0.6f;

            // Cache fog ConVars — read once, used for both fog controller and sky_camera
            const float flFogStart   = tod_sky_fog_start.GetFloat();
            const float flFogEnd     = tod_sky_fog_end.GetFloat();
            const bool  bFogBlend    = tod_sky_fog_blend.GetBool();
            const bool  bFogEnabled  = ( flDensity > 0.01f );

            // World fog via env_fog_controller
            if ( m_pFogController )
            {
                m_pFogController->m_fog.colorPrimary   = fogCol;
                m_pFogController->m_fog.colorSecondary = fogCol;
                m_pFogController->m_fog.maxdensity     = flDensity;
                m_pFogController->m_fog.enable         = bFogEnabled;
            }

            // 3D skybox horizon haze via sky_camera
            // Write to m_skyboxData then null m_pOldSkyCamera on all players
            // so ClientData_Update() re-copies it next frame.
            // Write sky fog directly into each player's networked m_skybox3d.
            // We bypass the sky_camera pointer-compare guard in ClientData_Update
            // because that guard is a one-shot copy — it won't push our per-frame
            // colour updates.  CNetworkColor32 assignment marks the field dirty
            // automatically, so only the changed bytes are sent.
            // We also keep m_skyboxData in sync so new-joining players (who trigger
            // the CopyFrom path) get the current colour immediately.
            if ( m_pSkyCamera )
            {
                m_pSkyCamera->m_skyboxData.fog.colorPrimary   = fogCol;
                m_pSkyCamera->m_skyboxData.fog.colorSecondary = fogCol;
                m_pSkyCamera->m_skyboxData.fog.dirPrimary     = vSunDir;
                m_pSkyCamera->m_skyboxData.fog.start          = flFogStart;
                m_pSkyCamera->m_skyboxData.fog.end            = flFogEnd;
                m_pSkyCamera->m_skyboxData.fog.maxdensity     = flDensity * 0.5f;
                m_pSkyCamera->m_skyboxData.fog.blend          = bFogBlend;
                m_pSkyCamera->m_skyboxData.fog.enable         = bFogEnabled;
            }

            // Push directly into all connected players' replicated local data.
            for ( int i = 1; i <= gpGlobals->maxClients; i++ )
            {
                CBasePlayer *pPl = UTIL_PlayerByIndex( i );
                if ( !pPl ) continue;

                pPl->m_Local.m_skybox3d.fog.colorPrimary   = fogCol;
                pPl->m_Local.m_skybox3d.fog.colorSecondary = fogCol;
                pPl->m_Local.m_skybox3d.fog.dirPrimary = vSunDir;
                pPl->m_Local.m_skybox3d.fog.start      = flFogStart;
                pPl->m_Local.m_skybox3d.fog.end        = flFogEnd;
                pPl->m_Local.m_skybox3d.fog.maxdensity = flDensity * 0.5f;
                pPl->m_Local.m_skybox3d.fog.blend      = bFogBlend;
                pPl->m_Local.m_skybox3d.fog.enable     = bFogEnabled;
            }
        }

        // ── sv_skyname phase swap ───────────────────────────────────────────
        SkyPhase_t ePhase = ClassifySkyPhase( flHour, m_flArcStart, m_flArcEnd );
        if ( bMoon ) ePhase = SKY_NIGHT;

        if ( ePhase != m_eLastSkyPhase )
        {
            m_eLastSkyPhase = ePhase;
            const char *pszNewSky = NULL;
            switch ( ePhase )
            {
            case SKY_DAY:   if ( *tod_skyname_day.GetString()   ) pszNewSky = tod_skyname_day.GetString();   break;
            case SKY_DUSK:  if ( *tod_skyname_dusk.GetString()  ) pszNewSky = tod_skyname_dusk.GetString();  break;
            case SKY_NIGHT: if ( *tod_skyname_night.GetString() ) pszNewSky = tod_skyname_night.GetString(); break;
            }
            if ( pszNewSky && m_refSvSkyname.IsValid() )
            {
                m_refSvSkyname.SetValue( pszNewSky );
                Msg( "[SkyTOD] Sky texture -> %s  (%s)\n", pszNewSky,
                     ePhase == SKY_DAY ? "day" : ePhase == SKY_DUSK ? "dusk" : "night" );
            }
        }
    }

    CEnvLight *m_pEnvLight;

    float  m_flCurrentHour;
    float  m_flMatchStartTime;
    float  m_flMatchDurationSec;
    float  m_flHourAtMatchStart;
    float  m_flMapDefaultHour;
    float  m_flArcStart;       // resolved sunrise or configured start
    float  m_flArcEnd;         // resolved sunset  or configured end

    // Geographic location resolved at LevelInit.
    // Set from map name table (Priority B) or ConVars (Priority C).
    float        m_flResolvedLat;
    float        m_flResolvedLon;
    float        m_flResolvedTZ;
	char  		 m_szCfgLocationBuf[256];  // scratch buffer for file-based geo config
    const char  *m_pszResolvedLocation;  // points into table or literal — never freed
    bool         m_bGeoFromTable;        // true = table match, false = ConVar fallback

    bool   m_bSessionLocked;
    bool   m_bMoonMode;
    bool   m_bLevelReady;

    // Cached day-of-year — refreshed at most once per real-world day.
    // Avoids a localtime() syscall in every PushSunState call.
    int    m_iCachedDayOfYear;
    float  m_flDayOfYearRefreshTime;

    float  m_flLastDebugPrint;

    ConVarRef m_crRef, m_cgRef, m_cbRef;   // cached at LevelInit
    ConVarRef m_refMaxRounds;              // cached at LevelInit — used by ShouldResetOnRound
    ConVarRef m_refWinLimit;               // cached at LevelInit — used by ShouldResetOnRound
    ConVarRef m_refSvSkyname;              // cached at LevelInit — used by PushSkyState

    // Sky state
    CFogController *m_pFogController;   // master env_fog_controller, cached at LevelInit
    CSkyCamera     *m_pSkyCamera;        // sky_camera entity, cached at LevelInit
    SkyPhase_t      m_eLastSkyPhase;     // track transitions for sv_skyname swaps
    color32         m_lastFogColor;      // avoid redundant fog writes
};

static CSkyTODSystem s_SkyTODSystem;

//=============================================================================
// Console commands
//=============================================================================

CON_COMMAND( tod_set_hour,
    "Set the in-game time to the given hour (0-24) and lock to MANUAL mode.\n"
    "Usage: tod_set_hour 7.5" )
{
    if ( args.ArgC() < 2 ) { Msg("Usage: tod_set_hour <hour>\n"); return; }
    s_SkyTODSystem.SetHour( atof(args[1]) );
}

CON_COMMAND( tod_print_state,
    "Print current time-of-day state and the server's real clock time." )
{
    s_SkyTODSystem.PrintState();
    float flC = GetServerClockHour();
    Msg( "  Map default:  %02d:%02d\n",
         (int)floorf(s_SkyTODSystem.GetMapDefaultHour()),
         (int)(fmodf(s_SkyTODSystem.GetMapDefaultHour(),1.f)*60.f) );
    Msg( "  Server clock: %02d:%02d\n", (int)floorf(flC), (int)(fmodf(flC,1.f)*60.f) );
}

CON_COMMAND( tod_round_reset,
    "Manually fire a round-start time reset (same as tod_reset_on_round 1 firing)." )
{
    s_SkyTODSystem.OnRoundStart();
}
