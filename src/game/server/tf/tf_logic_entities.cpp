#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_logic_entities.h"
#include "team_control_point_master.h"
#include "eventqueue.h"


BEGIN_DATADESC( CArenaLogic )
	DEFINE_KEYFIELD( m_flTimeToEnableCapPoint, FIELD_FLOAT, "CapEnableDelay" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

	DEFINE_OUTPUT( m_OnArenaRoundStart, "OnArenaRoundStart" ),
	DEFINE_OUTPUT( m_OnCapEnabled, "OnCapEnabled" ),

	//DEFINE_THINKFUNC( ArenaLogicThink ),
END_DATADESC()

BEGIN_DATADESC( CKothLogic )
	DEFINE_KEYFIELD( m_iTimerLength, FIELD_INTEGER, "timer_length" ),
	DEFINE_KEYFIELD( m_iUnlockPoint, FIELD_INTEGER, "unlock_point" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTimer", InputAddBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTimer", InputAddRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddGreenTimer", InputAddGreenTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddYellowTimer", InputAddYellowTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTimer", InputSetBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTimer", InputSetRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetGreenTimer", InputSetGreenTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetYellowTimer", InputSetYellowTimer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
END_DATADESC()

BEGIN_DATADESC( CCPTimerLogic )
	DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),
	DEFINE_KEYFIELD( m_nTimerLength, FIELD_INTEGER, "timer_length" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	// Outputs.
	DEFINE_OUTPUT( m_onCountdownStart, "OnCountdownStart" ),
	DEFINE_OUTPUT( m_onCountdown15SecRemain, "OnCountdown15SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown10SecRemain, "OnCountdown10SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown5SecRemain, "OnCountdown5SecRemain" ),
	DEFINE_OUTPUT( m_onCountdownEnd, "OnCountdownEnd" ),
END_DATADESC()

BEGIN_DATADESC( CTFHolidayEntity )
	DEFINE_KEYFIELD( m_nHolidayType, FIELD_INTEGER, "holiday_type" ),
	DEFINE_KEYFIELD( m_nTauntInHell, FIELD_INTEGER, "tauntInHell" ),
	DEFINE_KEYFIELD( m_nAllowHaunting, FIELD_INTEGER, "allowHaunting" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "HalloweenSetUsingSpells", InputHalloweenSetUsingSpells ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Halloween2013TeleportToHell", InputHalloweenTeleportToHell ),
END_DATADESC()

BEGIN_DATADESC( CLogicOnHoliday )
	DEFINE_INPUTFUNC( FIELD_VOID, "Fire", InputFire ),
	DEFINE_OUTPUT( m_IsAprilFools, "IsAprilFools" ),
	DEFINE_OUTPUT( m_IsFullMoon, "IsFullMoon" ),
	DEFINE_OUTPUT( m_IsHalloween, "IsHalloween" ),
	DEFINE_OUTPUT( m_IsSmissmas, "IsSmissmas" ),
	DEFINE_OUTPUT( m_IsTFBirthday, "IsTFBirthday" ),
	DEFINE_OUTPUT( m_IsValentines, "IsValentines" ),
	DEFINE_OUTPUT( m_IsNothing, "IsNothing" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_arena, CArenaLogic );
LINK_ENTITY_TO_CLASS( tf_logic_koth, CKothLogic );
LINK_ENTITY_TO_CLASS( tf_logic_cp_timer, CCPTimerLogic );
LINK_ENTITY_TO_CLASS( tf_logic_hybrid_ctf_cp, CHybridMap_CTF_CP );
LINK_ENTITY_TO_CLASS( tf_logic_multiple_escort, CMultipleEscortLogic );
LINK_ENTITY_TO_CLASS( tf_logic_medieval, CMedievalLogic );
LINK_ENTITY_TO_CLASS( tf_logic_holiday, CTFHolidayEntity );
LINK_ENTITY_TO_CLASS( tf_logic_on_holiday, CLogicOnHoliday );

//=============================================================================
// Arena Logic
//=============================================================================

void CArenaLogic::Spawn( void )
{
	BaseClass::Spawn();
	//SetThink( &CArenaLogic::ArenaLogicThink );
	//SetNextThink( gpGlobals->curtime );
}

void CArenaLogic::OnCapEnabled( void )
{
	if ( m_bCapUnlocked == false )
	{
		m_bCapUnlocked = true;
		m_OnCapEnabled.FireOutput( this, this );
	}
}

/*void CArenaLogic::ArenaLogicThink( void )
{
	// Live TF2 checks m_fCapEnableTime from TFGameRules here.
	SetNextThink( gpGlobals->curtime + 0.1 );

#ifdef GAME_DLL
	if ( TFGameRules()->State_Get() == GR_STATE_STALEMATE )
	{
		m_bCapUnlocked = true;
		m_OnCapEnabled.FireOutput(this, this);
	}
#endif
}*/

void CArenaLogic::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		variant_t sVariant;
		sVariant.SetInt( m_flTimeToEnableCapPoint );
		pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );
		g_EventQueue.AddEvent( pPoint, "SetUnlockTime", sVariant, 0.1, NULL, NULL );
	}
}

//=============================================================================
// King of the Hill Logic
//=============================================================================
CKothLogic::CKothLogic()
{
	m_iTimerLength = 180;
	m_iUnlockPoint = 30;
}

void CKothLogic::InputRoundSpawn( inputdata_t &inputdata )
{
	variant_t sVariant;

	if ( TFGameRules() )
	{
		sVariant.SetInt( m_iTimerLength );

		TFGameRules()->SetBlueKothRoundTimer( (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
		{
			TFGameRules()->GetBlueKothRoundTimer()->SetName( MAKE_STRING( "zz_blue_koth_timer" ) );
			TFGameRules()->GetBlueKothRoundTimer()->SetShowInHud( false );
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetBlueKothRoundTimer()->ChangeTeam( TF_TEAM_BLUE );
		}

		TFGameRules()->SetRedKothRoundTimer( (CTeamRoundTimer *)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetRedKothRoundTimer() )
		{
			TFGameRules()->GetRedKothRoundTimer()->SetName( MAKE_STRING( "zz_red_koth_timer" ) );
			TFGameRules()->GetRedKothRoundTimer()->SetShowInHud( false );
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetRedKothRoundTimer()->ChangeTeam( TF_TEAM_RED );
		}

		if ( TFGameRules()->IsFourTeamGame() )
		{
			TFGameRules()->SetGreenKothRoundTimer( (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

			if ( TFGameRules()->GetGreenKothRoundTimer() )
			{
				TFGameRules()->GetGreenKothRoundTimer()->SetName( MAKE_STRING( "zz_green_koth_timer" ) );
				TFGameRules()->GetGreenKothRoundTimer()->SetShowInHud( false );
				TFGameRules()->GetGreenKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				TFGameRules()->GetGreenKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
				TFGameRules()->GetGreenKothRoundTimer()->ChangeTeam( TF_TEAM_GREEN );
			}

			TFGameRules()->SetYellowKothRoundTimer( (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

			if ( TFGameRules()->GetYellowKothRoundTimer() )
			{
				TFGameRules()->GetYellowKothRoundTimer()->SetName( MAKE_STRING( "zz_yellow_koth_timer" ) );
				TFGameRules()->GetYellowKothRoundTimer()->SetShowInHud( false );
				TFGameRules()->GetYellowKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				TFGameRules()->GetYellowKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
				TFGameRules()->GetYellowKothRoundTimer()->ChangeTeam( TF_TEAM_RED );
			}
		}
	}
}

void CKothLogic::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		variant_t sVariant;
		sVariant.SetInt( m_iUnlockPoint );
		pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );
		g_EventQueue.AddEvent( pPoint, "SetUnlockTime", sVariant, 0.1, NULL, NULL );
	}
}

void CKothLogic::InputAddBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputAddRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputAddGreenTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetGreenKothRoundTimer() )
	{
		TFGameRules()->GetGreenKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputAddYellowTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetYellowKothRoundTimer() )
	{
		TFGameRules()->GetYellowKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetGreenTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetGreenKothRoundTimer() )
	{
		TFGameRules()->GetGreenKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CKothLogic::InputSetYellowTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetYellowKothRoundTimer() )
	{
		TFGameRules()->GetYellowKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

//=============================================================================
// Control Point Logic
//=============================================================================
CCPTimerLogic::CCPTimerLogic()
{
	m_bOn5SecRemain = true;
	m_bOn10SecRemain = true;
	m_bOn15SecRemain = true;
	m_bRestartTimer = true;
}

void CCPTimerLogic::Spawn( void )
{
	BaseClass::Spawn();
	SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, "CCPTimerLogicThink" );
}

void CCPTimerLogic::Think( void )
{
	if ( !TFGameRules() || !ObjectiveResource() )
		return;

	const float flThinkInterval = 0.15f;

	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		m_TimeRemaining.Invalidate();
		SetNextThink( gpGlobals->curtime + flThinkInterval );
	}

	if ( m_hPoint )
	{
		int iIndex = m_hPoint->GetPointIndex();

		if ( TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, iIndex ) )
		{
			if ( m_TimeRemaining.GetRemainingTime() <= 0.0 && m_bRestartTimer )
			{
				m_TimeRemaining.Start( flThinkInterval + m_nTimerLength );
				m_onCountdownStart.FireOutput( this, this );
				ObjectiveResource()->SetCPTimerTime( iIndex, gpGlobals->curtime + m_nTimerLength );
				m_bRestartTimer = false;
			}
			else
			{
				if ( flThinkInterval <= m_TimeRemaining.GetRemainingTime() )
				{
					// I don't think is actually doing anything
					float flTime = m_TimeRemaining.GetRemainingTime() - flThinkInterval;
					if ( flTime <= 15.0 && m_bOn15SecRemain )
					{
						m_bOn15SecRemain = false;
					}
					else if ( flTime <= 10.0 && m_bOn10SecRemain )
					{
						m_bOn10SecRemain = false;
					}
					else if ( flTime <= 5.0 && m_bOn5SecRemain )
					{
						m_bOn5SecRemain = false;
					}
				}
				else
				{
					if ( ObjectiveResource()->GetNumControlPoints() <= iIndex || ObjectiveResource()->GetCappingTeam( iIndex ) == TEAM_UNASSIGNED )
					{
						m_TimeRemaining.Invalidate();
						m_onCountdownEnd.FireOutput( this, this );
						m_bOn15SecRemain = true;
						m_bOn10SecRemain = true;
						m_bOn5SecRemain = true;
						m_bRestartTimer = true;
						ObjectiveResource()->SetCPTimerTime( iIndex, -1.0f );
						SetNextThink( TICK_NEVER_THINK );
					}
				}
			}
		}
		else
		{
			m_TimeRemaining.Invalidate();
			m_bRestartTimer = true;
		}
	}

	SetNextThink( gpGlobals->curtime + flThinkInterval );
}

void CCPTimerLogic::InputRoundSpawn( inputdata_t &inputdata )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		// We need to re-find our control point, because they're recreated over round restarts
		m_hPoint = dynamic_cast<CTeamControlPoint *>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( !m_hPoint )
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING( m_iszControlPointName ) );
		}
	}
}

//=============================================================================
// Holiday Logic
//=============================================================================
CTFHolidayEntity::CTFHolidayEntity()
{
	ListenForGameEvent( "player_turned_to_ghost" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_disconnect" );
}

void CTFHolidayEntity::FireGameEvent( IGameEvent *event )
{
}

void CTFHolidayEntity::InputHalloweenSetUsingSpells( inputdata_t &inputdata )
{
}

void CTFHolidayEntity::InputHalloweenTeleportToHell( inputdata_t &inputdata )
{
}

void CLogicOnHoliday::InputFire( inputdata_t & )
{
	bool isAprilFools = TF_IsHolidayActive( kHoliday_AprilFools );
	bool isFullMoon = TF_IsHolidayActive( kHoliday_FullMoon );
	bool isHalloween = TF_IsHolidayActive( kHoliday_Halloween );
	bool isSmissmas = TF_IsHolidayActive( kHoliday_Christmas );
	bool isTFBirthday = TF_IsHolidayActive( kHoliday_TF2Birthday );
	bool isValentines = TF_IsHolidayActive( kHoliday_ValentinesDay );
	bool isNothing = !(isTFBirthday || isHalloween || isSmissmas || isValentines || isFullMoon || isAprilFools);

	if ( isNothing )
	{ 
		m_IsNothing.FireOutput( this, this );
		return;
	}

	if ( isAprilFools )
		m_IsAprilFools.FireOutput( this, this );
	if ( isFullMoon )
		m_IsFullMoon.FireOutput( this, this );
	if ( isHalloween )
		m_IsHalloween.FireOutput( this, this );
	if ( isSmissmas )
		m_IsSmissmas.FireOutput( this, this );
	if ( isTFBirthday )
		m_IsTFBirthday.FireOutput( this, this );
	if ( isValentines )
		m_IsValentines.FireOutput( this, this );
}
