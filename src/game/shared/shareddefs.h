//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Definitions that are shared by the game DLL and the client DLL.
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHAREDDEFS_H
#define SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#define OLD_TICK_INTERVAL		(0.015f)
#define TICK_INTERVAL			(gpGlobals->interval_per_tick)


#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

#if defined( TF_DLL )
#define ANIMATION_CYCLE_BITS		10
#else
#define ANIMATION_CYCLE_BITS		15
#endif
#define ANIMATION_CYCLE_MINFRAC		(1.0f / (1<<ANIMATION_CYCLE_BITS))

// Each mod defines these for itself.
class CViewVectors
{
public:
	CViewVectors() {}

	CViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight )
	{
		m_vView = vView;
		m_vHullMin = vHullMin;
		m_vHullMax = vHullMax;
		m_vDuckHullMin = vDuckHullMin;
		m_vDuckHullMax = vDuckHullMax;
		m_vDuckView = vDuckView;
		m_vObsHullMin = vObsHullMin;
		m_vObsHullMax = vObsHullMax;
		m_vDeadViewHeight = vDeadViewHeight;
	}

	// Height above entity position where the viewer's eye is.
	Vector m_vView;
	
	Vector m_vHullMin;
	Vector m_vHullMax;
	
	Vector m_vDuckHullMin;
	Vector m_vDuckHullMax;
	Vector m_vDuckView;
	
	Vector m_vObsHullMin;
	Vector m_vObsHullMax;
	
	Vector m_vDeadViewHeight;
};

// Height above entity position where the viewer's eye is.
#define VEC_VIEW			g_pGameRules->GetViewVectors()->m_vView
#define VEC_HULL_MIN		g_pGameRules->GetViewVectors()->m_vHullMin
#define VEC_HULL_MAX		g_pGameRules->GetViewVectors()->m_vHullMax

#define VEC_DUCK_HULL_MIN	g_pGameRules->GetViewVectors()->m_vDuckHullMin
#define VEC_DUCK_HULL_MAX	g_pGameRules->GetViewVectors()->m_vDuckHullMax
#define VEC_DUCK_VIEW		g_pGameRules->GetViewVectors()->m_vDuckView

#define VEC_OBS_HULL_MIN	g_pGameRules->GetViewVectors()->m_vObsHullMin
#define VEC_OBS_HULL_MAX	g_pGameRules->GetViewVectors()->m_vObsHullMax

#define VEC_DEAD_VIEWHEIGHT	g_pGameRules->GetViewVectors()->m_vDeadViewHeight

// If the player (enemy bots) are scaled, adjust the hull
#define VEC_VIEW_SCALED( player )				( g_pGameRules->GetViewVectors()->m_vView * player->GetModelScale() )
#define VEC_HULL_MIN_SCALED( player )			( g_pGameRules->GetViewVectors()->m_vHullMin * player->GetModelScale() )
#define VEC_HULL_MAX_SCALED( player )			( g_pGameRules->GetViewVectors()->m_vHullMax * player->GetModelScale() )

#define VEC_DUCK_HULL_MIN_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vDuckHullMin * player->GetModelScale() )
#define VEC_DUCK_HULL_MAX_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vDuckHullMax * player->GetModelScale() )
#define VEC_DUCK_VIEW_SCALED( player )			( g_pGameRules->GetViewVectors()->m_vDuckView * player->GetModelScale() )

#define VEC_OBS_HULL_MIN_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vObsHullMin * player->GetModelScale() )
#define VEC_OBS_HULL_MAX_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vObsHullMax * player->GetModelScale() )

#define VEC_DEAD_VIEWHEIGHT_SCALED( player )	( g_pGameRules->GetViewVectors()->m_vDeadViewHeight * player->GetModelScale() )

#define WATERJUMP_HEIGHT			8

#define MAX_CLIMB_SPEED		200

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	#define TIME_TO_DUCK		0.2f
	#define TIME_TO_DUCK_MS		200.0f
#else
	#define TIME_TO_DUCK		0.4f
	#define TIME_TO_DUCK_MS		400.0f
#endif 
#define TIME_TO_UNDUCK		0.2f
#define TIME_TO_UNDUCK_MS	200.0f

#define MAX_WEAPON_SLOTS		6	// hud item selection slots
#define MAX_WEAPON_POSITIONS	20	// max number of items within a slot
#define MAX_ITEM_TYPES			6	// hud item selection slots
#define MAX_WEAPONS				48	// Max number of weapons available

#define MAX_ITEMS				5	// hard coded item types

#define WEAPON_NOCLIP			-1	// clip sizes set to this tell the weapon it doesn't use a clip

#define	MAX_AMMO_TYPES	32		// ???
#define MAX_AMMO_SLOTS  32		// not really slots

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

// Vote creation or processing failure codes
typedef enum
{
	VOTE_FAILED_GENERIC = 0,
	VOTE_FAILED_TRANSITIONING_PLAYERS,
	VOTE_FAILED_RATE_EXCEEDED,
	VOTE_FAILED_YES_MUST_EXCEED_NO,
	VOTE_FAILED_QUORUM_FAILURE,
	VOTE_FAILED_ISSUE_DISABLED,
	VOTE_FAILED_MAP_NOT_FOUND,
	VOTE_FAILED_MAP_NAME_REQUIRED,
	VOTE_FAILED_ON_COOLDOWN,
	VOTE_FAILED_TEAM_CANT_CALL,
	VOTE_FAILED_WAITINGFORPLAYERS,
	VOTE_FAILED_PLAYERNOTFOUND,
	VOTE_FAILED_CANNOT_KICK_ADMIN,
	VOTE_FAILED_SCRAMBLE_IN_PROGRESS,
	VOTE_FAILED_SPECTATOR,
	VOTE_FAILED_NEXTLEVEL_SET,
	VOTE_FAILED_MAP_NOT_VALID,
	VOTE_FAILED_CANNOT_KICK_FOR_TIME,
	VOTE_FAILED_CANNOT_KICK_DURING_ROUND,
	VOTE_FAILED_VOTE_IN_PROGRESS,
	VOTE_FAILED_KICK_LIMIT_REACHED,
	VOTE_FAILED_KICK_DENIED_BY_GC,
	// Special reason - the vote issue itself will handle this request specially -- the vote system shouldn't proceed
	// with the vote.  Shows no error to user.
	//
	// Used by match-based votekicks to send a votekick request to the match system when the user tries to call one --
	// which then owns starting a proper vote later or telling the user why the match system isn't allowing the
	// votekick.
	VOTE_FAILED_REQUEST_HANDLED_BY_ISSUE,

	// TF-specific?
	VOTE_FAILED_MODIFICATION_ALREADY_ACTIVE,

	VOTE_FAILED_PLAYER_TRANSITIONING,

	VOTE_FAILED_INVALID_ARGUMENT,
} vote_create_failed_t;

enum
{
	SERVER_MODIFICATION_ITEM_DURATION_IN_MINUTES = 120
};

#define MAX_VOTE_DETAILS_LENGTH 64
#define INVALID_ISSUE			-1
#define MAX_VOTE_OPTIONS		5
#define DEDICATED_SERVER		0

enum CastVote
{
	VOTE_OPTION1 = 0,  // Use this for Yes
	VOTE_OPTION2 = 1,  // Use this for No
	VOTE_OPTION3 = 2,
	VOTE_OPTION4 = 3,
	VOTE_OPTION5 = 4,
	VOTE_UNCAST  = 5,

	// Alias yes/no to OPTION1/OPTION2
	VOTE_YES = 0,
	VOTE_NO  = 1,
};

//===================================================================================================================
// Close caption flags
#define CLOSE_CAPTION_WARNIFMISSING	( 1<<0 )
#define CLOSE_CAPTION_FROMPLAYER	( 1<<1 )
#define CLOSE_CAPTION_GENDER_MALE	( 1<<2 )
#define CLOSE_CAPTION_GENDER_FEMALE	( 1<<3 )

//===================================================================================================================
// Hud Element hiding flags
#define	HIDEHUD_WEAPONSELECTION		( 1<<0 )	// Hide ammo count & weapon selection
#define	HIDEHUD_FLASHLIGHT			( 1<<1 )
#define	HIDEHUD_ALL					( 1<<2 )
#define HIDEHUD_HEALTH				( 1<<3 )	// Hide health & armor / suit battery
#define HIDEHUD_PLAYERDEAD			( 1<<4 )	// Hide when local player's dead
#define HIDEHUD_NEEDSUIT			( 1<<5 )	// Hide when the local player doesn't have the HEV suit
#define HIDEHUD_MISCSTATUS			( 1<<6 )	// Hide miscellaneous status elements (trains, pickup history, death notices, etc)
#define HIDEHUD_CHAT				( 1<<7 )	// Hide all communication elements (saytext, voice icon, etc)
#define	HIDEHUD_CROSSHAIR			( 1<<8 )	// Hide crosshairs
#define	HIDEHUD_VEHICLE_CROSSHAIR	( 1<<9 )	// Hide vehicle crosshair
#define HIDEHUD_INVEHICLE			( 1<<10 )
#define HIDEHUD_BONUS_PROGRESS		( 1<<11 )	// Hide bonus progress display (for bonus map challenges)

#if defined( TF_DLL ) || defined ( TF_CLIENT_DLL )
#define HIDEHUD_BUILDING_STATUS		        ( 1<<12 )	// Hide Engineer building status
#define HIDEHUD_CLOAK_AND_FEIGN             ( 1<<13 )	// Hide item effect meter (cloak, etc)
#define HIDEHUD_PIPES_AND_CHARGE            ( 1<<14 )	// Hide demo hud
#define HIDEHUD_METAL                       ( 1<<15 )	// Metal/account hud
#define HIDEHUD_TARGET_ID                   ( 1<<16 )	// Target ID
#define HIDEHUD_MATCH_STATUS				( 1<<17 )	// Hide match status
#define HIDEHUD_BITCOUNT			18
#else
#define HIDEHUD_BITCOUNT			12
#endif


//===================================================================================================================
// suit usage bits
#define bits_SUIT_DEVICE_SPRINT		0x00000001
#define bits_SUIT_DEVICE_FLASHLIGHT	0x00000002
#define bits_SUIT_DEVICE_BREATHER	0x00000004

#define MAX_SUIT_DEVICES			3


//===================================================================================================================
// Player Defines

// Max number of players in a game ( see const.h for ABSOLUTE_PLAYER_LIMIT (256 ) )
// The Source engine is really designed for 32 or less players.  If you raise this number above 32, you better know what you are doing
//  and have a good answer for a bunch of perf question related to player simulation, thinking logic, tracelines, networking overhead, etc.
// But if you are brave or are doing something interesting, go for it...   ywb 9/22/03

//You might be wondering why these aren't multiple of 2. Well the reason is that if servers decide to have HLTV or Replay enabled we need the extra slot.
//This is ok since MAX_PLAYERS is used for code specific things like arrays and loops, but it doesn't really means that this is the max number of players allowed
//Since this is decided by the gamerules (and it can be whatever number as long as its less than MAX_PLAYERS).
#if defined( CSTRIKE_DLL )
	#define MAX_PLAYERS				65  // Absolute max players supported
#elif defined( TF_DLL ) || defined ( TF_CLIENT_DLL ) || defined( HL2MP )
	#define MAX_PLAYERS				101
#else
	#define MAX_PLAYERS				33  // Absolute max players supported
#endif

// Josh: Accounts for code that may index this array by an entindex
// of player rather than the player index... :s
#define MAX_PLAYERS_ARRAY_SAFE		( MAX_PLAYERS + 1 )

inline bool IsIndexIntoPlayerArrayValid( int iIndex )
{
	if ( iIndex < 0 || iIndex >= MAX_PLAYERS_ARRAY_SAFE )
		return false;
		
	return true;
}

#define MAX_PLACE_NAME_LENGTH		18

#define MAX_FOV						90

//===================================================================================================================
// Team Defines
#define TEAM_ANY				-2
#define	TEAM_INVALID			-1
#define TEAM_UNASSIGNED			0	// not assigned to a team
#define TEAM_SPECTATOR			1	// spectator team
// Start your team numbers after this
#define LAST_SHARED_TEAM		TEAM_SPECTATOR

// The first team that's game specific (i.e. not unassigned / spectator)
#define FIRST_GAME_TEAM			(LAST_SHARED_TEAM+1)

#define MAX_TEAMS				32	// Max number of teams in a game
#define MAX_TEAM_NAME_LENGTH	32	// Max length of a team's name

#define MAX_TEAMS_ARRAY_SAFE 	MAX_TEAMS

// Weapon m_iState
#define WEAPON_IS_ONTARGET				0x40

#define WEAPON_NOT_CARRIED				0	// Weapon is on the ground
#define WEAPON_IS_CARRIED_BY_PLAYER		1	// This client is carrying this weapon.
#define WEAPON_IS_ACTIVE				2	// This client is carrying this weapon and it's the currently held weapon

// -----------------------------------------
// Skill Level
// -----------------------------------------
#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3


// Weapon flags
// -----------------------------------------
//	Flags - NOTE: KEEP g_ItemFlags IN WEAPON_PARSE.CPP UPDATED WITH THESE
// -----------------------------------------
#define ITEM_FLAG_SELECTONEMPTY		(1<<0)
#define ITEM_FLAG_NOAUTORELOAD		(1<<1)
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	(1<<2)
#define ITEM_FLAG_LIMITINWORLD		(1<<3)
#define ITEM_FLAG_EXHAUSTIBLE		(1<<4)	// A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_DOHITLOCATIONDMG	(1<<5)	// This weapon take hit location into account when applying damage
#define ITEM_FLAG_NOAMMOPICKUPS		(1<<6)	// Don't draw ammo pickup sprites/sounds when ammo is received
#define ITEM_FLAG_NOITEMPICKUP		(1<<7)	// Don't draw weapon pickup when this weapon is picked up by the player
// NOTE: KEEP g_ItemFlags IN WEAPON_PARSE.CPP UPDATED WITH THESE


// Humans only have left and right hands, though we might have aliens with more
//  than two, sigh
#define MAX_VIEWMODELS			2

#define MAX_BEAM_ENTS			10

#define TRACER_TYPE_DEFAULT		0x00000001
#define TRACER_TYPE_GUNSHIP		0x00000002
#define TRACER_TYPE_STRIDER		0x00000004 // Here ya go, Jay!
#define TRACER_TYPE_GAUSS		0x00000008
#define TRACER_TYPE_WATERBULLET	0x00000010

#define MUZZLEFLASH_TYPE_DEFAULT	0x00000001
#define MUZZLEFLASH_TYPE_GUNSHIP	0x00000002
#define MUZZLEFLASH_TYPE_STRIDER	0x00000004

// Muzzle flash definitions (for the flags field of the "MuzzleFlash" DispatchEffect)
enum
{
	MUZZLEFLASH_AR2				= 0,
	MUZZLEFLASH_SHOTGUN,
	MUZZLEFLASH_SMG1,
	MUZZLEFLASH_SMG2,
	MUZZLEFLASH_PISTOL,
	MUZZLEFLASH_COMBINE,
	MUZZLEFLASH_357,
	MUZZLEFLASH_RPG,
	MUZZLEFLASH_COMBINE_TURRET,

	MUZZLEFLASH_FIRSTPERSON		= 0x100,
};

// Tracer Flags
#define TRACER_FLAG_WHIZ			0x0001
#define TRACER_FLAG_USEATTACHMENT	0x0002

#define TRACER_DONT_USE_ATTACHMENT	-1

// Entity Dissolve types
enum
{
	ENTITY_DISSOLVE_NORMAL = 0,
	ENTITY_DISSOLVE_ELECTRICAL,
	ENTITY_DISSOLVE_ELECTRICAL_LIGHT,
	ENTITY_DISSOLVE_CORE,

	// NOTE: Be sure to up the bits if you make more dissolve types
	ENTITY_DISSOLVE_BITS = 3
};

// ---------------------------
//  Hit Group standards
// ---------------------------
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_GEAR		10			// alerts NPC, but doesn't do damage or bleed (1/100th damage)

//
// Enumerations for setting player animation.
//
enum PLAYER_ANIM
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_IN_VEHICLE,

	// TF Player animations
	PLAYER_RELOAD,
	PLAYER_START_AIMING,
	PLAYER_LEAVE_AIMING,
};

#ifdef HL2_DLL
// HL2 has 600 gravity by default
// NOTE: The discrete ticks can have quantization error, so these numbers are biased a little to
// make the heights more exact
#define PLAYER_FATAL_FALL_SPEED		922.5f // approx 60 feet sqrt( 2 * gravity * 60 * 12 )
#define PLAYER_MAX_SAFE_FALL_SPEED	526.5f // approx 20 feet sqrt( 2 * gravity * 20 * 12 )
#define PLAYER_LAND_ON_FLOATING_OBJECT	173 // Can fall another 173 in/sec without getting hurt
#define PLAYER_MIN_BOUNCE_SPEED		173
#define PLAYER_FALL_PUNCH_THRESHOLD 303.0f // won't punch player's screen/make scrape noise unless player falling at least this fast - at least a 76" fall (sqrt( 2 * g * 76))
#else
#define PLAYER_FATAL_FALL_SPEED		1024 // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580 // approx 20 feet
#define PLAYER_LAND_ON_FLOATING_OBJECT	200 // Can go another 200 units without getting hurt
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.
#endif
#define DAMAGE_FOR_FALL_SPEED		100.0f / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED ) // damage per unit per second.


#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669
#define AUTOAIM_20DEGREES 0.3490658503989

#define AUTOAIM_SCALE_DEFAULT		1.0f
#define AUTOAIM_SCALE_DIRECT_ONLY	0.0f

// instant damage

// For a means of resolving these consts into debug string text, see function
// CTakeDamageInfo::DebugGetDamageTypeString(unsigned int DamageType, char *outbuf, unsigned int outbuflength )
#define DMG_GENERIC			0			// generic damage -- do not use if you want players to flinch and bleed!
#define DMG_CRUSH			(1 << 0)	// crushed by falling or moving object. 
										// NOTE: It's assumed crush damage is occurring as a result of physics collision, so no extra physics force is generated by crush damage.
										// DON'T use DMG_CRUSH when damaging entities unless it's the result of a physics collision. You probably want DMG_CLUB instead.
#define DMG_BULLET			(1 << 1)	// shot
#define DMG_SLASH			(1 << 2)	// cut, clawed, stabbed
#define DMG_BURN			(1 << 3)	// heat burned
#define DMG_VEHICLE			(1 << 4)	// hit by a vehicle
#define DMG_FALL			(1 << 5)	// fell too far
#define DMG_BLAST			(1 << 6)	// explosive blast damage
#define DMG_CLUB			(1 << 7)	// crowbar, punch, headbutt
#define DMG_SHOCK			(1 << 8)	// electric shock
#define DMG_SONIC			(1 << 9)	// sound pulse shockwave
#define DMG_ENERGYBEAM		(1 << 10)	// laser or other high energy beam 
#define DMG_PREVENT_PHYSICS_FORCE		(1 << 11)	// Prevent a physics force 
#define DMG_NEVERGIB		(1 << 12)	// with this bit OR'd in, no damage type will be able to gib victims upon death
#define DMG_ALWAYSGIB		(1 << 13)	// with this bit OR'd in, any damage type can be made to gib victims upon death.
#define DMG_DROWN			(1 << 14)	// Drowning


#define DMG_PARALYZE		(1 << 15)	// slows affected creature down
#define DMG_NERVEGAS		(1 << 16)	// nerve toxins, very bad
#define DMG_POISON			(1 << 17)	// blood poisoning - heals over time like drowning damage
#define DMG_RADIATION		(1 << 18)	// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)	// drowning recovery
#define DMG_ACID			(1 << 20)	// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)	// in an oven

#define DMG_REMOVENORAGDOLL	(1<<22)		// with this bit OR'd in, no ragdoll will be created, and the target will be quietly removed.
										// use this to kill an entity that you've already got a server-side ragdoll for

#define DMG_PHYSGUN			(1<<23)		// Hit by manipulator. Usually doesn't do any damage.
#define DMG_PLASMA			(1<<24)		// Shot by Cremator
#define DMG_AIRBOAT			(1<<25)		// Hit by the airboat's gun

#define DMG_DISSOLVE		(1<<26)		// Dissolving!
#define DMG_BLAST_SURFACE	(1<<27)		// A blast on the surface of water that cannot harm things underwater
#define DMG_DIRECT			(1<<28)
#define DMG_BUCKSHOT		(1<<29)		// not quite a bullet. Little, rounder, different.

// NOTE: DO NOT ADD ANY MORE CUSTOM DMG_ TYPES. MODS USE THE DMG_LASTGENERICFLAG BELOW, AND
//		 IF YOU ADD NEW DMG_ TYPES, THEIR TYPES WILL BE HOSED. WE NEED A BETTER SOLUTION.

// TODO: keep this up to date so all the mod-specific flags don't overlap anything.
#define DMG_LASTGENERICFLAG	DMG_BUCKSHOT



// settings for m_takedamage
#define	DAMAGE_NO				0
#define DAMAGE_EVENTS_ONLY		1		// Call damage functions, but don't modify health
#define	DAMAGE_YES				2
#define	DAMAGE_AIM				3

// Spectator Movement modes
enum {
	OBS_MODE_NONE = 0,	// not in spectator mode
	OBS_MODE_DEATHCAM,	// special mode for death cam animation
	OBS_MODE_FREEZECAM,	// zooms to a target, and freeze-frames on them
	OBS_MODE_FIXED,		// view from a fixed camera position
	OBS_MODE_IN_EYE,	// follow a player in first person view
	OBS_MODE_CHASE,		// follow a player in third person view
	OBS_MODE_POI,		// PASSTIME point of interest - game objective, big fight, anything interesting; added in the middle of the enum due to tons of hard-coded "<ROAMING" enum compares
	OBS_MODE_ROAMING,	// free roaming

	NUM_OBSERVER_MODES,
};

#define LAST_PLAYER_OBSERVERMODE	OBS_MODE_ROAMING

// Force Camera Restrictions with mp_forcecamera
enum {
	OBS_ALLOW_ALL = 0,	// allow all modes, all targets
	OBS_ALLOW_TEAM,		// allow only own team & first person, no PIP
	OBS_ALLOW_NONE,		// don't allow any spectating after death (fixed & fade to black)

	OBS_ALLOW_NUM_MODES,
};

enum
{
	TYPE_TEXT = 0,	// just display this plain text
	TYPE_INDEX,		// lookup text & title in stringtable
	TYPE_URL,		// show this URL
	TYPE_FILE,		// show this local file
} ;

//=============================================================================
// HPE_BEGIN:
// [Forrest] Replaced text window command string with TEXTWINDOW_CMD enumeration
// of options.  Passing a command string is dangerous and allowed a server network
// message to run arbitrary commands on the client.
//=============================================================================
enum
{
	TEXTWINDOW_CMD_NONE = 0,
	TEXTWINDOW_CMD_JOINGAME,
	TEXTWINDOW_CMD_CHANGETEAM,
	TEXTWINDOW_CMD_IMPULSE101,
	TEXTWINDOW_CMD_MAPINFO,
	TEXTWINDOW_CMD_CLOSED_HTMLPAGE,
	TEXTWINDOW_CMD_CHOOSETEAM,
};
//=============================================================================
// HPE_END
//=============================================================================

// VGui Screen Flags
enum
{
	VGUI_SCREEN_ACTIVE = 0x1,
	VGUI_SCREEN_VISIBLE_TO_TEAMMATES = 0x2,
	VGUI_SCREEN_ATTACHED_TO_VIEWMODEL = 0x4,
	VGUI_SCREEN_TRANSPARENT = 0x8,
	VGUI_SCREEN_ONLY_USABLE_BY_OWNER = 0x10,

	VGUI_SCREEN_MAX_BITS = 5
};

typedef enum
{
	USE_OFF = 0, 
	USE_ON = 1, 
	USE_SET = 2, 
	USE_TOGGLE = 3
} USE_TYPE;

// basic team colors
#define COLOR_RED		Color(255, 64, 64, 255)
#define COLOR_BLUE		Color(153, 204, 255, 255)
#define COLOR_YELLOW	Color(255, 178, 0, 255)
#define COLOR_GREEN		Color(153, 255, 153, 255)
#define COLOR_GREY		Color(204, 204, 204, 255)
#define COLOR_WHITE		Color(255, 255, 255, 255)
#define COLOR_BLACK		Color(0, 0, 0, 255)

// All NPCs need this data
enum
{
	DONT_BLEED = -1,

	BLOOD_COLOR_RED = 0,
	BLOOD_COLOR_YELLOW,
	BLOOD_COLOR_GREEN,
	BLOOD_COLOR_MECH,

#if defined( HL2_EPISODIC )
	BLOOD_COLOR_ANTLION,		// FIXME: Move to Base HL2
	BLOOD_COLOR_ZOMBIE,			// FIXME: Move to Base HL2
	BLOOD_COLOR_ANTLION_WORKER,
#endif // HL2_EPISODIC
};

//-----------------------------------------------------------------------------
// Vehicles may have more than one passenger.
// This enum may be expanded by derived classes
//-----------------------------------------------------------------------------
enum PassengerRole_t
{
	VEHICLE_ROLE_NONE = -1,

	VEHICLE_ROLE_DRIVER = 0,	// Only one driver
	
	LAST_SHARED_VEHICLE_ROLE,
};

//-----------------------------------------------------------------------------
// Water splash effect flags
//-----------------------------------------------------------------------------
enum
{
	FX_WATER_IN_SLIME = 0x1,
};


// Shared think context stuff
#define	MAX_CONTEXT_LENGTH		32
#define NO_THINK_CONTEXT	-1

// entity flags, CBaseEntity::m_iEFlags
enum
{
	EFL_KILLME	=				(1<<0),	// This entity is marked for death -- This allows the game to actually delete ents at a safe time
	EFL_DORMANT	=				(1<<1),	// Entity is dormant, no updates to client
	EFL_NOCLIP_ACTIVE =			(1<<2),	// Lets us know when the noclip command is active.
	EFL_SETTING_UP_BONES =		(1<<3),	// Set while a model is setting up its bones.
	EFL_KEEP_ON_RECREATE_ENTITIES = (1<<4), // This is a special entity that should not be deleted when we restart entities only

	EFL_HAS_PLAYER_CHILD=		(1<<4),	// One of the child entities is a player.

	EFL_DIRTY_SHADOWUPDATE =	(1<<5),	// Client only- need shadow manager to update the shadow...
	EFL_NOTIFY =				(1<<6),	// Another entity is watching events on this entity (used by teleport)

	// The default behavior in ShouldTransmit is to not send an entity if it doesn't
	// have a model. Certain entities want to be sent anyway because all the drawing logic
	// is in the client DLL. They can set this flag and the engine will transmit them even
	// if they don't have a model.
	EFL_FORCE_CHECK_TRANSMIT =	(1<<7),

	EFL_BOT_FROZEN =			(1<<8),	// This is set on bots that are frozen.
	EFL_SERVER_ONLY =			(1<<9),	// Non-networked entity.
	EFL_NO_AUTO_EDICT_ATTACH =	(1<<10), // Don't attach the edict; we're doing it explicitly
	
	// Some dirty bits with respect to abs computations
	EFL_DIRTY_ABSTRANSFORM =	(1<<11),
	EFL_DIRTY_ABSVELOCITY =		(1<<12),
	EFL_DIRTY_ABSANGVELOCITY =	(1<<13),
	EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS	= (1<<14),
	EFL_DIRTY_SPATIAL_PARTITION = (1<<15),
	EFL_FORCE_ALLOW_MOVEPARENT	= (1<<16),

	EFL_IN_SKYBOX =				(1<<17),	// This is set if the entity detects that it's in the skybox.
											// This forces it to pass the "in PVS" for transmission.
	EFL_USE_PARTITION_WHEN_NOT_SOLID = (1<<18),	// Entities with this flag set show up in the partition even when not solid
	EFL_TOUCHING_FLUID =		(1<<19),	// Used to determine if an entity is floating

	// FIXME: Not really sure where I should add this...
	EFL_IS_BEING_LIFTED_BY_BARNACLE = (1<<20),
	EFL_NO_ROTORWASH_PUSH =		(1<<21),		// I shouldn't be pushed by the rotorwash
	EFL_NO_THINK_FUNCTION =		(1<<22),
	EFL_NO_GAME_PHYSICS_SIMULATION = (1<<23),

	EFL_CHECK_UNTOUCH =			(1<<24),
	EFL_DONTBLOCKLOS =			(1<<25),		// I shouldn't block NPC line-of-sight
	EFL_DONTWALKON =			(1<<26),		// NPC;s should not walk on this entity
	EFL_NO_DISSOLVE =			(1<<27),		// These guys shouldn't dissolve
	EFL_NO_MEGAPHYSCANNON_RAGDOLL = (1<<28),	// Mega physcannon can't ragdoll these guys.
	EFL_NO_WATER_VELOCITY_CHANGE  =	(1<<29),	// Don't adjust this entity's velocity when transitioning into water
	EFL_NO_PHYSCANNON_INTERACTION =	(1<<30),	// Physcannon can't pick these up or punt them
	EFL_NO_DAMAGE_FORCES =		(1<<31),	// Doesn't accept forces from physics damage
};

//-----------------------------------------------------------------------------
// EFFECTS
//-----------------------------------------------------------------------------
const int FX_BLOODSPRAY_DROPS	= 0x01;
const int FX_BLOODSPRAY_GORE	= 0x02;
const int FX_BLOODSPRAY_CLOUD	= 0x04;
const int FX_BLOODSPRAY_ALL		= 0xFF;

//-----------------------------------------------------------------------------
#define MAX_SCREEN_OVERLAYS		10

// These are the types of data that hang off of CBaseEntities and the flag bits used to mark their presence
enum
{
	GROUNDLINK = 0,
	TOUCHLINK,
	STEPSIMULATION,
	MODELSCALE,
	POSITIONWATCHER,
	PHYSICSPUSHLIST,
	VPHYSICSUPDATEAI,
	VPHYSICSWATCHER,

	// Must be last and <= 32
	NUM_DATAOBJECT_TYPES,
};

class CBaseEntity;

//-----------------------------------------------------------------------------
// Bullet firing information
//-----------------------------------------------------------------------------
class CBaseEntity;

enum FireBulletsFlags_t
{
	FIRE_BULLETS_FIRST_SHOT_ACCURATE = 0x1,		// Pop the first shot with perfect accuracy
	FIRE_BULLETS_DONT_HIT_UNDERWATER = 0x2,		// If the shot hits its target underwater, don't damage it
	FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS = 0x4,	// If the shot hits water surface, still call DoImpactEffect
	FIRE_BULLETS_TEMPORARY_DANGER_SOUND = 0x8,		// Danger sounds added from this impact can be stomped immediately if another is queued
};


struct FireBulletsInfo_t
{
	FireBulletsInfo_t()
	{
		m_iShots = 1;
		m_vecSpread.Init( 0, 0, 0 );
		m_flDistance = 8192;
		m_iTracerFreq = 4;
		m_flDamage = 0;
		m_iPlayerDamage = 0;
		m_pAttacker = NULL;
		m_nFlags = 0;
		m_pAdditionalIgnoreEnt = NULL;
		m_flDamageForceScale = 1.0f;

#ifdef _DEBUG
		m_iAmmoType = -1;
		m_vecSrc.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
		m_vecDirShooting.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
#endif
		m_bPrimaryAttack = true;
		m_bUseServerRandomSeed = false;
	}

	FireBulletsInfo_t( int nShots, const Vector &vecSrc, const Vector &vecDir, const Vector &vecSpread, float flDistance, int nAmmoType, bool bPrimaryAttack = true )
	{
		m_iShots = nShots;
		m_vecSrc = vecSrc;
		m_vecDirShooting = vecDir;
		m_vecSpread = vecSpread;
		m_flDistance = flDistance;
		m_iAmmoType = nAmmoType;
		m_iTracerFreq = 4;
		m_flDamage = 0;
		m_iPlayerDamage = 0;
		m_pAttacker = NULL;
		m_nFlags = 0;
		m_pAdditionalIgnoreEnt = NULL;
		m_flDamageForceScale = 1.0f;
		m_bPrimaryAttack = bPrimaryAttack;
		m_bUseServerRandomSeed = false;
	}

	int m_iShots;
	Vector m_vecSrc;
	Vector m_vecDirShooting;
	Vector m_vecSpread;
	float m_flDistance;
	int m_iAmmoType;
	int m_iTracerFreq;
	float m_flDamage;
	int m_iPlayerDamage;	// Damage to be used instead of m_flDamage if we hit a player
	int m_nFlags;			// See FireBulletsFlags_t
	float m_flDamageForceScale;
	CBaseEntity *m_pAttacker;
	CBaseEntity *m_pAdditionalIgnoreEnt;
	bool m_bPrimaryAttack;
	bool m_bUseServerRandomSeed;
};

//-----------------------------------------------------------------------------
// Purpose: Data for making the MOVETYPE_STEP entities appear to simulate every frame
//  We precompute the simulation and then meter it out each tick during networking of the 
//  entities origin and orientation.  Uses a bit more bandwidth, but it solves the NPCs interacting
//  with elevators/lifts bugs.
//-----------------------------------------------------------------------------
struct StepSimulationStep
{
	int			nTickCount;
	Vector		vecOrigin;
	Quaternion	qRotation;
};

struct StepSimulationData
{
	// Are we using the Step Simulation Data
	bool		m_bOriginActive;
	bool		m_bAnglesActive;

	// This is the pre-pre-Think position, orientation (Quaternion) and tick count
	StepSimulationStep	m_Previous2;

	// This is the pre-Think position, orientation (Quaternion) and tick count
	StepSimulationStep	m_Previous;

	// This is a potential mid-think position, orientation (Quaternion) and tick count
	// Used to mark motion discontinuities that happen between thinks
	StepSimulationStep	m_Discontinuity;

	// This is the goal or post-Think position and orientation (and Quaternion for blending) and next think time tick
	StepSimulationStep	m_Next;
	QAngle		m_angNextRotation;

	// This variable is used so that we only compute networked origin/angles once per tick
	int			m_nLastProcessTickCount;
	// The computed/interpolated network origin/angles to use
	Vector		m_vecNetworkOrigin;
	QAngle		m_angNetworkAngles;
};

//-----------------------------------------------------------------------------
// Purpose: Simple state tracking for changing model sideways shrinkage during barnacle swallow
//-----------------------------------------------------------------------------
struct ModelScale
{
	float		m_flModelScaleStart;
	float		m_flModelScaleGoal;
	float		m_flModelScaleFinishTime;
	float		m_flModelScaleStartTime;
};

#include "soundflags.h"

struct CSoundParameters;
typedef short HSOUNDSCRIPTHANDLE;
//-----------------------------------------------------------------------------
// Purpose: Aggregates and sets default parameters for EmitSound function calls
//-----------------------------------------------------------------------------
struct EmitSound_t
{
	EmitSound_t() :
		m_nChannel( 0 ),
		m_pSoundName( 0 ),
		m_flVolume( VOL_NORM ),
		m_SoundLevel( SNDLVL_NONE ),
		m_nFlags( 0 ),
		m_nPitch( PITCH_NORM ),
		m_nSpecialDSP( 0 ),
		m_pOrigin( 0 ),
		m_flSoundTime( 0.0f ),
		m_pflSoundDuration( 0 ),
		m_bEmitCloseCaption( true ),
		m_bWarnOnMissingCloseCaption( false ),
		m_bWarnOnDirectWaveReference( false ),
		m_nSpeakerEntity( -1 ),
		m_UtlVecSoundOrigin(),
		m_hSoundScriptHandle( -1 )
	{
	}

	EmitSound_t( const CSoundParameters &src );

	int							m_nChannel;
	char const					*m_pSoundName;
	float						m_flVolume;
	soundlevel_t				m_SoundLevel;
	int							m_nFlags;
	int							m_nPitch;
	int							m_nSpecialDSP;
	const Vector				*m_pOrigin;
	float						m_flSoundTime; ///< NOT DURATION, but rather, some absolute time in the future until which this sound should be delayed
	float						*m_pflSoundDuration;
	bool						m_bEmitCloseCaption;
	bool						m_bWarnOnMissingCloseCaption;
	bool						m_bWarnOnDirectWaveReference;
	int							m_nSpeakerEntity;
	mutable CUtlVector< Vector >	m_UtlVecSoundOrigin;  ///< Actual sound origin(s) (can be multiple if sound routed through speaker entity(ies) )
	mutable HSOUNDSCRIPTHANDLE		m_hSoundScriptHandle;
};

#define MAX_ACTORS_IN_SCENE 16

//-----------------------------------------------------------------------------
// Multiplayer specific defines
//-----------------------------------------------------------------------------
#define MAX_CONTROL_POINTS			8
#define MAX_CONTROL_POINT_GROUPS	8

// Maximum number of points that a control point may need owned to be cappable
#define MAX_PREVIOUS_POINTS			3

// The maximum number of teams the control point system knows how to deal with
#define MAX_CONTROL_POINT_TEAMS		8

// Maximum length of the cap layout string
#define MAX_CAPLAYOUT_LENGTH		32

// Maximum length of the current round printname
#define MAX_ROUND_NAME				32

// Maximum length of the current round name
#define MAX_ROUND_IMAGE_NAME		64

// Score added to the team score for a round win
#define TEAMPLAY_ROUND_WIN_SCORE	1

enum
{
	CP_WARN_NORMAL = 0,
	CP_WARN_FINALCAP,
	CP_WARN_NO_ANNOUNCEMENTS
};

// YWB:  3/12/2007
// Changing the following #define for Prediction Error checking (See gamemovement.cpp for overview) will to 1 or 2 enables the system, 0 turns it off
// Level 1 enables it, but doesn't force "full precision" networking, so you can still get lots of errors in position/velocity/etc.
// Level 2 enables it but also forces origins/angles to be sent full precision, so other fields can be error / tolerance checked
// NOTE:  This stuff only works on a listen server since it punches a hole from the client .dll to server .dll!!!
#define PREDICTION_ERROR_CHECK_LEVEL 0

//-----------------------------------------------------------------------------
// Round timer states
//-----------------------------------------------------------------------------
enum
{
	RT_STATE_SETUP,		// Timer is in setup mode
	RT_STATE_NORMAL,	// Timer is in normal mode
};

enum
{
	SIMULATION_TIME_WINDOW_BITS = 8,
};

//-----------------------------------------------------------------------------
// Commentary Mode
//-----------------------------------------------------------------------------
#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
#define GAME_HAS_NO_USE_KEY

#if defined( SPROP_COORD )
#undef SPROP_COORD
#endif

#define SPROP_COORD SPROP_COORD_MP

#endif

// The player's method of starting / stopping commentary
#ifdef GAME_HAS_NO_USE_KEY
#define COMMENTARY_BUTTONS		(IN_ATTACK | IN_ATTACK2 | IN_USE)
#else
#define COMMENTARY_BUTTONS		(IN_USE)
#endif

#define TEAM_TRAIN_MAX_TEAMS			4
#define TEAM_TRAIN_MAX_HILLS			5
#define TEAM_TRAIN_FLOATS_PER_HILL		2
#define TEAM_TRAIN_HILLS_ARRAY_SIZE		TEAM_TRAIN_MAX_TEAMS * TEAM_TRAIN_MAX_HILLS * TEAM_TRAIN_FLOATS_PER_HILL

enum
{
	HILL_TYPE_NONE = 0,
	HILL_TYPE_UPHILL,
	HILL_TYPE_DOWNHILL,
};

#define NOINTERP_PARITY_MAX			4
#define NOINTERP_PARITY_MAX_BITS	2

//-----------------------------------------------------------------------------
// Generic activity lookup support
//-----------------------------------------------------------------------------
enum
{
	kActivityLookup_Unknown = -2,			// hasn't been searched for
	kActivityLookup_Missing = -1,			// has been searched for but wasn't found
};

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
//-----------------------------------------------------------------------------
// Vision Filters.
//-----------------------------------------------------------------------------
// Also used in the item schema to define vision filter or vision mode opt in
#define TF_VISION_FILTER_NONE			0
#define TF_VISION_FILTER_PYRO			(1<<0)		// 1
#define TF_VISION_FILTER_HALLOWEEN		(1<<1)		// 2
#define TF_VISION_FILTER_ROME			(1<<2)		// 4

// THIS ENUM SHOULD MATCH THE ORDER OF THE FLAGS ABOVE
enum
{
	VISION_MODE_NONE = 0,
	VISION_MODE_PYRO,
	VISION_MODE_HALLOWEEN,
	VISION_MODE_ROME,

	MAX_VISION_MODES
};
#endif // TF_DLL || TF_CLIENT_DLL

class CPhysCollide;
struct collidelist_t
{
	const CPhysCollide	*pCollide;
	Vector			origin;
	QAngle			angles;
};


// TF2V Era classification:
// Epoch number is based on 09/17/2007 (the PC beta release) as Day 1.
// Quicker to save as a precalculated integer than strings.
// Calculate this by "September 16 2007 to <date>."


// Core timeline (anchor points)
#define TF2V_DAY_EPOCH                              0   // 2007/09/16 (Do not use: for reference only)
#define TF2V_DAY_BETA                               1   // 2007/09/17 (PC Beta Pre-Release)
#define TF2V_DAY_LAUNCH                            24   // 2007/10/10 (Orange Box Release)

// Major Updates
#define TF2V_DAY_MAJOR_GOLDRUSH                   226   // 2008/04/29 (Gold Rush Update)
#define TF2V_DAY_MAJOR_PYRO                       277   // 2008/06/19 (Pyro Update)
#define TF2V_DAY_MAJOR_HEAVY                      338   // 2008/08/19 (Heavy Update)
#define TF2V_DAY_MAJOR_SCOUT                      527   // 2009/02/24 (Scout Update)
#define TF2V_DAY_MAJOR_SNIPER_SPY                 613   // 2009/05/21 (Sniper vs Spy)
#define TF2V_DAY_MAJOR_CLASSLESS                  697   // 2009/08/13 (Classless Update)
#define TF2V_DAY_MAJOR_WAR                        823   // 2009/12/17 (WAR! Update)
#define TF2V_DAY_MAJOR_119                   	  956   // 2010/04/29 (119th Update)
#define TF2V_DAY_MAJOR_MAC                   	  998   // 2010/06/10 (Mac Update)
#define TF2V_DAY_MAJOR_ENGINEER                  1026   // 2010/07/08 (Engineer Update)
#define TF2V_DAY_MAJOR_MANNCONOMY                1110   // 2010/09/30 (Mannconomy Update)
#define TF2V_DAY_MAJOR_HATLESS                   1306   // 2011/04/14 (Hatless Update)
#define TF2V_DAY_MAJOR_REPLAY                    1327   // 2011/05/05 (Replay Update)
#define TF2V_DAY_MAJOR_UBER                      1376   // 2011/06/23 (Über Update)
#define TF2V_DAY_MAJOR_FOUNDRY                   1453   // 2011/09/08 (Foundry/Beta Update)
#define TF2V_DAY_MAJOR_MANNIVERSARY              1488   // 2011/10/13 (Manniversary Update)
#define TF2V_DAY_MAJOR_PYROMANIA                 1746   // 2012/06/27 (Pyromania)
#define TF2V_DAY_MAJOR_MVM                       1795   // 2012/08/15 (Mann vs Machine)
#define TF2V_DAY_MAJOR_MECHA                     1922   // 2012/12/20 (Mecha Update)
#define TF2V_DAY_MAJOR_ROBOTIC_BOOGALOO          2070   // 2013/05/17 (Robotic Boogaloo)
#define TF2V_DAY_MAJOR_TWOCITIES                 2258   // 2013/11/21 (Two Cities)
#define TF2V_DAY_MAJOR_LOVE_AND_WAR              2467   // 2014/06/18 (Love & War)
#define TF2V_DAY_MAJOR_END_OF_THE_LINE           2640   // 2014/12/08 (End of the Line)
#define TF2V_DAY_MAJOR_GUN_METTLE                2846   // 2015/07/02 (Gun Mettle)
#define TF2V_DAY_MAJOR_INVASION                  2942   // 2015/10/06 (Invasion)
#define TF2V_DAY_MAJOR_TOUGH_BREAK               3014   // 2015/12/17 (Tough Break)
#define TF2V_DAY_MAJOR_MEET_YOUR_MATCH           3217   // 2016/07/07 (Meet Your Match)
#define TF2V_DAY_MAJOR_JUNGLE_INFERNO            3686   // 2017/10/20 (Jungle Inferno)
// Last of the official patches.
#define TF2V_DAY_MAJOR_64BIT                     6059   // 2024/04/18 (64-bit Update)
#define TF2V_DAY_MAJOR_SDK                       6114   // 2024/06/12 (Source SDK Update)

// Content Packs
#define TF2V_DAY_CONTENT_FIRST_CONTENT             914    // 2010/03/18 (First Community Content)
#define TF2V_DAY_CONTENT_SECOND_CONTENT            977    // 2010/05/20 (Second Community Content)
#define TF2V_DAY_CONTENT_COMMUNITY_MAP             1257   // 2011/02/24 (Community Map Pack Update)
#define TF2V_DAY_CONTENT_SHOGUN                    1271   // 2011/03/10 (Shogun Pack)
#define TF2V_DAY_CONTENT_CHARITY             	   1283   // 2011/03/22 (Japan Charity)
#define TF2V_DAY_CONTENT_THIRD_CONTENT             1299   // 2011/04/07 (Third Community Content)
#define TF2V_DAY_CONTENT_GRORDBORT             	   1403   // 2011/07/20 (Dr. Grordbort's Victory Pack Update)
#define TF2V_DAY_CONTENT_MANNOTECHNOLOGY           1432   // 2011/08/18 (Manno-Technology Bundle)
#define TF2V_DAY_CONTENT_TRIAD                     1794   // 2012/08/14 (Triad Pack)
#define TF2V_DAY_CONTENT_STRONGBOX          	   2336   // 2014/02/07 (Strongbox Pack)

// Seasonal Updates
#define TF2V_DAY_FALL_2013                		3749   // 2013/10/10 (Fall Event 2013)
#define TF2V_DAY_SPRING_2016                 	3159   // 2016/05/11 (Mayflower)
#define TF2V_DAY_SPRING_2017                 	3528   // 2017/05/15 (Rainy Day)
#define TF2V_DAY_SPRING_2018                 	3845   // 2018/03/28 (Blue Moon)

// Seasonal Updates / Summer
#define TF2V_DAY_SUMMER_2011                     1383   // 2011/06/30 (Summer Camp Sale)
#define TF2V_DAY_SUMMER_2013                     2124   // 2013/07/10 (Summer Event 2013)
#define TF2V_DAY_SUMMER_2019                     4260   // 2019/05/16 
#define TF2V_DAY_SUMMER_2020                     4617   // 2020/05/07
#define TF2V_DAY_SUMMER_2021                     4981   // 2021/05/06
#define TF2V_DAY_SUMMER_2022                     5337   // 2022/04/27
#define TF2V_DAY_SUMMER_2023               		 5778   // 2023/07/12
#define TF2V_DAY_SUMMER_2024                     6071   // 2024/04/30
#define TF2V_DAY_SUMMER_2025                     6521   // 2025/07/24

// Holiday: Halloween (Scream Fortress)
#define TF2V_DAY_HALLOWEEN_2009                   774   // 2009/10/29 (Terrifying Team Fortress Haunted Hallowe'en Special)
#define TF2V_DAY_HALLOWEEN_2010                  1137   // 2010/10/27 (Second Annu-Hell Scream Fortress Hauntdead Halloween Special)
#define TF2V_DAY_HALLOWEEN_2011                  1502   // 2011/10/27 (Third Annual Scream Fortress Very Scary Halloween Special)
#define TF2V_DAY_HALLOWEEN_2012                  1867   // 2012/10/26 (Fourth Annual Spectral Halloween Special)
#define TF2V_DAY_HALLOWEEN_2013                  2235   // 2013/10/29 (Scream Fortress Fifth Annual Helloween Special)
#define TF2V_DAY_HALLOWEEN_2014                  2600   // 2014/10/29 (Scream Fortress VI)
#define TF2V_DAY_HALLOWEEN_2015                  2964   // 2015/10/28 (Scream Fortress VII)
#define TF2V_DAY_HALLOWEEN_2016                  3323   // 2016/10/21 (Scream Fortress VIII)
#define TF2V_DAY_HALLOWEEN_2017                  3687   // 2017/10/20 (Scream Fortress IX) (overlaps with Jungle Inferno)
#define TF2V_DAY_HALLOWEEN_2018                  4051   // 2018/10/19 (Scream Fortress X)
#define TF2V_DAY_HALLOWEEN_2019                  4406   // 2019/10/10 (Scream Fortress XI)
#define TF2V_DAY_HALLOWEEN_2020                  4764   // 2020/10/01 (Scream Fortress XII)
#define TF2V_DAY_HALLOWEEN_2021                  5133   // 2021/10/05 (Scream Fortress XIII)
#define TF2V_DAY_HALLOWEEN_2022                  5498   // 2022/10/05 (Scream Fortress XIV)
#define TF2V_DAY_HALLOWEEN_2023                  5867   // 2023/10/09 (Scream Fortress XV)
#define TF2V_DAY_HALLOWEEN_2024                  6233   // 2024/10/10 (Scream Fortress XVI)
#define TF2V_DAY_HALLOWEEN_2025                  6598   // 2025/10/09 (Scream Fortress XVII)

// Holiday: Australian Christmas / Smissmas
#define TF2V_DAY_SMISSMAS_2010               	 1187   // 2010/12/16 (Australian Christmas)
#define TF2V_DAY_SMISSMAS_2011               	 1551   // 2011/12/15 (Australian Christmas 2011)
#define TF2V_DAY_SMISSMAS_2012                   1922   // 2012/12/20 (Smissmass)
#define TF2V_DAY_SMISSMAS_2013                   2287   // 2013/12/20 (Smissmass 2013)
#define TF2V_DAY_SMISSMAS_2014                   2654   // 2014/12/22 (Smissmass 2014)
#define TF2V_DAY_SMISSMAS_2015                   3014   // 2015/12/17 (Smissmass 2015) (overlaps with Tough Break)
#define TF2V_DAY_SMISSMAS_2016                   3384   // 2016/12/21 (Smissmass 2016)
#define TF2V_DAY_SMISSMAS_2017                   3749   // 2017/12/21 (Smissmass 2017)
#define TF2V_DAY_SMISSMAS_2018                   4112   // 2018/12/19 (Smissmass 2018)
#define TF2V_DAY_SMISSMAS_2019                   4474   // 2019/12/16 (Smissmass 2019)
#define TF2V_DAY_SMISSMAS_2020                   4840   // 2020/12/16 (Smissmass 2020)
#define TF2V_DAY_SMISSMAS_2021                   5205   // 2021/12/16 (Smissmass 2021)
#define TF2V_DAY_SMISSMAS_2022                   5560   // 2022/12/06 (Smissmass 2022) (overlaps with VScript)
#define TF2V_DAY_SMISSMAS_2023                   5926   // 2023/12/07 (Smissmass 2023)
#define TF2V_DAY_SMISSMAS_2024                   6296   // 2024/12/11 (Smissmass 2024)
#define TF2V_DAY_SMISSMAS_2025                   6658   // 2025/12/09 (Smissmass 2025)

// Unnamed Promotional updates
#define TF2V_DAY_PROMO_SAM_MAX                    942   // 2010/04/15 (Sam & Max: The Devil's Playhouse)
#define TF2V_DAY_PROMO_ALIEN_SWARM               1037   // 2010/07/19 (Alien Swarm)
#define TF2V_DAY_PROMO_WORMS                     1074   // 2010/08/25 (Worms: Reloaded)
#define TF2V_DAY_PROMO_L4D2                      1116   // 2010/10/06 (Left 4 Dead 2)
#define TF2V_DAY_PROMO_POKER_NIGHT               1144   // 2010/11/03 (Poker Night at the Inventory)
#define TF2V_DAY_PROMO_POKER_NIGHT_UPDATE        1160   // 2010/11/19 (Poker Night Additional Items)
#define TF2V_DAY_PROMO_STEAM_TREASURE            1178   // 2010/12/07 (The Great Steam Treasure Hunt)
#define TF2V_DAY_PROMO_MNC                       1226   // 2011/01/24 (Monday Night Combat)
#define TF2V_DAY_PROMO_RIFT                      1234   // 2011/02/03 (RIFT)
#define TF2V_DAY_PROMO_HOMEFRONT                 1261   // 2011/02/28 (Homefront)
// Shogun Pack (March 10)
#define TF2V_DAY_PROMO_POTATO_SACK               1297   // 2011/04/05 (Portal 2/Potato Sack)
#define TF2V_DAY_PROMO_SPACECHEM                 1320   // 2011/04/28 (SpaceChem)
#define TF2V_DAY_PROMO_WITCHER2                  1334   // 2011/05/12 (The Witcher 2)
#define TF2V_DAY_PROMO_MAGICKA                   1353   // 2011/05/31 (Magicka: Nippon DLC)
#define TF2V_DAY_PROMO_RED_FACTION               1356   // 2011/06/03 (Red Faction: Armageddon / Magic The Gathering)
#define TF2V_DAY_PROMO_SPIRAL_KNIGHTS            1367   // 2011/06/14 (Spiral Knights)
// Dr. Grordbort's (July 20)
#define TF2V_DAY_PROMO_FROM_DUST                 1412   // 2011/07/29 (From Dust)
#define TF2V_DAY_PROMO_RED_ORCHESTRA             1416   // 2011/08/02 (Red Orchestra 2)
#define TF2V_DAY_PROMO_QUAKECON                  1417   // 2011/08/03 (Quakecon)
// Triad Pack (Aug 14) and Manno/Technology Bundle (Aug 18)
#define TF2V_DAY_PROMO_TROPICO4                  1444   // 2011/08/30 (Tropico 4)
#define TF2V_DAY_PROMO_FOOTBALL_MGR_2012         1489   // 2011/10/14 (Football Manager 2012)
#define TF2V_DAY_PROMO_MOUNT_BLADE               1508   // 2011/11/02 (Mount and Blade)
#define TF2V_DAY_PROMO_SAINTS_ROW                1516   // 2011/11/10 (Saints Row: The Third)
#define TF2V_DAY_PROMO_AC_REVELATIONS            1529   // 2011/11/23 (Assassin's Creed: Revelations)
#define TF2V_DAY_PROMO_KINGDOMS_AMALUR           1584   // 2012/01/17 (Kingdoms of Amalur)
#define TF2V_DAY_PROMO_TRINE2                    1591   // 2012/01/24 (Trine 2)
#define TF2V_DAY_PROMO_WARHAMMER                 1649   // 2012/03/22 (Warhammer 40,000: Space Marine)
#define TF2V_DAY_PROMO_MAX_PAYNE                 1705   // 2012/05/17 (Max Payne 3)
#define TF2V_DAY_PROMO_DOTA2                     1719   // 2012/05/31 (Dota 2)
#define TF2V_DAY_PROMO_KILLING_FLOOR             1719   // 2012/05/31 (Killing Floor)
#define TF2V_DAY_PROMO_AWESOMENAUTS              1782   // 2012/08/02 (Awesomenauts)
#define TF2V_DAY_PROMO_HERO_ACADEMY              1790   // 2012/08/10 (Hero Academy)
#define TF2V_DAY_PROMO_SLEEPING_DOGS             1795   // 2012/08/15 (Sleeping Dogs)
#define TF2V_DAY_PROMO_PLANTS_VS_ZOMBIES         1817   // 2012/09/06 (Plants vs Zombies)
#define TF2V_DAY_PROMO_BORDERLANDS2              1832   // 2012/09/21 (Borderlands 2)
#define TF2V_DAY_PROMO_DISHONORED                1838   // 2012/09/27 (Dishonored)
#define TF2V_DAY_PROMO_JAGGED_ALLIANCE           1858   // 2012/10/17 (Jagged Alliance)
#define TF2V_DAY_PROMO_SCRIBBLENAUTS             1888   // 2012/11/16 (Scribblenauts)
#define TF2V_DAY_PROMO_HITMAN                    1892   // 2012/11/20 (Hitman: Absolution)
#define TF2V_DAY_PROMO_SHOOT_MANY_ROBOTS         1902   // 2012/11/30
#define TF2V_DAY_PROMO_RAGE                      1949   // 2013/01/16 (R.A.G.E.)
#define TF2V_DAY_PROMO_BIOSHOCK                  1986   // 2013/02/22 (BioShock Infinite)
#define TF2V_DAY_PROMO_POKER_NIGHT_2             2047   // 2013/04/24 (Poker Night 2)
#define TF2V_DAY_PROMO_DONT_STARVE               2004   // 2013/03/12 (Don't Starve)
#define TF2V_DAY_PROMO_PC_GAMER                  2095   // 2013/06/11
#define TF2V_DAY_PROMO_VENTURE_BROS              2103   // 2013/06/19
#define TF2V_DAY_PROMO_PORTAL2                   2108   // 2013/06/24
#define TF2V_DAY_PROMO_QUANTUM_CONUNDRUM         2116   // 2013/07/02
#define TF2V_DAY_PROMO_WALKING_DEAD              2440   // 2014/05/22 (The Walking Dead Season 2)
#define TF2V_DAY_PROMO_QUAKE4                    2532   // 2014/08/22
#define TF2V_DAY_PROMO_ALIEN_ISOLATION           2573   // 2014/10/02 (Alien: Isolation)
#define TF2V_DAY_PROMO_LARA_CROFT                 2642   // 2014/12/10
#define TF2V_DAY_PROMO_ARKHAM_KNIGHT              3001   // 2015/12/04
#define TF2V_DAY_PROMO_DISGAEA                    3214   // 2016/07/04

#define TF2V_DAY_LAST_WEAPON_BALANCE			 TF2V_DAY_SPRING_2018 	  // 2018/03/28 (Blue Moon - Most recent weapon balancing)
#define TF2V_DAY_LATEST                          TF2V_DAY_SMISSMAS_2025   // 2025/12/09 (Smissmas 2025 - most recent content update)

#define TF2V_DAY_UNKNOWN                         11083   // 2038/01/19 (Y2K38 date - TF2 uses 32 bit time integers)

#define TF2V_ERA_DAY_MIN                         TF2V_DAY_BETA
#define TF2V_ERA_DAY_MAX                         TF2V_DAY_LATEST


#endif // SHAREDDEFS_H
