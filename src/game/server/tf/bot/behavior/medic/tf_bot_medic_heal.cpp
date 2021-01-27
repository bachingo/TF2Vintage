#include "cbase.h"
#include "../../tf_bot.h"
#include "tf_bot_medic_heal.h"
#include "tf_bot_medic_retreat.h"
#include "../tf_bot_use_teleporter.h"
#include "tf_obj_teleporter.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"


ConVar tf_bot_medic_stop_follow_range( "tf_bot_medic_stop_follow_range", "75", FCVAR_CHEAT );
ConVar tf_bot_medic_start_follow_range( "tf_bot_medic_start_follow_range", "250", FCVAR_CHEAT );
ConVar tf_bot_medic_max_heal_range( "tf_bot_medic_max_heal_range", "600", FCVAR_CHEAT );
ConVar tf_bot_medic_debug( "tf_bot_medic_debug", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );
ConVar tf_bot_medic_max_call_response_range( "tf_bot_medic_max_call_response_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_medic_cover_test_resolution( "tf_bot_medic_cover_test_resolution", "8", FCVAR_CHEAT );


class CKnownCollector : public IVision::IForEachKnownEntity
{
public:
	virtual bool Inspect( const CKnownEntity &known ) OVERRIDE
	{
		m_KnownEnts.AddToTail( &known );
		return true;
	}

	CUtlVector<const CKnownEntity *> m_KnownEnts;
};


class CFindMostInjuredNeighbor : public IVision::IForEachKnownEntity
{
public:
	CFindMostInjuredNeighbor( CTFBot *medic, float max_range, bool non_buffed ) :
		m_pMedic( medic ),
		m_flRangeLimit( max_range ),
		m_bUseNonBuffedMaxHealth( non_buffed )
	{
		m_pMostInjured = nullptr;
		m_flHealthRatio = 1.0f;
		m_bIsOnFire = false;
	}

	virtual bool Inspect( const CKnownEntity &known ) OVERRIDE;

	CTFBot *m_pMedic;
	CTFPlayer *m_pMostInjured;
	float m_flHealthRatio;
	bool m_bIsOnFire;
	float m_flRangeLimit;
	bool m_bUseNonBuffedMaxHealth;
};

bool CFindMostInjuredNeighbor::Inspect( const CKnownEntity &known )
{
	if ( !known.GetEntity()->IsPlayer() )
	{
		return true;
	}

	CTFPlayer *pPlayer = ToTFPlayer( known.GetEntity() );
	if ( m_pMedic->IsRangeGreaterThan( pPlayer, m_flRangeLimit ) || !m_pMedic->IsLineOfFireClear( pPlayer->EyePosition() ) )
	{
		return true;
	}

	if ( m_pMedic->IsSelf( pPlayer ) || !pPlayer->IsAlive() || pPlayer->InSameTeam( m_pMedic ) )
	{
		return true;
	}

	int iMaxHealth;
	if ( m_bUseNonBuffedMaxHealth )
	{
		iMaxHealth = pPlayer->GetMaxHealth();
	}
	else
	{
		iMaxHealth = pPlayer->m_Shared.GetMaxBuffedHealth(/* false, false */);
	}

	float flRatio = (float)pPlayer->GetHealth() / iMaxHealth;

	if ( m_bIsOnFire )
	{
		if ( !pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
		{
			return true;
		}
	}
	else
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
		{
			m_pMostInjured = pPlayer;
			m_flHealthRatio = flRatio;
			m_bIsOnFire = true;
			return true;
		}
	}

	if ( flRatio < m_flHealthRatio )
	{
		m_pMostInjured = pPlayer;
		m_flHealthRatio = flRatio;
	}

	return true;
}


class CSelectPrimaryPatient : public IVision::IForEachKnownEntity
{
public:
	CSelectPrimaryPatient( CTFBot *actor, CTFWeaponBase *weapon, CTFPlayer *currPatient )
	{
		m_pMedic = actor;
		m_pMedigun = dynamic_cast<CWeaponMedigun *>( weapon );
		m_pPatient = currPatient;
	}

	virtual bool Inspect( const CKnownEntity &known ) OVERRIDE;

	CTFPlayer *SelectPreferred( CTFPlayer *player1, CTFPlayer *player2 );

	CTFBot *m_pMedic;
	CWeaponMedigun *m_pMedigun;
	CTFPlayer *m_pPatient;
};

bool CSelectPrimaryPatient::Inspect( const CKnownEntity &known )
{
	VPROF_BUDGET( __FUNCTION__, "NextBotSpiky" );

	if ( known.GetEntity() == nullptr ||
		 !known.GetEntity()->IsPlayer() ||
		 !known.GetEntity()->IsAlive() ||
		 !m_pMedic->IsFriend( known.GetEntity() ) )
	{
		return true;
	}

	CTFPlayer *player = dynamic_cast<CTFPlayer *>( known.GetEntity() );
	if ( player == nullptr || m_pMedic->IsSelf( player ) )
	{
		return true;
	}

	if ( !player->HasTheFlag() && m_pMedic->GetSquad() )
	{
		if ( player->IsPlayerClass( TF_CLASS_MEDIC ) ||
			 player->IsPlayerClass( TF_CLASS_SNIPER ) ||
			 player->IsPlayerClass( TF_CLASS_ENGINEER ) ||
			 player->IsPlayerClass( TF_CLASS_SPY ) )
		{
			return true;
		}
	}

	m_pPatient = SelectPreferred( m_pPatient, player );

	return true;
}

CTFPlayer *CSelectPrimaryPatient::SelectPreferred( CTFPlayer *player1, CTFPlayer *player2 )
{
	static const int preferredClass[] = {
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_SOLDIER,
		TF_CLASS_PYRO,
		TF_CLASS_DEMOMAN,
		TF_CLASS_UNDEFINED,
	};

	if ( TFGameRules()->IsInTraining() )
	{
		if ( player1 != nullptr && !player1->IsBot() )
			return player1;
		else
			return player2;
	}

	if ( player1 )
	{
		if ( !player2 )
			return player1;

		const int nNumHealers1 = player1->m_Shared.GetNumHealers();
		for ( int i = 0; i < nNumHealers1; ++i )
		{
			CTFPlayer *healer = ToTFPlayer( player1->m_Shared.GetHealerByIndex( i ) );
			if ( healer && !m_pMedic->IsSelf( healer ) )
			{
				// Don't stack
				return player2;
			}
		}

		const int nNumHealers2 = player1->m_Shared.GetNumHealers();
		for ( int i = 0; i < nNumHealers2; ++i )
		{
			CTFPlayer *healer = ToTFPlayer( player2->m_Shared.GetHealerByIndex( i ) );
			if ( healer && !m_pMedic->IsSelf( healer ) )
			{
				// Don't stack
				return player1;
			}
		}

		CTFBotPathCost func( m_pMedic, FASTEST_ROUTE );
		bool bP1Called = false, bP2Called = false;
		int iPreferred1 = 999, iPreferred2 = 999;

		if ( !player1->IsBot() && player1->m_lastCalledMedic.HasStarted() && player1->m_lastCalledMedic.IsLessThen( 5.0f ) )
		{
			if ( m_pMedic->IsRangeLessThan( player1, tf_bot_medic_max_call_response_range.GetFloat() ) )
			{
				if( NavAreaTravelDistance( player1->GetLastKnownArea(), m_pMedic->GetLastKnownArea(), func, tf_bot_medic_max_call_response_range.GetFloat() * 1.5f ) >= 0.0f )
					bP1Called = true;
			}
		}

		if ( !player2->IsBot() && player2->m_lastCalledMedic.HasStarted() && player2->m_lastCalledMedic.IsLessThen( 5.0f ) )
		{
			if ( m_pMedic->IsRangeLessThan( player2, tf_bot_medic_max_call_response_range.GetFloat() ) )
			{
				if ( NavAreaTravelDistance( player2->GetLastKnownArea(), m_pMedic->GetLastKnownArea(), func, tf_bot_medic_max_call_response_range.GetFloat() * 1.5f ) >= 0.0f )
					bP2Called = true;
			}
		}

		if ( bP1Called )
		{
			if ( bP2Called )
			{
				if ( player1->m_lastCalledMedic.GetElapsedTime() < player2->m_lastCalledMedic.GetElapsedTime() )
					return player1;

				return player2;
			}

			return player1;
		}

		if ( bP2Called )
			return player2;

		for ( int i=0; preferredClass[i] != TF_CLASS_UNDEFINED; ++i )
		{
			if ( player1->IsPlayerClass( preferredClass[i] ) )
				iPreferred2 = ( i < 3 ) ? 0 : i;

			if ( player2->IsPlayerClass( preferredClass[i] ) )
				iPreferred1 = ( i < 3 ) ? 0 : i;
		}

		const float flPreferredTolerance = 300.0f;
		if ( iPreferred1 == iPreferred2 )
		{
			if ( ( m_pMedic->GetDistanceBetween( player1 ) - m_pMedic->GetDistanceBetween( player2 ) ) <= flPreferredTolerance )
				return player1;
		}

		const float flNewPreferredTol = 750.0f;
		if ( iPreferred2 > iPreferred1 )
		{
			if ( m_pMedic->GetDistanceBetween( player2 ) >= flNewPreferredTol )
				return player2;
		}
	}

	return player2;
}


CTFBotMedicHeal::CTFBotMedicHeal()
{
}

CTFBotMedicHeal::~CTFBotMedicHeal()
{
}


const char *CTFBotMedicHeal::GetName( void ) const
{
	return "Heal";
}


ActionResult<CTFBot> CTFBotMedicHeal::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_ChasePath.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_vecPatientPosition = vec3_origin;
	m_hPatient = nullptr;
	// dword @ 0x4868 = 0
	m_isPatientRunningTimer.Invalidate();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMedicHeal::Update( CTFBot *me, float interval )
{
	m_hPatient = SelectPatient( me, m_hPatient );
	if ( !m_hPatient )
	{
		return Action<CTFBot>::SuspendFor( new CTFBotMedicRetreat, "Retreating to find another patient to heal" );
	}

	if ( ( m_hPatient->GetAbsOrigin() - m_vecPatientPosition ).IsLengthGreaterThan( 200.0f ) )
	{
		m_vecPatientPosition = m_hPatient->GetAbsOrigin();
		m_isPatientRunningTimer.Start( 3.0f );
	}

	if ( m_hPatient->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
	{
		CObjectTeleporter *pClosest = NULL;
		float              flMinDist = FLT_MAX;

		CUtlVector<CBaseObject *> objVector;
		TFNavMesh()->CollectBuiltObjects( &objVector, me->GetTeamNumber() );

		for ( int i=0; i<objVector.Count(); ++i )
		{
			if ( objVector[i]->GetType() != OBJ_TELEPORTER )
				continue;
			
			CObjectTeleporter *teleporter = (CObjectTeleporter *)objVector[i];
			if ( teleporter->GetObjectMode() == TELEPORTER_TYPE_ENTRANCE && teleporter->IsReady() )
			{
				float flDist = ( teleporter->GetAbsOrigin() - m_hPatient->GetAbsOrigin() ).LengthSqr();
				if ( flDist < flMinDist )
				{
					flMinDist = flDist;
					pClosest = teleporter;
				}
			}
		}

		if ( pClosest )
			return Action<CTFBot>::SuspendFor( new CTFBotUseTeleporter( pClosest, CTFBotUseTeleporter::USE_WAIT ), "Following my patient through a teleporter" );
	}

	CTFPlayer *         actualHealTarget = m_hPatient;
	bool                isHealTargetBlocked = true;
	bool                isActivelyHealing = false;
	bool                bUnknownBool = false;
	const CKnownEntity *knownThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	CWeaponMedigun *medigun = dynamic_cast<CWeaponMedigun *>( me->GetActiveTFWeapon() );
	if ( medigun )
	{
		/*if ( medigun->GetMedigunType() == MEDIGUN_RESIST )
		{
			while ( ( me->HasAttribute( VACCINATORBULLETS ) && medigun->GetResistType() != MEDIGUN_BULLET_RESIST )
					|| ( me->HasAttribute( VACCINATORBLAST )   && medigun->GetResistType() != MEDIGUN_BLAST_RESIST )
					|| ( me->HasAttribute( VACCINATORFIRE )    && medigun->GetResistType() != MEDIGUN_FIRE_RESIST ) )
			{
				medigun->CycleResistType();
			}
		}*/

		if ( !medigun->IsReleasingCharge() && IsStable( m_hPatient ) && !TFGameRules()->IsInTraining() )
		{
			bool isInCombat = actualHealTarget ? actualHealTarget->GetTimeSinceWeaponFired() < 1.0f : false;

			CFindMostInjuredNeighbor neighbor( me, 0.9f * medigun->GetTargetRange(), isInCombat );
			me->GetVisionInterface()->ForEachKnownEntity( neighbor );

			float hurtRatio = isInCombat ? 0.5f : 1.0f;
			if ( neighbor.m_pMostInjured && neighbor.m_flHealthRatio < hurtRatio )
			{
				actualHealTarget = neighbor.m_pMostInjured;
			}
		}

		me->GetBodyInterface()->AimHeadTowards( actualHealTarget, IBody::CRITICAL, 1.0f, NULL, "Aiming at my patient" );

		if ( !medigun->GetHealTarget() || medigun->GetHealTarget() == actualHealTarget )
		{
			me->PressFireButton();
			isHealTargetBlocked = false;
			isActivelyHealing = ( medigun->GetHealTarget() != NULL );
		}
		else
		{
			if ( m_changePatientTimer.IsElapsed() )
			{
				m_changePatientTimer.Start( RandomFloat( 1.0f, 2.0f ) );
			}
			else
			{
				me->PressFireButton();
			}
		}

		bool useUber = false;
		if ( IsReadyToDeployUber( medigun ) && CanDeployUber( me, medigun ) )
		{
			if ( medigun->GetMedigunType() == TF_MEDIGUN_VACCINATOR )
			{
				if ( me->GetTimeSinceLastInjury( GetEnemyTeam( me ) ) < 1.0f )
				{
					useUber = true;
				}

				if ( m_hPatient->GetTimeSinceLastInjury( GetEnemyTeam( m_hPatient ) ) < 1.0f )
				{
					useUber = true;
				}
			}
			else
			{
				const float healthyRatio = 0.5f;
				useUber = ( ( (float)m_hPatient->GetHealth() / (float)m_hPatient->GetMaxHealth() ) < healthyRatio );

				if ( m_hPatient->m_Shared.InCond( TF_COND_INVULNERABLE ) || m_hPatient->m_Shared.InCond( TF_COND_MEGAHEAL ) )
				{
					useUber = false;
				}

				if ( me->GetHealth() < me->GetUberHealthThreshold() )
				{
					if ( me->GetTimeSinceLastInjury( GetEnemyTeam( me ) ) < 1.0f )
					{
						useUber = true;
					}
				}

				if ( me->GetHealth() < 25 )
				{
					useUber = true;
				}
			}

			if ( useUber )
			{
				if ( !m_delayUberTimer.HasStarted() )
				{
					m_delayUberTimer.Start( me->GetUberDeployDelayDuration() );
				}

				if ( m_delayUberTimer.IsElapsed() )
				{
					m_delayUberTimer.Invalidate();
					me->PressAltFireButton();
				}
			}
		}

		//Some mvm bot shield stuff or smthm..
	}

	bool isThreatened = false;
	if ( knownThreat && knownThreat->IsVisibleRecently() && knownThreat->GetEntity() )
	{
		if ( actualHealTarget )
		{
			float patientRangeSq = me->GetRangeSquaredTo( actualHealTarget );
			float threatRangeSq = me->GetRangeSquaredTo( knownThreat->GetEntity() );
			isThreatened = threatRangeSq < patientRangeSq;
		}
		else
		{
			isThreatened = true;
		}
	}

	bool outOfHealRange = me->IsRangeGreaterThan( actualHealTarget, 1.1f * tf_bot_medic_max_heal_range.GetFloat() );
	bool isPatientObscured = actualHealTarget ? !me->IsLineOfFireClear( actualHealTarget->EyePosition() ) : true;

	if ( !IsReadyToDeployUber( medigun ) && !me->m_Shared.InCond( TF_COND_INVULNERABLE ) && !isActivelyHealing && !bUnknownBool && ( isThreatened || outOfHealRange || isPatientObscured ) )
	{
		me->EquipBestWeaponForThreat( knownThreat );

		if ( knownThreat && knownThreat->GetEntity() )
		{
			me->GetBodyInterface()->AimHeadTowards( knownThreat->GetEntity(), IBody::IMPORTANT, 1.0f, NULL, "Aiming at an enemy" );
		}
	}
	else
	{
		CBaseCombatWeapon *gun = me->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY );
		if ( gun )
		{
			me->Weapon_Switch( gun );
		}
	}

	if ( me->m_Shared.InCond( TF_COND_INVULNERABLE ) || IsReadyToDeployUber( medigun ) || isHealTargetBlocked )
	{
		if ( me->IsRangeGreaterThan( m_hPatient, tf_bot_medic_stop_follow_range.GetFloat() ) || !me->IsAbleToSee( m_hPatient, CBaseCombatCharacter::DISREGARD_FOV ) )
		{
			CTFBotPathCost func( me, FASTEST_ROUTE );
			m_ChasePath.Update( me, m_hPatient, func );
		}
	}
	else
	{
		if ( m_coverTimer.IsElapsed() || IsVisibleToEnemy( me, me->EyePosition() ) )
		{
			m_coverTimer.Start( RandomFloat( 0.5f, 1.0f ) );

			ComputeFollowPosition( me );

			CTFBotPathCost func( me, FASTEST_ROUTE );
			m_PathFollower.Compute( me, m_vecFollowPosition, func );
		}

		m_PathFollower.Update( me );
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMedicHeal::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_ChasePath.Invalidate();

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotMedicHeal::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMedicHeal::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMedicHeal::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMedicHeal::OnActorEmoted( CTFBot *me, CBaseCombatCharacter *who, int concept )
{
	CTFPlayer *pPlayer = ToTFPlayer( who );
	if ( pPlayer == nullptr )
		return Action<CTFBot>::TryContinue();

	if ( concept == MP_CONCEPT_PLAYER_GO ||
		 concept == MP_CONCEPT_PLAYER_ACTIVATECHARGE )
	{
		CTFPlayer *pPatient = m_hPatient;
		if ( pPatient && pPlayer && ENTINDEX( pPlayer ) == ENTINDEX( pPatient ) )
		{
			CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( me->m_Shared.GetActiveTFWeapon() );

			if ( IsReadyToDeployUber( pMedigun ) )
				me->PressAltFireButton();
		}
	}

	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotMedicHeal::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotMedicHeal::ShouldRetreat( const INextBot *me ) const
{
	CTFBot *actor = static_cast<CTFBot *>( me->GetEntity() );

	return (QueryResultType)( actor->m_Shared.IsControlStunned() || actor->m_Shared.IsLoser() );
}

QueryResultType CTFBotMedicHeal::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	CTFBot *actor = static_cast<CTFBot *>( me->GetEntity() );

	return (QueryResultType)actor->IsCombatWeapon();
}


void CTFBotMedicHeal::ComputeFollowPosition( CTFBot *actor )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_vecFollowPosition = actor->GetAbsOrigin();

	if ( !m_hPatient )
		return;

	Vector vecFwd;
	m_hPatient->EyeVectors( &vecFwd );
	vecFwd.NormalizeInPlace();

	if ( !IsVisibleToEnemy( actor, m_hPatient->WorldSpaceCenter() ) )
	{
		if ( actor->IsAbleToSee( m_hPatient, CBaseCombatCharacter::DISREGARD_FOV ) )
		{
			if ( m_hPatient->GetTimeSinceWeaponFired() <= 5.0f || ( m_hPatient->GetAbsOrigin() - actor->GetAbsOrigin() ).Dot( vecFwd ) >= 0.0f || TFGameRules()->InSetup() )
			{
				m_vecFollowPosition = actor->GetAbsOrigin();
			}
			else
			{
				const float flRange = tf_bot_medic_stop_follow_range.GetFloat();
				m_vecFollowPosition = m_hPatient->GetAbsOrigin() - vecFwd * flRange;
			}
		}
		else
		{
			m_vecFollowPosition = m_hPatient->GetAbsOrigin();
		}

		return;
	}

	NextBotTraceFilterIgnoreActors filter( actor, COLLISION_GROUP_NONE );
	CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( actor->GetActiveTFWeapon() );
	float flRange = tf_bot_medic_max_heal_range.GetFloat();
	float flRandomRange = ( RandomFloat( 0, 100.0f ) + tf_bot_medic_stop_follow_range.GetFloat() );

	if ( !m_isPatientRunningTimer.IsElapsed() || IsReadyToDeployUber( pMedigun ) )
		flRange = tf_bot_medic_start_follow_range.GetFloat(); // Get closer

	if ( flRange >= flRandomRange )
	{
		float flCoverCoefficient = M_PI / tf_bot_medic_cover_test_resolution.GetFloat();
		float flMinDistance = FLT_MAX;

		while ( flRange >= flRandomRange )
		{
			float flCoverTest = 0.0f;
			while ( flCoverTest < ( M_PI_F * 2.0f ) )
			{
				float sin, cos;
				FastSinCos( flCoverTest, &sin, &cos );

				Vector vecStart = m_hPatient->WorldSpaceCenter();
				Vector vecEnd( vecStart.x + (cos * flRandomRange), vecStart.y + (sin * flRandomRange), vecStart.z );

				trace_t trace;
				UTIL_TraceLine( vecStart, vecEnd, MASK_BLOCKLOS_AND_NPCS, &filter, &trace );

				if ( trace.DidHit() )
				{
					float flHullWidth = actor->GetBodyInterface()->GetHullWidth();
					trace.endpos.x -= ( cos * ( flHullWidth * 0.5 ) );
					trace.endpos.y -= ( sin * ( flHullWidth * 0.5 ) );
				}

				TheNavMesh->GetSimpleGroundHeight( trace.endpos, &trace.endpos.z );

				Vector vecMaxs = actor->GetBodyInterface()->GetHullMaxs();
				if ( ( m_hPatient->GetAbsOrigin().z - trace.endpos.z ) <= vecMaxs.z )
				{
					trace.endpos.z += 62.0f;
					if ( !IsVisibleToEnemy( actor, trace.endpos ) )
					{
						if ( ( actor->EyePosition() - trace.endpos ).IsLengthLessThan( flMinDistance ) )
						{
							m_vecFollowPosition = trace.endpos;
							flMinDistance = ( actor->EyePosition() - trace.endpos ).LengthSqr();
						}

						if ( tf_bot_medic_debug.GetBool() )
						{
							NDebugOverlay::Cross3D( trace.endpos, 5.0f, 0, 255, 0, true, 1.0 );
							NDebugOverlay::Line( m_hPatient->GetAbsOrigin(), trace.endpos, 0, 255, 0, true, 1.0 );
						}
					}
					else if ( tf_bot_medic_debug.GetBool() )
					{
						NDebugOverlay::Cross3D( trace.endpos, 5.0f, 255, 0, 0, true, 1.0 );
						NDebugOverlay::Line( m_hPatient->GetAbsOrigin(), trace.endpos, 255, 0, 0, true, 1.0 );
					}
				}
				else if ( tf_bot_medic_debug.GetBool() )
				{
					NDebugOverlay::Cross3D( trace.endpos, 5.0f, 255, 100, 0, true, 1.0 );
					NDebugOverlay::Line( m_hPatient->GetAbsOrigin(), trace.endpos, 255, 100, 0, true, 1.0 );
				}

				flCoverTest += flCoverCoefficient;
			}

			flRandomRange += 100.0f;
		}
	}
}

bool CTFBotMedicHeal::IsGoodUberTarget( CTFPlayer *player ) const
{
	if ( player->IsPlayerClass( TF_CLASS_MEDIC ) ||
		 player->IsPlayerClass( TF_CLASS_SNIPER ) ||
		 player->IsPlayerClass( TF_CLASS_ENGINEER ) ||
		 player->IsPlayerClass( TF_CLASS_SCOUT ) ||
		 player->IsPlayerClass( TF_CLASS_SPY ) )
	{
		return false;
	}

	/* BUG: this function always returns false! */
	return false;
}

bool CTFBotMedicHeal::IsReadyToDeployUber( CWeaponMedigun *medigun ) const
{
	// TODO
	if ( !medigun )
		return false;

	return medigun->GetChargeLevel() >= 100.0f;
}

bool CTFBotMedicHeal::CanDeployUber( CTFBot *actor, CWeaponMedigun *medigun ) const
{
	return true;
}

bool CTFBotMedicHeal::IsStable( CTFPlayer *player ) const
{
	if ( player->GetTimeSinceLastInjury( GetEnemyTeam( player ) ) >= 3.0f && ( player->GetHealth() / player->GetMaxHealth() ) >= 1.0f )
	{
		if ( !player->m_Shared.InCond( TF_COND_BURNING ) && !player->m_Shared.InCond( TF_COND_BLEEDING ) )
			return true;
	}

	return false;
}

bool CTFBotMedicHeal::IsVisibleToEnemy( CTFBot *actor, const Vector& vecPatient ) const
{
	CKnownCollector functor;
	actor->GetVisionInterface()->ForEachKnownEntity( functor );

	for ( int i=0; i<functor.m_KnownEnts.Count(); ++i )
	{
		CBaseCombatCharacter *pBCC = functor.m_KnownEnts[i]->GetEntity()->MyCombatCharacterPointer();
		if ( pBCC && actor->IsEnemy( pBCC ) && pBCC->IsLineOfSightClear( vecPatient, CBaseCombatCharacter::IGNORE_ACTORS ) )
			return true;
	}

	return false;
}

CTFPlayer *CTFBotMedicHeal::SelectPatient( CTFBot *actor, CTFPlayer *currPatient )
{
	CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( actor->GetActiveTFWeapon() );
	CTFPlayer *pBestPatient = currPatient;

	if ( pMedigun )
	{
		if ( !currPatient || !currPatient->IsAlive() )
		{
			if ( !pMedigun->IsAttachedToBuilding() )
				pBestPatient = ToTFPlayer( pMedigun->GetHealTarget() );
		}

		if ( ( pMedigun->IsReleasingCharge() || IsReadyToDeployUber( pMedigun ) ) && pBestPatient && IsGoodUberTarget( pBestPatient ) )
			return pBestPatient;
	}

	CSelectPrimaryPatient func( actor, actor->GetActiveTFWeapon(), currPatient );

	// Some huge logic for MvM is here

	actor->GetVisionInterface()->ForEachKnownEntity( func );

	return func.m_pPatient;
}
