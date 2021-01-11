//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "fmtstr.h"
#include "entity_wheelofdoom.h"
#include "player_vs_environment/merasmus.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "props_shared.h"


class CMerasmusDancer : public CBaseAnimating
{
public:
	DECLARE_CLASS( CMerasmusDancer, CBaseAnimating );
	DECLARE_SERVERCLASS();

	CMerasmusDancer();
	virtual ~CMerasmusDancer() {}

	virtual void	Precache( void );
	virtual void	Spawn( void );

	void			DanceThink();

	bool			ShouldDelete( void ) const;

	void			PlayActivity( Activity act );
	void			PlaySequence( char const *pSequence );

	void			Dance();
	void			Vanish();
	void			HideStaff();
	void			BlastOff();

private:
	bool m_bApparate;
	CountdownTimer m_lifetimeDuration;
};

CMerasmusDancer::CMerasmusDancer()
{
	m_bApparate = false;
	m_lifetimeDuration.Invalidate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMerasmusDancer::Precache( void )
{
	BaseClass::Precache();

	int iMdlIndex = PrecacheModel( "models/bots/merasmus/merasmus.mdl" );
	PrecacheGibsForModel( iMdlIndex );

	PrecacheParticleSystem( "merasmus_tp" );

	PrecacheScriptSound( "Halloween.Merasmus_Hiding_Explode" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMerasmusDancer::Spawn( void )
{
	Precache();
	SetModel( "models/bots/merasmus/merasmus.mdl" );
	UseClientSideAnimation();

	m_lifetimeDuration.Invalidate();

	SetThink( &CMerasmusDancer::DanceThink );
	SetNextThink( gpGlobals->curtime );

	m_bApparate = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMerasmusDancer::DanceThink()
{
	if ( m_bApparate )
	{
		DispatchParticleEffect( "merasmus_tp", GetAbsOrigin(), GetAbsAngles() );
		EmitSound( "Halloween.Merasmus_Hiding_Explode" );
		m_bApparate = false;
	}

	if ( ShouldDelete() )
	{
		EmitSound( "Halloween.Merasmus_Hiding_Explode" );
		UTIL_Remove( this );

		return;
	}

	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMerasmusDancer::ShouldDelete( void ) const
{
	if ( m_lifetimeDuration.HasStarted() )
		return m_lifetimeDuration.IsElapsed();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMerasmusDancer::PlayActivity( Activity act )
{
	int iDesiredSeq = SelectWeightedSequence( act );
	if ( iDesiredSeq != 0 )
	{
		SetSequence( iDesiredSeq );
		SetCycle( 0 );
		ResetSequenceInfo();

		HideStaff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMerasmusDancer::PlaySequence( char const *pSequence )
{
	int iSequence = LookupSequence( pSequence );
	if ( iSequence != 0 )
	{
		SetSequence( iSequence );
		SetCycle( 0 );
		ResetSequenceInfo();

		HideStaff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMerasmusDancer::Dance()
{
	PlaySequence( "taunt06" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMerasmusDancer::Vanish()
{
	m_bApparate = true;
	m_lifetimeDuration.Start( 0 );
}

void CMerasmusDancer::HideStaff()
{
	int iBodyGroup = FindBodygroupByName( "staff" );
	SetBodygroup( iBodyGroup, 2 );
}

void CMerasmusDancer::BlastOff()
{
	m_bApparate = true;
	m_lifetimeDuration.Start( 0.3 );
	PlayActivity( ACT_FLY );
}

IMPLEMENT_SERVERCLASS_ST( CMerasmusDancer, DT_MerasmusDancer )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( merasmus_dancer, CMerasmusDancer );

#define WHEEL_SPIN_TIME			5.75f
#define WHEEL_EFFECT_DURATION	10.0f
#define	WHEEL_SPIN_TIME_BIAS	0.3f
#define	WHEEL_FASTEST_SPIN_RATE 0.1f
#define	WHEEL_SLOWEST_SPIN_RATE 0.55f

#define WHEEL_GROW_RATE			0.55f
#define WHEEL_SHRINK_RATE		0.55f

// TODO: More fitting names
#define WOD_BAD_EFFECT				(1<<0)
#define WOD_EFFECT_DOES_NOT_REAPPLY	(1<<1)

class CWheelOfDoomSpiral : public CBaseAnimating
{
	DECLARE_CLASS( CWheelOfDoomSpiral, CBaseAnimating );
public:

	DECLARE_DATADESC()

	virtual void	Precache( void );
	virtual void	Spawn( void );

	void			GrowThink( void );
	void			ShrinkThink( void );

	float m_flCurrentScale;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoomSpiral::Precache( void )
{
	PrecacheModel( "models/props_lakeside_event/wof_plane2.mdl" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoomSpiral::Spawn( void )
{
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoomSpiral::GrowThink( void )
{
	m_flCurrentScale += gpGlobals->frametime * WHEEL_GROW_RATE;
	if ( m_flCurrentScale >= 1.0f )
	{
		SetThink( NULL );
		m_flCurrentScale = 1.0f;
	}

	SetModelScale( m_flCurrentScale );

	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoomSpiral::ShrinkThink( void )
{
	m_flCurrentScale -= gpGlobals->frametime * WHEEL_SHRINK_RATE;
	if ( m_flCurrentScale <= 0.0f )
	{
		SetThink( NULL );
		AddEffects( EF_NODRAW );

		m_flCurrentScale = 0.0f;
	}

	SetModelScale( m_flCurrentScale );

	SetNextThink( gpGlobals->curtime );
}

BEGIN_DATADESC( CWheelOfDoomSpiral )
END_DATADESC()

LINK_ENTITY_TO_CLASS( wheel_of_doom_spiral, CWheelOfDoomSpiral );


BEGIN_DATADESC( CWheelOfDoom )
	DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "effect_duration" ),
	DEFINE_KEYFIELD( m_bHasSpiral, FIELD_BOOLEAN, "has_spiral" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Spin", InputSpin ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearAllEffects", InputClearAllEffects ),

	DEFINE_OUTPUT( m_EffectApplied, "OnEffectApplied" ),
	DEFINE_OUTPUT( m_EffectExpired, "OnEffectExpired" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( wheel_of_doom, CWheelOfDoom );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWheelOfDoom::CWheelOfDoom()
{
	AddEffects( EF_NODRAW );

	WOD_UberEffect *pUber = new WOD_UberEffect;
	pUber->szEffectName = "Uber";
	pUber->nType = 7;
	pUber->szBroadcastSound = "Halloween.MerasmusWheelUber";
	RegisterEffect( pUber );

	WOD_CritsEffect *pCrits = new WOD_CritsEffect;
	pCrits->szEffectName = "Crits";
	pCrits->nType = 8;
	pCrits->szBroadcastSound = "Halloween.MerasmusWheelCrits";
	RegisterEffect( pCrits );

	WOD_SuperSpeedEffect *pSpeed = new WOD_SuperSpeedEffect;
	pSpeed->szEffectName = "Super Speed";
	pSpeed->nType = 4;
	pSpeed->szBroadcastSound = "Halloween.MerasmusWheelSuperSpeed";
	RegisterEffect( pSpeed );

	WOD_SuperJumpEffect *pJump = new WOD_SuperJumpEffect;
	pJump->szEffectName = "Super Jump";
	pJump->nType = 2;
	pJump->szBroadcastSound = "Halloween.MerasmusWheelSuperJump";
	RegisterEffect( pJump );

	WOD_BigHeadEffect *pBigHead = new WOD_BigHeadEffect;
	pBigHead->szEffectName = "Big Head";
	pBigHead->nType = 6;
	pBigHead->szBroadcastSound = "Halloween.MerasmusWheelBigHead";
	RegisterEffect( pBigHead );

	WOD_SmallHeadEffect *pSmallHead = new WOD_SmallHeadEffect;
	pSmallHead->szEffectName = "Small Head";
	pSmallHead->nType = 3;
	pSmallHead->szBroadcastSound = "Halloween.MerasmusWheelShrunkHead";
	RegisterEffect( pSmallHead );

	WOD_LowGravityEffect *pLowGrav = new WOD_LowGravityEffect;
	pLowGrav->szEffectName = "Low Gravity";
	pLowGrav->nType = 5;
	pLowGrav->szBroadcastSound = "Halloween.MerasmusWheelGravity";
	RegisterEffect( pLowGrav );

	WOD_Dance *pDance = new WOD_Dance;
	pDance->szEffectName = "Dance";
	pDance->nType = 9;
	pDance->szBroadcastSound = "Halloween.MerasmusWheelDance";
	RegisterEffect( pDance, WOD_EFFECT_DOES_NOT_REAPPLY );

	WOD_Pee *pJarate = new WOD_Pee;
	pJarate->szEffectName = "Pee";
	pJarate->nType = 1;
	pJarate->szBroadcastSound = "Halloween.MerasmusWheelJarate";
	RegisterEffect( pJarate, WOD_BAD_EFFECT | WOD_EFFECT_DOES_NOT_REAPPLY );

	WOD_Burn *pBurn = new WOD_Burn;
	pBurn->szEffectName = "Burn";
	pBurn->nType = 1;
	pBurn->szBroadcastSound = "Halloween.MerasmusWheelBurn";
	RegisterEffect( pBurn, WOD_BAD_EFFECT | WOD_EFFECT_DOES_NOT_REAPPLY );

	WOD_Ghosts *pGhosts = new WOD_Ghosts;
	pGhosts->szEffectName = "Ghosts";
	pGhosts->nType = 1;
	pGhosts->szBroadcastSound = "Halloween.MerasmusWheelGhosts";
	RegisterEffect( pGhosts, WOD_BAD_EFFECT | WOD_EFFECT_DOES_NOT_REAPPLY );
}

CWheelOfDoom::~CWheelOfDoom()
{
	m_EffectManager.ClearEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::SpeakMagicConceptToAllPlayers( char const *szConcept )
{
	int iConcept = 0;
	if ( !Q_stricmp( szConcept, "Halloween.MerasmusWheelSuperSpeed" ) || !Q_stricmp( szConcept, "Halloween.MerasmusWheelCrits" ) || !Q_stricmp( szConcept, "Halloween.MerasmusWheelUber" ) )
		iConcept = MP_CONCEPT_MAGIC_GOOD;
	else if ( !Q_stricmp( szConcept, "Halloween.MerasmusWheelShrunkHead" ) )
		iConcept = MP_CONCEPT_MAGIC_SMALLHEAD;
	else if ( !Q_stricmp( szConcept, "Halloween.MerasmusWheelGravity" ) )
		iConcept = MP_CONCEPT_MAGIC_GRAVITY;
	else if ( !Q_stricmp( szConcept, "Halloween.MerasmusWheelBigHead" ) )
		iConcept = MP_CONCEPT_MAGIC_BIGHEAD;
	else if ( !Q_stricmp( szConcept, "Halloween.MerasmusWheelDance" ) )
		iConcept = MP_CONCEPT_MAGIC_DANCE;
	else return;

	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, TEAM_ANY );

	FOR_EACH_VEC( players, i )
		players[i]->SpeakConceptIfAllowed( iConcept );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::Spawn( void )
{
	Precache();
	SetModel( "models/props_lakeside_event/buff_plane.mdl" );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );

	ListenForGameEvent( "player_spawn" );

	SetThink( &CWheelOfDoom::IdleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( TFGameRules() )
	{
		TFGameRules()->SetActiveHalloweenEffect( -1 );
		TFGameRules()->SetTimeHalloweenEffectStarted( -1.0f );
		TFGameRules()->SetHalloweenEffectDuration( -1.0f );
	}

	if ( m_bHasSpiral )
	{
		CBaseAnimating *pSpiral = (CBaseAnimating *)CreateEntityByName( "wheel_of_doom_spiral" );
		if ( pSpiral )
		{
			pSpiral->SetModel( "models/props_lakeside_event/wof_plane2.mdl" );
			pSpiral->AddEffects( EF_NODRAW );
			pSpiral->SetAbsOrigin( GetAbsOrigin() );
			pSpiral->SetAbsAngles( GetAbsAngles() );
			pSpiral->SetParent( this );

			m_hSpiral = pSpiral;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::Precache( void )
{
	PrecacheModel("models/props_lakeside_event/buff_plane.mdl");
	PrecacheModel("models/props_lakeside_event/wof_plane2.mdl");

	PrecacheScriptSound("Halloween.WheelofFate");
	PrecacheScriptSound("Halloween.dance_howl");
	PrecacheScriptSound("Halloween.dance_loop");
	PrecacheScriptSound("Halloween.HeadlessBossAxeHitWorld");
	PrecacheScriptSound("Halloween.LightsOn");
	PrecacheScriptSound("Weapon_StickyBombLauncher.BoltForward");
	PrecacheScriptSound("TFPlayer.InvulnerableOff");

	m_EffectManager.Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "player_spawn" ) )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt( "userid" ) ) );
		if ( pPlayer )
		{
			m_EffectManager.ApplyAllEffectsToPlayer( pPlayer );
		}
	}
}

void CWheelOfDoom::PlaySound( char const *szSound )
{
	EmitSound( szSound );

	FOR_EACH_VEC( m_Wheels, i )
		m_Wheels[i]->EmitSound( szSound );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::SetSkin( int skin )
{
	m_nSkin = skin;

	FOR_EACH_VEC( m_Wheels, i )
		m_Wheels[i]->m_nSkin = skin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::SetScale( float scale )
{
	SetModelScale( scale );

	FOR_EACH_VEC( m_Wheels, i )
		m_Wheels[i]->SetModelScale( scale );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::StartSpin( void )
{
	RemoveEffects( EF_NODRAW );

	m_Wheels.Purge();

	CWheelOfDoom *pWheel = (CWheelOfDoom *)gEntList.FindEntityByClassname( NULL, "wheel_of_doom" );
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( 255, "Halloween.WheelofFate" );

		while ( pWheel )
		{
			if ( pWheel != this )
				m_Wheels.AddToTail( pWheel );

			pWheel = (CWheelOfDoom *)gEntList.FindEntityByClassname( pWheel, "wheel_of_doom" );
		}
	}

	if ( m_bHasSpiral )
	{
		m_hSpiral->RemoveEffects( EF_NODRAW );
		m_hSpiral->SetThink( &CWheelOfDoomSpiral::GrowThink );
		m_hSpiral->SetNextThink( gpGlobals->curtime );

		FOR_EACH_VEC( m_Wheels, i )
		{
			CWheelOfDoom *pWheel = m_Wheels[i];
			pWheel->RemoveEffects( EF_NODRAW );

			if ( pWheel->m_bHasSpiral )
			{
				pWheel->m_hSpiral->RemoveEffects( EF_NODRAW );
				pWheel->m_hSpiral->SetThink( &CWheelOfDoomSpiral::GrowThink );
				pWheel->m_hSpiral->SetNextThink( gpGlobals->curtime );
			}
		}
	}

	m_bHasSpiral = false;

	m_flSpinDuration = gpGlobals->curtime + WHEEL_SPIN_TIME;
	m_flNextThinkTick = CalcNextTickTime();
	m_flSpinAnnounce = gpGlobals->curtime + 1.6;
	m_flEffectEndTime = m_flSpinDuration + WHEEL_EFFECT_DURATION;

	SetThink( &CWheelOfDoom::SpinThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::SpinThink( void )
{
	if ( m_EffectManager.UpdateAndClearExpiredEffects() )
	{
		m_EffectExpired.FireOutput( this, this );

		TFGameRules()->SetActiveHalloweenEffect( -1 );
		TFGameRules()->SetTimeHalloweenEffectStarted( -1.0f );
		TFGameRules()->SetHalloweenEffectDuration( -1.0f );
	}

	if ( m_flSpinDuration > gpGlobals->curtime )
	{
		if( m_flNextThinkTick < gpGlobals->curtime )
		{
			int iRandomSkin = RandomInt( 1, 15 );
			iRandomSkin += ( m_nSkin == iRandomSkin );
			if ( iRandomSkin == 16 )
				iRandomSkin = 1;

			SetSkin( iRandomSkin );

			const float flScale = ( ( ( gpGlobals->curtime + WHEEL_SPIN_TIME ) - m_flSpinDuration ) * 0.10434783f ) + 0.3f;
			SetScale( flScale );

			m_flNextThinkTick = CalcNextTickTime();
		}
	}
	else if ( gpGlobals->curtime > m_flSpinDuration + 1.0f )
	{
		int iEffectSkin = m_EffectManager.AddEffect( m_pActiveEffect, m_flDuration );
		SetSkin( iEffectSkin );
		SetScale( 1.0f );

		m_EffectApplied.FireOutput( this, this );

		SetThink( &CWheelOfDoom::IdleThink );

		m_flSpinDuration = 0;
		m_flNextThinkTick = 0;
		m_pActiveEffect = nullptr;
	}

	if ( gpGlobals->curtime > m_flSpinAnnounce && !m_bHasSpiral )
	{
		m_bHasSpiral = true;

		CMerasmus *pMerasmus = assert_cast<CMerasmus *>( TFGameRules()->GetActiveBoss() );
		if ( pMerasmus )
		{
			pMerasmus->PlayHighPrioritySound( "Halloween.MerasmusWheelSpin" );
		}
		else
		{
			TFGameRules()->BroadcastSound( 255, "Halloween.MerasmusWheelSpin" );
		}
	}

	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::IdleThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( !m_EffectManager.UpdateAndClearExpiredEffects() )
		return;

	AddEffects( EF_NODRAW );
	SetSkin( 0 );

	m_EffectExpired.FireOutput( this, this );

	TFGameRules()->SetActiveHalloweenEffect( -1 );
	TFGameRules()->SetTimeHalloweenEffectStarted( -1.0f );
	TFGameRules()->SetHalloweenEffectDuration( -1.0f );

	if ( m_bHasSpiral )
	{
		m_hSpiral->SetThink( &CWheelOfDoomSpiral::ShrinkThink );
		m_hSpiral->SetNextThink( gpGlobals->curtime );

		FOR_EACH_VEC( m_Wheels, i )
		{
			CWheelOfDoom *pWheel = m_Wheels[i];
			if ( !pWheel->m_bHasSpiral )
				continue;

			pWheel->m_hSpiral->SetThink( &CWheelOfDoomSpiral::ShrinkThink );
			pWheel->m_hSpiral->SetNextThink( gpGlobals->curtime );
		}
	}
}

bool CWheelOfDoom::IsDoneBroadcastingEffectSound( void ) const
{
	return gpGlobals->curtime < m_flEffectEndTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::InputSpin( inputdata_t &data )
{
	m_pActiveEffect = GetRandomEffectWithFlags();
	StartSpin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::InputClearAllEffects( inputdata_t &data )
{
	m_EffectManager.ClearEffects();
}

void CWheelOfDoom::RegisterEffect( WOD_BaseEffect *pEffect, int flags )
{
	pEffect->fFlags = flags;
	m_Effects.AddToTail( pEffect );
}

CWheelOfDoom::WOD_BaseEffect *CWheelOfDoom::GetRandomEffectWithFlags( void ) const
{
	CUtlVector<WOD_BaseEffect *> effectsToPickFrom;
	int nNumFlagged = 0;
	int nNumOthers = 0;

	FOR_EACH_VEC( m_Effects, i )
	{
		if ( ( m_Effects[i]->fFlags & WOD_BAD_EFFECT ) == 0 )
		{
			effectsToPickFrom.AddToHead( m_Effects[i] );
			++nNumOthers;
		}
		else
		{
			++nNumFlagged;
			effectsToPickFrom.AddToTail( m_Effects[i] );
		}
	}

	if ( !effectsToPickFrom.IsEmpty() )
	{
		int iRandom = 0;
		if ( nNumFlagged != 0 )
		{
			iRandom = RandomInt( 0, nNumOthers );
			if ( iRandom == nNumOthers )
				iRandom = nNumOthers + RandomInt( 0, nNumFlagged - 1 );
		}
		else
		{
			iRandom = RandomInt( 0, nNumOthers - 1 );
		}

		return effectsToPickFrom[ iRandom ];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWheelOfDoom::CalcNextTickTime( void ) const
{
	const float flBias = Bias( CalcSpinCompletion(), WHEEL_SPIN_TIME_BIAS );
	const float flInvBias = 1.0 - flBias;
	const float flSpinDelta = ( flBias * WHEEL_SLOWEST_SPIN_RATE ) + ( flInvBias * WHEEL_FASTEST_SPIN_RATE );
	return ( flSpinDelta + gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWheelOfDoom::CalcSpinCompletion( void ) const
{
	return ( (( gpGlobals->curtime + WHEEL_SPIN_TIME ) - m_flSpinDuration ) / WHEEL_SPIN_TIME );
}

//-----------------------------------------------------------------------------
// Purpose: Debug
//-----------------------------------------------------------------------------
void CWheelOfDoom::DBG_ApplyEffectByName( char const *szEffectName )
{
	FOR_EACH_VEC( m_Effects, i )
	{
		if ( FStrEq( szEffectName, m_Effects[i]->szEffectName ) )
			m_EffectManager.AddEffect( m_Effects[i], m_Effects[i]->flDuration );
	}
}
void CWheelOfDoom::DBG_ApplyEffectByName( CCommand const &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "You must provide an effect name" );
		return;
	}

	DBG_ApplyEffectByName( args.Arg( 1 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::ApplyAttributeToAllPlayers( char const *szAtribName, float flValue )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players );

	FOR_EACH_VEC( players, i )
		ApplyAttributeToPlayer( players[i], szAtribName, flValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::ApplyAttributeToPlayer( CTFPlayer *pTarget, char const *szAtribName, float flValue )
{
	CEconAttributeDefinition *pAttrib = GetItemSchema()->GetAttributeDefinitionByName( szAtribName );
	pTarget->GetAttributeList()->SetRuntimeAttributeValue( pAttrib, flValue );
	pTarget->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::RemoveAttributeFromAllPlayers( char const *szAtribName )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players );

	FOR_EACH_VEC( players, i )
		RemoveAttributeFromPlayer( players[i], szAtribName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::RemoveAttributeFromPlayer( CTFPlayer *pTarget, char const *szAtribName )
{
	CEconAttributeDefinition *pAttrib = GetItemSchema()->GetAttributeDefinitionByName( szAtribName );
	pTarget->GetAttributeList()->RemoveAttribute( pAttrib );
	pTarget->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::WOD_BaseEffect::InitEffect( float flDuration )
{
	this->flDuration = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWheelOfDoom::EffectManager::AddEffect( WOD_BaseEffect *pEffect, float flDuration )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players );

	pEffect->InitEffect( flDuration );
	pEffect->ActivateEffect( players );

	const float flExpireTime = pEffect->flDuration - gpGlobals->curtime;
	DevMsg( "[WHEEL OF DOOM]\t Activating: \"%s\" set to expire in %3.2fs\n", pEffect->szEffectName, flExpireTime );

	if ( TFGameRules() )
	{
		CMerasmus *pMerasmus = assert_cast<CMerasmus *>( TFGameRules()->GetActiveBoss() );
		if ( pMerasmus )
		{
			pMerasmus->PlayHighPrioritySound( pEffect->szBroadcastSound );
		}
		else
		{
			TFGameRules()->BroadcastSound( 255, pEffect->szBroadcastSound );
		}

		TFGameRules()->SetActiveHalloweenEffect( pEffect->nType );
		TFGameRules()->SetTimeHalloweenEffectStarted( gpGlobals->curtime );
		TFGameRules()->SetHalloweenEffectDuration( pEffect->flDuration - gpGlobals->curtime );

		SpeakMagicConceptToAllPlayers( pEffect->szBroadcastSound );
	}

	m_Effects.AddToTail( pEffect );

	return pEffect->nType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::EffectManager::ClearEffects( void )
{
	FOR_EACH_VEC( m_Effects, i )
	{
		DevMsg( "[WHEEL OF DOOM]\t Deactivating: %s\n", m_Effects[i]->szEffectName );

		CUtlVector<CTFPlayer *> players;
		CollectPlayers( &players );

		m_Effects[i]->DeactivateEffect( players );
	}

	m_Effects.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::EffectManager::ApplyAllEffectsToPlayer( CTFPlayer *pTarget )
{
	CUtlVector<CTFPlayer *> player;
	player.AddToTail( pTarget );

	FOR_EACH_VEC( m_Effects, i )
	{
		if ( m_Effects[i]->fFlags & WOD_EFFECT_DOES_NOT_REAPPLY )
			continue;

		m_Effects[i]->ActivateEffect( player );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWheelOfDoom::EffectManager::Precache( void )
{
	FOR_EACH_VEC( m_Effects, i )
	{
		PrecacheScriptSound( m_Effects[i]->szBroadcastSound );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if an active effect has expired
//-----------------------------------------------------------------------------
bool CWheelOfDoom::EffectManager::UpdateAndClearExpiredEffects( void )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players );

	bool bSomethingExpired = false;
	FOR_EACH_VEC_BACK( m_Effects, i )
	{
		WOD_BaseEffect *pEffect = m_Effects[i];
		pEffect->UpdateEffect( players );

		if ( pEffect->flDuration >= gpGlobals->curtime )
			continue;

		pEffect->DeactivateEffect( players );
		bSomethingExpired = true;

		m_Effects.Remove( i );
	}

	return bSomethingExpired;
}

void CWheelOfDoom::WOD_Burn::InitEffect( float flDuration )
{
	this->flDuration = gpGlobals->curtime + WHEEL_EFFECT_DURATION;
}

void CWheelOfDoom::WOD_Burn::ActivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
		data[i]->m_Shared.Burn( NULL, flDuration - gpGlobals->curtime );
}

void CWheelOfDoom::WOD_Pee::ActivateEffect( EffectData_t &data )
{
	m_SpawnPoints.RemoveAll();

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, "spawn_cloud" );
	while ( pEntity )
	{
		CHandle<CBaseEntity> hndl( pEntity );
		m_SpawnPoints.AddToTail( hndl );

		pEntity = gEntList.FindEntityByName( pEntity, "spawn_cloud" );
	}

	m_flNextSpawn = gpGlobals->curtime + 0.25f;
}

void CWheelOfDoom::WOD_Pee::UpdateEffect( EffectData_t &data )
{
	if ( m_SpawnPoints.IsEmpty() )
		return;

	if ( m_flNextSpawn > gpGlobals->curtime )
		return;

	m_flNextSpawn = RandomFloat( 0.2, 0.5 ) + gpGlobals->curtime;

	CBaseEntity *pEntity = m_SpawnPoints.Random();

	Vector vecMins = pEntity->WorldAlignMins();
	Vector vecMaxs = pEntity->WorldAlignMaxs();

	Vector vecLoc = pEntity->GetAbsOrigin();
	vecLoc.x += RandomFloat( vecMins.x, vecMaxs.x );
	vecLoc.y += RandomFloat( vecMins.y, vecMaxs.y );
	vecLoc.z += RandomFloat( vecMins.z, vecMaxs.z );

	CBaseEntity *pProjectile = CBaseEntity::CreateNoSpawn( "tf_projectile_jar", vecLoc, vec3_angle );
	DispatchSpawn( pProjectile );

	IPhysicsObject *pPhys = pProjectile->VPhysicsGetObject();
	if ( pPhys )
	{
		AngularImpulse impulse( RandomFloat( -300.0, 300.0 ), RandomFloat( -300.0, 300.0 ), RandomFloat( -300.0, 300.0 ) );
		pPhys->AddVelocity( &vec3_origin, &impulse );
	}
}

void CWheelOfDoom::WOD_Ghosts::ActivateEffect( EffectData_t &data )
{
	if( TFGameRules() )
	{
		TFGameRules()->BeginHaunting( 4, ( flDuration - gpGlobals->curtime ) / 2, flDuration - gpGlobals->curtime );
	}
}

void CWheelOfDoom::WOD_Ghosts::DeactivateEffect( EffectData_t &data )
{
}

void CWheelOfDoom::WOD_UberEffect::InitEffect( float flDuration )
{
	this->flDuration = Min( flDuration, WHEEL_EFFECT_DURATION ) + gpGlobals->curtime;
}

void CWheelOfDoom::WOD_UberEffect::ActivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
		data[i]->m_Shared.AddCond( TF_COND_INVULNERABLE, flDuration - gpGlobals->curtime );
}

void CWheelOfDoom::WOD_CritsEffect::ActivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
		data[i]->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, flDuration - gpGlobals->curtime );
}

void CWheelOfDoom::WOD_SuperJumpEffect::ActivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
	{
		ApplyAttributeToPlayer( data[i], "major increased jump height", 3.f );
		ApplyAttributeToPlayer( data[i], "cancel falling damage", 1.f );
	}
}

void CWheelOfDoom::WOD_SuperJumpEffect::DeactivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
	{
		RemoveAttributeFromPlayer( data[i], "major increased jump height" );
		RemoveAttributeFromPlayer( data[i], "cancel falling damage" );
	}
}

void CWheelOfDoom::WOD_SuperSpeedEffect::ActivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
		ApplyAttributeToPlayer( data[i], "major move speed bonus", 2.f );
}

void CWheelOfDoom::WOD_SuperSpeedEffect::DeactivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
		RemoveAttributeFromPlayer( data[i], "major move speed bonus" );
}

void CWheelOfDoom::WOD_LowGravityEffect::ActivateEffect( EffectData_t &data )
{
	TFGameRules()->SetGravityMultiplier( 0.25f );
}

void CWheelOfDoom::WOD_LowGravityEffect::DeactivateEffect( EffectData_t &data )
{
	TFGameRules()->SetGravityMultiplier( 1.0f );
}

void CWheelOfDoom::WOD_SmallHeadEffect::ActivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
	{
		ApplyAttributeToPlayer( data[i], "voice pitch scale", 1.3f );
		ApplyAttributeToPlayer( data[i], "head scale", 0.5f );
	}
}

void CWheelOfDoom::WOD_SmallHeadEffect::DeactivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
	{
		RemoveAttributeFromPlayer( data[i], "voice pitch scale" );
		RemoveAttributeFromPlayer( data[i], "head scale" );
	}
}

void CWheelOfDoom::WOD_BigHeadEffect::ActivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
	{
		ApplyAttributeToPlayer( data[i], "voice pitch scale", 0.85f );
		ApplyAttributeToPlayer( data[i], "head scale", 3.0f );
	}
}

void CWheelOfDoom::WOD_BigHeadEffect::DeactivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
	{
		RemoveAttributeFromPlayer( data[i], "voice pitch scale" );
		RemoveAttributeFromPlayer( data[i], "head scale" );
	}
}

void CWheelOfDoom::WOD_Dance::InitEffect( float flDuration )
{
	this->flDuration = gpGlobals->curtime + 8.0;
	m_flNextDance = gpGlobals->curtime + 1.5;

	m_iSpawnSide = RandomInt( 0, 1 );

	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, TEAM_ANY, true );
	
	int nDancers = 0;
	float flMaxY = FLT_MAX;
	float flMinY = FLT_MIN;
	float flAvgX = 0.0f;
	float flZ = 0.0f;

	FOR_EACH_VEC( players, i )
	{
		CTFPlayer *pPlayer = players[i];
		CBaseEntity *pPoint = gEntList.FindEntityByName( NULL, CFmtStr( "dance_teleport_%s%d", 
																		pPlayer->GetTeamNumber() == TF_TEAM_RED ? "red" : "blue", 
																		GetNumOfTeamDancing( pPlayer->GetTeamNumber() ) ) );
		if ( pPoint )
		{
			Dancer_t pDancer ={
				pPoint->GetAbsOrigin(),
				pPoint->GetAbsAngles(),
				pPlayer
			};

			SlamPosAndAngles( pPlayer, pDancer.pos, pDancer.ang );

			flMaxY = Min( flMaxY, pDancer.pos.y );
			flMinY = Max( flMinY, pDancer.pos.y );
			flZ = pDancer.pos.z;
			flAvgX += pDancer.pos.x;

			pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_THRILLER );

			m_Dancers.AddToTail( pDancer );
			nDancers++;
		}
	}

	flAvgX /= nDancers;

	MerasmusCreateInfo_t leftSide(
		Vector( flAvgX, flMaxY, flZ ),
		QAngle( 0, 90.f, 0 )
	);
	m_CreateInfos.AddToTail( leftSide );

	MerasmusCreateInfo_t rightSide(
		Vector( flAvgX, flMinY, flZ ),
		QAngle( 0, -90.f, 0 )
	);
	m_CreateInfos.AddToTail( rightSide );
}

void CWheelOfDoom::WOD_Dance::UpdateEffect( EffectData_t &data )
{
	bool bSwitchSides = gpGlobals->curtime > m_flNextDance;
	if ( bSwitchSides )
		m_flNextDance = gpGlobals->curtime + 3.5;

	FOR_EACH_VEC_BACK( m_Dancers, i )
	{
		Dancer_t const &pDancer = m_Dancers[i];
		CTFPlayer *pPlayer = pDancer.hPlayer.Get();
		if ( pPlayer )
		{
			if ( bSwitchSides )
				pPlayer->Taunt();

			if ( m_flNextDance - gpGlobals->curtime < 0.2 )
				SlamPosAndAngles( pPlayer, pDancer.pos, pDancer.ang );
		}
		else
		{
			m_Dancers.Remove( i );
		}
	}

	if ( bSwitchSides )
	{
		if ( m_hDancer )
		{
			m_hDancer->Vanish();
			m_hDancer = NULL;
		}

		MerasmusCreateInfo_t const &info = m_CreateInfos[ m_iSpawnSide % m_CreateInfos.Count() ];
		CMerasmusDancer *pDancer = (CMerasmusDancer *)CBaseEntity::Create( "merasmus_dancer", info.pos, info.dir );
		if ( pDancer )
		{
			m_hDancer = pDancer;
			m_hDancer->Dance();
		}

		m_iSpawnSide = ( m_iSpawnSide + 1 ) % m_CreateInfos.Count();
	}
}

void CWheelOfDoom::WOD_Dance::DeactivateEffect( EffectData_t &data )
{
	FOR_EACH_VEC( data, i )
		data[i]->m_Shared.RemoveCond( TF_COND_HALLOWEEN_THRILLER );

	m_hDancer->BlastOff();

	m_CreateInfos.Purge();
	m_Dancers.Purge();
}

int CWheelOfDoom::WOD_Dance::GetNumOfTeamDancing( int iTeam )
{
	int iCount = 0;
	FOR_EACH_VEC( m_Dancers, i )
	{
		if ( m_Dancers[i].hPlayer != nullptr && m_Dancers[i].hPlayer->GetTeamNumber() == iTeam )
			++iCount;
	}

	return iCount;
}

void CWheelOfDoom::WOD_Dance::SlamPosAndAngles(CTFPlayer *pTarget, Vector const &pos, QAngle const &ang)
{
	pTarget->Teleport( &pos, &ang, &vec3_origin );
	pTarget->pl.v_angle = ang;
}
