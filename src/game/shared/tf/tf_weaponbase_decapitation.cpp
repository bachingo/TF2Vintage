#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#else
#include "tf_player.h"
#endif

#include "tf_wearable_demoshield.h"
#include "tf_weaponbase_decapitation.h"


CTFDecapitationMeleeWeaponBase::CTFDecapitationMeleeWeaponBase()
{
	m_hEquippedShield = nullptr;
}

CTFDecapitationMeleeWeaponBase::~CTFDecapitationMeleeWeaponBase()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFDecapitationMeleeWeaponBase::TranslateViewmodelHandActivity( int iActivity )
{
	int iTranslation = iActivity;

	if (GetTFPlayerOwner())
	{
		if (( !GetItem() || GetItem()->GetAnimationSlot() != TF_WPN_TYPE_MELEE_ALLCLASS )/* && DWORD( pOwner + 9236 ) == 4*/)
		{
			switch (iActivity)
			{
				case ACT_VM_DRAW:
					iTranslation = ACT_VM_DRAW_SPECIAL;
					break;
				case ACT_VM_HOLSTER:
					iTranslation = ACT_VM_HOLSTER_SPECIAL;
					break;
				case ACT_VM_IDLE:
					iTranslation = ACT_VM_IDLE_SPECIAL;
					break;
				case ACT_VM_PULLBACK:
					iTranslation = ACT_VM_PULLBACK_SPECIAL;
					break;
				case ACT_VM_PRIMARYATTACK:
				case ACT_VM_SECONDARYATTACK:
					iTranslation = ACT_VM_PRIMARYATTACK_SPECIAL;
					break;
				case ACT_VM_HITCENTER:
					iTranslation = ACT_VM_HITCENTER_SPECIAL;
					break;
				case ACT_VM_SWINGHARD:
					iTranslation = ACT_VM_SWINGHARD_SPECIAL;
					break;
				case ACT_VM_IDLE_TO_LOWERED:
					iTranslation = ACT_VM_IDLE_TO_LOWERED_SPECIAL;
					break;
				case ACT_VM_IDLE_LOWERED:
					iTranslation = ACT_VM_IDLE_LOWERED_SPECIAL;
					break;
				case ACT_VM_LOWERED_TO_IDLE:
					iTranslation = ACT_VM_LOWERED_TO_IDLE_SPECIAL;
					break;
				default:
					return BaseClass::TranslateViewmodelHandActivity( iTranslation );
			}
		}
	}

	return BaseClass::TranslateViewmodelHandActivity( iTranslation );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDecapitationMeleeWeaponBase::CanDecapitate( void )
{
	if (GetItem() && GetItem()->GetStaticData())
		return CAttributeManager::AttribHookValue<int>( 0, "decapitate_type", this ) != 0;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFDecapitationMeleeWeaponBase::GetMeleeDamage( CBaseEntity * pTarget, int &iCustomDamage )
{
	float res = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );
	iCustomDamage = TF_DMG_CUSTOM_DECAPITATION;
	return res;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDecapitationMeleeWeaponBase::Holster( CBaseCombatWeapon *pSwitchTo )
{
	if (BaseClass::Holster( pSwitchTo ))
	{
		StopListeningForAllEvents();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::SetupGameEventListeners( void )
{
	ListenForGameEvent( "player_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::FireGameEvent( IGameEvent *event )
{
	if (FStrEq( event->GetName(), "player_death" ))
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if (pOwner && engine->GetPlayerUserId( pOwner->edict() ) == event->GetInt( "attacker" ) && 
			 (event->GetInt( "weaponid" ) == TF_WEAPON_SWORD || event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING) && CanDecapitate())
		{
			CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt( "userid" ) ) );
			OnDecapitation( pVictim );
		}
	}
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::UpdateViewModel( void )
{
	BaseClass::UpdateViewModel();

	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if (pOwner && pOwner->GetViewModel( m_nViewModelIndex ))
	{
		C_TFViewModel *vm = dynamic_cast<C_TFViewModel *>( pOwner->GetViewModel( m_nViewModelIndex, false ) );
		if (pOwner->m_Shared.HasDemoShieldEquipped())
		{
			const char *pszModel = NULL;

			if (m_hEquippedShield != nullptr)
			{
				C_TFWearableDemoShield *pShield = dynamic_cast<C_TFWearableDemoShield *>( m_hEquippedShield.Get() );
				int vmType = pShield->GetItem()->GetStaticData()->attach_to_hands_vm_only;
				if (vmType == VMTYPE_TF2)
				{
					pszModel = pShield->GetItem()->GetPlayerDisplayModel( pOwner->GetPlayerClass()->GetClassIndex() );
				}

				if (pszModel && pszModel[0] != '\0')
				{
					vm->UpdateViewmodelAddon( pszModel, 1 );
				}
				else
				{
					vm->RemoveViewmodelAddon( 1 );
				}

				return;
			}

			for (int i=0; i<pOwner->GetNumWearables(); ++i)
			{
				C_TFWearableDemoShield *pShield = dynamic_cast<C_TFWearableDemoShield *>( pOwner->GetWearable( i ) );
				if (pShield)
				{
					m_hEquippedShield = pShield;
					int vmType = pShield->GetItem()->GetStaticData()->attach_to_hands_vm_only;
					if (vmType == VMTYPE_TF2)
					{
						pszModel = pShield->GetItem()->GetPlayerDisplayModel( pOwner->GetPlayerClass()->GetClassIndex() );
					}

					if (pszModel && pszModel[0] != '\0')
					{
						vm->UpdateViewmodelAddon( pszModel, 1 );
					}
					else
					{
						vm->RemoveViewmodelAddon( 1 );
					}

					return;
				}
			}
		}
		else
		{
			vm->RemoveViewmodelAddon( 1 );
		}
	}
}

#endif