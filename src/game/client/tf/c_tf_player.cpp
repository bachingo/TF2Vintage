//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_player.h"
#include "c_user_message_register.h"
#include "view.h"
#include "iclientvehicle.h"
#include "ivieweffects.h"
#include "input.h"
#include "IEffects.h"
#include "fx.h"
#include "econ_wearable.h"
#include "c_basetempentity.h"
#include "hud_macros.h"
#include "engine/ivdebugoverlay.h"
#include "smoke_fog_overlay.h"
#include "playerandobjectenumerator.h"
#include "bone_setup.h"
#include "in_buttons.h"
#include "r_efx.h"
#include "dlight.h"
#include "shake.h"
#include "cl_animevent.h"
#include "tf_weaponbase.h"
#include "c_tf_playerresource.h"
#include "toolframework/itoolframework.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "prediction.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "tf_fx_muzzleflash.h"
#include "dt_utlvector_recv.h"
#include "tf_gamerules.h"
#include "view_scene.h"
#include "c_baseobject.h"
#include "toolframework_client.h"
#include "soundenvelope.h"
#include "voice_status.h"
#include "clienteffectprecachesystem.h"
#include "functionproxy.h"
#include "toolframework_client.h"
#include "choreoevent.h"
#include "c_entitydissolve.h"
#include "vguicenterprint.h"
#include "eventlist.h"
#include "tf_hud_statpanel.h"
#include "input.h"
#include "tf_wearable.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "c_tf_weapon_builder.h"
#include "tf_hud_mediccallers.h"
#include "in_main.h"
#include "basemodelpanel.h"
#include "c_team.h"
#include "collisionutils.h"
#include "c_merasmus_bomb_effect.h"
// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "c_tf_team.h"
#include "tf_viewmodel.h"

#include "tf_inventory.h"

#if defined( CTFPlayer )
#undef CTFPlayer
#endif

#include "materialsystem/imesh.h"		//for materials->FindMaterial
#include "iviewrender.h"				//for view->

#include "cam_thirdperson.h"
#include "tf_hud_chat.h"
#include "iclientmode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_playergib_forceup( "tf_playersgib_forceup", "1.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Upward added velocity for gibs." );
ConVar tf_playergib_force( "tf_playersgib_force", "500.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Gibs force." );
ConVar tf_playergib_maxspeed( "tf_playergib_maxspeed", "400", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Max gib speed." );

ConVar tf_spectate_pyrovision( "tf_spectate_pyrovision", "0", FCVAR_ARCHIVE, "When on, spectator will see the world with Pyrovision active" );

ConVar cl_autorezoom( "cl_autorezoom", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "When set to 1, sniper rifle will re-zoom after firing a zoomed shot." );

ConVar cl_autoreload( "cl_autoreload", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "When set to 1, clip-using weapons will automatically be reloaded whenever they're not being fired." );

ConVar cl_forced_vision_filter( "cl_forced_vision_filter", "0", FCVAR_DONTRECORD, "1=Pyrovision, 2=Halloween" );

ConVar cl_fp_ragdoll( "cl_fp_ragdoll", "1", FCVAR_ARCHIVE, "Allow first person ragdolls" );
ConVar cl_fp_ragdoll_auto( "cl_fp_ragdoll_auto", "1", FCVAR_ARCHIVE, "Autoswitch to ragdoll thirdperson-view when necessary" );

ConVar tf2v_model_muzzleflash( "tf2v_model_muzzleflash", "0", FCVAR_ARCHIVE, "Use the tf2 beta model based muzzleflash" );
ConVar tf2v_muzzlelight( "tf2v_muzzlelight", "0", FCVAR_ARCHIVE, "Enable dynamic lights for muzzleflashes and the flamethrower" );
ConVar tf2v_showchatbubbles( "tf2v_showchatbubbles", "1", FCVAR_ARCHIVE, "Show bubble icons over typing players" );

ConVar tf2v_show_veterancy( "tf2v_show_veterancy", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Enable or disable veterancy status if awarded." );

extern ConVar tf_halloween;
extern ConVar tf2v_new_flame_damage;

static void BuildDecapitatedTransform( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = pAnimating->LookupBone( "bip_head" );
		if ( iBone != -1 )
		{
			MatrixScaleByZero( pAnimating->GetBoneForWrite( iBone ) );
		}

		iBone = pAnimating->LookupBone( "prp_helmet" );
		if ( iBone != -1 )
		{
			MatrixScaleByZero( pAnimating->GetBoneForWrite( iBone ) );
		}

		iBone = pAnimating->LookupBone( "prp_hat" );
		if ( iBone != -1 )
		{
			MatrixScaleByZero( pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

#define BDAY_HAT_MODEL		"models/effects/bday_hat.mdl"

IMaterial *g_pHeadLabelMaterial[4] ={NULL, NULL};
void	SetupHeadLabelMaterials( void );

extern CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position,
	const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params );

const char *pszHeadLabelNames[] =
{
	"effects/speech_voice_red",
	"effects/speech_voice_blue",
	"effects/speech_voice_green",
	"effects/speech_voice_yellow"
};

const char *g_pszHeadGibs[] = {
	"",
	"models/player/gibs/scoutgib007.mdl",
	"models/player/gibs/snipergib005.mdl",
	"models/player/gibs/soldiergib007.mdl",
	"models/player/gibs/demogib006.mdl",
	"models/player/gibs/medicgib007.mdl",
	"models/player/gibs/heavygib007.mdl",
	"models/player/gibs/pyrogib008.mdl",
	"models/player/gibs/spygib007.mdl",
	"models/player/gibs/engineergib006.mdl",
};

#define TF_PLAYER_HEAD_LABEL_RED 0
#define TF_PLAYER_HEAD_LABEL_BLUE 1
#define TF_PLAYER_HEAD_LABEL_GREEN 2
#define TF_PLAYER_HEAD_LABEL_YELLOW 3


CLIENTEFFECT_REGISTER_BEGIN( PrecacheInvuln )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_blue.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_red.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_green.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_yellow.vmt" )
CLIENTEFFECT_REGISTER_END()

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		VPROF( "C_TEPlayerAnimEvent::PostDataUpdate" );

		// Create the effect.
		if ( m_iPlayerIndex == TF_PLAYER_INDEX_NONE )
			return;

		EHANDLE hPlayer = cl_entitylist->GetNetworkableHandle( m_iPlayerIndex );
		if ( !hPlayer )
			return;

		C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer *>( hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
		}
	}

public:
	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

//-----------------------------------------------------------------------------
// Data tables and prediction tables.
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()


//=============================================================================
//
// Ragdoll
//
// ----------------------------------------------------------------------------- //
// Client ragdoll entity.
// ----------------------------------------------------------------------------- //
ConVar cl_ragdoll_physics_enable( "cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics." );
ConVar cl_ragdoll_fade_time( "cl_ragdoll_fade_time", "15", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_forcefade( "cl_ragdoll_forcefade", "0", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_pronecheck_distance( "cl_ragdoll_pronecheck_distance", "64", FCVAR_GAMEDLL );
ConVar tf_always_deathanim( "tf_always_deathanim", "0", FCVAR_CHEAT, "Force death anims to always play." );

extern C_EntityDissolve *DissolveEffect( C_BaseEntity *pTarget, float flTime );
class C_TFRagdoll : public C_BaseFlex
{
public:

	DECLARE_CLASS( C_TFRagdoll, C_BaseFlex );
	DECLARE_CLIENTCLASS();

	C_TFRagdoll();
	~C_TFRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	virtual int InternalDrawModel( int flags );

	IRagdoll *GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );

	void ClientThink( void );
	void StartFadeOut( float fDelay );
	void EndFadeOut();

	EHANDLE GetPlayerHandle( void )
	{
		if ( m_iPlayerIndex == TF_PLAYER_INDEX_NONE )
			return NULL;
		return cl_entitylist->GetNetworkableHandle( m_iPlayerIndex );
	}

	bool IsRagdollVisible();
	bool IsDecapitation();
	float GetBurnStartTime() { return m_flBurnEffectStartTime; }
	float GetBurnEndTime() { return m_flBurnEffectEndTime; }

	void DissolveEntity( C_BaseEntity *pEntity );

	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual float FrameAdvance( float flInterval = 0.0f );

	virtual void BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );
	virtual bool GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool GetAttachment( int number, Vector &origin, QAngle &angles ) OVERRIDE { return BaseClass::GetAttachment( number, origin, angles ); }

	float GetInvisibilityLevel( void )
	{
		if ( m_bCloaked )
			return m_flInvisibilityLevel;

		if ( m_flUncloakCompleteTime == 0.0f )
			return 0.0f;

		return RemapValClamped( m_flUncloakCompleteTime - gpGlobals->curtime, 0.0f, 2.0f, 0.0f, 1.0f );
	}

	bool IsPlayingDeathAnim( void ) const { return m_bPlayDeathAnim; }

private:
	friend class C_TFPlayer;
	C_TFRagdoll( const C_TFRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity );

	void CreateTFRagdoll( void );
	void CreateTFGibs( bool bKill, bool bLocalOrigin );
	void CreateTFHeadGib( void );
	void CreateWearableGibs( bool bDisguised );

protected:
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	int	  m_iPlayerIndex;
	float m_fDeathTime;
	bool  m_bGib;
	bool  m_bBurning;
	bool  m_bElectrocuted;
	bool  m_bSlamRagdoll;
	bool  m_bDissolve;
	bool  m_bFeignDeath;
	bool  m_bWasDisguised;
	bool  m_bBecomeAsh;
	bool  m_bOnGround;
	bool  m_bCloaked;
	float m_flInvisibilityLevel;
	float m_flUncloakCompleteTime;
	int   m_iDamageCustom;
	int	  m_iTeam;
	int	  m_iClass;
	bool  m_bStartedDying;
	bool  m_bGoldRagdoll;
	bool  m_bIceRagdoll;
	bool  m_bCritOnHardHit;
	float m_flHeadScale;
	float m_flBurnEffectStartTime;	// start time of burning, or 0 if not burning
	float m_flBurnEffectEndTime;	// start time of burning, or 0 if not burning
	float m_flDeathDelay;
	bool  m_bFixedConstraints;
	bool  m_bFadingOut;
	bool  m_bPlayDeathAnim;
	CountdownTimer m_freezeTimer;
	CountdownTimer m_frozenTimer;

	// Decapitation
	matrix3x4_t m_Head;
	bool m_bHeadTransform;

	CMaterialReference m_MatOverride;

	CUtlVector< CHandle<CEconWearable> > m_hRagdollWearables;
};

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_TFRagdoll, DT_TFRagdoll, CTFRagdoll )
	RecvPropVector( RECVINFO( m_vecRagdollOrigin ) ),
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropVector( RECVINFO( m_vecForce ) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) ),
	RecvPropInt( RECVINFO( m_nForceBone ) ),
	RecvPropBool( RECVINFO( m_bGib ) ),
	RecvPropBool( RECVINFO( m_bBurning ) ),
	RecvPropBool( RECVINFO( m_bElectrocuted ) ),
	RecvPropBool( RECVINFO( m_bFeignDeath ) ),
	RecvPropBool( RECVINFO( m_bWasDisguised ) ),
	RecvPropBool( RECVINFO( m_bBecomeAsh ) ),
	RecvPropBool( RECVINFO( m_bOnGround ) ),
	RecvPropBool( RECVINFO( m_bCloaked ) ),
	RecvPropInt( RECVINFO( m_iDamageCustom ) ),
	RecvPropInt( RECVINFO( m_iTeam ) ),
	RecvPropInt( RECVINFO( m_iClass ) ),
	RecvPropUtlVector( RECVINFO_UTLVECTOR( m_hRagdollWearables ), MAX_WEARABLES_SENT_FROM_SERVER, RecvPropEHandle( NULL, 0, 0 ) ),
	RecvPropBool( RECVINFO( m_bGoldRagdoll ) ),
	RecvPropBool( RECVINFO( m_bIceRagdoll ) ),
	RecvPropBool( RECVINFO( m_bCritOnHardHit ) ),
	RecvPropFloat( RECVINFO( m_flHeadScale ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::C_TFRagdoll()
{
	m_iPlayerIndex = TF_PLAYER_INDEX_NONE;
	m_fDeathTime = -1.0f;
	m_flBurnEffectStartTime = 0.0f;
	m_flBurnEffectEndTime = 0.0f;
	m_iDamageCustom = false;
	m_bGoldRagdoll = false;
	m_bSlamRagdoll = false;
	m_bFadingOut = false;
	m_bCloaked = false;
	m_freezeTimer.Invalidate();
	m_frozenTimer.Invalidate();
	m_iTeam = -1;
	m_iClass = -1;
	m_nForceBone = -1;
	m_bHeadTransform = 0;
	m_bStartedDying = 0;
	m_flDeathDelay = 0.3f;
	m_flHeadScale = 1.0f;

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::~C_TFRagdoll()
{
	m_MatOverride.Shutdown();
	PhysCleanupFrictionSounds( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSourceEntity - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;

	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();

	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		const char *pszName = pDestEntry->watcher->GetDebugName();
		for ( int j = 0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(), pszName ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dissolve the targetted entity
//-----------------------------------------------------------------------------
void C_TFRagdoll::DissolveEntity( C_BaseEntity *pEntity )
{
	C_EntityDissolve *pDissolver = DissolveEffect( pEntity, gpGlobals->curtime );
	if ( !pDissolver )
		return;

	if ( m_iTeam == TF_TEAM_BLUE )
		pDissolver->SetEffectColor( Vector( BitsToFloat( 0x4337999A ), BitsToFloat( 0x42606666 ), BitsToFloat( 0x426A999A ) ) );
	else
		pDissolver->SetEffectColor( Vector( BitsToFloat( 0x42AFF333 ), BitsToFloat( 0x43049999 ), BitsToFloat( 0x4321ECCD ) ) );

	pDissolver->m_nRenderFX = kRenderFxNone;
	pDissolver->SetRenderMode( kRenderTransColor );
	pDissolver->SetRenderColor( 255, 255, 255, 255 );

	pDissolver->m_vDissolverOrigin = GetLocalOrigin();
	pDissolver->m_flFadeInStart = 0.0f;
	pDissolver->m_flFadeInLength = 1.0f;
	pDissolver->m_flFadeOutModelStart = 1.9f;
	pDissolver->m_flFadeOutModelLength = 0.1f;
	pDissolver->m_flFadeOutStart = 2.0f;
	pDissolver->m_flFadeOutLength = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Setup vertex weights for drawing
//-----------------------------------------------------------------------------
void C_TFRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	// While we're dying, we want to mimic the facial animation of the player. Once they're dead, we just stay as we are.
	EHANDLE hPlayer = GetPlayerHandle();
	if ( ( hPlayer && hPlayer->IsAlive() ) || !hPlayer )
	{
		BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
	else if ( hPlayer )
	{
		hPlayer->SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTrace - 
//			iDamageType - 
//			*pCustomImpactName - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	VPROF( "C_TFRagdoll::ImpactTrace" );
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( !pPhysicsObject )
		return;

	Vector vecDir;
	VectorSubtract( pTrace->endpos, pTrace->startpos, vecDir );

	if ( iDamageType == DMG_BLAST )
	{
		// Adjust the impact strength and apply the force at the center of mass.
		vecDir *= 4000;
		pPhysicsObject->ApplyForceCenter( vecDir );
	}
	else
	{
		// Find the apporx. impact point.
		Vector vecHitPos;
		VectorMA( pTrace->startpos, pTrace->fraction, vecDir, vecHitPos );
		VectorNormalize( vecDir );

		// Adjust the impact strength and apply the force at the impact point..
		vecDir *= 4000;
		pPhysicsObject->ApplyForceOffset( vecDir, vecHitPos );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

// ---------------------------------------------------------------------------- -
// Purpose: 
// Input  : flInterval - 
// Output : float
//-----------------------------------------------------------------------------
float C_TFRagdoll::FrameAdvance( float flInterval )
{
	if ( m_freezeTimer.HasStarted() && !m_freezeTimer.IsElapsed() )
		return BaseClass::FrameAdvance( flInterval );

	float flRet = 0.0f; 
	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

	if ( m_frozenTimer.HasStarted() )
	{
		if ( !m_frozenTimer.IsElapsed() )
			return flRet;

		m_frozenTimer.Invalidate();
		m_nRenderFX = kRenderFxRagdoll;

		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, true );

		SetAbsVelocity( vec3_origin );

		m_bStartedDying = true;
	}

	flRet = BaseClass::FrameAdvance( flInterval );

	if ( !m_bStartedDying && IsSequenceFinished() && m_bPlayDeathAnim )
	{
		m_nRenderFX = kRenderFxRagdoll;

		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, false );

		SetAbsVelocity( vec3_origin );

		m_bStartedDying = true;

		StartFadeOut( cl_ragdoll_fade_time.GetFloat() );
	}

	return flRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed );

	float flHeadScale = 1.0f;
	if ( m_iPlayerIndex != TF_PLAYER_INDEX_NONE )
	{
		C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer *>( cl_entitylist->GetBaseEntity( m_iPlayerIndex ) );
		if ( pPlayer )
			flHeadScale = pPlayer->m_flHeadScale;
	}

	BuildBigHeadTransformation( this, pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed, flHeadScale );

	if ( IsDecapitation() && !m_bHeadTransform )
	{
		m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
		BuildDecapitatedTransform( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TFRagdoll::GetAttachment( int number, matrix3x4_t &matrix )
{
	// Sword decapitation
	if ( IsDecapitation() && LookupAttachment( "head" ) == number )
	{
		MatrixCopy( m_Head, matrix );
		return true;
	}

	return BaseClass::GetAttachment( number, matrix );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFRagdoll( void )
{
	// Get the player.
	C_TFPlayer *pPlayer = ToTFPlayer( GetPlayerHandle() );

	int nModelIndex = -1;
	if ( pPlayer && !pPlayer->ShouldDrawSpyAsDisguised() )
	{
		char const *szModelName = pPlayer->GetPlayerClass()->GetModelName();
		nModelIndex = modelinfo->GetModelIndex( szModelName );
	}
	else
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( m_iClass );
		if ( pData )
		{
			nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );
		}
	}

	if ( nModelIndex != -1 )
	{
		SetModelIndex( nModelIndex );
		
		switch ( m_iTeam )
		{
				
			case TF_TEAM_RED:
				m_nSkin = 0;
				break;
					
			case TF_TEAM_BLUE:
				m_nSkin = 1;
				break;
				
			case TF_TEAM_GREEN:
				m_nSkin = 8;
				break;

			case TF_TEAM_YELLOW:
				m_nSkin = 9;
				break;
				
			default:
				m_nSkin = 0;
				break;
				
		}
		
		if ( TFGameRules()->IsHolidayActive(kHoliday_Halloween) && !m_bGib )
		{
			m_nSkin += 4;
			if ( m_iClass == TF_CLASS_SPY )
				m_nSkin += 18;
		}
		
	}

	//if ( pPlayer && pPlayer->BRenderAsZombie( false ) )
	//	m_nSkin += m_iClass == TF_CLASS_SPY ? 22 : 4;

	if ( m_bGoldRagdoll || m_iDamageCustom == TF_DMG_CUSTOM_GOLD_WRENCH )
	{
		EmitSound( "Saxxy.TurnGold" );
		m_bFixedConstraints = true;
	}

	if ( m_bIceRagdoll )
	{
		EmitSound( "Icicle.TurnToIce" );
		ParticleProp()->Create( "xms_icicle_impact_dryice", PATTACH_ABSORIGIN_FOLLOW );

		m_freezeTimer.Start( RandomFloat( 0.1f, 0.75f ) );
		m_frozenTimer.Start( RandomFloat( 9.0f, 11.0f ) );
	}

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// Move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.		
		if ( !pPlayer->IsLocalPlayer() && pPlayer->IsInterpolationEnabled() )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( pPlayer->GetRenderOrigin() );
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( m_vecRagdollVelocity );

			// Hack! Find a neutral standing pose or use the idle.
			int iSeq = LookupSequence( "RagdollSpawn" );
			if ( iSeq == -1 )
			{
				Assert( false );
				iSeq = 0;
			}
			SetSequence( iSeq );
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}

		if ( !m_bFeignDeath || m_bWasDisguised )
			m_nBody = pPlayer->GetBody();
	}
	else
	{
		// Overwrite network origin so later interpolation will use this position.
		SetNetworkOrigin( m_vecRagdollOrigin );
		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
	}

	if ( m_bCloaked )
		AddEffects( EF_NOSHADOW );

	if ( pPlayer && !m_bGoldRagdoll )
	{
		bool bFloatingAnim = false;
		int iSeq = pPlayer->m_Shared.GetSequenceForDeath( this, m_iDamageCustom );
		int iDesiredSeq = -1;

		if ( m_bDissolve && !m_bGib )
		{
			iSeq = pPlayer->LookupSequence( "dieviolent" );
			bFloatingAnim = true;
		}

		if ( iSeq >= 0 )
		{
			switch ( m_iDamageCustom )
			{
				case TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING:
				case TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH:
				case TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF:
					iDesiredSeq = iSeq;
					break;
				default:
					if ( m_bIceRagdoll || tf_always_deathanim.GetBool() || RandomFloat() <= 0.25f )
						iDesiredSeq = iSeq;
					break;
			}
		}

		if ( iDesiredSeq >= 0 && ( m_bOnGround || bFloatingAnim ) && cl_ragdoll_physics_enable.GetBool() )
		{
			// Slam velocity when doing death animation.
			SetAbsOrigin( pPlayer->GetNetworkOrigin() );
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( vec3_origin );
			m_vecForce = vec3_origin;

			ClientLeafSystem()->SetRenderGroup( GetRenderHandle(), RENDER_GROUP_OPAQUE_ENTITY );
			UpdateVisibility();

			SetSequence( iSeq );
			SetCycle( 0.0f );
			ResetSequenceInfo();

			m_bPlayDeathAnim = true;
		}
		else if ( m_bIceRagdoll )
		{
			m_freezeTimer.Invalidate();
			m_frozenTimer.Invalidate();
			m_bFixedConstraints = true;
		}
	}

	// Turn it into a ragdoll.
	if ( !m_bPlayDeathAnim && cl_ragdoll_physics_enable.GetBool() )
	{
		// Make us a ragdoll..
		m_nRenderFX = kRenderFxRagdoll;

		matrix3x4_t boneDelta0[MAXSTUDIOBONES];
		matrix3x4_t boneDelta1[MAXSTUDIOBONES];
		matrix3x4_t currentBones[MAXSTUDIOBONES];
		const float boneDt = 0.05f;

		// We have to make sure that we're initting this client ragdoll off of the same model.
		// GetRagdollInitBoneArrays uses the *player* Hdr, which may be a different model than
		// the ragdoll Hdr, if we try to create a ragdoll in the same frame that the player
		// changes their player model.
		CStudioHdr *pRagdollHdr = GetModelPtr();
		CStudioHdr *pPlayerHdr = NULL;
		if ( pPlayer )
			pPlayerHdr = pPlayer->GetModelPtr();

		bool bChangedModel = false;

		if ( pRagdollHdr && pPlayerHdr )
		{
			bChangedModel = pRagdollHdr->GetVirtualModel() != pPlayerHdr->GetVirtualModel();

			AssertMsg( !bChangedModel, "C_TFRagdoll::CreateTFRagdoll: Trying to create ragdoll with a different model than the player it's based on" );
		}

		bool bBoneArraysInited;
		if ( pPlayer && !pPlayer->IsDormant() && !bChangedModel )
		{
			bBoneArraysInited = pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		else
		{
			bBoneArraysInited = GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		}

		if ( bBoneArraysInited )
		{
			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, m_bFixedConstraints );
		}
	}
	else
	{
		ClientLeafSystem()->SetRenderGroup( GetRenderHandle(), RENDER_GROUP_TRANSLUCENT_ENTITY );
	}

	StartFadeOut( cl_ragdoll_fade_time.GetFloat() );

	if ( m_bBurning )
	{
		if ( pPlayer )	// Try to use the player's information for burning.
		{
			m_flBurnEffectStartTime = pPlayer->m_flBurnEffectStartTime;
			m_flBurnEffectEndTime = pPlayer->m_flBurnEffectEndTime;
		}
		else			// If we can't, construct an approximation.
		{
			m_flBurnEffectStartTime = gpGlobals->curtime;
			m_flBurnEffectEndTime = tf2v_new_flame_damage.GetBool() ? (gpGlobals->curtime + TF_BURNING_FLAME_LIFE_JI )  : (gpGlobals->curtime + TF_BURNING_FLAME_LIFE ) ;
		}
		if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) )
			ParticleProp()->Create( "burningplayer_corpse_rainbow", PATTACH_ABSORIGIN_FOLLOW );			
		else
			ParticleProp()->Create( "burningplayer_corpse", PATTACH_ABSORIGIN_FOLLOW );
	}

	if ( m_bElectrocuted )
	{
		const char *pszEffect = ConstructTeamParticle( "electrocuted_%s", m_iTeam );
		EmitSound( "TFPlayer.MedicChargedDeath" );
		ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
	}

	if ( m_bBecomeAsh && !m_bDissolve && !m_bGib )
	{
		ParticleProp()->Create( "drg_fiery_death", PATTACH_ABSORIGIN_FOLLOW );
		m_flDeathDelay = 0.5f;
	}

	if ( pPlayer )
	{
		pPlayer->CreateBoneAttachmentsFromWearables( this, m_bWasDisguised );
		pPlayer->MoveBoneAttachments( this );

		int nBombinomiconDeath = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nBombinomiconDeath, bombinomicon_effect_on_death );
		if ( nBombinomiconDeath == 1 && !m_bGib && !m_bGoldRagdoll )
			m_flDeathDelay = 1.2f;
	}

	const char *pszMaterial = NULL;
	if ( m_bGoldRagdoll )
		pszMaterial = "models/player/shared/gold_player.vmt";
	if ( m_bIceRagdoll )
		pszMaterial = "models/player/shared/ice_player.vmt";

	if ( pszMaterial )
	{
		m_MatOverride.Init( pszMaterial, "ClientEffect textures", true );

		for ( C_BaseEntity *pClientEntity = cl_entitylist->FirstBaseEntity(); pClientEntity; pClientEntity = cl_entitylist->NextBaseEntity( pClientEntity ) )
		{
			if ( pClientEntity->GetFollowedEntity() == this )
			{
				C_EconEntity *pEconEnt = dynamic_cast<C_EconEntity *>( pClientEntity );
				if ( pEconEnt )
					pEconEnt->SetMaterialOverride( GetTeamNumber(), pszMaterial );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFGibs( bool bKill, bool bLocalOrigin )
{
	C_TFPlayer *pPlayer = NULL;
	EHANDLE hPlayer = GetPlayerHandle();
	if ( hPlayer )
	{
		pPlayer = dynamic_cast<C_TFPlayer *>( hPlayer.Get() );
	}
	if ( pPlayer )
	{
		int nBombinomiconDeath = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nBombinomiconDeath, bombinomicon_effect_on_death );
		if ( nBombinomiconDeath == 1 )
		{
			m_vecForce *= Vector( 2, 2, 6 );
			const char *pszEffect = "bombinomicon_burningdebris";
			if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
				pszEffect = "bombinomicon_burningdebris_halloween";

			Vector vecOrigin;
			if ( bLocalOrigin )
				vecOrigin = GetLocalOrigin();
			else
				vecOrigin = m_vecRagdollOrigin;

			DispatchParticleEffect( pszEffect, vecOrigin, GetLocalAngles() );
			EmitSound( "Bombinomicon.Explode" );
		}

		if ( pPlayer->m_hFirstGib == NULL || m_bFeignDeath )
		{
			Vector vecVelocity = m_vecForce + m_vecRagdollVelocity;
			VectorNormalize( vecVelocity );

			Vector vecOrigin;
			if ( bLocalOrigin )
				vecOrigin = GetLocalOrigin();
			else
				vecOrigin = m_vecRagdollOrigin;

			pPlayer->CreatePlayerGibs( vecOrigin, vecVelocity, m_vecForce.Length(), m_bBurning, false, false, m_bWasDisguised );
		}
	}

	if ( pPlayer )
	{
		if( TFGameRules() && ( TFGameRules()->IsBirthday() || UTIL_IsLowViolence() ) )
		{
			DispatchParticleEffect( "bday_confetti", pPlayer->GetAbsOrigin() + Vector( 0, 0, 32 ), vec3_angle );

			if( TFGameRules()->IsBirthday() )
				C_BaseEntity::EmitSound( "Game.HappyBirthday" );
		}
		else
		{
			if( m_bCritOnHardHit && !UTIL_IsLowViolence() )
				DispatchParticleEffect( "tfc_sniper_mist", pPlayer->WorldSpaceCenter(), vec3_angle );
		}
	}

	if ( bKill )
	{
		EndFadeOut();
	}
	else
	{
		SetRenderMode( kRenderNone );
		UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFHeadGib( void )
{
	C_TFPlayer *pPlayer = NULL;
	EHANDLE hPlayer = GetPlayerHandle();
	if ( hPlayer )
	{
		pPlayer = dynamic_cast<C_TFPlayer *>( hPlayer.Get() );
	}
	if ( pPlayer && ( pPlayer->m_hFirstGib == NULL ) )
	{
		Vector vecVelocity = m_vecForce + m_vecRagdollVelocity;
		VectorNormalize( vecVelocity );
		pPlayer->CreatePlayerGibs( m_vecRagdollOrigin, vecVelocity, m_vecForce.Length(), m_bBurning, false, true, m_bWasDisguised );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateWearableGibs( bool bDisguised )
{
	C_TFPlayer *pPlayer = NULL;
	EHANDLE hPlayer = GetPlayerHandle();
	if ( hPlayer )
	{
		pPlayer = dynamic_cast<C_TFPlayer *>( hPlayer.Get() );
	}
	if ( pPlayer )
	{
		Vector vecVelocity = m_vecForce + m_vecRagdollVelocity;
		VectorNormalize( vecVelocity );
		pPlayer->CreatePlayerGibs( m_vecRagdollOrigin, vecVelocity, m_vecForce.Length(), m_bBurning, true, false, bDisguised );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		bool bCreateRagdoll = true;

		// Get the player.
		EHANDLE hPlayer = GetPlayerHandle();
		if ( hPlayer )
		{
			// If we're getting the initial update for this player (e.g., after resetting entities after
			//  lots of packet loss, then don't create gibs, ragdolls if the player and it's gib/ragdoll
			//  both show up on same frame.
			if ( abs( hPlayer->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}
		else if ( C_BasePlayer::GetLocalPlayer() )
		{
			// Ditto for recreation of the local player
			if ( abs( C_BasePlayer::GetLocalPlayer()->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}

		C_TFPlayer *pPlayer = ToTFPlayer( hPlayer );
		if ( m_bCloaked && pPlayer )
			m_flUncloakCompleteTime = gpGlobals->curtime * 2.0f + pPlayer->GetPercentInvisible();

		if ( m_iDamageCustom == TF_DMG_CUSTOM_PLASMA_CHARGED )
		{
			if ( !m_bBecomeAsh )
				m_bDissolve = true;

			m_bGib = true;
		}
		else if ( m_iDamageCustom == TF_DMG_CUSTOM_PLASMA )
		{
			if ( !m_bBecomeAsh )
				m_bDissolve = true;

			m_bGib = false;
		}

		if ( bCreateRagdoll )
		{
			// Delete our mask if we're disguised.
			if (pPlayer && pPlayer->m_Shared.InCond(TF_COND_DISGUISED))
			{			
				pPlayer->UpdateSpyMask();
			}
		
			if ( m_bGib )
			{
				CreateTFGibs( !m_bDissolve, false );
			}
			else
			{
				CreateTFRagdoll();
				
				if ( IsDecapitation() )
				{
					CreateTFHeadGib();

					if ( !UTIL_IsLowViolence() ) // Pyrovision check
					{
						EmitSound( "TFPlayer.Decapitated" );
						ParticleProp()->Create( "blood_decap", PATTACH_POINT_FOLLOW, "head" );
					}
				}
			}

			CreateWearableGibs( m_bWasDisguised );
		}
	}
	else
	{
		if ( !cl_ragdoll_physics_enable.GetBool() )
		{
			// Don't let it set us back to a ragdoll with data from the server.
			m_nRenderFX = kRenderFxNone;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : 
//-----------------------------------------------------------------------------
int C_TFRagdoll::InternalDrawModel( int flags )
{
	if ( m_MatOverride )
		modelrender->ForcedMaterialOverride( m_MatOverride );

	int result = C_BaseAnimating::InternalDrawModel( flags );

	if ( m_MatOverride )
		modelrender->ForcedMaterialOverride( NULL );

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll *C_TFRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFRagdoll::IsRagdollVisible()
{
	Vector vMins = Vector( -1, -1, -1 );	//WorldAlignMins();
	Vector vMaxs = Vector( 1, 1, 1 );	//WorldAlignMaxs();

	Vector origin = GetAbsOrigin();

	if ( !engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin ) )
	{
		return false;
	}
	else if ( engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TFRagdoll::IsDecapitation()
{
	// Only decapitate if the ragdoll is going to stick around for a while (?)
	if ( cl_ragdoll_fade_time.GetFloat() > 5.0f && ( m_iDamageCustom == TF_DMG_CUSTOM_DECAPITATION || m_iDamageCustom == TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING
													|| m_iDamageCustom == TF_DMG_CUSTOM_DECAPITATION_BOSS || m_iDamageCustom == TF_DMG_CUSTOM_HEADSHOT_DECAPITATION ) )
	{
		return true;
	}
	return false;
}

extern ConVar g_ragdoll_lvfadespeed;
extern ConVar g_ragdoll_fadespeed;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::ClientThink( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	if ( IsDecapitation() )
	{
		m_bHeadTransform = true;
		BaseClass::GetAttachment( LookupAttachment( "head" ), m_Head );
		m_bHeadTransform = false;

		m_BoneAccessor.SetReadableBones( 0 );
		SetupBones( NULL, -1, BONE_USED_BY_ATTACHMENT, gpGlobals->curtime );
	}

	if ( m_bCloaked )
	{
		if ( m_flInvisibilityLevel < 1.0f )
			m_flInvisibilityLevel = Min( m_flInvisibilityLevel + gpGlobals->frametime, 1.0f );
	}

	C_TFPlayer *pPlayer = ToTFPlayer( GetPlayerHandle() );
	bool bBombinomiconDeath = false;
	if ( pPlayer )
	{
		int nBombinomiconDeath = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nBombinomiconDeath, bombinomicon_effect_on_death );
		bBombinomiconDeath = nBombinomiconDeath != 0;
	}

	if ( !m_bGib )
	{
		if ( !m_bDissolve )
		{
			if ( bBombinomiconDeath && ( GetFlags() & FL_DISSOLVING ) )
			{
				m_flDeathDelay -= gpGlobals->frametime;
				if ( m_flDeathDelay <= 0 )
					CreateTFGibs( !m_bDissolve, true );
			}
			else
			{
				if ( !m_bBecomeAsh )
				{
					if ( bBombinomiconDeath )
					{
						m_flDeathDelay -= gpGlobals->frametime;
						if ( m_flDeathDelay <= 0 )
						{
							CreateTFGibs( !m_bDissolve, true );
							return;
						}
					}
				}
				else
				{
					m_flDeathDelay -= gpGlobals->frametime;
					if ( m_flDeathDelay <= 0 )
					{
						if ( !bBombinomiconDeath )
						{
							AddEffects( EF_NODRAW );
							ParticleProp()->StopParticlesNamed( "drg_fiery_death", true, true );

							for ( C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity) )
							{
								if ( pEntity->GetFollowedEntity() == this )
								{
									CEconEntity *pItem = dynamic_cast< CEconEntity * >( pEntity );
									if ( pItem )
									{
										pItem->AddEffects( EF_NODRAW );
									}
								}
							}
						}
						else
						{
							CreateTFGibs( !m_bDissolve, true );
						}
						return;
					}
				}
			}
		}
		else
		{
			m_bDissolve = false;
			m_flDeathDelay = 1.2f;

			DissolveEntity( this );
			EmitSound( "TFPlayer.Dissolve" );

			for ( C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity) )
			{
				if ( pEntity->GetFollowedEntity() == this )
				{
					CEconEntity *pItem = dynamic_cast< CEconEntity * >( pEntity );
					if ( pItem )
					{
						DissolveEntity( pItem );
					}
				}
			}
		}
	}
	else
	{
		if ( m_bDissolve )
		{
			m_flDeathDelay -= gpGlobals->frametime;
			if ( m_flDeathDelay > 0 )
				return;

			m_bDissolve = false;

			if ( pPlayer )
			{
				if ( bBombinomiconDeath )
				{
					CreateTFGibs( true, true );
				}
				else
				{
					for ( int i=0; i<pPlayer->m_hSpawnedGibs.Count(); ++i )
					{
						C_BaseEntity *pGiblet = pPlayer->m_hSpawnedGibs[i];

						pGiblet->SetAbsVelocity( vec3_origin );
						DissolveEntity( pGiblet );
						pGiblet->ParticleProp()->StopParticlesInvolving( pGiblet );
					}
				}
			}

			EndFadeOut();
			return;
		}
	}

	if ( m_bFadingOut == true )
	{
		int iAlpha = GetRenderColor().a;
		int iFadeSpeed = (g_RagdollLVManager.IsLowViolence()) ? g_ragdoll_lvfadespeed.GetInt() : g_ragdoll_fadespeed.GetInt();

		if (iFadeSpeed < 1)
		{
			iAlpha = 0;
		}
		else
		{
			iAlpha = Max(iAlpha - (iFadeSpeed * gpGlobals->frametime), 0.0f);
		}

		SetRenderMode( kRenderTransAlpha );
		SetRenderColorA( iAlpha );

		if ( iAlpha == 0 )
		{
			EndFadeOut(); // remove clientside ragdoll
		}

		return;
	}

	// if the player is looking at us, delay the fade
	if ( IsRagdollVisible() )
	{
		if ( cl_ragdoll_forcefade.GetBool() )
		{
			m_bFadingOut = true;
			float flDelay = cl_ragdoll_fade_time.GetFloat() * 0.33f;

			// If we were just fully healed, remove all decals
			RemoveAllDecals();

			if (flDelay > 0.01f)
			{
				m_fDeathTime = gpGlobals->curtime + flDelay;
				return;
			}
			m_fDeathTime = -1;
		}
		else
		{
			// Fade out after the specified delay.
			StartFadeOut(cl_ragdoll_fade_time.GetFloat() * 0.33f);
			return;
		}
	}

	if ( m_fDeathTime > gpGlobals->curtime )
		return;

	EndFadeOut(); // remove clientside ragdoll
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::StartFadeOut( float fDelay )
{
	if ( !cl_ragdoll_forcefade.GetBool() )
	{
		m_fDeathTime = gpGlobals->curtime + fDelay;
	}
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::EndFadeOut()
{
	SetNextClientThink( CLIENT_THINK_NEVER );
	ClearRagdoll();
	SetRenderMode( kRenderNone );
	UpdateVisibility();
	ParticleProp()->StopEmission();
	DestroyBoneAttachments();
	
	for ( int i = 0; i < m_hRagdollWearables.Count(); i++ )
	{
		if ( m_hRagdollWearables[i] )
		{
			m_hRagdollWearables[i]->AddEffects( EF_NODRAW );
			m_hRagdollWearables[i]->SetMoveType( MOVETYPE_NONE );
		}
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CSpyInvisProxy : public CEntityMaterialProxy
{
public:
	CSpyInvisProxy( void );
	virtual				~CSpyInvisProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *GetMaterial();

private:

	IMaterialVar *m_pPercentInvisible;
	IMaterialVar *m_pCloakColorTint;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSpyInvisProxy::CSpyInvisProxy( void )
{
	m_pPercentInvisible = NULL;
	m_pCloakColorTint = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSpyInvisProxy::~CSpyInvisProxy( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CSpyInvisProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bInvis;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bInvis );

	bool bTint;
	m_pCloakColorTint = pMaterial->FindVar( "$cloakColorTint", &bTint );

	return ( bInvis && bTint );
}

ConVar tf_teammate_max_invis( "tf_teammate_max_invis", "0.95", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CSpyInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible || !m_pCloakColorTint )
		return;

	if ( !pEnt )
		return;

	C_TFPlayer *pPlayer = ToTFPlayer( pEnt );

	if ( pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
	}
	else
	{
		C_TFPlayer *pOwner = ToTFPlayer( pEnt->GetOwnerEntity() );

		C_TFRagdoll *pRagdoll = dynamic_cast<C_TFRagdoll *>( pEnt );
		if ( pRagdoll )
		{
			pPlayer = ToTFPlayer( pRagdoll->GetPlayerHandle().Get() );
			m_pPercentInvisible->SetFloatValue( pRagdoll->GetInvisibilityLevel() );
		}
		else
		{
			if ( pOwner )
				m_pPercentInvisible->SetFloatValue( pOwner->GetEffectiveInvisibilityLevel() );
			else
				m_pPercentInvisible->SetFloatValue( 0.0 );

			return;
		}
	}

	if ( !pPlayer )
		return;

	float r, g, b;

	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			r = 1.0; g = 0.5; b = 0.4;
			break;

		case TF_TEAM_BLUE:
			r = 0.4; g = 0.5; b = 1.0;
			break;
			
		case TF_TEAM_GREEN:
			r = 0.4; g = 1.0; b = 0.5;
			break;

		case TF_TEAM_YELLOW:
			r = 1.0; g = 0.5; b = 0.5;
			break;

		default:
			r = 0.4; g = 0.5; b = 1.0;
			break;
	}

	m_pCloakColorTint->SetVecValue( r, g, b );
}

IMaterial *CSpyInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}

EXPOSE_INTERFACE( CSpyInvisProxy, IMaterialProxy, "spy_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for invulnerability material
//			Returns 1 if the player is invulnerable, and 0 if the player is losing / doesn't have invuln.
//-----------------------------------------------------------------------------
class CProxyInvulnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_TFPlayer *pPlayer = NULL;
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( 0.0 );
			return;
		}

		if ( pEntity->IsPlayer() )
		{
			pPlayer = dynamic_cast<C_TFPlayer *>( pEntity );
		}
		else
		{
			// See if it's a weapon
			C_TFWeaponBase *pWeapon = dynamic_cast<C_TFWeaponBase *>( pEntity );
			if ( pWeapon )
			{
				pPlayer = (C_TFPlayer *)pWeapon->GetOwner();
			}
			else
			{
				C_TFViewModel *pVM = dynamic_cast<C_TFViewModel *>( pEntity );
				if ( pVM )
				{
					pPlayer = (C_TFPlayer *)pVM->GetOwner();
				}
				else
				{
					C_ViewmodelAttachmentModel *pVMAddon = dynamic_cast<C_ViewmodelAttachmentModel *>( pEntity );
					if ( pVMAddon )
					{
						pVM = pVMAddon->m_ViewModel.Get();
						if ( pVM )
							pPlayer = (C_TFPlayer *)pVM->GetOwner();
					}
				}
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
			{
				m_pResult->SetFloatValue( 1.0 );
			}
			else
			{
				m_pResult->SetFloatValue( 0.0 );
			}
		}

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyInvulnLevel, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for burning material on player models
//			Returns 0.0->1.0 for level of burn to show on player skin
//-----------------------------------------------------------------------------
class CProxyBurnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( !pC_BaseEntity )
		{
			m_pResult->SetFloatValue( 0.0f );
			return;
		}

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		// default to zero
		float flBurnStartTime = 0;
		float flBurnEndTime = 0;

		C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer *>( pEntity );
		if ( pPlayer )
		{
			// is the player burning?
			if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
			{
				flBurnStartTime = pPlayer->m_flBurnEffectStartTime;
				flBurnEndTime = pPlayer->m_flBurnEffectEndTime;
			}
		}
		else
		{
			// is the ragdoll burning?
			C_TFRagdoll *pRagDoll = dynamic_cast<C_TFRagdoll *>( pEntity );
			if ( pRagDoll )
			{
				flBurnStartTime = pRagDoll->GetBurnStartTime();
				flBurnEndTime = pRagDoll->GetBurnEndTime();	
			}
		}

		float flResult = 0.0;

		// if player/ragdoll is burning, set the burn level on the skin
		if ( flBurnStartTime > 0 )
		{
			float flBurnPeakTime = flBurnStartTime + 0.3;
			float flTempResult;
			if ( gpGlobals->curtime < flBurnPeakTime )
			{
				// fade in from 0->1 in 0.3 seconds
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnStartTime, flBurnPeakTime, 0.0, 1.0 );
			}
			else
			{
				// fade out from 1->0 in the remaining time until flame extinguished
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnPeakTime, flBurnEndTime, 1.0, 0.0 );
			}

			// We have to do some more calc here instead of in materialvars.
			flResult = 1.0 - fabs( flTempResult - 1.0 );
		}

		m_pResult->SetFloatValue( flResult );

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyBurnLevel, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for jarate
//			Returns the RGB value for the appropriate tint condition.
//-----------------------------------------------------------------------------
class CProxyYellowLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( !pC_BaseEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer *>( pEntity );

		if ( !pPlayer )
		{
			C_TFWeaponBase *pWeapon = dynamic_cast<C_TFWeaponBase *>( pEntity );
			if ( pWeapon )
			{
				pPlayer = (C_TFPlayer *)pWeapon->GetOwner();
			}
			else
			{
				C_TFViewModel *pVM;
				C_ViewmodelAttachmentModel *pVMAddon = dynamic_cast<C_ViewmodelAttachmentModel *>( pEntity );
				if ( pVMAddon )
				{
					pVM = dynamic_cast<C_TFViewModel *>( pVMAddon->m_ViewModel.Get() );
				}
				else
				{
					pVM = dynamic_cast<C_TFViewModel *>( pEntity );
				}

				if ( pVM )
				{
					pPlayer = (C_TFPlayer *)pVM->GetOwner();
				}
			}
		}

		Vector vecColor = Vector( 1, 1, 1 );

		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_URINE ) )
		{

			vecColor.Init( 10, 10, 1 );
		}

		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};

EXPOSE_INTERFACE( CProxyYellowLevel, IMaterialProxy, "YellowLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for the weapon glow color when critted
//-----------------------------------------------------------------------------
class CProxyModelGlowColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( !pC_BaseEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		Vector vecColor = Vector( 1, 1, 1 );

		C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer *>( pEntity );

		if ( !pPlayer )
		{
			C_TFWeaponBase *pWeapon = dynamic_cast<C_TFWeaponBase *>( pEntity );
			if ( pWeapon )
			{
				pPlayer = (C_TFPlayer *)pWeapon->GetOwner();
			}
			else
			{
				C_TFViewModel *pVM;
				C_ViewmodelAttachmentModel *pVMAddon = dynamic_cast<C_ViewmodelAttachmentModel *>( pEntity );
				if ( pVMAddon )
				{
					pVM = dynamic_cast<C_TFViewModel *>( pVMAddon->m_ViewModel.Get() );
				}
				else
				{
					pVM = dynamic_cast<C_TFViewModel *>( pEntity );
				}

				if ( pVM )
				{
					pPlayer = (C_TFPlayer *)pVM->GetOwner();
				}
			}
		}
		/*
			Live TF2 crit glow colors
			RED Crit: 94 8 5
			BLU Crit: 6 21 80
			RED Mini-Crit: 237 140 55
			BLU Mini-Crit: 28 168 112
			Hype Mode: 50 2 50
		*/

		if ( pPlayer && pPlayer->m_Shared.IsCritBoosted() )
		{
			if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) ||
				pPlayer->InSameTeam( C_TFPlayer::GetLocalTFPlayer() ) ||
				pPlayer->GetTeamNumber() == pPlayer->m_Shared.GetDisguiseTeam() )
			{
				switch ( pPlayer->GetTeamNumber() )
				{
					case TF_TEAM_RED:
					vecColor = Vector( 94, 8, 5 );
					break;
					case TF_TEAM_BLUE:
					vecColor = Vector( 6, 21, 80 );
					break;
					case TF_TEAM_GREEN:
					vecColor = Vector( 1, 28, 9 );
					break;
					case TF_TEAM_YELLOW:
					vecColor = Vector( 28, 28, 9 );
					break;
				}
			}
		}
		else if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SODAPOPPER_HYPE ) )
		{
			vecColor = Vector( 50, 2, 48 );
		}
		else if ( pPlayer && pPlayer->m_Shared.IsMiniCritBoosted() )
		{
			if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) ||
				pPlayer->InSameTeam( C_TFPlayer::GetLocalTFPlayer() ) ||
				pPlayer->GetTeamNumber() == pPlayer->m_Shared.GetDisguiseTeam() )
			{
				switch ( pPlayer->GetTeamNumber() )
				{
					case TF_TEAM_RED:
						vecColor = Vector( 237, 140, 55 );
						break;
					case TF_TEAM_BLUE:
						vecColor = Vector( 28, 168, 112 );
						break;
				}
			}
		}
		else if ( pPlayer && ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) || pPlayer->m_Shared.m_bShieldChargeStopped ) )
		{
			float flAmt = 1.0f;
			if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
				flAmt = ( 100.0f - pPlayer->m_Shared.GetShieldChargeMeter() ) / 100.0f;
			else
				flAmt = Min( ( ( gpGlobals->curtime - pPlayer->m_Shared.m_flShieldChargeEndTime ) + -1.5f ) * 10.0f / 3.0f, 1.0f );

			switch ( pPlayer->GetTeamNumber() )
			{
				case TF_TEAM_RED:
					vecColor.x = Max( flAmt * 80.0f, 1.0f );
					vecColor.y = Max( flAmt * 8.0f, 1.0f );
					vecColor.z = Max( flAmt * 5.0f, 1.0f );
					break;
				case TF_TEAM_BLUE:
					vecColor.x = Max( flAmt * 5.0f, 1.0f );
					vecColor.y = Max( flAmt * 20.0f, 1.0f );
					vecColor.z = Max( flAmt * 80.0f, 1.0f );
					break;
			}
		}

		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};

EXPOSE_INTERFACE( CProxyModelGlowColor, IMaterialProxy, "ModelGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for coloring items 
//-----------------------------------------------------------------------------
class CProxyItemTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( !pC_BaseEntity )
			return;

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		CModelPanelModel *pPanelModel = dynamic_cast<CModelPanelModel *>( pEntity );
		if ( pPanelModel )
		{
			m_pResult->SetVecValue( pPanelModel->m_vecModelColor.x, pPanelModel->m_vecModelColor.y, pPanelModel->m_vecModelColor.z );
			return;
		}

		bool bBlueTeam = pEntity->GetTeam() > 0 && pEntity->GetTeamNumber() == TF_TEAM_BLUE;
		C_EconEntity *pEconEnt = dynamic_cast<C_EconEntity *>( pEntity );

		CEconItemView *pItem = NULL;
		if (pEconEnt)
		{
			pItem = pEconEnt->GetItem();
		}
		else
		{
			C_EconEntity *pOwner = dynamic_cast<C_EconEntity *>( pEntity->GetOwnerEntity() );
			if ( pOwner )
				pItem = pOwner->GetItem();
		}

		if ( !pItem )
		{
			C_TFWeaponBaseGrenadeProj *pProjectile = dynamic_cast<C_TFWeaponBaseGrenadeProj *>( pEntity );
			if ( pProjectile )
			{
				pEconEnt = dynamic_cast<C_EconEntity *>( pProjectile->m_hLauncher.Get() );
				if ( pEconEnt )
					pItem = pEconEnt->GetItem();
			}
		}

		if( pItem )
		{
			uint nPaintRGB = pItem->GetModifiedRGBValue( bBlueTeam );
			if ( nPaintRGB != 0 )
			{
				float flPaint[3] ={
					Clamp( ( (nPaintRGB & 0x00FF0000) >> 16 ) / 255.0f, 0.0f, 1.0f ),
					Clamp( ( (nPaintRGB & 0x0000FF00) >> 8 ) / 255.0f, 0.0f, 1.0f ),
					Clamp( ( (nPaintRGB & 0x000000FF) ) / 255.0f, 0.0f, 1.0f )
				};

				m_pResult->SetVecValue( flPaint[0], flPaint[1], flPaint[2] );
			}
			return;
		}

		m_pResult->SetVecValue( 0, 0, 0 );
	}
};

EXPOSE_INTERFACE( CProxyItemTintColor, IMaterialProxy, "ItemTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for pulsing the Wheatly Sappers eye glow when he talks
//-----------------------------------------------------------------------------
class CProxyWheatlyEyeGlow : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer )
			return;

		static float s_flCurr = 0.2f;
		static float s_flDir = 0.2f;
		static float s_flEyePose = 0.0f;
		static float s_flNextEyeChange = 0;
		static float s_flEyeDir = 0.005f;

		// TODO: Name constants

		C_TFWeaponSapper *pWeapon = dynamic_cast<C_TFWeaponSapper *>( pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_BUILDING ) );
		if ( pWeapon )
		{
			if ( pWeapon->IsWheatleyTalking() )
			{
				float flNoise = RandomGaussianFloat( 0.0f, 0.01f );
				s_flCurr += s_flDir + flNoise;

				if ( s_flCurr > 1.3f && s_flDir > 0 )
				{
					s_flDir = -0.006f;
				}
				else if ( s_flCurr < 0.6f && s_flDir < 0 )
				{
					s_flDir = 0.01f;
				}

				if ( s_flNextEyeChange < gpGlobals->curtime ) 
				{
					s_flEyePose += s_flEyeDir;

					if ( s_flEyePose > 0.35f && s_flEyeDir > 0 )
					{
						s_flNextEyeChange = gpGlobals->curtime + 1.9f;
						s_flEyeDir = -0.005f;
					}

					if ( s_flEyePose < 0.1f && s_flEyeDir < 0 )
					{
						s_flNextEyeChange = gpGlobals->curtime + 1.9f;
						s_flEyeDir = 0.005f;
					}
				}
			}
			else
			{
				s_flCurr += -0.006f;

				if ( s_flCurr < 0.4f )
				{
					s_flCurr = 0.4f;
					s_flDir = 0.01f;
				}

				if ( s_flEyePose > 0 )
				{
					s_flEyePose -= 0.005f;
				}

				s_flEyeDir = 0.005f;
			}

			CBaseViewModel *pViewModel = pPlayer->GetViewModel( 0 );
			if ( pViewModel )
			{
				pViewModel->SetPoseParameter( "eyelids", s_flEyePose );
			}
		}

		m_pResult->SetFloatValue( s_flCurr );

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyWheatlyEyeGlow, IMaterialProxy, "WheatlyEyeGlow" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Stub class for the CommunityWeapon material proxy used by live TF2
//-----------------------------------------------------------------------------
class CProxyCommunityWeapon : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return true;
	}
	void OnBind( void *pC_BaseEntity )
	{
	}
};

EXPOSE_INTERFACE( CProxyCommunityWeapon, IMaterialProxy, "CommunityWeapon" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Stub class for the AnimatedWeaponSheen material proxy used by live TF2
//-----------------------------------------------------------------------------
class CProxyAnimatedWeaponSheen : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return true;
	}
	void OnBind( void *pC_BaseEntity )
	{

	}
};

EXPOSE_INTERFACE( CProxyAnimatedWeaponSheen, IMaterialProxy, "AnimatedWeaponSheen" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Stub class for the WeaponSkin material proxy used by live TF2
//-----------------------------------------------------------------------------
class CProxyWeaponSkin : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		return true;
	}
	void OnBind( void *pC_BaseEntity )
	{

	}
};

EXPOSE_INTERFACE( CProxyWeaponSkin, IMaterialProxy, "WeaponSkin" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Universal proxy from live tf2 used for spy invisiblity material
//			Its' purpose is to replace weapon_invis, vm_invis and spy_invis
//-----------------------------------------------------------------------------
class CInvisProxy : public CEntityMaterialProxy
{
public:
	CInvisProxy( void );
	virtual				~CInvisProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *GetMaterial();

	virtual void		HandleSpyInvis( C_TFPlayer *pPlayer );
	virtual void		HandleVMInvis( C_TFViewModel *pVM );
	virtual void		HandleWeaponInvis( C_BaseEntity *pC_BaseEntity );

private:

	IMaterialVar *m_pPercentInvisible;
	IMaterialVar *m_pCloakColorTint;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInvisProxy::CInvisProxy( void )
{
	m_pPercentInvisible = NULL;
	m_pCloakColorTint = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInvisProxy::~CInvisProxy( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CInvisProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bInvis;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bInvis );

	bool bTint;
	m_pCloakColorTint = pMaterial->FindVar( "$cloakColorTint", &bTint );

	// if we have $cloakColorTint, it's spy_invis
	if ( bTint )
	{
		return ( bInvis && bTint );
	}

	return ( bTint );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !pEnt )
		return;

	m_pPercentInvisible->SetFloatValue( 0.0 );

	C_TFPlayer *pPlayer = ToTFPlayer( pEnt );

	C_TFViewModel *pVM;
	C_ViewmodelAttachmentModel *pVMAddon = dynamic_cast<C_ViewmodelAttachmentModel *>( pEnt );
	if ( pVMAddon )
	{
		pVM = dynamic_cast<C_TFViewModel *>( pVMAddon->m_ViewModel.Get() );
	}
	else
	{
		pVM = dynamic_cast<C_TFViewModel *>( pEnt );
	}

	if ( pPlayer )
	{
		HandleSpyInvis( pPlayer );
	}
	else if ( pVM )
	{
		HandleVMInvis( pVM );
	}
	else
	{
		HandleWeaponInvis( pEnt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleSpyInvis( C_TFPlayer *pPlayer )
{
	if ( !m_pPercentInvisible || !m_pCloakColorTint )
		return;

	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );

	float r, g, b;

	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			r = 1.0; g = 0.5; b = 0.4;
			break;

		case TF_TEAM_BLUE:
			r = 0.4; g = 0.5; b = 1.0;
			break;
			
		case TF_TEAM_GREEN:
			r = 0.4; g = 1.0; b = 0.5;
			break;

		case TF_TEAM_YELLOW:
			r = 1.0; g = 0.5; b = 0.5;
			break;

		default:
			r = 0.4; g = 0.5; b = 1.0;
			break;
	}

	m_pCloakColorTint->SetVecValue( r, g, b );
}

extern ConVar tf_vm_min_invis;
extern ConVar tf_vm_max_invis;
//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleVMInvis( C_TFViewModel *pVM )
{
	if ( !m_pPercentInvisible )
		return;

	C_TFPlayer *pPlayer = ToTFPlayer( pVM->GetOwner() );

	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	float flPercentInvisible = pPlayer->GetPercentInvisible();

	// remap from 0.22 to 0.5
	// but drop to 0.0 if we're not invis at all
	float flWeaponInvis = ( flPercentInvisible < 0.01 ) ?
		0.0 :
		RemapVal( flPercentInvisible, 0.0, 1.0, tf_vm_min_invis.GetFloat(), tf_vm_max_invis.GetFloat() );

	if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
	{
		// Hacky fix to make viewmodel blink more obvious
		m_pPercentInvisible->SetFloatValue( flWeaponInvis - 0.1 );
	}
	else
	{
		m_pPercentInvisible->SetFloatValue( flWeaponInvis );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleWeaponInvis( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	C_BaseEntity *pMoveParent = pEnt->GetMoveParent();
	if ( !pMoveParent || !pMoveParent->IsPlayer() )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	C_TFPlayer *pPlayer = ToTFPlayer( pMoveParent );
	Assert( pPlayer );

	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
}

IMaterial *CInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}

EXPOSE_INTERFACE( CInvisProxy, IMaterialProxy, "invis" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Player's object UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_PlayerObjectList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFPlayer *pPlayer = (C_TFPlayer *)pStruct;
	CBaseHandle *pHandle = (CBaseHandle *)( &( pPlayer->m_aObjects[pData->m_iElement] ) );
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );
}

void RecvProxyArrayLength_PlayerObjects( void *pStruct, int objectID, int currentArrayLength )
{
	C_TFPlayer *pPlayer = (C_TFPlayer *)pStruct;

	if ( pPlayer->m_aObjects.Count() != currentArrayLength )
	{
		pPlayer->m_aObjects.SetSize( currentArrayLength );
	}

	pPlayer->ForceUpdateObjectHudState();
}

// specific to the local player
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropArray2(
		RecvProxyArrayLength_PlayerObjects,
		RecvPropInt( "player_object_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerObjectList ),
		MAX_OBJECTS_PER_PLAYER,
		0,
		"player_object_array" ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
END_RECV_TABLE()

// all players except the local player
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFNonLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_TFPlayer, DT_TFPlayer, CTFPlayer )
	RecvPropBool( RECVINFO( m_bSaveMeParity ) ),

	// This will create a race condition with the local player, but the data will be the same so.....
	RecvPropInt( RECVINFO( m_nWaterLevel ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropDataTable( RECVINFO_DT( m_PlayerClass ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerClassShared ) ),
	RecvPropDataTable( RECVINFO_DT( m_Shared ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerShared ) ),
	RecvPropDataTable( RECVINFO_DT( m_AttributeManager ), 0, &REFERENCE_RECV_TABLE( DT_AttributeManager ) ),
	
	RecvPropEHandle( RECVINFO( m_hItem ) ),
	RecvPropEHandle( RECVINFO( m_hGrapplingHookTarget ) ),

	RecvPropVector( RECVINFO( m_vecPlayerColor ) ),

	RecvPropDataTable( "tflocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TFLocalPlayerExclusive ) ),
	RecvPropDataTable( "tfnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TFNonLocalPlayerExclusive ) ),

	RecvPropFloat( RECVINFO( m_flHeadScale ) ),

	RecvPropInt( RECVINFO( m_nForceTauntCam ) ),
	RecvPropBool( RECVINFO( m_bTyping ) ),
	RecvPropInt( RECVINFO( m_iSpawnCounter ) ),
	RecvPropFloat( RECVINFO( m_flLastDamageTime ) ),
	RecvPropFloat( RECVINFO( m_flInspectTime ) ),
END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_TFPlayer )
	DEFINE_PRED_TYPEDESCRIPTION( m_Shared, CTFPlayerShared ),
	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nMuzzleFlashParity, FIELD_CHARACTER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_hOffHandWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// ------------------------------------------------------------------------------------------ //
// C_TFPlayer implementation.
// ------------------------------------------------------------------------------------------ //

C_TFPlayer::C_TFPlayer() :
	m_iv_angEyeAngles( "C_TFPlayer::m_iv_angEyeAngles" )
{
	m_pAttributes = this;

	m_PlayerAnimState = CreateTFPlayerAnimState( this );
	m_Shared.Init( this );

	m_iIDEntIndex = 0;

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_pTeleporterEffect = NULL;
	m_pBurningSound = NULL;
	m_pBurningEffect = NULL;
	m_flBurnEffectStartTime = 0;
	m_flBurnEffectEndTime = 0;
	m_pDisguisingEffect = NULL;
	m_pSaveMeEffect = NULL;
	m_pOverhealEffect = NULL;
	m_pTypingEffect = NULL;

	m_aGibs.Purge();

	m_flHeadScale = 1.0f;

	m_bCigaretteSmokeActive = false;

	m_hRagdoll.Set( NULL );

	m_iPreviousMetal = 0;
	m_bIsDisplayingNemesisIcon = false;

	m_bWasTaunting = false;
	m_flTauntOffTime = 0.0f;
	m_angTauntPredViewAngles.Init();
	m_angTauntEngViewAngles.Init();

	m_flWaterImpactTime = 0.0f;

	m_flWaterEntryTime = 0;
	m_nOldWaterLevel = WL_NotInWater;
	m_bWaterExitEffectActive = false;

	m_bUpdateObjectHudState = false;
	
	m_bTyping = false;

	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "player_regenerate" );
	ListenForGameEvent( "sticky_jump" );
	ListenForGameEvent( "rocket_jump" );
	
	// Load phonemes from files.
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes.txt" );	// Basic English.
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes_local.txt" ); // Localized translations.
	engine->AddPhonemeFile( NULL );	// Finish loading phonemes.
}

C_TFPlayer::~C_TFPlayer()
{
	ShowNemesisIcon( false );
	m_PlayerAnimState->Release();
}

void C_TFPlayer::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( V_strcmp( type, "player_regenerate" ) == 0 )
	{
		// Regenerate Weapons
		for ( int i = 0; i < MAX_ITEMS; i++ )
		{
			C_TFWeaponBase *pWeapon = dynamic_cast<C_TFWeaponBase *>( Weapon_GetSlot( i ) );
			if ( pWeapon )
				pWeapon->WeaponRegenerate();
		}
	}
	else if ( V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		if ( !IsLocalPlayer() )
		{
			// Update any effects affected by disguise.
			m_Shared.UpdateCritBoostEffect();
			UpdateOverhealEffect();
		}
	}
	else if ( V_strcmp( event->GetName(), "rocket_jump" ) == 0 || V_strcmp( event->GetName(), "sticky_jump" ) == 0 )
	{
		if ( event->GetBool( "playsound" ) && GetUserID() == event->GetInt( "userid" ) && !m_pJumpSound )
		{
			C_RecipientFilter filter;
			filter.AddAllPlayers();
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

			if ( !m_pJumpSound )
			{
				CLocalPlayerFilter filter;
				m_pJumpSound = controller.SoundCreate( filter, entindex(), "BlastJump.Whistle" );
			}

			controller.Play( m_pJumpSound, 0.0, 100 );
			controller.SoundChangeVolume( m_pJumpSound, 1.0, 0.1 );
		}
	}
	else
	{
		BaseClass::FireGameEvent( event );
	}
}

C_TFPlayer *C_TFPlayer::GetLocalTFPlayer()
{
	return ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
}

const QAngle &C_TFPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateOnRemove( void )
{
	// Stop the taunt.
	if ( m_bWasTaunting )
	{
		TurnOffTauntCam();
	}

	// HACK!!! ChrisG needs to fix this in the particle system.
	ParticleProp()->OwnerSetDormantTo( true );
	ParticleProp()->StopParticlesInvolving( this );

	m_Shared.RemoveAllCond( this );
	m_Shared.ResetMeters();
	m_Shared.UpdateCritBoostEffect( true );

	if ( IsLocalPlayer() )
	{
		CTFStatPanel *pStatPanel = GetStatPanel();
		pStatPanel->OnLocalPlayerRemove( this );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: returns max health for this player
//-----------------------------------------------------------------------------
int C_TFPlayer::GetMaxHealth( void ) const
{
	if ( TFPlayerResource() )
	{
		return TFPlayerResource()->GetMaxHealth( entindex() );
	}

	return TF_HEALTH_UNDEFINED;
}

int C_TFPlayer::GetMaxHealthForBuffing( void ) const
{
	if ( TFPlayerResource() )
	{
		return TFPlayerResource()->GetMaxHealthForBuffing( entindex() );
	}

	return TF_HEALTH_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Deal with recording
//-----------------------------------------------------------------------------
void C_TFPlayer::GetToolRecordingState( KeyValues *msg )
{
#ifndef _XBOX
	BaseClass::GetToolRecordingState( msg );
	BaseEntityRecordingState_t *pBaseEntityState = (BaseEntityRecordingState_t *)msg->GetPtr( "baseentity" );

	bool bDormant = IsDormant();
	bool bDead = !IsAlive();
	bool bSpectator = ( GetTeamNumber() == TEAM_SPECTATOR );
	bool bNoRender = ( GetRenderMode() == kRenderNone );
	bool bDeathCam = ( GetObserverMode() == OBS_MODE_DEATHCAM );
	bool bNoDraw = IsEffectActive( EF_NODRAW );

	bool bVisible =
		!bDormant &&
		!bDead &&
		!bSpectator &&
		!bNoRender &&
		!bDeathCam &&
		!bNoDraw;

	bool changed = m_bToolRecordingVisibility != bVisible;
	// Remember state
	m_bToolRecordingVisibility = bVisible;

	pBaseEntityState->m_bVisible = bVisible;
	if ( changed && !bVisible )
	{
		// If the entity becomes invisible this frame, we still want to record a final animation sample so that we have data to interpolate
		//  toward just before the logs return "false" for visiblity.  Otherwise the animation will freeze on the last frame while the model
		//  is still able to render for just a bit.
		pBaseEntityState->m_bRecordFinalVisibleSample = true;
	}
#endif
}


void C_TFPlayer::UpdateClientSideAnimation()
{
	// Update the animation data. It does the local check here so this works when using
	// a third-person camera (and we don't have valid player angles).

	if ( this == C_TFPlayer::GetLocalTFPlayer() )
		m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );
	else
		m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetDormant( bool bDormant )
{
	// If I'm burning, stop the burning sounds
	if ( !IsDormant() && bDormant )
	{
		if ( m_pBurningSound )
		{
			StopBurningSound();
		}
		if ( m_bIsDisplayingNemesisIcon )
		{
			ShowNemesisIcon( false );
		}
	}

	if ( IsDormant() && !bDormant )
	{
		m_bUpdatePartyHat = true;
	}

	m_Shared.UpdateCritBoostEffect( true );
	UpdateOverhealEffect( true );

	// Deliberately skip base combat weapon
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldHealth = m_iHealth;
	m_iOldPlayerClass = m_PlayerClass.GetClassIndex();
	m_iOldState = m_Shared.GetCond();
	m_iOldSpawnCounter = m_iSpawnCounter;
	m_bOldSaveMeParity = m_bSaveMeParity;
	m_nOldWaterLevel = GetWaterLevel();

	m_iOldTeam = GetTeamNumber();
	C_TFPlayerClass *pClass = GetPlayerClass();
	m_iOldClass = pClass ? pClass->GetClassIndex() : TF_CLASS_UNDEFINED;
	m_bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
	m_iOldDisguiseTeam = m_Shared.GetDisguiseTeam();
	m_iOldDisguiseClass = m_Shared.GetDisguiseClass();
	m_hOldActiveWeapon.Set( GetActiveTFWeapon() );

	m_Shared.OnPreDataChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );

	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		InitInvulnerableMaterial();
	}
	else
	{
		if ( m_iOldTeam != GetTeamNumber() || m_iOldDisguiseTeam != m_Shared.GetDisguiseTeam() )
		{
			InitInvulnerableMaterial();
			m_bUpdatePartyHat = true;
		}

		UpdateWearables();
	}

	C_TFWeaponBase *pActiveWpn = GetActiveTFWeapon();
	if ( pActiveWpn )
	{
		if ( m_hOldActiveWeapon.Get() == NULL ||
			pActiveWpn != m_hOldActiveWeapon.Get() ||
			m_iOldPlayerClass != m_PlayerClass.GetClassIndex() )
		{
			pActiveWpn->SetViewModel();

			if ( ShouldDrawThisPlayer() )
			{
				m_Shared.UpdateCritBoostEffect();
			}
		}
	}

	// Check for full health and remove decals.
	if ( ( m_iHealth > m_iOldHealth && m_iHealth >= GetMaxHealth() ) || m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		// If we were just fully healed, remove all decals
		RemoveAllDecals();
	}

	// Detect class changes
	if ( m_iOldPlayerClass != m_PlayerClass.GetClassIndex() )
	{
		OnPlayerClassChange();
	}

	bool bJustSpawned = false;

	if ( m_iOldSpawnCounter != m_iSpawnCounter )
	{
		ClientPlayerRespawn();

		bJustSpawned = true;
		m_bUpdatePartyHat = true;
	}

	if ( m_bSaveMeParity != m_bOldSaveMeParity )
	{
		// Player has triggered a save me command
		CreateSaveMeEffect();
	}
	
	UpdateTypingBubble();

	if ( m_Shared.InCond( TF_COND_BURNING ) && !m_pBurningSound )
	{
		StartBurningSound();
	}

	if ( !m_Shared.InCond( TF_COND_BLASTJUMPING ) && m_pJumpSound )
	{
		if ( m_pJumpSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pJumpSound );
			m_pJumpSound = NULL;
		}
	}

	// See if we should show or hide nemesis icon for this player
	bool bShouldDisplayNemesisIcon = ShouldShowNemesisIcon();
	if ( bShouldDisplayNemesisIcon != m_bIsDisplayingNemesisIcon )
	{
		ShowNemesisIcon( bShouldDisplayNemesisIcon );
	}

	m_Shared.OnDataChanged();

	if ( m_bDisguised != m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		m_flDisguiseEndEffectStartTime = Max( m_flDisguiseEndEffectStartTime, gpGlobals->curtime );
	}

	int nNewWaterLevel = GetWaterLevel();

	if ( nNewWaterLevel != m_nOldWaterLevel )
	{
		if ( ( m_nOldWaterLevel == WL_NotInWater ) && ( nNewWaterLevel > WL_NotInWater ) )
		{
			// Set when we do a transition to/from partially in water to completely out
			m_flWaterEntryTime = gpGlobals->curtime;
		}

		// If player is now up to his eyes in water and has entered the water very recently (not just bobbing eyes in and out), play a bubble effect.
		if ( ( nNewWaterLevel == WL_Eyes ) && ( gpGlobals->curtime - m_flWaterEntryTime ) < 0.5f )
		{
			CNewParticleEffect *pEffect = ParticleProp()->Create( "water_playerdive", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pEffect, 1, NULL, PATTACH_WORLDORIGIN, NULL, WorldSpaceCenter() );
		}
		// If player was up to his eyes in water and is now out to waist level or less, play a water drip effect
		else if ( m_nOldWaterLevel == WL_Eyes && ( nNewWaterLevel < WL_Eyes ) && !bJustSpawned )
		{
			CNewParticleEffect *pWaterExitEffect = ParticleProp()->Create( "water_playeremerge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pWaterExitEffect, 1, this, PATTACH_ABSORIGIN_FOLLOW );
			m_bWaterExitEffectActive = true;
		}
	}

	if ( IsLocalPlayer() )
	{
		if ( updateType == DATA_UPDATE_CREATED )
		{
			SetupHeadLabelMaterials();
			GetClientVoiceMgr()->SetHeadLabelOffset( 50 );
		}

		if ( m_iOldTeam != GetTeamNumber() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeteam" );
			if ( event )
			{
				gameeventmanager->FireEventClientSide( event );
			}
			if ( IsX360() )
			{
				const char *pTeam = NULL;
				switch ( GetTeamNumber() )
				{
					case TF_TEAM_RED:
						pTeam = "red";
						break;

					case TF_TEAM_BLUE:
						pTeam = "blue";
						break;

					case TF_TEAM_GREEN:
						pTeam = "green";
						break;

					case TF_TEAM_YELLOW:
						pTeam = "yellow";
						break;
						
					case TEAM_SPECTATOR:
						pTeam = "spectate";
						break;
				}

				if ( pTeam )
				{
					engine->ChangeTeam( pTeam );
				}
			}
		}

		if ( !IsPlayerClass( m_iOldClass ) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeclass" );
			if ( event )
			{
				event->SetInt( "updateType", updateType );
				gameeventmanager->FireEventClientSide( event );
			}
		}


		if ( m_iOldClass == TF_CLASS_SPY &&
			( m_bDisguised != m_Shared.InCond( TF_COND_DISGUISED ) || m_iOldDisguiseClass != m_Shared.GetDisguiseClass() ) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changedisguise" );
			if ( event )
			{
				event->SetBool( "disguised", m_Shared.InCond( TF_COND_DISGUISED ) );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		// If our metal amount changed, send a game event
		int iCurrentMetal = GetAmmoCount( TF_AMMO_METAL );

		if ( iCurrentMetal != m_iPreviousMetal )
		{
			//msg
			IGameEvent *event = gameeventmanager->CreateEvent( "player_account_changed" );
			if ( event )
			{
				event->SetInt( "old_account", m_iPreviousMetal );
				event->SetInt( "new_account", iCurrentMetal );
				gameeventmanager->FireEventClientSide( event );
			}

			m_iPreviousMetal = iCurrentMetal;
		}

	}

	// Some time in this network transmit we changed the size of the object array.
	// recalc the whole thing and update the hud
	if ( m_bUpdateObjectHudState )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "building_info_changed" );
		if ( event )
		{
			event->SetInt( "building_type", -1 );
			event->SetInt( "object_mode", OBJECT_MODE_NONE );
			gameeventmanager->FireEventClientSide( event );
		}

		m_bUpdateObjectHudState = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitInvulnerableMaterial( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	const char *pszMaterial = NULL;

	int iVisibleTeam = GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !InSameTeam( pLocalPlayer ) )
	{
		iVisibleTeam = m_Shared.GetDisguiseTeam();
	}

	switch ( iVisibleTeam )
	{
		case TF_TEAM_RED:
			pszMaterial = "models/effects/invulnfx_red.vmt";
			break;
		case TF_TEAM_BLUE:
			pszMaterial = "models/effects/invulnfx_blue.vmt";
			break;
		case TF_TEAM_GREEN:
			pszMaterial = "models/effects/invulnfx_green.vmt";
			break;
		case TF_TEAM_YELLOW:
			pszMaterial = "models/effects/invulnfx_yellow.vmt";
			break;
		default:
			break;
	}

	if ( pszMaterial )
	{
		m_InvulnerableMaterial.Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
	else
	{
		m_InvulnerableMaterial.Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::StartBurningSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( !m_pBurningSound )
	{
		CLocalPlayerFilter filter;
		m_pBurningSound = controller.SoundCreate( filter, entindex(), "Player.OnFire" );
	}

	controller.Play( m_pBurningSound, 0.0, 100 );
	controller.SoundChangeVolume( m_pBurningSound, 1.0, 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::StopBurningSound( void )
{
	if ( m_pBurningSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBurningSound );
		m_pBurningSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::GetGlowEffectColor( float *r, float *g, float *b )
{
	switch ( GetTeamNumber() )
	{
		case TF_TEAM_BLUE:
			*r = 0.49f; *g = 0.66f; *b = 0.7699971f;
			break;

		case TF_TEAM_RED:
			*r = 0.74f; *g = 0.23f; *b = 0.23f;
			break;

		case TF_TEAM_GREEN:
			*r = 0.03f; *g = 0.68f; *b = 0;
			break;

		case TF_TEAM_YELLOW:
			*r = 1.0f; *g = 0.62f; *b = 0;
			break;
			
		default:
			*r = 0.76f; *g = 0.76f; *b = 0.76f;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateRecentlyTeleportedEffect( void )
{
	if ( m_Shared.ShouldShowRecentlyTeleported() )
	{
		if ( !m_pTeleporterEffect )
		{
			int iTeam = GetTeamNumber();
			if ( m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				iTeam = m_Shared.GetDisguiseTeam();
			}

			const char *pszEffect = ConstructTeamParticle( "player_recent_teleport_%s", iTeam );

			if ( pszEffect )
			{
				m_pTeleporterEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
			}
		}
	}
	else
	{
		if ( m_pTeleporterEffect )
		{
			ParticleProp()->StopEmission( m_pTeleporterEffect );
			m_pTeleporterEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Kill viewmodel particle effects
//-----------------------------------------------------------------------------
void C_TFPlayer::StopViewModelParticles( C_BaseEntity *pEntity )
{
	if ( pEntity && pEntity->ParticleProp() )
	{
		pEntity->ParticleProp()->StopEmissionAndDestroyImmediately();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnPlayerClassChange( void )
{
	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	// Execute the class cfg
	if (IsLocalPlayer())
	{
		char szCommand[128];
		Q_snprintf(szCommand, sizeof(szCommand), "exec %s.cfg\n", GetPlayerClass()->GetName());
		engine->ExecuteClientCmd(szCommand);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitPhonemeMappings()
{
	CStudioHdr *pStudio = GetModelPtr();
	if ( pStudio )
	{
		char szBasename[MAX_PATH];
		Q_StripExtension( pStudio->pszName(), szBasename, sizeof( szBasename ) );
		char szExpressionName[MAX_PATH];
		Q_snprintf( szExpressionName, sizeof( szExpressionName ), "%s/phonemes/phonemes", szBasename );
		if ( FindSceneFile( szExpressionName ) )
		{
			SetupMappings( szExpressionName );
		}
		else
		{
			BaseClass::InitPhonemeMappings();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ResetFlexWeights( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr || pStudioHdr->numflexdesc() == 0 )
		return;

	// Reset the flex weights to their starting position.
	LocalFlexController_t iController;
	for ( iController = LocalFlexController_t( 0 ); iController < pStudioHdr->numflexcontrollers(); ++iController )
	{
		SetFlexWeight( iController, 0.0f );
	}

	// Reset the prediction interpolation values.
	m_iv_flexWeight.Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_TFPlayer::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Initialize the gibs.
	InitPlayerGibs();

	InitializePoseParams();

	// Init flexes, cancel any scenes we're playing
	ClearSceneEvents( NULL, false );

	// Reset the flex weights.
	ResetFlexWeights( hdr );

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->OnNewModel();
	}

	if ( hdr )
	{
		InitPhonemeMappings();
	}

	// No longer needed for rendering, but still used for code.
	
	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		m_iSpyMaskBodygroup = FindBodygroupByName( "spyMask" );
	}
	
	else
	{
		m_iSpyMaskBodygroup = -1;
	}
	
	if ( m_hSpyMask )
	{
		// Local player must have changed team.
		m_hSpyMask->UpdateVisibility();
	}
	m_bUpdatePartyHat = true;

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateSpyMask(void)
{
	C_TFSpyMask *pMask = m_hSpyMask.Get();

	if ( IsAlive() )
	{
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			// Create mask if we don't already have one.
			if ( !pMask )
			{
				pMask = new C_TFSpyMask();

				if ( !pMask->InitializeAsClientEntity( "models/player/items/spy/spyMask.mdl", RENDER_GROUP_OPAQUE_ENTITY ) )
				{
					pMask->Release();
					return;
				}

				pMask->SetOwnerEntity( this );
				pMask->FollowEntity( this );
				pMask->UpdateVisibility();

				m_hSpyMask = pMask;
			}
		}
		else if ( pMask )
		{
			pMask->Release();
			m_hSpyMask = NULL;
		}
	}
	else
	{
		if ( pMask )
		{
			pMask->AddEffects(EF_NODRAW);
			pMask->SetMoveType(MOVETYPE_NONE);
			pMask->Release();
			m_hSpyMask = NULL;
		}	
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdatePartyHat( void )
{
	if ( TFGameRules() && TFGameRules()->IsBirthday() && !IsLocalPlayer() && IsAlive() &&
		GetTeamNumber() >= FIRST_GAME_TEAM && !IsPlayerClass( TF_CLASS_UNDEFINED ) )
	{
		if ( m_hPartyHat )
		{
			m_hPartyHat->Release();
		}

		m_hPartyHat = C_PlayerAttachedModel::Create( BDAY_HAT_MODEL, this, LookupAttachment( "partyhat" ), vec3_origin, PAM_PERMANENT, 0 );

		// C_PlayerAttachedModel::Create can return NULL!
		if ( m_hPartyHat )
		{
			int iVisibleTeam = GetTeamNumber();
			if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
			{
				iVisibleTeam = m_Shared.GetDisguiseTeam();
			}
			m_hPartyHat->m_nSkin = iVisibleTeam - 2;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is this player an enemy to the local player
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsEnemyPlayer( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return false;

	switch ( pLocalPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			return ( GetTeamNumber() == TF_TEAM_BLUE || GetTeamNumber() == TF_TEAM_GREEN || GetTeamNumber() == TF_TEAM_YELLOW );

		case TF_TEAM_BLUE:
			return ( GetTeamNumber() == TF_TEAM_RED || GetTeamNumber() == TF_TEAM_GREEN || GetTeamNumber() == TF_TEAM_YELLOW );

		case TF_TEAM_GREEN:
			return ( GetTeamNumber() == TF_TEAM_RED || GetTeamNumber() == TF_TEAM_BLUE || GetTeamNumber() == TF_TEAM_YELLOW );

		case TF_TEAM_YELLOW:
			return ( GetTeamNumber() == TF_TEAM_RED || GetTeamNumber() == TF_TEAM_BLUE || GetTeamNumber() == TF_TEAM_GREEN );

		default:
			return false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Displays a nemesis icon on this player to the local player
//-----------------------------------------------------------------------------
void C_TFPlayer::ShowNemesisIcon( bool bShow )
{
	if ( bShow )
	{
		const char *pszEffect = ConstructTeamParticle( "particle_nemesis_%s", GetTeamNumber(), true );
		ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "head" );
	}
	else
	{
		// stop effects for both team colors (to make sure we remove effects in event of team change)
		ParticleProp()->StopParticlesNamed( "particle_nemesis_red", true );
		ParticleProp()->StopParticlesNamed( "particle_nemesis_blue", true );
	}
	m_bIsDisplayingNemesisIcon = bShow;
}

#define	TF_TAUNT_PITCH	0
#define TF_TAUNT_YAW	1
#define TF_TAUNT_DIST	2

#define TF_TAUNT_MAXYAW		135
#define TF_TAUNT_MINYAW		-135
#define TF_TAUNT_MAXPITCH	90
#define TF_TAUNT_MINPITCH	0
#define TF_TAUNT_IDEALLAG	4.0f

static Vector TF_TAUNTCAM_HULL_MIN( -9.0f, -9.0f, -9.0f );
static Vector TF_TAUNTCAM_HULL_MAX( 9.0f, 9.0f, 9.0f );

static ConVar tf_tauntcam_yaw( "tf_tauntcam_yaw", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
static ConVar tf_tauntcam_pitch( "tf_tauntcam_pitch", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
static ConVar tf_tauntcam_dist( "tf_tauntcam_dist", "150", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar setcamerathird( "setcamerathird", "0", 0 );

extern ConVar cam_idealdist;
extern ConVar cam_idealdistright;
extern ConVar cam_idealdistup;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOnTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;

	// Already in third person?
	if ( g_ThirdPersonManager.WantToUseGameThirdPerson() )
		return;

	// Save the old view angles.
	/*engine->GetViewAngles( m_angTauntEngViewAngles );
	prediction->GetViewAngles( m_angTauntPredViewAngles );*/

	m_TauntCameraData.m_flPitch = tf_tauntcam_pitch.GetFloat();
	m_TauntCameraData.m_flYaw =  tf_tauntcam_yaw.GetFloat();
	m_TauntCameraData.m_flDist = tf_tauntcam_dist.GetFloat();
	m_TauntCameraData.m_flLag = 4.0f;
	m_TauntCameraData.m_vecHullMin.Init( -9.0f, -9.0f, -9.0f );
	m_TauntCameraData.m_vecHullMax.Init( 9.0f, 9.0f, 9.0f );

	QAngle vecCameraOffset( tf_tauntcam_pitch.GetFloat(), tf_tauntcam_yaw.GetFloat(), tf_tauntcam_dist.GetFloat() );

	g_ThirdPersonManager.SetDesiredCameraOffset( Vector( tf_tauntcam_dist.GetFloat(), 0.0f, 0.0f ) );
	g_ThirdPersonManager.SetOverridingThirdPerson( true );
	::input->CAM_ToThirdPerson();
	ThirdPersonSwitch( true );

	::input->CAM_SetCameraThirdData( &m_TauntCameraData, vecCameraOffset );

	if ( m_hItem )
	{
		m_hItem->UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOffTauntCam( void )
{
	m_bWasTaunting = false;
	m_flTauntOffTime = 0.0f;

	if ( !IsLocalPlayer() )
		return;

	/*Vector vecOffset = g_ThirdPersonManager.GetCameraOffsetAngles();

	tf_tauntcam_pitch.SetValue( vecOffset[PITCH] - m_angTauntPredViewAngles[PITCH] );
	tf_tauntcam_yaw.SetValue( vecOffset[YAW] - m_angTauntPredViewAngles[YAW] );*/

	g_ThirdPersonManager.SetOverridingThirdPerson( false );
	::input->CAM_SetCameraThirdData( NULL, vec3_angle );

	if ( g_ThirdPersonManager.WantToUseGameThirdPerson() )
	{
		ThirdPersonSwitch( true );
		return;
	}

	::input->CAM_ToFirstPerson();
	ThirdPersonSwitch( false );

	// Reset the old view angles.
	/*engine->SetViewAngles( m_angTauntEngViewAngles );
	prediction->SetViewAngles( m_angTauntPredViewAngles );*/

	// Force the feet to line up with the view direction post taunt.
	m_PlayerAnimState->m_bForceAimYaw = true;

	if ( GetViewModel() )
	{
		GetViewModel()->UpdateVisibility();
	}

	if ( m_hItem )
	{
		m_hItem->UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::HandleTaunting( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Clear the taunt slot.
	if ( ( !m_bWasTaunting || m_flTauntOffTime != 0.0f ) && (
		m_Shared.InCond( TF_COND_TAUNTING ) ||
		m_Shared.IsControlStunned() ||
		m_Shared.IsLoser() ||
		m_nForceTauntCam ||
		m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) ||
		m_Shared.InCond( TF_COND_HALLOWEEN_GIANT ) ||
		m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) ||
		m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) ) )
	{
		m_bWasTaunting = true;
		m_flTauntOffTime = 0.0f;

		// Handle the camera for the local player.
		if ( pLocalPlayer )
		{
			TurnOnTauntCam();
		}
	}

	if ( m_bWasTaunting && m_flTauntOffTime == 0.0f &&  (
		!m_Shared.InCond( TF_COND_TAUNTING ) &&
		!m_Shared.IsControlStunned() &&
		!m_Shared.IsLoser() &&
		!m_nForceTauntCam &&
		!m_Shared.InCond( TF_COND_PHASE ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_GIANT ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) ) )
	{
		m_flTauntOffTime = gpGlobals->curtime;

		// Clear the vcd slot.
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	}

	TauntCamInterpolation();
}

//---------------------------------------------------------------------------- -
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TauntCamInterpolation( void )
{
	if ( m_flTauntOffTime != 0.0f )
	{
		// Pull the camera back in over the course of half a second.
		float flDist = RemapValClamped( gpGlobals->curtime - m_flTauntOffTime, 0.0f, 0.5f, tf_tauntcam_dist.GetFloat(), 0.0f );

		// Snap the camera back into first person
		if ( flDist == 0.0f || !m_bWasTaunting || !IsAlive() || g_ThirdPersonManager.WantToUseGameThirdPerson() )
		{
			TurnOffTauntCam();
		}
		else
		{
			g_ThirdPersonManager.SetDesiredCameraOffset( Vector( flDist, 0.0f, 0.0f ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );

	if ( bThirdPerson )
	{
		if ( g_ThirdPersonManager.WantToUseGameThirdPerson() )
		{
			Vector vecOffset( TF_CAMERA_DIST, TF_CAMERA_DIST_RIGHT, TF_CAMERA_DIST_UP );

			// Flip the angle if viewmodels are flipped.
			if ( cl_flipviewmodels.GetBool() )
			{
				vecOffset.y *= -1.0f;
			}

			g_ThirdPersonManager.SetDesiredCameraOffset( vecOffset );
		}
	}

	m_Shared.UpdateCritBoostEffect();
	UpdateOverhealEffect();
}

void C_TFPlayer::BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed );

	BuildBigHeadTransformation( this, pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed, m_flHeadScale );

	BuildFirstPersonMeathookTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed, "bip_head" );
}

void C_TFPlayer::CreateBoneAttachmentsFromWearables( C_TFRagdoll *pRagdoll, bool bDisguised )
{
	if ( bDisguised && !ShouldDrawSpyAsDisguised() )
		return;

	FOR_EACH_VEC( m_hMyWearables, i )
	{
		C_TFWearable *pTFWearable = dynamic_cast<C_TFWearable *>( m_hMyWearables[i].Get() );
		if ( pTFWearable == nullptr )
			continue;

		if ( pTFWearable->IsViewModelWearable() )
			continue;

		/*if ( bDisguised && !pTFWearable->m_bDisguiseWearable ||
			 !bDisguised && pTFWearable->m_bDisguiseWearable )
			continue;*/

		if ( pTFWearable->GetFlags() & EF_NODRAW )
			continue;

		//pTFWearable->OnWearerDeath();

		if ( pTFWearable->GetDropType() > DROPTYPE_NONE ) // Fall off or break
			continue;

		if ( pRagdoll->m_iDamageCustom == TF_DMG_CUSTOM_DECAPITATION_BOSS || 
			 pRagdoll->m_iDamageCustom == TF_DMG_CUSTOM_MERASMUS_DECAPITATION ||
			 pRagdoll->m_iDamageCustom == TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING || 
			 pRagdoll->m_iDamageCustom == TF_DMG_CUSTOM_DECAPITATION )
		 {
			// Don't put a hat on a decapitated corpse!
			 if (pTFWearable->GetLoadoutSlot() == TF_LOADOUT_SLOT_HAT)
				continue; 
		 }

		C_EconWearableGib *pProp = new C_EconWearableGib;
		if ( pProp == nullptr )
			return;

		const model_t *pModel = pTFWearable->GetModel();
		const char *szModel = modelinfo->GetModelName( pModel );
		if ( !szModel || !*szModel || *szModel == '?' )
			continue;

		const CEconItemView *pItem = pTFWearable->GetItem();
		if ( pItem )
			pProp->SetItem( *pItem );

		string_t iModel = AllocPooledString( szModel );
		pProp->SetModelName( iModel );

		if ( pProp->Initialize( true ) )
		{
			pProp->m_nSkin = pTFWearable->GetSkin();
			pProp->AttachEntityToBone( this, -1, Vector(0,0,0), QAngle(0,0,0) );

			// We set this when we update the wearables, no need to do it twice.
			/*if ( pItem && pItem->GetStaticData() )
			{
				PerTeamVisuals_t *pVisuals = pItem->GetStaticData()->GetVisuals( GetTeamNumber() );
				if ( pVisuals && pVisuals->use_per_class_bodygroups )
					pProp->SetBodygroup( 1, pRagdoll->m_iClass - 1 );
			}*/
		}
		else
		{
			pProp->Release();
			continue;
		}
	}
}

void C_TFPlayer::CalcMinViewmodelOffset( void )
{
	for ( int i = 0; i < MAX_VIEWMODELS; i++ )
	{
		C_TFViewModel *vm = dynamic_cast<C_TFViewModel *>( GetViewModel( i ) );
		if ( !vm )
			continue;

		vm->CalcMinViewmodelOffset( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TFPlayer::CanLightCigarette( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Start smoke if we're not invisible or disguised
	if ( IsPlayerClass( TF_CLASS_SPY ) && IsAlive() &&									// only on spy model
		( !m_Shared.InCond( TF_COND_DISGUISED ) || !IsEnemyPlayer() ) &&	// disguise doesn't show for teammates
		GetPercentInvisible() <= 0 &&										// don't start if invis
		( pLocalPlayer != this ) && 										// don't show to local player
		!m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) &&				// don't show if we're a dispenser
		!( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalPlayer->GetObserverTarget() == this ) )	// not if we're spectating this player first person
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldDrawSpyAsDisguised( void )
{
	C_BasePlayer *pLocal = GetLocalPlayer();
	if ( pLocal == nullptr )
		return false;

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		int iEnemyTeam = GetEnemyTeam( this );
		if ( iEnemyTeam == pLocal->GetTeamNumber() )
		{
			if ( m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) )
				return true;

			if ( m_Shared.GetDisguiseTeam() != pLocal->GetTeamNumber() )
				return true;
		}
	}

	return false;
}

void C_TFPlayer::ClientThink()
{
	// Pass on through to the base class.
	BaseClass::ClientThink();

	UpdateIDTarget();
	UpdatePlayerGlows();
	UpdateLookAt();

	// Handle invisibility.
	m_Shared.InvisibilityThink();

	m_Shared.ConditionThink();

	m_Shared.ClientShieldChargeThink();
	m_Shared.ClientDemoBuffThink();

	// Clear our healer, it'll be reset by the medigun client think if we're being healed
	m_hHealer = NULL;


	if ( CanLightCigarette() )
	{
		if ( !m_bCigaretteSmokeActive )
		{
			int iSmokeAttachment = LookupAttachment( "cig_smoke" );
			ParticleProp()->Create( "cig_smoke", PATTACH_POINT_FOLLOW, iSmokeAttachment );
			m_bCigaretteSmokeActive = true;
		}
	}
	else	// stop the smoke otherwise if its active
	{
		if ( m_bCigaretteSmokeActive )
		{
			ParticleProp()->StopParticlesNamed( "cig_smoke", false );
			m_bCigaretteSmokeActive = false;
		}
	}

	if ( m_bWaterExitEffectActive && !IsAlive() )
	{
		ParticleProp()->StopParticlesNamed( "water_playeremerge", false );
		m_bWaterExitEffectActive = false;
	}

	if ( m_bUpdatePartyHat )
	{
		UpdatePartyHat();
		m_bUpdatePartyHat = false;
	}

	if ( m_pSaveMeEffect )
	{
		// Kill the effect if either
		// a) the player is dead
		// b) the enemy disguised spy is now invisible

		if ( !IsAlive() ||
			( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() && ( GetPercentInvisible() > 0 ) ) )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pSaveMeEffect );
			m_pSaveMeEffect = NULL;
		}
	}

	if ( ( !IsAlive() || IsPlayerDead() ) && IsLocalPlayer() )
	{
		if ( GetTeamNumber() != TEAM_SPECTATOR && GetObserverMode() != OBS_MODE_IN_EYE )
		{
			C_TFViewModel *vm = dynamic_cast<C_TFViewModel *>( GetViewModel( 0 ) );
			if ( vm )
			{
				for ( int i = 0; i < MAX_VIEWMODELS; i++ )
				{
					vm->RemoveViewmodelAddon( i );
				}
			}
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateLookAt( void )
{
	bool bFoundViewTarget = false;

	Vector vForward;
	AngleVectors( GetLocalAngles(), &vForward );

	Vector vMyOrigin =  GetAbsOrigin();

	Vector vecLookAtTarget = vec3_origin;

	for ( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
	{
		C_BaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if ( !pEnt || !pEnt->IsPlayer() )
			continue;

		if ( !pEnt->IsAlive() )
			continue;

		if ( pEnt == this )
			continue;

		Vector vDir = pEnt->GetAbsOrigin() - vMyOrigin;

		if ( vDir.Length() > 300 )
			continue;

		VectorNormalize( vDir );

		if ( DotProduct( vForward, vDir ) < 0.0f )
			continue;

		vecLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if ( bFoundViewTarget == false )
	{
		// no target, look forward
		vecLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	// orient eyes
	m_viewtarget = vecLookAtTarget;

	// TODO: fix blink track for player models
	// blinking
	/*if (m_blinkTimer.IsElapsed())
	{
		m_blinktoggle = !m_blinktoggle;
		m_blinkTimer.Start( RandomFloat( 1.5f, 4.0f ) );
	}*/


	/*
	// Figure out where we want to look in world space.
	QAngle desiredAngles;
	Vector to = vecLookAtTarget - EyePosition();
	VectorAngles( to, desiredAngles );

	// Figure out where our body is facing in world space.
	QAngle bodyAngles( 0, 0, 0 );
	bodyAngles[YAW] = GetLocalAngles()[YAW];

	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];

	// Set the head's yaw.
	float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] );
	desired = clamp( -desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

	// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );

	SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

	// Set the head's yaw.
	desired = AngleNormalize( desiredAngles[PITCH] );
	desired = clamp( desired, m_headPitchMin, m_headPitchMax );

	m_flCurrentHeadPitch = ApproachAngle( -desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
	m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
	SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Adds glow to players and buildings.
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdatePlayerGlows()
{
	if ( !g_PR )
		return;
		
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pLocalPlayer)
		return;

	if ( pLocalPlayer->m_Shared.InCond( TF_COND_TEAM_GLOWS ) ) 
	{

		int nLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
		
		// loop through the players
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			if ( !g_PR->IsConnected( i ) )
				continue;

			C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer || ( pPlayer == pLocalPlayer ) )
				continue;
			
			int nPlayerTeamNumber = pPlayer->GetTeamNumber();

			// remove the entities we don't want to draw anymore
			if ( pPlayer->IsDormant() ||
				( nPlayerTeamNumber < FIRST_GAME_TEAM ) ||
				( !pPlayer->IsAlive() ) ||
				( pPlayer->m_Shared.IsStealthed() && ( nLocalPlayerTeam >= FIRST_GAME_TEAM ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) ) ||
				( !pPlayer->IsPlayerClass( TF_CLASS_SPY ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) ) ||
				( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) ) ||
				( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) && ( pPlayer->m_Shared.GetDisguiseTeam() != nLocalPlayerTeam ) ) )
			{
				if ( pPlayer->IsClientSideGlowEnabled() )
				{
					pPlayer->SetClientSideGlowEnabled( false );
				}
				continue;
			}

			// passed all of the tests, so make sure they're in the list
			int nVecIndex = -1;
			FOR_EACH_VEC( m_vecEntitiesToDraw, nTemp )
			{
				if ( m_vecEntitiesToDraw[nTemp].m_nEntIndex == i )
				{
					nVecIndex = nTemp;
					break;
				}
			}
			if ( nVecIndex == -1 )
			{
				nVecIndex = m_vecEntitiesToDraw.AddToTail();
			}

			// set the player index
			m_vecEntitiesToDraw[nVecIndex].m_nEntIndex = i;

			// disguised Spy?
			C_TFPlayer *pDisguiseTarget = NULL;
			if ( nLocalPlayerTeam >= FIRST_GAME_TEAM )
			{
				if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) )
				{
					pDisguiseTarget = ToTFPlayer( pPlayer->m_Shared.GetDisguiseTarget() );
				}
			}

 			// what color should we use?
			float r, g, b;
			pPlayer->GetGlowEffectColor( &r, &g, &b );
			m_vecEntitiesToDraw[nVecIndex].m_clrGlowColor = Color( r * 255, g * 255, b * 255, 255 );

			if ( !pPlayer->IsClientSideGlowEnabled() )
			{
				pPlayer->SetClientSideGlowEnabled( true );
			}
		}

		// loop through the buildings
		for ( int nCount = 0; nCount < IBaseObjectAutoList::AutoList().Count(); nCount++ )
		{
			bool bDraw = false;
			C_BaseObject *pObject = static_cast<C_BaseObject*>( IBaseObjectAutoList::AutoList()[nCount] );
			if ( !pObject->IsDormant() && !pObject->IsEffectActive( EF_NODRAW ) )
			{
				if ( ( nLocalPlayerTeam >= FIRST_GAME_TEAM ) && ( nLocalPlayerTeam == pObject->GetTeamNumber() ) )
				{
					bDraw = true;
				}
			}

			if ( bDraw )
			{
				int nVecIndex = -1;
				FOR_EACH_VEC( m_vecEntitiesToDraw, nTemp )
				{
					if ( m_vecEntitiesToDraw[nTemp].m_nEntIndex == pObject->entindex() )
					{
						nVecIndex = nTemp;
						break;
					}
				}
				if ( nVecIndex == -1 )
				{
					nVecIndex = m_vecEntitiesToDraw.AddToTail();
				}

				// set the player index
				m_vecEntitiesToDraw[nVecIndex].m_nEntIndex = pObject->entindex();

				if ( pObject->GetType() == OBJ_TELEPORTER )
				{
					m_vecEntitiesToDraw[nVecIndex].m_nOffset = 30;
				}
				else if ( pObject->GetType() == OBJ_DISPENSER )
				{
					m_vecEntitiesToDraw[nVecIndex].m_nOffset = 70;
				}
				else
				{
					switch ( pObject->GetUpgradeLevel() )
					{
					case 1:
						m_vecEntitiesToDraw[nVecIndex].m_nOffset = 50;
						break;
					case 2:
						m_vecEntitiesToDraw[nVecIndex].m_nOffset = 65;
						break;
					case 3:
					default:
						m_vecEntitiesToDraw[nVecIndex].m_nOffset = 80;
						break;
					}
				}

				// what color should we use?
				float r, g, b;
				pObject->GetGlowEffectColor( &r, &g, &b );
				m_vecEntitiesToDraw[nVecIndex].m_clrGlowColor = Color( r * 255, g * 255, b * 255, 255 );

				if ( !pObject->IsClientSideGlowEnabled() )
				{
					pObject->SetClientSideGlowEnabled( true );
				}
			}
			else
			{
				if ( pObject->IsClientSideGlowEnabled() )
				{
					pObject->SetClientSideGlowEnabled( false );
				}
				continue;
			}
		}
	}
	else if (m_vecEntitiesToDraw.Count() > 0 )
	{
		if ( !g_PR )
			return;

		FOR_EACH_VEC( m_vecEntitiesToDraw, i )
		{
			int nEntIndex = m_vecEntitiesToDraw[i].m_nEntIndex;
			if ( IsPlayerIndex( nEntIndex ) && !g_PR->IsConnected( nEntIndex ) )
				continue;

			CBaseCombatCharacter *pEnt = dynamic_cast< CBaseCombatCharacter* >( cl_entitylist->GetEnt( nEntIndex ) );
			if ( !pEnt )
				continue;

			if ( pEnt->IsClientSideGlowEnabled() )
			{
				pEnt->SetClientSideGlowEnabled( false );
			}
		}

		m_vecEntitiesToDraw.Purge();
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: Try to steer away from any players and objects we might interpenetrate
//-----------------------------------------------------------------------------
#define TF_AVOID_MAX_RADIUS_SQR		5184.0f			// Based on player extents and max buildable extents.
#define TF_OO_AVOID_MAX_RADIUS_SQR	0.00019f

ConVar tf_max_separation_force( "tf_max_separation_force", "256", FCVAR_DEVELOPMENTONLY );

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

void C_TFPlayer::AvoidPlayers( CUserCmd *pCmd )
{
	// Turn off the avoid player code.
	if ( !tf_avoidteammates.GetBool() || !tf_avoidteammates_pushaway.GetBool() )
		return;

	// Don't test if the player doesn't exist or is dead.
	if ( IsAlive() == false )
		return;

	C_Team *pTeam = (C_Team *)GetTeam();
	if ( !pTeam )
		return;

	// Up vector.
	static Vector vecUp( 0.0f, 0.0f, 1.0f );

	Vector vecTFPlayerCenter = GetAbsOrigin();
	Vector vecTFPlayerMin = GetPlayerMins();
	Vector vecTFPlayerMax = GetPlayerMaxs();
	float flZHeight = vecTFPlayerMax.z - vecTFPlayerMin.z;
	vecTFPlayerCenter.z += 0.5f * flZHeight;
	VectorAdd( vecTFPlayerMin, vecTFPlayerCenter, vecTFPlayerMin );
	VectorAdd( vecTFPlayerMax, vecTFPlayerCenter, vecTFPlayerMax );

	// Find an intersecting player or object.
	int nAvoidPlayerCount = 0;
	C_TFPlayer *pAvoidPlayerList[MAX_PLAYERS];

	C_TFPlayer *pIntersectPlayer = NULL;
	C_BaseObject *pIntersectObject = NULL;
	float flAvoidRadius = 0.0f;

	Vector vecAvoidCenter, vecAvoidMin, vecAvoidMax;
	for ( int i = 0; i < pTeam->GetNumPlayers(); ++i )
	{
		C_TFPlayer *pAvoidPlayer = static_cast<C_TFPlayer *>( pTeam->GetPlayer( i ) );
		if ( pAvoidPlayer == NULL )
			continue;
		// Is the avoid player me?
		if ( pAvoidPlayer == this )
			continue;

		// Save as list to check against for objects.
		pAvoidPlayerList[nAvoidPlayerCount] = pAvoidPlayer;
		++nAvoidPlayerCount;

		// Check to see if the avoid player is dormant.
		if ( pAvoidPlayer->IsDormant() )
			continue;

		// Is the avoid player solid?
		if ( pAvoidPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
			continue;

		Vector t1, t2;

		vecAvoidCenter = pAvoidPlayer->GetAbsOrigin();
		vecAvoidMin = pAvoidPlayer->GetPlayerMins();
		vecAvoidMax = pAvoidPlayer->GetPlayerMaxs();
		flZHeight = vecAvoidMax.z - vecAvoidMin.z;
		vecAvoidCenter.z += 0.5f * flZHeight;
		VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
		VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

		if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
		{
			// Need to avoid this player.
			if ( !pIntersectPlayer )
			{
				pIntersectPlayer = pAvoidPlayer;
				break;
			}
		}
	}

	// We didn't find a player - look for objects to avoid.
	if ( !pIntersectPlayer )
	{
		for ( int iPlayer = 0; iPlayer < nAvoidPlayerCount; ++iPlayer )
		{
			// Stop when we found an intersecting object.
			if ( pIntersectObject )
				break;

			C_TFTeam *pTeam = (C_TFTeam *)GetTeam();

			for ( int iObject = 0; iObject < pTeam->GetNumObjects(); ++iObject )
			{
				C_BaseObject *pAvoidObject = pTeam->GetObject( iObject );
				if ( !pAvoidObject )
					continue;

				// Check to see if the object is dormant.
				if ( pAvoidObject->IsDormant() )
					continue;

				// Is the object solid.
				if ( pAvoidObject->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
					continue;

				// If we shouldn't avoid it, see if we intersect it.
				if ( pAvoidObject->ShouldPlayersAvoid() )
				{
					vecAvoidCenter = pAvoidObject->WorldSpaceCenter();
					vecAvoidMin = pAvoidObject->WorldAlignMins();
					vecAvoidMax = pAvoidObject->WorldAlignMaxs();
					VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
					VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

					if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
					{
						// Need to avoid this object.
						pIntersectObject = pAvoidObject;
						break;
					}
				}
			}
		}
	}

	// Anything to avoid?
	if ( !pIntersectPlayer && !pIntersectObject )
	{
		m_Shared.SetSeparation( false );
		m_Shared.SetSeparationVelocity( vec3_origin );
		return;
	}

	// Calculate the push strength and direction.
	Vector vecDelta;

	// Avoid a player - they have precedence.
	if ( pIntersectPlayer )
	{
		VectorSubtract( pIntersectPlayer->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectPlayer->WorldAlignMaxs() - pIntersectPlayer->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}
	// Avoid a object.
	else
	{
		VectorSubtract( pIntersectObject->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectObject->WorldAlignMaxs() - pIntersectObject->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}

	float flPushStrength = RemapValClamped( vecDelta.Length(), flAvoidRadius, 0, 0, tf_max_separation_force.GetInt() ); //flPushScale;

	//Msg( "PushScale = %f\n", flPushStrength );

	// Check to see if we have enough push strength to make a difference.
	if ( flPushStrength < 0.01f )
		return;

	Vector vecPush;
	if ( GetAbsVelocity().Length2DSqr() > 0.1f )
	{
		Vector vecVelocity = GetAbsVelocity();
		vecVelocity.z = 0.0f;
		CrossProduct( vecUp, vecVelocity, vecPush );
		VectorNormalize( vecPush );
	}
	else
	{
		// We are not moving, but we're still intersecting.
		QAngle angView = pCmd->viewangles;
		angView.x = 0.0f;
		AngleVectors( angView, NULL, &vecPush, NULL );
	}

	// Move away from the other player/object.
	Vector vecSeparationVelocity;
	if ( vecDelta.Dot( vecPush ) < 0 )
	{
		vecSeparationVelocity = vecPush * flPushStrength;
	}
	else
	{
		vecSeparationVelocity = vecPush * -flPushStrength;
	}

	// Don't allow the max push speed to be greater than the max player speed.
	float flMaxPlayerSpeed = MaxSpeed();
	float flCropFraction = 1.33333333f;

	if ( ( GetFlags() & FL_DUCKING ) && ( GetGroundEntity() != NULL ) )
	{
		flMaxPlayerSpeed *= flCropFraction;
	}

	float flMaxPlayerSpeedSqr = flMaxPlayerSpeed * flMaxPlayerSpeed;

	if ( vecSeparationVelocity.LengthSqr() > flMaxPlayerSpeedSqr )
	{
		vecSeparationVelocity.NormalizeInPlace();
		VectorScale( vecSeparationVelocity, flMaxPlayerSpeed, vecSeparationVelocity );
	}

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;
	Vector currentdir;
	Vector rightdir;

	AngleVectors( vAngles, &currentdir, &rightdir, NULL );

	Vector vDirection = vecSeparationVelocity;

	VectorNormalize( vDirection );

	float fwd = currentdir.Dot( vDirection );
	float rt = rightdir.Dot( vDirection );

	float forward = fwd * flPushStrength;
	float side = rt * flPushStrength;

	//Msg( "fwd: %f - rt: %f - forward: %f - side: %f\n", fwd, rt, forward, side );

	m_Shared.SetSeparation( true );
	m_Shared.SetSeparationVelocity( vecSeparationVelocity );

	pCmd->forwardmove	+= forward;
	pCmd->sidemove		+= side;

	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
	}
	else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
	}

	float flSideScale = 1.0f;
	if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
	{
		flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
	}

	float flScale = Min( flForwardScale, flSideScale );
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "Pforwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInputSampleTime - 
//			*pCmd - 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{
	static QAngle angMoveAngle( 0.0f, 0.0f, 0.0f );

	bool bNoTaunt = true;
	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		// show centerprint message 
		pCmd->forwardmove = 0.0f;
		pCmd->sidemove = 0.0f;
		pCmd->upmove = 0.0f;
		int nOldButtons = pCmd->buttons;
		pCmd->buttons = 0;
		pCmd->weaponselect = 0;

		// Re-add IN_ATTACK2 if player is Demoman with sticky launcher. This is done so they can detonate stickies while taunting.
		if ( ( nOldButtons & IN_ATTACK2 ) && IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			C_TFWeaponBase *pWeapon = Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER );
			if ( pWeapon )
			{
				pCmd->buttons |= IN_ATTACK2;
			}
		}

		VectorCopy( angMoveAngle, pCmd->viewangles );
		bNoTaunt = false;
	}
	else if ( m_Shared.InCond( TF_COND_NO_MOVE ) )
	{
		pCmd->forwardmove = 0.0f;
		pCmd->sidemove = 0.0f;
		VectorCopy( pCmd->viewangles, angMoveAngle );
	}
	else if ( m_Shared.InCond( TF_COND_REDUCED_MOVE ) )
	{
		pCmd->forwardmove = 0.33f;
		pCmd->sidemove = 0.33f;
		VectorCopy( pCmd->viewangles, angMoveAngle );
	}
	else if ( m_Shared.InCond( TF_COND_PHASE ) )
	{
		pCmd->weaponselect = 0;
		int nOldButtons = pCmd->buttons;
		pCmd->buttons = 0;

		// Scout can jump and duck while phased
		if ( ( nOldButtons & IN_JUMP ) )
		{
			pCmd->buttons |= IN_JUMP;
		}

		if ( ( nOldButtons & IN_DUCK ) )
		{
			pCmd->buttons |= IN_DUCK;
		}
	}
	else
	{
		VectorCopy( pCmd->viewangles, angMoveAngle );
	}
	
	// HACK: We're using an unused bit in buttons var to set the typing status based on whether player's chat panel is open.
	if ( GetTFChatHud() && GetTFChatHud()->GetMessageMode() != MM_NONE )
	{
		pCmd->buttons |= IN_TYPING;
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd );

	AvoidPlayers( pCmd );

	return bNoTaunt;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() )
	{
		if ( !prediction->IsFirstTimePredicted() )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent( event, nData );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetObserverCamOrigin( void )
{
	if ( !IsAlive() )
	{
		if ( m_hFirstGib )
		{
			IPhysicsObject *pPhysicsObject = m_hFirstGib->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
				Vector vecWorld;
				m_hFirstGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &vecWorld );
				return ( vecWorld );
			}
			return m_hFirstGib->GetRenderOrigin();
		}

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();
		if ( pRagdoll )
			return pRagdoll->GetRagdollOrigin();
	}

	return BaseClass::GetObserverCamOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Consider the viewer and other factors when determining resulting
// invisibility
//-----------------------------------------------------------------------------
float C_TFPlayer::GetEffectiveInvisibilityLevel( void )
{
	float flPercentInvisible = GetPercentInvisible();

	// If this is a teammate of the local player or viewer is observer,
	// dont go above a certain max invis
	if ( !IsEnemyPlayer() )
	{
		float flMax = tf_teammate_max_invis.GetFloat();
		if ( flPercentInvisible > flMax )
		{
			flPercentInvisible = flMax;
		}
	}
	else
	{
		// If this player just killed me, show them slightly
		// less than full invis in the deathcam and freezecam

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer )
		{
			int iObserverMode = pLocalPlayer->GetObserverMode();

			if ( ( iObserverMode == OBS_MODE_FREEZECAM || iObserverMode == OBS_MODE_DEATHCAM ) &&
				pLocalPlayer->GetObserverTarget() == this )
			{
				float flMax = tf_teammate_max_invis.GetFloat();
				if ( flPercentInvisible > flMax )
				{
					flPercentInvisible = flMax;
				}
			}
		}
	}

	return flPercentInvisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::DrawModel( int flags )
{
	// If we're a dead player with a fresh ragdoll, don't draw
	if ( m_nRenderFX == kRenderFxRagdoll )
		return 0;

	// Don't draw the model at all if we're fully invisible
	if ( GetEffectiveInvisibilityLevel() >= 1.0f )
	{
		if ( m_hPartyHat && ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90 ) && !m_hPartyHat->IsEffectActive( EF_NODRAW ) )
		{
			m_hPartyHat->SetEffects( EF_NODRAW );
		}
		if ( m_hBombHat && ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90 ) && !m_hBombHat->IsEffectActive( EF_NODRAW ) )
		{
			m_hBombHat->SetEffects( EF_NODRAW );
		}
		return 0;
	}
	else
	{
		if ( m_hBombHat && ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90 ) && m_hBombHat->IsEffectActive( EF_NODRAW ) )
		{
			m_hBombHat->RemoveEffects( EF_NODRAW );
		}
		if ( m_hPartyHat && ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90 ) && m_hPartyHat->IsEffectActive( EF_NODRAW ) )
		{
			m_hPartyHat->RemoveEffects( EF_NODRAW );
		}
	}

	CMatRenderContextPtr pRenderContext( materials );
	bool bDoEffect = false;

	float flAmountToChop = 0.0;
	if ( m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		flAmountToChop = ( gpGlobals->curtime - m_flDisguiseEffectStartTime ) *
			( 1.0 / TF_TIME_TO_DISGUISE );
	}
	else
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			float flETime = gpGlobals->curtime - m_flDisguiseEffectStartTime;
			if ( ( flETime > 0.0 ) && ( flETime < TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT ) )
			{
				flAmountToChop = 1.0 - ( flETime * ( 1.0/TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT ) );
			}
		}

	bDoEffect = ( flAmountToChop > 0.0 ) && ( !IsLocalPlayer() );
#if ( SHOW_DISGUISE_EFFECT == 0  )
	bDoEffect = false;
#endif
	bDoEffect = false;
	if ( bDoEffect )
	{
		Vector vMyOrigin =  GetAbsOrigin();
		BoxDeformation_t mybox;
		mybox.m_ClampMins = vMyOrigin - Vector( 100, 100, 100 );
		mybox.m_ClampMaxes = vMyOrigin + Vector( 500, 500, 72 * ( 1 - flAmountToChop ) );
		pRenderContext->PushDeformation( &mybox );
	}

	int ret = BaseClass::DrawModel( flags );

	if ( bDoEffect )
		pRenderContext->PopDeformation();
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ProcessMuzzleFlashEvent()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// Reenable when the weapons have muzzle flash attachments in the right spot.
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( this == pLocalPlayer && !bInToolRecordingMode )
		return; // don't show own world muzzle flash for localplayer

	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		// also don't show in 1st person spec mode
		if ( pLocalPlayer->GetObserverTarget() == this )
			return;
	}

	C_TFWeaponBase *pWeapon = m_Shared.GetActiveTFWeapon();
	if ( !pWeapon )
		return;

	pWeapon->ProcessMuzzleFlashEvent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetForcedIDTarget( int iTarget )
{
	m_iForcedIDTarget = iTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Update this client's targetid entity
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateIDTarget()
{
	if ( !IsLocalPlayer() )
		return;

	// don't show IDs if mp_fadetoblack is on
	if ( GetTeamNumber() > TEAM_SPECTATOR && mp_fadetoblack.GetBool() && !IsAlive() )
	{
		m_iIDEntIndex = 0;
		return;
	}

	if ( m_iForcedIDTarget )
	{
		m_iIDEntIndex = m_iForcedIDTarget;
		return;
	}

	// If we're in deathcam, ID our killer
	if ( ( GetObserverMode() == OBS_MODE_DEATHCAM || GetObserverMode() == OBS_MODE_CHASE ) && GetObserverTarget() && GetObserverTarget() != GetLocalTFPlayer() )
	{
		m_iIDEntIndex = GetObserverTarget()->entindex();
		return;
	}

	// Clear old target and find a new one
	m_iIDEntIndex = 0;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), MAX_TRACE_LENGTH, MainViewForward(), vecEnd );
	VectorMA( MainViewOrigin(), 10, MainViewForward(), vecStart );

	// If we're in observer mode, ignore our observer target. Otherwise, ignore ourselves.
	if ( IsObserver() )
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, GetObserverTarget(), COLLISION_GROUP_NONE, &tr );
	}
	else
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	}

	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;

		if ( pEntity && ( pEntity != this ) )
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display appropriate hints for the target we're looking at
//-----------------------------------------------------------------------------
void C_TFPlayer::DisplaysHintsForTarget( C_BaseEntity *pTarget )
{
	// If the entity provides hints, ask them if they have one for this player
	ITargetIDProvidesHint *pHintInterface = dynamic_cast<ITargetIDProvidesHint *>( pTarget );
	if ( pHintInterface )
	{
		pHintInterface->DisplayHintTo( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetRenderTeamNumber( void )
{
	return m_nSkin;
}

static Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
static Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcDeathCamView( Vector &eyeOrigin, QAngle &eyeAngles, float &fov )
{
	C_BaseEntity *killer = GetObserverTarget();

	// Swing to face our killer within half the death anim time
	float interpolation = ( gpGlobals->curtime - m_flDeathTime ) / ( TF_DEATH_ANIMATION_TIME * 0.5 );
	interpolation = clamp( interpolation, 0.0f, 1.0f );
	interpolation = SimpleSpline( interpolation );

	m_flObserverChaseDistance += gpGlobals->frametime*48.0f;
	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, CHASE_CAM_DISTANCE_MIN, CHASE_CAM_DISTANCE_MAX );

	QAngle aForward = eyeAngles = EyeAngles();
	Vector origin = EyePosition();

	if ( m_hRagdoll )
	{
		C_TFRagdoll *pRagdoll = assert_cast<C_TFRagdoll *>( m_hRagdoll.Get() );
		if ( pRagdoll && pRagdoll->IsPlayingDeathAnim() )
			origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( this ).z * 4;
	}
	else
	{
		IRagdoll *pRagdoll = GetRepresentativeRagdoll();
		if ( pRagdoll )
		{
			origin = pRagdoll->GetRagdollOrigin();
			origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( this ).z; // look over ragdoll, not through
		}
	}

	if ( killer && ( killer != this ) )
	{
		Vector vKiller = killer->EyePosition() - origin;
		QAngle aKiller; VectorAngles( vKiller, aKiller );
		InterpolateAngles( aForward, aKiller, eyeAngles, interpolation );
	};

	Vector vForward; AngleVectors( eyeAngles, &vForward );

	VectorNormalize( vForward );

	VectorMA( origin, -m_flObserverChaseDistance, vForward, eyeOrigin );

	trace_t trace; // clip against world
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if ( trace.fraction < 1.0 )
	{
		eyeOrigin = trace.endpos;
		m_flObserverChaseDistance = VectorLength( origin - eyeOrigin );
	}

	fov = GetFOV();
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

float C_TFPlayer::GetMinFOV() const
{
	// Min FOV for Sniper Rifle
	return 20;
}

int C_TFPlayer::GetVisionFilterFlags( bool bWeaponsCheck )
{
	//if( g_pEngineClientReplay->IsPlayingReplayDemo() )
	//	return tf_replay_pyrovision.GetBool() ? TF_VISION_FILTER_PYRO : TF_VISION_FILTER_NONE;
	
	int nVisionFlags = 0;
	CALL_ATTRIB_HOOK_INT( nVisionFlags, vision_opt_in_flags );

	if ( IsLocalPlayer() )
	{
		if ( bWeaponsCheck )
		{
			if ( m_nForceVisionFilterFlags != TF_VISION_FILTER_NONE )
				return m_nForceVisionFilterFlags;
		}

		if ( GetRenderTeamNumber() == TEAM_SPECTATOR && tf_spectate_pyrovision.GetBool() )
			nVisionFlags |= TF_VISION_FILTER_PYRO;
	}

	if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		nVisionFlags |= TF_VISION_FILTER_HALLOWEEN;

	if ( cl_forced_vision_filter.GetBool() )
		return cl_forced_vision_filter.GetInt();

	return nVisionFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const QAngle &C_TFPlayer::EyeAngles()
{
	if ( IsLocalPlayer() && g_nKillCamMode == OBS_MODE_NONE )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &color - 
//-----------------------------------------------------------------------------
void C_TFPlayer::GetTeamColor( Color &color )
{
	color[3] = 255;

	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			color[0] = 159;
			color[1] = 55;
			color[2] = 34;
			break;
		case TF_TEAM_BLUE:
			color[0] = 76;
			color[1] = 109;
			color[2] = 129;
			break;
		case TF_TEAM_GREEN:
			color[0] = 59;
			color[1] = 120;
			color[2] = 55;
			break;
		case TF_TEAM_YELLOW:
			color[0] = 145;
			color[1] = 145;
			color[2] = 55;
			break;
		default:
			color[0] = 255;
			color[1] = 255;
			color[2] = 255;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bCopyEntity - 
// Output : C_BaseAnimating *
//-----------------------------------------------------------------------------
C_BaseAnimating *C_TFPlayer::BecomeRagdollOnClient()
{
	// Let the C_TFRagdoll take care of this.
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll *C_TFPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_TFRagdoll *pRagdoll = static_cast<C_TFRagdoll *>( m_hRagdoll.Get() );
		if ( !pRagdoll )
			return NULL;

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitPlayerGibs( void )
{
	// Clear out the gib list and create a new one.
	m_aGibs.Purge();
	BuildGibList( m_aGibs, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );

	if ( ( TFGameRules() && TFGameRules()->IsBirthday() ) || UTIL_IsLowViolence() )
	{
		for ( int i = 0; i < m_aGibs.Count(); i++ )
		{
			if ( RandomFloat( 0, 1 ) < 0.75 )
			{
				Q_strncpy( m_aGibs[i].modelName, g_pszBDayGibs[RandomInt( 0, ARRAYSIZE( g_pszBDayGibs )-1 )], sizeof( m_aGibs[i].modelName ) );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			&vecImpactVelocity - 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning, bool bWearables, bool bHeadGib, bool bDisguised )
{
	// Make sure we have Gibs to create.
	if ( m_aGibs.Count() == 0 )
		return;

	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );

	Vector vecBreakVelocity = vecVelocity;
	vecBreakVelocity.z += tf_playergib_forceup.GetFloat();
	VectorNormalize( vecBreakVelocity );
	vecBreakVelocity *= tf_playergib_force.GetFloat();

	// Cap the impulse.
	float flSpeed = vecBreakVelocity.Length();
	if ( flSpeed > tf_playergib_maxspeed.GetFloat() )
	{
		VectorScale( vecBreakVelocity, tf_playergib_maxspeed.GetFloat() / flSpeed, vecBreakVelocity );
	}

	breakablepropparams_t breakParams( vecOrigin, GetRenderAngles(), vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;

	if ( bWearables )
	{
		FOR_EACH_VEC( m_hMyWearables, i )
		{
			C_TFWearable *pTFWearable = dynamic_cast<C_TFWearable *>( m_hMyWearables[i].Get() );
			if ( pTFWearable == nullptr )
				continue;

			if ( pTFWearable->GetDropType() != DROPTYPE_DROP )
				continue;

			/*if ( bDisguised && !pTFWearable->m_bDisguiseWearable ||
				 !bDisguised && pTFWearable->m_bDisguiseWearable )
				continue;*/

			if ( pTFWearable->IsDynamicModelLoading() && pTFWearable->GetModelPtr() == nullptr )
				continue;

			const model_t *pModel = modelinfo->GetModel( pTFWearable->GetModelIndex() );
			const char *szModel = modelinfo->GetModelName( pModel );
			if ( !szModel || !*szModel )
				continue;

			Vector vecOrigin; matrix3x4_t root;
			if ( pTFWearable->IsDynamicModelLoading() || !pTFWearable->GetRootBone( root ) )
				vecOrigin = pTFWearable->GetAbsOrigin();
			else
				MatrixPosition( root, vecOrigin );

			if( IsEntityPositionReasonable( vecOrigin ) )
			{
				CTraceFilterNoNPCsOrPlayer filter( this, COLLISION_GROUP_DEBRIS );
				Vector vecMins, vecMaxs;
				modelinfo->GetModelBounds( pModel, vecMins, vecMaxs );

				trace_t tr;
				UTIL_TraceHull( vecOrigin, vecOrigin, vecMins, vecMaxs, MASK_SOLID, &filter, &tr );
				if ( tr.startsolid )
					continue;

				C_EconWearableGib *pProp = new C_EconWearableGib;
				if ( pProp == nullptr )
					return;

				const CEconItemView *pItem = pTFWearable->GetItem();
				if ( pItem )
					pProp->SetItem( *pItem );

				string_t iModel = AllocPooledString( szModel );
				pProp->SetModelName( iModel );

				pProp->SetAbsOrigin( vecOrigin );
				pProp->SetAbsAngles( pTFWearable->GetAbsAngles() );

				pProp->SetOwnerEntity( this );
				pProp->ChangeTeam( GetTeamNumber() );

				if ( pProp->Initialize( false ) )
				{
					pProp->m_nSkin = pTFWearable->GetSkin();
					pProp->StartFadeOut( 15.0f );

					if ( pProp->VPhysicsGetObject() )
					{
						IPhysicsObject *pPhysics = pProp->VPhysicsGetObject();

						Vector vecDir = vecVelocity.Normalized();
						vecDir.x += RandomFloat( -1.0f, 1.0f );
						vecDir.y += RandomFloat( -1.0f, 1.0f );
						vecDir.z += RandomFloat( 0.0f, 1.0f );

						vecDir.NormalizeInPlace();

						Vector vecNewVelocity = vecVelocity + vecDir * vecVelocity.Normalized() * RandomFloat( -0.25f, 0.25f );
						pPhysics->AddVelocity( &vecNewVelocity, &angularImpulse );
					}
				}
				else
				{
					pProp->Release();
				}
			}
		}
	}
	else
	{
		// Break up the player.
		m_hSpawnedGibs.Purge();

		if ( bHeadGib )
		{
			if ( !UTIL_IsLowViolence() )
			{
				CUtlVector<breakmodel_t> list;
				const int iClassIdx = GetPlayerClass()->GetClassIndex();
				FOR_EACH_VEC( m_aGibs, i )
				{
					breakmodel_t const &breakModel = m_aGibs[i];
					if ( !V_stricmp( breakModel.modelName, g_pszHeadGibs[ iClassIdx ] ) )
						list.AddToHead( breakModel );
				}

				m_hFirstGib = CreateGibsFromList( list, GetModelIndex(), NULL, breakParams, this, -1, false, true, &m_hSpawnedGibs, bBurning );
				if ( m_hFirstGib )
				{
					Vector velocity, impulse;
					IPhysicsObject *pPhys = m_hFirstGib->VPhysicsGetObject();
					if ( pPhys )
					{
						pPhys->GetVelocity( &velocity, &impulse );
						impulse.x *= 6.0f;
						pPhys->AddVelocity( &velocity, &impulse );
					}
				}
			}
		}
		else
		{
			CUtlVector<breakmodel_t> list;
			list.AddVectorToTail( m_aGibs );
			if ( UTIL_IsLowViolence() )
			{
				if ( Q_strcmp( m_aGibs[0].modelName, g_pszBDayGibs[4]) != 0 )
				{
					for( int i=0; i<list.Count(); ++i )
					{
						V_strncpy( list[i].modelName, g_pszBDayGibs[ RandomInt( 4, ARRAYSIZE( g_pszBDayGibs )-1 ) ], sizeof list[i].modelName );
					}
				}
			}

			m_hFirstGib = CreateGibsFromList( m_aGibs, GetModelIndex(), NULL, breakParams, this, -1, false, true, &m_hSpawnedGibs, bBurning );
		}
	}

	DropPartyHat( breakParams, vecBreakVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::DropPartyHat( breakablepropparams_t const &breakParams, Vector const &vecBreakVelocity )
{
	if ( m_hPartyHat )
	{
		breakmodel_t breakModel;
		Q_strncpy( breakModel.modelName, BDAY_HAT_MODEL, sizeof( breakModel.modelName ) );
		breakModel.health = 1;
		breakModel.fadeTime = RandomFloat( 5, 10 );
		breakModel.fadeMinDist = 0.0f;
		breakModel.fadeMaxDist = 0.0f;
		breakModel.burstScale = breakParams.defBurstScale;
		breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
		breakModel.isRagdoll = false;
		breakModel.isMotionDisabled = false;
		breakModel.placementName[0] = 0;
		breakModel.placementIsBone = false;
		breakModel.offset = GetAbsOrigin() - m_hPartyHat->GetAbsOrigin();
		BreakModelCreateSingle( this, &breakModel, m_hPartyHat->GetAbsOrigin(), m_hPartyHat->GetAbsAngles(), vecBreakVelocity, breakParams.angularVelocity, m_hPartyHat->m_nSkin, breakParams );

		m_hPartyHat->Release();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::DropHat( breakablepropparams_t const &breakParams, Vector const &vecBreakVelocity )
{
	FOR_EACH_VEC( m_hMyWearables, i )
	{
		CEconWearable* pItem = m_hMyWearables[i];
		if ( pItem && pItem->GetItem()->GetStaticData() )
		{
			if ( pItem->GetDropType() > DROPTYPE_NONE )
			{
				breakmodel_t breakModel;
				V_strcpy_safe( breakModel.modelName, STRING( pItem->GetModelName() ) );

				breakModel.health = 1;
				breakModel.fadeTime = RandomFloat( 5, 10 );
				breakModel.fadeMinDist = 0.0f;
				breakModel.fadeMaxDist = 0.0f;
				breakModel.burstScale = breakParams.defBurstScale;
				breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
				breakModel.isRagdoll = false;
				breakModel.isMotionDisabled = false;
				breakModel.placementName[0] = 0;
				breakModel.placementIsBone = false;
				breakModel.offset = GetAbsOrigin() - pItem->GetAbsOrigin();
				BreakModelCreateSingle( this, &breakModel, pItem->GetAbsOrigin(), pItem->GetAbsAngles(), vecBreakVelocity, breakParams.angularVelocity, pItem->m_nSkin, breakParams );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: How many buildables does this player own
//-----------------------------------------------------------------------------
int	C_TFPlayer::GetObjectCount( void )
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObject( int index )
{
	return m_aObjects[index].Get();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObjectOfType( int iObjectType, int iObjectMode )
{
	int iCount = m_aObjects.Count();

	for ( int i=0; i<iCount; i++ )
	{
		C_BaseObject *pObj = m_aObjects[i].Get();

		if ( !pObj )
			continue;

		if ( pObj->IsDormant() || pObj->IsMarkedForDeletion() )
			continue;

		if ( pObj->GetType() == iObjectType && pObj->GetObjectMode() == iObjectMode )
		{
			return pObj;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( ( ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ) && tf_avoidteammates.GetBool() ) ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS )
	{
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				if ( !( contentsMask & CONTENTS_REDTEAM ) )
					return false;
				break;

			case TF_TEAM_BLUE:
				if ( !( contentsMask & CONTENTS_BLUETEAM ) )
					return false;
				break;
				
			case TF_TEAM_GREEN:
				if ( !(contentsMask & CONTENTS_GREENTEAM ) )
					return false;
				break;

			case TF_TEAM_YELLOW:
				if ( !(contentsMask & CONTENTS_YELLOWTEAM ) )
					return false;
				break;
		}
	}
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

float C_TFPlayer::GetPercentInvisible( void )
{
	return m_Shared.GetPercentInvisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetSkin()
{
	C_TFPlayer *pLocalPlayer = GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return 0;

	int iVisibleTeam = GetTeamNumber();

	// if this player is disguised and on the other team, use disguise team
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
	{
		iVisibleTeam = m_Shared.GetDisguiseTeam();
	}

	int nSkin;

	switch (iVisibleTeam)
	{
		case TF_TEAM_RED:
			nSkin = 0;
			break;

		case TF_TEAM_BLUE:
			nSkin = 1;
			break;
			
		case TF_TEAM_GREEN:
			nSkin = 8;
			break;

		case TF_TEAM_YELLOW:
			nSkin = 9;
			break;

		default:
			nSkin = 0;
			break;
	}

	if ( TFGameRules()->IsHolidayActive(kHoliday_Halloween) )
	{
		nSkin += 4;
		if ( IsPlayerClass(TF_CLASS_SPY) )
			nSkin += 18;
	}	

	// 3, 4, 8, 9 are invulnerable
	if (m_Shared.InCond(TF_COND_INVULNERABLE))
	{
		nSkin += 2;
	}
	// This part shouldn't matter anymore.
	/*
	else if (m_Shared.InCond(TF_COND_DISGUISED))
	{
		if (!IsEnemyPlayer())
		{
			if (TFGameRules()->IsHolidayActive( kHoliday_Halloween ))
			{
				if (iVisibleTeam == 2)
				{
					nSkin = 0;
				}
				else if (iVisibleTeam == 3)
				{
					nSkin = 1;
				}


				// Show team members what this player is disguised as
				nSkin += 4 + ((m_Shared.GetDisguiseClass() - TF_FIRST_NORMAL_CLASS) * 2);
			}
			else
			{
				// Show team members what this player is disguised as
				nSkin += 4 + ((m_Shared.GetDisguiseClass() - TF_FIRST_NORMAL_CLASS) * 2);
			}



		}
		else if (m_Shared.GetDisguiseClass() == TF_CLASS_SPY)
		{
			if (TFGameRules()->IsHolidayActive( kHoliday_Halloween ))
			{
				if (iVisibleTeam == 2)
				{
					nSkin = 0;
				}
				else if (iVisibleTeam == 3)
				{
					nSkin = 1;
				}

				// This player is disguised as an enemy spy so enemies should see a fake mask
				nSkin += 4 + ((m_Shared.GetMaskClass() - TF_FIRST_NORMAL_CLASS) * 2);
			}
			else
			{
				// This player is disguised as an enemy spy so enemies should see a fake mask
				nSkin += 4 + ((m_Shared.GetMaskClass() - TF_FIRST_NORMAL_CLASS) * 2);
			}
		}
	}
	*/

	return nSkin;
}

//-----------------------------------------------------------------------------
// Purpose: Determines if we show the original spy body or the disguised body.
//-----------------------------------------------------------------------------
int C_TFPlayer::GetBody(void)
{
	// Enemies disguised use a different set of bodygroups.
	if ( ShouldDrawSpyAsDisguised() )
		return m_Shared.GetDisguiseBody();

	return m_nBody;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iClass - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsPlayerClass( int iClass )
{
	C_TFPlayerClass *pClass = GetPlayerClass();
	if ( !pClass )
		return false;

	return ( pClass->GetClassIndex() == iClass );
}

//-----------------------------------------------------------------------------
// Purpose: Don't take damage decals while stealthed
//-----------------------------------------------------------------------------
void C_TFPlayer::AddDecal( const Vector &rayStart, const Vector &rayEnd,
	const Vector &decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t &tr, int maxLODToDecal )
{
	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		return;
	}

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		return;
	}

	if ( m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		Vector vecDir = rayEnd - rayStart;
		VectorNormalize( vecDir );
		g_pEffects->Ricochet( rayEnd - ( vecDir * 8 ), -vecDir );
		return;
	}

	// don't decal from inside the player
	if ( tr.startsolid )
	{
		return;
	}

	BaseClass::AddDecal( rayStart, rayEnd, decalCenter, hitbox, decalIndex, doTrace, tr, maxLODToDecal );
}

//-----------------------------------------------------------------------------
// Called every time the player respawns
//-----------------------------------------------------------------------------
void C_TFPlayer::ClientPlayerRespawn( void )
{
	if ( IsLocalPlayer() )
	{
		// Dod called these, not sure why
		//MoveToLastReceivedPosition( true );
		//ResetLatched();

		// Reset the camera.
		m_bWasTaunting = false;
		HandleTaunting();

		IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_respawn" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}

		ResetToneMapping( 1.0 );

		// Release the duck toggle key
		KeyUp( &in_ducktoggle, NULL );

		LoadInventory();
	}

	// Reset attachments
	DestroyBoneAttachments();

	UpdateVisibility();

	// Update min. viewmodel
	CalcMinViewmodelOffset();

	// Reset rage
	m_Shared.ResetRageSystem();

	m_flCreateBombHeadAt = -1.0f;

	m_hFirstGib = NULL;
	m_hSpawnedGibs.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreateSaveMeEffect( void )
{
	// Don't create them for the local player
	if ( !ShouldDrawThisPlayer() )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Only show the bubble to teammates and players who are on the our disguise team.
	if ( GetTeamNumber() != pLocalPlayer->GetTeamNumber() && !( m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseTeam() == pLocalPlayer->GetTeamNumber() ) )
		return;

	if ( m_pSaveMeEffect )
	{
		ParticleProp()->StopEmission( m_pSaveMeEffect );
		m_pSaveMeEffect = NULL;
	}

	m_pSaveMeEffect = ParticleProp()->Create( "speech_mediccall", PATTACH_POINT_FOLLOW, "head" );

	if ( m_pSaveMeEffect )
	{
		// Set "redness" of the bubble based on player's health.
		float flHealthRatio = clamp( (float)GetHealth() / (float)GetMaxHealth(), 0.0f, 1.0f );
		m_pSaveMeEffect->SetControlPoint( 1, Vector( flHealthRatio ) );
	}

	// If the local player is a medic, add this player to our list of medic callers
	if ( pLocalPlayer && pLocalPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pLocalPlayer->IsAlive() == true )
	{
		Vector vecPos;
		if ( GetAttachmentLocal( LookupAttachment( "head" ), vecPos ) )
		{
			vecPos += Vector( 0, 0, 18 );	// Particle effect is 18 units above the attachment
			CTFMedicCallerPanel::AddMedicCaller( this, 5.0, vecPos );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsOverridingViewmodel( void )
{
	C_TFPlayer *pPlayer = this;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE &&
		pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = assert_cast<C_TFPlayer *>( pLocalPlayer->GetObserverTarget() );
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
		return true;

	return BaseClass::IsOverridingViewmodel();
}

//-----------------------------------------------------------------------------
// Purpose: Draw my viewmodel in some special way
//-----------------------------------------------------------------------------
int	C_TFPlayer::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int ret = 0;

	C_TFPlayer *pPlayer = this;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE &&
		pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = assert_cast<C_TFPlayer *>( pLocalPlayer->GetObserverTarget() );
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		// Force the invulnerable material
		modelrender->ForcedMaterialOverride( pPlayer->GetInvulnMaterial() );
		ret = pViewmodel->DrawOverriddenViewmodel( flags );
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetHealer( C_TFPlayer *pHealer, float flChargeLevel )
{
	// We may be getting healed by multiple healers. Show the healer
	// who's got the highest charge level.
	if ( m_hHealer )
	{
		if ( m_flHealerChargeLevel > flChargeLevel )
			return;
	}

	m_hHealer = pHealer;
	m_flHealerChargeLevel = flChargeLevel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CanShowClassMenu( void )
{
	return ( GetTeamNumber() > LAST_SHARED_TEAM );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitializePoseParams( void )
{
	/*
	m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );
	*/

	CStudioHdr *hdr = GetModelPtr();
	Assert( hdr );
	if ( !hdr )
		return;

	for ( int i = 0; i < hdr->GetNumPoseParameters(); i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetChaseCamViewOffset( CBaseEntity *target )
{
	if ( target->IsBaseObject() )
		return Vector( 0, 0, 64 );

	return BaseClass::GetChaseCamViewOffset( target );
}

//-----------------------------------------------------------------------------
// Purpose: Called from PostDataUpdate to update the model index
//-----------------------------------------------------------------------------
void C_TFPlayer::ValidateModelIndex( void )
{
	if ( m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) && IsEnemyPlayer() && GetGroundEntity() && IsDucked() )
	{
		m_nModelIndex = modelinfo->GetModelIndex( "models/buildables/dispenser_light.mdl" );

		if ( GetLocalPlayer() != this )
			SetAbsAngles( vec3_angle );
	}
	else if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( m_Shared.GetDisguiseClass() );
		m_nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );

		for (int i = 0; i < GetNumWearables(); i++)
		{
			C_TFWearable *pWearable = static_cast<C_TFWearable *>(GetWearable(i));

			if (!pWearable)
				continue;


			pWearable->UpdateModelToClass();

		}
	}
	else
	{
		C_TFPlayerClass *pClass = GetPlayerClass();
		if ( pClass )
		{
			m_nModelIndex = modelinfo->GetModelIndex( pClass->GetModelName() );
		}

		for (int i = 0; i < GetNumWearables(); i++)
		{
			C_TFWearable *pWearable = static_cast<C_TFWearable *>(GetWearable(i));

			if (!pWearable)
				continue;


			pWearable->UpdateModelToClass();

		}
	}
	
	// Don't need to set bodygroup anymore.
	/*
	if ( m_iSpyMaskBodygroup > -1 && GetModelPtr() != NULL )
	{
		SetBodygroup( m_iSpyMaskBodygroup, ( m_Shared.InCond( TF_COND_DISGUISED ) && ( !IsEnemyPlayer() || m_Shared.GetDisguiseClass() == TF_CLASS_SPY ) ) );
	}
	*/
	BaseClass::ValidateModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
void C_TFPlayer::Simulate( void )
{
	//Frame updates
	if ( IsLocalPlayer() )
	{
		//Update the flashlight
		Flashlight();
	}

	// TF doesn't do step sounds based on velocity, instead using anim events
	// So we deliberately skip over the base player simulate, which calls them.
	BaseClass::BaseClass::Simulate();
}

void C_TFPlayer::LoadInventory( void )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		for ( int iSlot = 0; iSlot <= TF_LOADOUT_SLOT_MISC3; iSlot++ )
		{
			int iPreset = GetTFInventory()->GetWeaponPreset( iClass, iSlot );
			char szCmd[64];
			Q_snprintf( szCmd, sizeof( szCmd ), "weaponpresetclass %d %d %d;", iClass, iSlot, iPreset );
			engine->ExecuteClientCmd( szCmd );
		}
	}
}

void C_TFPlayer::EditInventory( int iSlot, int iWeapon )
{
	int iClass = GetPlayerClass()->GetClassIndex();
	GetTFInventory()->SetWeaponPreset( iClass, iSlot, iWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::FireEvent( const Vector &origin, const QAngle &angles, int event, const char *options )
{
	if ( event == 7001 )
	{
		// Force a footstep sound
		m_flStepSoundTime = 0;
		Vector vel;
		EstimateAbsVelocity( vel );
		UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
	}
	else if ( event == AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN )
	{
		C_TFWeaponBase *pTFWeapon = GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			C_BaseAnimating *vm = pTFWeapon->GetAppropriateWorldOrViewModel();
			if ( vm )
			{
				vm->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
			}
		}
	}
	else if ( event == AE_WPN_HIDE )
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( false );
		}
	}
	else if ( event == AE_WPN_UNHIDE )
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( true );
		}
	}
	else if ( event == AE_WPN_PLAYWPNSOUND )
	{
		if ( GetActiveWeapon() )
		{
			WeaponSound_t nWeaponSound = EMPTY;
			GetWeaponSoundFromString( options );

			if ( nWeaponSound != EMPTY )
			{
				GetActiveWeapon()->WeaponSound( nWeaponSound );
			}
		}
	}
	else if ( event == TF_AE_CIGARETTE_THROW )
	{
		CEffectData data;
		int iAttach = LookupAttachment( options );
		GetAttachment( iAttach, data.m_vOrigin, data.m_vAngles );

		data.m_vAngles = GetRenderAngles();

		data.m_hEntity = ClientEntityList().EntIndexToHandle( entindex() );
		DispatchEffect( "TF_ThrowCigarette", data );
		return;
	}
	else
		BaseClass::FireEvent( origin, angles, event, options );
}

// Shadows

ConVar cl_blobbyshadows( "cl_blobbyshadows", "0", FCVAR_CLIENTDLL );
extern ConVar tf2v_disable_player_shadows;
ShadowType_t C_TFPlayer::ShadowCastType( void )
{
	if ( tf2v_disable_player_shadows.GetBool() )
		return SHADOWS_NONE;

	// Removed the GetPercentInvisible - should be taken care off in BindProxy now.
	if ( !IsVisible() /*|| GetPercentInvisible() > 0.0f*/ )
		return SHADOWS_NONE;

	if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) )
		return SHADOWS_NONE;

	// If in ragdoll mode.
	if ( m_nRenderFX == kRenderFxRagdoll )
		return SHADOWS_NONE;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// if we're first person spectating this player
	if ( pLocalPlayer &&
		pLocalPlayer->GetObserverTarget() == this &&
		pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		return SHADOWS_NONE;
	}

	if ( cl_blobbyshadows.GetBool() )
		return SHADOWS_SIMPLE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

float g_flFattenAmt = 4;
void C_TFPlayer::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Don't let the render bounds change when we're using blobby shadows, or else the shadow
		// will pop and stretch.
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();
	}
	else
	{
		GetRenderBounds( mins, maxs );

		// We do this because the normal bbox calculations don't take pose params into account, and 
		// the rotation of the guy's upper torso can place his gun a ways out of his bbox, and 
		// the shadow will get cut off as he rotates.
		//
		// Thus, we give it some padding here.
		mins -= Vector( g_flFattenAmt, g_flFattenAmt, 0 );
		maxs += Vector( g_flFattenAmt, g_flFattenAmt, 0 );
	}
}


void C_TFPlayer::GetRenderBounds( Vector &theMins, Vector &theMaxs )
{
	// TODO POSTSHIP - this hack/fix goes hand-in-hand with a fix in CalcSequenceBoundingBoxes in utils/studiomdl/simplify.cpp.
	// When we enable the fix in CalcSequenceBoundingBoxes, we can get rid of this.
	//
	// What we're doing right here is making sure it only uses the bbox for our lower-body sequences since,
	// with the current animations and the bug in CalcSequenceBoundingBoxes, are WAY bigger than they need to be.
	C_BaseAnimating::GetRenderBounds( theMins, theMaxs );
}


bool C_TFPlayer::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Blobby shadows should sit directly underneath us.
		pDirection->Init( 0, 0, -1 );
		return true;
	}
	else
	{
		return BaseClass::GetShadowCastDirection( pDirection, shadowType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is the nemesis of the local player
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsNemesisOfLocalPlayer()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		// return whether this player is dominating the local player
		return m_Shared.IsPlayerDominated( pLocalPlayer->entindex() );
	}
	return false;
}

extern ConVar tf_tournament_hide_domination_icons;
//-----------------------------------------------------------------------------
// Purpose: Returns whether we should show the nemesis icon for this player
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldShowNemesisIcon()
{
	// we should show the nemesis effect on this player if he is the nemesis of the local player,
	// and is not dead, cloaked or disguised
	if ( IsNemesisOfLocalPlayer() && g_PR && g_PR->IsConnected( entindex() ) )
	{
		bool bStealthed = m_Shared.InCond( TF_COND_STEALTHED );
		bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
		bool bTournamentHide = TFGameRules()->IsInTournamentMode() && tf_tournament_hide_domination_icons.GetBool();
		if ( IsAlive() && !bStealthed && !bDisguised && !bTournamentHide )
			return true;
	}
	return false;
}

bool C_TFPlayer::IsWeaponLowered( void )
{
	C_TFWeaponBase *pWeapon = GetActiveTFWeapon();

	if ( !pWeapon )
		return false;

	C_TFGameRules *pRules = TFGameRules();

	// Lower losing team's weapons in bonus round
	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
		return true;

	// Hide all view models after the game is over
	if ( pRules->State_Get() == GR_STATE_GAME_OVER )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
		case CChoreoEvent::SEQUENCE:
		case CChoreoEvent::GESTURE:
			return StartGestureSceneEvent( info, scene, event, actor, pTarget );
		default:
			return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget )
{
	// Get the (gesture) sequence.
	info->m_nSequence = LookupSequence( event->GetParameters() );
	if ( info->m_nSequence < 0 )
		return false;

	// Player the (gesture) sequence.
	m_PlayerAnimState->AddVCDSequenceToGestureSlot( GESTURE_SLOT_VCD, info->m_nSequence );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetNumActivePipebombs( void )
{
	if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		C_TFPipebombLauncher *pWeapon = dynamic_cast <C_TFPipebombLauncher *>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );

		if ( pWeapon )
		{
			return pWeapon->GetPipeBombCount();
		}
	}

	return 0;
}

bool C_TFPlayer::IsAllowedToSwitchWeapons( void )
{
	if ( IsWeaponLowered() == true )
		return false;

	if ( m_Shared.InCond( TF_COND_STUNNED ) )
		return false;

	return BaseClass::IsAllowedToSwitchWeapons();
}

IMaterial *C_TFPlayer::GetHeadLabelMaterial( void )
{
	if ( g_pHeadLabelMaterial[0] == NULL )
		SetupHeadLabelMaterials();

	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_RED];
			break;

		case TF_TEAM_BLUE:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_BLUE];
			break;
			
		case TF_TEAM_GREEN:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_GREEN];
			break;

		case TF_TEAM_YELLOW:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_YELLOW];
			break;
			
	}

	return BaseClass::GetHeadLabelMaterial();
}

void SetupHeadLabelMaterials( void )
{
	for ( int i = 0; i < ( TF_TEAM_COUNT - 2 ); i++ )
	{
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->DecrementReferenceCount();
			g_pHeadLabelMaterial[i] = NULL;
		}

		g_pHeadLabelMaterial[i] = materials->FindMaterial( pszHeadLabelNames[i], TEXTURE_GROUP_VGUI );
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->IncrementReferenceCount();
		}
	}
}

void C_TFPlayer::ComputeFxBlend( void )
{
	BaseClass::ComputeFxBlend();

	if ( GetPlayerClass()->IsClass( TF_CLASS_SPY ) )
	{
		float flInvisible = GetPercentInvisible();
		if ( flInvisible != 0.0f )
		{
			// Tell our shadow
			ClientShadowHandle_t hShadow = GetShadowHandle();
			if ( hShadow != CLIENTSHADOW_INVALID_HANDLE )
			{
				g_pClientShadowMgr->SetFalloffBias( hShadow, flInvisible * 255 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	HandleTaunting();

	if ( m_lifeState != LIFE_ALIVE && m_hRagdoll.Get() )
	{
		// First person ragdolls
		if ( cl_fp_ragdoll.GetBool() && GetObserverMode() == OBS_MODE_DEATHCAM )
		{
			C_TFRagdoll *pRagdoll = assert_cast<C_TFRagdoll *>( m_hRagdoll.Get() );

			int iAttachment = pRagdoll->LookupAttachment( "eyes" );
			if ( iAttachment >= 0 )
			{
				pRagdoll->GetAttachment( iAttachment, eyeOrigin, eyeAngles );

				Vector vForward;
				AngleVectors( eyeAngles, &vForward );

				if ( cl_fp_ragdoll_auto.GetBool() )
				{
					// DM: Don't use first person view when we are very close to something
					trace_t tr;
					UTIL_TraceLine( eyeOrigin, eyeOrigin + ( vForward * 10000 ), MASK_ALL, pRagdoll, COLLISION_GROUP_NONE, &tr );

					if ( ( !( tr.fraction < 1 ) || ( tr.endpos.DistTo( eyeOrigin ) > 25 ) ) )
						return;
				}
				else
					return;
			}
		}
	}

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

static void cc_tf_crashclient()
{
	C_TFPlayer *pPlayer = NULL;
	pPlayer->ComputeFxBlend();
}
static ConCommand tf_crashclient( "tf_crashclient", cc_tf_crashclient, "Crashes this client for testing.", FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::ForceUpdateObjectHudState( void )
{
	m_bUpdateObjectHudState = true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the weapon passed in would occupy a slot already occupied by the carrier
// Input  : *pWeapon - weapon to test for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon == NULL )
		return false;

	//Check to see if there's a resident weapon already in this slot
	if ( Weapon_GetSlot( pWeapon->GetSlot() ) == NULL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon (if any) in the requested slot
// Input  : slot - which slot to poll
//-----------------------------------------------------------------------------
CBaseCombatWeapon *C_TFPlayer::Weapon_GetSlot( int slot ) const
{
	int	targetSlot = slot;

	// Check for that slot being occupied already
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( GetWeapon( i ) != NULL )
		{
			// If the slots match, it's already occupied
			if ( GetWeapon( i )->GetSlot() == targetSlot )
				return GetWeapon( i );
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::CreateBombinomiconHint( void )
{
	if ( !IsLocalPlayer() )
		return;

	Vector vecOffset( -40.0, 0, 120.0 ); QAngle angOffset( 30.0, 0, 0 );
	m_hBombinomicon = C_MerasmusBombEffect::Create( "models/props_halloween/bombonomicon.mdl", this, vecOffset, angOffset, 100.0f, 4.0f, PAM_SPIN_Z );

	m_flCreateBombHeadAt = gpGlobals->curtime + 2.0;

	CSoundParameters parms;
	if ( !GetParametersForSound( "Halloween.BombinomiconSpin", parms, "" ) )
		return;

	CSingleUserRecipientFilter filter( this );
	EmitSound_t emit( parms );
	EmitSound( filter, m_hBombinomicon->entindex(), emit );
}

void C_TFPlayer::DestroyBombinomiconHint( void )
{
	if ( m_hBombinomicon )
		m_hBombinomicon->Release();
}

void C_TFPlayer::UpdateHalloweenBombHead( void )
{
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
	{
		if( m_hBombHat != nullptr )
			return;

		if ( m_flCreateBombHeadAt > gpGlobals->curtime )
			return;

		int iHead = LookupAttachment( "head" );
		m_hBombHat = C_PlayerAttachedModel::Create( "models/props_lakeside_event/bomb_temp_hat.mdl", this, iHead, vec3_origin, PAM_PERMANENT );
	}
	else
	{
		if ( m_hBombHat )
			m_hBombHat->Release();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateOverhealEffect( bool bForceHide /*= false*/ )
{
	bool bShouldShow = !bForceHide;
	int iTeamNum = GetTeamNumber();

	if ( bShouldShow )
	{
		if ( !IsAlive() || InFirstPersonView() )
		{
			bShouldShow = false;
		}
		else if ( IsEnemyPlayer() )
		{
			if ( m_Shared.InCond( TF_COND_STEALTHED ) )
			{
				// Don't give away cloaked spies.
				bShouldShow = false;
			}
			else if ( m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				// Disguised spies should use their fake health instead.
				if ( m_Shared.InCond( TF_COND_DISGUISE_HEALTH_OVERHEALED ) )
				{
					// Use the disguise team.
					iTeamNum = m_Shared.GetDisguiseTeam();
				}
				else
				{
					bShouldShow = false;
				}
			}
			else if ( !m_Shared.InCond( TF_COND_HEALTH_OVERHEALED ) )
			{
				bShouldShow = false;
			}
		}
		else if ( !m_Shared.InCond( TF_COND_HEALTH_OVERHEALED ) )
		{
			bShouldShow = false;
		}
	}

	if ( bShouldShow )
	{
		// If we've undisguised recently change the overheal particle color
		if ( m_pOverhealEffect && iTeamNum != m_iOldOverhealTeamNum )
		{
			ParticleProp()->StopEmission( m_pOverhealEffect );
			m_pOverhealEffect = NULL;
		}

		if ( !m_pOverhealEffect )
		{
			const char *pszEffect = ConstructTeamParticle( "overhealedplayer_%s_pluses", iTeamNum, false );
			m_pOverhealEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
		}
	}
	else
	{
		if ( m_pOverhealEffect )
		{
			ParticleProp()->StopEmission( m_pOverhealEffect );
			m_pOverhealEffect = NULL;
		}
	}

	// Update our overheal state
	m_iOldOverhealTeamNum = iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateDemomanEyeEffect( int iDecapCount )
{
	if ( m_pDemoEyeEffect )
	{
		ParticleProp()->StopEmission( m_pDemoEyeEffect );
		m_pDemoEyeEffect = NULL;
	}

	iDecapCount = Min( iDecapCount, 4 );
	switch ( iDecapCount )
	{
		case 1:
			m_pDemoEyeEffect = ParticleProp()->Create( "eye_powerup_green_lvl_1", PATTACH_POINT_FOLLOW, "eyeglow_L" );
			break;
		case 2:
			m_pDemoEyeEffect = ParticleProp()->Create( "eye_powerup_green_lvl_2", PATTACH_POINT_FOLLOW, "eyeglow_L" );
			break;
		case 3:
			m_pDemoEyeEffect = ParticleProp()->Create( "eye_powerup_green_lvl_3", PATTACH_POINT_FOLLOW, "eyeglow_L" );
			break;
		case 4:
			m_pDemoEyeEffect = ParticleProp()->Create( "eye_powerup_green_lvl_4", PATTACH_POINT_FOLLOW, "eyeglow_L" );
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateTypingBubble( void )
{
	// If they're disabled, don't even bother with the rest of the function.
	if ( !tf2v_showchatbubbles.GetBool() || !GetTFChatHud() )
		return;
		
	// Don't show the bubble for local player.
	if ( InFirstPersonView() )
		return;

	// Check if it's all or team chat.
	if ( GetTFChatHud()->GetMessageMode() == MM_SAY )
	{
		// for all chat, show it over anyone's head.
		if ( m_bTyping && IsAlive() && !m_Shared.IsStealthed() )
		{
			if ( !m_pTypingEffect )
				m_pTypingEffect = ParticleProp()->Create( "speech_typing", PATTACH_POINT_FOLLOW, "head" );
		}
		else 
		{
			if ( m_pTypingEffect )
			{
				ParticleProp()->StopEmissionAndDestroyImmediately( m_pTypingEffect );
				m_pTypingEffect = NULL;
			}
		}
	}
	else
	{
		// for team chat, show over teammates only.
		if ( m_bTyping && IsAlive() && ( !m_Shared.IsStealthed() || !IsEnemyPlayer() ) )
		{
			if ( !m_pTypingEffect )
				m_pTypingEffect = ParticleProp()->Create( "speech_typing", PATTACH_POINT_FOLLOW, "head" );
		}
		else 
		{
			if ( m_pTypingEffect )
			{
				ParticleProp()->StopEmissionAndDestroyImmediately( m_pTypingEffect );
				m_pTypingEffect = NULL;
			}
		}
	}
}



#include "c_obj_sentrygun.h"


static void cc_tf_debugsentrydmg()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	pPlayer->UpdateIDTarget();
	int iTarget = pPlayer->GetIDTarget();
	if ( iTarget > 0 )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( iTarget );

		C_ObjectSentrygun *pSentry = dynamic_cast<C_ObjectSentrygun *>( pEnt );

		if ( pSentry )
		{
			pSentry->DebugDamageParticles();
		}
	}
}
static ConCommand tf_debugsentrydamage( "tf_debugsentrydamage", cc_tf_debugsentrydmg, "", FCVAR_DEVELOPMENTONLY );

#include "clientsteamcontext.h"

const char *pszTF2VHideousEasterEgg[] =
{
	"https://wiki.teamfortress.com/wiki/Well-Rounded_Rifleman",
	"https://wiki.teamfortress.com/wiki/Graybanns",
	"https://wiki.teamfortress.com/wiki/HDMI_Patch",
	"https://wiki.teamfortress.com/wiki/Soldered_Sensei",
	"https://wiki.teamfortress.com/wiki/Soldier%27s_Sparkplug",
	"https://wiki.teamfortress.com/wiki/Steel_Shako",
	"https://wiki.teamfortress.com/wiki/Timeless_Topper",
	"https://wiki.teamfortress.com/wiki/Western_Wear",
};

static void cc_tf2v_hideous()
{
	if ( steamapicontext && steamapicontext->SteamFriends() )
	{
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( pszTF2VHideousEasterEgg[RandomInt(0,7)]);
	}
}
static ConCommand hideous( "hideous", cc_tf2v_hideous, "", FCVAR_NONE );
