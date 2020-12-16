//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_powerup_bottle.h"
#include "tf_gamerules.h"

#if defined( CLIENT_DLL )
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef GAME_DLL
extern ConVar cl_hud_minmode;
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFPowerupBottle, DT_TFPowerupBottle )
BEGIN_NETWORK_TABLE( CTFPowerupBottle, DT_TFPowerupBottle )
#if defined( GAME_DLL )
	SendPropBool( SENDINFO( m_bActive ) ),
	SendPropInt( SENDINFO( m_usNumCharges ), -1, SPROP_UNSIGNED ),
#else
	RecvPropBool( RECVINFO( m_bActive ) ),
	RecvPropInt( RECVINFO( m_usNumCharges ) ),
#endif // GAME_DLL
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFPowerupBottle )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_powerup_bottle, CTFPowerupBottle );
PRECACHE_REGISTER( tf_powerup_bottle );

CTFPowerupBottle::CTFPowerupBottle()
{
#ifdef CLIENT_DLL
	ListenForGameEvent( "player_spawn" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFPowerupBottle::GetSkin()
{
	int nMode = 0;
	CALL_ATTRIB_HOOK_INT( nMode, set_weapon_mode );
	if ( nMode != 1 )
	{
		return ( ( GetNumCharges() > 0 ) ? 1 : 0 );
	}

	return BaseClass::GetSkin();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::Precache( void )
{
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_generic.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_krit.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_uber.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_tele.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_ammo.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_build.mdl" );

	BaseClass::Precache();
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "player_spawn" ) )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pOwner )
			return;

		const int nUserID = event->GetInt( "userid" );
		CBasePlayer *pPlayer = UTIL_PlayerByUserId( nUserID );
		if ( pPlayer && pPlayer == pOwner )
		{
			m_flLastSpawnTime = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFPowerupBottle::GetWorldModelIndex( void )
{
	int nMode = 0;
	CALL_ATTRIB_HOOK_INT( nMode, set_weapon_mode );
	if ( nMode != 1 && ( GetNumCharges() > 0 ) )
	{
		switch ( GetPowerupType() )
		{
			case POWERUP_BOTTLE_CRITBOOST:
				return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_krit.mdl" );

			case POWERUP_BOTTLE_UBERCHARGE:
				return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_uber.mdl" );

			case POWERUP_BOTTLE_RECALL:
				return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_tele.mdl" );

			case POWERUP_BOTTLE_REFILL_AMMO:
				return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_ammo.mdl" );

			case POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE:
				return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_build.mdl" );
		}
	}

	return BaseClass::GetWorldModelIndex();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerupBottle::ReapplyProvision( void )
{
	BaseClass::ReapplyProvision();

	CBaseEntity *pOwner = GetOwnerEntity();
	IHasAttributes *pOwnerAttribInterface = GetAttribInterface( pOwner );
	if ( pOwnerAttribInterface )
	{
		if ( m_bActive )
		{
			if ( !pOwnerAttribInterface->GetAttributeManager()->IsBeingProvidedToBy( this ) )
			{
				GetAttributeManager()->ProvideTo( pOwner );
			}
		}
		else
		{
			GetAttributeManager()->StopProvidingTo( pOwner );
		}

	#if defined( GAME_DLL )
		// TODO: Big block of canteen specialist logic
		CTFPlayer *pTFOwner = dynamic_cast< CTFPlayer* >( pOwner );
		if ( pTFOwner )
		{
		}
	#endif
	}
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerupBottle::UnEquip( CBasePlayer* pOwner )
{
	BaseClass::UnEquip( pOwner );
	RemoveEffect();
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerupBottle::AllowedToUse() const
{
	if ( TFGameRules() && !( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS || TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) )
		return false;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsObserver() || !pPlayer->IsAlive() )
		return false;

#ifdef GAME_DLL
	m_flLastSpawnTime = pPlayer->GetSpawnTime();
#endif

	if ( gpGlobals->curtime < m_flLastSpawnTime + 0.7f )
		return false;

	return true;
}

const char* CTFPowerupBottle::GetEffectLabelText( void )
{
#ifndef GAME_DLL
	if ( cl_hud_minmode.GetBool() )
	{
		return "#TF_PVE_UsePowerup_MinMode";
	}
#endif

	switch ( GetPowerupType() )
	{
		case POWERUP_BOTTLE_CRITBOOST:
			return "#TF_PVE_UsePowerup_CritBoost";

		case POWERUP_BOTTLE_UBERCHARGE:
			return "#TF_PVE_UsePowerup_Ubercharge";

		case POWERUP_BOTTLE_RECALL:
			return "#TF_PVE_UsePowerup_Recall";

		case POWERUP_BOTTLE_REFILL_AMMO:
			return "#TF_PVE_UsePowerup_RefillAmmo";

		case POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE:
			return "#TF_PVE_UsePowerup_BuildinginstaUpgrade";
	}

	return "#TF_PVE_UsePowerup_CritBoost";
}

const char* CTFPowerupBottle::GetEffectIconName( void )
{
	switch ( GetPowerupType() )
	{
		case POWERUP_BOTTLE_CRITBOOST:
			return "../hud/ico_powerup_critboost_red";

		case POWERUP_BOTTLE_UBERCHARGE:
			return "../hud/ico_powerup_ubercharge_red";

		case POWERUP_BOTTLE_RECALL:
			return "../hud/ico_powerup_recall_red";

		case POWERUP_BOTTLE_REFILL_AMMO:
			return "../hud/ico_powerup_refill_ammo_red";

		case POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE:
			return "../hud/ico_powerup_building_instant_red";
	}

	return "../hud/ico_powerup_critboost_red";
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFPowerupBottle::GetNumCharges() const
{
	return m_usNumCharges; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::SetNumCharges( int usNumCharges )
{
	static CSchemaAttributeHandle pAttrDef_PowerupCharges( "powerup charges" );

	m_usNumCharges = usNumCharges; 

	if ( !pAttrDef_PowerupCharges )
		return;

	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( !pItem )
		return;

	pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_PowerupCharges, usNumCharges );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFPowerupBottle::GetMaxNumCharges() const
{
	int iMaxNumCharges = 0;
	CALL_ATTRIB_HOOK_INT( iMaxNumCharges, powerup_max_charges );
	iMaxNumCharges = Min( iMaxNumCharges, 6 );

	return iMaxNumCharges;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PowerupBottleType_t CTFPowerupBottle::GetPowerupType( void ) const
{
	int nHasCritBoost = 0;
	CALL_ATTRIB_HOOK_INT( nHasCritBoost, critboost );
	if ( nHasCritBoost )
	{
		return POWERUP_BOTTLE_CRITBOOST;
	}

	int nHasUbercharge = 0;
	CALL_ATTRIB_HOOK_INT( nHasUbercharge, ubercharge );
	if ( nHasUbercharge )
	{
		return POWERUP_BOTTLE_UBERCHARGE;
	}

	int nHasRecall = 0;
	CALL_ATTRIB_HOOK_INT( nHasRecall, recall );
	if ( nHasRecall )
	{
		return POWERUP_BOTTLE_RECALL;
	}

	int nHasRefillAmmo = 0;
	CALL_ATTRIB_HOOK_INT( nHasRefillAmmo, refill_ammo );
	if ( nHasRefillAmmo )
	{
		return POWERUP_BOTTLE_REFILL_AMMO;
	}

	int nHasInstaBuildingUpgrade = 0;
	CALL_ATTRIB_HOOK_INT( nHasInstaBuildingUpgrade, building_instant_upgrade );
	if ( nHasInstaBuildingUpgrade )
	{
		return POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE;
	}

	return POWERUP_BOTTLE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::RemoveEffect()
{
	m_bActive = false;
	ReapplyProvision();
	SetContextThink( NULL, 0, "PowerupBottleThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerupBottle::Reset( void )
{
	m_bActive = false;
	m_usNumCharges = 0;

#ifdef GAME_DLL
	class CAttributeIterator_ZeroRefundableCurrency : public IEconUntypedAttributeIterator
	{
	public:
		CAttributeIterator_ZeroRefundableCurrency( CAttributeList *pAttribList )
			: m_pAttribList( pAttribList )
		{
		}

	private:
		virtual bool OnIterateAttributeValueUntyped( CEconAttributeDefinition const *pAttrDef )
		{
			if ( FindAttribute( m_pAttribList, pAttrDef ) )
			{
				m_pAttribList->SetRuntimeAttributeRefundableCurrency( pAttrDef, 0 );
			}

			return true;
		}

		CAttributeList *m_pAttribList;
	};

	CAttributeIterator_ZeroRefundableCurrency it( GetAttributeList() );
	GetAttributeList()->IterateAttributes( &it );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::StatusThink()
{
	RemoveEffect();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerupBottle::Use()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner == nullptr )
		return false;

	if ( !m_bActive && GetNumCharges() > 0 )
	{
		if ( !AllowedToUse() )
			return false;

	#ifdef GAME_DLL
		// Use up one charge worth of refundable money when a charge is used
		class CAttributeIterator_ConsumeOneRefundableCharge : public IEconUntypedAttributeIterator
		{
		public:
			CAttributeIterator_ConsumeOneRefundableCharge( CAttributeList *pAttribList, int iNumCharges )
				: m_pAttribList( pAttribList ), m_iNumCharges( iNumCharges )
			{
			}

		private:
			virtual bool OnIterateAttributeValueUntyped( CEconAttributeDefinition const *pAttrDef )
			{
				if ( FindAttribute( m_pAttribList, pAttrDef ) )
				{
					int nRefundableCurrency = m_pAttribList->GetRuntimeAttributeRefundableCurrency( pAttrDef );
					if ( nRefundableCurrency > 0 )
					{
						m_pAttribList->SetRuntimeAttributeRefundableCurrency( pAttrDef, nRefundableCurrency - (nRefundableCurrency / m_iNumCharges) );
					}
				}

				return true;
			}

			CAttributeList *m_pAttribList;
			int m_iNumCharges;
		};

		CAttributeIterator_ConsumeOneRefundableCharge it( GetAttributeList(), GetNumCharges() );
		GetAttributeList()->IterateAttributes( &it );
	#endif

		float flDuration = 0;
		CALL_ATTRIB_HOOK_FLOAT( flDuration, powerup_duration );
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, flDuration, canteen_specialist );

		IGameEvent *event = gameeventmanager->CreateEvent( "player_used_powerup_bottle" );
		if ( event )
		{
			event->SetInt( "player", pOwner->entindex() );
			event->SetInt( "type", GetPowerupType() );
			event->SetFloat( "time", flDuration );
			gameeventmanager->FireEvent( event );
		}

	#ifdef GAME_DLL
		if ( pOwner )
		{
			/*EconEntity_OnOwnerKillEaterEventNoPartner( this, pOwner, 109 );
			pOwner->ForgetFirstUpgradeForItem( GetAttributeContainer()->GetItem() );*/
		}
	#endif

		m_usNumCharges--;
		m_bActive = true;
		ReapplyProvision();

		SetContextThink( &CTFPowerupBottle::StatusThink, gpGlobals->curtime + flDuration, "PowerupBottleThink" );
		return true;
	}

	return false;
}
