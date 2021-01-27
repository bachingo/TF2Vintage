//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "tf_base_boss.h"

ConVar tf_base_boss_speed( "tf_base_boss_speed", "75", FCVAR_CHEAT );
ConVar tf_base_boss_max_turn_rate( "tf_base_boss_max_turn_rate", "25", FCVAR_CHEAT );

IMPLEMENT_SERVERCLASS_ST( CTFBaseBoss, DT_TFBaseBoss )
	SendPropFloat( SENDINFO( m_lastHealthPercentage ), 11, SPROP_CHANGES_OFTEN|SPROP_NOSCALE, 0.0, 1.0 ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFBaseBoss )
	DEFINE_KEYFIELD( m_initialHealth, FIELD_INTEGER, "health" ),
	DEFINE_KEYFIELD( m_modelString, FIELD_STRING, "model" ),
	DEFINE_KEYFIELD( m_speed, FIELD_FLOAT, "speed" ),
	DEFINE_KEYFIELD( m_startDisabled, FIELD_INTEGER, "start_disabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxHealth", InputSetMaxHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddHealth", InputAddHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveHealth", InputRemoveHealth ),

	DEFINE_OUTPUT( m_outputOnHealthBelow90Percent,	"OnHealthBelow90Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow80Percent,	"OnHealthBelow80Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow70Percent,	"OnHealthBelow70Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow60Percent,	"OnHealthBelow60Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow50Percent,	"OnHealthBelow50Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow40Percent,	"OnHealthBelow40Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow30Percent,	"OnHealthBelow30Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow20Percent,	"OnHealthBelow20Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow10Percent,	"OnHealthBelow10Percent" ),
	DEFINE_OUTPUT( m_outputOnKilled, "OnKilled" ),

	DEFINE_THINKFUNC( BossThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( base_boss, CTFBaseBoss );


CTFBaseBoss::CTFBaseBoss()
{
	m_modelString = NULL_STRING;
	m_lastHealthPercentage = 1.0f;
	m_speed = tf_base_boss_speed.GetFloat();
	m_locomotor = new CTFBaseBossLocomotion( this );
	m_nCurrencyValue = 125;
	m_initialHealth = 0;
	m_bResolvePlayerCollisions = true;
}

CTFBaseBoss::~CTFBaseBoss()
{
	if ( m_locomotor )
		delete m_locomotor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::Precache( void )
{
	if ( m_modelString != NULL_STRING )
	{
		PrecacheModel( STRING( m_modelString ) );
	}

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	if ( m_modelString != NULL_STRING )
	{
		SetModel( STRING( m_modelString ) );
	}

	m_bEnabled = m_startDisabled == false;
	m_nDamagePoseParameter = -1;

	SetHealth( m_initialHealth );
	SetMaxHealth( m_initialHealth );

	if ( TFGameRules() )
		TFGameRules()->RegisterBoss( this );

	SetContextThink( &CTFBaseBoss::BossThink, gpGlobals->curtime, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	if ( pOther && pOther->IsBaseObject() )
	{
		// flatten buildings outright
		pOther->TakeDamage( CTakeDamageInfo( this, this, 99999.9f, DMG_CRUSH ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBaseBoss::OnTakeDamage( const CTakeDamageInfo &info )
{
	// There's a call to CTFGameRules::ApplyOnDamageModifyRules here
	// but that doesn't change it unless the victim is a player, so
	// I'm skipping it

	if ( info.GetDamage() > 0 && info.GetAttacker() != this )
	{
		CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
		if ( pAttacker && info.GetWeapon() )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if ( pWeapon )
			{
				pWeapon->ApplyOnHitAttributes( this, pAttacker, info );
			}
		}
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBaseBoss::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( info.GetAttacker() == NULL )
		return 0;
	if ( info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
		return 0;

	CTakeDamageInfo newInfo( info );
	ModifyDamage( &newInfo );

	IGameEvent *event = gameeventmanager->CreateEvent( "npc_hurt" );
	if ( event )
	{

		event->SetInt( "entindex", entindex() );
		event->SetInt( "health", GetHealth() );
		event->SetInt( "damageamount", newInfo.GetDamage() );
		event->SetBool( "crit", ( newInfo.GetDamageType() & DMG_CRITICAL ) ? true : false );

		CTFPlayer *pAttacker = ToTFPlayer( newInfo.GetAttacker() );
		if ( pAttacker )
		{
			event->SetInt( "attacker_player", pAttacker->GetUserID() );

			if ( pAttacker->GetActiveTFWeapon() )
			{
				event->SetInt( "weaponid", pAttacker->GetActiveTFWeapon()->GetWeaponID() );
			}
			else
			{
				event->SetInt( "weaponid", 0 );
			}
		}
		else
		{
			event->SetInt( "attacker_player", 0 );
			event->SetInt( "weaponid", 0 );
		}

		gameeventmanager->FireEvent( event );
	}

	int nDamageTaken = BaseClass::OnTakeDamage_Alive( newInfo );

	const float flHealthPercentage = (float)GetHealth() / GetMaxHealth();
	if ( m_lastHealthPercentage > 0.9f && flHealthPercentage < 0.9f )
		m_outputOnHealthBelow90Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.8f && flHealthPercentage < 0.8f )
		m_outputOnHealthBelow80Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.7f && flHealthPercentage < 0.7f )
		m_outputOnHealthBelow70Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.6f && flHealthPercentage < 0.6f )
		m_outputOnHealthBelow60Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.5f && flHealthPercentage < 0.5f )
		m_outputOnHealthBelow50Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.4f && flHealthPercentage < 0.4f )
		m_outputOnHealthBelow40Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.3f && flHealthPercentage < 0.3f )
		m_outputOnHealthBelow30Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.2f && flHealthPercentage < 0.2f )
		m_outputOnHealthBelow20Percent.FireOutput( this, this );
	else if ( m_lastHealthPercentage > 0.1f && flHealthPercentage < 0.1f )
		m_outputOnHealthBelow10Percent.FireOutput( this, this );

	m_lastHealthPercentage = flHealthPercentage;

	CTFPlayer *pAttacker = ToTFPlayer( newInfo.GetAttacker() );
	if ( pAttacker )
	{
		CTF_GameStats.Event_BossDamage( pAttacker, newInfo.GetDamage() );
	}

	return nDamageTaken;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::Event_Killed( const CTakeDamageInfo &info )
{
	m_outputOnKilled.FireOutput( this, this );

	// TODO - dispense this
	m_nCurrencyValue = GetCurrencyValue();

	BaseClass::Event_Killed( info );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::UpdateOnRemove()
{
	if ( TFGameRules() )
		TFGameRules()->RemoveBoss( this );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::BossThink( void )
{
	SetNextThink( gpGlobals->curtime );

	if ( m_nDamagePoseParameter < 0 )
	{
		m_nDamagePoseParameter = LookupPoseParameter( "damage" );
	}

	if ( m_nDamagePoseParameter >= 0 )
	{
		// Avoid divide by zero
		const float flHealthFrac = (float)GetHealth() / ( GetMaxHealth() + FLT_EPSILON );
		SetPoseParameter( m_nDamagePoseParameter, 1.0f - flHealthFrac );
	}

	if ( !m_bEnabled )
		return;

	Update();

	if ( m_bResolvePlayerCollisions )
	{
		CUtlVector<CTFPlayer *> playerVector;
		CollectPlayers( &playerVector, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );

		for( int i=0; i<playerVector.Count(); ++i )
			ResolvePlayerCollision( playerVector[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetSpeed( inputdata_t &inputdata )
{
	m_speed = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetHealth( inputdata_t &inputdata )
{
	m_iHealth = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputSetMaxHealth( inputdata_t &inputdata )
{
	m_iMaxHealth = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputAddHealth( inputdata_t &inputdata )
{
	int iHealth = inputdata.value.Int();
	SetHealth( Min( GetMaxHealth(), GetHealth() + iHealth ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::InputRemoveHealth( inputdata_t &inputdata )
{
	int iDamage = inputdata.value.Int();
	SetHealth( GetHealth() - iDamage );

	if ( GetHealth() <= 0 )
	{
		CTakeDamageInfo info( inputdata.pCaller, inputdata.pActivator, vec3_origin, GetAbsOrigin(), iDamage, DMG_GENERIC );
		Event_Killed( info );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBoss::ResolvePlayerCollision( CTFPlayer *pPlayer )
{
	Vector vecBossMins = WorldAlignMins();
	Vector vecBossMaxs = WorldAlignMaxs();
	Vector vecPlayerMins = pPlayer->WorldAlignMins();
	Vector vecPlayerMaxs = pPlayer->WorldAlignMaxs();

	Vector vecBossOOBBMins = vecBossMins + GetAbsOrigin();
	Vector vecBossOOBBMaxs = vecBossMaxs + GetAbsOrigin();
	Vector vecPlayerOOBBMins = vecPlayerMins + pPlayer->GetAbsOrigin();
	Vector vecPlayerOOBBMaxs = vecPlayerMaxs + pPlayer->GetAbsOrigin();

	// Are we even intersecting?
	if ( vecBossOOBBMaxs.x < vecPlayerOOBBMins.x || vecBossOOBBMins.x > vecPlayerOOBBMaxs.x )
		return;
	if ( vecBossOOBBMaxs.y < vecPlayerOOBBMins.y || vecBossOOBBMins.y > vecPlayerOOBBMaxs.y )
		return;
	if ( vecBossOOBBMaxs.z < vecPlayerOOBBMins.z || vecBossOOBBMins.z > vecPlayerOOBBMaxs.z )
		return;

	// Overlap between me and player
	Vector vecOBBOverlap;
	// Which direction to slide the player
	Vector vecShiftDir;

	Vector vecToPlayer = pPlayer->WorldSpaceCenter() - WorldSpaceCenter();
	if ( vecToPlayer.x >= 0 )
	{
		vecOBBOverlap.x = vecBossOOBBMaxs.x - vecPlayerOOBBMins.x;
		vecShiftDir.x = 1.0f;
	}
	else
	{
		vecOBBOverlap.x = vecPlayerOOBBMaxs.x - vecBossOOBBMins.x;
		vecShiftDir.x = -1.0f;
	}
	if ( vecToPlayer.y >= 0 )
	{
		vecOBBOverlap.y = vecBossOOBBMaxs.y - vecPlayerOOBBMins.y;
		vecShiftDir.y = 1.0f;
	}
	else
	{
		vecOBBOverlap.y = vecPlayerOOBBMaxs.y - vecBossOOBBMins.y;
		vecShiftDir.y = -1.0f;
	}
	if ( vecToPlayer.z >= 0 )
	{
		vecOBBOverlap.z = vecBossOOBBMaxs.z - vecPlayerOOBBMins.z;
		vecShiftDir.z = 1.0f;
	}
	else
	{
		vecOBBOverlap.z = 99999.9f;
		vecShiftDir.z = -1.0f;
	}

	// Push the player out of the way based on the shortest distance of overlap, bloated a bit
	Vector vecPlayerShift = pPlayer->GetAbsOrigin();
	if ( vecOBBOverlap.x < vecOBBOverlap.y )
	{
		if ( vecOBBOverlap.x < vecOBBOverlap.z )
			vecPlayerShift.x += vecShiftDir.x * ( vecOBBOverlap.x + 5.0 );
		else
			vecPlayerShift.z += vecShiftDir.z * ( vecOBBOverlap.z + 5.0 );
	}
	else if ( vecOBBOverlap.z < vecOBBOverlap.y )
		vecPlayerShift.z += vecShiftDir.z * ( vecOBBOverlap.z + 5.0 );
	else
		vecPlayerShift.y += vecShiftDir.y * ( vecOBBOverlap.y + 5.0 );

	trace_t trace;
	UTIL_TraceHull( vecPlayerShift, 
					vecPlayerShift, 
					vecPlayerMins, 
					vecPlayerMaxs, 
					MASK_PLAYERSOLID, 
					pPlayer, 
					COLLISION_GROUP_PLAYER_MOVEMENT, 
					&trace );

	// Try to move them into walls safely
	if ( trace.DidHit() )
	{
		UTIL_TraceHull( vecPlayerShift + Vector( 0, 0, 32 ), 
						vecPlayerShift, 
						vecPlayerMins, 
						vecPlayerMaxs, 
						MASK_PLAYERSOLID, 
						pPlayer, 
						COLLISION_GROUP_PLAYER_MOVEMENT, 
						&trace );

		// Kill them if we push them into a wall and still overlap
		if ( trace.startsolid )
		{
			CTakeDamageInfo info( this, this, 99999.9f, DMG_CRUSH );
			pPlayer->TakeDamage(info);

			return;
		}
		else
		{
			vecPlayerShift = trace.endpos;
		}
	}

	pPlayer->SetAbsOrigin( vecPlayerShift );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBaseBossLocomotion::GetRunSpeed( void ) const
{
	CTFBaseBoss *boss = (CTFBaseBoss *)GetBot()->GetEntity();
	return boss->GetMaxSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseBossLocomotion::FaceTowards( const Vector &target )
{
	CTFBaseBoss *pBoss = (CTFBaseBoss *)GetBot()->GetEntity();

	QAngle vecAngles = pBoss->GetLocalAngles();

	const float flYawDiff = UTIL_AngleDiff( UTIL_VecToYaw( target - GetFeet() ), vecAngles.y );
	const float flDeltaYaw = tf_base_boss_max_turn_rate.GetFloat() * GetUpdateInterval();

	if ( flYawDiff < -flDeltaYaw )
		vecAngles.y -= flDeltaYaw;
	else if ( flYawDiff > flDeltaYaw )
		vecAngles.y += flDeltaYaw;
	else
		vecAngles.y += flYawDiff;

	Vector vecForward, vecRight;
	pBoss->GetVectors( &vecForward, &vecRight, NULL );
	vecForward = CrossProduct( GetGroundNormal(), vecRight );

	const float flPitchDiff = UTIL_AngleDiff( UTIL_VecToPitch( vecForward ), vecAngles.x );
	const float flDeltaPitch = tf_base_boss_max_turn_rate.GetFloat() * GetUpdateInterval();

	if ( flPitchDiff < -flDeltaPitch )
		vecAngles.x -= flDeltaPitch;
	else if ( flPitchDiff > flDeltaPitch )
		vecAngles.x += flDeltaPitch;
	else
		vecAngles.x += flPitchDiff;

	pBoss->SetLocalAngles( vecAngles );
	pBoss->UpdateCollisionBounds();
}
