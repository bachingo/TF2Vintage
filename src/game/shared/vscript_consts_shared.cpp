//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: VScript constants and enums shared between the server and client.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "activitylist.h"
#include "in_buttons.h"
#include "teamplayroundbased_gamerules.h"
#ifdef CLIENT_DLL
#include "c_ai_basenpc.h"
#else
#include "ai_basenpc.h"
#include "globalstate.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FButtons, "Button mask bindings" )

	DEFINE_ENUMCONST( IN_ATTACK, "Button for +attack" )
	DEFINE_ENUMCONST( IN_JUMP, "Button for +jump" )
	DEFINE_ENUMCONST( IN_DUCK, "Button for +duck" )
	DEFINE_ENUMCONST( IN_FORWARD, "Button for + forward" )
	DEFINE_ENUMCONST( IN_BACK, "Button for +back" )
	DEFINE_ENUMCONST( IN_USE, "Button for +use" )
	DEFINE_ENUMCONST( IN_CANCEL, "Special button flag for attack cancel" )
	DEFINE_ENUMCONST( IN_LEFT, "Button for +left" )
	DEFINE_ENUMCONST( IN_RIGHT, "Button for +right" )
	DEFINE_ENUMCONST( IN_MOVELEFT, "Button for +moveleft" )
	DEFINE_ENUMCONST( IN_MOVERIGHT, "Button for +moveright" )
	DEFINE_ENUMCONST( IN_ATTACK2, "Button for +attack2" )
	DEFINE_ENUMCONST( IN_RUN, "Unused button (see IN.SPEED for sprint)" )
	DEFINE_ENUMCONST( IN_RELOAD, "Button for +reload" )
	DEFINE_ENUMCONST( IN_ALT1, "Button for +alt1" )
	DEFINE_ENUMCONST( IN_ALT2, "Button for +alt2" )
	DEFINE_ENUMCONST( IN_SCORE, "Button for +score" )
	DEFINE_ENUMCONST( IN_SPEED, "Button for +speed" )
	DEFINE_ENUMCONST( IN_WALK, "Button for +walk" )
	DEFINE_ENUMCONST( IN_ZOOM, "Button for +zoom" )
	DEFINE_ENUMCONST( IN_WEAPON1, "Special button used by weapons themselves" )
	DEFINE_ENUMCONST( IN_WEAPON2, "Special button used by weapons themselves" )
	DEFINE_ENUMCONST( IN_BULLRUSH, "Unused button" )
	DEFINE_ENUMCONST( IN_GRENADE1, "Button for +grenade1" )
	DEFINE_ENUMCONST( IN_GRENADE2, "Button for +grenade2" )
	DEFINE_ENUMCONST( IN_ATTACK3, "Button for +attack3" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FDmgType, "Damage flags for TakeDamage" )

	DEFINE_ENUMCONST( DMG_GENERIC, "" )
	DEFINE_ENUMCONST( DMG_CRUSH, "" )
	DEFINE_ENUMCONST( DMG_BULLET, "" )
	DEFINE_ENUMCONST( DMG_SLASH, "" )
	DEFINE_ENUMCONST( DMG_BURN, "" )
	DEFINE_ENUMCONST( DMG_VEHICLE, "" )
	DEFINE_ENUMCONST( DMG_FALL, "" )
	DEFINE_ENUMCONST( DMG_BLAST, "" )
	DEFINE_ENUMCONST( DMG_CLUB, "" )
	DEFINE_ENUMCONST( DMG_SHOCK, "" )
	DEFINE_ENUMCONST( DMG_SONIC, "" )
	DEFINE_ENUMCONST( DMG_ENERGYBEAM, "" )
	DEFINE_ENUMCONST( DMG_PREVENT_PHYSICS_FORCE, "" )
	DEFINE_ENUMCONST( DMG_NEVERGIB, "" )
	DEFINE_ENUMCONST( DMG_ALWAYSGIB, "" )
	DEFINE_ENUMCONST( DMG_DROWN, "" )
	DEFINE_ENUMCONST( DMG_PARALYZE, "" )
	DEFINE_ENUMCONST( DMG_NERVEGAS, "" )
	DEFINE_ENUMCONST( DMG_POISON, "" )
	DEFINE_ENUMCONST( DMG_RADIATION, "" )
	DEFINE_ENUMCONST( DMG_DROWNRECOVER, "" )
	DEFINE_ENUMCONST( DMG_ACID, "" )
	DEFINE_ENUMCONST( DMG_SLOWBURN, "" )
	DEFINE_ENUMCONST( DMG_REMOVENORAGDOLL, "" )
	DEFINE_ENUMCONST( DMG_PHYSGUN, "" )
	DEFINE_ENUMCONST( DMG_PLASMA, "" )
	DEFINE_ENUMCONST( DMG_AIRBOAT, "" )
	DEFINE_ENUMCONST( DMG_DISSOLVE, "" )
	DEFINE_ENUMCONST( DMG_BLAST_SURFACE, "" )
	DEFINE_ENUMCONST( DMG_DIRECT, "" )
	DEFINE_ENUMCONST( DMG_BUCKSHOT, "" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ERenderMode, "Render modes used by Get/SetRenderMode" )

	DEFINE_ENUMCONST( kRenderNormal, "" )
	DEFINE_ENUMCONST( kRenderTransColor, "" )
	DEFINE_ENUMCONST( kRenderTransTexture, "" )
	DEFINE_ENUMCONST( kRenderGlow, "" )
	DEFINE_ENUMCONST( kRenderTransAlpha, "" )
	DEFINE_ENUMCONST( kRenderTransAdd, "" )
	DEFINE_ENUMCONST( kRenderEnvironmental, "" )
	DEFINE_ENUMCONST( kRenderTransAddFrameBlend, "" )
	DEFINE_ENUMCONST( kRenderTransAlphaAdd, "" )
	DEFINE_ENUMCONST( kRenderWorldGlow, "" )
	DEFINE_ENUMCONST( kRenderNone, "" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ERenderFx, "Render effects used by Get/SetRenderFx" )

	DEFINE_ENUMCONST( kRenderFxNone, "" )
	DEFINE_ENUMCONST( kRenderFxPulseSlow, "" )
	DEFINE_ENUMCONST( kRenderFxPulseFast, "" )
	DEFINE_ENUMCONST( kRenderFxPulseSlowWide, "" )
	DEFINE_ENUMCONST( kRenderFxPulseFastWide, "" )
	DEFINE_ENUMCONST( kRenderFxFadeSlow, "" )
	DEFINE_ENUMCONST( kRenderFxFadeFast, "" )
	DEFINE_ENUMCONST( kRenderFxSolidSlow, "" )
	DEFINE_ENUMCONST( kRenderFxSolidFast, "" )
	DEFINE_ENUMCONST( kRenderFxStrobeSlow, "" )
	DEFINE_ENUMCONST( kRenderFxStrobeFast, "" )
	DEFINE_ENUMCONST( kRenderFxStrobeFaster, "" )
	DEFINE_ENUMCONST( kRenderFxFlickerSlow, "" )
	DEFINE_ENUMCONST( kRenderFxFlickerFast, "" )
	DEFINE_ENUMCONST( kRenderFxNoDissipation, "" )
	DEFINE_ENUMCONST( kRenderFxDistort, "" )
	DEFINE_ENUMCONST( kRenderFxHologram, "" )
	DEFINE_ENUMCONST( kRenderFxExplode, "" )
	DEFINE_ENUMCONST( kRenderFxGlowShell, "" )
	DEFINE_ENUMCONST( kRenderFxClampMinScale, "" )
	DEFINE_ENUMCONST( kRenderFxEnvRain, "" )
	DEFINE_ENUMCONST( kRenderFxEnvSnow, "" )
	DEFINE_ENUMCONST( kRenderFxSpotlight, "" )
	DEFINE_ENUMCONST( kRenderFxRagdoll, "" )
	DEFINE_ENUMCONST( kRenderFxPulseFastWider, "" )
	DEFINE_ENUMCONST( kRenderFxMax, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FEntityEffects, "" )

	DEFINE_ENUMCONST( EF_BONEMERGE, "" )
	DEFINE_ENUMCONST( EF_BRIGHTLIGHT, "" )
	DEFINE_ENUMCONST( EF_DIMLIGHT, "" )
	DEFINE_ENUMCONST( EF_NOINTERP, "" )
	DEFINE_ENUMCONST( EF_NOSHADOW, "" )
	DEFINE_ENUMCONST( EF_NODRAW, "" )
	DEFINE_ENUMCONST( EF_NORECEIVESHADOW, "" )
	DEFINE_ENUMCONST( EF_BONEMERGE_FASTCULL, "" )
	DEFINE_ENUMCONST( EF_ITEM_BLINK, "" )
	DEFINE_ENUMCONST( EF_PARENT_ANIMATES, "" )
	DEFINE_ENUMCONST( EF_MAX_BITS, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FEntityEFlags, "Flags used in AddEFlags" )

	DEFINE_ENUMCONST( EFL_KILLME, "" )
	DEFINE_ENUMCONST( EFL_DORMANT, "" )
	DEFINE_ENUMCONST( EFL_NOCLIP_ACTIVE, "" )
	DEFINE_ENUMCONST( EFL_SETTING_UP_BONES, "" )
	DEFINE_ENUMCONST( EFL_HAS_PLAYER_CHILD, "" )
	DEFINE_ENUMCONST( EFL_KEEP_ON_RECREATE_ENTITIES, "" )
	DEFINE_ENUMCONST( EFL_DIRTY_SHADOWUPDATE, "" )
	DEFINE_ENUMCONST( EFL_NOTIFY, "" )
	DEFINE_ENUMCONST( EFL_FORCE_CHECK_TRANSMIT, "" )
	DEFINE_ENUMCONST( EFL_BOT_FROZEN, "" )
	DEFINE_ENUMCONST( EFL_SERVER_ONLY, "" )
	DEFINE_ENUMCONST( EFL_NO_AUTO_EDICT_ATTACH, "" )
	DEFINE_ENUMCONST( EFL_DIRTY_ABSTRANSFORM, "" )
	DEFINE_ENUMCONST( EFL_DIRTY_ABSVELOCITY, "" )
	DEFINE_ENUMCONST( EFL_DIRTY_ABSANGVELOCITY, "" )
	DEFINE_ENUMCONST( EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS, "" )
	DEFINE_ENUMCONST( EFL_DIRTY_SPATIAL_PARTITION, "")
	DEFINE_ENUMCONST( EFL_FORCE_ALLOW_MOVEPARENT, "" )
	DEFINE_ENUMCONST( EFL_IN_SKYBOX, "" )
	DEFINE_ENUMCONST( EFL_USE_PARTITION_WHEN_NOT_SOLID, "" )
	DEFINE_ENUMCONST( EFL_TOUCHING_FLUID, "" )
	DEFINE_ENUMCONST( EFL_IS_BEING_LIFTED_BY_BARNACLE, "" )
	DEFINE_ENUMCONST( EFL_NO_ROTORWASH_PUSH, "" )
	DEFINE_ENUMCONST( EFL_NO_THINK_FUNCTION, "" )
	DEFINE_ENUMCONST( EFL_NO_GAME_PHYSICS_SIMULATION, "" )
	DEFINE_ENUMCONST( EFL_CHECK_UNTOUCH, "" )
	DEFINE_ENUMCONST( EFL_DONTBLOCKLOS, "" )
	DEFINE_ENUMCONST( EFL_NO_DISSOLVE, "" )
	DEFINE_ENUMCONST( EFL_NO_MEGAPHYSCANNON_RAGDOLL, "" )
	DEFINE_ENUMCONST( EFL_NO_WATER_VELOCITY_CHANGE, "" )
	DEFINE_ENUMCONST( EFL_NO_PHYSCANNON_INTERACTION, "" )
	DEFINE_ENUMCONST( EFL_NO_DAMAGE_FORCES, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( EMoveCollide, "" )

	DEFINE_ENUMCONST( MOVECOLLIDE_DEFAULT, "" )
	DEFINE_ENUMCONST( MOVECOLLIDE_FLY_BOUNCE, "" )
	DEFINE_ENUMCONST( MOVECOLLIDE_FLY_CUSTOM, "" )
	DEFINE_ENUMCONST( MOVECOLLIDE_FLY_SLIDE, "" )
	DEFINE_ENUMCONST( MOVECOLLIDE_MAX_BITS, "" )
	DEFINE_ENUMCONST( MOVECOLLIDE_COUNT, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( EMoveType, "" )

	DEFINE_ENUMCONST( MOVETYPE_NONE, "" )
	DEFINE_ENUMCONST( MOVETYPE_ISOMETRIC, "" )
	DEFINE_ENUMCONST( MOVETYPE_WALK, "" )
	DEFINE_ENUMCONST( MOVETYPE_STEP, "" )
	DEFINE_ENUMCONST( MOVETYPE_FLY, "" )
	DEFINE_ENUMCONST( MOVETYPE_FLYGRAVITY, "" )
	DEFINE_ENUMCONST( MOVETYPE_VPHYSICS, "" )
	DEFINE_ENUMCONST( MOVETYPE_PUSH, "" )
	DEFINE_ENUMCONST( MOVETYPE_NOCLIP, "" )
	DEFINE_ENUMCONST( MOVETYPE_LADDER, "" )
	DEFINE_ENUMCONST( MOVETYPE_OBSERVER, "" )
	DEFINE_ENUMCONST( MOVETYPE_CUSTOM, "" )
	DEFINE_ENUMCONST( MOVETYPE_LAST, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ESpectatorMode, "" )

	DEFINE_ENUMCONST( OBS_MODE_NONE, "" )
	DEFINE_ENUMCONST( OBS_MODE_DEATHCAM, "" )
	DEFINE_ENUMCONST( OBS_MODE_FREEZECAM, "" )
	DEFINE_ENUMCONST( OBS_MODE_FIXED, "" )
	DEFINE_ENUMCONST( OBS_MODE_IN_EYE, "" )
	DEFINE_ENUMCONST( OBS_MODE_CHASE, "" )
	DEFINE_ENUMCONST( OBS_MODE_POI, "" )
	DEFINE_ENUMCONST( OBS_MODE_ROAMING, "" )
	DEFINE_ENUMCONST( NUM_OBSERVER_MODES, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( EHitGroup, "Hit groups from traces" )

	DEFINE_ENUMCONST( HITGROUP_GENERIC, "" )
	DEFINE_ENUMCONST( HITGROUP_HEAD, "" )
	DEFINE_ENUMCONST( HITGROUP_CHEST, "" )
	DEFINE_ENUMCONST( HITGROUP_STOMACH, "" )
	DEFINE_ENUMCONST( HITGROUP_LEFTARM, "" )
	DEFINE_ENUMCONST( HITGROUP_RIGHTARM, "" )
	DEFINE_ENUMCONST( HITGROUP_LEFTLEG, "" )
	DEFINE_ENUMCONST( HITGROUP_RIGHTLEG, "" )
	DEFINE_ENUMCONST( HITGROUP_GEAR, "" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ESolidType, "" )

	DEFINE_ENUMCONST( SOLID_NONE, "" )
	DEFINE_ENUMCONST( SOLID_BSP, "" )
	DEFINE_ENUMCONST( SOLID_BBOX, "" )
	DEFINE_ENUMCONST( SOLID_OBB, "" )
	DEFINE_ENUMCONST( SOLID_OBB_YAW, "" )
	DEFINE_ENUMCONST( SOLID_CUSTOM, "" )
	DEFINE_ENUMCONST( SOLID_VPHYSICS, "" )
	DEFINE_ENUMCONST( SOLID_LAST, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ECollisionGroup, "" )

	DEFINE_ENUMCONST( COLLISION_GROUP_NONE, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_DEBRIS, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_DEBRIS_TRIGGER, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_INTERACTIVE_DEBRIS, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_INTERACTIVE, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_PLAYER, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_BREAKABLE_GLASS, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_VEHICLE, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_PLAYER_MOVEMENT, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_NPC, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_IN_VEHICLE, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_WEAPON, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_VEHICLE_CLIP, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_PROJECTILE, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_DOOR_BLOCKER, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_PASSABLE_DOOR, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_DISSOLVING, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_PUSHAWAY, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_NPC_ACTOR, "" )
	DEFINE_ENUMCONST( COLLISION_GROUP_NPC_SCRIPTED, "" )
	DEFINE_ENUMCONST( LAST_SHARED_COLLISION_GROUP, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FSolid, "" )

	DEFINE_ENUMCONST( FSOLID_CUSTOMRAYTEST, "" )
	DEFINE_ENUMCONST( FSOLID_CUSTOMBOXTEST, "" )
	DEFINE_ENUMCONST( FSOLID_NOT_SOLID, "" )
	DEFINE_ENUMCONST( FSOLID_TRIGGER, "" )
	DEFINE_ENUMCONST( FSOLID_NOT_STANDABLE, "" )
	DEFINE_ENUMCONST( FSOLID_VOLUME_CONTENTS, "" )
	DEFINE_ENUMCONST( FSOLID_FORCE_WORLD_ALIGNED, "" )
	DEFINE_ENUMCONST( FSOLID_USE_TRIGGER_BOUNDS, "" )
	DEFINE_ENUMCONST( FSOLID_ROOT_PARENT_ALIGNED, "" )
	DEFINE_ENUMCONST( FSOLID_TRIGGER_TOUCH_DEBRIS, "" )
	DEFINE_ENUMCONST( FSOLID_MAX_BITS, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FSurf, "" )

	DEFINE_ENUMCONST( SURF_LIGHT, "" )
	DEFINE_ENUMCONST( SURF_SKY2D, "" )
	DEFINE_ENUMCONST( SURF_SKY, "" )
	DEFINE_ENUMCONST( SURF_WARP, "" )
	DEFINE_ENUMCONST( SURF_TRANS, "" )
	DEFINE_ENUMCONST( SURF_NOPORTAL, "" )
	DEFINE_ENUMCONST( SURF_TRIGGER, "" )
	DEFINE_ENUMCONST( SURF_NODRAW, "" )
	DEFINE_ENUMCONST( SURF_HINT, "" )
	DEFINE_ENUMCONST( SURF_SKIP, "" )
	DEFINE_ENUMCONST( SURF_NOLIGHT, "" )
	DEFINE_ENUMCONST( SURF_BUMPLIGHT, "" )
	DEFINE_ENUMCONST( SURF_NOSHADOWS, "" )
	DEFINE_ENUMCONST( SURF_NODECALS, "" )
	DEFINE_ENUMCONST( SURF_NOCHOP, "" )
	DEFINE_ENUMCONST( SURF_HITBOX, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FContents, "Contents of a solid to test intersection" )

	DEFINE_ENUMCONST( CONTENTS_EMPTY, "" )
	DEFINE_ENUMCONST( CONTENTS_SOLID, "" )
	DEFINE_ENUMCONST( CONTENTS_WINDOW, "" )
	DEFINE_ENUMCONST( CONTENTS_AUX, "" )
	DEFINE_ENUMCONST( CONTENTS_GRATE, "" )
	DEFINE_ENUMCONST( CONTENTS_SLIME, "" )
	DEFINE_ENUMCONST( CONTENTS_WATER, "" )
	DEFINE_ENUMCONST( CONTENTS_BLOCKLOS, "" )
	DEFINE_ENUMCONST( CONTENTS_OPAQUE, "" )
	DEFINE_ENUMCONST( LAST_VISIBLE_CONTENTS, "" )
	DEFINE_ENUMCONST( ALL_VISIBLE_CONTENTS, "" )
	DEFINE_ENUMCONST( CONTENTS_TESTFOGVOLUME, "" )
	DEFINE_ENUMCONST( CONTENTS_UNUSED, "" )
	DEFINE_ENUMCONST( CONTENTS_UNUSED6, "" )
	DEFINE_ENUMCONST( CONTENTS_TEAM1, "" )
	DEFINE_ENUMCONST( CONTENTS_TEAM2, "" )
	DEFINE_ENUMCONST( CONTENTS_IGNORE_NODRAW_OPAQUE, "" )
	DEFINE_ENUMCONST( CONTENTS_MOVEABLE, "" )
	DEFINE_ENUMCONST( CONTENTS_AREAPORTAL, "" )
	DEFINE_ENUMCONST( CONTENTS_PLAYERCLIP, "" )
	DEFINE_ENUMCONST( CONTENTS_MONSTERCLIP, "" )
	DEFINE_ENUMCONST( CONTENTS_CURRENT_0, "" )
	DEFINE_ENUMCONST( CONTENTS_CURRENT_90, "" )
	DEFINE_ENUMCONST( CONTENTS_CURRENT_180, "" )
	DEFINE_ENUMCONST( CONTENTS_CURRENT_270, "" )
	DEFINE_ENUMCONST( CONTENTS_CURRENT_UP, "" )
	DEFINE_ENUMCONST( CONTENTS_CURRENT_DOWN, "" )
	DEFINE_ENUMCONST( CONTENTS_ORIGIN, "" )
	DEFINE_ENUMCONST( CONTENTS_MONSTER, "" )
	DEFINE_ENUMCONST( CONTENTS_DEBRIS, "" )
	DEFINE_ENUMCONST( CONTENTS_DETAIL, "" )
	DEFINE_ENUMCONST( CONTENTS_TRANSLUCENT, "" )
	DEFINE_ENUMCONST( CONTENTS_LADDER, "" )
	DEFINE_ENUMCONST( CONTENTS_HITBOX, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( EMapLoad, "Map load enum for GetLoadType()" )

	DEFINE_ENUMCONST_NAMED( MapLoad_NewGame, "NewGame", "Map was loaded from a new game" )
	DEFINE_ENUMCONST_NAMED( MapLoad_LoadGame, "LoadGame", "Map was loaded from a save file" )
	DEFINE_ENUMCONST_NAMED( MapLoad_Transition, "Transition", "Map was loaded from a level transition" )
	DEFINE_ENUMCONST_NAMED( MapLoad_Background, "Background", "Map was loaded as a background map" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( ERoundState, "Round state from GetRoundState" )

	DEFINE_ENUMCONST( GR_STATE_INIT, "" )
	DEFINE_ENUMCONST( GR_STATE_PREGAME, "" )
	DEFINE_ENUMCONST( GR_STATE_STARTGAME, "" )
	DEFINE_ENUMCONST( GR_STATE_PREROUND, "" )
	DEFINE_ENUMCONST( GR_STATE_RND_RUNNING, "" )
	DEFINE_ENUMCONST( GR_STATE_TEAM_WIN, "" )
	DEFINE_ENUMCONST( GR_STATE_RESTART, "" )
	DEFINE_ENUMCONST( GR_STATE_STALEMATE, "" )
	DEFINE_ENUMCONST( GR_STATE_GAME_OVER, "" )
	DEFINE_ENUMCONST( GR_STATE_BONUS, "" )
	DEFINE_ENUMCONST( GR_STATE_BETWEEN_RNDS, "" )
	DEFINE_ENUMCONST( GR_NUM_ROUND_STATES, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( EHudNotify, "" )

	DEFINE_ENUMCONST( HUD_PRINTNOTIFY, "" )
	DEFINE_ENUMCONST( HUD_PRINTCONSOLE, "" )
	DEFINE_ENUMCONST( HUD_PRINTTALK, "" )
	DEFINE_ENUMCONST( HUD_PRINTCENTER, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FHideHud, "" )

	DEFINE_ENUMCONST( HIDEHUD_WEAPONSELECTION, "" )
	DEFINE_ENUMCONST( HIDEHUD_FLASHLIGHT, "" )
	DEFINE_ENUMCONST( HIDEHUD_ALL, "" )
	DEFINE_ENUMCONST( HIDEHUD_HEALTH, "" )
	DEFINE_ENUMCONST( HIDEHUD_PLAYERDEAD, "" )
	DEFINE_ENUMCONST( HIDEHUD_NEEDSUIT, "" )
	DEFINE_ENUMCONST( HIDEHUD_MISCSTATUS, "" )
	DEFINE_ENUMCONST( HIDEHUD_CHAT, "" )
	DEFINE_ENUMCONST( HIDEHUD_CROSSHAIR, "" )
	DEFINE_ENUMCONST( HIDEHUD_VEHICLE_CROSSHAIR, "" )
	DEFINE_ENUMCONST( HIDEHUD_INVEHICLE, "" )
	DEFINE_ENUMCONST( HIDEHUD_BONUS_PROGRESS, "" )
	DEFINE_ENUMCONST( HIDEHUD_BITCOUNT, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( FPlayer, "" )

	DEFINE_ENUMCONST( FL_ONGROUND, "" )
	DEFINE_ENUMCONST( FL_DUCKING, "" )
	DEFINE_ENUMCONST( FL_ANIMDUCKING, "" )
	DEFINE_ENUMCONST( FL_WATERJUMP, "" )
	DEFINE_ENUMCONST( FL_ONTRAIN, "" )
	DEFINE_ENUMCONST( FL_INRAIN, "" )
	DEFINE_ENUMCONST( FL_FROZEN, "" )
	DEFINE_ENUMCONST( FL_ATCONTROLS, "" )
	DEFINE_ENUMCONST( FL_CLIENT, "" )
	DEFINE_ENUMCONST( FL_FAKECLIENT, "" )
	DEFINE_ENUMCONST( FL_INWATER, "" )
	DEFINE_ENUMCONST( FL_FLY, "" )
	DEFINE_ENUMCONST( FL_SWIM, "" )
	DEFINE_ENUMCONST( FL_CONVEYOR, "" )
	DEFINE_ENUMCONST( FL_NPC, "" )
	DEFINE_ENUMCONST( FL_GODMODE, "" )
	DEFINE_ENUMCONST( FL_NOTARGET, "" )
	DEFINE_ENUMCONST( FL_AIMTARGET, "" )
	DEFINE_ENUMCONST( FL_PARTIALGROUND, "" )
	DEFINE_ENUMCONST( FL_STATICPROP, "" )
	DEFINE_ENUMCONST( FL_GRAPHED, "" )
	DEFINE_ENUMCONST( FL_GRENADE, "" )
	DEFINE_ENUMCONST( FL_STEPMOVEMENT, "" )
	DEFINE_ENUMCONST( FL_DONTTOUCH, "" )
	DEFINE_ENUMCONST( FL_BASEVELOCITY, "" )
	DEFINE_ENUMCONST( FL_WORLDBRUSH, "" )
	DEFINE_ENUMCONST( FL_OBJECT, "" )
	DEFINE_ENUMCONST( FL_KILLME, "" )
	DEFINE_ENUMCONST( FL_ONFIRE, "" )
	DEFINE_ENUMCONST( FL_DISSOLVING, "" )
	DEFINE_ENUMCONST( FL_TRANSRAGDOLL, "" )
	DEFINE_ENUMCONST( FL_UNBLOCKABLE_BY_PLAYER, "" )
	DEFINE_ENUMCONST( PLAYER_FLAG_BITS, "" )

END_SCRIPTENUM()

//=============================================================================
//=============================================================================

void RegisterActivityConstants()
{
	// Make sure there are no activities declared yet
	if (g_pScriptVM->ValueExists( "ACT_RESET" ))
		return;

	// Register activity constants by just iterating through the entire activity list
	for (int i = 0; i < ActivityList_HighestIndex(); i++)
	{
		ScriptRegisterConstantNamed( g_pScriptVM, i, ActivityList_NameForIndex(i), "" );
	}
}

//=============================================================================
//=============================================================================

extern void RegisterWeaponScriptConstants();

void RegisterSharedScriptConstants()
{
	//
	// Enums
	//
	ScriptEnumDesc_t *pEnumDesc = ScriptEnumDesc_t::GetDescList();
	while ( pEnumDesc )
	{
		g_pScriptVM->RegisterEnum( pEnumDesc );
		pEnumDesc = pEnumDesc->m_pNext;
	}

	// 
	// Activities
	// 

	// Scripts have to use this function before using any activity constants.
	// This is because initializing 1,700+ constants every time a level loads and letting them lay around
	// usually doing nothing sounds like a bad idea.
	ScriptRegisterFunction( g_pScriptVM, RegisterActivityConstants, "Registers all activity IDs as usable constants." );

#ifdef GAME_DLL
	// 
	// Sound Types, Contexts, and Channels
	// (QueryHearSound hook can use these)
	// 
	ScriptRegisterConstant( g_pScriptVM, SOUND_NONE, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_COMBAT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_WORLD, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PLAYER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_DANGER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_BULLET_IMPACT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CARCASS, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_MEAT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_GARBAGE, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_THUMPER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_BUGBAIT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PHYSICS_DANGER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_DANGER_SNIPERONLY, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_MOVE_AWAY, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PLAYER_VEHICLE, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_LOW, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_MEDIUM, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_HIGH, "Sound type used in QueryHearSound hooks, etc." );

	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_FROM_SNIPER, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_GUNFIRE, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_MORTAR, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_COMBINE_ONLY,  "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_REACT_TO_SOURCE, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_EXPLOSION, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_EXCLUDE_COMBINE, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_DANGER_APPROACH, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_ALLIES_ONLY, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_PLAYER_VEHICLE, "Sound context used in QueryHearSound hooks, etc." );

	ScriptRegisterConstant( g_pScriptVM, ALL_CONTEXTS, "All sound contexts useable in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, ALL_SCENTS, "All \"scent\" sound types useable in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, ALL_SOUNDS, "All sound types useable in QueryHearSound hooks, etc." );

	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_UNSPECIFIED, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATING, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATED_DANGER, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_WEAPON, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_INJURY, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_BULLET_IMPACT, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_NPC_FOOTSTEP, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_SPOOKY_NOISE, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_ZOMBINE_GRENADE, "Sound channel used in QueryHearSound hooks, etc." );

	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_MACHINEGUN, "SOUNDENT_VOLUME_MACHINEGUN", "Sound volume preset for use in InsertAISound, etc." );
	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_SHOTGUN, "SOUNDENT_VOLUME_SHOTGUN", "Sound volume preset for use in InsertAISound, etc." );
	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_PISTOL, "SOUNDENT_VOLUME_PISTOL", "Sound volume preset for use in InsertAISound, etc." );
	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_EMPTY, "SOUNDENT_VOLUME_EMPTY", "Sound volume preset for use in InsertAISound, etc." );

	// 
	// Capabilities
	// 
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_GROUND, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_JUMP, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_FLY, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_CLIMB, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_SWIM, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_CRAWL, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_SHOOT, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SKIP_NAV_GROUND_CHECK, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	//ScriptRegisterConstant( g_pScriptVM, bits_CAP_HEAR, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_AUTO_DOORS, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_OPEN_DOORS, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_TURN_HEAD, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_RANGE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_RANGE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_MELEE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_MELEE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_RANGE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_RANGE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_MELEE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_MELEE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE_WEAPONS, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	//ScriptRegisterConstant( g_pScriptVM, bits_CAP_STRAFE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_ANIMATEDFACE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE_SHOT_REGULATOR, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_FRIENDLY_DMG_IMMUNE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SQUAD, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_DUCK, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_NO_HIT_PLAYER, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_AIM_GUN, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_NO_HIT_SQUADMATES, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SIMPLE_RADIUS_DAMAGE, "NPC/player/weapon capability used in GetCapabilities(), etc." );

	ScriptRegisterConstant( g_pScriptVM, bits_CAP_DOORS_GROUP, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_RANGE_ATTACK_GROUP, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MELEE_ATTACK_GROUP, "NPC/player/weapon capability used in GetCapabilities(), etc." );

	// 
	// Class_T classes
	// 
	ScriptRegisterConstant( g_pScriptVM, CLASS_NONE, "No class." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER, "Used by players." );

#ifdef HL2_DLL

	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY, "Used by citizens, hacked manhacks, and other misc. allies." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY_VITAL, "Used by Alyx, Barney, and other allies vital to HL2." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ANTLION, "Used by antlions, antlion guards, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_BARNACLE, "Used by barnacles." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_BULLSEYE, "Used by npc_bullseye." );
	//ScriptRegisterConstant( g_pScriptVM, CLASS_BULLSQUID, "Used by bullsquids." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_CITIZEN_PASSIVE, "Used by citizens when the \"gordon_precriminal\" or \"citizens_passive\" states are enabled." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_CITIZEN_REBEL, "UNUSED IN HL2. Rebels normally use CLASS_PLAYER_ALLY." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_COMBINE, "Used by Combine soldiers, Combine turrets, and other misc. Combine NPCs." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_COMBINE_GUNSHIP, "Used by Combine gunships, helicopters, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_CONSCRIPT, "UNUSED IN HL2. Would've been used by conscripts." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_HEADCRAB, "Used by headcrabs." );
	//ScriptRegisterConstant( g_pScriptVM, CLASS_HOUNDEYE, "Used by houndeyes." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_MANHACK, "Used by Combine manhacks." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_METROPOLICE, "Used by Combine metrocops." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_MILITARY, "In HL2, this is only used by npc_combinecamera and func_guntarget. This appears to be recognized as a Combine class." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_SCANNER, "Used by Combine city scanners and claw scanners." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_STALKER, "Used by Combine stalkers." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_VORTIGAUNT, "Used by vortigaunts." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ZOMBIE, "Used by zombies." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PROTOSNIPER, "Used by Combine snipers." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_MISSILE, "Used by RPG and APC missiles." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_FLARE, "Used by env_flares." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_EARTH_FAUNA, "Used by birds and other terrestrial animals." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_HACKED_ROLLERMINE, "Used by rollermines which were hacked by Alyx." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_COMBINE_HUNTER, "Used by Combine hunters." );

#elif defined( HL1_DLL )

	ScriptRegisterConstant( g_pScriptVM, CLASS_HUMAN_PASSIVE, "Used by scientists." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_HUMAN_MILITARY, "Used by HECU marines, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_MILITARY, "Used by alien grunts, alien slaves/vortigaunts, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_MONSTER, "Used by zombies, houndeyes, barnacles, and other misc. monsters." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_PREY, "Used by headcrabs, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_PREDATOR, "Used by bullsquids, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_INSECT, "Used by cockroaches." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY, "Used by security guards/Barneys." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_BIOWEAPON, "Used by a player's hivehand hornets." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_BIOWEAPON, "Used by an alien grunt's hivehand hornets." );

#else

	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY, "Used by player allies." );

#endif

	ScriptRegisterConstant( g_pScriptVM, NUM_AI_CLASSES, "Number of AI classes." );

	// 
	// Misc. AI
	// 
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_INVALID, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_NONE, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_IDLE, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_ALERT, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_COMBAT, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_SCRIPT, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_PLAYDEAD, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_PRONE, "When in clutches of barnacle (NPC state type used in GetNPCState(), etc.)" );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_DEAD, "NPC state type used in GetNPCState(), etc." );

	ScriptRegisterConstant( g_pScriptVM, AISS_AWAKE, "NPC is awake. (NPC sleep state used in Get/SetSleepState())" );
	ScriptRegisterConstant( g_pScriptVM, AISS_WAITING_FOR_THREAT, "NPC is asleep and will awaken upon seeing an enemy. (NPC sleep state used in Get/SetSleepState())" );
	ScriptRegisterConstant( g_pScriptVM, AISS_WAITING_FOR_PVS, "NPC is asleep and will awaken upon entering a player's PVS. (NPC sleep state used in Get/SetSleepState())" );
	ScriptRegisterConstant( g_pScriptVM, AISS_WAITING_FOR_INPUT, "NPC is asleep and will only awaken upon receiving the Wake input. (NPC sleep state used in Get/SetSleepState())" );
	//ScriptRegisterConstant( g_pScriptVM, AISS_AUTO_PVS, "" );
	//ScriptRegisterConstant( g_pScriptVM, AISS_AUTO_PVS_AFTER_PVS, "" );
	ScriptRegisterConstant( g_pScriptVM, AI_SLEEP_FLAGS_NONE, "No sleep flags. (NPC sleep flag used in Add/Remove/HasSleepFlags())" );
	ScriptRegisterConstant( g_pScriptVM, AI_SLEEP_FLAG_AUTO_PVS, "Indicates a NPC will sleep upon exiting PVS. (NPC sleep flag used in Add/Remove/HasSleepFlags())" );
	ScriptRegisterConstant( g_pScriptVM, AI_SLEEP_FLAG_AUTO_PVS_AFTER_PVS, "Indicates a NPC will sleep upon exiting PVS after entering PVS for the first time(?????) (NPC sleep flag used in Add/Remove/HasSleepFlags())" );

	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_PLAYING, "SCRIPT_PLAYING", "Playing the action animation." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_WAIT, "SCRIPT_WAIT", "Waiting on everyone in the script to be ready. Plays the pre idle animation if there is one." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_POST_IDLE, "SCRIPT_POST_IDLE", "Playing the post idle animation after playing the action animation." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_CLEANUP, "SCRIPT_CLEANUP", "Cancelling the script / cleaning up." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_WALK_TO_MARK, "SCRIPT_WALK_TO_MARK", "Walking to the scripted sequence position." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_RUN_TO_MARK, "SCRIPT_RUN_TO_MARK", "Running to the scripted sequence position." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_CUSTOM_MOVE_TO_MARK, "SCRIPT_CUSTOM_MOVE_TO_MARK", "Moving to the scripted sequence position while playing a custom movement animation." );
#endif

	// 
	// Misc. General
	// 
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_NO, "Don't take damage (Use with GetTakeDamage/SetTakeDamage)" );
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_EVENTS_ONLY, "Call damage functions, but don't modify health (Use with GetTakeDamage/SetTakeDamage)" );
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_YES, "Allow damage to be taken (Use with GetTakeDamage/SetTakeDamage)" );
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_AIM, "(Use with GetTakeDamage/SetTakeDamage)" );

#ifdef GAME_DLL
	ScriptRegisterConstant( g_pScriptVM, GLOBAL_OFF, "Global state used by the Globals singleton." );
	ScriptRegisterConstant( g_pScriptVM, GLOBAL_ON, "Global state used by the Globals singleton." );
	ScriptRegisterConstant( g_pScriptVM, GLOBAL_DEAD, "Global state used by the Globals singleton." );

	ScriptRegisterConstantNamed( g_pScriptVM, 0.03125, "Server.DIST_EPSILON", "" );
	ScriptRegisterConstantNamed( g_pScriptVM, MAX_PLAYERS, "Server.MAX_PLAYERS", "" );
	ScriptRegisterConstantNamed( g_pScriptVM, MAX_EDICTS, "Server.MAX_EDICTS", "" );
#endif

	RegisterWeaponScriptConstants();
}
