//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectSentrygun
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tf_player.h"
#include "vgui_bitmapbutton.h"
#include "vgui/ILocalize.h"
#include "tf_fx_muzzleflash.h"
#include "eventlist.h"
#include "hintsystem.h"
#include <vgui_controls/ProgressBar.h>
#include "tf_weapon_laser_pointer.h"


#include "c_obj_sentrygun.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SENTRY_SHIELD "models/buildables/sentry_shield.mdl"

using namespace vgui;


IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( C_ObjectSentrygun, DT_SentrygunLocalData )
	RecvPropInt( RECVINFO( m_iKills ) ),
	RecvPropInt( RECVINFO( m_iAssists ) ),
END_NETWORK_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_ObjectSentrygun, DT_ObjectSentrygun, CObjectSentrygun )
	RecvPropInt( RECVINFO( m_iAmmoShells ) ),
	RecvPropInt( RECVINFO( m_iAmmoRockets ) ),
	RecvPropInt( RECVINFO( m_iState ) ),
	RecvPropVector( RECVINFO( m_vecEnd ) ),
	RecvPropDataTable( "SentrygunLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SentrygunLocalData ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ObjectSentrygun::C_ObjectSentrygun() :
	m_iv_vecEnd( "C_ObjectSentrygun::m_iv_vecEnd" )
{
	m_pDamageEffects = NULL;
	m_pLaserBeam = NULL;
	m_iOldUpgradeLevel = 0;
	m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;

	AddVar( &m_vecEnd, &m_iv_vecEnd, LATCH_ANIMATION_VAR, true );
}

C_ObjectSentrygun::~C_ObjectSentrygun() 
{ 
	if ( m_pLaserBeam )
		DestroyLaserBeam();
	if ( m_pShield )
		DestroyShield();
	if ( m_pSiren )
		DestroySiren();
}

void C_ObjectSentrygun::GetAmmoCount( int &iShells, int &iMaxShells, int &iRockets, int & iMaxRockets )
{
	iShells = m_iAmmoShells;
	iMaxShells = m_iMaxAmmoShells;
	iRockets = m_iAmmoRockets;
	iMaxRockets = SENTRYGUN_MAX_ROCKETS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::UpgradeLevelChanged()
{
	switch( m_iUpgradeLevel )
	{
	case 1:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_1, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
			break;
		}
	case 2:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_2, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_2;
			break;
		}
	case 3:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_3, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_3;
			break;
		}
	default: 
		{ 
			Assert( 0 ); 
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldBodygroups = GetBody();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_iOldUpgradeLevel != m_iUpgradeLevel )
	{
		UpgradeLevelChanged();
		m_iOldUpgradeLevel = m_iUpgradeLevel;
	}

	// Check if we started carrying the sentry between updates
	if ( IsBeingCarried() && !m_bCarriedOld )
	{
		DestroySiren();
	}
	m_bCarriedOld = IsBeingCarried();

	// intercept bodygroup sets from the server
	// we aren't clientsideanimating, but we don't want the server setting our
	// bodygroup while we are placing
	if ( IsPlacing() && m_iOldBodygroups != GetBody() )
	{
		m_nBody = m_iOldBodygroups;
	}
}

void C_ObjectSentrygun::GetStatusText( wchar_t *pStatus, int iMaxStatusLen )
{
	float flHealthPercent = (float)GetHealth() / (float)GetMaxHealth();
	wchar_t wszHealthPercent[32];
	_snwprintf(wszHealthPercent, sizeof(wszHealthPercent)/sizeof(wchar_t) - 1, L"%d%%", (int)( flHealthPercent * 100 ) );

	if ( IsBuilding() )
	{
		// "Sentry Gun Building... 85%" 

		wchar_t *pszTemplate = g_pVGuiLocalize->Find( "#TF_ObjStatus_SentryGun_Building" );

		if ( pszTemplate )
		{
			g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszTemplate,
				1,
				wszHealthPercent );
		}
	}
	else if ( m_iUpgradeLevel < 3 )
	{
		// "Sentry Gun ( Level 1 )  Health 100%  Shells 89/100" 

		wchar_t wszLevel[16]; 

		_snwprintf(wszLevel, sizeof(wszLevel)/sizeof(wchar_t) - 1, L"%d", m_iUpgradeLevel );

		wchar_t wszShells[16];
		_snwprintf(wszShells, sizeof(wszShells)/sizeof(wchar_t) - 1, L"%d/%d", m_iAmmoShells, m_iMaxAmmoShells );

		wchar_t *pszTemplate = g_pVGuiLocalize->Find( "#TF_ObjStatus_SentryGun_Level1" );

		if ( pszTemplate )
		{
			g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszTemplate,
				3,
				wszLevel,
				wszHealthPercent,
				wszShells );
		}
	}
	else
	{
		// "Sentry Gun ( Level 3 )  Health 100%  Shells 89/100  Rockets 18/20" 

		wchar_t wszShells[16];
		_snwprintf(wszShells, sizeof(wszShells)/sizeof(wchar_t) - 1, L"%d/%d", m_iAmmoShells, m_iMaxAmmoShells );

		wchar_t wszRockets[16];
		_snwprintf(wszRockets, sizeof(wszRockets)/sizeof(wchar_t) - 1, L"%d/%d", m_iAmmoRockets, SENTRYGUN_MAX_ROCKETS );

		wchar_t *pszTemplate = g_pVGuiLocalize->Find( "#TF_ObjStatus_SentryGun_Level3" );

		if ( pszTemplate )
		{
			g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszTemplate,
				3,
				wszHealthPercent,
				wszShells,
				wszRockets );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::DisplayHintTo( C_BasePlayer *pPlayer )
{
	bool bHintPlayed = false;

	C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if ( InSameTeam( pPlayer ) )
	{
		// We're looking at a friendly object. 
		if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// If the sentrygun can be upgraded, and I can afford it, let me know
			if ( GetHealth() == GetMaxHealth() && GetUpgradeLevel() < 3 )
			{
				if ( pTFPlayer->GetBuildResources() >= SENTRYGUN_UPGRADE_COST )
				{
					bHintPlayed = pTFPlayer->HintMessage( HINT_ENGINEER_UPGRADE_SENTRYGUN, false, true );
				}
				else
				{
					bHintPlayed = pTFPlayer->HintMessage( HINT_ENGINEER_METAL_TO_UPGRADE, false, true );
				}
			}
		}
	}

	if ( !bHintPlayed )
	{
		BaseClass::DisplayHintTo( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_ObjectSentrygun::IsUpgrading( void ) const
{
	return ( m_iState == SENTRY_STATE_UPGRADING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::GetTargetIDString( wchar_t *sIDString, int iMaxLenInBytes )
{
	return BaseClass::GetTargetIDString( sIDString, iMaxLenInBytes );

	sIDString[0] = '\0';

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	if ( GetTeamNumber() == pLocalPlayer->GetTeamNumber() )
	{
		wchar_t wszBuilderName[ MAX_PLAYER_NAME_LENGTH ];
		wchar_t wszHealthText[ 10 ];
		wchar_t wszObjectName[ 32 ];
		wchar_t wszUpgradeProgress[ 32 ];

		g_pVGuiLocalize->ConvertANSIToUnicode( GetStatusName(), wszObjectName, sizeof(wszObjectName) );

		C_BasePlayer *pBuilder = GetOwner();

		if ( pBuilder )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pBuilder->GetPlayerName(), wszBuilderName, sizeof(wszBuilderName) );
		}
		else
		{
			wszBuilderName[0] = '\0';
		}

		// building or live, show health
		_snwprintf( wszHealthText, ARRAYSIZE(wszHealthText) - 1, L"%.0f%%", ( (float)GetHealth() / (float)GetMaxHealth() ) * 100 );
		wszHealthText[ ARRAYSIZE(wszHealthText)-1 ] = '\0';

		if ( m_iUpgradeLevel < m_iHighestUpgradeLevel )
		{
			// level 1 and 2 show upgrade progress
			_snwprintf( wszUpgradeProgress, ARRAYSIZE(wszUpgradeProgress) - 1, L"%d / %d", m_iUpgradeMetal, m_iUpgradeMetalRequired );
			wszUpgradeProgress[ ARRAYSIZE(wszUpgradeProgress)-1 ] = '\0';

			const char *printFormatString = "#TF_playerid_object_upgrading";

			g_pVGuiLocalize->ConstructString( sIDString, iMaxLenInBytes, g_pVGuiLocalize->Find(printFormatString),
				4,
				wszObjectName,
				wszBuilderName,
				wszHealthText,
				wszUpgradeProgress );
		}
		else
		{
			const char *printFormatString = "#TF_playerid_object";

			g_pVGuiLocalize->ConstructString( sIDString, iMaxLenInBytes, g_pVGuiLocalize->Find(printFormatString),
				3,
				wszObjectName,
				wszBuilderName,
				wszHealthText );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *C_ObjectSentrygun::GetHudStatusIcon( void )
{
	const char *pszResult;

	switch( m_iUpgradeLevel )
	{
	case 1:
	default:
		pszResult = "obj_status_sentrygun_1";
		break;
	case 2:
		pszResult = "obj_status_sentrygun_2";
		break;
	case 3:
		pszResult = "obj_status_sentrygun_3";
		break;
	}

	return pszResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
BuildingHudAlert_t C_ObjectSentrygun::GetBuildingAlertLevel( void )
{
	BuildingHudAlert_t baseAlertLevel = BaseClass::GetBuildingAlertLevel();

	// Just warn on low shells.

	float flShellPercent = (float)m_iAmmoShells / (float)m_iMaxAmmoShells;

	BuildingHudAlert_t alertLevel = BUILDING_HUD_ALERT_NONE;

	if ( !IsBuilding() && flShellPercent < 0.25 )
	{
		alertLevel = BUILDING_HUD_ALERT_VERY_LOW_AMMO;
	}
	else if ( !IsBuilding() && flShellPercent < 0.50 )
	{
		alertLevel = BUILDING_HUD_ALERT_LOW_AMMO;
	}

	return max( baseAlertLevel, alertLevel );
}

//-----------------------------------------------------------------------------
// Purpose: Only use the smaller bbox for shadow calc, don't include the range bodygroup
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	mins = CollisionProp()->OBBMins();
	maxs = CollisionProp()->OBBMaxs();
}

//-----------------------------------------------------------------------------
// Purpose: Re-calc our damage particles when we get a new model
//-----------------------------------------------------------------------------
CStudioHdr *C_ObjectSentrygun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	UpdateDamageEffects( m_damageLevel );
	DestroySiren();

	// Reset Bodygroups
	for ( int i = GetNumBodyGroups()-1; i >= 0; i-- )
	{
		if ( IsMiniBuilding() && V_strcmp( GetBodygroupName( i ), "mini_sentry_light" ) == 0 )
			SetBodygroup( i, 1 );
		else
			SetBodygroup( i, 0 );
	}

	m_iPlacementBodygroup = FindBodygroupByName( "sentry1_range" );

	// Apparently mini-range isn't actually changed in live tf2
	//if ( IsMiniBuilding() )
		//m_iPlacementBodygroup = FindBodygroupByName( "sentry1_range_mini" );

	// Start thinking on the next frame
	SetNextClientThink( gpGlobals->frametime + 1 );

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
{
	if ( m_pDamageEffects )
	{
		ParticleProp()->StopEmission( m_pDamageEffects );
		m_pDamageEffects = NULL;
	}

	if ( IsPlacing() )
		return;

	const char *pszEffect = "";

	switch( damageLevel )
	{
	case BUILDING_DAMAGE_LEVEL_LIGHT:
		pszEffect = "sentrydamage_1";
		break;
	case BUILDING_DAMAGE_LEVEL_MEDIUM:
		pszEffect = "sentrydamage_2";
		break;
	case BUILDING_DAMAGE_LEVEL_HEAVY:
		pszEffect = "sentrydamage_3";
		break;
	case BUILDING_DAMAGE_LEVEL_CRITICAL:
		pszEffect = "sentrydamage_4";
		break;

	default:
		break;
	}

	if ( V_strlen(pszEffect) > 0 )
	{
		switch( m_iUpgradeLevel )
		{
		case 1:
		case 2:
			m_pDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "build_point_0" );
			break;

		case 3:
			m_pDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "sentrydamage" );
			break;
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: placement state has changed, update the model
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnPlacementStateChanged( bool bValidPlacement )
{
	if ( bValidPlacement && m_iPlacementBodygroup >= 0 )
	{
		SetBodygroup( m_iPlacementBodygroup, 1 );
	}
	else
	{
		SetBodygroup( m_iPlacementBodygroup, 0 );
	}

	BaseClass::OnPlacementStateChanged( bValidPlacement );
}

void C_ObjectSentrygun::DebugDamageParticles( void )
{
	Msg( "Health %d\n", GetHealth() );

	BuildingDamageLevel_t damageLevel = CalculateDamageLevel();
	Msg( "Damage Level %d\n", (int)damageLevel );

	if ( m_pDamageEffects )
	{
		Msg( "m_pDamageEffects is valid\n" );
	}
	else
	{
		Msg( "m_pDamageEffects is NULL\n" );
	}

	// print all particles owned by particleprop
	ParticleProp()->DebugPrintEffects();
}

void C_ObjectSentrygun::CreateLaserBeam( void ) 
{
	Vector vecColor;

	// Don't bother creating a new shield if one already exists
	if ( !m_pShield )
	{
		m_pShield = new C_BaseAnimating();
		m_pShield->SetModel( SENTRY_SHIELD );
		m_pShield->SetAbsOrigin( GetAbsOrigin() );

		// TODO: Figure out how to get this to show up correctly on the shield
		//m_pShieldEffects = m_pShield->ParticleProp()->Create( "turret_shield", PATTACH_ABSORIGIN_FOLLOW );
	}

	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			m_pShield->m_nSkin = 0;
			vecColor.Init( 255, -255, -255 );
			break;
		case TF_TEAM_BLUE:
			m_pShield->m_nSkin = 1;
			vecColor.Init( -255, -255, 255 );
			break;
	}

	// create pLaser
	m_pLaserBeam = ParticleProp()->Create( "laser_sight_beam", PATTACH_POINT_FOLLOW, "laser_origin" );
	if ( m_pLaserBeam)
		m_pLaserBeam->SetControlPoint( 2, vecColor );

	/*if ( m_pShieldEffects )
		m_pShieldEffects->SetControlPoint( 1, vecColor );*/
}

void C_ObjectSentrygun::ClientThink( void ) 
{
	if ( m_iState != SENTRY_STATE_WRANGLED )
	{
		if ( m_pLaserBeam )
			DestroyLaserBeam();

		if ( m_pShield && m_iState != SENTRY_STATE_WRANGLED_RECOVERY )
			DestroyShield();
	}
	else
	{
		if ( !m_pLaserBeam )
			CreateLaserBeam();

		if ( m_pLaserBeam )
		{
			// **NOTE:: Live TF2 does something similar to what's commented out here
			// with vecOut being interpolated instead of m_vecEnd but it's probably
			// cheaper to use the math the server already computed

			/*if ( gpGlobals->curtime > m_flUpdateEndpointTime )
			{
				if ( m_vecMuzzle == vec3_origin )
				{
					// Get the attachment for the trace
					const char *pszAttachment = "";
					switch( m_iUpgradeLevel )
					{
					case 1: 
						pszAttachment = "muzzle";
						break;
					case 2:
						pszAttachment ="muzzle_l";
						break;
					case 3:
						pszAttachment = "rocket_l";
						break;
					};

					GetAttachment( pszAttachment , m_vecMuzzle );
				}

				Vector vecEnd, vecForward, vecOut;

				C_TFPlayer *pOwner = GetBuilder();
				if ( m_vecLaser == vec3_origin && pOwner )
				{
					m_vecLaser = ( pOwner->GetClassEyeHeight() * 0.75 )  + pOwner->GetAbsOrigin();
				}

				trace_t tr;

				// Check where the sentry is looking on the client for interpolation
				CTraceFilterIgnoreTeammatesAndTeamObjects *pFilter = new CTraceFilterIgnoreTeammatesAndTeamObjects( this, COLLISION_GROUP_NONE, GetTeamNumber() );
				UTIL_TraceLine( m_vecMuzzle, m_vecEnd, MASK_SOLID, pFilter, &tr );

				InterpolateVector( gpGlobals->frametime * 25.0f, m_vecLaser, tr.endpos, vecOut );
				m_vecLaser = vecOut;

				m_flUpdateEndpointTime = gpGlobals->curtime + 0.2f
			}
			m_pLaserBeam->SetControlPoint( 1, vecOut );*/

			m_pLaserBeam->SetControlPoint( 1, m_vecEnd );
		}
	}
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void C_ObjectSentrygun::OnStartDisabled()
{
	DestroySiren();
	BaseClass::OnStartDisabled();
}

void C_ObjectSentrygun::OnEndDisabled()
{
	CreateSiren();
	BaseClass::OnEndDisabled();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnGoActive( void )
{
	CreateSiren();
	BaseClass::OnGoActive();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnGoInactive( void )
{
	DestroySiren();
	BaseClass::OnGoInactive();
}

void C_ObjectSentrygun::CreateSiren( void )
{
	if ( !IsMiniBuilding() || m_pSiren )
		return;

	// mini-sentry siren
	const char *pszEffect = "";
	switch( GetTeamNumber() )
	{
	case TF_TEAM_RED:
		pszEffect = "cart_flashinglight_red";
		break;
	case TF_TEAM_BLUE:
	default:
		pszEffect = "cart_flashinglight";
		break;
	}

	m_pSiren = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "siren" );
}

void C_ObjectSentrygun::DestroySiren( void )
{
	ParticleProp()->StopEmissionAndDestroyImmediately();

	if ( m_pSiren )
	{
		m_pSiren = NULL;
	}
}

void C_ObjectSentrygun::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
}