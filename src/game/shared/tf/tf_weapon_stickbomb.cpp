#include "cbase.h"
#include "tf_weapon_stickbomb.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "c_tf_viewmodeladdon.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFStickBomb, DT_TFWeaponStickBomb );

BEGIN_NETWORK_TABLE( CTFStickBomb, DT_TFWeaponStickBomb )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iDetonated ) )
#else
	SendPropInt( SENDINFO( m_iDetonated ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFStickBomb )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_stickbomb, CTFStickBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_stickbomb );


#define MODEL_NORMAL   "models/workshop/weapons/c_models/c_caber/c_caber.mdl"
#define MODEL_EXPLODED "models/workshop/weapons/c_models/c_caber/c_caber_exploded.mdl"

#define TF_STICKBOMB_NORMAL    0
#define TF_STICKBOMB_DETONATED 1

#ifdef GAME_DLL
extern ConVar tf2v_use_new_caber;
#endif

CTFStickBomb::CTFStickBomb()
{
	m_iDetonated = TF_STICKBOMB_NORMAL;
}

const char *CTFStickBomb::GetWorldModel() const
{
	if ( m_iDetonated == TF_STICKBOMB_DETONATED )
	{
		return MODEL_EXPLODED;
	}
	
	return BaseClass::GetWorldModel();
}

void CTFStickBomb::Precache()
{
	BaseClass::Precache();
	
	PrecacheModel( MODEL_NORMAL );
	PrecacheModel( MODEL_EXPLODED );
}

void CTFStickBomb::Smack()
{
	BaseClass::Smack();
	
	if ( m_iDetonated == TF_STICKBOMB_NORMAL && ConnectedHit() ) 
	{
		m_iDetonated = TF_STICKBOMB_DETONATED;
		//m_bBroken = true;
		
		SwitchBodyGroups();
		
#ifdef GAME_DLL
		CTFPlayer *owner = GetTFPlayerOwner();
		if ( owner ) 
		{
			// TF2 does these things and doesn't use the results:
			// calls owner->EyeAngles() and then AngleVectors()
			// calls this->GetSwingRange()
			
			// my bet: they meant to multiply the fwd vector by the swing range
			// and then use that for the damage force, but they typo'd it and
			// just reused the shoot position instead
			
			Vector VecWpnPos = owner->Weapon_ShootPosition();
			
			CPVSFilter filter( VecWpnPos );
			TE_TFExplosion( filter, 0.0f, VecWpnPos, Vector( 0.0f, 0.0f, 1.0f ),
				TF_WEAPON_GRENADELAUNCHER, ENTINDEX( owner ) );
				
			float flDamage = 100.0f;
			if ( tf2v_use_new_caber.GetBool() )
				flDamage = 75.0f;
			float flRadius = 100.0f;
			
			/* why is the damage force vector set to Weapon_ShootPosition()?
			 * I dunno! */
			CTakeDamageInfo dmgInfo( owner, owner, this, VecWpnPos, VecWpnPos, flDamage,
				DMG_BLAST | DMG_CRITICAL | ( IsCurrentAttackACrit() ? DMG_USEDISTANCEMOD : 0 ),
				TF_DMG_CUSTOM_STICKBOMB_EXPLOSION, &VecWpnPos );
			
			CTFRadiusDamageInfo radiusInfo( &dmgInfo, VecWpnPos, flRadius, NULL, flRadius );
			TFGameRules()->RadiusDamage( radiusInfo );
		}
#endif
	}
}

void CTFStickBomb::SwitchBodyGroups()
{
#ifdef CLIENT_DLL
	C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
	if ( pAttach )
	{
		int bodygroup = pAttach->FindBodygroupByName( "broken" );
		pAttach->SetBodygroup( bodygroup, m_iDetonated );
	}
#endif
}

void CTFStickBomb::WeaponRegenerate()
{
	m_iDetonated = TF_STICKBOMB_NORMAL;
	SetContextThink( &CTFStickBomb::SwitchBodyGroups, gpGlobals->curtime + 0.01f, "SwitchBodyGroups" );
}

void CTFStickBomb::WeaponReset()
{
	m_iDetonated = TF_STICKBOMB_NORMAL;
	BaseClass::WeaponReset();
}

#ifdef CLIENT_DLL
int C_TFStickBomb::GetWorldModelIndex()
{
	if ( !modelinfo ) {
		return BaseClass::GetWorldModelIndex();
	}
	
	int index = modelinfo->GetModelIndex( m_iDetonated == TF_STICKBOMB_DETONATED ? MODEL_EXPLODED : MODEL_NORMAL );
	m_iWorldModelIndex = index;
	return index;
}
#endif
