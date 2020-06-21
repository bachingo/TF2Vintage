
//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TF_SHAREDDEFS_H
#define TF_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "mp_shareddefs.h"

// Using MAP_DEBUG mode?
#ifdef MAP_DEBUG
	#define MDEBUG(x) x
#else
	#define MDEBUG(x)
#endif

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
enum
{
	TF_TEAM_RED = FIRST_GAME_TEAM,
	TF_TEAM_BLUE,
	TF_TEAM_GREEN,
	TF_TEAM_YELLOW,
	TF_TEAM_COUNT,

	TF_TEAM_NPC
};



#define TF_TEAM_PLAYER_BOSS TF_TEAM_BLUE
#define TF_TEAM_PLAYER_HORDE TF_TEAM_RED

#define TF_TEAM_MVM_BOTS TF_TEAM_BLUE
#define TF_TEAM_MVM_PLAYERS TF_TEAM_RED

#define TF_TEAM_AUTOASSIGN (TF_TEAM_COUNT + 1 )

extern const char *g_aTeamNames[TF_TEAM_COUNT];
extern const char *g_aTeamNamesShort[TF_TEAM_COUNT];
extern const char *g_aTeamParticleNames[TF_TEAM_COUNT];
extern color32 g_aTeamColors[TF_TEAM_COUNT];
extern color32 g_aTeamSkinColors[TF_TEAM_COUNT];

bool IsTeamName( const char *name );

const char *GetTeamParticleName( int iTeam, bool bDummyBoolean = false, const char **pNames = g_aTeamParticleNames );
const char *ConstructTeamParticle( const char *pszFormat, int iTeam, bool bDummyBoolean = false, const char **pNames = g_aTeamParticleNames );
void PrecacheTeamParticles( const char *pszFormat, bool bDummyBoolean = false, const char **pNames = g_aTeamParticleNames );

#define CONTENTS_REDTEAM	CONTENTS_TEAM1
#define CONTENTS_BLUETEAM	CONTENTS_TEAM2
#define CONTENTS_GREENTEAM	CONTENTS_UNUSED
#define CONTENTS_YELLOWTEAM	CONTENTS_UNUSED6
			
// Team roles
enum 
{
	TEAM_ROLE_NONE = 0,
	TEAM_ROLE_DEFENDERS,
	TEAM_ROLE_ATTACKERS,

	NUM_TEAM_ROLES,
};

//-----------------------------------------------------------------------------
// CVar replacements
//-----------------------------------------------------------------------------
#define TF_DAMAGE_CRIT_CHANCE				0.02f // Originally 0.05f, deprecated by tf2v_critchance
#define TF_DAMAGE_CRIT_CHANCE_RAPID			0.02f // Originally 0.05f, deprecated by tf2v_critchance_rapid
#define TF_DAMAGE_CRIT_DURATION_RAPID		2.0f  // Originally 2.0f, deprecated by tf2v_crit_duration_rapid
#define TF_DAMAGE_CRIT_CHANCE_MELEE			0.10f // Originally 0.15f, deprecated by tf2v_critchance_melee

#define TF_DAMAGE_CRITMOD_MAXTIME			20
#define TF_DAMAGE_CRITMOD_MINTIME			2
#define TF_DAMAGE_CRITMOD_DAMAGE			800	  // Originally 1600, deprecated by tf2v_critmod_range
#define TF_DAMAGE_CRITMOD_MAXMULT			6

#define TF_DAMAGE_CRIT_MULTIPLIER			3.0f
#define TF_DAMAGE_MINICRIT_MULTIPLIER		1.35f

#define TF_ROCKET_RADIUS	146.0f	// Matches grenade radius.
#define TF_ROCKET_SELF_DAMAGE_RADIUS	121.0f	// Original rocket radius, used for self damage.
#define TF_ROCKET_SELF_RADIUS_RATIO ( TF_ROCKET_SELF_DAMAGE_RADIUS / TF_ROCKET_RADIUS ) // Used for self damage calculations on certain projectiles.

//-----------------------------------------------------------------------------
// TF-specific viewport panels
//-----------------------------------------------------------------------------
#define PANEL_CLASS_BLUE		"class_blue"
#define PANEL_CLASS_RED			"class_red"
#define PANEL_CLASS_GREEN		"class_green"
#define PANEL_CLASS_YELLOW		"class_yellow"
#define PANEL_MAPINFO			"mapinfo"
#define PANEL_ROUNDINFO			"roundinfo"
#define PANEL_ARENATEAMSELECT 	"arenateamselect"

#define PANEL_FOURTEAMSCOREBOARD "fourteamscoreboard"
#define PANEL_FOURTEAMSELECT	"fourteamselect"

// file we'll save our list of viewed intro movies in
#define MOVIES_FILE				"viewed.res"

//-----------------------------------------------------------------------------
// Used in calculating the health percentage of a player
//-----------------------------------------------------------------------------
#define TF_HEALTH_UNDEFINED		1

//-----------------------------------------------------------------------------
// Used to mark a spy's disguise attribute (team or class) as "unused"
//-----------------------------------------------------------------------------
#define TF_SPY_UNDEFINED		TEAM_UNASSIGNED

#define COLOR_TF_BLUE	Color( 64, 64, 255, 255 )
#define COLOR_TF_RED	Color( 255, 64, 64, 255 )
#define COLOR_TF_GREEN	Color( 64, 255, 64, 255 )
#define COLOR_TF_YELLOW	Color( 255, 255, 64, 255 )
#define COLOR_TF_SPECTATOR Color( 245, 229, 196, 255 )

#define COLOR_EYEBALLBOSS_TEXT	Color( 134, 80, 172, 255 )
#define COLOR_MERASMUS_TEXT	Color( 112, 176, 74, 255 )


//-----------------------------------------------------------------------------
// Player Classes.
//-----------------------------------------------------------------------------

#define TF_FIRST_NORMAL_CLASS	( TF_CLASS_UNDEFINED + 1 )
#define TF_LAST_NORMAL_CLASS	( TF_CLASS_UNDEFINED + 9 )

#define TF_FIRST_BOSS_CLASS		( TF_LAST_NORMAL_CLASS + 1 )
#define TF_LAST_BOSS_CLASS		( TF_CLASS_COUNT_ALL - 1 )


#define	TF_CLASS_MENU_BUTTONS	( TF_CLASS_RANDOM + 1 )

enum
{
	TF_CLASS_UNDEFINED = 0,

	TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
    TF_CLASS_SNIPER,
    TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,		// TF_LAST_NORMAL_CLASS
	
	// New classes go here.
	// These are special classes and are not found normally.
	TF_CLASS_SAXTON,		// TF_FIRST_BOSS_CLASS
							// TF_LAST_BOSS_CLASS
	TF_CLASS_COUNT_ALL,

	TF_CLASS_RANDOM
};

extern const char *g_aPlayerClassNames[];				// localized class names
extern const char *g_aPlayerClassNames_NonLocalized[];	// non-localized class names
extern const char *g_aRawPlayerClassNames[TF_CLASS_MENU_BUTTONS];
extern const char *g_aRawPlayerClassNamesShort[TF_CLASS_MENU_BUTTONS];

bool IsPlayerClassName( const char *name );
int GetClassIndexFromString( const char *name, int maxClass = TF_LAST_NORMAL_CLASS );
char const *GetPlayerClassName( int iClassIdx );
char const *GetPlayerClassLocalizationKey( int iClassIdx );

extern const char *g_aDominationEmblems[];
extern const char *g_aPlayerClassEmblems[];
extern const char *g_aPlayerClassEmblemsDead[];

//-----------------------------------------------------------------------------
// For entity_capture_flags to use when placed in the world
//-----------------------------------------------------------------------------
enum
{
	TF_FLAGTYPE_CTF = 0,
	TF_FLAGTYPE_ATTACK_DEFEND,
	TF_FLAGTYPE_TERRITORY_CONTROL,
	TF_FLAGTYPE_INVADE,
	TF_FLAGTYPE_KINGOFTHEHILL,
};

//-----------------------------------------------------------------------------
// For the game rules to determine which type of game we're playing
//-----------------------------------------------------------------------------
enum
{
	TF_GAMETYPE_UNDEFINED = 0,
	TF_GAMETYPE_CTF,
	TF_GAMETYPE_CP,
	TF_GAMETYPE_ESCORT,
	TF_GAMETYPE_ARENA,
	TF_GAMETYPE_MVM,
	TF_GAMETYPE_RD,
	TF_GAMETYPE_PASSTIME,
	TF_GAMETYPE_PD,
	TF_GAMETYPE_MEDIEVAL,
};
extern const char *g_aGameTypeNames[];	// localized gametype names

//-----------------------------------------------------------------------------
// Buildings.
//-----------------------------------------------------------------------------
enum
{
	TF_BUILDING_SENTRY				= (1<<0),
	TF_BUILDING_DISPENSER			= (1<<1),
	TF_BUILDING_TELEPORT			= (1<<2),
};

//-----------------------------------------------------------------------------
// Items.
//-----------------------------------------------------------------------------
enum
{
	TF_ITEM_UNDEFINED		= 0,
	TF_ITEM_CAPTURE_FLAG	= (1<<0),
	TF_ITEM_HEALTH_KIT		= (1<<1),
	TF_ITEM_ARMOR			= (1<<2),
	TF_ITEM_AMMO_PACK		= (1<<3),
	TF_ITEM_GRENADE_PACK	= (1<<4),
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
enum
{
	TF_AMMO_DUMMY = 0,	// Dummy index to make the CAmmoDef indices correct for the other ammo types.
	TF_AMMO_PRIMARY,
	TF_AMMO_SECONDARY,
	TF_AMMO_METAL,
	TF_AMMO_GRENADES1,
	TF_AMMO_GRENADES2,
	TF_AMMO_GRENADES3, // Spells etc.
	TF_AMMO_COUNT
};

enum EAmmoSource
{
	TF_AMMO_SOURCE_AMMOPACK = 0, // Default, used for ammopacks
	TF_AMMO_SOURCE_RESUPPLY, // Maybe?
	TF_AMMO_SOURCE_DISPENSER,
	TF_AMMO_SOURCE_COUNT
};

//-----------------------------------------------------------------------------
// Grenade Launcher mode (for pipebombs).
//-----------------------------------------------------------------------------
enum
{
	TF_GL_MODE_REGULAR = 0,
	TF_GL_MODE_REMOTE_DETONATE,
	TF_GL_MODE_FIZZLE,
	TF_GL_MODE_BETA_DETONATE,
	TF_GL_MODE_CANNONBALL,
};

//-----------------------------------------------------------------------------
// Weapon Types
//-----------------------------------------------------------------------------
enum
{
	TF_WPN_TYPE_PRIMARY = 0,
	TF_WPN_TYPE_SECONDARY,
	TF_WPN_TYPE_MELEE,
	TF_WPN_TYPE_GRENADE,
	TF_WPN_TYPE_BUILDING,
	TF_WPN_TYPE_PDA,
	TF_WPN_TYPE_ITEM1,
	TF_WPN_TYPE_ITEM2,
	TF_WPN_TYPE_HEAD,
	TF_WPN_TYPE_MISC,
	TF_WPN_TYPE_MELEE_ALLCLASS,
	TF_WPN_TYPE_SECONDARY2,
	TF_WPN_TYPE_PRIMARY2,
	TF_WPN_TYPE_ITEM3,
	TF_WPN_TYPE_ITEM4,

	TF_WPN_TYPE_COUNT
};

extern const char *g_AnimSlots[];
extern const char *g_LoadoutSlots[];
extern const char *g_InventoryLoadoutPresets[];
extern const char *g_LoadoutTranslations[];
extern const char *g_LoadoutDropTypes[];

//-----------------------------------------------------------------------------
// Loadout slots
//-----------------------------------------------------------------------------
enum
{
	TF_LOADOUT_SLOT_PRIMARY = 0,
	TF_LOADOUT_SLOT_SECONDARY,
	TF_LOADOUT_SLOT_MELEE,
	TF_LOADOUT_SLOT_PDA1,
	TF_LOADOUT_SLOT_PDA2,
	TF_LOADOUT_SLOT_BUILDING,		// TF_PLAYER_WEAPON_COUNT	
	
	TF_LOADOUT_SLOT_UTILITY,		
	TF_LOADOUT_SLOT_ACTION,
	
	TF_LOADOUT_SLOT_HAT,	// TF_FIRST_COSMETIC_SLOT
	TF_LOADOUT_SLOT_MISC1,
	TF_LOADOUT_SLOT_MISC2,
	TF_LOADOUT_SLOT_MISC3,
	TF_LOADOUT_SLOT_EVENT,
	TF_LOADOUT_SLOT_MEDAL,	// TF_LAST_COSMETIC_SLOT
							
	TF_LOADOUT_SLOT_TAUNT1,	// TF_FIRST_TAUNT_SLOT
	TF_LOADOUT_SLOT_TAUNT2,
	TF_LOADOUT_SLOT_TAUNT3,
	TF_LOADOUT_SLOT_TAUNT4,
	TF_LOADOUT_SLOT_TAUNT5,
	TF_LOADOUT_SLOT_TAUNT6,
	TF_LOADOUT_SLOT_TAUNT7,
	TF_LOADOUT_SLOT_TAUNT8, // TF_LAST_TAUNT_SLOT
	
	TF_LOADOUT_SLOT_COUNT
};

extern const char *g_aAmmoNames[];

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
#define TF_PLAYER_WEAPON_COUNT		TF_LOADOUT_SLOT_BUILDING		// This is for weapon slots. for all slots, see TF_LOADOUT_SLOT_COUNT.
#define TF_PLAYER_GRENADE_COUNT		2
#define TF_PLAYER_BUILDABLE_COUNT	4

#define TF_FIRST_COSMETIC_SLOT TF_LOADOUT_SLOT_HAT
#define TF_LAST_COSMETIC_SLOT TF_LOADOUT_SLOT_MEDAL

#define TF_FIRST_TAUNT_SLOT	TF_LOADOUT_SLOT_TAUNT1
#define TF_LAST_TAUNT_SLOT TF_LOADOUT_SLOT_TAUNT8


#define TF_PLAYER_MISC_COUNT		3		// Total amount of all misc slots.
#define TF_PLAYER_TAUNT_COUNT		8		// Total amount of all action slots.

#define TF_WEAPON_PRIMARY_MODE		0
#define TF_WEAPON_SECONDARY_MODE	1

#define TF_WEAPON_GRENADE_FRICTION						0.6f
#define TF_WEAPON_GRENADE_GRAVITY						0.81f
#define TF_WEAPON_GRENADE_INITPRIME						0.8f
#define TF_WEAPON_GRENADE_CONCUSSION_TIME				15.0f
#define TF_WEAPON_GRENADE_MIRV_BOMB_COUNT				4
#define TF_WEAPON_GRENADE_CALTROP_TIME					8.0f

#define TF_WEAPON_PIPEBOMB_WORLD_COUNT					15
#define TF_WEAPON_PIPEBOMB_COUNT						8
#define TF_WEAPON_PIPEBOMB_INTERVAL						0.6f
#define TF_PIPEBOMB_MIN_CHARGE_VEL						900
#define TF_PIPEBOMB_MAX_CHARGE_VEL						2400
#define TF_PIPEBOMB_MAX_CHARGE_TIME						4.0f

#define TF_WEAPON_ROCKET_INTERVAL						0.8f

#define TF_WEAPON_FLAMETHROWER_INTERVAL					0.15f
#define TF_WEAPON_FLAMETHROWER_ROCKET_INTERVAL			0.8f

#define TF_BOW_MIN_CHARGE_DAMAGE						50.0f
#define TF_BOW_MIN_CHARGE_VEL							1800
#define TF_BOW_MAX_CHARGE_VEL							2600
#define TF_BOW_MAX_CHARGE_TIME							1.0f
#define TF_BOW_CHARGE_TIRED_TIME						5.0f
#define TF_BOW_TIRED_SPREAD								6.0f

#define TF_WEAPON_ZOOM_FOV								20

#define TF_WEAPON_MAX_REVENGE							35

// Weapon Identifications as defined in Live TF2
typedef enum class WeaponId {
	TF_WEAPON_NONE = 0,
	TF_WEAPON_BAT,
	TF_WEAPON_BOTTLE,
	TF_WEAPON_FIREAXE,
	TF_WEAPON_CLUB,
	TF_WEAPON_CROWBAR,
	TF_WEAPON_KNIFE,
	TF_WEAPON_FISTS,
	TF_WEAPON_SHOVEL,
	TF_WEAPON_WRENCH,
	TF_WEAPON_BONESAW,
	TF_WEAPON_SHOTGUN_PRIMARY,
	TF_WEAPON_SHOTGUN_SOLDIER,
	TF_WEAPON_SHOTGUN_HWG,
	TF_WEAPON_SHOTGUN_PYRO,
	TF_WEAPON_SCATTERGUN,
	TF_WEAPON_SNIPERRIFLE,
	TF_WEAPON_MINIGUN,
	TF_WEAPON_SMG,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_TRANQ,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_FLAMETHROWER,
	TF_WEAPON_GRENADE_NORMAL,
	TF_WEAPON_GRENADE_CONCUSSION,
	TF_WEAPON_GRENADE_NAIL,
	TF_WEAPON_GRENADE_MIRV,
	TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	TF_WEAPON_GRENADE_NAPALM,
	TF_WEAPON_GRENADE_GAS,
	TF_WEAPON_GRENADE_EMP,
	TF_WEAPON_GRENADE_CALTROP,
	TF_WEAPON_GRENADE_PIPEBOMB,
	TF_WEAPON_GRENADE_SMOKE_BOMB,
	TF_WEAPON_GRENADE_HEAL,
	TF_WEAPON_PISTOL,
	TF_WEAPON_PISTOL_SCOUT,
	TF_WEAPON_REVOLVER,
	TF_WEAPON_NAILGUN,
	TF_WEAPON_PDA,
	TF_WEAPON_PDA_ENGINEER_BUILD,
	TF_WEAPON_PDA_ENGINEER_DESTROY,
	TF_WEAPON_PDA_SPY,
	TF_WEAPON_BUILDER,
	TF_WEAPON_MEDIGUN,
	TF_WEAPON_GRENADE_MIRVBOMB,
	TF_WEAPON_FLAMETHROWER_ROCKET,
	TF_WEAPON_GRENADE_DEMOMAN,
	TF_WEAPON_SENTRY_BULLET,
	TF_WEAPON_SENTRY_ROCKET,
	TF_WEAPON_DISPENSER,
	TF_WEAPON_INVIS,
	TF_WEAPON_FLAG, // ADD NEW WEAPONS AFTER THIS
	TF_WEAPON_FLAREGUN,
	TF_WEAPON_LUNCHBOX,
	TF_WEAPON_LUNCHBOX_DRINK,
	TF_WEAPON_COMPOUND_BOW,
	TF_WEAPON_JAR,
	TF_WEAPON_LASER_POINTER,
	TF_WEAPON_HANDGUN_SCOUT_PRIMARY,
	TF_WEAPON_STICKBOMB,
	TF_WEAPON_BAT_WOOD,
	TF_WEAPON_ROBOT_ARM,
	TF_WEAPON_BUFF_ITEM,
	TF_WEAPON_SWORD,
	TF_WEAPON_SENTRY_REVENGE,
	TF_WEAPON_JAR_MILK,
	TF_WEAPON_ASSAULTRIFLE,
	TF_WEAPON_MINIGUN_REAL,
	TF_WEAPON_HUNTERRIFLE,
	TF_WEAPON_UMBRELLA,
	TF_WEAPON_HAMMERFISTS,
	TF_WEAPON_CHAINSAW,
	TF_WEAPON_HEAVYARTILLERY,
	TF_WEAPON_ROCKETLAUNCHER_LEGACY,
	TF_WEAPON_GRENADELAUNCHER_LEGACY,
	TF_WEAPON_PIPEBOMBLAUNCHER_LEGACY,
	TF_WEAPON_CROSSBOW,
	TF_WEAPON_PIPEBOMBLAUNCHER_TF2BETA,
	TF_WEAPON_PIPEBOMBLAUNCHER_TFC,
	TF_WEAPON_SYRINGE,
	TF_WEAPON_SNIPERRIFLE_REAL,
	TF_WEAPON_SNIPERRIFLE_CLASSIC,
	TF_WEAPON_GRENADE_PIPEBOMB_BETA,
	TF_WEAPON_SHOVELFIST,
	TF_WEAPON_SODA_POPPER,
	TF_WEAPON_PEP_BRAWLER_BLASTER,
	TF_WEAPON_SNIPERRIFLE_DECAP,
	TF_WEAPON_KATANA,
	TF_WEAPON_ROCKETLAUNCHER_AIRSTRIKE,
	TF_WEAPON_PARACHUTE,
	TF_WEAPON_SLAP,
	TF_WEAPON_REVOLVER_DEX,
	TF_WEAPON_PUMPKIN_BOMB,
	TF_WEAPON_GRENADE_STUNBALL,
	TF_WEAPON_GRENADE_JAR,
	TF_WEAPON_GRENADE_JAR_MILK,
	TF_WEAPON_DIRECTHIT,
	TF_WEAPON_LIFELINE,
	TF_WEAPON_DISPENSER_GUN,
	TF_WEAPON_BAT_FISH,
	TF_WEAPON_HANDGUN_SCOUT_SEC,
	TF_WEAPON_RAYGUN,
	TF_WEAPON_PARTICLE_CANNON,
	TF_WEAPON_MECHANICAL_ARM,
	TF_WEAPON_DRG_POMSON,
	TF_WEAPON_BAT_GIFTWRAP,
	TF_WEAPON_GRENADE_ORNAMENT,
	TF_WEAPON_RAYGUN_REVENGE,
	TF_WEAPON_CLEAVER,
	TF_WEAPON_GRENADE_CLEAVER,
	TF_WEAPON_STICKY_BALL_LAUNCHER,
	TF_WEAPON_GRENADE_STICKY_BALL,
	TF_WEAPON_SHOTGUN_BUILDING_RESCUE,
	TF_WEAPON_CANNON,
	TF_WEAPON_THROWABLE,
	TF_WEAPON_GRENADE_THROWABLE,
	TF_WEAPON_PDA_SPY_BUILD,
	TF_WEAPON_GRENADE_WATERBALLOON,
	TF_WEAPON_HARVESTER_SAW,
	TF_WEAPON_SPELLBOOK,
	TF_WEAPON_SPELLBOOK_PROJECTILE,
	TF_WEAPON_GRAPPLINGHOOK,
	TF_WEAPON_PASSTIME_GUN,
	TF_WEAPON_CHARGED_SMG,
	TF_WEAPON_BREAKABLE_SIGN,
	TF_WEAPON_ROCKETPACK,
	TF_WEAPON_JAR_GAS,
	TF_WEAPON_GRENADE_JAR_GAS,
	TF_WEAPON_FLAME_BALL,
	TF_WEAPON_FLAREGUN_REVENGE,
	TF_WEAPON_ROCKETLAUNCHER_FIREBALL,
} WeaponId_t;

enum
{
	TF_WEAPON_NONE = 0,
	TF_WEAPON_BAT,
	TF_WEAPON_BOTTLE,
	TF_WEAPON_FIREAXE,
	TF_WEAPON_CLUB,
	TF_WEAPON_CROWBAR,
	TF_WEAPON_KNIFE,
	TF_WEAPON_FISTS,
	TF_WEAPON_SHOVEL,
	TF_WEAPON_WRENCH,
	TF_WEAPON_BONESAW,
	TF_WEAPON_SHOTGUN_PRIMARY,
	TF_WEAPON_SHOTGUN_SOLDIER,
	TF_WEAPON_SHOTGUN_HWG,
	TF_WEAPON_SHOTGUN_PYRO,
	TF_WEAPON_SCATTERGUN,
	TF_WEAPON_SNIPERRIFLE,
	TF_WEAPON_MINIGUN,
	TF_WEAPON_SMG,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_TRANQ,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_FLAMETHROWER,
	TF_WEAPON_GRENADE_NORMAL,
	TF_WEAPON_GRENADE_CONCUSSION,
	TF_WEAPON_GRENADE_NAIL,
	TF_WEAPON_GRENADE_MIRV,
	TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	TF_WEAPON_GRENADE_NAPALM,
	TF_WEAPON_GRENADE_GAS,
	TF_WEAPON_GRENADE_EMP,
	TF_WEAPON_GRENADE_CALTROP,
	TF_WEAPON_GRENADE_PIPEBOMB,
	TF_WEAPON_GRENADE_SMOKE_BOMB,
	TF_WEAPON_GRENADE_HEAL,
	TF_WEAPON_PISTOL,
	TF_WEAPON_PISTOL_SCOUT,
	TF_WEAPON_REVOLVER,
	TF_WEAPON_NAILGUN,
	TF_WEAPON_PDA,
	TF_WEAPON_PDA_ENGINEER_BUILD,
	TF_WEAPON_PDA_ENGINEER_DESTROY,
	TF_WEAPON_PDA_SPY,
	TF_WEAPON_BUILDER,
	TF_WEAPON_MEDIGUN,
	TF_WEAPON_GRENADE_MIRVBOMB,
	TF_WEAPON_FLAMETHROWER_ROCKET,
	TF_WEAPON_GRENADE_DEMOMAN,
	TF_WEAPON_SENTRY_BULLET,
	TF_WEAPON_SENTRY_ROCKET,
	TF_WEAPON_DISPENSER,
	TF_WEAPON_INVIS,
	TF_WEAPON_FLAG, // ADD NEW WEAPONS AFTER THIS
	TF_WEAPON_FLAREGUN,
	TF_WEAPON_LUNCHBOX,
	TF_WEAPON_LUNCHBOX_DRINK,
	TF_WEAPON_COMPOUND_BOW,
	TF_WEAPON_JAR,
	TF_WEAPON_LASER_POINTER,
	TF_WEAPON_HANDGUN_SCOUT_PRIMARY,
	TF_WEAPON_STICKBOMB,
	TF_WEAPON_BAT_WOOD,
	TF_WEAPON_ROBOT_ARM,
	TF_WEAPON_BUFF_ITEM,
	TF_WEAPON_SWORD,
	TF_WEAPON_SENTRY_REVENGE,
	TF_WEAPON_JAR_MILK,
	TF_WEAPON_ASSAULTRIFLE,
	TF_WEAPON_MINIGUN_REAL,
	TF_WEAPON_HUNTERRIFLE,
	TF_WEAPON_UMBRELLA,
	TF_WEAPON_HAMMERFISTS,
	TF_WEAPON_CHAINSAW,
	TF_WEAPON_HEAVYARTILLERY,
	TF_WEAPON_ROCKETLAUNCHER_LEGACY,
	TF_WEAPON_GRENADELAUNCHER_LEGACY,
	TF_WEAPON_PIPEBOMBLAUNCHER_LEGACY,
	TF_WEAPON_CROSSBOW,
	TF_WEAPON_PIPEBOMBLAUNCHER_TF2BETA,
	TF_WEAPON_PIPEBOMBLAUNCHER_TFC,
	TF_WEAPON_SYRINGE,
	TF_WEAPON_SNIPERRIFLE_REAL,
	TF_WEAPON_SNIPERRIFLE_CLASSIC,
	TF_WEAPON_GRENADE_PIPEBOMB_BETA,
	TF_WEAPON_SHOVELFIST,
	TF_WEAPON_SODA_POPPER,
	TF_WEAPON_PEP_BRAWLER_BLASTER,
	TF_WEAPON_SNIPERRIFLE_DECAP,
	TF_WEAPON_KATANA,
	TF_WEAPON_ROCKETLAUNCHER_AIRSTRIKE,
	TF_WEAPON_PARACHUTE,
	TF_WEAPON_SLAP,
	TF_WEAPON_REVOLVER_DEX,
	TF_WEAPON_PUMPKIN_BOMB,
	TF_WEAPON_GRENADE_STUNBALL,
	TF_WEAPON_GRENADE_JAR,
	TF_WEAPON_GRENADE_JAR_MILK,
	TF_WEAPON_DIRECTHIT,
	TF_WEAPON_LIFELINE,
	TF_WEAPON_DISPENSER_GUN,
	TF_WEAPON_BAT_FISH,
	TF_WEAPON_HANDGUN_SCOUT_SEC,
	TF_WEAPON_RAYGUN,
	TF_WEAPON_PARTICLE_CANNON,
	TF_WEAPON_MECHANICAL_ARM,
	TF_WEAPON_DRG_POMSON,
	TF_WEAPON_BAT_GIFTWRAP,
	TF_WEAPON_GRENADE_ORNAMENT,
	TF_WEAPON_RAYGUN_REVENGE,
	TF_WEAPON_CLEAVER,
	TF_WEAPON_GRENADE_CLEAVER,
	TF_WEAPON_STICKY_BALL_LAUNCHER,
	TF_WEAPON_GRENADE_STICKY_BALL,
	TF_WEAPON_SHOTGUN_BUILDING_RESCUE,
	TF_WEAPON_CANNON,
	TF_WEAPON_THROWABLE,
	TF_WEAPON_GRENADE_THROWABLE,
	TF_WEAPON_PDA_SPY_BUILD,
	TF_WEAPON_GRENADE_WATERBALLOON,
	TF_WEAPON_HARVESTER_SAW,
	TF_WEAPON_SPELLBOOK,
	TF_WEAPON_SPELLBOOK_PROJECTILE,
	TF_WEAPON_GRAPPLINGHOOK,
	TF_WEAPON_PASSTIME_GUN,
	TF_WEAPON_CHARGED_SMG,
	TF_WEAPON_BREAKABLE_SIGN,
	TF_WEAPON_ROCKETPACK,
	TF_WEAPON_JAR_GAS,
	TF_WEAPON_GRENADE_JAR_GAS,
	TF_WEAPON_FLAME_BALL,
	TF_WEAPON_FLAREGUN_REVENGE,
	TF_WEAPON_ROCKETLAUNCHER_FIREBALL,

	TF_WEAPON_COUNT
};

extern const char *g_aWeaponNames[];
extern int g_aWeaponDamageTypes[];
extern const Vector g_vecFixedWpnSpreadPellets[];

int GetWeaponId( const char *pszWeaponName );
#ifdef GAME_DLL
int GetWeaponFromDamage( const CTakeDamageInfo &info );
#endif
int GetBuildableId( const char *pszBuildableName );

const char *WeaponIdToAlias( int iWeapon );
const char *WeaponIdToClassname( int iWeapon );
const char *TranslateWeaponEntForClass( const char *pszName, int iClass );

bool WeaponID_IsSniperRifle( int iWeaponID );
bool WeaponID_IsLunchbox( int iWeaponID );

enum
{
	TF_PROJECTILE_NONE,
	TF_PROJECTILE_BULLET,
	TF_PROJECTILE_ROCKET,
	TF_PROJECTILE_PIPEBOMB,
	TF_PROJECTILE_PIPEBOMB_REMOTE,
	TF_PROJECTILE_SYRINGE,
	TF_PROJECTILE_FLARE,
	TF_PROJECTILE_JAR,
	TF_PROJECTILE_ARROW,
	TF_PROJECTILE_FLAME_ROCKET,
	TF_PROJECTILE_JAR_MILK,
	TF_PROJECTILE_HEALING_BOLT,
	TF_PROJECTILE_ENERGY_BALL,
	TF_PROJECTILE_ENERGY_RING,
	TF_PROJECTILE_PIPEBOMB_REMOTE_PRACTICE,
	TF_PROJECTILE_CLEAVER,
	TF_PROJECTILE_STICKY_BALL,
	TF_PROJECTILE_CANNONBALL,
	TF_PROJECTILE_BUILDING_REPAIR_BOLT,
	TF_PROJECTILE_FESTIVE_ARROW,
	TF_PROJECTILE_THROWABLE,
	TF_PROJECTILE_SPELLFIREBALL,
	TF_PROJECTILE_FESTIVE_URINE,
	TF_PROJECTILE_FESTIVE_HEALING_BOLT,
	TF_PROJECTILE_BREADMONSTER_JARATE,
	TF_PROJECTILE_BREADMONSTER_MADMILK,
	TF_PROJECTILE_GRAPPLINGHOOK,
	TF_PROJECTILE_SENTRY_ROCKET,
	TF_PROJECTILE_BREAD_MONSTER,
	// Legacy content.
	TF_PROJECTILE_NAIL,
	TF_PROJECTILE_DART,
	// Cut grenade content.
	TF_WEAPON_GRENADE_CALTROP_PROJECTILE,
	TF_WEAPON_GRENADE_CONCUSSION_PROJECTILE,
	TF_WEAPON_GRENADE_EMP_PROJECTILE,
	TF_WEAPON_GRENADE_GAS_PROJECTILE,
	TF_WEAPON_GRENADE_HEAL_PROJECTILE,
	TF_WEAPON_GRENADE_MIRV_PROJECTILE,
	TF_WEAPON_GRENADE_NAIL_PROJECTILE,
	TF_WEAPON_GRENADE_NAPALM_PROJECTILE,
	TF_WEAPON_GRENADE_NORMAL_PROJECTILE,
	TF_WEAPON_GRENADE_SMOKE_BOMB_PROJECTILE,
	TF_WEAPON_GRENADE_PIPEBOMB_PROJECTILE,
	// End of grenade content.
	// Add new ones below this line!
	TF_PROJECTILE_BALLOFFIRE,
	TF_PROJECTILE_ENERGYORB,
	TF_PROJECTILE_JAR_GAS,
	
	

	TF_NUM_PROJECTILES
};

extern const char *g_szProjectileNames[];

//-----------------------------------------------------------------------------
// Attributes.
//-----------------------------------------------------------------------------
#define TF_PLAYER_VIEW_OFFSET	Vector( 0, 0, 64.0 ) //--> see GetViewVectors()

//-----------------------------------------------------------------------------
// TF Player Condition.
//-----------------------------------------------------------------------------

// Burning
// Original Burn Definitions
#define TF_BURNING_FREQUENCY		0.5f		// 2 ticks per second
#define TF_BURNING_FLAME_LIFE		10.0
#define TF_BURNING_FLAME_LIFE_PYRO	0.25		// pyro only displays burning effect momentarily
#define TF_BURNING_DMG				3

// New (Jungle Inferno) Burn Definitions
#define TF_BURNING_FLAME_LIFE_MIN_JI 4.0
#define TF_BURNING_FLAME_LIFE_MAX_JI 10.0
#define TF_BURNING_FLAME_LIFE_JI 	 7.5
#define TF_BURNING_DMG_JI			 4

// Bleeding
#define TF_BLEEDING_FREQUENCY		0.5f
#define TF_BLEEDING_DAMAGE			4

// Disguising
#define TF_TIME_TO_CHANGE_DISGUISE 0.5
#define TF_TIME_TO_DISGUISE 2.0
#define TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT 5.0


#define SHOW_DISGUISE_EFFECT 1
#define TF_DISGUISE_TARGET_INDEX_NONE	( MAX_PLAYERS + 1 )
#define TF_PLAYER_INDEX_NONE			( MAX_PLAYERS + 1 )

#define TF_MAX_PRESETS 4		// Needs to match g_InventoryLoadoutPresets

// Most of these conds aren't actually implemented but putting them here for compatibility.
enum
{
	TF_COND_AIMING = 0,		// Sniper aiming, Heavy minigun.
	TF_COND_ZOOMED,
	TF_COND_DISGUISING,
	TF_COND_DISGUISED,
	TF_COND_STEALTHED,
	TF_COND_INVULNERABLE,
	TF_COND_TELEPORTED,
	TF_COND_TAUNTING,
	TF_COND_INVULNERABLE_WEARINGOFF,
	TF_COND_STEALTHED_BLINK,
	TF_COND_SELECTED_TO_TELEPORT,
	TF_COND_CRITBOOSTED,
	TF_COND_TMPDAMAGEBONUS,
	TF_COND_FEIGN_DEATH,
	TF_COND_PHASE,
	TF_COND_STUNNED,
	TF_COND_OFFENSEBUFF,
	TF_COND_SHIELD_CHARGE,
	TF_COND_DEMO_BUFF,
	TF_COND_ENERGY_BUFF,
	TF_COND_RADIUSHEAL,
	TF_COND_HEALTH_BUFF,
	TF_COND_BURNING,
	TF_COND_HEALTH_OVERHEALED,
	TF_COND_URINE,
	TF_COND_BLEEDING,
	TF_COND_DEFENSEBUFF,
	TF_COND_MAD_MILK,
	TF_COND_MEGAHEAL,
	TF_COND_REGENONDAMAGEBUFF,
	TF_COND_MARKEDFORDEATH,
	TF_COND_NOHEALINGDAMAGEBUFF,
	TF_COND_SPEED_BOOST,
	TF_COND_CRITBOOSTED_PUMPKIN,
	TF_COND_CRITBOOSTED_USER_BUFF,
	TF_COND_CRITBOOSTED_DEMO_CHARGE,
	TF_COND_SODAPOPPER_HYPE,
	TF_COND_CRITBOOSTED_FIRST_BLOOD,
	TF_COND_CRITBOOSTED_BONUS_TIME,
	TF_COND_CRITBOOSTED_CTF_CAPTURE,
	TF_COND_CRITBOOSTED_ON_KILL,
	TF_COND_CANNOT_SWITCH_FROM_MELEE,
	TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK,
	TF_COND_REPROGRAMMED,
	TF_COND_CRITBOOSTED_RAGE_BUFF,
	TF_COND_DEFENSEBUFF_HIGH,
	TF_COND_SNIPERCHARGE_RAGE_BUFF,
	TF_COND_DISGUISE_WEARINGOFF,
	TF_COND_MARKEDFORDEATH_SELF,
	TF_COND_DISGUISED_AS_DISPENSER,
	TF_COND_SAPPED,
	TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGE,
	TF_COND_INVULNERABLE_USER_BUFF,
	TF_COND_HALLOWEEN_BOMB_HEAD,
	TF_COND_HALLOWEEN_THRILLER,
	TF_COND_RADIUSHEAL_ON_DAMAGE,
	TF_COND_CRITBOOSTED_CARD_EFFECT,
	TF_COND_INVULNERABLE_CARD_EFFECT,
	TF_COND_MEDIGUN_UBER_BULLET_RESIST,
	TF_COND_MEDIGUN_UBER_BLAST_RESIST,
	TF_COND_MEDIGUN_UBER_FIRE_RESIST,
	TF_COND_MEDIGUN_SMALL_BULLET_RESIST,
	TF_COND_MEDIGUN_SMALL_BLAST_RESIST,
	TF_COND_MEDIGUN_SMALL_FIRE_RESIST,
	TF_COND_STEALTHED_USER_BUFF,
	TF_COND_MEDIGUN_DEBUFF,
	TF_COND_STEALTHED_USER_BUFF_FADING,
	TF_COND_BULLET_IMMUNE,
	TF_COND_BLAST_IMMUNE,
	TF_COND_FIRE_IMMUNE,
	TF_COND_PREVENT_DEATH,
	TF_COND_MVM_BOT_STUN_RADIOWAVE,
	TF_COND_HALLOWEEN_SPEED_BOOST,
	TF_COND_HALLOWEEN_QUICK_HEAL,
	TF_COND_HALLOWEEN_GIANT,
	TF_COND_HALLOWEEN_TINY,
	TF_COND_HALLOWEEN_IN_HELL,
	TF_COND_HALLOWEEN_GHOST_MODE,
	TF_COND_MINICRITBOOSTED_ON_KILL,
	TF_COND_OBSCURED_SMOKE,
	TF_COND_PARACHUTE_ACTIVE,
	TF_COND_BLASTJUMPING,
	TF_COND_HALLOWEEN_KART,
	TF_COND_HALLOWEEN_KART_DASH,
	TF_COND_BALLOON_HEAD,
	TF_COND_MELEE_ONLY,
	TF_COND_SWIMMING_CURSE,
	TF_COND_FREEZE_INPUT,
	TF_COND_HALLOWEEN_KART_CAGE,
	TF_COND_DONOTUSE_0,
	TF_COND_RUNE_STRENGTH,
	TF_COND_RUNE_HASTE,
	TF_COND_RUNE_REGEN,
	TF_COND_RUNE_RESIST,
	TF_COND_RUNE_VAMPIRE,
	TF_COND_RUNE_REFLECT,
	TF_COND_RUNE_PRECISION,
	TF_COND_RUNE_AGILITY,
	TF_COND_GRAPPLINGHOOK,
	TF_COND_GRAPPLINGHOOK_SAFEFALL,
	TF_COND_GRAPPLINGHOOK_LATCHED,
	TF_COND_GRAPPLINGHOOK_BLEEDING,
	TF_COND_AFTERBURN_IMMUNE,
	TF_COND_RUNE_KNOCKOUT,
	TF_COND_RUNE_IMBALANCE,
	TF_COND_CRITBOOSTED_RUNE_TEMP,
	TF_COND_PASSTIME_INTERCEPTION,
	TF_COND_SWIMMING_NO_EFFECTS,
	TF_COND_PURGATORY,
	TF_COND_RUNE_KING,
	TF_COND_RUNE_PLAGUE,
	TF_COND_RUNE_SUPERNOVA,
	TF_COND_PLAGUE,
	TF_COND_KING_BUFFED,
	TF_COND_TEAM_GLOWS,
	TF_COND_KNOCKED_INTO_AIR,
	TF_COND_COMPETITIVE_WINNER,
	TF_COND_COMPETITIVE_LOSER,
	TF_COND_HEALING_DEBUFF,
	TF_COND_PASSTIME_PENALTY_DEBUFF,
	TF_COND_GRAPPLED_TO_PLAYER,
	TF_COND_GRAPPLED_BY_PLAYER,
	TF_COND_PARACHUTE_DEPLOYED,
	TF_COND_GAS,
	TF_COND_BURNING_PYRO,
	TF_COND_ROCKETPACK,
	TF_COND_LOST_FOOTING,
	TF_COND_AIR_CURRENT,

	// TF2V conds
	TF_COND_NO_MOVE,
	TF_COND_DISGUISE_HEALTH_OVERHEALED,
	TF_COND_LUNCHBOX_HEALTH_BUFF,
	TF_COND_SMOKE_BOMB,
	TF_COND_REDUCED_MOVE,
	TF_COND_SPEED_BOOST_FEIGN,
	TF_COND_BLINK_IMMUNE,
	TF_COND_BERSERK,
	TF_COND_CRITBOOSTED_ACTIVEWEAPON,
	TF_COND_MINICRITBOOSTED_RAGE_BUFF,
	TF_COND_MINICRITBOOSTED_ACTIVEWEAPON,

	TF_COND_LAST
};

extern int condition_to_attribute_translation[];

int ConditionExpiresFast( int nCond );

//-----------------------------------------------------------------------------
// Mediguns.
//-----------------------------------------------------------------------------
enum
{
	TF_MEDIGUN_STOCK = 0,
	TF_MEDIGUN_KRITZKRIEG,
	TF_MEDIGUN_QUICKFIX,
	TF_MEDIGUN_VACCINATOR,
	TF_MEDIGUN_OVERHEALER,
	TF_MEDIGUN_COUNT
};

enum medigun_charge_types
{
	TF_CHARGE_NONE = -1,
	TF_CHARGE_INVULNERABLE = 0,
	TF_CHARGE_CRITBOOSTED,
	TF_CHARGE_MEGAHEAL,
	TF_CHARGE_BULLET_RESIST,
	TF_CHARGE_BLAST_RESIST,
	TF_CHARGE_FIRE_RESIST,
	TF_CHARGE_COUNT
};

typedef struct
{
	int condition_enable;
	int condition_disable;
	const char *sound_enable;
	const char *sound_disable;
} MedigunEffects_t;

extern MedigunEffects_t g_MedigunEffects[];

//-----------------------------------------------------------------------------
// TF Player State.
//-----------------------------------------------------------------------------
enum 
{
	TF_STATE_ACTIVE = 0,		// Happily running around in the game.
	TF_STATE_WELCOME,			// First entering the server (shows level intro screen).
	TF_STATE_OBSERVER,			// Game observer mode.
	TF_STATE_DYING,				// Player is dying.
	TF_STATE_COUNT
};

//-----------------------------------------------------------------------------
// TF FlagInfo State.
//-----------------------------------------------------------------------------
#define TF_FLAGINFO_NONE		0
#define TF_FLAGINFO_STOLEN		(1<<0)
#define TF_FLAGINFO_DROPPED		(1<<1)

enum {
	TF_FLAGEVENT_PICKUP = 1,
	TF_FLAGEVENT_CAPTURE,
	TF_FLAGEVENT_DEFEND,
	TF_FLAGEVENT_DROPPED
};

//-----------------------------------------------------------------------------
// Class data
//-----------------------------------------------------------------------------
#define TF_MEDIC_REGEN_TIME			1.0		// Number of seconds between each regen.
#define TF_MEDIC_REGEN_AMOUNT		1 		// Amount of health regenerated each regen.

//-----------------------------------------------------------------------------
// Assist-damage constants
//-----------------------------------------------------------------------------
#define TF_TIME_ASSIST_KILL				3.0f	// Time window for a recent damager to get credit for an assist for a kill
#define TF_TIME_ENV_DEATH_KILL_CREDIT	5.0f	// Time window for a recent damager to get credit for an environmental kill
#define TF_TIME_SUICIDE_KILL_CREDIT		10.0f	// Time window for a recent damager to get credit for a kill if target suicides

//-----------------------------------------------------------------------------
// Domination/nemesis constants
//-----------------------------------------------------------------------------
#define TF_KILLS_DOMINATION				4		// # of unanswered kills to dominate another player

//-----------------------------------------------------------------------------
// TF Hints
//-----------------------------------------------------------------------------
enum
{
	HINT_FRIEND_SEEN = 0,				// #Hint_spotted_a_friend
	HINT_ENEMY_SEEN,					// #Hint_spotted_an_enemy
	HINT_ENEMY_KILLED,					// #Hint_killing_enemies_is_good
	HINT_AMMO_EXHAUSTED,				// #Hint_out_of_ammo
	HINT_TURN_OFF_HINTS,				// #Hint_turn_off_hints
	HINT_PICKUP_AMMO,					// #Hint_pickup_ammo
	HINT_CANNOT_TELE_WITH_FLAG,			// #Hint_Cannot_Teleport_With_Flag
	HINT_CANNOT_CLOAK_WITH_FLAG,		// #Hint_Cannot_Cloak_With_Flag
	HINT_CANNOT_DISGUISE_WITH_FLAG,		// #Hint_Cannot_Disguise_With_Flag
	HINT_CANNOT_ATTACK_WHILE_CLOAKED,	// #Hint_Cannot_Attack_While_Cloaked
	HINT_CLASSMENU,						// #Hint_ClassMenu

	// Grenades
	HINT_GREN_CALTROPS,					// #Hint_gren_caltrops
	HINT_GREN_CONCUSSION,				// #Hint_gren_concussion
	HINT_GREN_EMP,						// #Hint_gren_emp
	HINT_GREN_GAS,						// #Hint_gren_gas
	HINT_GREN_MIRV,						// #Hint_gren_mirv
	HINT_GREN_NAIL,						// #Hint_gren_nail
	HINT_GREN_NAPALM,					// #Hint_gren_napalm
	HINT_GREN_NORMAL,					// #Hint_gren_normal

	// Weapon alt-fires
	HINT_ALTFIRE_SNIPERRIFLE,			// #Hint_altfire_sniperrifle
	HINT_ALTFIRE_FLAMETHROWER,			// #Hint_altfire_flamethrower
	HINT_ALTFIRE_GRENADELAUNCHER,		// #Hint_altfire_grenadelauncher
	HINT_ALTFIRE_PIPEBOMBLAUNCHER,		// #Hint_altfire_pipebomblauncher
	HINT_ALTFIRE_ROTATE_BUILDING,		// #Hint_altfire_rotate_building

	// Class specific
	// Soldier
	HINT_SOLDIER_RPG_RELOAD,			// #Hint_Soldier_rpg_reload

	// Engineer
	HINT_ENGINEER_USE_WRENCH_ONOWN,		// "#Hint_Engineer_use_wrench_onown",
	HINT_ENGINEER_USE_WRENCH_ONOTHER,	// "#Hint_Engineer_use_wrench_onother",
	HINT_ENGINEER_USE_WRENCH_FRIEND,	// "#Hint_Engineer_use_wrench_onfriend",
	HINT_ENGINEER_BUILD_SENTRYGUN,		// "#Hint_Engineer_build_sentrygun"
	HINT_ENGINEER_BUILD_DISPENSER,		// "#Hint_Engineer_build_dispenser"
	HINT_ENGINEER_BUILD_TELEPORTERS,	// "#Hint_Engineer_build_teleporters"
	HINT_ENGINEER_PICKUP_METAL,			// "#Hint_Engineer_pickup_metal"
	HINT_ENGINEER_REPAIR_OBJECT,		// "#Hint_Engineer_repair_object"
	HINT_ENGINEER_METAL_TO_UPGRADE,		// "#Hint_Engineer_metal_to_upgrade"
	HINT_ENGINEER_UPGRADE_SENTRYGUN,	// "#Hint_Engineer_upgrade_sentrygun"

	HINT_OBJECT_HAS_SAPPER,				// "#Hint_object_has_sapper"

	HINT_OBJECT_YOUR_OBJECT_SAPPED,		// "#Hint_object_your_object_sapped"
	HINT_OBJECT_ENEMY_USING_DISPENSER,	// "#Hint_enemy_using_dispenser"
	HINT_OBJECT_ENEMY_USING_TP_ENTRANCE,	// "#Hint_enemy_using_tp_entrance"
	HINT_OBJECT_ENEMY_USING_TP_EXIT,	// "#Hint_enemy_using_tp_exit"

	NUM_HINTS
};
extern const char *g_pszHintMessages[];



/*======================*/
//      Menu stuff      //
/*======================*/

#define MENU_DEFAULT				1
#define MENU_TEAM 					2
#define MENU_CLASS 					3
#define MENU_MAPBRIEFING			4
#define MENU_INTRO 					5
#define MENU_CLASSHELP				6
#define MENU_CLASSHELP2 			7
#define MENU_REPEATHELP 			8

#define MENU_SPECHELP				9


#define MENU_SPY					12
#define MENU_SPY_SKIN				13
#define MENU_SPY_COLOR				14
#define MENU_ENGINEER				15
#define MENU_ENGINEER_FIX_DISPENSER	16
#define MENU_ENGINEER_FIX_SENTRYGUN	17
#define MENU_ENGINEER_FIX_MORTAR	18
#define MENU_DISPENSER				19
#define MENU_CLASS_CHANGE			20
#define MENU_TEAM_CHANGE			21

#define MENU_REFRESH_RATE 			25

#define MENU_VOICETWEAK				50

// Additional classes
// NOTE: adding them onto the Class_T's in baseentity.h is cheesy, but so is
// having an #ifdef for each mod in baseentity.h.
#define CLASS_TFGOAL				((Class_T)NUM_AI_CLASSES)
#define CLASS_TFGOAL_TIMER			((Class_T)(NUM_AI_CLASSES+1))
#define CLASS_TFGOAL_ITEM			((Class_T)(NUM_AI_CLASSES+2))
#define CLASS_TFSPAWN				((Class_T)(NUM_AI_CLASSES+3))
#define CLASS_MACHINE				((Class_T)(NUM_AI_CLASSES+4))

// TeamFortress State Flags
#define TFSTATE_GRENPRIMED		0x000001 // Whether the player has a primed grenade
#define TFSTATE_RELOADING		0x000002 // Whether the player is reloading
#define TFSTATE_ALTKILL			0x000004 // #TRUE if killed with a weapon not in self.weapon: NOT USED ANYMORE
#define TFSTATE_RANDOMPC		0x000008 // Whether Playerclass is random, new one each respawn
#define TFSTATE_INFECTED		0x000010 // set when player is infected by the bioweapon
#define TFSTATE_INVINCIBLE		0x000020 // Player has permanent Invincibility (Usually by GoalItem)
#define TFSTATE_INVISIBLE		0x000040 // Player has permanent Invisibility (Usually by GoalItem)
#define TFSTATE_QUAD			0x000080 // Player has permanent Quad Damage (Usually by GoalItem)
#define TFSTATE_RADSUIT			0x000100 // Player has permanent Radsuit (Usually by GoalItem)
#define TFSTATE_BURNING			0x000200 // Is on fire
#define TFSTATE_GRENTHROWING	0x000400  // is throwing a grenade
#define TFSTATE_AIMING			0x000800  // is using the laser sight
#define TFSTATE_ZOOMOFF			0x001000  // doesn't want the FOV changed when zooming
#define TFSTATE_RESPAWN_READY	0x002000  // is waiting for respawn, and has pressed fire
#define TFSTATE_HALLUCINATING	0x004000  // set when player is hallucinating
#define TFSTATE_TRANQUILISED	0x008000  // set when player is tranquilised
#define TFSTATE_CANT_MOVE		0x010000  // player isn't allowed to move
#define TFSTATE_RESET_FLAMETIME 0x020000 // set when the player has to have his flames increased in health
#define TFSTATE_HIGHEST_VALUE	TFSTATE_RESET_FLAMETIME

// items
#define IT_SHOTGUN				(1<<0)
#define IT_SUPER_SHOTGUN		(1<<1) 
#define IT_NAILGUN				(1<<2) 
#define IT_SUPER_NAILGUN		(1<<3) 
#define IT_GRENADE_LAUNCHER		(1<<4) 
#define IT_ROCKET_LAUNCHER		(1<<5) 
#define IT_LIGHTNING			(1<<6) 
#define IT_EXTRA_WEAPON			(1<<7) 

#define IT_SHELLS				(1<<8) 
#define IT_BULLETS				(1<<9) 
#define IT_ROCKETS				(1<<10) 
#define IT_CELLS				(1<<11) 
#define IT_AXE					(1<<12) 

#define IT_ARMOR1				(1<<13) 
#define IT_ARMOR2				(1<<14) 
#define IT_ARMOR3				(1<<15) 
#define IT_SUPERHEALTH			(1<<16) 

#define IT_KEY1					(1<<17) 
#define IT_KEY2					(1<<18) 

#define IT_INVISIBILITY			(1<<19) 
#define IT_INVULNERABILITY		(1<<20) 
#define IT_SUIT					(1<<21)
#define IT_QUAD					(1<<22) 
#define IT_HOOK					(1<<23)

#define IT_KEY3					(1<<24)	// Stomp invisibility
#define IT_KEY4					(1<<25)	// Stomp invulnerability
#define IT_LAST_ITEM			IT_KEY4

/*==================================================*/
/* New Weapon Related Defines		                */
/*==================================================*/

// Medikit
#define WEAP_MEDIKIT_OVERHEAL 50 // Amount of superhealth over max_health the medikit will dispense
#define WEAP_MEDIKIT_HEAL	200  // Amount medikit heals per hit

//--------------
// TF Specific damage flags
//--------------
//#define DMG_UNUSED					(DMG_LASTGENERICFLAG<<2)
// We can't add anymore dmg flags, because we'd be over the 32 bit limit.
// So lets re-use some of the old dmg flags in TF
#define DMG_USE_HITLOCATIONS	(DMG_AIRBOAT)
#define DMG_HALF_FALLOFF		(DMG_RADIATION)
#define DMG_CRITICAL			(DMG_ACID)
#define DMG_MINICRITICAL		(DMG_PHYSGUN)
#define DMG_RADIUS_MAX			(DMG_ENERGYBEAM)
#define DMG_IGNITE				(DMG_PLASMA)
#define DMG_USEDISTANCEMOD		(DMG_SLOWBURN)		// NEED TO REMOVE CALTROPS
#define DMG_NOCLOSEDISTANCEMOD	(DMG_POISON)
#define DMG_MELEE				(DMG_BLAST_SURFACE) // Identifier for melee attributes

#define TF_DMG_SENTINEL_VALUE	0xFFFFFFFF

// This can only ever be used on a TakeHealth call, since it re-uses a dmg flag that means something else
#define DMG_IGNORE_MAXHEALTH	(DMG_BULLET)

// Special Damage types
enum
{
	TF_DMG_CUSTOM_NONE, // TODO: Remove this at some point

	TF_DMG_CUSTOM_HEADSHOT,
	TF_DMG_CUSTOM_BACKSTAB,
	TF_DMG_CUSTOM_BURNING,
	TF_DMG_WRENCH_FIX,
	TF_DMG_CUSTOM_MINIGUN,
	TF_DMG_CUSTOM_SUICIDE,
	TF_DMG_CUSTOM_TAUNTATK_HADOUKEN,
	TF_DMG_CUSTOM_BURNING_FLARE,
	TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON,
	TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM,
	TF_DMG_CUSTOM_PENETRATE_MY_TEAM,
	TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS,
	TF_DMG_CUSTOM_TAUNTATK_FENCING,
	TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE,
	TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB,
	TF_DMG_CUSTOM_TELEFRAG,
	TF_DMG_CUSTOM_BURNING_ARROW,
	TF_DMG_CUSTOM_FLYINGBURN,
	TF_DMG_CUSTOM_PUMPKIN_BOMB,
	TF_DMG_CUSTOM_DECAPITATION,
	TF_DMG_CUSTOM_TAUNTATK_GRENADE,
	TF_DMG_CUSTOM_BASEBALL,
	TF_DMG_CUSTOM_CHARGE_IMPACT,
	TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING,
	TF_DMG_CUSTOM_AIR_STICKY_BURST,
	TF_DMG_CUSTOM_DEFENSIVE_STICKY,
	TF_DMG_CUSTOM_PICKAXE,
	TF_DMG_CUSTOM_ROCKET_DIRECTHIT,
	TF_DMG_CUSTOM_TAUNTATK_UBERSLICE,
	TF_DMG_CUSTOM_PLAYER_SENTRY,
	TF_DMG_CUSTOM_STANDARD_STICKY,
	TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT,
	TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH,
	TF_DMG_CUSTOM_BLEEDING,
	TF_DMG_CUSTOM_GOLD_WRENCH,
	TF_DMG_CUSTOM_CARRIED_BUILDING,
	TF_DMG_CUSTOM_COMBO_PUNCH,
	TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL,
	TF_DMG_CUSTOM_FISH_KILL,
	TF_DMG_CUSTOM_TRIGGER_HURT,
	TF_DMG_CUSTOM_DECAPITATION_BOSS,
	TF_DMG_CUSTOM_STICKBOMB_EXPLOSION,
	TF_DMG_CUSTOM_AEGIS_ROUND,
	TF_DMG_CUSTOM_FLARE_EXPLOSION,
	TF_DMG_CUSTOM_BOOTS_STOMP,
	TF_DMG_CUSTOM_PLASMA,
	TF_DMG_CUSTOM_PLASMA_CHARGED,
	TF_DMG_CUSTOM_PLASMA_GIB,
	TF_DMG_CUSTOM_PRACTICE_STICKY,
	TF_DMG_CUSTOM_EYEBALL_ROCKET,
	TF_DMG_CUSTOM_HEADSHOT_DECAPITATION,
	TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON,
	TF_DMG_CUSTOM_FLARE_PELLET,
	TF_DMG_CUSTOM_CLEAVER,
	TF_DMG_CUSTOM_CLEAVER_CRIT,
	TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH,
	TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB,
	TF_DMG_CUSTOM_MERASMUS_GRENADE,
	TF_DMG_CUSTOM_MERASMUS_ZAP,
	TF_DMG_CUSTOM_MERASMUS_DECAPITATION,
	TF_DMG_CUSTOM_CANNONBALL_PUSH,
	TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF,
	TF_DMG_CUSTOM_THROWABLE,
	TF_DMG_CUSTOM_THROWABLE_KILL,
	TF_DMG_CUSTOM_SPELL_TELEPORT,
	TF_DMG_CUSTOM_SPELL_SKELETON,
	TF_DMG_CUSTOM_SPELL_MIRV,
	TF_DMG_CUSTOM_SPELL_METEOR,
	TF_DMG_CUSTOM_SPELL_LIGHTNING,
	TF_DMG_CUSTOM_SPELL_FIREBALL,
	TF_DMG_CUSTOM_SPELL_MONOCULUS,
	TF_DMG_CUSTOM_SPELL_BLASTJUMP,
	TF_DMG_CUSTOM_SPELL_BATS,
	TF_DMG_CUSTOM_SPELL_TINY,
	TF_DMG_CUSTOM_KART,
	TF_DMG_CUSTOM_GIANT_HAMMER,
	TF_DMG_CUSTOM_RUNE_REFLECT,
	TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE,
	TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNIN,
	TF_DMG_CUSTOM_SLAP_KILL,
	TF_DMG_CUSTOM_CROC,
	TF_DMG_CUSTOM_TAUNTATK_GASBLAST,
	TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED,
};

// Crit types
enum ECritType
{
	kCritType_None,
	kCritType_MiniCrit,
	kCritType_Crit
};

#define TF_JUMP_ROCKET	( 1 << 0 )
#define TF_JUMP_STICKY	( 1 << 1 )
#define TF_JUMP_OTHER	( 1 << 2 )

enum
{
	TFCOLLISION_GROUP_GRENADES = LAST_SHARED_COLLISION_GROUP,
	TFCOLLISION_GROUP_OBJECT,
	TFCOLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT,
	TFCOLLISION_GROUP_COMBATOBJECT,
	TFCOLLISION_GROUP_ROCKETS,		// Solid to players, but not player movement. ensures touch calls are originating from rocket
	TFCOLLISION_GROUP_RESPAWNROOMS,
	TFCOLLISION_GROUP_PUMPKIN_BOMB, // Bombs
	TFCOLLISION_GROUP_ARROWS, // Arrows
	TFCOLLISION_GROUP_NONE, // Collides with nothing
};

//-----------------
// TF Objects Info
//-----------------

#define SENTRYGUN_UPGRADE_COST			130
#define SENTRYGUN_UPGRADE_METAL			200
#define SENTRYGUN_EYE_OFFSET_LEVEL_1	Vector( 0, 0, 32 )
#define SENTRYGUN_EYE_OFFSET_LEVEL_2	Vector( 0, 0, 40 )
#define SENTRYGUN_EYE_OFFSET_LEVEL_3	Vector( 0, 0, 46 )
#define SENTRYGUN_MAX_SHELLS_1			150
#define SENTRYGUN_MAX_SHELLS_2			200
#define SENTRYGUN_MAX_SHELLS_3			200
#define SENTRYGUN_MAX_ROCKETS			20
#define SENTRYGUN_BASE_RANGE			1100.0f

// Dispenser's maximum carrying capability
#define DISPENSER_MAX_METAL_AMMO		400
#define	MAX_DISPENSER_HEALING_TARGETS	32

//--------------------------------------------------------------------------
// OBJECTS
//--------------------------------------------------------------------------
enum
{
	OBJ_DISPENSER=0,
	OBJ_TELEPORTER,
	OBJ_SENTRYGUN,

	// Attachment Objects
	OBJ_ATTACHMENT_SAPPER,

	// If you add a new object, you need to add it to the g_ObjectInfos array 
	// in tf_shareddefs.cpp, and add it's data to the scripts/object.txt

	OBJ_LAST,
};

// Warning levels for buildings in the building hud, in priority order
typedef enum
{
	BUILDING_HUD_ALERT_NONE = 0,
	BUILDING_HUD_ALERT_LOW_AMMO,
	BUILDING_HUD_ALERT_LOW_HEALTH,
	BUILDING_HUD_ALERT_VERY_LOW_AMMO,
	BUILDING_HUD_ALERT_VERY_LOW_HEALTH,
	BUILDING_HUD_ALERT_SAPPER,	

	MAX_BUILDING_HUD_ALERT_LEVEL
} BuildingHudAlert_t;

typedef enum
{
	BUILDING_DAMAGE_LEVEL_NONE = 0,		// 100%
	BUILDING_DAMAGE_LEVEL_LIGHT,		// 75% - 99%
	BUILDING_DAMAGE_LEVEL_MEDIUM,		// 50% - 76%
	BUILDING_DAMAGE_LEVEL_HEAVY,		// 25% - 49%	
	BUILDING_DAMAGE_LEVEL_CRITICAL,		// 0% - 24%

	MAX_BUILDING_DAMAGE_LEVEL
} BuildingDamageLevel_t;

//--------------
// Scoring
//--------------

#define TF_SCORE_KILL							1
#define TF_SCORE_DEATH							0
#define TF_SCORE_CAPTURE						2
#define TF_SCORE_DEFEND							1
#define TF_SCORE_DESTROY_BUILDING				1
#define TF_SCORE_HEADSHOT_PER_POINT				2
#define TF_SCORE_BACKSTAB						1
#define TF_SCORE_INVULN							1
#define TF_SCORE_REVENGE						1
#define TF_SCORE_KILL_ASSISTS_PER_POINT			2
#define TF_SCORE_TELEPORTS_PER_POINT			2	
#define TF_SCORE_HEAL_HEALTHUNITS_PER_POINT		600
#define TF_SCORE_DAMAGE_PER_POINT				600
#define TF_SCORE_BONUS_PER_POINT				1

//-------------------------
// Shared Teleporter State
//-------------------------
enum
{
	TELEPORTER_STATE_BUILDING = 0,				// Building, not active yet
	TELEPORTER_STATE_IDLE,						// Does not have a matching teleporter yet
	TELEPORTER_STATE_READY,						// Found match, charged and ready
	TELEPORTER_STATE_SENDING,					// Teleporting a player away
	TELEPORTER_STATE_RECEIVING,					
	TELEPORTER_STATE_RECEIVING_RELEASE,
	TELEPORTER_STATE_RECHARGING,				// Waiting for recharge
	TELEPORTER_STATE_UPGRADING
};

#define OBJECT_MODE_NONE			0
#define TELEPORTER_TYPE_ENTRANCE	0
#define TELEPORTER_TYPE_EXIT		1

#define TELEPORTER_RECHARGE_TIME				10		// seconds to recharge

extern float g_flTeleporterRechargeTimes[];
extern float g_flDispenserAmmoRates[];
extern float g_flDispenserCloakRates[];
extern float g_flDispenserHealRates[];

//-------------------------
// Shared Sentry State
//-------------------------
enum
{
	SENTRY_STATE_INACTIVE = 0,
	SENTRY_STATE_SEARCHING,
	SENTRY_STATE_ATTACKING,
	SENTRY_STATE_UPGRADING,
	SENTRY_STATE_WRANGLED,
	SENTRY_STATE_WRANGLED_RECOVERY,
	SENTRY_STATE_SAPPER_RECOVERY,

	SENTRY_NUM_STATES,
};

//--------------------------------------------------------------------------
// OBJECT FLAGS
//--------------------------------------------------------------------------
enum
{
	OF_ALLOW_REPEAT_PLACEMENT				= 0x01,
	OF_MUST_BE_BUILT_ON_ATTACHMENT			= 0x02,
	OF_IS_CART_OBJECT						= 0x04, //I'm not sure what the exact name is, but live tf2 uses it for the payload bomb dispenser object

	OF_BIT_COUNT	= 4
};

//--------------------------------------------------------------------------
// Builder "weapon" states
//--------------------------------------------------------------------------
enum 
{
	BS_IDLE = 0,
	BS_SELECTING,
	BS_PLACING,
	BS_PLACING_INVALID
};


//--------------------------------------------------------------------------
// Builder object id...
//--------------------------------------------------------------------------
enum
{
	BUILDER_OBJECT_BITS = 8,
	BUILDER_INVALID_OBJECT = ((1 << BUILDER_OBJECT_BITS) - 1)
};

// Analyzer state
enum
{
	AS_INACTIVE = 0,
	AS_SUBVERTING,
	AS_ANALYZING
};

// Max number of objects a team can have
#define MAX_OBJECTS_PER_PLAYER	4
//#define MAX_OBJECTS_PER_TEAM	128

// sanity check that commands send via user command are somewhat valid
#define MAX_OBJECT_SCREEN_INPUT_DISTANCE	100

//--------------------------------------------------------------------------
// BUILDING
//--------------------------------------------------------------------------
// Build checks will return one of these for a player
enum
{
	CB_CAN_BUILD,			// Player is allowed to build this object
	CB_CANNOT_BUILD,		// Player is not allowed to build this object
	CB_LIMIT_REACHED,		// Player has reached the limit of the number of these objects allowed
	CB_NEED_RESOURCES,		// Player doesn't have enough resources to build this object
	CB_NEED_ADRENALIN,		// Commando doesn't have enough adrenalin to build a rally flag
	CB_UNKNOWN_OBJECT,		// Error message, tried to build unknown object
};

// Build animation events
#define TF_OBJ_ENABLEBODYGROUP			6000
#define TF_OBJ_DISABLEBODYGROUP			6001
#define TF_OBJ_ENABLEALLBODYGROUPS		6002
#define TF_OBJ_DISABLEALLBODYGROUPS		6003
#define TF_OBJ_PLAYBUILDSOUND			6004

#define TF_AE_CIGARETTE_THROW			7000

#define OBJECT_COST_MULTIPLIER_PER_OBJECT			3
#define OBJECT_UPGRADE_COST_MULTIPLIER_PER_LEVEL	3

//--------------------------------------------------------------------------
// Powerups
//--------------------------------------------------------------------------
enum
{
	POWERUP_BOOST,		// Medic, buff station
	POWERUP_EMP,		// Technician
	POWERUP_RUSH,		// Rally flag
	POWERUP_POWER,		// Object power
	MAX_POWERUPS
};

//--------------------------------------------------------------------------
// Stun
//--------------------------------------------------------------------------
#define TF_STUNFLAG_SLOWDOWN			(1<<0) // activates slowdown modifier
#define TF_STUNFLAG_BONKSTUCK			(1<<1) // bonk sound, stuck
#define TF_STUNFLAG_LIMITMOVEMENT		(1<<2) // disable forward/backward movement
#define TF_STUNFLAG_CHEERSOUND			(1<<3) // cheering sound
#define TF_STUNFLAG_NOSOUNDOREFFECT		(1<<4) // no sound or particle
#define TF_STUNFLAG_THIRDPERSON			(1<<5) // panic animation
#define TF_STUNFLAG_GHOSTEFFECT			(1<<6) // ghost particles
#define TF_STUNFLAG_BONKEFFECT			(1<<7) // sandman particles
#define TF_STUNFLAG_RESISTDAMAGE		(1<<8) // damage resist modifier
	
enum
{
	TF_STUNFLAGS_LOSERSTATE		= TF_STUNFLAG_THIRDPERSON | TF_STUNFLAG_SLOWDOWN | TF_STUNFLAG_NOSOUNDOREFFECT, // Currently unused
	TF_STUNFLAGS_GHOSTSCARE		= TF_STUNFLAG_THIRDPERSON | TF_STUNFLAG_GHOSTEFFECT, // Ghost stun
	TF_STUNFLAGS_SMALLBONK		= TF_STUNFLAG_THIRDPERSON | TF_STUNFLAG_SLOWDOWN | TF_STUNFLAG_BONKEFFECT, // Half stun
	TF_STUNFLAGS_NORMALBONK		= TF_STUNFLAG_BONKSTUCK, // Full stun
	TF_STUNFLAGS_BIGBONK		= TF_STUNFLAG_CHEERSOUND | TF_STUNFLAG_BONKSTUCK | TF_STUNFLAG_RESISTDAMAGE | TF_STUNFLAG_BONKEFFECT, // Moonshot
	TF_STUNFLAGS_COUNT // This doesn't really work with flags
};

//--------------------------------------------------------------------------
// Holiday
//--------------------------------------------------------------------------
enum EHoliday
{
	kHoliday_None,
	kHoliday_TF2Birthday,
	kHoliday_Halloween,
	kHoliday_Christmas,
	kHoliday_CommunityUpdate,
	kHoliday_EOTL,
	kHoliday_ValentinesDay,
	kHoliday_MeetThePyro,
	kHoliday_FullMoon,
	kHoliday_HalloweenOrFullMoon,
	kHoliday_HalloweenOrFullMoonOrValentines,
	kHoliday_AprilFools,
	kHoliday_BreadUpdate,
	kHoliday_SoldierMemorial,

	kHolidayCount,
};

//--------------------------------------------------------------------------
// Hype
//--------------------------------------------------------------------------
#define TF_SCATTERGUN_HYPE_COUNT 350 // Damage to give before filling the Soda Popper's HYPE meter.
#define TF_SCATTERGUN_BOOST_COUNT 100 // Damage to give before filling the Baby Face Blaster's BOOST meter.

//--------------------------------------------------------------------------
// Rage
//--------------------------------------------------------------------------
enum
{
	TF_BUFF_OFFENSE = 1,
	TF_BUFF_DEFENSE,
	TF_BUFF_REGENONDAMAGE,
	TF_BUFF_COUNT
};

#define TF_BUFF_OFFENSE_COUNT 600				// Damage to give before filling the Buff Banner rage.
#define TF_BUFF_DEFENSE_COUNT 175				// Damage to take before filling the Battalion's Backup rage.
#define TF_BUFF_REGENONDAMAGE_OFFENSE_COUNT 600 // Damage to give before filling the Concheror rage.
#define TF_BUFF_REGENONDAMAGE_DEFENSE_COUNT 210 // Damage to take before filling the Concheror rage.

#define TF_BUFF_REGENONDAMAGE_OFFENSE_COUNT_NEW 480 // Damage to give before filling the Concheror rage, using the modern value.

#define	MAX_CABLE_CONNECTIONS 4

bool IsObjectAnUpgrade( int iObjectType );
bool IsObjectAVehicle( int iObjectType );
bool IsObjectADefensiveBuilding( int iObjectType );

class CHudTexture;

#define OBJECT_MAX_GIB_MODELS	9
#define TEMP_OBJECT_LIFETIME	10.0f

class CObjectInfo
{
public:
	CObjectInfo( char *pObjectName );
	CObjectInfo( const CObjectInfo& obj ) {}
	~CObjectInfo();

	// This is initialized by the code and matched with a section in objects.txt
	char	*m_pObjectName;

	// This stuff all comes from objects.txt
	char	*m_pClassName;					// Code classname (in LINK_ENTITY_TO_CLASS).
	char	*m_pStatusName;					// Shows up when crosshairs are on the object.
	float	m_flBuildTime;
	int		m_nMaxObjects;					// Maximum number of objects per player
	int		m_Cost;							// Base object resource cost
	float	m_CostMultiplierPerInstance;	// Cost multiplier
	int		m_UpgradeCost;					// Base object resource cost for upgrading
	float	m_flUpgradeDuration;
	int		m_MaxUpgradeLevel;				// Max object upgrade level
	char	*m_pBuilderWeaponName;			// Names shown for each object onscreen when using the builder weapon
	char	*m_pBuilderPlacementString;		// String shown to player during placement of this object
	int		m_SelectionSlot;				// Weapon selection slots for objects
	int		m_SelectionPosition;			// Weapon selection positions for objects
	bool	m_bSolidToPlayerMovement;
	bool	m_bUseItemInfo;
	char    *m_pViewModel;					// View model to show in builder weapon for this object
	char    *m_pPlayerModel;				// World model to show attached to the player
	int		m_iDisplayPriority;				// Priority for ordering in the hud display ( higher is closer to top )
	bool	m_bVisibleInWeaponSelection;	// should show up and be selectable via the weapon selection?
	char	*m_pExplodeSound;				// gamesound to play when object explodes
	char	*m_pExplosionParticleEffect;	// particle effect to play when object explodes
	bool	m_bAutoSwitchTo;				// should we let players switch back to the builder weapon representing this?
	char	*m_pUpgradeSound;				// gamesound to play when upgrading
	int		m_BuildCount;					// ???
	bool	m_bRequiresOwnBuilder;			// ???

	CUtlVector< const char * > m_AltModes;

	// HUD weapon selection menu icon ( from hud_textures.txt )
	char	*m_pIconActive;
	char	*m_pIconInactive;
	char	*m_pIconMenu;

	// HUD building status icon
	char	*m_pHudStatusIcon;

	// gibs
	int		m_iMetalToDropInGibs;
};

// Loads the objects.txt script.
class IBaseFileSystem;
void LoadObjectInfos( IBaseFileSystem *pFileSystem );

// Get a CObjectInfo from a TFOBJ_ define.
const CObjectInfo* GetObjectInfo( int iObject );

// Object utility funcs
bool	ClassCanBuild( int iClass, int iObjectType );
int		CalculateObjectCost( int iObjectType, bool bMini = false /*, int iNumberOfObjects, int iTeam, bool bLast = false*/ );
int		CalculateObjectUpgrade( int iObjectType, int iObjectLevel );

// Shell ejections
enum
{
	EJECTBRASS_PISTOL,
	EJECTBRASS_MINIGUN,
};

// Win panel styles
enum
{
	WINPANEL_BASIC = 0,
};

#define TF_DEATH_ANIMATION_TIME			2.0

typedef enum
{
	TAUNT_NORMAL,
	TAUNT_LAUGH,
	TAUNT_EUREKA,
} taunts_t;

// Taunt attack types
enum
{
	TAUNTATK_NONE,
 	TAUNTATK_PYRO_HADOUKEN,
 	TAUNTATK_HEAVY_EAT, // 1st Nom
 	TAUNTATK_HEAVY_RADIAL_BUFF, // 2nd Nom gives hp
 	TAUNTATK_SCOUT_DRINK,
 	TAUNTATK_HEAVY_HIGH_NOON, // POW!
 	TAUNTATK_SCOUT_GRAND_SLAM, // Sandman
 	TAUNTATK_MEDIC_INHALE, // Oktoberfest
 	TAUNTATK_SPY_FENCING_SLASH_A, // Just lay
 	TAUNTATK_SPY_FENCING_SLASH_B, // Your weapon down
 	TAUNTATK_SPY_FENCING_STAB, // And walk away.
 	TAUNTATK_RPS_KILL,
 	TAUNTATK_SNIPER_ARROW_STAB_IMPALE, // Stab stab
 	TAUNTATK_SNIPER_ARROW_STAB_KILL, // STAB
 	TAUNTATK_SOLDIER_GRENADE_KILL, // Equalizer
 	TAUNTATK_DEMOMAN_BARBARIAN_SWING,
 	TAUNTATK_MEDIC_UBERSLICE_IMPALE, // I'm going to saw
 	TAUNTATK_MEDIC_UBERSLICE_KILL, // THROUGH YOUR BONES!
 	TAUNTATK_FLIP_LAND_PARTICLE,
 	TAUNTATK_RPS_PARTICLE,
 	TAUNTATK_HIGHFIVE_PARTICLE,
 	TAUNTATK_ENGINEER_GUITAR_SMASH,
 	TAUNTATK_ENGINEER_ARM_IMPALE, // Grinder Start
 	TAUNTATK_ENGINEER_ARM_KILL, // Grinder Kill
 	TAUNTATK_ENGINEER_ARM_BLEND, // Grinder Loop
 	TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN,
 	TAUNTATK_SHOW_ITEM,
 	TAUNTATK_MEDIC_RELEASE_DOVES,
 	TAUNTATK_PYRO_ARMAGEDDON,
 	TAUNTATK_PYRO_SCORCHSHOT,
 	TAUNTATK_ALLCLASS_GUITAR_RIFF,
 	TAUNTATK_MEDIC_HEROIC_TAUNT,
	TAUNTATK_PYRO_GASBLAST,
};

typedef enum
{
	HUD_NOTIFY_YOUR_FLAG_TAKEN,
	HUD_NOTIFY_YOUR_FLAG_DROPPED,
	HUD_NOTIFY_YOUR_FLAG_RETURNED,
	HUD_NOTIFY_YOUR_FLAG_CAPTURED,

	HUD_NOTIFY_ENEMY_FLAG_TAKEN,
	HUD_NOTIFY_ENEMY_FLAG_DROPPED,
	HUD_NOTIFY_ENEMY_FLAG_RETURNED,
	HUD_NOTIFY_ENEMY_FLAG_CAPTURED,

	HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP,

	HUD_NOTIFY_NO_INVULN_WITH_FLAG,
	HUD_NOTIFY_NO_TELE_WITH_FLAG,

	HUD_NOTIFY_SPECIAL,

	HUD_NOTIFY_GOLDEN_WRENCH,

	HUD_NOTIFY_RD_ROBOT_ATTACKED,

	HUD_NOTIFY_HOW_TO_CONTROL_GHOST,
	HUD_NOTIFY_HOW_TO_CONTROL_KART,

	HUD_NOTIFY_PASSTIME_HOWTO,
	HUD_NOTIFY_PASSTIME_BALL_BASKET,
	HUD_NOTIFY_PASSTIME_BALL_ENDZONE,
	HUD_NOTIFY_PASSTIME_SCORE,
	HUD_NOTIFY_PASSTIME_FRIENDLY_SCORE,
	HUD_NOTIFY_PASSTIME_ENEMY_SCORE,
	HUD_NOTIFY_PASSTIME_NO_TELE,
	HUD_NOTIFY_PASSTIME_NO_CARRY,
	HUD_NOTIFY_PASSTIME_NO_INVULN,
	HUD_NOTIFY_PASSTIME_NO_DISGUISE,
	HUD_NOTIFY_PASSTIME_NO_CLOAK,
	HUD_NOTIFY_PASSTIME_NO_OOB,
	HUD_NOTIFY_PASSTIME_NO_HOLSTER,
	HUD_NOTIFY_PASSTIME_NO_TAUNT,

	NUM_STOCK_NOTIFICATIONS
} HudNotification_t;

class CTraceFilterIgnorePlayers : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnorePlayers, CTraceFilterSimple );

	CTraceFilterIgnorePlayers( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		return pEntity && !pEntity->IsPlayer();
	}
};

class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
		{
			return false;
		}

		return true;
	}

	int m_iIgnoreTeam;
};

class CTraceFilterIgnoreTeammatesAndTeamObjects : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammatesAndTeamObjects, CTraceFilterSimple );

	CTraceFilterIgnoreTeammatesAndTeamObjects( const IHandleEntity *passentity, int collisionGroup, int teamNumber )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
		m_iTeamNumber = teamNumber;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity && pEntity->GetTeamNumber() == m_iTeamNumber )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

private:
	int m_iTeamNumber;
};

// Unused
#define TF_DEATH_FIRST_BLOOD	0x0010
#define TF_DEATH_FEIGN_DEATH	0x0020
#define TF_DEATH_GIB			0x0080
#define TF_DEATH_PURGATORY		0x0100
#define TF_DEATH_AUSTRALIUM		0x0400

#define HUD_ALERT_SCRAMBLE_TEAMS 0

#define TF_CAMERA_DIST 64
#define TF_CAMERA_DIST_RIGHT 30
#define TF_CAMERA_DIST_UP 0

inline int GetEnemyTeam( CBaseEntity *ent )
{
	int myTeam = ent->GetTeamNumber();
	return ( myTeam == TF_TEAM_BLUE ? TF_TEAM_RED : ( myTeam == TF_TEAM_RED ? TF_TEAM_BLUE : TEAM_ANY ) );
}

bool IsSpaceToSpawnHere( const Vector &vecPos );

void BuildBigHeadTransformation( CBaseAnimating *pAnimating, CStudioHdr *pStudio, Vector *pos, Quaternion *q, matrix3x4_t const &cameraTransformation, int boneMask, class CBoneBitList &boneComputed, float flScale );

#endif // TF_SHAREDDEFS_H
