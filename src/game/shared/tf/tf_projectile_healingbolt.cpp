//=============================================================================//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_healingbolt.h"
#include "tf_gamerules.h"

#ifdef GAME_DLL
	#include "props_shared.h"
	#include "debugoverlay_shared.h"
	#include "tf_gamestats.h"
#endif

#ifdef GAME_DLL
extern ConVar tf_debug_arrows;
ConVar tf2v_healing_bolts( "tf2v_healing_bolts", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables crossbow bolts to be able to heal teammates." );
ConVar tf2v_healing_bolts_heal_factor( "tf2v_healing_bolts_heal_factor", "2", FCVAR_NOTIFY | FCVAR_REPLICATED, "Multiplication factor of healing when using a healing bolt." );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_HealingBolt, DT_TFProjectile_HealingBolt )
BEGIN_NETWORK_TABLE( CTFProjectile_HealingBolt, DT_TFProjectile_HealingBolt )
#ifdef CLIENT_DLL

#else

#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_HealingBolt )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_healing_bolt, CTFProjectile_HealingBolt );
PRECACHE_REGISTER( tf_projectile_healing_bolt );

CTFProjectile_HealingBolt::CTFProjectile_HealingBolt()
{
}

CTFProjectile_HealingBolt::~CTFProjectile_HealingBolt()
{
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_HealingBolt::ImpactTeamPlayer( CTFPlayer *pTarget )
{
	IGameEvent *event = NULL;

	// Bail out early if not allowed
	if ( !tf2v_healing_bolts.GetBool() )
		return;

	if ( pTarget == nullptr )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner == nullptr )
		return;

	float flHealing = GetDamage() * tf2v_healing_bolts_heal_factor.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTarget, flHealing, mult_healing_from_medics );

	CTFWeaponBase *pWeapon = pTarget->GetActiveTFWeapon();
	if ( pWeapon )
	{
		int nBlocksHealing = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, nBlocksHealing, weapon_blocks_healing );
		if ( nBlocksHealing == 1 )
			return;

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flHealing, mult_health_fromhealers_penalty_active );
	}
	
	if ( pTarget->TakeHealth( flHealing, DMG_GENERIC ) )
	{
		PlayImpactSound( pOwner, "Weapon_Arrow.ImpactFleshCrossbowHeal" );

		CTF_GameStats.Event_PlayerHealedOther( pOwner, flHealing );

		event = gameeventmanager->CreateEvent( "player_healed" );
		if ( event )
		{
			event->SetInt( "priority", 1 ); // HLTV priority
			event->SetInt( "patient", pTarget->entindex() );
			event->SetInt( "healer", pOwner->entindex() );
			event->SetInt( "amount", flHealing );

			gameeventmanager->FireEvent( event );
		}

		event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "entindex", pTarget->entindex() );
			event->SetInt( "amount", flHealing );

			gameeventmanager->FireEvent( event );
		}

		// Display health particles for a short duration
		pTarget->m_Shared.AddCond( TF_COND_HEALTH_OVERHEALED, 1.2f );
	}
}

float CTFProjectile_HealingBolt::GetDamage( void )
{
	return m_flDamage * RemapValClamped( gpGlobals->curtime - m_flCreateTime, 0.0f, 0.6f, 0.5f, 1.0f );
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_HealingBolt::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		char const *pszEffect = ConstructTeamParticle( "healshot_trail_%s", GetTeamNumber() );
		ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
	}
}

#endif
