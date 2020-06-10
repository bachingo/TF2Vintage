//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_player_shared.h"
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "effect_dispatch_data.h"
#include "tf_item.h"
#include "entity_capture_flag.h"
#include "baseobject_shared.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_shotgun.h"
#include "in_buttons.h"
#include "tf_viewmodel.h"
#include "tf_weapon_invis.h"
#include "tf_wearable_demoshield.h"
#include "tf_weapon_buff_item.h"
#include "tf_weapon_sword.h"
#include "animation.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_fx.h"
#include "soundenvelope.h"
#include "c_tf_playerclass.h"
#include "iviewrender.h"
#include "engine/ivdebugoverlay.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#include "prediction.h"

#define CTFPlayerClass C_TFPlayerClass

// Server specific.
#else
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "util.h"
#include "tf_team.h"
#include "player_vs_environment/merasmus.h"
#include "tf_gamestats.h"
#include "tf_playerclass.h"
#include "tf_weapon_builder.h"
#endif

ConVar tf_spy_invis_time( "tf_spy_invis_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );
ConVar tf_spy_invis_unstealth_time( "tf_spy_invis_unstealth_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );

ConVar tf_spy_max_cloaked_speed( "tf_spy_max_cloaked_speed", "999", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );	// no cap
ConVar tf_max_health_boost( "tf_max_health_boost", "1.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max health factor that players can be boosted to by healers.", true, 1.0, false, 0 );
ConVar tf_invuln_time( "tf_invuln_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Time it takes for invulnerability to wear off." );
ConVar tf_soldier_buff_pulses( "tf_soldier_buff_pulses", "10", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Time it takes for buff to wear off." );

#ifdef GAME_DLL
ConVar tf_boost_drain_time( "tf_boost_drain_time", "15.0", FCVAR_DEVELOPMENTONLY, "Time it takes for a full health boost to drain away from a player.", true, 0.1, false, 0 );
ConVar tf_debug_bullets( "tf_debug_bullets", "0", FCVAR_CHEAT, "Visualize bullet traces." );
ConVar tf_damage_events_track_for( "tf_damage_events_track_for", "30", FCVAR_DEVELOPMENTONLY );
#endif

ConVar tf_useparticletracers( "tf_useparticletracers", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use particle tracers instead of old style ones." );
ConVar tf_spy_cloak_consume_rate( "tf_spy_cloak_consume_rate", "10.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to use per second while cloaked, from 100 max )" );	// 10 seconds of invis
ConVar tf_spy_cloak_regen_rate( "tf_spy_cloak_regen_rate", "3.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to regen per second, up to 100 max" );		// 30 seconds to full charge
ConVar tf_spy_cloak_no_attack_time( "tf_spy_cloak_no_attack_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "time after uncloaking that the spy is prohibited from attacking" );

//ConVar tf_spy_stealth_blink_time( "tf_spy_stealth_blink_time", "0.3", FCVAR_DEVELOPMENTONLY, "time after being hit the spy blinks into view" );
//ConVar tf_spy_stealth_blink_scale( "tf_spy_stealth_blink_scale", "0.85", FCVAR_DEVELOPMENTONLY, "percentage visible scalar after being hit the spy blinks into view" );

ConVar tf_demoman_charge_drain_time( "tf_demoman_charge_drain_time", "1.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
ConVar tf_demoman_charge_regen_rate( "tf_demoman_charge_regen_rate", "8.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );

ConVar tf_scout_energydrink_consume_rate( "tf_scout_energydrink_consume_rate", "12.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
ConVar tf_scout_energydrink_regen_rate( "tf_scout_energydrink_regen_rate", "3.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );

ConVar tf_tournament_hide_domination_icons( "tf_tournament_hide_domination_icons", "0", FCVAR_REPLICATED, "Tournament mode server convar that forces clients to not display the domination icons above players dominating them." );

ConVar tf_damage_disablespread( "tf_damage_disablespread", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the random damage spread applied to all player damage." );
ConVar tf_always_loser( "tf_always_loser", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Force loserstate to true." );

ConVar sv_showimpacts( "sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point (1=both, 2=client-only, 3=server-only)" );
ConVar sv_showplayerhitboxes("sv_showplayerhitboxes", "0", FCVAR_REPLICATED, "Show lag compensated hitboxes for the specified player index whenever a player fires." );

ConVar tf2v_building_hauling( "tf2v_building_hauling", "1", FCVAR_REPLICATED, "Toggle Engineer's building hauling ability." );
ConVar tf2v_disable_player_shadows( "tf2v_disable_player_shadows", "0", FCVAR_REPLICATED, "Disables rendering of player shadows regardless of client's graphical settings." );
ConVar tf2v_critmod_range( "tf2v_critmod_range", "800", FCVAR_NOTIFY | FCVAR_REPLICATED, "Recent Damage (in HP) before peak critical chance levels off." );

ConVar tf2v_new_flame_damage( "tf2v_new_flame_damage", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables Jungle Inferno fire and afterburn damage calculations." );

ConVar tf2v_clamp_speed( "tf2v_clamp_speed", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable maximum speed bonuses a player can have at a single time." );
ConVar tf2v_clamp_speed_difference( "tf2v_clamp_speed_difference", "105", FCVAR_NOTIFY | FCVAR_REPLICATED, "Maxmimum speed a player can be boosted from their base speed in HU/s." );
ConVar tf2v_clamp_speed_absolute( "tf2v_clamp_speed_absolute", "450", FCVAR_NOTIFY | FCVAR_REPLICATED, "Absolute max speed (HU/s) for non-charging players." );

ConVar tf2v_use_new_spy_movespeeds( "tf2v_use_new_spy_movespeeds", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables the MYM era move speed for spies." );
ConVar tf2v_use_new_hauling_speed( "tf2v_use_new_hauling_speed", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables the F2P era movement decrease type for building hauling." );
ConVar tf2v_use_spy_moveattrib ("tf2v_use_spy_moveattrib", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Should spies be affected by their disguise's speed attributes?" );
ConVar tf2v_use_medic_speed_match( "tf2v_use_medic_speed_match", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables movespeed matching for medics." );
ConVar tf2v_allow_spy_sprint( "tf2v_allow_spy_sprint", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows spies to override their disguise speed by holding reload." );
ConVar tf2v_disguise_speed_match( "tf2v_disguise_speed_match", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows spies to always move at their disguised class' speed, including faster classes." );



ConVar tf2v_era_ammocounts("tf2v_era_ammocounts", "2", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables retail launch ammo pools for the Rocket Launcher, Grenade Launcher and Stickybomb Launcher." );

ConVar tf2v_use_new_minigun_aim_speed("tf2v_use_new_minigun_aim_speed", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Aims the minigun at 110HU/s compared to the original 80HU/s." );

ConVar tf_feign_death_duration( "tf_feign_death_duration", "6.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Time that feign death buffs last." );

ConVar tf_enable_grenades( "tf_enable_grenades", "0", FCVAR_REPLICATED, "Enable outfitting the grenade loadout slots" );

ConVar tf2v_allow_disguiseweapons( "tf2v_allow_disguiseweapons", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows spy to change disguise weapon using lastdisguise.", true, 0, true, 1);
ConVar tf2v_use_fast_redisguise( "tf2v_use_fast_redisguise", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Disguising while currently disguised is faster.", true, 0, true, 1);

ConVar tf2v_use_new_atomizer( "tf2v_use_new_atomizer", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Swaps between the old and modern Atomizer airdash mechanic.", true, 0, true, 1);
ConVar tf2v_use_new_sodapopper( "tf2v_use_new_sodapopper", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Swaps between minicrits versus five airdashes when under Soda Popper Hype.", true, 0, true, 1);

ConVar tf2v_use_new_short_circuit( "tf2v_use_new_short_circuit", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Swaps the Short Circuit's Secondary Attack.", true, 0, true, 1);

ConVar tf2v_use_new_cloak( "tf2v_use_new_cloak", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Gives cloaked spies a 20% damage resist and 25% shorter debuff duration.", true, 0, true, 1);

ConVar tf2v_use_new_cleaners( "tf2v_use_new_cleaners", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Swaps Minicrit on Kill for the CRIKEY meter.", true, 0, true, 1);

ConVar tf2v_use_new_beggars( "tf2v_use_new_beggars", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Deducts ammo from the Beggar's Bazooka when it misfires." );

#ifdef CLIENT_DLL
ConVar tf2v_enable_burning_death( "tf2v_enable_burning_death", "0", FCVAR_REPLICATED, "Enables an animation that plays sometimes when dying to fire damage.", true, 0.0f, true, 1.0f );
#endif

#ifdef GAME_DLL
extern ConVar tf2v_allow_cosmetics;
extern ConVar tf2v_randomizer;
extern ConVar tf2v_random_weapons;
#endif

ConVar tf2v_legacy_weapons( "tf2v_legacy_weapons", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Disables all new weapons as well as Econ Item System." );
ConVar tf2v_force_year_weapons( "tf2v_force_year_weapons", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Limit weapons based on year." );
ConVar tf2v_allowed_year_weapons( "tf2v_allowed_year_weapons", "2020", FCVAR_NOTIFY | FCVAR_REPLICATED, "Maximum year allowed for items." );

extern ConVar tf2v_assault_ctf_rules;

#define TF_SPY_STEALTH_BLINKTIME   0.3f
#define TF_SPY_STEALTH_BLINKSCALE  0.85f

#define TF_PLAYER_CONDITION_CONTEXT	"TFPlayerConditionContext"

#define MAX_DAMAGE_EVENTS		128

#define TF_BUFF_RADIUS			450.0f

const char *g_pszBDayGibs[22] =
{
	"models/effects/bday_gib01.mdl",
	"models/effects/bday_gib02.mdl",
	"models/effects/bday_gib03.mdl",
	"models/effects/bday_gib04.mdl",
	"models/player/gibs/gibs_balloon.mdl",
	"models/player/gibs/gibs_burger.mdl",
	"models/player/gibs/gibs_boot.mdl",
	"models/player/gibs/gibs_bolt.mdl",
	"models/player/gibs/gibs_can.mdl",
	"models/player/gibs/gibs_clock.mdl",
	"models/player/gibs/gibs_fish.mdl",
	"models/player/gibs/gibs_gear1.mdl",
	"models/player/gibs/gibs_gear2.mdl",
	"models/player/gibs/gibs_gear3.mdl",
	"models/player/gibs/gibs_gear4.mdl",
	"models/player/gibs/gibs_gear5.mdl",
	"models/player/gibs/gibs_hubcap.mdl",
	"models/player/gibs/gibs_licenseplate.mdl",
	"models/player/gibs/gibs_spring1.mdl",
	"models/player/gibs/gibs_spring2.mdl",
	"models/player/gibs/gibs_teeth.mdl",
	"models/player/gibs/gibs_tire.mdl"
};

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_ScriptCreatedItem )
#else
EXTERN_SEND_TABLE( DT_ScriptCreatedItem )
#endif
//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	RecvPropInt( RECVINFO( m_nDesiredDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDesiredDisguiseClass ) ),
	RecvPropTime( RECVINFO( m_flStealthNoAttackExpire ) ),
	RecvPropTime( RECVINFO( m_flStealthNextChangeTime ) ),
	RecvPropFloat(  RECVINFO( m_flCloakMeter ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominated ), RecvPropBool( RECVINFO( m_bPlayerDominated[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominatingMe ), RecvPropBool( RECVINFO( m_bPlayerDominatingMe[0] ) ) ),
	RecvPropInt( RECVINFO( m_iDesiredWeaponID ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nStreaks ), RecvPropInt( RECVINFO( m_nStreaks[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flCondExpireTimeLeft ), RecvPropFloat(RECVINFO( m_flCondExpireTimeLeft[0] ) ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx2 ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx3 ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx4 ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	RecvPropInt( RECVINFO( m_nPlayerCond ) ),
	RecvPropInt( RECVINFO( m_bJumping ) ),
	RecvPropInt( RECVINFO( m_nNumHealers ) ),
	RecvPropInt( RECVINFO( m_iCritMult ) ),
	RecvPropInt( RECVINFO( m_nAirDashCount ) ),
	RecvPropInt( RECVINFO( m_nAirDucked ) ),
	RecvPropInt( RECVINFO( m_nPlayerState ) ),
	RecvPropInt( RECVINFO( m_iDesiredPlayerClass ) ),
	RecvPropTime( RECVINFO( m_flStunExpireTime ) ),
	RecvPropInt( RECVINFO( m_nStunFlags ) ),
	RecvPropFloat( RECVINFO( m_flStunMovementSpeed ) ),
	RecvPropFloat( RECVINFO( m_flStunResistance ) ),
	RecvPropEHandle( RECVINFO( m_hStunner ) ),
	RecvPropInt( RECVINFO( m_iDecapitations ) ),
	RecvPropInt( RECVINFO( m_iHeadshots ) ),
	RecvPropInt( RECVINFO( m_iStrike ) ),
	RecvPropInt( RECVINFO( m_iKillstreak ) ),
	RecvPropInt( RECVINFO( m_iSapperKill ) ),
	RecvPropInt( RECVINFO( m_iRevengeCrits ) ),
	RecvPropInt( RECVINFO( m_iAirblastCrits ) ),
	RecvPropInt( RECVINFO( m_bShieldEquipped ) ),
	RecvPropInt( RECVINFO( m_iNextMeleeCrit ) ),
	RecvPropEHandle( RECVINFO( m_hCarriedObject ) ),
	RecvPropBool( RECVINFO( m_bCarryingObject ) ),
	RecvPropInt( RECVINFO( m_nTeamTeleporterUsed ) ),
	RecvPropBool( RECVINFO( m_bArenaSpectator ) ),
	RecvPropBool( RECVINFO( m_bGunslinger ) ),
	RecvPropInt( RECVINFO( m_iRespawnParticleID ) ),
	RecvPropInt( RECVINFO( m_iMaxHealth ) ),
	RecvPropFloat( RECVINFO( m_flEffectBarProgress ) ),
	RecvPropFloat( RECVINFO( m_flEnergyDrinkMeter ) ),
	RecvPropFloat( RECVINFO( m_flFocusLevel ) ),
	RecvPropFloat( RECVINFO( m_flChargeMeter ) ),
	RecvPropFloat( RECVINFO( m_flHypeMeter ) ),
	// Spy.
	RecvPropTime( RECVINFO( m_flInvisChangeCompleteTime ) ),
	RecvPropInt( RECVINFO( m_nDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDisguiseClass ) ),
	RecvPropInt( RECVINFO( m_nMaskClass ) ),
	RecvPropInt( RECVINFO( m_iDisguiseTargetIndex) ),
	RecvPropInt( RECVINFO( m_iDisguiseHealth ) ),
	RecvPropInt( RECVINFO( m_iDisguiseMaxHealth ) ),
	RecvPropFloat( RECVINFO( m_flDisguiseChargeLevel ) ),
	RecvPropInt( RECVINFO( m_iLeechHealth ) ),
	RecvPropDataTable( RECVINFO_DT( m_DisguiseItem ), 0, &REFERENCE_RECV_TABLE( DT_ScriptCreatedItem ) ),
	// Local Data.
	RecvPropDataTable( "tfsharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TFPlayerSharedLocal ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CTFPlayerShared )
	DEFINE_PRED_FIELD( m_nPlayerState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCond, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx3, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx4, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCloakMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nAirDashCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nAirDucked, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flInvisChangeCompleteTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iDesiredWeaponID, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iRespawnParticleID, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flEffectBarProgress, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flEnergyDrinkMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flFocusLevel, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flChargeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flHypeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	SendPropInt( SENDINFO( m_nDesiredDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDesiredDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flStealthNoAttackExpire ) ),
	SendPropTime( SENDINFO( m_flStealthNextChangeTime ) ),
	SendPropFloat( SENDINFO( m_flCloakMeter ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0, 100.0 ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominated ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominated ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominatingMe ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominatingMe ) ) ),
	SendPropInt( SENDINFO( m_iDesiredWeaponID ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nStreaks ), SendPropInt( SENDINFO_ARRAY( m_nStreaks ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flCondExpireTimeLeft ), SendPropFloat( SENDINFO_ARRAY( m_flCondExpireTimeLeft ) ) ),
	SendPropInt( SENDINFO( m_nPlayerCondEx ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerCondEx2 ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerCondEx3 ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerCondEx4 ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	SendPropInt( SENDINFO( m_nPlayerCond ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bJumping ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nNumHealers ), 5, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iCritMult ), 8, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nAirDashCount ), 8, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nAirDucked ), 2, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerState ), Q_log2( TF_STATE_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDesiredPlayerClass ), Q_log2( TF_CLASS_COUNT_ALL ) + 1, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flStunExpireTime ) ),
	SendPropInt( SENDINFO( m_nStunFlags ), -1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flStunMovementSpeed ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flStunResistance ), 0, SPROP_NOSCALE ),
	SendPropEHandle( SENDINFO( m_hStunner ) ),
	SendPropInt( SENDINFO( m_iDecapitations ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iHeadshots ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iStrike ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iKillstreak ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSapperKill ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iRevengeCrits ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iAirblastCrits ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bShieldEquipped ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iNextMeleeCrit ) ),
	SendPropEHandle( SENDINFO(m_hCarriedObject ) ),
	SendPropBool( SENDINFO( m_bCarryingObject ) ),
	SendPropInt( SENDINFO( m_nTeamTeleporterUsed ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bArenaSpectator ) ),
	SendPropBool( SENDINFO( m_bGunslinger ) ),
	SendPropInt( SENDINFO( m_iRespawnParticleID ), 0, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iMaxHealth ), 10 ),
	SendPropFloat( SENDINFO( m_flEffectBarProgress ), 11, 0, 0.0f, 100.0f ),
	SendPropFloat( SENDINFO( m_flEnergyDrinkMeter ), 11, 0, 0.0f, 100.0f ),
	SendPropFloat( SENDINFO( m_flFocusLevel ), 11, 0, 0.0f, 100.0f ),
	SendPropFloat( SENDINFO( m_flChargeMeter ), 11, 0, 0.0f, 100.0f ),
	SendPropFloat( SENDINFO( m_flHypeMeter ), 11, 0, 0.0f, 100.0f ),
	// Spy
	SendPropTime( SENDINFO( m_flInvisChangeCompleteTime ) ),
	SendPropInt( SENDINFO( m_nDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMaskClass ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDisguiseTargetIndex ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDisguiseHealth ), 10 ),
	SendPropInt( SENDINFO( m_iDisguiseMaxHealth ), 10 ),
	SendPropFloat( SENDINFO( m_flDisguiseChargeLevel ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_iLeechHealth ), 10 ),
	SendPropDataTable( SENDINFO_DT( m_DisguiseItem ), &REFERENCE_SEND_TABLE( DT_ScriptCreatedItem ) ),
	// Local Data.
	SendPropDataTable( "tfsharedlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFPlayerSharedLocal ), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()

#endif


// --------------------------------------------------------------------------------------------------- //
// Shared CTFPlayer implementation.
// --------------------------------------------------------------------------------------------------- //

// --------------------------------------------------------------------------------------------------- //
// CTFPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //

CTFPlayerShared::CTFPlayerShared()
{
	m_nPlayerState.Set( TF_STATE_WELCOME );
	m_bJumping = false;
	m_nAirDashCount = 0;
	m_flLastDashTime = 0;
	m_nAirDucked = 0;
	m_flStealthNoAttackExpire = 0.0f;
	m_flStealthNextChangeTime = 0.0f;
	m_iCritMult = 0;
	m_flInvisibility = 0.0f;

	m_iDesiredWeaponID = -1;
	m_iRespawnParticleID = 0;

	m_iStunPhase = 0;

	m_nTeamTeleporterUsed = TEAM_UNASSIGNED;

	m_bArenaSpectator = false;

	m_bGunslinger = false;

	// Reset our meters
	SetDecapitationCount( 0 );
	SetHeadshotCount( 0 );
	SetStrikeCount( 0 );
	SetSapperKillCount( 0 );
	SetRevengeCritCount( 0 );
	SetAirblastCritCount( 0 );
	SetHypeMeterAbsolute( 0 );
	SetSanguisugeHealth(0);
	SetKillstreakCount( 0 );
	SetFocusLevel( 0 );
	SetFireRageMeter(0);
	SetCrikeyMeter(0);

	m_flEnergyDrinkDrainRate = tf_scout_energydrink_consume_rate.GetFloat();
	m_flEnergyDrinkRegenRate = tf_scout_energydrink_regen_rate.GetFloat();
	
	m_bSpySprint = false;
	
	m_iWearableBodygroups = 0;
	m_iDisguiseBodygroups = 0;
	m_iWeaponBodygroup = 0;

#ifdef CLIENT_DLL
	m_iDisguiseWeaponModelIndex = -1;
	m_pDisguiseWeaponInfo = NULL;
	m_pCritSound = NULL;
	m_pCritEffect = NULL;
#else
	memset( m_flChargeOffTime, 0, sizeof( m_flChargeOffTime ) );
	memset( m_bChargeSounds, 0, sizeof( m_bChargeSounds ) );
#endif
}

void CTFPlayerShared::Init( CTFPlayer *pPlayer )
{
	m_pOuter = pPlayer;

	m_flNextBurningSound = 0;

	SetJumping( false );
}

//-----------------------------------------------------------------------------
// Purpose: Add a condition and duration
// duration of PERMANENT_CONDITION means infinite duration
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddCond( int nCond, float flDuration /* = PERMANENT_CONDITION */ )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );
	int nCondFlag = nCond;
	int *pVar = NULL;
	if ( nCond < 128 )
	{
		if ( nCond < 96 )
		{
			if ( nCond < 64 )
			{
				if ( nCond < 32 )
				{
					pVar = &m_nPlayerCond.GetForModify();
				}
				else
				{
					pVar = &m_nPlayerCondEx.GetForModify();
					nCondFlag -= 32;
				}
			}
			else
			{
				pVar = &m_nPlayerCondEx2.GetForModify();
				nCondFlag -= 64;
			}
		}
		else
		{
			pVar = &m_nPlayerCondEx3.GetForModify();
			nCondFlag -= 96;
		}
	}
	else
	{
		pVar = &m_nPlayerCondEx4.GetForModify();
		nCondFlag -= 128;
	}

	*pVar |= ( 1 << nCondFlag );
	m_flCondExpireTimeLeft.Set( nCond, flDuration );
	OnConditionAdded( nCond );
}

//-----------------------------------------------------------------------------
// Purpose: Forcibly remove a condition
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveCond(int nCond)
{
	Assert(nCond >= 0 && nCond < TF_COND_LAST);

	int nCondFlag = nCond;
	int *pVar = NULL;
	if ( nCond < 128 )
	{
		if ( nCond < 96 )
		{
			if ( nCond < 64 )
			{
				if ( nCond < 32 )
				{
					pVar = &m_nPlayerCond.GetForModify();
				}
				else
				{
					pVar = &m_nPlayerCondEx.GetForModify();
					nCondFlag -= 32;
				}
			}
			else
			{
				pVar = &m_nPlayerCondEx2.GetForModify();
				nCondFlag -= 64;
			}
		}
		else
		{
			pVar = &m_nPlayerCondEx3.GetForModify();
			nCondFlag -= 96;
		}
	}
	else
	{
		pVar = &m_nPlayerCondEx4.GetForModify();
		nCondFlag -= 128;
	}

	*pVar &= ~(1 << nCondFlag);
	m_flCondExpireTimeLeft.Set(nCond, 0);

	OnConditionRemoved(nCond);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCond(int nCond)
{
	Assert(nCond >= 0 && nCond < TF_COND_LAST);

	int nCondFlag = nCond;
	const int *pVar = NULL;
	if ( nCond < 128 )
	{
		if ( nCond < 96 )
		{
			if ( nCond < 64 )
			{
				if ( nCond < 32 )
				{
					pVar = &m_nPlayerCond.GetForModify();
				}
				else
				{
					pVar = &m_nPlayerCondEx.GetForModify();
					nCondFlag -= 32;
				}
			}
			else
			{
				pVar = &m_nPlayerCondEx2.GetForModify();
				nCondFlag -= 64;
			}
		}
		else
		{
			pVar = &m_nPlayerCondEx3.GetForModify();
			nCondFlag -= 96;
		}
	}
	else
	{
		pVar = &m_nPlayerCondEx4.GetForModify();
		nCondFlag -= 128;
	}

	return ((*pVar & (1 << nCondFlag)) != 0);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetConditionDuration( int nCond )
{
	Assert(nCond >= 0 && nCond < TF_COND_LAST);

	if (InCond(nCond))
	{
		return m_flCondExpireTimeLeft[nCond];
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsCritBoosted( void )
{
	// Oh man...
	if (InCond( TF_COND_CRITBOOSTED ) ||
		InCond( TF_COND_CRITBOOSTED_PUMPKIN ) ||
		InCond( TF_COND_CRITBOOSTED_USER_BUFF ) ||
		InCond( TF_COND_CRITBOOSTED_FIRST_BLOOD ) ||
		InCond( TF_COND_CRITBOOSTED_BONUS_TIME ) ||
		InCond( TF_COND_CRITBOOSTED_CTF_CAPTURE ) ||
		InCond( TF_COND_CRITBOOSTED_ON_KILL ) ||
		InCond( TF_COND_CRITBOOSTED_CARD_EFFECT ) ||
		InCond( TF_COND_CRITBOOSTED_RUNE_TEMP ) ||
		InCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON ) )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsMiniCritBoosted( void )
{
	if (InCond( TF_COND_OFFENSEBUFF ) ||
		InCond( TF_COND_ENERGY_BUFF ) ||
		InCond( TF_COND_BERSERK ) ||
		( InCond( TF_COND_SODAPOPPER_HYPE ) && !tf2v_use_new_sodapopper.GetBool() )||
		InCond( TF_COND_MINICRITBOOSTED_RAGE_BUFF ) ||
		InCond( TF_COND_MINICRITBOOSTED_ON_KILL ) )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsSpeedBoosted( void )
{
	if (InCond( TF_COND_SPEED_BOOST ) ||
		InCond( TF_COND_HALLOWEEN_SPEED_BOOST ))
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsInvulnerable( void )
{
	if (InCond( TF_COND_INVULNERABLE ) ||
		InCond( TF_COND_INVULNERABLE_CARD_EFFECT ) ||
		InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGE ))
		 return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsStealthed( void )
{
	if (InCond( TF_COND_STEALTHED ) ||
		InCond( TF_COND_STEALTHED_USER_BUFF ) ||
		InCond( TF_COND_STEALTHED_USER_BUFF_FADING ))
		 return true;
	return false;
}

void CTFPlayerShared::DebugPrintConditions(void)
{
#ifndef CLIENT_DLL
	const char *szDll = "Server";
#else
	const char *szDll = "Client";
#endif

	Msg("( %s ) Conditions for player ( %d )\n", szDll, m_pOuter->entindex());

	int i;
	int iNumFound = 0;
	for (i = 0; i<TF_COND_LAST; i++)
	{
		if (InCond(i))
		{
			if (m_flCondExpireTimeLeft[i] == PERMANENT_CONDITION)
			{
				Msg("( %s ) Condition %d - ( permanent cond )\n", szDll, i);
			}
			else
			{
				Msg("( %s ) Condition %d - ( %.1f left )\n", szDll, i, m_flCondExpireTimeLeft[i]);
			}

			iNumFound++;
		}
	}

	if (iNumFound == 0)
	{
		Msg("( %s ) No active conditions\n", szDll);
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnPreDataChanged(void)
{
	m_nOldConditions = m_nPlayerCond;
	m_nOldConditionsEx = m_nPlayerCondEx;
	m_nOldConditionsEx2 = m_nPlayerCondEx2;
	m_nOldConditionsEx3 = m_nPlayerCondEx3;
	m_nOldConditionsEx4 = m_nPlayerCondEx4;
	m_nOldDisguiseClass = GetDisguiseClass();
	m_nOldDisguiseTeam = GetDisguiseTeam();
	m_iOldDisguiseWeaponModelIndex = m_iDisguiseWeaponModelIndex;
	m_iOldDisguiseWeaponID = m_DisguiseItem.GetItemDefIndex();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDataChanged(void)
{
	// Update conditions from last network change
	SyncConditions(m_nPlayerCond, m_nOldConditions, 0, 0);
	SyncConditions(m_nPlayerCondEx, m_nOldConditionsEx, 0, 32);
	SyncConditions(m_nPlayerCondEx2, m_nOldConditionsEx2, 0, 64);
	SyncConditions(m_nPlayerCondEx3, m_nOldConditionsEx3, 0, 96);
	SyncConditions(m_nPlayerCondEx4, m_nOldConditionsEx4, 0, 128);

	m_nOldConditions = m_nPlayerCond;
	m_nOldConditionsEx = m_nPlayerCondEx;
	m_nOldConditionsEx2 = m_nPlayerCondEx2;
	m_nOldConditionsEx3 = m_nPlayerCondEx3;
	m_nOldConditionsEx4 = m_nPlayerCondEx4;

	if (m_nOldDisguiseClass != GetDisguiseClass() || m_nOldDisguiseTeam != GetDisguiseTeam())
	{
		OnDisguiseChanged();
	}

	if (m_iOldDisguiseWeaponID != m_DisguiseItem.GetItemDefIndex())
	{
		RecalcDisguiseWeapon();
	}

	if (m_iDisguiseWeaponModelIndex != m_iOldDisguiseWeaponModelIndex)
	{
		C_BaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();

		if (pWeapon)
		{
			pWeapon->SetModelIndex(pWeapon->GetWorldModelIndex());
		}
	}

	if (IsLoser())
	{
		C_BaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();
		if (pWeapon && !pWeapon->IsEffectActive(EF_NODRAW))
		{
			pWeapon->SetWeaponVisible(false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: check the newly networked conditions for changes
//-----------------------------------------------------------------------------
void CTFPlayerShared::SyncConditions(int nCond, int nOldCond, int nUnused, int iOffset)
{
	if (nCond == nOldCond)
		return;

	int nCondChanged = nCond ^ nOldCond;
	int nCondAdded = nCondChanged & nCond;
	int nCondRemoved = nCondChanged & nOldCond;

	int i;
	for (i = 0; i < 32; i++)
	{
		if ( nCondAdded & ( 1 << i ) )
		{
			OnConditionAdded( i + iOffset );
		}
		else if ( nCondRemoved & ( 1 << i ) )
		{
			OnConditionRemoved( i + iOffset );
		}
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Remove any conditions affecting players
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAllCond(CTFPlayer *pPlayer)
{
	int i;
	for (i = 0; i < TF_COND_LAST; i++)
	{
		if (InCond(i))
		{
			RemoveCond(i);
		}
	}

	// Now remove all the rest
	m_nPlayerCond = 0;
	m_nPlayerCondEx = 0;
	m_nPlayerCondEx2 = 0;
	m_nPlayerCondEx3 = 0;
	m_nPlayerCondEx4 = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we add the bit,
// and client when it recieves the new cond bits and finds one added
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionAdded(int nCond)
{
	switch (nCond)
	{
	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_STEALTHED:
		OnAddStealthed();
		break;

	case TF_COND_FEIGN_DEATH:
		OnAddFeignDeath();
		break;

	case TF_COND_INVULNERABLE:
		OnAddInvulnerable();
		break;
		
	case TF_COND_MEGAHEAL:
		OnAddMegaheal();
		break;

	case TF_COND_TELEPORTED:
		OnAddTeleported();
		break;

	case TF_COND_DISGUISING:
		OnAddDisguising();
		break;

	case TF_COND_DISGUISED:
		OnAddDisguised();
		break;

	case TF_COND_TAUNTING:
		OnAddTaunting();
		break;

	case TF_COND_SHIELD_CHARGE:
		OnAddShieldCharge();
		break;

	#ifdef CLIENT_DLL
	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
		m_pOuter->StopSound( "DemoCharge.ChargeCritOn" );
		m_pOuter->EmitSound( "DemoCharge.ChargeCritOn" );
		UpdateCritBoostEffect();
		break;
	#endif

	case TF_COND_CRITBOOSTED:
	case TF_COND_CRITBOOSTED_PUMPKIN:
	case TF_COND_CRITBOOSTED_USER_BUFF:
	case TF_COND_CRITBOOSTED_FIRST_BLOOD:
	case TF_COND_CRITBOOSTED_BONUS_TIME:
	case TF_COND_CRITBOOSTED_CTF_CAPTURE:
	case TF_COND_CRITBOOSTED_ON_KILL:
	case TF_COND_CRITBOOSTED_CARD_EFFECT:
	case TF_COND_CRITBOOSTED_RUNE_TEMP:
	case TF_COND_CRITBOOSTED_ACTIVEWEAPON:
		OnAddCritboosted();
		break;

	case TF_COND_BURNING:
		OnAddBurning();
		break;

	case TF_COND_HEALTH_OVERHEALED:
	case TF_COND_DISGUISE_HEALTH_OVERHEALED:
#ifdef CLIENT_DLL
		m_pOuter->UpdateOverhealEffect();
#endif
		break;

	case TF_COND_STUNNED:
		OnAddStunned();
		break;

	case TF_COND_DISGUISED_AS_DISPENSER:
	case TF_COND_BERSERK:
		m_pOuter->TeamFortress_SetSpeed();
		break;

	case TF_COND_HALLOWEEN_GIANT:
		OnAddHalloweenGiant();
		break;

	case TF_COND_HALLOWEEN_TINY:
		OnAddHalloweenTiny();
		break;

	case TF_COND_URINE:
		OnAddUrine();
		break;

#ifdef CLIENT_DLL
	case TF_COND_BLEEDING:
		if ( m_pOuter->IsLocalPlayer() )
		{
			IMaterial *pMaterial = materials->FindMaterial( "effects/bleed_overlay", TEXTURE_GROUP_CLIENT_EFFECTS, false );
			if ( !IsErrorMaterial( pMaterial ) )
			{
				view->SetScreenOverlayMaterial( pMaterial );
			}
		}
		break;
#endif

	case TF_COND_MAD_MILK:
		OnAddMadMilk();
		break;
		
	case TF_COND_GAS:
		OnAddGas();
		break;

	case TF_COND_PHASE:
		OnAddPhase();
		break;

	case TF_COND_SPEED_BOOST:
	case TF_COND_HALLOWEEN_SPEED_BOOST:
	case TF_COND_SPEED_BOOST_FEIGN:
		OnAddSpeedBoost();
		break;

	case TF_COND_OFFENSEBUFF:
	case TF_COND_DEFENSEBUFF:
	case TF_COND_REGENONDAMAGEBUFF:
	case TF_COND_RADIUSHEAL:
		OnAddBuff();
		break;

	case TF_COND_PURGATORY:
		OnAddInPurgatory();
		break;
		
	case TF_COND_MARKEDFORDEATH:
	case TF_COND_MARKEDFORDEATH_SELF:
		OnAddMarkedForDeath();
		break;

	case TF_COND_HALLOWEEN_THRILLER:
		OnAddHalloweenThriller();
		break;

	case TF_COND_HALLOWEEN_BOMB_HEAD:
		OnAddHalloweenBombHead();
		break;
		
	case TF_COND_MEDIGUN_SMALL_BULLET_RESIST:
	case TF_COND_MEDIGUN_SMALL_BLAST_RESIST:
	case TF_COND_MEDIGUN_SMALL_FIRE_RESIST:
		UpdateResistanceIcon();
		break;
	case TF_COND_MEDIGUN_UBER_BULLET_RESIST:
	case TF_COND_MEDIGUN_UBER_BLAST_RESIST:
	case TF_COND_MEDIGUN_UBER_FIRE_RESIST:	
		UpdateResistanceIcon();
		UpdateResistanceShield();
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we remove the bit,
// and client when it recieves the new cond bits and finds one removed
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionRemoved(int nCond)
{
	switch (nCond)
	{
	case TF_COND_ZOOMED:
		OnRemoveZoomed();
		break;

	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_STEALTHED:
		OnRemoveStealthed();
		break;

	case TF_COND_DISGUISED:
		OnRemoveDisguised();
		break;

	case TF_COND_DISGUISING:
		OnRemoveDisguising();
		break;

	case TF_COND_INVULNERABLE:
		OnRemoveInvulnerable();
		break;
		
	case TF_COND_MEGAHEAL:
		OnRemoveMegaheal();
		break;

	case TF_COND_TELEPORTED:
		OnRemoveTeleported();
		break;

	case TF_COND_TAUNTING:
		OnRemoveTaunting();
		break;

	case TF_COND_SHIELD_CHARGE:
		OnRemoveShieldCharge();
		break;

	#ifdef CLIENT_DLL
	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
		m_pOuter->StopSound( "DemoCharge.ChargeCritOn" );
		m_pOuter->EmitSound( "DemoCharge.ChargeCritOff" );
		UpdateCritBoostEffect();
		break;
	#endif

	case TF_COND_CRITBOOSTED:
	case TF_COND_CRITBOOSTED_PUMPKIN:
	case TF_COND_CRITBOOSTED_USER_BUFF:
	case TF_COND_CRITBOOSTED_FIRST_BLOOD:
	case TF_COND_CRITBOOSTED_BONUS_TIME:
	case TF_COND_CRITBOOSTED_CTF_CAPTURE:
	case TF_COND_CRITBOOSTED_ON_KILL:
	case TF_COND_CRITBOOSTED_CARD_EFFECT:
	case TF_COND_CRITBOOSTED_RUNE_TEMP:
	case TF_COND_CRITBOOSTED_ACTIVEWEAPON:
		OnRemoveCritboosted();
		break;

	case TF_COND_BURNING:
		OnRemoveBurning();
		break;

	case TF_COND_HEALTH_OVERHEALED:
	case TF_COND_DISGUISE_HEALTH_OVERHEALED:
#ifdef CLIENT_DLL
		m_pOuter->UpdateOverhealEffect();
#endif

	case TF_COND_DISGUISED_AS_DISPENSER:
	case TF_COND_BERSERK:	
		m_pOuter->TeamFortress_SetSpeed();
		break;

	case TF_COND_STUNNED:
		OnRemoveStunned();
		break;

	case TF_COND_HALLOWEEN_GIANT:
		OnRemoveHalloweenGiant();
		break;

	case TF_COND_HALLOWEEN_TINY:
		OnRemoveHalloweenTiny();
		break;

	case TF_COND_URINE:
		OnRemoveUrine();
		break;

#ifdef CLIENT_DLL
	case TF_COND_BLEEDING:
		if ( m_pOuter->IsLocalPlayer() )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
		break;
#endif

	case TF_COND_MAD_MILK:
		OnRemoveMadMilk();
		break;
		
	case TF_COND_GAS:
		OnRemoveGas();
		break;

	case TF_COND_PHASE:
		OnRemovePhase();
		break;

	case TF_COND_SPEED_BOOST:
	case TF_COND_HALLOWEEN_SPEED_BOOST:
	case TF_COND_SPEED_BOOST_FEIGN:
		OnRemoveSpeedBoost();
		break;
		
	case TF_COND_OFFENSEBUFF:
	case TF_COND_DEFENSEBUFF:
	case TF_COND_REGENONDAMAGEBUFF:
	case TF_COND_RADIUSHEAL:
		OnRemoveBuff();
		break;

	case TF_COND_PURGATORY:
		OnRemoveInPurgatory();
		break;
		
	case TF_COND_MARKEDFORDEATH:
	case TF_COND_MARKEDFORDEATH_SELF:
		OnRemoveMarkedForDeath();
		break;

	case TF_COND_HALLOWEEN_THRILLER:
		OnRemoveHalloweenThriller();
		break;

	case TF_COND_HALLOWEEN_BOMB_HEAD:
		OnRemoveHalloweenBombHead();
		break;

	case TF_COND_MEDIGUN_SMALL_BULLET_RESIST:
	case TF_COND_MEDIGUN_SMALL_BLAST_RESIST:
	case TF_COND_MEDIGUN_SMALL_FIRE_RESIST:
		UpdateResistanceIcon();
		break;
	case TF_COND_MEDIGUN_UBER_BULLET_RESIST:
	case TF_COND_MEDIGUN_UBER_BLAST_RESIST:
	case TF_COND_MEDIGUN_UBER_FIRE_RESIST:	
		UpdateResistanceIcon();
		UpdateResistanceShield();
		break;
	
	default:
		break;
	}
}

#ifdef GAME_DLL
void CTFPlayerShared::AddAttributeToPlayer( char const *szName, float flValue )
{
	CEconAttributeDefinition *pDefinition = GetItemSchema()->GetAttributeDefinitionByName( szName );
	if ( pDefinition )
		m_pOuter->GetAttributeList()->SetRuntimeAttributeValue( pDefinition, flValue );
}

void CTFPlayerShared::RemoveAttributeFromPlayer( char const *szName )
{
	CEconAttributeDefinition *pDefinition = GetItemSchema()->GetAttributeDefinitionByName( szName );
	m_pOuter->GetAttributeList()->RemoveAttribute( pDefinition );
}
#endif

int CTFPlayerShared::GetMaxBuffedHealth( void )
{
	float flBoostMax = m_pOuter->GetMaxHealthForBuffing() * tf_max_health_boost.GetFloat();

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	return iRoundDown;
}

int CTFPlayerShared::GetMaxHealth( void )
{
	return m_iMaxHealth;
}

int	CTFPlayerShared::GetDisguiseMaxHealth( void )
{
	return GetPlayerClassData( m_nDisguiseClass )->m_nMaxHealth;
}

int CTFPlayerShared::GetDisguiseMaxBuffedHealth( void )
{
	float flBoostMax = GetDisguiseMaxHealth() * tf_max_health_boost.GetFloat();

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	return iRoundDown;
}

//-----------------------------------------------------------------------------
// Purpose: Runs SERVER SIDE only Condition Think
// If a player needs something to be updated no matter what do it here (invul, etc).
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionGameRulesThink(void)
{
#ifdef GAME_DLL
	if ( m_flNextCritUpdate < gpGlobals->curtime )
	{
		UpdateCritMult();
		m_flNextCritUpdate = gpGlobals->curtime + 0.5;
	}

	int i;
	for (i = 0; i < TF_COND_LAST; i++)
	{
		if ( InCond( i ) )
		{
			// Ignore permanent conditions
			if ( m_flCondExpireTimeLeft[i] != PERMANENT_CONDITION )
			{
				float flReduction = gpGlobals->frametime;

				if ( ConditionExpiresFast( i ) )
				{
					// If we're being healed, we reduce bad conditions faster
					if ( m_aHealers.Count() > 0 )
					{
						if ( i == TF_COND_URINE || i == TF_COND_MAD_MILK || i == TF_COND_GAS  )
							flReduction *= m_aHealers.Count() + 1;
						else
							flReduction += ( m_aHealers.Count() * flReduction * 4 );
					}
				}

				m_flCondExpireTimeLeft.Set( i, max( m_flCondExpireTimeLeft[i] - flReduction, 0 ) );

				if ( m_flCondExpireTimeLeft[i] == 0 )
				{
					RemoveCond( i );
				}
			}
		}
	}
	
	
	// Our health will only decay ( from being medic buffed ) if we are not being healed by a medic
	// Dispensers can give us the TF_COND_HEALTH_BUFF, but will not maintain or give us health above 100%s
	bool bDecayHealth = true;
	
	
	// Get our overheal differences, and our base overheal.
	int iOverhealDifference = ( GetMaxBuffedHealth() - GetMaxHealth() );

	// If we're being healed, heal ourselves
	if ( InCond( TF_COND_HEALTH_BUFF ) )
	{
		// Start these off at 0 so we accept nerfs, but prefer buffs.
		float flMaxOverhealRatio = 0.0;
		float flOverhealAmount;
	
		// Heal faster if we haven't been in combat for a while
		float flTimeSinceDamage = gpGlobals->curtime - m_pOuter->GetLastDamageTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10, 15, 1.0, 3.0 );

		bool bHasFullHealth = m_pOuter->GetHealth() >= m_pOuter->GetMaxHealth();

		float fTotalHealAmount = 0.0f;
		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			// Dispensers refill cloak.
			if ( m_aHealers[i].bDispenserHeal )
			{
				AddToSpyCloakMeter( m_aHealers[i].flAmount * gpGlobals->frametime );
			}

			// Dispensers don't heal above 100%
			if ( bHasFullHealth && m_aHealers[i].bDispenserHeal )
			{
				continue;
			}
			
			// Being healed, don't decay our health
			bDecayHealth = false;
			
			// Dispensers heal at a constant rate
			if ( m_aHealers[i].bDispenserHeal )
			{
				// Dispensers heal at a slower rate, but ignore flScale
				m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount;
			}
			else	// We're being healed by a medic
			{
				// We're being healed by a medic
				flOverhealAmount = 1.0f;
				// Check our overheal level, and cap if necessary.
				if ( m_aHealers[i].pPlayer.IsValid() )
				{
					// Check the mult_medigun_overheal_amount attribute.
					CTFPlayer *pHealer = static_cast< CTFPlayer  *>( static_cast< CBaseEntity  *>( m_aHealers[i].pPlayer ) );

					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pHealer, flOverhealAmount, mult_medigun_overheal_amount);
				}
						
				// Iterate our overheal amount, if we're a higher value.
				if (flOverhealAmount > flMaxOverhealRatio)
					flMaxOverhealRatio = flOverhealAmount;
					
				// Check our healer's overheal attribute.
				if ( bHasFullHealth )
				{			
					// Calculate out the max health we can heal up to for the person.
					int iMaxOverheal = floor( ( iOverhealDifference * flOverhealAmount ) + GetMaxHealth() );
					// Don't heal if our health is above the overheal ratio.
					if ( m_pOuter->GetHealth() > iMaxOverheal )
						continue;
				}
					// Player heals are affected by the last damage time
					m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flScale;
			}

			fTotalHealAmount += m_aHealers[i].flAmount;
		}

		int nHealthToAdd = ( int )m_flHealFraction;
		if ( nHealthToAdd > 0 )
		{
			m_flHealFraction -= nHealthToAdd;

			// Overheal modifier attributes
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( ToTFPlayer(m_pOuter), flMaxOverhealRatio, mult_patient_overheal_penalty );
			CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
			if ( pWeapon )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flMaxOverhealRatio, mult_patient_overheal_penalty_active );
			}

			// Modify our max overheal.
			int iBoostMax;
			if ( flMaxOverhealRatio != 1.0f )
				iBoostMax = ( ( iOverhealDifference ) * flMaxOverhealRatio ) + GetMaxHealth();
			else
				iBoostMax = GetMaxBuffedHealth();

			if ( InCond( TF_COND_DISGUISED ) )
			{
				// Separate cap for disguised health
				//int iFakeBoostMax = GetDisguiseMaxBuffedHealth();
				//int nFakeHealthToAdd = clamp(nHealthToAdd, 0, iFakeBoostMax - m_iDisguiseHealth);
				//m_iDisguiseHealth += nFakeHealthToAdd;
				CTFPlayer *pDisguiseTarget = ToTFPlayer(GetDisguiseTarget());
				int nFakeHealthToAdd = nHealthToAdd;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pDisguiseTarget, nFakeHealthToAdd, mult_health_fromhealers );
				AddDisguiseHealth( nFakeHealthToAdd, true, flMaxOverhealRatio );
			}

			// Cap it to the max we'll boost a player's health
			CALL_ATTRIB_HOOK_INT_ON_OTHER( ToTFPlayer(m_pOuter), nHealthToAdd, mult_health_fromhealers );
			nHealthToAdd = clamp( nHealthToAdd, 0, ( iBoostMax - m_pOuter->GetHealth() ) );
			m_pOuter->TakeHealth( nHealthToAdd, DMG_IGNORE_MAXHEALTH );			
			

			// split up total healing based on the amount each healer contributes
			for ( int i = 0; i < m_aHealers.Count(); i++ )
			{
				if ( m_aHealers[i].pPlayer.IsValid() )
				{
					CTFPlayer *pPlayer = static_cast< CTFPlayer  *>( static_cast< CBaseEntity  *>( m_aHealers[i].pPlayer ) );
					if ( IsAlly( pPlayer ) )
					{
						CTF_GameStats.Event_PlayerHealedOther( pPlayer, nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount ) );
					}
					else
					{
						CTF_GameStats.Event_PlayerLeachedHealth( m_pOuter, m_aHealers[i].bDispenserHeal, nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount ) );
					}
				}
			}
		}

		const float flReduction = 2;	 // ( flReduction + 1 ) x faster reduction

		if ( InCond( TF_COND_BURNING ) )
		{
			// Reduce the duration of this burn 
			m_flFlameRemoveTime -= flReduction * gpGlobals->frametime;
		}

		if ( InCond( TF_COND_BLEEDING ) )
		{
			for ( int i=0; i<m_aBleeds.Count(); ++i )
			{
				bleed_struct_t *bleed = &m_aBleeds[i];
				bleed->m_flEndTime -= flReduction * gpGlobals->frametime;
			}
		}
	}

	if ( bDecayHealth )
	{
		// If we're not being buffed, our health drains back to our max
		if ( m_pOuter->GetHealth() > ( m_pOuter->GetMaxHealth() + m_pOuter->m_Shared.GetSanguisugeHealth() ) )
		{
			float flBoostMaxAmount = GetMaxBuffedHealth() - m_pOuter->GetMaxHealth();
			m_flHealFraction += ( gpGlobals->frametime * ( flBoostMaxAmount / ( tf_boost_drain_time.GetFloat() ) ) );
			int nHealthToDrain = ( int )m_flHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flHealFraction -= nHealthToDrain;

				// Manually subtract the health so we don't generate pain sounds / etc
				m_pOuter->m_iHealth -= nHealthToDrain;
			}
		}
		

		if ( InCond( TF_COND_DISGUISED ) && m_iDisguiseHealth > m_iDisguiseMaxHealth )
		{
			float flBoostMaxAmount = GetDisguiseMaxBuffedHealth() - m_iDisguiseMaxHealth;
			m_flDisguiseHealFraction += ( gpGlobals->frametime * ( flBoostMaxAmount / tf_boost_drain_time.GetFloat() ) );

			int nHealthToDrain = ( int )m_flDisguiseHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flDisguiseHealFraction -= nHealthToDrain;

				// Reduce our fake disguised health by roughly the same amount
				m_iDisguiseHealth -= nHealthToDrain;
			}
		}
	}

	// Overheal effects
	if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
	{
		if ( !InCond( TF_COND_HEALTH_OVERHEALED ) )
		{
			AddCond( TF_COND_HEALTH_OVERHEALED );
		}
	}
	else
	{
		if ( InCond( TF_COND_HEALTH_OVERHEALED ) )
		{
			RemoveCond( TF_COND_HEALTH_OVERHEALED );
		}
	}

	// Overheal effects for disguised spies
	if ( InCond( TF_COND_DISGUISED ) && m_iDisguiseHealth > m_iDisguiseMaxHealth )
	{
		if ( !InCond( TF_COND_DISGUISE_HEALTH_OVERHEALED ) )
		{
			AddCond( TF_COND_DISGUISE_HEALTH_OVERHEALED );
		}
	}
	else
	{
		if ( InCond( TF_COND_DISGUISE_HEALTH_OVERHEALED ) )
		{
			RemoveCond( TF_COND_DISGUISE_HEALTH_OVERHEALED );
		}
	}

	// Taunt
	if ( InCond( TF_COND_TAUNTING ) )
	{
		if ( gpGlobals->curtime > m_flTauntRemoveTime )
		{
			m_pOuter->ResetTauntHandle();

			//m_pOuter->SnapEyeAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetAbsAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetLocalAngles( m_pOuter->m_angTauntCamera );

			RemoveCond( TF_COND_TAUNTING );
		}
	}

	if ( InCond( TF_COND_BURNING ) )
	{
		// If we're underwater, put the fire out
		if ( gpGlobals->curtime > m_flFlameRemoveTime || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			RemoveCond( TF_COND_BURNING );
			if (InCond(TF_COND_BURNING_PYRO))
				RemoveCond(TF_COND_BURNING_PYRO);
		}
		else if ( ( gpGlobals->curtime >= m_flFlameBurnTime ) && ( ( TF_CLASS_PYRO != m_pOuter->GetPlayerClass()->GetClassIndex() ) || m_pOuter->m_Shared.InCond(TF_COND_BURNING_PYRO) ) )
		{
			float flBurnDamage = tf2v_new_flame_damage.GetBool() ? TF_BURNING_DMG_JI : TF_BURNING_DMG;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hBurnWeapon, flBurnDamage, mult_wpn_burndmg );
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hBurnAttacker->GetActiveTFWeapon(), flBurnDamage, mult_dmg_vs_burning ); // This attribute is based on being the active weapon for afterburn.

			// Burn the player (if not pyro, who does not take persistent burning damage)
			CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, m_hBurnWeapon, flBurnDamage, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_BURNING );
			m_pOuter->TakeDamage( info );
			m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
		}

		if ( m_flNextBurningSound < gpGlobals->curtime )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_ONFIRE );
			m_flNextBurningSound = gpGlobals->curtime + 2.5;
		}
	}

	if ( InCond( TF_COND_BLEEDING ) )
	{
		for ( int i = m_aBleeds.Count() - 1; i >= 0; --i )
		{
			bleed_struct_t *bleed = &m_aBleeds[i];
			if ( gpGlobals->curtime >= bleed->m_flEndTime )
			{
				m_aBleeds.FastRemove( i );
				continue;
			}
			else if ( gpGlobals->curtime >= bleed->m_flBleedTime )
			{
				bleed->m_flBleedTime = gpGlobals->curtime + TF_BLEEDING_FREQUENCY;

				CTakeDamageInfo info( bleed->m_hAttacker, bleed->m_hAttacker, bleed->m_hWeapon, (float)bleed->m_iDamage, DMG_SLASH, TF_DMG_CUSTOM_BLEEDING );
				m_pOuter->TakeDamage( info );
			}
		}

		if ( m_aBleeds.IsEmpty() )
			RemoveCond( TF_COND_BLEEDING );
	}

	if ( m_pOuter->GetWaterLevel() >= WL_Waist )
	{
		if ( InCond( TF_COND_URINE ) )
		{
			RemoveCond( TF_COND_URINE );
		}

		if ( InCond( TF_COND_MAD_MILK ) )
		{
			RemoveCond( TF_COND_MAD_MILK );
		}
		
		if ( InCond( TF_COND_GAS ) )
		{
			RemoveCond( TF_COND_GAS );
		}
	}

	if ( InCond(TF_COND_DISGUISING ) )
	{
		if ( gpGlobals->curtime > m_flDisguiseCompleteTime )
		{
			CompleteDisguise();
		}
	}

	// Stops the drain hack.
	if ( m_pOuter->IsPlayerClass( TF_CLASS_MEDIC ) || tf2v_randomizer.GetBool() )
	{
		CWeaponMedigun *pWeapon = m_pOuter->GetMedigun();
		if ( pWeapon && pWeapon->IsReleasingCharge() )
		{
			pWeapon->DrainCharge();
		}
	}

	TestAndExpireChargeEffect( TF_CHARGE_INVULNERABLE );
	TestAndExpireChargeEffect( TF_CHARGE_CRITBOOSTED );
	TestAndExpireChargeEffect( TF_CHARGE_MEGAHEAL );

	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		if (TF_SPY_STEALTH_BLINKTIME/*tf_spy_stealth_blink_time.GetFloat()*/ < ( gpGlobals->curtime - m_flLastStealthExposeTime ) )
		{
			RemoveCond( TF_COND_STEALTHED_BLINK );
		}
	}

	if ( InCond( TF_COND_STUNNED ) )
	{
		if ( gpGlobals->curtime > m_flStunExpireTime )
		{
			// Only check stun phase if we're unable to move
			if ( !( m_nStunFlags & TF_STUNFLAG_BONKSTUCK ) || m_iStunPhase == STUN_PHASE_END )
			{
				RemoveCond( TF_COND_STUNNED );
			}
		}
	}
	
	// Check to see if we should use the disguised class' speed, or our own.
	if ( ( InCond( TF_COND_DISGUISED ) && !InCond(TF_COND_STEALTHED) ) && tf2v_allow_spy_sprint.GetBool() )
	{
		// Override disguise speed when holding down the reload key
		if (m_pOuter->m_nButtons & IN_RELOAD)
		{
			// Check to see if this needs reloading first.
			CTFWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();
			if ((pWpn->Clip1() >= pWpn->GetMaxClip1()) || (m_pOuter->GetAmmoCount(pWpn->GetPrimaryAmmoType()) <= 0))
			{
				if ( !GetSpySprint() )
				{
					SetSpySprint(true);
					m_pOuter->TeamFortress_SetSpeed();

				}
			}
		}
		else	// Turn it off, use the standard disguise speed.
		{
			if ( GetSpySprint() )
			{
				SetSpySprint(false);
				m_pOuter->TeamFortress_SetSpeed();

			}			
			
		}
	}
	else if (!tf2v_allow_spy_sprint.GetBool() && GetSpySprint() ) // Don't allow us to spy sprint.
	{
		SetSpySprint(false);
		m_pOuter->TeamFortress_SetSpeed();
	}
	
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do CLIENT/SERVER SHARED condition thinks.
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionThink( void )
{
	if ( InCond( TF_COND_PHASE ) )
	{
		if ( gpGlobals->curtime > m_flPhaseTime )
		{
			UpdatePhaseEffects();

			// limit how often we can update in case of spam
			m_flPhaseTime = gpGlobals->curtime + 0.25f;
		}
	}

	UpdateRageBuffsAndRage();

#ifdef GAME_DLL
	UpdateCloakMeter();
	UpdateSanguisugeHealth();
	UpdateChargeMeter();
	UpdateEnergyDrinkMeter();
	UpdateFocusLevel();
	UpdateFireRage();
	UpdateCrikeyMeter();
#else
	m_pOuter->UpdateHalloweenBombHead();
#endif

#if defined(GAME_DLL)
	if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) )
	{
		CMerasmus *pMerasmus = assert_cast<CMerasmus *>( TFGameRules()->GetActiveBoss() );
		if ( pMerasmus && InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) && m_pOuter->IsAlive() )
		{
			Vector vecToMerasmus = m_pOuter->EyePosition() - pMerasmus->WorldSpaceCenter();
			if ( vecToMerasmus.LengthSqr() < Square( 100.0f ) )
				pMerasmus->AddStun( m_pOuter );
		}
	}
#endif
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ClientShieldChargeThink( void )
{
	if (m_bShieldChargeStopped && ( gpGlobals->curtime - m_flShieldChargeEndTime ) >= 0.3f)
	{
		RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );

		m_pOuter->StopSound( "DemoCharge.ChargeCritOn" );
		m_pOuter->EmitSound( "DemoCharge.ChargeCritOff" );

		UpdateCritBoostEffect();

		m_bShieldChargeStopped = false;
	}
	else if (m_flChargeMeter < 75.0f && InCond( TF_COND_SHIELD_CHARGE ))
	{
		AddCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ClientDemoBuffThink( void )
{
	if (m_iDecapitations > 0 && m_iDecapitations != m_iDecapitationsParity)
	{
		m_iDecapitationsParity = m_iDecapitations;
		m_pOuter->UpdateDemomanEyeEffect( m_iDecapitations );
	}
}

#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveZoomed(void)
{
#ifdef GAME_DLL
	m_pOuter->SetFOV(m_pOuter, 0, 0.1f);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguising(void)
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
	}

	if ( ( !m_pOuter->IsLocalPlayer() || !m_pOuter->InFirstPersonView() ) && ( !InCond( TF_COND_STEALTHED ) || !m_pOuter->IsEnemyPlayer() ) )
	{
		const char *pszEffectName = ConstructTeamParticle( "spy_start_disguise_%s", m_pOuter->GetTeamNumber() );

		m_pOuter->m_pDisguisingEffect = m_pOuter->ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
		m_pOuter->m_flDisguiseEffectStartTime = gpGlobals->curtime;
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: set up effects for when player finished disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguised(void)
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		// turn off disguising particles
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
	m_pOuter->m_flDisguiseEndEffectStartTime = gpGlobals->curtime;
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: start, end, and changing disguise classes
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDisguiseChanged( void )
{
	// recalc disguise model index
	//RecalcDisguiseWeapon();
	m_pOuter->UpdateRecentlyTeleportedEffect();
	UpdateCritBoostEffect();
	m_pOuter->UpdateOverhealEffect();
	m_pOuter->UpdateSpyMask();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddInvulnerable( void )
{
#ifndef CLIENT_DLL
	// Stock uber removes negative conditions.
	if ( InCond( TF_COND_BURNING ) )
	{
		RemoveCond( TF_COND_BURNING );
		if (InCond(TF_COND_BURNING_PYRO))
			RemoveCond(TF_COND_BURNING_PYRO);
	}

	if ( InCond( TF_COND_URINE ) )
	{
		RemoveCond( TF_COND_URINE );
	}

	if ( InCond( TF_COND_MAD_MILK ) )
	{
		RemoveCond( TF_COND_MAD_MILK );
	}
	
	if ( InCond( TF_COND_GAS ) )
	{
		RemoveCond( TF_COND_GAS );
	}
	
	if ( InCond( TF_COND_MARKEDFORDEATH ) )
	{
		RemoveCond( TF_COND_MARKEDFORDEATH );
	}
#else
	if ( m_pOuter->IsLocalPlayer() )
	{
		char *pEffectName = NULL;

		switch( m_pOuter->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		case TF_TEAM_RED:
			pEffectName =  "effects/invuln_overlay_red";
			break;
		default:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
		
		// We need to add the ubercharge effect to the Spy's mask.
		if ( InCond( TF_COND_DISGUISED ) )
		{	
			m_pOuter->UpdateSpyMask();
		}
	}
#endif
}

void CTFPlayerShared::OnAddMegaheal( void )
{
#ifndef CLIENT_DLL
	
#else
	if ( m_pOuter->IsLocalPlayer() )
	{
		char *pEffectName = NULL;

		switch( m_pOuter->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		case TF_TEAM_RED:
			pEffectName =  "effects/invuln_overlay_red";
			break;
		default:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInvulnerable(void)
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
	// We need to revert the mask back to the regular disguise class.
	if ( InCond( TF_COND_DISGUISED ) )
	{	
		m_pOuter->UpdateSpyMask();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMegaheal(void)
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#endif
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::ShouldShowRecentlyTeleported( void )
{
	if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( InCond( TF_COND_STEALTHED ) )
			return false;

		if ( InCond( TF_COND_DISGUISED ) && ( m_pOuter->IsLocalPlayer() || !m_pOuter->InSameTeam( C_TFPlayer::GetLocalTFPlayer() ) ) )
		{
			if ( GetDisguiseTeam() != m_nTeamTeleporterUsed )
				return false;
		}
		else if ( m_pOuter->GetTeamNumber() != m_nTeamTeleporterUsed )
			return false;
	}

	return ( InCond( TF_COND_TELEPORTED ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTeleported(void)
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTeleported(void)
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTaunting(void)
{
	CTFWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();
	if (pWpn)
	{
		// cancel any reload in progress.
		pWpn->AbortReload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTaunting(void)
{
#ifdef GAME_DLL
	m_pOuter->ClearTauntAttack();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStunned(void)
{
	if ( IsControlStunned() || IsLoser() )
	{
		RemoveCond( TF_COND_SHIELD_CHARGE );

		CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
		if ( pWeapon )
		{
			pWeapon->OnControlStunned();
		}

		m_pOuter->TeamFortress_SetSpeed();
	}

	// Check if effects are disabled
	if ( !( m_nStunFlags & TF_STUNFLAG_NOSOUNDOREFFECT ) )
	{
#ifdef CLIENT_DLL
		if ( !m_pStun )
		{
			if ( m_nStunFlags & TF_STUNFLAG_BONKEFFECT )
			{
				// Half stun
				m_pStun = m_pOuter->ParticleProp()->Create( "conc_stars", PATTACH_POINT_FOLLOW, "head" );
			}
			else if ( m_nStunFlags & TF_STUNFLAG_GHOSTEFFECT )
			{
				// Ghost stun
				m_pStun = m_pOuter->ParticleProp()->Create( "yikes_fx", PATTACH_POINT_FOLLOW, "head" );
			}
		}
#else
		if ( ( m_nStunFlags & TF_STUNFLAG_GHOSTEFFECT ) )
		{
			// Play the scream sound
			m_pOuter->EmitSound( "Halloween.PlayerScream" );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStunned(void)
{
	m_flStunExpireTime = 0.0f;
	m_flStunMovementSpeed = 0.0f;
	m_flStunResistance = 0.0f;
	m_hStunner = NULL;
	m_iStunPhase = STUN_PHASE_NONE;

	m_pOuter->TeamFortress_SetSpeed();

	CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();

	if ( pWeapon )
	{
		pWeapon->SetWeaponVisible( true );
	}

#ifdef CLIENT_DLL
	m_pOuter->ParticleProp()->StopEmission( m_pStun );
	m_pStun = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddSlowed(void)
{
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: Remove slowdown effect
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveSlowed(void)
{
	// Set speed back to normal
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddShieldCharge( void )
{
	m_pOuter->TeamFortress_SetSpeed();

	UpdatePhaseEffects();

#ifdef CLIENT_DLL
	// Start screaming if we're a Demoman.
	if ( m_pOuter->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		m_pOuter->EmitSound( "DemoCharge.Charging" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveShieldCharge( void )
{
	m_pOuter->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	for (int i = 0; i < m_pPhaseTrails.Count(); i++)
	{
		UTIL_Remove( m_pPhaseTrails[i] );
	}
	m_pPhaseTrails.RemoveAll();
#else
	m_pOuter->ParticleProp()->StopEmission( m_pWarp );
	m_pWarp = NULL;

	m_bShieldChargeStopped = true;
	m_flShieldChargeEndTime = gpGlobals->curtime;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddCritboosted(void)
{
#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenGiant(void)
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale(2.0, 0.0);
	m_pOuter->SetHealth(m_pOuter->GetMaxHealth());
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenGiant(void)
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale(1.0, 0.0);
	m_pOuter->SetHealth(m_pOuter->GetMaxHealth());
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenTiny(void)
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale(0.5, 0.0);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenTiny(void)
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale(1.0, 0.0);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveCritboosted(void)
{
#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif
}

void CTFPlayerShared::OnAddSpeedBoost( void )
{
#ifdef GAME_DLL
 	CSingleUserRecipientFilter filter( m_pOuter );
 	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "DisciplineDevice.PowerUp" );
#endif
	m_pOuter->TeamFortress_SetSpeed();
	UpdateSpeedBoostEffects();
}
//-----------------------------------------------------------------------------
 // Purpose: 
 //-----------------------------------------------------------------------------
 void CTFPlayerShared::OnRemoveSpeedBoost( void )
 {
 #ifdef GAME_DLL
 	CSingleUserRecipientFilter filter( m_pOuter );
 	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "DisciplineDevice.PowerDown" );
#endif
	m_pOuter->TeamFortress_SetSpeed();
	UpdateSpeedBoostEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddUrine(void)
{
#ifdef GAME_DLL
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_JARATE_HIT );
#else
	m_pOuter->ParticleProp()->Create( "peejar_drips", PATTACH_ABSORIGIN_FOLLOW );

	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( "effects/jarate_overlay", TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveUrine(void)
{
#ifdef GAME_DLL
	if( m_nPlayerState != TF_STATE_DYING )
	{
		m_hUrineAttacker = NULL;
	}
#else
	m_pOuter->ParticleProp()->StopParticlesNamed( "peejar_drips" );

	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#endif
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMadMilk( void )
{
#ifdef GAME_DLL
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_JARATE_HIT );
#else
	m_pOuter->ParticleProp()->Create( "peejar_drips_milk", PATTACH_ABSORIGIN_FOLLOW );

	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( "effects/milk_screen", TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMadMilk( void )
{
#ifdef GAME_DLL
	if ( m_nPlayerState != TF_STATE_DYING )
	{
		m_hMilkAttacker = NULL;
	}
#else
	m_pOuter->ParticleProp()->StopParticlesNamed( "peejar_drips_milk" );

	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#endif
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddGas( void )
{
#ifdef GAME_DLL
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_JARATE_HIT );
#else
	m_pOuter->ParticleProp()->Create( "peejar_drips_gas", PATTACH_ABSORIGIN_FOLLOW );

	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( "effects/gas_overlay", TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveGas( void )
{
#ifdef GAME_DLL
	if ( m_nPlayerState != TF_STATE_DYING )
	{
		m_hMilkAttacker = NULL;
	}
#else
	m_pOuter->ParticleProp()->StopParticlesNamed( "peejar_drips_gas" );

	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddPhase(void)
{
#ifdef GAME_DLL
	m_pOuter->DropFlag();
#endif
	UpdatePhaseEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemovePhase(void)
{
#ifdef GAME_DLL
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_TIRED );

	for ( int i = 0; i < m_pPhaseTrails.Count(); i++ )
	{
		m_pPhaseTrails[i]->SUB_Remove();
	}
	m_pPhaseTrails.RemoveAll();
#else
	m_pOuter->ParticleProp()->StopEmission( m_pWarp );
	m_pWarp = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBuff( void )
{
#ifdef CLIENT_DLL
	// Start the buff effect
	if ( !m_pBuffAura )
	{
		const char *pszEffectName = ConstructTeamParticle( "soldierbuff_%s_buffed", m_pOuter->GetTeamNumber() );

		m_pBuffAura = m_pOuter->ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBuff( void )
{
#ifdef CLIENT_DLL
	if ( m_pBuffAura )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pBuffAura );
		m_pBuffAura = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddInPurgatory( void )
{
#ifdef GAME_DLL
	m_pOuter->SetHealth( GetMaxHealth() );
	m_pOuter->RemoveOwnedProjectiles();
	AddCond( TF_COND_INVULNERABLE, 1.5f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Apply effects when player escapes the underworld
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInPurgatory( void )
{
#ifdef GAME_DLL
	if ( m_nPlayerState != TF_STATE_DYING )
	{
		AddCond( TF_COND_INVULNERABLE, 10.0f );
		AddCond( TF_COND_SPEED_BOOST, 10.0f );
		AddCond( TF_COND_CRITBOOSTED_PUMPKIN, 10.0f );
		m_pOuter->m_purgatoryDuration.Start( 10.0f );

		m_pOuter->SetHealth( GetMaxHealth() );

		m_pOuter->RemoveOwnedProjectiles();

		// Write to chat that player has escaped the underworld
		CReliableBroadcastRecipientFilter filter;
		UTIL_SayText2Filter( filter, m_pOuter, false, TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) ? "#TF_Halloween_Skull_Island_Escape" : "#TF_Halloween_Underworld", m_pOuter->GetPlayerName() );
		TFGameRules()->BroadcastSound( 255, "Halloween.PlayerEscapedUnderworld" );

		// Let the map know we escaped the underworld
		IGameEvent *event = gameeventmanager->CreateEvent( "escaped_loot_island" );
		if ( event )
		{
			event->SetInt( "player", m_pOuter->GetUserID() );
			gameeventmanager->FireEvent( event, true );
		}

		CTeam *pTeam = m_pOuter->GetTeam();
		if ( pTeam )
		{
			UTIL_LogPrintf( "HALLOWEEN: \"%s<%i><%s><%s>\" %s\n", m_pOuter->GetPlayerName(), m_pOuter->GetUserID(), m_pOuter->GetNetworkIDString(), pTeam->GetName(), "purgatory_escaped" );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Appled when the player is Marked for Death
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMarkedForDeath( void )
{
#ifdef CLIENT_DLL
	if ( !m_pOuter->m_Shared.InCond(TF_COND_STEALTHED) && !m_pOuter->m_Shared.InCond(TF_COND_STEALTHED_BLINK) )
	{
		// Start the Marked for Death icon
		if (!m_pMarkedIcon )
			m_pMarkedIcon = m_pOuter->ParticleProp()->Create( "mark_for_death", PATTACH_POINT_FOLLOW, "head" );
	}
	else
	{
		// We're cloaked, don't give our position away.
		if (m_pMarkedIcon )
		{
			m_pOuter->ParticleProp()->StopEmission( m_pMarkedIcon );
			m_pMarkedIcon = NULL;
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Remove Marked for Death effects
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMarkedForDeath( void )
{
#ifdef CLIENT_DLL
	// Destroy the Marked for Death icon.
	if ( m_pMarkedIcon )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pMarkedIcon );
		m_pMarkedIcon = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Appled on resistance changes.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateResistanceIcon(void)
{
#ifdef CLIENT_DLL
	bool bShouldUpdate = false;
	int nCurrentResistance = 0;

	// Hide the icon when cloaked.
	if (m_pOuter->m_Shared.InCond(TF_COND_STEALTHED) || m_pOuter->m_Shared.InCond(TF_COND_STEALTHED_BLINK))
	{
		if (m_pResistanceIcon)
		{
			m_pOuter->ParticleProp()->StopEmission(m_pResistanceIcon);
			m_pResistanceIcon = NULL;
		}
		return;
	}

	if (m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_SMALL_BULLET_RESIST) || m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST))
		nCurrentResistance = 3;
	else if (m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_SMALL_BLAST_RESIST) || m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST))
		nCurrentResistance = 4;
	else if (m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_SMALL_FIRE_RESIST) || m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST))
		nCurrentResistance = 5;

	// Different icon, update.
	if (m_nCurrentResistanceIcon != nCurrentResistance)
		bShouldUpdate = true;

	// Check if we should change team icon.
	int iVisibleTeam = m_pOuter->GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
	if (m_pOuter->m_Shared.InCond(TF_COND_DISGUISED) && m_pOuter->IsEnemyPlayer())
		iVisibleTeam = m_pOuter->m_Shared.GetDisguiseTeam();

	if (iVisibleTeam != m_nResistanceIconTeam)
		bShouldUpdate = true;

	// Generate a new icon
	if (bShouldUpdate)
	{
		// No buff, turn off our resistance icon.
		if (!nCurrentResistance && m_pResistanceIcon)
		{
			m_pOuter->ParticleProp()->StopEmission(m_pResistanceIcon);
			m_pResistanceIcon = NULL;
			m_nResistanceIconTeam = 0;
			m_nCurrentResistanceIcon = 0;
			return;
		}

		// Kill the current icon.
		if (m_pResistanceIcon)
		{
			m_pOuter->ParticleProp()->StopEmission(m_pResistanceIcon);
			m_pResistanceIcon = NULL;
		}

		// Generate icon.
		const char* pszEffect = nullptr;
		if (iVisibleTeam == TF_TEAM_RED)
		{
			switch (nCurrentResistance)
			{
			case 3:
				pszEffect = "vaccinator_red_buff1";
				break;

			case 4:
				pszEffect = "vaccinator_red_buff2";
				break;

			case 5:
				pszEffect = "vaccinator_red_buff3";
				break;
			}
		}
		else
		{
			switch (nCurrentResistance)
			{
			case 3:
				pszEffect = "vaccinator_blue_buff1";
				break;

			case 4:
				pszEffect = "vaccinator_blue_buff2";
				break;

			case 5:
				pszEffect = "vaccinator_blue_buff3";
				break;
			}
		}

		if (pszEffect[0] == '\0')
		{
			m_pResistanceIcon = m_pOuter->ParticleProp()->Create(pszEffect, PATTACH_POINT_FOLLOW, "head");
			m_nCurrentResistanceIcon = nCurrentResistance;
			m_nResistanceIconTeam = iVisibleTeam;
		}
		return;
	}
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Appled on shield add/remove.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateResistanceShield(void)
{
#ifdef CLIENT_DLL
	bool bDisplayShield = false;

	// Hide the shield when cloaked.
	if (m_pOuter->m_Shared.InCond(TF_COND_STEALTHED) || m_pOuter->m_Shared.InCond(TF_COND_STEALTHED_BLINK))
	{
		if (m_pResistanceShield)
		{
			m_pResistanceShield->Remove();
			m_pResistanceShield = NULL;
			m_nResistanceShieldTeam = -1;
		}
		return;
	}

	// Display a shield if we have a "big" resist.
	if (m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST))
		bDisplayShield = true;
	else if (m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST))
		bDisplayShield = true;
	else if (m_pOuter->m_Shared.InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST))
		bDisplayShield = true;


	// Not supposed to have a shield, remove it and bail.
	if (!bDisplayShield && m_pResistanceShield)
	{
		m_pResistanceShield->Remove();
		m_pResistanceShield = NULL;
		m_nResistanceShieldTeam = -1;
		return;
	}

	bool bRefreshShield = false;
	// Check if we should change team icon.
	int iVisibleTeam = m_pOuter->GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
	if (m_pOuter->m_Shared.InCond(TF_COND_DISGUISED) && m_pOuter->IsEnemyPlayer())
		iVisibleTeam = m_pOuter->m_Shared.GetDisguiseTeam();

	if (iVisibleTeam != m_nResistanceShieldTeam)
		bRefreshShield = true;

	// Generate a new shield
	if (bRefreshShield)
	{
		if (m_pResistanceShield)
		{
			m_pResistanceShield->Remove();
			m_pResistanceShield = NULL;
		}

		m_pResistanceShield = new C_BaseAnimating(); 
		if (m_pResistanceShield)
		{
			m_pResistanceShield->SetModel("models/effects/resist_shield/resist_shield.mdl");
			m_pResistanceShield->SetAbsOrigin(m_pOuter->GetAbsOrigin());
			m_pResistanceShield->ChangeTeam(iVisibleTeam);
			m_pResistanceShield->m_nSkin = (iVisibleTeam) ? 0 : 1;
			m_pResistanceShield->SetModelScale(m_pOuter->GetModelScale());
		}

		m_nResistanceShieldTeam = iVisibleTeam;
	}
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenThriller( void )
{
#if defined( CLIENT_DLL )
	if ( m_pOuter == C_TFPlayer::GetLocalTFPlayer() )
	{
		m_pOuter->EmitSound( "Halloween.dance_howl" );
		m_pOuter->EmitSound( "Halloween.dance_loop" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenThriller( void )
{
#if defined( GAME_DLL )
	StopHealing( m_pOuter );
#else
	if ( m_pOuter == C_TFPlayer::GetLocalTFPlayer() )
		m_pOuter->StopSound( "Halloween.dance_loop" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenBombHead( void )
{
#ifdef GAME_DLL
	if ( InCond( TF_COND_HALLOWEEN_KART ) )
		RemoveAttributeFromPlayer( "head scale" );
#else
	m_pOuter->UpdateHalloweenBombHead();
	m_pOuter->CreateBombinomiconHint();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenBombHead( void )
{
#ifdef GAME_DLL
	if ( InCond( TF_COND_HALLOWEEN_KART ) )
		AddAttributeToPlayer( "head scale", 3.0 );

	if ( m_pOuter->IsAlive() )
	{
		Vector vecOrigin = m_pOuter->GetAbsOrigin();
		m_pOuter->BombHeadExplode( false );

		if ( TFGameRules()->State_Get() != GR_STATE_PREROUND )
			TFGameRules()->PushAllPlayersAway( vecOrigin, 150.0, 350.0, TEAM_ANY, NULL );

		CPVSFilter filter( vecOrigin );
		TE_TFParticleEffect( filter, 0, "bombinomicon_burningdebris", vecOrigin, vec3_angle );
	}
#else
	m_pOuter->UpdateHalloweenBombHead();
	m_pOuter->DestroyBombinomiconHint();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculatePlayerBodygroups(void)
{
	//CTFWeaponBase::UpdateWeaponBodyGroups((CTFWeaponBase *)v4, 0);
	//CEconWearable::UpdateWearableBodyGroups(*((CEconWearable **)this + 521));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::Burn( CBaseCombatCharacter *pAttacker, float flFlameDuration )
{
	Burn( m_pOuter, NULL, flFlameDuration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::Burn( CTFPlayer *pAttacker, CTFWeaponBase *pWeapon /*= NULL*/, float flFlameDuration )
{
#ifdef CLIENT_DLL

#else
	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	// pyros don't burn persistently or take persistent burning damage, but we show brief burn effect so attacker can tell they hit
	bool bVictimIsPyro = ( TF_CLASS_PYRO ==  m_pOuter->GetPlayerClass()->GetClassIndex() );
	bool bVictimIsGassed = m_pOuter->m_Shared.InCond(TF_COND_GAS);
	int nVictimIsFlameProof = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(ToTFPlayer(m_pOuter), nVictimIsFlameProof, set_fire_retardant);

	if (nVictimIsFlameProof == 0 && !m_pOuter->m_Shared.InCond(TF_COND_AFTERBURN_IMMUNE))
	{
		if (!InCond(TF_COND_BURNING))
		{
			AddCond(TF_COND_BURNING);
				
			m_flFlameBurnTime = gpGlobals->curtime;	//asap
			// let the attacker know he burned me
			if (pAttacker && ( !bVictimIsPyro || bVictimIsGassed ) )
			{
				pAttacker->OnBurnOther(m_pOuter);
			}
			if (tf2v_new_flame_damage.GetBool())	// Start keeping track of flame stacks.
				m_flFlameStack = 0;
				
			//Pyros don't usually burn, but we need to check if we should burn a pyro that is gassed.
			if (bVictimIsPyro && bVictimIsGassed)
			{
				if (!m_pOuter->m_Shared.InCond(TF_COND_BURNING_PYRO))
					m_pOuter->m_Shared.AddCond(TF_COND_BURNING_PYRO);
			}
				
		}
		
		bool bBurningPyro = m_pOuter->m_Shared.InCond(TF_COND_BURNING_PYRO);

		if (bVictimIsGassed)	// Character is gassed, do a long burn.
		{
			m_flFlameLife = TF_BURNING_FLAME_LIFE_MAX_JI; // 10 seconds.

			if ( flFlameDuration != -1.0f  )
				m_flFlameLife = flFlameDuration;
		}
		else if (tf2v_new_flame_damage.GetBool())	// Jungle Inferno Calculations
		{
			CTFWeaponBase *pWeapon = pAttacker->GetActiveTFWeapon(); // Check the weapon we're using to calculate afterburn.
			if ( ( bVictimIsPyro || bVictimIsGassed) && ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER ) ) // If we have a Flamethrower, stack our afterburn from 4-10 seconds.
			{
				m_flFlameStack += 1;	// Add a flame to our stack.
				float flFlameLifeAdjusted = Min( (TF_BURNING_FLAME_LIFE_MIN_JI + (0.4 * (m_flFlameStack - 1) ) ), TF_BURNING_FLAME_LIFE_MAX_JI ); // Duration is 4 + ( 0.4 * (n - 1 ) ) seconds, where n is flame thrower hits.
				if ( m_flFlameRemoveTime && ( ( flFlameLifeAdjusted + gpGlobals->curtime ) < m_flFlameRemoveTime ) ) // If we're less than the original remove time, increase the duration.
					flFlameLifeAdjusted = ( m_flFlameRemoveTime - gpGlobals->curtime ); // Reset the afterburn time to the longer value.
				
				if ( flFlameDuration != -1.0f  )
					m_flFlameLife = flFlameDuration;
				else
					m_flFlameLife = flFlameLifeAdjusted;
					
			}
			else // Something other than a flame thrower, or we're attacking a pyro.
			{
				m_flFlameLife = ( bVictimIsPyro && !bBurningPyro ) ? TF_BURNING_FLAME_LIFE_PYRO : TF_BURNING_FLAME_LIFE_JI;

				if ( m_flFlameRemoveTime && ( ( m_flFlameLife + gpGlobals->curtime ) < m_flFlameRemoveTime ) ) // If less than original remove time, increase duration.
				{
					m_flFlameLife = ( m_flFlameRemoveTime - gpGlobals->curtime ); // Reset the afterburn time to the longer value.
				}
				
				if ( flFlameDuration != -1.0f  )
					m_flFlameLife = flFlameDuration;
			}
		}
		else	// Original Flame Calculations
		{
			m_flFlameLife = ( bVictimIsPyro && !bBurningPyro ) ? TF_BURNING_FLAME_LIFE_PYRO : TF_BURNING_FLAME_LIFE;

			if ( flFlameDuration != -1.0f  )
				m_flFlameLife = flFlameDuration;
		}
		
		if (bVictimIsGassed)	// If hit by gas, remove the gas.
			m_pOuter->m_Shared.RemoveCond(TF_COND_GAS);

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pAttacker, m_flFlameLife, mult_wpn_burntime);
			
		m_flFlameRemoveTime = gpGlobals->curtime + m_flFlameLife;
		m_hBurnAttacker = pAttacker;
		m_hBurnWeapon = pWeapon;
	}
	else // Don't burn players with fire retardant items.
	{
		if (InCond(TF_COND_BURNING))
		{
			// If we're on fire, stop burning and prevent us from being burnt.
			RemoveCond(TF_COND_BURNING);
			if (InCond(TF_COND_BURNING_PYRO))
				RemoveCond(TF_COND_BURNING_PYRO);
		}
	}

#endif
}

void CTFPlayerShared::StunPlayer( float flDuration, float flSpeed, float flResistance, int nStunFlags, CTFPlayer *pStunner )
{
	float flNextStunExpireTime = Max( m_flStunExpireTime.Get(), gpGlobals->curtime + flDuration );
	m_hStunner = pStunner;
	m_nStunFlags = nStunFlags;
	m_flStunMovementSpeed = flSpeed;
	m_flStunResistance = flResistance;

	if ( m_flStunExpireTime < flNextStunExpireTime )
	{
		AddCond( TF_COND_STUNNED );
		m_flStunExpireTime = flNextStunExpireTime;

#ifdef GAME_DLL
		if( !( m_nStunFlags & TF_STUNFLAG_THIRDPERSON ) )
			m_pOuter->PlayStunSound( m_hStunner, m_nStunFlags /*, current stun flags*/ );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::MakeBleed( CTFPlayer *pAttacker, CTFWeaponBase *pWeapon, float flBleedDuration, int iDamage )
{
#ifdef GAME_DLL
	if (m_pOuter->IsAlive() && ( pAttacker || pWeapon ))
	{
		float flEndAt = gpGlobals->curtime + flBleedDuration;
		for (int i=0; i<m_aBleeds.Count(); ++i)
		{
			bleed_struct_t *bleed = &m_aBleeds[i];
			if (bleed->m_hAttacker == pAttacker && bleed->m_hWeapon == pWeapon)
			{
				bleed->m_flEndTime = flEndAt;

				if (!InCond( TF_COND_BLEEDING ))
					AddCond( TF_COND_BLEEDING );

				return;
			}
		}

		bleed_struct_t bleed = {
			pAttacker,
			pWeapon,
			flBleedDuration,
			flEndAt,
			iDamage
		};
		m_aBleeds.AddToTail( bleed );

		if (!InCond( TF_COND_BLEEDING ))
			AddCond( TF_COND_BLEEDING );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsControlStunned( void )
{
	if (InCond( TF_COND_STUNNED ) && ( m_nStunFlags & TF_STUNFLAG_BONKSTUCK ) != 0)
		return true;
	return false;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Bonk phase effects
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddPhaseEffects( void )
{
	CTFPlayer *pPlayer = m_pOuter;
	if ( !pPlayer)
		return;


	// TODO: Clean this up a bit more
	const char* pszEffect = m_pOuter->GetTeamNumber() == TF_TEAM_BLUE ? "effects/beam001_blu.vmt" : "effects/beam001_red.vmt";
	Vector vecOrigin = pPlayer->GetAbsOrigin();
	
	CSpriteTrail *pPhaseTrail = CSpriteTrail::SpriteTrailCreate( pszEffect, vecOrigin, true );
	pPhaseTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, 0 );
	pPhaseTrail->SetStartWidth( 12.0f );
	pPhaseTrail->SetTextureResolution( 0.01416667 );
	pPhaseTrail->SetLifeTime( 1.0 );
	pPhaseTrail->SetAttachment( pPlayer, pPlayer->LookupAttachment( "back_upper" ) );
	m_pPhaseTrails.AddToTail( pPhaseTrail );

	pPhaseTrail = CSpriteTrail::SpriteTrailCreate( pszEffect, vecOrigin, true );
	pPhaseTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, 0 );
	pPhaseTrail->SetStartWidth( 16.0f );
	pPhaseTrail->SetTextureResolution( 0.01416667 );
	pPhaseTrail->SetLifeTime( 1.0 );
	pPhaseTrail->SetAttachment( pPlayer, pPlayer->LookupAttachment( "back_lower" ) );
	m_pPhaseTrails.AddToTail( pPhaseTrail );

	// White trail for socks
	pPhaseTrail = CSpriteTrail::SpriteTrailCreate( "effects/beam001_white.vmt", vecOrigin, true );
	pPhaseTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, 0 );
	pPhaseTrail->SetStartWidth( 8.0f );
	pPhaseTrail->SetTextureResolution( 0.01416667 );
	pPhaseTrail->SetLifeTime( 0.5 );
	pPhaseTrail->SetAttachment( pPlayer, pPlayer->LookupAttachment( "foot_R" ) );
	m_pPhaseTrails.AddToTail( pPhaseTrail );

	pPhaseTrail = CSpriteTrail::SpriteTrailCreate( "effects/beam001_white.vmt", vecOrigin, true );
	pPhaseTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, 0 );
	pPhaseTrail->SetStartWidth( 8.0f );
	pPhaseTrail->SetTextureResolution( 0.01416667 );
	pPhaseTrail->SetLifeTime( 0.5 );
	pPhaseTrail->SetAttachment( pPlayer, pPlayer->LookupAttachment( "foot_L" ) );
	m_pPhaseTrails.AddToTail( pPhaseTrail );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Update phase effects
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdatePhaseEffects(void)
{
	if ( !InCond( TF_COND_PHASE ) && !InCond( TF_COND_SHIELD_CHARGE ) )
	{
		return;
	}

#ifdef CLIENT_DLL
	if(  m_pOuter->GetAbsVelocity() != vec3_origin )
	{
		// We're on the move
		if( m_pWarp )
		{
			m_pOuter->ParticleProp()->StopEmission( m_pWarp );
			m_pWarp = NULL;
		}
	}
	else
	{
		// We're not moving
		if ( !m_pWarp )
		{
			m_pWarp = m_pOuter->ParticleProp()->Create( "warp_version", PATTACH_ABSORIGIN_FOLLOW );
		}
	}
#else
	if ( m_pPhaseTrails.IsEmpty() )
	{
		AddPhaseEffects();
	}
		
	// Turn on the trails if they're not active already
	if ( m_pPhaseTrails[0] && !m_pPhaseTrails[0]->IsOn() )
	{
		for( int i = 0; i < m_pPhaseTrails.Count(); i++ )
		{
			m_pPhaseTrails[i]->TurnOn();
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Update speedboost effects
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateSpeedBoostEffects(void)
{
	if ( !IsSpeedBoosted() )
	{
#ifdef CLIENT_DLL
		if( m_pSpeedTrails )
		{
			m_pOuter->ParticleProp()->StopEmission( m_pSpeedTrails );
			m_pSpeedTrails = NULL;
		}
#endif
		return;
	}

#ifdef CLIENT_DLL
	if(  m_pOuter->GetAbsVelocity() != vec3_origin )
	{
		// We're on the move
		if ( !m_pSpeedTrails )
		{
			m_pSpeedTrails = m_pOuter->ParticleProp()->Create( "speed_boost_trail", PATTACH_ABSORIGIN_FOLLOW );
		}
	}
	else
	{
		// We're not moving
		if( m_pSpeedTrails )
		{
			m_pOuter->ParticleProp()->StopEmission( m_pSpeedTrails );
			m_pSpeedTrails = NULL;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBurning(void)
{
#ifdef CLIENT_DLL
	m_pOuter->StopBurningSound();

	if ( m_pOuter->m_pBurningEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pBurningEffect );
		m_pOuter->m_pBurningEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}

	m_pOuter->m_flBurnEffectStartTime = 0;
	m_pOuter->m_flBurnEffectEndTime = 0;
#else
	m_hBurnAttacker = NULL;
	m_hBurnWeapon = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStealthed(void)
{
#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "Player.Spy_Cloak" );
	UpdateCritBoostEffect();
	m_pOuter->UpdateOverhealEffect();
	m_pOuter->RemoveAllDecals();
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif

	if ( InCond( TF_COND_FEIGN_DEATH ) )
	{
		m_flInvisChangeCompleteTime = gpGlobals->curtime;
	}
	else
	{
		float flCloakRate = tf_spy_invis_time.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flCloakRate, mult_cloak_rate );
		m_flInvisChangeCompleteTime = gpGlobals->curtime + flCloakRate;
	}

	// set our offhand weapon to be the invis weapon
	for (int i = 0; i < m_pOuter->WeaponCount(); i++)
	{
		CTFWeaponBase *pWpn = (CTFWeaponBase *)m_pOuter->GetWeapon(i);
		if (!pWpn)
			continue;

		if (pWpn->GetWeaponID() != TF_WEAPON_INVIS)
			continue;

		// try to switch to this weapon
		m_pOuter->SetOffHandWeapon( pWpn );
		break;
	}

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddFeignDeath( void )
{
	if ( !IsStealthed() )
		AddCond( TF_COND_STEALTHED );

	m_bFeignDeathReady = false;
	m_bFeigningDeath = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStealthed(void)
{
#ifdef CLIENT_DLL
	C_TFWeaponInvis *pInvis = dynamic_cast<C_TFWeaponInvis *>( m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
	int nQuietUncloak = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, nQuietUncloak, set_quiet_unstealth );
	if ( nQuietUncloak != 0 )
	{
		m_pOuter->EmitSound( "Player.Spy_UnCloakReduced" );
	}
	else if ( pInvis && pInvis->HasFeignDeath() )
	{
		m_pOuter->EmitSound( "Player.Spy_UnCloakFeignDeath" );
	}
	else
	{
		m_pOuter->EmitSound( "Player.Spy_UnCloak" );
	}

	UpdateCritBoostEffect();
	m_pOuter->UpdateOverhealEffect();
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif

	if ( InCond( TF_COND_FEIGN_DEATH ) )
	{
		RemoveCond( TF_COND_FEIGN_DEATH );
		if ( m_flCloakMeter > 40.0f )
			m_flCloakMeter = 40.0f;

		m_bFeigningDeath = false;
	}

	m_pOuter->HolsterOffHandWeapon();

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguising(void)
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
#else
	// PistonMiner: Removed the reset as we need this for later.

	//m_nDesiredDisguiseTeam = TF_SPY_UNDEFINED;

	// Do not reset this value, we use the last desired disguise class for the
	// 'lastdisguise' command

	//m_nDesiredDisguiseClass = TF_CLASS_UNDEFINED;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguised(void)
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsEnemyPlayer() )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( TF_CLASS_SPY );
		int iIndex = modelinfo->GetModelIndex( pData->GetModelName() );

		m_pOuter->SetModelIndex( iIndex );
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

	// They may have called for medic and created a visible medic bubble
	m_pOuter->ParticleProp()->StopParticlesNamed( "speech_mediccall", true );

	UpdateCritBoostEffect();
	m_pOuter->UpdateOverhealEffect();

#else
	m_nDisguiseTeam = TF_SPY_UNDEFINED;
	m_nDisguiseClass.Set(TF_CLASS_UNDEFINED);
	m_nMaskClass = TF_CLASS_UNDEFINED;
	m_hDisguiseTarget.Set(NULL);
	m_iDisguiseTargetIndex = TF_DISGUISE_TARGET_INDEX_NONE;
	m_iDisguiseHealth = 0;
	m_iDisguiseMaxHealth = 0;
	m_flDisguiseChargeLevel = 0.0f;
	m_DisguiseItem.SetItemDefIndex( -1 );

	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->TeamFortress_SetSpeed();

	m_pOuter->ClearExpression();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBurning(void)
{
#ifdef CLIENT_DLL
	// Start the burning effect
	if ( !m_pOuter->m_pBurningEffect )
	{
		const char *pszEffectName;
		if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) )
			pszEffectName = ConstructTeamParticle( "burningplayer_rainbow_%s", m_pOuter->GetTeamNumber(), true );
		else
			pszEffectName = ConstructTeamParticle( "burningplayer_%s", m_pOuter->GetTeamNumber(), true );

		m_pOuter->m_pBurningEffect = m_pOuter->ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	
	if (m_flFlameBurnTime)
		m_pOuter->m_flBurnEffectStartTime = m_flFlameBurnTime;
	else	// Failsafe in case we can't call the time for some reason.
		m_pOuter->m_flBurnEffectStartTime = gpGlobals->curtime;
			
	if (m_flFlameRemoveTime)
		m_pOuter->m_flBurnEffectEndTime = m_flFlameRemoveTime;
	else	// Failsafe in case we can't call the time for some reason.	
		m_pOuter->m_flBurnEffectEndTime = tf2v_new_flame_damage.GetBool() ? (gpGlobals->curtime + TF_BURNING_FLAME_LIFE_MAX_JI )  : (gpGlobals->curtime + TF_BURNING_FLAME_LIFE ) ;


	}
	// set the burning screen overlay
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( "effects/imcookin", TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif

	/*
	#ifdef GAME_DLL

	if ( player == robin || player == cook )
	{
	CSingleUserRecipientFilter filter( m_pOuter );
	TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_SPECIAL );
	}

	#endif
	*/

	// play a fire-starting sound
	m_pOuter->EmitSound("Fire.Engulf");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetStealthNoAttackExpireTime(void)
{
	return m_flStealthNoAttackExpire;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominated(CTFPlayer *pPlayer, bool bDominated)
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominated.Set(iPlayerIndex, bDominated);
	pPlayer->m_Shared.SetPlayerDominatingMe(m_pOuter, bDominated);
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is being dominated by the other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominatingMe(CTFPlayer *pPlayer, bool bDominated)
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominatingMe.Set(iPlayerIndex, bDominated);
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Gets the number of players currently dominated
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDominationCount( void )
{
	int iDominationCount = 0;
	for (int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++)
	{
		if (IsPlayerDominated(playerIndex))
		{
			iDominationCount++;
		}
	}
	return iDominationCount;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominated(int iPlayerIndex)
{
#ifdef CLIENT_DLL
	// On the client, we only have data for the local player.
	// As a result, it's only valid to ask for dominations related to the local player
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	Assert( m_pOuter->IsLocalPlayer() || pLocalPlayer->entindex() == iPlayerIndex );

	if ( m_pOuter->IsLocalPlayer() )
		return m_bPlayerDominated.Get( iPlayerIndex );

	return pLocalPlayer->m_Shared.IsPlayerDominatingMe( m_pOuter->entindex() );
#else
	// Server has all the data.
	return m_bPlayerDominated.Get( iPlayerIndex );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominatingMe( int iPlayerIndex )
{
	return m_bPlayerDominatingMe.Get( iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::NoteLastDamageTime( int nDamage )
{
	// we took damage
	if ( nDamage > 5 && !InCond(TF_COND_BLINK_IMMUNE) )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnSpyTouchedByEnemy( void )
{
	if ( !InCond(TF_COND_BLINK_IMMUNE) )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::FadeInvis( float flInvisFadeTime )
{
	RemoveCond( TF_COND_STEALTHED );

	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flInvisFadeTime, mult_decloak_rate );

	if ( flInvisFadeTime < 0.15 )
	{
		// this was a force respawn, they can attack whenever
	}
	else
	{
		// next attack in some time
		m_flStealthNoAttackExpire = gpGlobals->curtime + tf_spy_cloak_no_attack_time.GetFloat();
	}

	m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisFadeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Approach our desired level of invisibility
//-----------------------------------------------------------------------------
void CTFPlayerShared::InvisibilityThink( void )
{
	float flTargetInvis = 0.0f;
	float flTargetInvisScale = 1.0f;
	if ( InCond( TF_COND_STEALTHED_BLINK ) || InCond( TF_COND_URINE ) || InCond( TF_COND_MAD_MILK ) || InCond(TF_COND_GAS) )
	{
		// We were bumped into or hit for some damage.
		flTargetInvisScale = TF_SPY_STEALTH_BLINKSCALE;/*tf_spy_stealth_blink_scale.GetFloat();*/
	}

	// Go invisible or appear.
	if ( m_flInvisChangeCompleteTime > gpGlobals->curtime )
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f - ( m_flInvisChangeCompleteTime - gpGlobals->curtime );
		}
		else
		{
			flTargetInvis = ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) * 0.5f;
		}
	}
	else
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f;

			if ( m_bHasMotionCloak && m_flCloakMeter == 0.0f )
			{
				float flSpeed = m_pOuter->GetAbsVelocity().LengthSqr();
				float flMaxSpeed = Square( m_pOuter->MaxSpeed() );
				if ( flMaxSpeed == 0.0f )
				{
					if (flSpeed >= 0.0f)
						flTargetInvis *= 0.5f;
				}
				else
				{
					flTargetInvis *= 1.0f + ( ( flSpeed * -0.5f ) / flMaxSpeed );
				}
			}
		}
		else
		{
			flTargetInvis = 0.0f;
		}
	}

	flTargetInvis *= flTargetInvisScale;
	m_flInvisibility = Clamp( flTargetInvis, 0.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// Purpose: How invisible is the player [0..1]
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetPercentInvisible(void)
{
	return m_flInvisibility;
}

//-----------------------------------------------------------------------------
// Purpose: Start the process of disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::Disguise( int nTeam, int nClass, CTFPlayer *pTarget, bool b1 )
{
#ifndef CLIENT_DLL
	int nRealTeam = m_pOuter->GetTeamNumber();
	int nRealClass = m_pOuter->GetPlayerClass()->GetClassIndex();

	Assert( ( nClass >= TF_CLASS_SCOUT ) && ( nClass <= TF_CLASS_ENGINEER ) );

	// we're not a spy
	if ( nRealClass != TF_CLASS_SPY )
	{
		return;
	}

	// Can't disguise while taunting.
	if ( InCond( TF_COND_TAUNTING ) || InCond( TF_COND_STUNNED ) )
	{
		return;
	}

	// we're not disguising as anything but ourselves (so reset everything)
	if ( nRealTeam == nTeam && nRealClass == nClass )
	{
		RemoveDisguise();
		return;
	}

	// Ignore disguise of the same type, switch disguise weapon instead.
	if ( nTeam == m_nDisguiseTeam && nClass == m_nDisguiseClass && !b1 )
	{
		CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
		if ( tf2v_allow_disguiseweapons.GetBool() ) // Disguise to whatever slot we're currently holding.
			RecalcDisguiseWeapon( pWeapon ? pWeapon->GetSlot() : 0 );
		else // Force the primary weapon.
			RecalcDisguiseWeapon( 0 );
		return;
	}

	// invalid team
	if ( nTeam <= TEAM_SPECTATOR || nTeam >= TF_TEAM_COUNT )
	{
		return;
	}

	// invalid class
	if ( nClass < TF_FIRST_NORMAL_CLASS || nClass > TF_LAST_NORMAL_CLASS )
	{
		return;
	}

	m_hForcedDisguise = pTarget;

	m_nDesiredDisguiseClass = nClass;
	m_nDesiredDisguiseTeam = nTeam;

	AddCond( TF_COND_DISGUISING );

	// Start the think to complete our disguise
	float flDisguiseTime = gpGlobals->curtime + TF_TIME_TO_DISGUISE;

	// Switching disguises is faster if we're already disguised (and we're using modern disguise times)
	if ( InCond(TF_COND_DISGUISED ) && tf2v_use_fast_redisguise.GetBool() )
		flDisguiseTime = gpGlobals->curtime + TF_TIME_TO_CHANGE_DISGUISE;
	
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, flDisguiseTime, disguise_speed_penalty );

	m_flDisguiseCompleteTime = pTarget ? 0.0f : flDisguiseTime;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Set our target with a player we've found to emulate
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
void CTFPlayerShared::FindDisguiseTarget(void)
{
	m_hDisguiseTarget = m_pOuter->TeamFortress_GetDisguiseTarget( m_nDisguiseTeam, m_nDisguiseClass );

	if ( m_hForcedDisguise )
	{
		m_hDisguiseTarget = m_hForcedDisguise.Get();
		m_hForcedDisguise = nullptr;
	}

	if ( m_hDisguiseTarget )
	{
		m_iDisguiseTargetIndex.Set( m_hDisguiseTarget->entindex() );
		Assert( m_iDisguiseTargetIndex >= 1 && m_iDisguiseTargetIndex <= MAX_PLAYERS );
	}
	else
	{
		m_iDisguiseTargetIndex.Set( TF_DISGUISE_TARGET_INDEX_NONE );
	}
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Complete our disguise
//-----------------------------------------------------------------------------
void CTFPlayerShared::CompleteDisguise(void)
{
#ifndef CLIENT_DLL
	AddCond(TF_COND_DISGUISED);

	m_nDisguiseClass = m_nDesiredDisguiseClass;
	m_nDisguiseTeam = m_nDesiredDisguiseTeam;

	RemoveCond(TF_COND_DISGUISING);

	FindDisguiseTarget();

	CTFPlayer *pDisguiseTarget = ToTFPlayer(GetDisguiseTarget());

	// If we have a disguise target with matching class then take their values.
	// Otherwise, generate random health and uber.
	if ( pDisguiseTarget && pDisguiseTarget->IsPlayerClass( m_nDisguiseClass ) )
	{
		int iMaxHealth = pDisguiseTarget->GetMaxHealth();

		if ( pDisguiseTarget->IsAlive() )
			m_iDisguiseHealth = pDisguiseTarget->GetHealth();

		else
			m_iDisguiseHealth = random->RandomInt( iMaxHealth / 2, iMaxHealth );
		m_iDisguiseMaxHealth = iMaxHealth;


		if (m_nDisguiseClass == TF_CLASS_MEDIC)
		{
			m_flDisguiseChargeLevel = pDisguiseTarget->MedicGetChargeLevel();
		}
	}

	else
	{
		int iMaxHealth = GetPlayerClassData(m_nDisguiseClass)->m_nMaxHealth;
		m_iDisguiseHealth = random->RandomInt(iMaxHealth / 2, iMaxHealth);
		m_iDisguiseMaxHealth = iMaxHealth;

		if (m_nDisguiseClass == TF_CLASS_MEDIC)
		{
			m_flDisguiseChargeLevel = random->RandomFloat(0.0f, 0.99f);
		}
	}

	if (m_nDisguiseClass == TF_CLASS_SPY)
	{
		m_nMaskClass = random->RandomInt(TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS);
	}
	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->TeamFortress_SetSpeed();

	m_pOuter->ClearExpression();

	m_DisguiseItem.SetItemDefIndex( -1 );

	RecalcDisguiseWeapon();
	CalculateDisguiseWearables();
	
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetDisguiseHealth(int iDisguiseHealth)
{
	m_iDisguiseHealth = iDisguiseHealth;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::AddDisguiseHealth(int iHealthToAdd, bool bOverheal /*= false*/, float flOverhealAmount /*=1.0*/)
{
	Assert(InCond(TF_COND_DISGUISED));

	int iMaxHealth;
	if (!bOverheal)
		iMaxHealth = GetDisguiseMaxHealth();
	else
	{
		if ( flOverhealAmount != 1.0f )
			iMaxHealth = ( ( GetDisguiseMaxBuffedHealth() - GetDisguiseMaxHealth() ) * flOverhealAmount ) + GetDisguiseMaxHealth();
		else
			iMaxHealth = GetDisguiseMaxBuffedHealth();
	}
		

	iHealthToAdd = clamp(iHealthToAdd, 0, iMaxHealth - m_iDisguiseHealth);
	if (iHealthToAdd <= 0)
		return 0;

	m_iDisguiseHealth += iHealthToAdd;

	return iHealthToAdd;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveDisguise(void)
{
#ifdef CLIENT_DLL


#else
	RemoveCond(TF_COND_DISGUISED);
	RemoveCond(TF_COND_DISGUISING);

	CalculateDisguiseWearables();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::CalculateDisguiseWearables(void)
{
#if defined ( USES_ECON_ITEMS ) || defined ( TF_VINTAGE )

	// Remove our current disguise wearables.
	for (int i = 0; i < m_pOuter->GetNumDisguiseWearables(); i++)
	{
		CEconWearable *pWearable =  m_pOuter->GetDisguiseWearable(i);
		if ( !pWearable )
			continue;

		m_pOuter->RemoveDisguiseWearable(pWearable);
	}
	
	// Purge all of our disguise bodygroups.
	SetDisguiseBodygroups(0);
	
	if (InCond(TF_COND_DISGUISED))
	{
		// Get our disguise target.
		CTFPlayer *pDisguiseTarget = ToTFPlayer(GetDisguiseTarget());
		if (pDisguiseTarget)
		{
			// Get the wearables that they have.
			for (int i = 0; i < pDisguiseTarget->GetNumWearables(); i++)
			{
				CTFWearable *pWearable = dynamic_cast<CTFWearable *>(pDisguiseTarget->GetWearable(i));
				if (pWearable == nullptr)
					continue;

				// Add this wearable to my list of disguise wearables.
				m_pOuter->EquipDisguiseWearable(pWearable);
				
			}
		}
		
		// Update the disguise bodygroups as well.
		SetDisguiseBodygroups(pDisguiseTarget->m_Shared.GetWearableBodygroups());
	}
	
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalcDisguiseWeapon(int iSlot /*= 0*/)
{
	// If we're using legacy weapons, calculate the disguise weapon using that logic instead.
	if (tf2v_legacy_weapons.GetBool() || (tf2v_force_year_weapons.GetBool() && tf2v_allowed_year_weapons.GetInt() <= 2007))
	{
#ifdef CLIENT_DLL
		RecalcDisguiseWeaponLegacy();
#endif
		return;
	}
#ifndef CLIENT_DLL
	// IMPORTANT!!! - This whole function will need to be rewritten if we switch to using item schema.
	// So please remind me about this when we do. (Nicknine)
	if ( !InCond( TF_COND_DISGUISED ) )
	{
		m_DisguiseItem.SetItemDefIndex( -1 );
		return;
	}

	Assert(m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY);

	CEconItemView *pDisguiseItem = NULL;
	CTFPlayer *pDisguiseTarget = ToTFPlayer(GetDisguiseTarget());

	// Find the weapon in the same slot
	for ( int i = 0; i < TF_PLAYER_WEAPON_COUNT; i++ )	
	{
			
		// Use disguise target's weapons if possible.
		CEconItemView *pItem = NULL;

		if ( pDisguiseTarget && pDisguiseTarget->IsPlayerClass( m_nDisguiseClass ) )
			pItem = pDisguiseTarget->GetLoadoutItem( m_nDisguiseClass, i );
		else
			pItem = GetTFInventory()->GetItem( m_nDisguiseClass, i, 0 );

		if ( !pItem )
			continue;

			CTFWeaponInfo *pWeaponInfo = GetTFWeaponInfoForItem( pItem->GetItemDefIndex(), m_nDisguiseClass );
		if ( pWeaponInfo && pWeaponInfo->iSlot == iSlot )
			{
				pDisguiseItem = pItem;
				break;
			}
	}

	// Can't find the disguise item in this loadout, cycle the next one.
	// We always have at least one of these slots available for the player to use.
	if (pDisguiseItem == NULL)
	{
		switch (iSlot)
		{
			case TF_LOADOUT_SLOT_PRIMARY:
				return RecalcDisguiseWeapon(TF_LOADOUT_SLOT_SECONDARY);			
				break;			
			case TF_LOADOUT_SLOT_SECONDARY:
				return RecalcDisguiseWeapon(TF_LOADOUT_SLOT_MELEE);			
				break;		
			case TF_LOADOUT_SLOT_MELEE:
				return RecalcDisguiseWeapon(TF_LOADOUT_SLOT_PRIMARY);	
				break;
		}
	}
	
	// Don't switch to builder as it's too complicated.
	if ( pDisguiseItem )
	{
		m_DisguiseItem = *pDisguiseItem;
	}
#else
	if ( !InCond( TF_COND_DISGUISED ) )
	{
		m_iDisguiseWeaponModelIndex = -1;
		m_pDisguiseWeaponInfo = NULL;
		return;
	}

	CTFWeaponInfo *pDisguiseWeaponInfo = GetTFWeaponInfoForItem(m_DisguiseItem.GetItemDefIndex(), m_nDisguiseClass);

	m_pDisguiseWeaponInfo = pDisguiseWeaponInfo;
	m_iDisguiseWeaponModelIndex = -1;

	if ( pDisguiseWeaponInfo )
	{
		m_iDisguiseWeaponModelIndex = modelinfo->GetModelIndex(m_DisguiseItem.GetWorldDisplayModel());
	}

#endif
	// If we have a weapon that changes bodygroup (ie: gloves) update the bodygroup for it.
	SetWeaponDisguiseBodygroups(0);
	if (GetDisguiseItem()->GetStaticData() && GetDisguiseItem()->GetStaticData()->hide_bodygroups_deployed_only)
	{
		CEconItemDefinition *pStatic = GetDisguiseItem()->GetStaticData();
		if ( pStatic )
		{
			PerTeamVisuals_t *pVisuals = pStatic->GetVisuals();
			if ( pVisuals )
			{
				for (int i = 0; i < ToTFPlayer(GetDisguiseTarget())->GetNumBodyGroups(); i++)
				{
					unsigned int index = pVisuals->player_bodygroups.Find(ToTFPlayer(GetDisguiseTarget())->GetBodygroupName(i));
					if ( pVisuals->player_bodygroups.IsValidIndex( index ) )
					{
						bool bTrue = pVisuals->player_bodygroups.Element( index );
						if ( bTrue )
						{
							SetBodygroup(ToTFPlayer(GetDisguiseTarget())->GetModelPtr(), m_iWeaponBodygroup, i, 1);
						}
						else
							SetBodygroup(ToTFPlayer(GetDisguiseTarget())->GetModelPtr(), m_iWeaponBodygroup, i, 0);
					}
				}
			}
		}	
	}
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Crit effects handling.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritBoostEffect( bool bForceHide /*= false*/ )
{
	bool bShouldShow = !bForceHide;

	if ( bShouldShow )
	{
		if ( m_pOuter->IsDormant() )
		{
			bShouldShow = false;
		}
		else if ( !IsCritBoosted() && !InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
		{
			bShouldShow = false;
		}
		else if ( InCond( TF_COND_STEALTHED ) )
		{
			bShouldShow = false;
		}
		else if ( InCond( TF_COND_DISGUISED ) &&
			!m_pOuter->InSameTeam( C_TFPlayer::GetLocalTFPlayer() ) &&
			m_pOuter->GetTeamNumber() != GetDisguiseTeam() )
		{
			// Don't show crit effect for disguised enemy spies unless they're disguised
			// as their own team.
			bShouldShow = false;
		}
	}

	if ( bShouldShow )
	{
		// Update crit effect model.
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );
			m_pCritEffect = NULL;
		}

		if ( !m_pOuter->ShouldDrawThisPlayer() )
		{
			m_hCritEffectHost = m_pOuter->GetViewModel();
		}
		else
		{
			C_BaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();
			
			// Don't add crit effect to weapons without a model.
			if (pWeapon && pWeapon->GetWorldModelIndex() != 0)
			{
				m_hCritEffectHost = pWeapon;
			}
			else
			{
				m_hCritEffectHost = m_pOuter;
			}
		}

		if ( m_hCritEffectHost.Get() )
		{
			const char *pszEffect = ConstructTeamParticle( "critgun_weaponmodel_%s", m_pOuter->GetTeamNumber(), true, g_aTeamNamesShort );
			m_pCritEffect = m_hCritEffectHost->ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
		}

		if ( !m_pCritSound )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			CLocalPlayerFilter filter;
			m_pCritSound = controller.SoundCreate( filter, m_pOuter->entindex(), "Weapon_General.CritPower" );
			controller.Play( m_pCritSound, 1.0, 100 );
		}
	}
	else
	{
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );
			m_pCritEffect = NULL;
		}

		m_hCritEffectHost = NULL;

		if ( m_pCritSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pCritSound );
			m_pCritSound = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalcDisguiseWeaponLegacy( void )
{
	if ( !InCond( TF_COND_DISGUISED ) ) 
	{
		m_iDisguiseWeaponModelIndex = -1;
		m_pDisguiseWeaponInfo = NULL;
		return;
	}

	Assert( m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY );

	CTFWeaponInfo *pDisguiseWeaponInfo = NULL;

	TFPlayerClassData_t *pData = GetPlayerClassData( m_nDisguiseClass );

	Assert( pData );

	// Find the weapon in the same slot
	int i;
	for ( i=0;i<TF_PLAYER_WEAPON_COUNT;i++ )
	{
		if ( pData->m_aWeapons[i] != TF_WEAPON_NONE )
		{
			const char *pWpnName = WeaponIdToAlias( pData->m_aWeapons[i] );

			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWpnName );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );

			// find the primary weapon
			if ( pWeaponInfo && pWeaponInfo->iSlot == 0 )
			{
				pDisguiseWeaponInfo = pWeaponInfo;
				break;
			}
		}
	}

	Assert( pDisguiseWeaponInfo != NULL && "Cannot find slot 0 primary weapon for desired disguise class\n" );

	m_pDisguiseWeaponInfo = pDisguiseWeaponInfo;
	m_iDisguiseWeaponModelIndex = -1;

	if ( pDisguiseWeaponInfo )
	{
		m_iDisguiseWeaponModelIndex = modelinfo->GetModelIndex( pDisguiseWeaponInfo->szWorldModel );
	}
}

CTFWeaponInfo *CTFPlayerShared::GetDisguiseWeaponInfo( void )
{
	if (InCond(TF_COND_DISGUISED) && m_pDisguiseWeaponInfo == NULL)
	{
		RecalcDisguiseWeapon();
	}

	return m_pDisguiseWeaponInfo;
}
#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::Heal(CTFPlayer *pPlayer, float flAmount, bool bDispenserHeal /* = false */)
{
	Assert(FindHealerIndex(pPlayer) == m_aHealers.InvalidIndex());

	healers_t newHealer;
	newHealer.pPlayer = pPlayer;
	newHealer.flAmount = flAmount;
	newHealer.bDispenserHeal = bDispenserHeal;
	m_aHealers.AddToTail(newHealer);

	AddCond(TF_COND_HEALTH_BUFF, PERMANENT_CONDITION);

	RecalculateChargeEffects();

	m_nNumHealers = m_aHealers.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::StopHealing(CTFPlayer *pPlayer)
{
	int iIndex = FindHealerIndex(pPlayer);
	if ( iIndex == m_aHealers.InvalidIndex() )
		return;

	m_aHealers.Remove(iIndex);

	if (!m_aHealers.Count())
	{
		RemoveCond(TF_COND_HEALTH_BUFF);
	}

	RecalculateChargeEffects();

	m_nNumHealers = m_aHealers.Count();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetPassiveChargeEffect( CTFPlayer *pPlayer )
{
	if ( !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && !tf2v_randomizer.GetBool() )
		return 0;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( !pWpn )
		return 0;

	CWeaponMedigun *pMedigun = dynamic_cast < CWeaponMedigun * >( pWpn );
	if ( pMedigun && pMedigun->HasMultipleHealingModes()  )
		return pMedigun->GetCurrentResistanceType();

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
medigun_charge_types CTFPlayerShared::GetChargeEffectBeingProvided( CTFPlayer *pPlayer )
{
	if ( !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && !tf2v_randomizer.GetBool() )
		return TF_CHARGE_NONE;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( !pWpn )
		return TF_CHARGE_NONE;

	CWeaponMedigun *pMedigun = dynamic_cast < CWeaponMedigun * >( pWpn );
	if ( pMedigun && pMedigun->IsReleasingCharge()  )
		return pMedigun->GetChargeType();

	return TF_CHARGE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculateChargeEffects(bool bInstantRemove)
{
	bool bShouldCharge[TF_CHARGE_COUNT] = {};
	CTFPlayer *pProviders[TF_CHARGE_COUNT] = {};

	medigun_charge_types selfCharge = GetChargeEffectBeingProvided(m_pOuter);
	
	// Charging self?
	if (selfCharge != TF_CHARGE_NONE)
	{
		bShouldCharge[selfCharge] = true;
		pProviders[selfCharge] = m_pOuter;
	}
	else
	{
		// Check players healing us.
		for (int i = 0; i < m_aHealers.Count(); i++)
		{
			if (!m_aHealers[i].pPlayer)
				continue;

			CTFPlayer *pPlayer = ToTFPlayer(m_aHealers[i].pPlayer);
			if (!pPlayer)
				continue;

			medigun_charge_types chargeType = GetChargeEffectBeingProvided(pPlayer);

			if (chargeType != TF_CHARGE_NONE)
			{
				bShouldCharge[chargeType] = true;
				pProviders[chargeType] = pPlayer;
			}
		}
	}

	// Deny stock uber while carrying flag.
	if (m_pOuter->HasTheFlag())
	{
		bShouldCharge[TF_CHARGE_INVULNERABLE] = false;
	}

	for (int i = 0; i < TF_CHARGE_COUNT; i++)
	{
		float flRemoveTime = i == TF_CHARGE_INVULNERABLE ? tf_invuln_time.GetFloat() : 0.0f;
		SetChargeEffect((medigun_charge_types)i, bShouldCharge[i], bInstantRemove, g_MedigunEffects[i], flRemoveTime, pProviders[i]);
	}
	
	// Check passive resistances.
	bool bShouldResist[TF_CHARGE_COUNT] = {};
	CTFPlayer *pResistors[TF_CHARGE_COUNT] = {};
	int nPassiveChargeType = GetPassiveChargeEffect(m_pOuter);
	if (nPassiveChargeType != 0)
	{
		bShouldResist[nPassiveChargeType] = true;
		pResistors[nPassiveChargeType] = m_pOuter;
	}
	else
	{
		// Check players healing us.
		for (int i = 0; i < m_aHealers.Count(); i++)
		{
			if (!m_aHealers[i].pPlayer)
				continue;

			CTFPlayer *pPlayer = ToTFPlayer(m_aHealers[i].pPlayer);
			if (!pPlayer)
				continue;

			int chargeType = GetPassiveChargeEffect(pPlayer);

			if (chargeType)
			{
				bShouldResist[chargeType] = true;
				pResistors[chargeType] = pPlayer;
			}
		}
	}
	
	// Start at 3 because that's where resistances kick in.
	for (int i = 3; i < TF_CHARGE_COUNT; i++)
	{
		SetPassiveResist(i, bShouldResist[i], pProviders[i]);	
	}
	
}

void CTFPlayerShared::SetChargeEffect(medigun_charge_types chargeType, bool bShouldCharge, bool bInstantRemove, const MedigunEffects_t &chargeEffect, float flRemoveTime, CTFPlayer *pProvider)
{
	if (InCond(chargeEffect.condition_enable) == bShouldCharge)
	{
		if (bShouldCharge && m_flChargeOffTime[chargeType] != 0.0f)
		{
			m_flChargeOffTime[chargeType] = 0.0f;

			if (chargeEffect.condition_disable != TF_COND_LAST)
				RemoveCond(chargeEffect.condition_disable);
		}
		return;
	}

	if (bShouldCharge)
	{
		Assert(chargeType != TF_CHARGE_INVULNERABLE || !m_pOuter->HasTheFlag());

		if (m_flChargeOffTime[chargeType] != 0.0f)
		{
			m_pOuter->StopSound(chargeEffect.sound_disable);

			m_flChargeOffTime[chargeType] = 0.0f;

			if (chargeEffect.condition_disable != TF_COND_LAST)
				RemoveCond(chargeEffect.condition_disable);
		}

		// Charge on.
		AddCond(chargeEffect.condition_enable);

		CSingleUserRecipientFilter filter(m_pOuter);
		m_pOuter->EmitSound(filter, m_pOuter->entindex(), chargeEffect.sound_enable);
		m_bChargeSounds[chargeType] = true;
	}
	else
	{
		if (m_bChargeSounds[chargeType])
		{
			m_pOuter->StopSound(chargeEffect.sound_enable);
			m_bChargeSounds[chargeType] = false;
		}

		if (m_flChargeOffTime[chargeType] == 0.0f)
		{
			CSingleUserRecipientFilter filter(m_pOuter);
			m_pOuter->EmitSound(filter, m_pOuter->entindex(), chargeEffect.sound_disable);
		}

		if (bInstantRemove)
		{
			m_flChargeOffTime[chargeType] = 0.0f;
			RemoveCond(chargeEffect.condition_enable);

			if (chargeEffect.condition_disable != TF_COND_LAST)
				RemoveCond(chargeEffect.condition_disable);
		}
		else
		{
			// Already turning it off?
			if (m_flChargeOffTime[chargeType] != 0.0f)
				return;

			if (chargeEffect.condition_disable != TF_COND_LAST)
				AddCond(chargeEffect.condition_disable);

			m_flChargeOffTime[chargeType] = gpGlobals->curtime + flRemoveTime;
		}
	}
}


void CTFPlayerShared::SetPassiveResist(int nResistanceType, bool bShouldResist, CTFPlayer *pProvider)
{
	// Don't have a resistance for any others, just bail.
	if (nResistanceType < 3 || nResistanceType > 5)
		return;

	int nConditionSmall = 0; // The "small" resistance.
	int nConditionBig = 0;	// The "big" resistance.

	switch (nResistanceType)
	{
		case 3:	// Bullets.
			nConditionSmall = TF_COND_MEDIGUN_SMALL_BULLET_RESIST;
			nConditionBig = TF_COND_MEDIGUN_UBER_BULLET_RESIST;
			break;
		case 4:	// Explosives.
			nConditionSmall = TF_COND_MEDIGUN_SMALL_BLAST_RESIST;
			nConditionBig = TF_COND_MEDIGUN_UBER_BLAST_RESIST;
			break;
		case 5:	// Fire
			nConditionSmall = TF_COND_MEDIGUN_SMALL_FIRE_RESIST;
			nConditionBig = TF_COND_MEDIGUN_UBER_FIRE_RESIST;
			break;
	}

	// Don't allow the resistances to stack.
	if ( InCond(nConditionBig) && InCond(nConditionSmall) )
	{
		RemoveCond(nConditionSmall);
	}
	else if ( !InCond(nConditionBig) )
	{
		if (bShouldResist)
		{
			// Charge on.
			AddCond(nConditionSmall);
		}
		else
		{
			RemoveCond(nConditionSmall);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::TestAndExpireChargeEffect(medigun_charge_types chargeType)
{
	if (InCond(g_MedigunEffects[chargeType].condition_enable))
	{
		bool bRemoveCharge = false;

		if ((TFGameRules()->State_Get() == GR_STATE_TEAM_WIN) && (TFGameRules()->GetWinningTeam() != m_pOuter->GetTeamNumber()))
		{
			bRemoveCharge = true;
		}

		if (m_flChargeOffTime[chargeType] != 0.0f)
		{
			if (gpGlobals->curtime > m_flChargeOffTime[chargeType])
			{
				bRemoveCharge = true;
			}
		}

		if (bRemoveCharge == true)
		{
			m_flChargeOffTime[chargeType] = 0.0f;

			if (g_MedigunEffects[chargeType].condition_disable != TF_COND_LAST)
			{
				RemoveCond(g_MedigunEffects[chargeType].condition_disable);
			}

			RemoveCond(g_MedigunEffects[chargeType].condition_enable);
		}
	}
	else if (m_bChargeSounds[chargeType])
	{
		// If we're still playing charge sound but not actually charged, stop the sound.
		// This can happen if player respawns while crit boosted.
		m_pOuter->StopSound(g_MedigunEffects[chargeType].sound_enable);
		m_bChargeSounds[chargeType] = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EHANDLE	CTFPlayerShared::GetHealerByIndex( int index )
{
	if (m_aHealers.IsValidIndex( index ))
		return m_aHealers[index].pPlayer;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::FindHealerIndex( CTFPlayer *pPlayer )
{
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		if ( m_aHealers[i].pPlayer == pPlayer )
			return i;
	}

	return m_aHealers.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first healer in the healer array.  Note that this
//		is an arbitrary healer.
//-----------------------------------------------------------------------------
EHANDLE CTFPlayerShared::GetFirstHealer()
{
	if ( m_aHealers.Count() > 0 )
		return m_aHealers.Head().pPlayer;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::HealthKitPickupEffects( int iAmount )
{
	if ( InCond( TF_COND_BURNING ) )
	{
		RemoveCond( TF_COND_BURNING );
		if (InCond(TF_COND_BURNING_PYRO))
			RemoveCond(TF_COND_BURNING_PYRO);
	}
	if ( InCond( TF_COND_BLEEDING ) )
		RemoveCond( TF_COND_BLEEDING );

	if ( IsStealthed() || !m_pOuter )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
	if ( event )
	{
		event->SetInt( "amount", iAmount );
		event->SetInt( "entindex", m_pOuter->entindex() );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::HealerIsDispenser( int index ) const
{
	if( !m_aHealers.IsValidIndex(index) )
		return false;

	return m_aHealers[index].bDispenserHeal;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayerShared::GetActiveTFWeapon() const
{
	return m_pOuter->GetActiveTFWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Team check.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsAlly(CBaseEntity *pEntity)
{
	return (pEntity->GetTeamNumber() == m_pOuter->GetTeamNumber());
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine if player should do loser animations.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsLoser( void )
{
	if ( !m_pOuter->IsAlive() )
		return false;

	if ( tf_always_loser.GetBool() )
		return true;

	if ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		if ( iWinner != m_pOuter->GetTeamNumber() )
		{
			if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
			{
				if ( InCond(TF_COND_DISGUISED ) )
				{
					return (iWinner != GetDisguiseTeam());
				}
			}
			return true;
		}
	}

	// Check if we should be in the loser state while stunned
	if ( InCond( TF_COND_STUNNED ) && ( m_nStunFlags & TF_STUNFLAG_THIRDPERSON ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDesiredPlayerClassIndex(void)
{
	return m_iDesiredPlayerClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : Successful
//-----------------------------------------------------------------------------
bool CTFPlayerShared::AddToSpyCloakMeter( float amt, bool bForce, bool bIgnoreAttribs )
{
	CTFWeaponInvis *pInvis = dynamic_cast<CTFWeaponInvis *>( m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
	if ( !pInvis )
		return false;

	if ( !bForce && pInvis->HasMotionCloak() )
		return false;

	if ( pInvis->HasFeignDeath() )
		amt = Min( amt, 35.0f );

	if ( bIgnoreAttribs )
	{
		if ( amt <= 0.0f || m_flCloakMeter >= 100.0f )
			return false;

		m_flCloakMeter = Clamp( m_flCloakMeter + amt, 0.0f, 100.0f );
		return true;
	}

	int iNoRegenFromItems = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pInvis, iNoRegenFromItems, mod_cloak_no_regen_from_items );
	if ( iNoRegenFromItems )
		return false;

	int iNoCloakWhenCloaked = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pInvis, iNoCloakWhenCloaked, NoCloakWhenCloaked );
	if ( iNoCloakWhenCloaked )
	{
		if (InCond( TF_COND_STEALTHED ))
			return false;
	}

	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pInvis, amt, ReducedCloakFromAmmo );
	if ( amt <= 0.0f || m_flCloakMeter >= 100.0f )
		return false;

	m_flCloakMeter = Clamp( m_flCloakMeter + amt, 0.0f, 100.0f );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Used to deduct cloak from the player.
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveFromSpyCloakMeter(float amt, bool bDrainSound /* = false */)
{
	CTFWeaponInvis *pInvis = dynamic_cast<CTFWeaponInvis *>( m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
	if ( !pInvis )
		return;

	// Remove cloak, but never let us fall under 0.
	if ( m_flCloakMeter > 0)
	{
		m_flCloakMeter = Max( m_flCloakMeter - amt, 0.0f );
		
#ifdef GAME_DLL
		if (bDrainSound)
		{
			// Emit a sound, to warn us that our charge was drained.
			CSingleUserRecipientFilter filter( m_pOuter );
			m_pOuter->EmitSound( filter, m_pOuter->entindex(), "Weapon_Pomson.DrainedVictim" );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetJumping(bool bJumping)
{
	m_bJumping = bJumping;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanGoombaStomp( void )
{
	int nBootsStomp = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, nBootsStomp, boots_falling_stomp );
	return ( ( nBootsStomp == 1 ) || InCond(TF_COND_ROCKETPACK) );
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we can do an airdash. Supercedes bAirDash. [ CTFPlayerShared::SetAirDash(bool bAirDash) ]
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanAirDash( void )
{
	int nSetNoDoubleJump = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, nSetNoDoubleJump, set_scout_doublejump_disabled );
	if ( nSetNoDoubleJump == 1 )
		return false;

	if ( InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	if ( InCond( TF_COND_HALLOWEEN_SPEED_BOOST ) )
		return true;

	// The regular amount of airjumps we can do is one, but attributes can affect this.
	int nMaxAirJumps = 1;
	
	// Check to see if we have attributes for extra airdashes.
	// How we check this depends on old or new logic.
	if (tf2v_use_new_atomizer.GetBool()) // Modern logic: Check based on active weapon.
	{
		CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
		if (pWeapon)
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, nMaxAirJumps, air_dash_count );	
	}
	else								// Old logic: Check based on player.
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, nMaxAirJumps, air_dash_count );
	
	// If in Soda Popper mode, get five dashes. Do not overlap with attributes.
	if ( InCond( TF_COND_SODAPOPPER_HYPE ) && tf2v_use_new_sodapopper.GetBool() )
		nMaxAirJumps = 5;
	
	return ( nMaxAirJumps > GetAirDashCount() );
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we have a parachute to deploy.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::HasParachute( void )
{
	// No parachute, no deploy.
	int nHasParachute = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, nHasParachute, parachute_attribute );
	if ( nHasParachute == 0 )
		return false;
	else if ( !InCond( TF_COND_PARACHUTE_DEPLOYED ) )
	{
		// If we haven't already pulled the parachute this jump, allow.
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we can deploy a parachute.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanParachute( void )
{
	if ( InCond(TF_COND_PARACHUTE_DEPLOYED) )
		return false;
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we are currently parachuting.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsParachuting( void )
{
	if ( InCond(TF_COND_PARACHUTE_ACTIVE) )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Affects our parachute status.
//-----------------------------------------------------------------------------
void CTFPlayerShared::DeployParachute( void )
{
	CTFParachute *pParachute = dynamic_cast<CTFParachute *>(m_pOuter->Weapon_OwnsThisID(TF_WEAPON_PARACHUTE));
	// If we haven't opened a parachute, open one.
	if ( !InCond( TF_COND_PARACHUTE_ACTIVE ) )
	{
		AddCond( TF_COND_PARACHUTE_ACTIVE );
		if (pParachute)
			pParachute->DeployParachute();
	}
	else
	{
		// Close our parachute, and add the deployed condition.
		RemoveCond( TF_COND_PARACHUTE_ACTIVE );
		AddCond( TF_COND_PARACHUTE_DEPLOYED );
		if (pParachute)
			pParachute->RetractParachute();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets our parachute.
//-----------------------------------------------------------------------------
void CTFPlayerShared::ResetParachute( void )
{ 
	CTFParachute *pParachute = dynamic_cast<CTFParachute *>(m_pOuter->Weapon_OwnsThisID(TF_WEAPON_PARACHUTE));
	if ( InCond(TF_COND_PARACHUTE_DEPLOYED) ) 
		RemoveCond(TF_COND_PARACHUTE_DEPLOYED); 
	if ( InCond(TF_COND_PARACHUTE_ACTIVE) ) 
		RemoveCond(TF_COND_PARACHUTE_ACTIVE); 	
	if (pParachute && pParachute->IsOpened() )
			pParachute->RetractParachute();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetLastDashTime(float flLastDash)
{
	m_flLastDashTime = flLastDash;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::IncrementAirDucks(void)
{
	m_nAirDucked++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ResetAirDucks(void)
{
	m_nAirDucked = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom )
{
	const char *pszSequence = NULL;

	switch(iDamageCustom)
	{
		case TF_DMG_CUSTOM_BACKSTAB:
			pszSequence = "primary_death_backstab";
			break;
		case TF_DMG_CUSTOM_HEADSHOT:
		case TF_DMG_CUSTOM_DECAPITATION:
		case TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING:
		case TF_DMG_CUSTOM_DECAPITATION_BOSS:
			pszSequence = "primary_death_headshot";
			break;
	}

#ifdef CLIENT_DLL
	if (iDamageCustom == TF_DMG_CUSTOM_BURNING && tf2v_enable_burning_death.GetBool())
		pszSequence = "primary_death_burning";
#endif

	if (pszSequence != NULL)
	{
		return pAnim->LookupSequence( pszSequence );
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetCritMult(void)
{
	float flRemapCritMul = RemapValClamped(m_iCritMult, 0, 255, 1.0, TF_DAMAGE_CRITMOD_MAXMULT);
	/*#ifdef CLIENT_DLL
	Msg("CLIENT: Crit mult %.2f - %d\n",flRemapCritMul, m_iCritMult);
	#else
	Msg("SERVER: Crit mult %.2f - %d\n", flRemapCritMul, m_iCritMult );
	#endif*/

	return flRemapCritMul;
}

//-----------------------------------------------------------------------------
// Purpose: Set hype/boost meter progress
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddHypeMeter( float value )
{
	m_flHypeMeter = Min( ( m_flHypeMeter + value ) , 100.0f );
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: Set hype/boost meter progress
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveHypeMeter( float value )
{
	m_flHypeMeter = Max( ( m_flHypeMeter - value ) , 0.0f );
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: Update rage buffs
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateRageBuffsAndRage( void )
{
	if ( IsRageActive() )
	{
		if ( m_flEffectBarProgress > 0.0f )
		{
			if ( gpGlobals->curtime > m_flNextRageCheckTime && m_flRageTimeRemaining > 0.0f )
			{
				m_flNextRageCheckTime = gpGlobals->curtime + 1.0f;
				m_flRageTimeRemaining--;
				PulseRageBuff();
			}

			m_flEffectBarProgress -= ( 100.0f / tf_soldier_buff_pulses.GetFloat() ) * gpGlobals->frametime;
		}
		else
		{
			ResetRageSystem();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set rage meter progress
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetRageMeter( float flRagePercent, int iBuffType )
{
	if ( !IsRageActive() )
	{
		if ( m_pOuter )
		{
			CTFBuffItem *pBuffItem = ( CTFBuffItem * )m_pOuter->Weapon_GetSlot( TF_LOADOUT_SLOT_SECONDARY );
			if ( pBuffItem && pBuffItem->GetBuffType() == iBuffType )
			{
				// Only build rage if we're using this type of banner
				m_flEffectBarProgress = Min( m_flEffectBarProgress + flRagePercent, 100.0f );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activate rage
//-----------------------------------------------------------------------------
void CTFPlayerShared::ActivateRageBuff( CBaseEntity *pEntity, int iBuffType )
{
	
	if ( m_flEffectBarProgress < 100.0f && ( iBuffType != TF_COND_RADIUSHEAL ) )
		return;

	m_flNextRageCheckTime = gpGlobals->curtime + 1.0f;
	
	float flBuffDuration;
	if ( iBuffType != TF_COND_RADIUSHEAL )
		flBuffDuration = tf_soldier_buff_pulses.GetFloat();
	else
		flBuffDuration = 4.5f; // Taunt duration

	float flModBuffDuration = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(m_pOuter, flModBuffDuration, mod_buff_duration);

	m_flRageTimeRemaining = flBuffDuration + flModBuffDuration;
	m_iActiveBuffType = iBuffType;

#ifdef GAME_DLL
	//*(this + 112) = pEntity;

	if ( m_pOuter )
	{
		if ( iBuffType == TF_BUFF_OFFENSE )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
		}
		else if ( iBuffType == TF_BUFF_DEFENSE )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_INCOMING );
		}
	}

#endif

	if ( !IsRageActive() )
	{
		m_bRageActive = true;
	}

	PulseRageBuff();
}


//-----------------------------------------------------------------------------
// Purpose: Give rage buffs to nearby players
//-----------------------------------------------------------------------------
void CTFPlayerShared::PulseRageBuff( /*CTFPlayerShared::ERageBuffSlot*/ )
{
	// g_SoldierBuffAttributeIDToConditionMap is called here in Live TF2
#ifdef GAME_DLL
	CTFPlayer *pOuter = m_pOuter;

	if ( !m_pOuter )
		return;

	CBaseEntity *pEntity = NULL;
	Vector vecOrigin = pOuter->GetAbsOrigin();

	for ( CEntitySphereQuery sphere( vecOrigin, TF_BUFF_RADIUS ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( !pEntity )
			continue;

		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( vecOrigin, &vecHitPoint );
		Vector vecDir = vecHitPoint - vecOrigin;

		CTFPlayer *pPlayer = ToTFPlayer( pEntity );

		if ( vecDir.LengthSqr() < ( TF_BUFF_RADIUS * TF_BUFF_RADIUS ) )
		{
			if ( pPlayer && pPlayer->InSameTeam( pOuter ) )
			{
				switch ( m_iActiveBuffType )
				{
					case TF_BUFF_OFFENSE:
						pPlayer->m_Shared.AddCond( TF_COND_OFFENSEBUFF, 1.2f );
						break;
					case TF_BUFF_DEFENSE:
						pPlayer->m_Shared.AddCond( TF_COND_DEFENSEBUFF, 1.2f );
						break;
					case TF_BUFF_REGENONDAMAGE:
						pPlayer->m_Shared.AddCond( TF_COND_REGENONDAMAGEBUFF, 1.2f );
						break;
					case TF_COND_RADIUSHEAL:
						pPlayer->m_Shared.AddCond(TF_COND_RADIUSHEAL, TF_MEDIC_REGEN_TIME);
						pPlayer->AOEHeal(pPlayer, pOuter);
						break;
				}

				// Achievements
				if ( m_iActiveBuffType != TF_COND_RADIUSHEAL )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "player_buff" );
					if ( event )
					{
						event->SetInt( "userid", pPlayer->GetUserID() );
						event->SetInt( "buff_type", m_iActiveBuffType );
						event->SetInt( "buff_owner", pOuter->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
 			
			if (pPlayer && pPlayer->m_Shared.InCond(TF_COND_DISGUISED) && !pPlayer->m_Shared.InCond(TF_COND_STEALTHED) && (!pPlayer->InSameTeam(pOuter) && pPlayer->m_Shared.GetDisguiseTeam() == pOuter->GetTeamNumber()) && m_iActiveBuffType == TF_COND_RADIUSHEAL)
			{
				// Also heal disguised spies.
				pPlayer->m_Shared.AddCond(TF_COND_RADIUSHEAL, TF_MEDIC_REGEN_TIME);
				pPlayer->AOEHeal(pPlayer, pOuter);
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Reset rage variables
//-----------------------------------------------------------------------------
void CTFPlayerShared::ResetRageSystem( void )
{
	m_flEffectBarProgress = 0.0f;
	m_flNextRageCheckTime = 0.0f;
	m_flRageTimeRemaining = 0.0f;
	m_iActiveBuffType = 0;
	m_bRageActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePlayer *CTFPlayerShared::GetKnockbackWeaponOwner( void )
{
	if ( m_iWeaponKnockbackID == -1 )
		return NULL;

	Assert( dynamic_cast<CTFPlayer *>( UTIL_PlayerByUserId( m_iWeaponKnockbackID ) ) );
	return UTIL_PlayerByUserId( m_iWeaponKnockbackID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::HasDemoShieldEquipped( void ) const
{
	return m_bShieldEquipped;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::CalcChargeCrit( bool bForceCrit )
{
	if (m_flChargeMeter <= 33.0f || bForceCrit)
	{
		m_iNextMeleeCrit = kCritType_Crit;
	}
	else if (m_flChargeMeter <= 75.0f)
	{
		m_iNextMeleeCrit = kCritType_MiniCrit;
	}

#ifdef GAME_DLL
	m_pOuter->SetContextThink( &CTFPlayer::RemoveMeleeCrit, gpGlobals->curtime + 0.3f, "RemoveMeleeCrit" );
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCloakMeter( void )
{
	if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( InCond( TF_COND_STEALTHED ) )
		{
			if ( m_bHasMotionCloak )
			{
				float flSpeed = m_pOuter->GetAbsVelocity().LengthSqr();
				if ( flSpeed == 0.0f )
				{
					m_flCloakMeter += gpGlobals->frametime * m_flCloakRegenRate;

					if ( m_flCloakMeter >= 100.0f )
						m_flCloakMeter = 100.0f;
				}
				else
				{
					float flMaxSpeed = Square( m_pOuter->MaxSpeed() );
					if ( flMaxSpeed == 0.0f )
					{
						m_flCloakMeter -= m_flCloakDrainRate * gpGlobals->frametime * 1.5f;
					}
					else
					{
						m_flCloakMeter -= ( m_flCloakDrainRate * gpGlobals->frametime * 1.5f ) * Min( flSpeed / flMaxSpeed, 1.0f );
					}
				}
			}
			else
			{
				m_flCloakMeter -= gpGlobals->frametime * m_flCloakDrainRate;
			}
			
			// New cloak increases debuff speed by 25%
			if (tf2v_use_new_cloak.GetBool())
			{
				float flReduction = 0.75f * gpGlobals->frametime;
				if ( InCond( TF_COND_BURNING ) )
				{
					// Reduce the duration of this burn 
					m_flFlameRemoveTime -=  flReduction;
				}
				if ( InCond( TF_COND_BLEEDING ) )
				{
					for ( int i=0; i<m_aBleeds.Count(); ++i )
					{
						bleed_struct_t *bleed = &m_aBleeds[i];
						bleed->m_flEndTime -= flReduction;
					}
				}
				
				// Reduce Jarate
				if ( InCond( TF_COND_URINE ) )
				{
					m_flCondExpireTimeLeft.Set( TF_COND_URINE, max( m_flCondExpireTimeLeft[TF_COND_URINE] - flReduction, 0 ) );

					if ( m_flCondExpireTimeLeft[TF_COND_URINE] == 0 )
					{
						RemoveCond( TF_COND_URINE );
					}
				}

				// Reduce Mad Milk
				if ( InCond( TF_COND_MAD_MILK ) )
				{
					m_flCondExpireTimeLeft.Set( TF_COND_MAD_MILK, max( m_flCondExpireTimeLeft[TF_COND_MAD_MILK] - flReduction, 0 ) );

					if ( m_flCondExpireTimeLeft[TF_COND_MAD_MILK] == 0 )
					{
						RemoveCond( TF_COND_MAD_MILK );
					}
				}
				
				// Reduce Gas
				if ( InCond( TF_COND_GAS ) )
				{
					m_flCondExpireTimeLeft.Set( TF_COND_MAD_MILK, max( m_flCondExpireTimeLeft[TF_COND_MAD_MILK] - flReduction, 0 ) );

					if ( m_flCondExpireTimeLeft[TF_COND_MAD_MILK] == 0 )
					{
						RemoveCond( TF_COND_MAD_MILK );
					}
				}
			}

			if (m_flCloakMeter <= 0.0f)
			{
				m_flCloakMeter = 0.0f;

				if ( !m_bHasMotionCloak )
					FadeInvis( tf_spy_invis_unstealth_time.GetFloat() );
			}
		}
		else
		{
			m_flCloakMeter += gpGlobals->frametime * m_flCloakRegenRate;

			if (m_flCloakMeter >= 100.0f)
				m_flCloakMeter = 100.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateSanguisugeHealth( void )
{
	// Drain our Sanguisuge health, but at a much slower rate. (2HP per second)
	if ( (m_pOuter->m_Shared.GetSanguisugeHealth() > 0) && (gpGlobals->curtime >= m_pOuter->m_Shared.m_iLeechDecayTime ) )
	{
		float flHealthtoRemove = 1; // 2HP per second; 1HP two ticks a second.
		// Manually subtract the health from our Sanguisuge pool as well.
		m_pOuter->m_iHealth -= (int)flHealthtoRemove;
		m_pOuter->m_Shared.ChangeSanguisugeHealth( (int)flHealthtoRemove * -1 );
		m_pOuter->m_Shared.SetNextSanguisugeDecay();
		if (m_pOuter->m_Shared.GetSanguisugeHealth() < 0) // Can't be below 0.
			m_pOuter->m_Shared.SetSanguisugeHealth(0);
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateChargeMeter( void )
{
	if (InCond( TF_COND_SHIELD_CHARGE ))
	{
		m_flChargeMeter -= ( 100 / m_flChargeDrainRate ) * gpGlobals->frametime;

		if (m_flChargeMeter <= 0.0f)
		{
			m_flChargeMeter = 0.0f;
			RemoveCond( TF_COND_SHIELD_CHARGE );
		}
	}
	else if (m_flChargeMeter < 100.0f)
	{
		m_flChargeMeter += m_flChargeRegenRate * gpGlobals->frametime;
		m_flChargeMeter = Min( m_flChargeMeter.Get(), 100.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateEnergyDrinkMeter( void )
{
	if ( m_flEnergyDrinkMeter > 100.0f ) // Prevent our meter from going over 100
	{
		m_flEnergyDrinkMeter = 100.0f;
	}
		
	if ( InCond( TF_COND_SODAPOPPER_HYPE ) )
	{
		m_flHypeMeter -= ( gpGlobals->frametime * m_flEnergyDrinkDrainRate ) * 0.75;

		if ( m_flHypeMeter <= 0.0f )
			RemoveCond( TF_COND_SODAPOPPER_HYPE );
		
		return;
	}
	
	if ( InCond( TF_COND_PHASE ) || InCond( TF_COND_ENERGY_BUFF ) )
	{
		m_flEnergyDrinkMeter -= m_flEnergyDrinkDrainRate * gpGlobals->frametime;

		if ( m_flEnergyDrinkMeter <= 0.0f )
		{
			RemoveCond( TF_COND_PHASE );
			RemoveCond( TF_COND_ENERGY_BUFF );

			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_TIRED );
		}
		else if ( InCond( TF_COND_PHASE ) )
		{
			UpdatePhaseEffects();
		}
		
		return;	
	}

	// No Bonk/Cola/Popper active, regen our meter
	if ( m_flEnergyDrinkMeter >= 100.0f )
			return;

	m_flEnergyDrinkMeter += m_flEnergyDrinkRegenRate * gpGlobals->frametime;

	if ( m_pOuter->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX_DRINK ) )
	{
		if ( m_flEnergyDrinkMeter >= 100.0f )
			return;

		if ( m_pOuter->GetAmmoCount( TF_AMMO_GRENADES2 ) != m_pOuter->GetMaxAmmo( TF_AMMO_GRENADES2 ) )
			return;
	}

	m_flEnergyDrinkMeter = Min( m_flEnergyDrinkMeter.Get(), 100.0f );
	
}

//-----------------------------------------------------------------------------
// Purpose: Updates our Sniper's Focus level.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateFocusLevel( void )
{
	if ( m_flFocusLevel > 100.0f ) // Prevent our meter from going over 100
	{
		m_flFocusLevel = 100.0f;
	}
		
	if ( InCond( TF_COND_SNIPERCHARGE_RAGE_BUFF ) )
	{
		m_flFocusLevel -= ( gpGlobals->frametime * 10 );	// Max is 10 seconds.

		if ( m_flFocusLevel <= 0.0f )
		{
			RemoveCond( TF_COND_SNIPERCHARGE_RAGE_BUFF );
			m_flFocusLevel = 0;
		}
	}
	
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Adds focus to our character. input is kill (true) or assist (false).
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddFocusLevel(bool bKillOrAssist)
{
	if (bKillOrAssist)	// Kill
	{
		float flKillLevel = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pOuter, flKillLevel, rage_on_kill);
		if (flKillLevel)
			m_flFocusLevel = Min(m_flFocusLevel + flKillLevel, 100.0f);
		return;
	}
	else 				//Assist
	{
		float flAssistLevel = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(m_pOuter, flAssistLevel, rage_on_assist);
		if (flAssistLevel)
			m_flFocusLevel = Min(m_flFocusLevel + flAssistLevel, 100.0f);
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates our Fire Rage.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateFireRage( void )
{
	if ( m_flFireRage > 100.0f ) // Prevent our meter from going over 100
	{
		m_flFireRage = 100.0f;
	}
		
	if ( InCond( TF_COND_CRITBOOSTED_RAGE_BUFF ) )
	{
		m_flFireRage -= ( gpGlobals->frametime * 10 );	// Max is 10 seconds.

		if ( m_flFireRage <= 0.0f )
		{
			RemoveCond( TF_COND_CRITBOOSTED_RAGE_BUFF );
			m_flFireRage = 0;
		}
	}
	
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Updates our Crikey.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCrikeyMeter( void )
{
	if ( m_flCrikeyMeter > 100.0f ) // Prevent our meter from going over 100
	{
		m_flCrikeyMeter = 100.0f;
	}
	
	float flCrikeyDuration = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(m_pOuter, flCrikeyDuration, minicrit_boost_when_charged);
		
	if ( InCond( TF_COND_MINICRITBOOSTED_RAGE_BUFF ) )
	{
		if (flCrikeyDuration > 0)
		{
			m_flCrikeyMeter -= ( gpGlobals->frametime * (100/flCrikeyDuration) );	// Regularly 8 seconds. Don't allow division by zero.

			if ( m_flCrikeyMeter <= 0.0f )
			{
				RemoveCond( TF_COND_MINICRITBOOSTED_RAGE_BUFF );
				m_flCrikeyMeter = 0;
			}
		}
		else
		{
			RemoveCond( TF_COND_MINICRITBOOSTED_RAGE_BUFF );
			m_flCrikeyMeter = 0;
		}	
	}
	
	return;
}


#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::EndCharge( void )
{
	if (InCond( TF_COND_SHIELD_CHARGE ) && m_flChargeMeter < 90.0f)
	{
	#ifdef GAME_DLL
		CalcChargeCrit( false );

		CTFWearableDemoShield *pShield = GetEquippedDemoShield( m_pOuter );
		if (pShield) pShield->ShieldBash( m_pOuter );
	#endif

		m_flChargeMeter = 0;
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritMult(void)
{
	const float flMinMult = 1.0;
	const float flMaxMult = TF_DAMAGE_CRITMOD_MAXMULT;

	if (m_DamageEvents.Count() == 0)
	{
		m_iCritMult = RemapValClamped(flMinMult, flMinMult, flMaxMult, 0, 255);
		return;
	}

	//Msg( "Crit mult update for %s\n", m_pOuter->GetPlayerName() );
	//Msg( "   Entries: %d\n", m_DamageEvents.Count() );

	// Go through the damage multipliers and remove expired ones, while summing damage of the others
	float flTotalDamage = 0;
	for (int i = m_DamageEvents.Count() - 1; i >= 0; i--)
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if (flDelta > tf_damage_events_track_for.GetFloat())
		{
			//Msg( "      Discarded (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			m_DamageEvents.Remove(i);
			continue;
		}

		// Ignore damage we've just done. We do this so that we have time to get those damage events
		// to the client in time for using them in prediction in this code.
		if (flDelta < TF_DAMAGE_CRITMOD_MINTIME)
		{
			//Msg( "      Ignored (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			continue;
		}

		if (flDelta > TF_DAMAGE_CRITMOD_MAXTIME)
			continue;

		//Msg( "      Added %.2f (%d: time %.2f, now %.2f)\n", m_DamageEvents[i].flDamage, i, m_DamageEvents[i].flTime, gpGlobals->curtime );

		flTotalDamage += m_DamageEvents[i].flDamage;
	}

	float flMult = RemapValClamped(flTotalDamage, 0, tf2v_critmod_range.GetFloat(), flMinMult, flMaxMult);

	//Msg( "   TotalDamage: %.2f   -> Mult %.2f\n", flTotalDamage, flMult );

	m_iCritMult = (int)RemapValClamped(flMult, flMinMult, flMaxMult, 0, 255);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecordDamageEvent(const CTakeDamageInfo &info, bool bKill)
{
	if (m_DamageEvents.Count() >= MAX_DAMAGE_EVENTS)
	{
		// Remove the oldest event
		m_DamageEvents.Remove(m_DamageEvents.Count() - 1);
	}

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = info.GetDamage();
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].bKill = bKill;

	// Don't count critical damage
	if (info.GetDamageType() & DMG_CRITICAL)
	{
		m_DamageEvents[iIndex].flDamage /= TF_DAMAGE_CRIT_MULTIPLIER;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::GetNumKillsInTime(float flTime)
{
	if (tf_damage_events_track_for.GetFloat() < flTime)
	{
		Warning("Player asking for damage events for time %.0f, but tf_damage_events_track_for is only tracking events for %.0f\n", flTime, tf_damage_events_track_for.GetFloat());
	}

	int iKills = 0;
	for (int i = m_DamageEvents.Count() - 1; i >= 0; i--)
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if (flDelta < flTime)
		{
			if (m_DamageEvents[i].bKill)
			{
				iKills++;
			}
		}
	}

	return iKills;
}

#endif

//=============================================================================
//
// Shared player code that isn't CTFPlayerShared
//

//-----------------------------------------------------------------------------
// Purpose:
//   Input: info
//          bDoEffects - effects (blood, etc.) should only happen client-side.
//-----------------------------------------------------------------------------
void CTFPlayer::FireBullet(const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType /*= TF_DMG_CUSTOM_NONE*/)
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	CTFWeaponBase *pTFWeapon = dynamic_cast< CTFWeaponBase* >(pWeapon);

	// Fire a bullet (ignoring the shooter).
	Vector vecStart = info.m_vecSrc;
	Vector vecEnd = vecStart + info.m_vecDirShooting * info.m_flDistance;
	trace_t trace;
	UTIL_TraceLine(vecStart, vecEnd, (MASK_SOLID | CONTENTS_HITBOX), this, COLLISION_GROUP_NONE, &trace);
	
	

#ifdef GAME_DLL
	if (tf_debug_bullets.GetBool())
	{
		NDebugOverlay::Line(vecStart, trace.endpos, 0, 255, 0, true, 30);
	}
#endif


#ifdef CLIENT_DLL
	if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 2)
	{
		// draw red client impact markers
		debugoverlay->AddBoxOverlay(trace.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 255, 0, 0, 127, 4);

		if (trace.m_pEnt && trace.m_pEnt->IsPlayer())
		{
			C_BasePlayer *player = ToBasePlayer(trace.m_pEnt);
			player->DrawClientHitboxes(4, true);
		}
	}
#else
	if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 3)
	{
		// draw blue server impact markers
		NDebugOverlay::Box(trace.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 0, 255, 127, 4);

		if (trace.m_pEnt && trace.m_pEnt->IsPlayer())
		{
			CBasePlayer *player = ToBasePlayer(trace.m_pEnt);
			player->DrawServerHitboxes(4, true);
		}
	}
#endif

	if (sv_showplayerhitboxes.GetInt() > 0)
	{
		CBasePlayer *lagPlayer = UTIL_PlayerByIndex(sv_showplayerhitboxes.GetInt());
		if (lagPlayer)
		{
#ifdef CLIENT_DLL
			lagPlayer->DrawClientHitboxes(4, true);
#else
			lagPlayer->DrawServerHitboxes(4, true);
#endif
		}
	}

	if (trace.fraction < 1.0)
	{
		// Verify we have an entity at the point of impact.
		Assert(trace.m_pEnt);

		if (bDoEffects)
		{
			// If shot starts out of water and ends in water
			if (!(enginetrace->GetPointContents(trace.startpos) & (CONTENTS_WATER | CONTENTS_SLIME)) &&
				(enginetrace->GetPointContents(trace.endpos) & (CONTENTS_WATER | CONTENTS_SLIME)))
			{
				// Water impact effects.
				ImpactWaterTrace(trace, vecStart);
			}
			else
			{
				// Regular impact effects.

				// don't decal your teammates or objects on your team
				if (trace.m_pEnt->GetTeamNumber() != GetTeamNumber())
				{
					UTIL_ImpactTrace(&trace, nDamageType);
				}
			}

#ifdef CLIENT_DLL
			static int	tracerCount;
			if ((info.m_iTracerFreq != 0) && (tracerCount++ % info.m_iTracerFreq) == 0)
			{
				// if this is a local player, start at attachment on view model
				// else start on attachment on weapon model

				int iEntIndex = entindex();
				int iUseAttachment = TRACER_DONT_USE_ATTACHMENT;
				int iAttachment = 1;


				if (pWeapon)
				{
					if ( pTFWeapon )
					{
						C_BaseViewModel *vm = pTFWeapon->GetViewmodelAddon();
						iAttachment = vm ? vm->LookupAttachment( "muzzle" ) : pWeapon->LookupAttachment( "muzzle" );
					}
					else
					{
						iAttachment = pWeapon->LookupAttachment( "muzzle" );
					}
				}

				bool bInToolRecordingMode = clienttools->IsInRecordingMode();
				bool bZoomedSniperRifle = false;
				
				if (pTFWeapon)
				{
					if ( WeaponID_IsSniperRifle( pTFWeapon->GetWeaponID() ) && m_Shared.InCond(TF_COND_ZOOMED) )
						bZoomedSniperRifle = true;
				}

				// try to align tracers to actual weapon barrel if possible
				if (!ShouldDrawThisPlayer() && !bInToolRecordingMode && !bZoomedSniperRifle)
				{
					C_TFViewModel *pViewModel = dynamic_cast<C_TFViewModel *>(GetViewModel());

					if (pViewModel)
					{
						iEntIndex = pViewModel->entindex();
						pViewModel->GetAttachment(iAttachment, vecStart);
					}
				}
				else if (!IsDormant())
				{
					// fill in with third person weapon model index

					if (pWeapon)
					{
						iEntIndex = pWeapon->entindex();

						int nModelIndex = pWeapon->GetModelIndex();
						int nWorldModelIndex = pWeapon->GetWorldModelIndex();
						if (bInToolRecordingMode && nModelIndex != nWorldModelIndex)
						{
							pWeapon->SetModelIndex(nWorldModelIndex);
						}

						pWeapon->GetAttachment(iAttachment, vecStart);

						if (bInToolRecordingMode && nModelIndex != nWorldModelIndex)
						{
							pWeapon->SetModelIndex(nModelIndex);
						}
					}
				}

				if (tf_useparticletracers.GetBool())
				{
					const char *pszTracerEffect = GetTracerType();
					if (pszTracerEffect && pszTracerEffect[0])
					{
						char szTracerEffect[128];
						if (nDamageType & DMG_CRITICAL)
						{
							Q_snprintf(szTracerEffect, sizeof(szTracerEffect), "%s_crit", pszTracerEffect);
							pszTracerEffect = szTracerEffect;
						}

						UTIL_ParticleTracer(pszTracerEffect, vecStart, trace.endpos, entindex(), iUseAttachment, true);
					}
				}
				else
				{
					UTIL_Tracer(vecStart, trace.endpos, entindex(), iUseAttachment, 5000, true, GetTracerType());
				}
			}
#endif

		}

		// Server specific.
#ifndef CLIENT_DLL
		if (!pTFWeapon->IsPenetrating() )
		{
			// See what material we hit.
			CTakeDamageInfo dmgInfo( this, info.m_pAttacker, GetActiveWeapon(), info.m_flDamage, nDamageType, nCustomDamageType );
			CalculateBulletDamageForce(&dmgInfo, info.m_iAmmoType, info.m_vecDirShooting, trace.endpos, 1.0);	//MATTTODO bullet forces
			trace.m_pEnt->DispatchTraceAttack(dmgInfo, info.m_vecDirShooting, &trace);
		}
		else
		{		
			// Penetration mechanics.
			// We need to trace each entity it hit with a ray and see if it's a player.
			CBaseEntity *pEnt[256];
			Ray_t ray;
			ray.Init( vecStart, vecEnd );
			int nTargets = UTIL_EntitiesAlongRay(pEnt, ARRAYSIZE(pEnt), ray, FL_CLIENT | FL_OBJECT);
			for (int i = 0; i< nTargets; i++)
			{
				// Get the entity information.
				CBaseEntity *pTarget = pEnt[i];
				if (!pTarget)
					continue;

				// Don't attack ourselves.
				if (pTarget == info.m_pAttacker)
					continue;
				
				// Don't attack friendlies.
				if (info.m_pAttacker->GetTeamNumber() == pTarget->GetTeamNumber())
					continue;
					
				// See what material we hit.
				CTakeDamageInfo dmgInfo( this, info.m_pAttacker, GetActiveWeapon(), info.m_flDamage, nDamageType, nCustomDamageType );
				// Increment our penetration count.
				dmgInfo.SetPlayerPenetrationCount( i );
				CalculateBulletDamageForce(&dmgInfo, info.m_iAmmoType, info.m_vecDirShooting, trace.endpos, 1.0);	//MATTTODO bullet forces
				pTarget->DispatchTraceAttack(dmgInfo, info.m_vecDirShooting, &trace);
				
			}
		}
#endif
	}
}

#ifdef CLIENT_DLL
static ConVar tf_impactwatertimeenable("tf_impactwatertimeenable", "0", FCVAR_CHEAT, "Draw impact debris effects.");
static ConVar tf_impactwatertime("tf_impactwatertime", "1.0f", FCVAR_CHEAT, "Draw impact debris effects.");
#endif

//-----------------------------------------------------------------------------
// Purpose: Trace from the shooter to the point of impact (another player,
//          world, etc.), but this time take into account water/slime surfaces.
//   Input: trace - initial trace from player to point of impact
//          vecStart - starting point of the trace 
//-----------------------------------------------------------------------------
void CTFPlayer::ImpactWaterTrace(trace_t &trace, const Vector &vecStart)
{
#ifdef CLIENT_DLL
	if (tf_impactwatertimeenable.GetBool())
	{
		if (m_flWaterImpactTime > gpGlobals->curtime)
			return;
	}
#endif 

	trace_t traceWater;
	UTIL_TraceLine(vecStart, trace.endpos, (MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME),
		this, COLLISION_GROUP_NONE, &traceWater);
	if (traceWater.fraction < 1.0f)
	{
		CEffectData	data;
		data.m_vOrigin = traceWater.endpos;
		data.m_vNormal = traceWater.plane.normal;
		data.m_flScale = random->RandomFloat(8, 12);
		if (traceWater.contents & CONTENTS_SLIME)
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		const char *pszEffectName = "tf_gunshotsplash";
		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		if (pWeapon && (TF_WEAPON_MINIGUN == pWeapon->GetWeaponID()))
		{
			// for the minigun, use a different, cheaper splash effect because it can create so many of them
			pszEffectName = "tf_gunshotsplash_minigun";
		}
		DispatchEffect(pszEffectName, data);

#ifdef CLIENT_DLL
		if (tf_impactwatertimeenable.GetBool())
		{
			m_flWaterImpactTime = gpGlobals->curtime + tf_impactwatertime.GetFloat();
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayer::GetActiveTFWeapon( void ) const
{
	CBaseCombatWeapon *pRet = GetActiveWeapon();
	if ( pRet )
	{
		Assert( dynamic_cast< CTFWeaponBase* >( pRet ) != NULL );
		return static_cast< CTFWeaponBase * >( pRet );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsActiveTFWeapon( int iWeaponID )
{
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if (pWeapon)
	{
		return pWeapon->GetWeaponID() == iWeaponID;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: How much build resource ( metal ) does this player have
//-----------------------------------------------------------------------------
int CTFPlayer::GetBuildResources( void )
{
	return GetAmmoCount( TF_AMMO_METAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TeamFortress_SetSpeed()
{
	int playerclass = GetPlayerClass()->GetClassIndex();
	float maxfbspeed = 0.0f;

	// Spectators can move while in Classic Observer mode
	if ( IsObserver() )
	{
		if ( GetObserverMode() == OBS_MODE_ROAMING )
			SetMaxSpeed( GetPlayerClassData( TF_CLASS_SCOUT )->m_flMaxSpeed );
		else
			SetMaxSpeed( 0 );
		return;
	}

	// Check for any reason why they can't move at all
	if ( playerclass == TF_CLASS_UNDEFINED || GameRules()->InRoundRestart() )
	{
		SetAbsVelocity( vec3_origin );
		SetMaxSpeed( 1 );
		return;
	}

	// First, get their max class speed
	maxfbspeed = GetPlayerClassData( playerclass )->m_flMaxSpeed;
	
	// Modern spies move at 320 units/s rather than the old 300 units/s.
	// Because the speed is defined elsewhere, just use a fraction here. (~107%)
	if ( ( playerclass == TF_CLASS_SPY ) && ( tf2v_use_new_spy_movespeeds.GetBool() ) )
		maxfbspeed *= ( 320 / 300 );
	
	float flBaseSpeed = maxfbspeed;
	
	// Slow us down if we're disguised as a slower class
	// unless we're cloaked.
	if (m_Shared.InCond(TF_COND_DISGUISED) && ( !m_Shared.InCond(TF_COND_STEALTHED) && ( !m_Shared.GetSpySprint() || ( ( m_Shared.GetDisguiseClass() == TF_CLASS_SCOUT || m_Shared.GetDisguiseClass() == TF_CLASS_MEDIC ) && tf2v_disguise_speed_match.GetBool() ) ) ) )
	{
		float flMaxDisguiseSpeed = GetPlayerClassData( m_Shared.GetDisguiseClass() )->m_flMaxSpeed;
		
		if ( ( m_Shared.GetDisguiseClass() == TF_CLASS_SPY ) && ( tf2v_use_new_spy_movespeeds.GetBool() ) )
			flMaxDisguiseSpeed *= ( 320 / 300 );
		
		// Allow us to match that class' speed.
		if (tf2v_disguise_speed_match.GetBool())
			maxfbspeed = flMaxDisguiseSpeed;
		else 	// Force our speed to be slower.
			maxfbspeed = Min( flMaxDisguiseSpeed, maxfbspeed );
		flBaseSpeed = maxfbspeed;
	}
	
	if (m_Shared.InCond( TF_COND_SHIELD_CHARGE ))
	{
		// Charges use 250% movement speed, or 750HU/s.
		maxfbspeed = 750.0f;
		flBaseSpeed = maxfbspeed;
	}
	
	if ( m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) )
	{
		maxfbspeed *= 2.0f;
	}

	// Speed Boost Effects.
	if ( m_Shared.IsSpeedBoosted() )
	{
		// 40% Speed increase.
		maxfbspeed *= 1.4f;
	}
	
	// Berserk Effect. Normally this wouldn't stack with other speed boosts.
	if ( m_Shared.InCond(TF_COND_BERSERK) )
	{
		// 30% Speed increase.
		maxfbspeed *= 1.3f;
	}
	
	if ( m_Shared.InCond(TF_COND_STEALTHED) && m_Shared.InCond(TF_COND_SPEED_BOOST_FEIGN) )
	{
		// 40% Speed increase. This allows for both speed boosts to be stacked.
		maxfbspeed *= 1.4f;
	}
	
	CTFWeaponBase *pWeapon;
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.InCond( TF_COND_STEALTHED ) && tf2v_use_spy_moveattrib.GetBool() )
	{
		pWeapon = (CTFWeaponBase *)ToTFPlayer(m_Shared.GetDisguiseTarget())->Weapon_GetSlot(m_Shared.GetDisguiseItem()->GetItemSlot());
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( ToTFPlayer(m_Shared.GetDisguiseTarget()), maxfbspeed, mult_player_movespeed );
	}
	else
	{
		pWeapon = GetActiveTFWeapon();
		CALL_ATTRIB_HOOK_FLOAT( maxfbspeed, mult_player_movespeed );
	}

	// Second, see if any flags are slowing them down
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag *>( GetItem() );

		if ( pFlag )
		{
			if ( pFlag->GetGameType() == TF_FLAGTYPE_ATTACK_DEFEND || pFlag->GetGameType() == TF_FLAGTYPE_TERRITORY_CONTROL )
			{
				maxfbspeed *= 0.5;
			}
			else // If we're playing Assault rules CTF, reduce speed.
			if ( pFlag->GetGameType() == TF_FLAGTYPE_CTF && tf2v_assault_ctf_rules.GetBool() )
				maxfbspeed *= 0.80;
		}
	}

	// if they're aiming or spun up, reduce their speed
	if ( m_Shared.InCond( TF_COND_AIMING ) )
	{
		int nNoZoomPenalty = 0;
		CALL_ATTRIB_HOOK_FLOAT( nNoZoomPenalty, mod_zoom_speed_disabled );
		CALL_ATTRIB_HOOK_FLOAT( nNoZoomPenalty, unimplemented_mod_zoom_speed_disabled );
	
		if (nNoZoomPenalty == 0)
		{
			// Heavy moves slightly faster spun-up
			if ( pWeapon && pWeapon->IsWeapon( TF_WEAPON_MINIGUN ) && tf2v_use_new_minigun_aim_speed.GetBool() )
			{
				if ( maxfbspeed > 110 )
					maxfbspeed = 110;
			}
			else
			{
				if ( maxfbspeed > 80 )
					maxfbspeed = 80;
			}
			
			// Modify our movement speed when required.
			CALL_ATTRIB_HOOK_FLOAT( maxfbspeed, mult_player_aiming_movespeed );
		}
	}

	// Engineer moves slower while a hauling an object.
	if ( playerclass == TF_CLASS_ENGINEER && m_Shared.IsCarryingObject() )
	{
		if ( tf2v_use_new_hauling_speed.GetBool() )
			maxfbspeed *= 0.90f;
		else
			maxfbspeed *= 0.75f;
	}

	if ( Weapon_OwnsThisID(TF_WEAPON_MEDIGUN) )
	{
		float flResourceMult = 1.f;
		CALL_ATTRIB_HOOK_FLOAT( flResourceMult, mult_player_movespeed_resource_level );
		if ( flResourceMult != 1.0f )
		{
			CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
			maxfbspeed *= RemapValClamped( pMedigun->GetChargeLevel(), 0.0, 1.0, 1.0, flResourceMult );
		}
	}

	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if ( maxfbspeed > tf_spy_max_cloaked_speed.GetFloat() )
			maxfbspeed = tf_spy_max_cloaked_speed.GetFloat();
	}

	if ( pWeapon )
	{
		maxfbspeed *= pWeapon->GetSpeedMod();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, maxfbspeed, mult_player_movespeed_active );
	}
	
	if ( ( m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.InCond( TF_COND_STEALTHED ) ) && tf2v_use_spy_moveattrib.GetBool() )
	{
		if (ToTFPlayer(m_Shared.GetDisguiseTarget())->m_Shared.m_bShieldEquipped == true)
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(ToTFPlayer(m_Shared.GetDisguiseTarget()), maxfbspeed, mult_player_movespeed_shieldrequired);
	}
	else
	{
		if (m_Shared.m_bShieldEquipped == true)
			CALL_ATTRIB_HOOK_FLOAT( maxfbspeed, mult_player_movespeed_shieldrequired );
	}
		
	CTFSword *pSword = dynamic_cast<CTFSword *>( Weapon_OwnsThisID( TF_WEAPON_SWORD ) );
	if (pSword)
		maxfbspeed *= pSword->GetSwordSpeedMod();


	// (Potentially) Reduce our speed if we were stunned
	if ( m_Shared.InCond( TF_COND_STUNNED ) && ( m_Shared.m_nStunFlags & TF_STUNFLAG_SLOWDOWN ) )
	{
		maxfbspeed *= m_Shared.m_flStunMovementSpeed;
	}

	// If we have speed caps on, check for speed caps.
	if ( tf2v_clamp_speed.GetInt() != 0 )
	{
		// Speed caps for shield charges.
		if ( m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			if ( maxfbspeed > flBaseSpeed )
			{
				// If we're shield charging and faster than our base charge, set it to the standard charge speed.
				maxfbspeed = flBaseSpeed;
			}
		}
		else // Non-shield charge related caps.
		{
			// Relative speed cap.
			if ( maxfbspeed > ( flBaseSpeed + tf2v_clamp_speed_difference.GetInt() ) ) 
			{
				// If we're not charging, clamp the max speed boost we can get.
				maxfbspeed = ( flBaseSpeed + tf2v_clamp_speed_difference.GetInt() );
			}
			// Absolute speed cap.
			if ( maxfbspeed > tf2v_clamp_speed_absolute.GetInt() )
			{
				// If we're not in a shield charge, cap our speed to the max value allowed.
				maxfbspeed = tf2v_clamp_speed_absolute.GetInt();
			}
		}
	}
	
	// if we're in bonus time because a team has won, give the winners 110% speed and the losers 90% speed
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();

		if ( iWinner != TEAM_UNASSIGNED )
		{
			if ( iWinner == GetTeamNumber() )
			{
				maxfbspeed *= 1.1f;
			}
			else
			{
				maxfbspeed *= 0.9f;
			}
		}
	}
	
#ifdef GAME_DLL
	// Check players healing us and update logic if needed.
	for (int i = 0; i < m_Shared.m_aHealers.Count(); i++)
	{
		if (!m_Shared.m_aHealers[i].pPlayer)
			continue;
			
		if (!m_Shared.m_aHealers[i].pPlayer.IsValid())
			continue;

		CTFPlayer *pHealer = ToTFPlayer(m_Shared.m_aHealers[i].pPlayer);
		if (!pHealer)
			continue;

		CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( pHealer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		if (!pMedigun)
			continue;
		
		int nCanFollowCharges = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pMedigun, nCanFollowCharges, set_weapon_mode);
		// Beta Quick Fix: Always match speed.
		if ( nCanFollowCharges == 4 )
			pHealer->SetMaxSpeed(maxfbspeed);
		// Quick-Fix: Follow shield charges, but only when we are allowed to match speed.
		if ( nCanFollowCharges == 2 && tf2v_use_medic_speed_match.GetBool() )
			pHealer->SetMaxSpeed(maxfbspeed);
		// Other mediguns can match speed, but only when allowed to and not on shield charges.
		else if ( tf2v_use_medic_speed_match.GetBool() && !m_Shared.InCond(TF_COND_SHIELD_CHARGE) )
			pHealer->SetMaxSpeed(maxfbspeed);
	}
#endif

	// Set the speed
	SetMaxSpeed( maxfbspeed );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::HasItem(void)
{
	return (m_hItem != NULL);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::SetItem(CTFItem *pItem)
{
	m_hItem = pItem;

#ifndef CLIENT_DLL
	if (pItem && pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG)
	{
		RemoveInvisibility();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItem	*CTFPlayer::GetItem(void)
{
	return m_hItem;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player allowed to use a teleporter ?
//-----------------------------------------------------------------------------
bool CTFPlayer::HasTheFlag(void)
{
	if (HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG)
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Are we allowed to pick the flag up?
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAllowedToPickUpFlag(void)
{
	int bNotAllowedToPickUpFlag = 0;
	CALL_ATTRIB_HOOK_INT(bNotAllowedToPickUpFlag, cannot_pick_up_intelligence);

	if (bNotAllowedToPickUpFlag > 0)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player's allowed to build another one of the specified object
//-----------------------------------------------------------------------------
int CTFPlayer::CanBuild(int iObjectType, int iObjectMode)
{
	if (iObjectType < 0 || iObjectType >= OBJ_LAST)
		return CB_UNKNOWN_OBJECT;

#ifndef CLIENT_DLL
	CTFPlayerClass *pCls = GetPlayerClass();

	if (m_Shared.IsCarryingObject())
	{
		CBaseObject *pObject = m_Shared.GetCarriedObject();
		if (pObject && pObject->GetType() == iObjectType && pObject->GetObjectMode() == iObjectMode)
		{
			return CB_CAN_BUILD;
		}
		else
		{
			Assert(0);
		}
	}

	if (pCls && pCls->CanBuildObject(iObjectType) == false)
	{
		return CB_CANNOT_BUILD;
	}

	if (GetObjectInfo(iObjectType)->m_AltModes.Count() != 0
		&& GetObjectInfo(iObjectType)->m_AltModes.Count() <= iObjectMode * 3)
	{
		return CB_CANNOT_BUILD;
	}
	else if (GetObjectInfo(iObjectType)->m_AltModes.Count() == 0 && iObjectMode != 0)
	{
		return CB_CANNOT_BUILD;
	}

#endif

	int iObjectCount = GetNumObjects(iObjectType, iObjectMode);

	// Make sure we haven't hit maximum number
	if (iObjectCount >= GetObjectInfo(iObjectType)->m_nMaxObjects && GetObjectInfo(iObjectType)->m_nMaxObjects != -1)
	{
		return CB_LIMIT_REACHED;
	}

	// Find out how much the object should cost
	int iCost = ModCalculateObjectCost( iObjectType, HasGunslinger() );

	// Make sure we have enough resources
	if (GetBuildResources() < iCost)
	{
		return CB_NEED_RESOURCES;
	}

	return CB_CAN_BUILD;
}

//-----------------------------------------------------------------------------
// Purpose: Used for calculating object cost when we want to factor in attributes into the cost.
//-----------------------------------------------------------------------------
int CTFPlayer::ModCalculateObjectCost(int iObjectType, bool bMini /*= false*/)
{
	float flBuildCostRatio = 1.0f;

	CALL_ATTRIB_HOOK_FLOAT(flBuildCostRatio, cannot_pick_up_intelligence);
	if (iObjectType == OBJ_TELEPORTER)
		CALL_ATTRIB_HOOK_FLOAT(flBuildCostRatio, cannot_pick_up_intelligence);

	int iCost = CalculateObjectCost(iObjectType, bMini );
	iCost *= flBuildCostRatio;

	return iCost;

}

//-----------------------------------------------------------------------------
// Purpose: Get the number of objects of the specified type that this player has
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumObjects(int iObjectType, int iObjectMode)
{
	int iCount = 0;
	for (int i = 0; i < GetObjectCount(); i++)
	{
		if (!GetObject(i))
			continue;

		if (GetObject(i)->GetType() == iObjectType && GetObject(i)->GetObjectMode() == iObjectMode && !GetObject(i)->IsBeingCarried())
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ItemPostFrame()
{
	if (m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible())
	{
		if (gpGlobals->curtime < m_flNextAttack)
		{
			m_hOffHandWeapon->ItemBusyFrame();
		}
		else
		{
#if defined( CLIENT_DLL )
			// Not predicting this weapon
			if (m_hOffHandWeapon->IsPredicted())
#endif
			{
				m_hOffHandWeapon->ItemPostFrame();
			}
		}
	}

	BaseClass::ItemPostFrame();
}

void CTFPlayer::SetOffHandWeapon(CTFWeaponBase *pWeapon)
{
	m_hOffHandWeapon = pWeapon;
	if (m_hOffHandWeapon.Get())
	{
		m_hOffHandWeapon->Deploy();
	}
}

// Set to NULL at the end of the holster?
void CTFPlayer::HolsterOffHandWeapon(void)
{
	if (m_hOffHandWeapon.Get())
	{
		m_hOffHandWeapon->Holster();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should record our last weapon when switching between the two specified weapons
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_ShouldSetLast(CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon)
{
	// if the weapon doesn't want to be auto-switched to, don't!	
	CTFWeaponBase *pTFWeapon = dynamic_cast< CTFWeaponBase * >(pOldWeapon);

	if (pTFWeapon->AllowsAutoSwitchTo() == false)
	{
		return false;
	}

	return BaseClass::Weapon_ShouldSetLast(pOldWeapon, pNewWeapon);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_Switch(CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
	m_PlayerAnimState->ResetGestureSlot(GESTURE_SLOT_ATTACK_AND_RELOAD);
	return BaseClass::Weapon_Switch(pWeapon, viewmodelindex);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::GetStepSoundVelocities(float *velwalk, float *velrun)
{
	float flMaxSpeed = MaxSpeed();

	if ((GetFlags() & FL_DUCKING) || (GetMoveType() == MOVETYPE_LADDER))
	{
		*velwalk = flMaxSpeed * 0.25;
		*velrun = flMaxSpeed * 0.3;
	}
	else
	{
		*velwalk = flMaxSpeed * 0.3;
		*velrun = flMaxSpeed * 0.8;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetStepSoundTime(stepsoundtimes_t iStepSoundTime, bool bWalking)
{
	float flMaxSpeed = MaxSpeed();

	switch (iStepSoundTime)
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = RemapValClamped(flMaxSpeed, 200, 450, 400, 200);
		if (bWalking)
		{
			m_flStepSoundTime += 100;
		}
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = RemapValClamped(flMaxSpeed, 200, 450, 600, 400);
		break;

	default:
		Assert(0);
		break;
	}

	if ((GetFlags() & FL_DUCKING) || (GetMoveType() == MOVETYPE_LADDER))
	{
		m_flStepSoundTime += 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAttack(void)
{
	CTFGameRules *pRules = TFGameRules();

	Assert(pRules);

	if ((m_Shared.GetStealthNoAttackExpireTime() > gpGlobals->curtime || m_Shared.InCond( TF_COND_STEALTHED )) && !m_Shared.InCond( TF_COND_STEALTHED_USER_BUFF ))
	{
#ifdef CLIENT_DLL
		HintMessage(HINT_CANNOT_ATTACK_WHILE_CLOAKED, true, true);
#endif
		return false;
	}

	if ( m_Shared.IsFeignDeathReady() )
		return false;

	// Stunned players cannot attack
	if ( m_Shared.InCond( TF_COND_STUNNED ) )
		return false;

	if (pRules->State_Get() == GR_STATE_TEAM_WIN && pRules->GetWinningTeam() != GetTeamNumber())
	{
		return false;
	}

	return true;
}

#ifdef GAME_DLL
bool CTFPlayer::CanPickupBuilding(CBaseObject *pObject)
{
	if ( !pObject )
		return false;

	if ( pObject->GetBuilder() != this )
		return false;

	if ( pObject->IsBuilding() || pObject->IsUpgrading() || pObject->IsRedeploying() )
		return false;

	if ( pObject->HasSapper() )
		return false;

	return true;

	if ( m_Shared.InCond( TF_COND_STUNNED ) )
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::TryToPickupBuilding( void )
{
	if ( !tf2v_building_hauling.GetBool() )
		return false;

	if ( m_Shared.IsLoser() )
		return false;

	if ( IsActiveTFWeapon( TF_WEAPON_BUILDER ) )
		return false;

	int bCannotPickUpBuildings = 0;
	CALL_ATTRIB_HOOK_INT( bCannotPickUpBuildings, cannot_pick_up_buildings );
	if ( bCannotPickUpBuildings != 0 )
		return false;

	Vector vecForward;
	AngleVectors( EyeAngles(), &vecForward );
	Vector vecSwingStart = Weapon_ShootPosition();
	
	// Check if we can pick up buildings remotely, and how much it costs.
	bool bHasRemotePickup = false;
	int nRemotePickup = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetActiveTFWeapon(), nRemotePickup, building_teleporting_pickup);
	if ( nRemotePickup != 0 )
	{
		// Check if we also have enough metal to perform a remote pickup.
		if ( GetBuildResources() >= nRemotePickup )
			bHasRemotePickup = true;
	}
	
	// Trace to see if we touch anything.
	// First trace is the normal pickup, while second trace is for teleport pickups.
	float flRange = 150.0f;
	int iMaxHaulingChecks = ( bHasRemotePickup ? 2 : 1 );
	for ( int iTraces = 1; iTraces <= iMaxHaulingChecks; iTraces++ )
	{
		// Increase our range on the teleport pickup iteration.
		if ( iTraces == 2 )
			flRange = 5500.0f;
		
		Vector vecSwingEnd = vecSwingStart + vecForward * flRange;
		// only trace against objects
		// See if we hit anything.
		trace_t trace;

		CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );

		if ( trace.fraction < 1.0f &&
			trace.m_pEnt &&
			trace.m_pEnt->IsBaseObject() &&
			trace.m_pEnt->GetTeamNumber() == GetTeamNumber() )
		{
			CBaseObject *pObject = dynamic_cast<CBaseObject *>( trace.m_pEnt );
			if ( CanPickupBuilding( pObject ) )
			{
				CTFWeaponBase *pWpn = Weapon_OwnsThisID( TF_WEAPON_BUILDER );

				if ( pWpn )
				{
					CTFWeaponBuilder *pBuilder = dynamic_cast< CTFWeaponBuilder * >( pWpn );

					// Is this the builder that builds the object we're looking for?
					if ( pBuilder )
					{
						pObject->MakeCarriedObject(this);

						pBuilder->SetSubType( pObject->ObjectType() );
						pBuilder->SetObjectMode( pObject->GetObjectMode() );

						SpeakConceptIfAllowed( MP_CONCEPT_PICKUP_BUILDING );

						// try to switch to this weapon
						Weapon_Switch( pBuilder );

						m_flNextCarryTalkTime = gpGlobals->curtime + RandomFloat( 6.0f, 12.0f );

						// If remote pickup, subtract the metal from our current metal reserve.
						if ( iTraces == 2 )
							RemoveBuildResources( nRemotePickup );
						return true;
					}
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetCarriedObject(CBaseObject *pObj)
{
	if (pObj)
	{
		m_bCarryingObject = true;
		m_hCarriedObject = pObj;
	}
	else
	{
		m_bCarryingObject = false;
		m_hCarriedObject = NULL;
	}

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject* CTFPlayerShared::GetCarriedObject(void)
{
	CBaseObject *pObj = m_hCarriedObject.Get();
	return pObj;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::IncKillstreak( int weaponSlot )
{
	int currentStreak = m_nStreaks.Get( weaponSlot );
	m_nStreaks.Set( weaponSlot, ++currentStreak );
}

//-----------------------------------------------------------------------------
// Purpose: Weapons can call this on secondary attack and it will link to the class
// ability
//-----------------------------------------------------------------------------
bool CTFPlayer::DoClassSpecialSkill(void)
{
	bool bDoSkill = false;

	switch ( GetPlayerClass()->GetClassIndex() )
	{
		case TF_CLASS_SPY:
		{
			if ( m_Shared.m_flStealthNextChangeTime <= gpGlobals->curtime )
			{
				// Toggle invisibility
				CTFWeaponInvis *pInvis = dynamic_cast<CTFWeaponInvis *>( Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
				if ( pInvis )
				{
					bDoSkill = pInvis->ActivateInvisibility();
				}

				if ( bDoSkill )
					m_Shared.m_flStealthNextChangeTime = gpGlobals->curtime + 0.5;
			}
			break;
		}
		case TF_CLASS_DEMOMAN:
		{
			CTFPipebombLauncher *pPipebombLauncher = static_cast<CTFPipebombLauncher*>(Weapon_OwnsThisID(TF_WEAPON_PIPEBOMBLAUNCHER));

			if ( pPipebombLauncher )
			{
				pPipebombLauncher->SecondaryAttack();

				bDoSkill = true;
			}

			break;
		}
		case TF_CLASS_ENGINEER:
		{
			bDoSkill = false;
		#ifdef GAME_DLL
			bDoSkill = TryToPickupBuilding();
		#endif
			break;
		}

		default:
			break;
	}

	CTFWearableDemoShield *pShield = GetEquippedDemoShield( this );
	if ( pShield )
	{
		bDoSkill = false;
	#ifdef GAME_DLL
		if (pShield->DoSpecialAction( this ))
		{
			bDoSkill = true;
			
		}
	#endif
	}

	return bDoSkill;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanGoInvisible( bool bFeigning )
{
	if ( !bFeigning && HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		HintMessage(HINT_CANNOT_CLOAK_WITH_FLAG);
		return false;
	}

	// Stunned players cannot go invisible
	if ( m_Shared.InCond( TF_COND_STUNNED ) )
	{
		return false;
	}

	CTFGameRules *pRules = TFGameRules();

	Assert(pRules);

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//ConVar testclassviewheight( "testclassviewheight", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
//Vector vecTestViewHeight(0,0,0);

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
Vector CTFPlayer::GetClassEyeHeight(void)
{
	CTFPlayerClass *pClass = GetPlayerClass();

	if (!pClass)
		return VEC_VIEW_SCALED(this);

	/*if ( testclassviewheight.GetFloat() > 0 )
	{
	vecTestViewHeight.z = testclassviewheight.GetFloat();
	return vecTestViewHeight;
	}*/

	int iClassIndex = pClass->GetClassIndex();

	if (iClassIndex < TF_FIRST_NORMAL_CLASS || iClassIndex > TF_CLASS_COUNT_ALL)
		return VEC_VIEW_SCALED(this);

	return (g_TFClassViewVectors[pClass->GetClassIndex()] * GetModelScale());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::MedicGetChargeLevel( void )
{
	CWeaponMedigun *pMedigun = GetMedigun();

	if ( pMedigun )
		return pMedigun->GetChargeLevel();

	return 0.0f;

	// Spy has a fake uber level.
	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		return m_Shared.m_flDisguiseChargeLevel;
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::MedicGetHealTarget( void )
{
	CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( GetActiveTFWeapon() );
	if (pMedigun)
		return pMedigun->GetHealTarget();

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMedigun *CTFPlayer::GetMedigun(void)
{
	CTFWeaponBase *pWeapon = Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
	if ( pWeapon )
		return static_cast<CWeaponMedigun *>( pWeapon );

	return NULL;
}

CTFWeaponBase *CTFPlayer::Weapon_OwnsThisID( int iWeaponID )
{
	for (int i = 0; i < WeaponCount(); i++)
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		if ( pWpn->GetWeaponID() == iWeaponID )
		{
			return pWpn;
		}
	}

	return NULL;
}

CTFWeaponBase *CTFPlayer::Weapon_GetWeaponByType(int iType)
{
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase * )GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		int iWeaponRole = pWpn->GetTFWpnData().m_iWeaponType;

		if ( iWeaponRole == iType )
		{
			return pWpn;
		}
	}

	return NULL;

}

CEconEntity *CTFPlayer::GetEntityForLoadoutSlot(int iSlot)
{
	if ( iSlot >= TF_LOADOUT_SLOT_HAT )
	{
		// Weapons don't get equipped in cosmetic slots.
		return GetWearableForLoadoutSlot( iSlot );
	}

	int iClass = m_PlayerClass.GetClassIndex();

	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CBaseCombatWeapon *pWeapon = GetWeapon( i );
		if ( !pWeapon )
			continue;

		CEconItemDefinition *pItemDef = pWeapon->GetItem()->GetStaticData();

		if ( pItemDef && pItemDef->GetLoadoutSlot(iClass) == iSlot )
		{
			return pWeapon;
		}
	}

	// Wearable?
	CEconWearable *pWearable = GetWearableForLoadoutSlot(iSlot);
	if ( pWearable )
		return pWearable;

	return NULL;
}

CEconWearable *CTFPlayer::GetWearableForLoadoutSlot(int iSlot)
{
	int iClass = m_PlayerClass.GetClassIndex();

	for (int i = 0; i < GetNumWearables(); i++)
	{
		CEconWearable *pWearable = GetWearable(i);

		if (!pWearable)
			continue;

		CEconItemDefinition *pItemDef = pWearable->GetItem()->GetStaticData();

		if (pItemDef && pItemDef->GetLoadoutSlot(iClass) == iSlot)
		{
			return pWearable;
		}
	}

	return NULL;
}

int CTFPlayer::GetMaxAmmo( int iAmmoIndex, int iClassNumber /*= -1*/ ) const
{
	if ( !GetPlayerClass()->GetData() )
		return 0;

	int iMaxAmmo = 0;

	if ( iClassNumber != -1 )
	{
		iMaxAmmo = GetPlayerClassData( iClassNumber )->m_aAmmoMax[iAmmoIndex];
	}
	else
	{
		iMaxAmmo = GetPlayerClass()->GetData()->m_aAmmoMax[iAmmoIndex];
	}

	// If we have a weapon that overrides max ammo, use its value.
	// BUG: If player has multiple weapons using same ammo type then only the first one's value is used.
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTFWeaponBase *pWpn = (CTFWeaponBase *)GetWeapon( i );

		if ( !pWpn )
			continue;

		if ( pWpn->GetPrimaryAmmoType() != iAmmoIndex )
			continue;

		// Random weapons should use the ammo of the player class related to this weapon
		// BUG: Client calls to this method will return incorrect max ammo values in randomizer
#ifdef GAME_DLL
		if ( tf2v_randomizer.GetBool() || tf2v_random_weapons.GetBool() )
		{
			CEconItemView *pItem = pWpn->GetItem();
			if ( pItem )
			{
				iMaxAmmo = GetPlayerClassData( pItem->GetItemClassNumber() )->m_aAmmoMax[iAmmoIndex];
			}
		}
#endif
	}
	
	// If we're using 2007 era ammocounts, re-adjust our ammo pools.
	if ( tf2v_era_ammocounts.GetInt() == 0 )
	{
		switch (iClassNumber)
		{
			case TF_CLASS_SOLDIER:
				if ( iAmmoIndex == TF_AMMO_PRIMARY )
					iMaxAmmo *= ( 36.f / 20.f ); 
				break;
			
			case TF_CLASS_DEMOMAN:
				if ( iAmmoIndex == TF_AMMO_PRIMARY )
					iMaxAmmo *= ( 30.f / 16.f ); 
				else if ( iAmmoIndex == TF_AMMO_SECONDARY )
					iMaxAmmo *= ( 40.f / 24.f ); 
				break;
	
			default:
				break;
		}
	}
	else if ( tf2v_era_ammocounts.GetInt() == 1 )
	{
		// If we're using 2008 ammo settings, reduce only the Rocket Launcher.
		if ( (iClassNumber == TF_CLASS_SOLDIER) && ( iAmmoIndex == TF_AMMO_PRIMARY ) )
			iMaxAmmo *= ( 16.f / 20.f ); 
	}

	switch ( iAmmoIndex )
	{
		case TF_AMMO_PRIMARY:
			CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_primary );
			break;
		case TF_AMMO_SECONDARY:
			CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_secondary );
			break;
		case TF_AMMO_METAL:
			CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_metal );
			break;
		case TF_AMMO_GRENADES1:
			CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_grenades1 );
			break;
		case TF_AMMO_GRENADES2:
			CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_grenades2 );
			break;
		case TF_AMMO_GRENADES3:
			iMaxAmmo = 1;
			break;
	}

	return iMaxAmmo;
}

void CTFPlayer::PlayStepSound(Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force)
{
#ifdef CLIENT_DLL
	// Don't make predicted footstep sounds in third person, animevents will take care of that.
	if (prediction->InPrediction() && C_BasePlayer::ShouldDrawLocalPlayer())
		return;
#endif

	BaseClass::PlayStepSound(vecOrigin, psurface, fvol, force);
}

#ifndef CLIENT_DLL

CON_COMMAND_F( addcond, "", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if (pPlayer && args.ArgC() > 1)
	{
		float flDuration = -1.0f;
		if (args.ArgC() > 2)
			flDuration = atof( args[2] );

		pPlayer->m_Shared.AddCond( atoi( args[1] ), flDuration );
	}
}

CON_COMMAND_F( removecond, "", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if (pPlayer && args.ArgC() > 1)
	{
		pPlayer->m_Shared.RemoveCond( atoi( args[1] ) );
	}
}

#endif
