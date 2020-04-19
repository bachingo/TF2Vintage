//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "particle_parse.h"
#include "tf_gamerules.h"
#include "entity_merasmus_trickortreatprop.h"
#include "player_vs_environment/merasmus.h"



IMPLEMENT_AUTO_LIST( ITFMerasmusTrickOrTreatPropAutoList )

LINK_ENTITY_TO_CLASS( tf_merasmus_trick_or_treat_prop, CTFMerasmusTrickOrTreatProp );

ConVar tf_merasmus_prop_health( "tf_merasmus_prop_health", "150", FCVAR_CHEAT|FCVAR_GAMEDLL );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMerasmusTrickOrTreatProp::CTFMerasmusTrickOrTreatProp()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMerasmusTrickOrTreatProp *CTFMerasmusTrickOrTreatProp::Create( Vector const &vecOrigin, QAngle const &vecAngles )
{
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFMerasmusTrickOrTreatProp::GetRandomPropModelName( void )
{
	return gs_pszDisguiseProps[ RandomInt( 0, ARRAYSIZE( gs_pszDisguiseProps ) ) ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMerasmusTrickOrTreatProp::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( GetRandomPropModelName() );

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );

	m_takedamage = DAMAGE_YES;

	SetHealth( tf_merasmus_prop_health.GetInt() );
	SetMaxHealth( GetHealth() );

	DispatchParticleEffect( "merasmus_object_spawn", WorldSpaceCenter(), GetAbsAngles() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMerasmusTrickOrTreatProp::Event_Killed( CTakeDamageInfo const &info )
{
	// Disappear, like magic!
	SpawnTrickOrTreatItem();
	DispatchParticleEffect( "merasmus_object_spawn", WorldSpaceCenter(), GetAbsAngles() );
	EmitSound( "Halloween.Merasmus_Hiding_Explode" );

	if ( !TFGameRules()->m_hBosses.IsEmpty() && TFGameRules()->m_hBosses.Head() )
	{
		// Theoretically one shouldn't exist without the other, so assumptions are made here
		CMerasmus *pMerasmus = assert_cast<CMerasmus *>( TFGameRules()->m_hBosses[0].Get() );
		if ( pMerasmus->IsNextKilledPropMerasmus() )
		{
			pMerasmus->SetAbsOrigin( GetAbsOrigin() );
			pMerasmus->NotifyFound( ToTFPlayer( info.GetAttacker() ) );
		}
		else
		{
			CPVSFilter filter( GetAbsOrigin() );
			if ( RandomInt( 1, 3 ) == 1 )
				pMerasmus->PlayLowPrioritySound( filter, "Halloween.MerasmusTauntFakeProp" );
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFMerasmusTrickOrTreatProp::OnTakeDamage( CTakeDamageInfo const &info )
{
	DispatchParticleEffect( "merasmus_blood", info.GetDamagePosition(), GetAbsAngles() );

	CTakeDamageInfo newInfo = info;

	CTFPlayer *pPlayer = ToTFPlayer( info.GetAttacker() );
	if ( pPlayer && ( pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) || pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) ) )
	{
		if ( info.GetDamageType() & DMG_BLAST )
			newInfo.SetDamage( GetHealth() * 2 );
	}

	return BaseClass::OnTakeDamage( newInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMerasmusTrickOrTreatProp::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	if ( !pOther->IsPlayer() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
		{
			pPlayer->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );
			pPlayer->BombHeadExplode( false );

			CTakeDamageInfo info( pPlayer, pPlayer, 99999, DMG_BLAST );
			Event_Killed( info );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMerasmusTrickOrTreatProp::SpawnTrickOrTreatItem( void )
{
}
