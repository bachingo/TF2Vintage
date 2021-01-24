//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_tf_player.h"
#include "tf_weapon_invis.h"
#include "tf_wearable_demoshield.h"
#include "tf_weapon_sword.h"
#include "tf_weapon_bat_wood.h"
#include "tf_weapon_lunchbox_drink.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_jar.h"
#include "tf_weapon_buff_item.h"
#include "tf_weapon_shotgun.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_rocketlauncher.h"
#include "tf_weapon_revolver.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_flaregun.h"
#include "tf_weapon_particlecannon.h"
#include "tf_weapon_raygun.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_rocketpack.h"
#include "tf_weapon_smg.h"
#include "tf_powerup_bottle.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/AnimationController.h>
#include "hud.h"
#include "hudelement.h"
#include "engine/IEngineSound.h"
#include "tf_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar tf2v_show_killstreak_counter( "tf2v_show_killstreak_counter", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Displays the Killstreak counter on the HUD.", true, 0.0f, true, 1.0f );

extern ConVar tf2v_use_new_cleaners;
extern ConVar tf2v_new_chocolate_behavior;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudItemEffectMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudItemEffectMeter, EditablePanel );

public:
	CHudItemEffectMeter( const char *pElementName );

	virtual void    ApplySchemeSettings( IScheme *scheme );
	virtual void    PerformLayout( void );
	virtual bool	ShouldDraw( void );

	virtual void    Update( C_TFPlayer *pPlayer );

	virtual int     GetCount( void )                    { return -1; }
	virtual float   GetProgress( void );
	virtual const char* GetLabelText( void );
	virtual const char* GetIconName( void )				{ return ""; }
	virtual const char* GetResourceName( void )         { return "resource/UI/HudItemEffectMeter.res"; }
	virtual Color	GetFgColor( void )					{ return COLOR_WHITE; }
	virtual bool    ShouldBeep( void );
	virtual bool    ShouldFlash( void )                 { return false; }
	virtual bool    IsEnabled( void ) OVERRIDE          { return m_bEnabled; }

	virtual CBaseAnimating* GetItem( void ) const		{ return m_hItem; }
	void            SetItem( CBaseAnimating *pItem )    { m_hItem = pItem; }

private:
	ContinuousProgressBar *m_pEffectMeter;
	CExLabel *m_pEffectMeterLabel;

	CPanelAnimationVarAliasType( int, m_iXOffset, "x_offset", "0", "proportional_int" );

	CHandle<CBaseAnimating> m_hItem;
	float m_flOldCharge;

protected:
	bool m_bEnabled;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudItemEffectMeterKillstreak : public CHudItemEffectMeter
{
	DECLARE_CLASS_SIMPLE( CHudItemEffectMeterKillstreak, CHudItemEffectMeter );
public:
	CHudItemEffectMeterKillstreak( const char *pElementName )
		: CHudItemEffectMeter( pElementName )
	{
	}

	virtual int     GetCount(void);
	virtual bool    IsEnabled(void);
	virtual const char* GetLabelText(void)				{ return "#TF_KillStreak"; }
	virtual const char* GetResourceName(void)			{ return "resource/UI/HudItemEffectMeter_Killstreak.res"; }
};


//-----------------------------------------------------------------------------
// Purpose: Templatized item effect meter elements
//-----------------------------------------------------------------------------
template<typename Class>
class CHudItemEffectMeterTemp : public CHudItemEffectMeter
{
	DECLARE_CLASS( CHudItemEffectMeterTemp<Class>, CHudItemEffectMeter );
public:
	CHudItemEffectMeterTemp( const char *pElementName, bool bShouldBeep = true, const char *pResourceName = nullptr )
		: CHudItemEffectMeter( pElementName ), m_pszResource( pResourceName ), m_bBeeps( bShouldBeep )
	{
	}

	virtual int     GetCount( void )          { return -1; }
	virtual float   GetProgress( void );
	virtual const char* GetLabelText( void );
	virtual const char* GetResourceName( void );
	virtual const char* GetIconName( void );
	virtual Color	GetFgColor( void )        { return COLOR_WHITE; }
	virtual bool    ShouldBeep( void )        { return m_bBeeps; }
	virtual bool    ShouldFlash( void )       { return false; }
	virtual bool    IsEnabled( void );
	virtual bool	ShouldDraw(void);

	virtual Class*  GetWeapon( void );

private:
	const char *m_pszResource;
	bool m_bBeeps;
};


class CHudItemEffects : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	CHudItemEffects();
	virtual ~CHudItemEffects();

	virtual bool Init( void );
	virtual void Shutdown( void );
	virtual void Update( float frametime );

	virtual void FireGameEvent( IGameEvent *event );

	void SetPlayer( void );

	inline void AddItemMeter( CHudItemEffectMeter *pMeter )
	{
		DHANDLE<CHudItemEffectMeter> hndl;
		hndl = pMeter;
		if ( hndl )
		{
			gHUD.AddHudElement( hndl.Get() );
			m_pEffectBars.AddToTail( hndl );
			hndl->SetVisible( false );
		}
	}

	static CUtlVector< DHANDLE<CHudItemEffectMeter> > m_pEffectBars;
};
CUtlVector< DHANDLE<CHudItemEffectMeter> > CHudItemEffects::m_pEffectBars;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudItemEffectMeter::CHudItemEffectMeter( const char *pElementName )
	: BaseClass( NULL, pElementName ), CHudElement( pElementName )
{
	SetParent( g_pClientMode->GetViewport() );

	m_pEffectMeter = new ContinuousProgressBar( this, "ItemEffectMeter" );
	m_pEffectMeterLabel = new CExLabel( this, "ItemEffectMeterLabel", "" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_hItem = NULL;
	m_bEnabled = true;
	m_flOldCharge = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( GetResourceName() );

	if ( m_pEffectMeterLabel )
	{
		wchar_t *pszLocalized = g_pVGuiLocalize->Find( GetLabelText() );
		if ( pszLocalized )
			m_pEffectMeterLabel->SetText( pszLocalized );
		else
			m_pEffectMeterLabel->SetText( GetLabelText() );
	}

	CTFImagePanel *pIcon = dynamic_cast<CTFImagePanel *>( FindChildByName( "ItemEffectIcon" ) );
	if ( pIcon )
	{
		pIcon->SetImage( GetIconName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int iNumPanels = 0;
	CHudItemEffectMeter *pLastMeter = nullptr;
	for ( int i = 0; i < CHudItemEffects::m_pEffectBars.Count(); ++i )
	{
		CHudItemEffectMeter *pMeter = CHudItemEffects::m_pEffectBars[i];
		if ( pMeter && pMeter->IsEnabled() )
		{
			iNumPanels++;
			pLastMeter = pMeter;
		}
	}

	if ( iNumPanels > 1 && pLastMeter )
	{
		int x = 0, y = 0;
		GetPos( x, y );
		SetPos( x - m_iXOffset, y );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemEffectMeter::ShouldDraw( void )
{
	bool bResult = false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsAlive() && IsEnabled() )
		bResult = CHudElement::ShouldDraw();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		bResult = false;

	if ( bResult != IsVisible() )
	{
		SetVisible( bResult );
		if ( bResult )
			InvalidateLayout( false, true );
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::Update( C_TFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		m_flOldCharge = 1.0f;
		return;
	}

	if ( m_pEffectMeter && IsEnabled() )
	{
		if ( GetCount() >= 0 )
			SetDialogVariable( "progresscount", GetCount() );

		float flCharge = GetProgress();
		m_pEffectMeter->SetProgress( flCharge );

		// Play a ding when full charged.
		if ( m_flOldCharge < 1.0f && flCharge >= 1.0f && ShouldBeep() )
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "TFPlayer.Recharged" );
		}

		if ( ShouldFlash() )
		{
			// Meter is depleting
			int r = 10 * ( (int)( gpGlobals->realtime * 10.0f ) % 10 ) + 160;
			m_pEffectMeter->SetFgColor( Color( r, 0, 0, 255 ) );
		}
		else
		{
			m_pEffectMeter->SetFgColor( GetFgColor() );
		}

		m_flOldCharge = flCharge;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CHudItemEffectMeter::GetProgress( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsAlive() )
	{
		return pPlayer->m_Shared.GetSpyCloakMeter() / 100;
	}

	return 1.0f;
}

const char *CHudItemEffectMeter::GetLabelText( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		CTFWeaponInvis *pInvis = (CTFWeaponInvis *)pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS );
		if ( pInvis )
		{
			if ( pInvis->HasFeignDeath() )
				return "#TF_Feign";
			if ( pInvis->HasMotionCloak() )
				return "#TF_CloakDagger";
		}
	}

	return "#TF_Cloak";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemEffectMeter::ShouldBeep( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsAlive() )
	{
		C_TFWeaponInvis *pWatch = static_cast<C_TFWeaponInvis *>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
		if ( pWatch && !pWatch->HasFeignDeath() )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename Class>
float CHudItemEffectMeterTemp<Class>::GetProgress( void )
{
	Class *pWeapon = GetWeapon();
	if ( pWeapon )
		return pWeapon->GetEffectBarProgress();

	return CHudItemEffectMeter::GetProgress();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename Class>
const char *CHudItemEffectMeterTemp<Class>::GetLabelText( void )
{
	Class *pWeapon = GetWeapon();
	if ( pWeapon )
		return pWeapon->GetEffectLabelText();

	return CHudItemEffectMeter::GetLabelText();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename Class>
const char *CHudItemEffectMeterTemp<Class>::GetIconName( void )
{
	Class *pWeapon = GetWeapon();
	if ( pWeapon )
		return pWeapon->GetEffectIconName();

	return CHudItemEffectMeter::GetLabelText();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename Class>
const char *CHudItemEffectMeterTemp<Class>::GetResourceName( void )
{
	if ( m_pszResource )
		return m_pszResource;

	return CHudItemEffectMeter::GetResourceName();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename Class>
bool CHudItemEffectMeterTemp<Class>::IsEnabled( void )
{
	if ( GetWeapon() )
		return true;

	return CHudItemEffectMeter::IsEnabled();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename Class>
bool CHudItemEffectMeterTemp<Class>::ShouldDraw( void )
{
	return CHudItemEffectMeter::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename Class>
Class *CHudItemEffectMeterTemp<Class>::GetWeapon( void )
{
	if ( GetItem() == NULL )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			for ( int i = 0; i < pPlayer->WeaponCount(); ++i )
			{
				Class *pWeapon = dynamic_cast<Class *>( pPlayer->GetWeapon( i ) );
				if ( pWeapon )
				{
					SetItem( pWeapon );
					break;
				}
			}
		}

		if ( !GetItem() )
			m_bEnabled = false;
	}

	return static_cast<Class *>( GetItem() );
}

//-----------------------------------------------------------------------------
// C_TFSword Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFSword>::IsEnabled( void )
{
	if ( GetWeapon() && GetWeapon()->CanDecapitate() )
		return true;

	return false;
}

template<>
int CHudItemEffectMeterTemp<C_TFSword>::GetCount( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFSword *pSword = GetWeapon();
		if ( pSword && pSword->CanDecapitate() )
			return pPlayer->m_Shared.GetDecapitationCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// C_TFShotgun_Revenge Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFShotgun_Revenge>::IsEnabled( void )
{
	if ( GetWeapon() )
		return true;

	return false;
}

template<>
int CHudItemEffectMeterTemp<C_TFShotgun_Revenge>::GetCount( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFShotgun_Revenge *pRevenge = GetWeapon();
		if ( pRevenge && pRevenge->CanGetRevengeCrits() )
			return pPlayer->m_Shared.GetRevengeCritCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// C_TFBuffItem Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFBuffItem>::ShouldFlash( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	if ( pPlayer->m_Shared.GetRageProgress() >= 100.0f )
		return true;

	return pPlayer->m_Shared.IsRageActive();
}

//-----------------------------------------------------------------------------
// C_TFSniperRifle_Decap Specialization
//-----------------------------------------------------------------------------
template<>
int CHudItemEffectMeterTemp<C_TFSniperRifle_Decap>::GetCount( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFSniperRifle_Decap *pSniperRifle = GetWeapon();
		if ( pSniperRifle )
			return pPlayer->m_Shared.GetHeadshotCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// C_TFSniperRifle Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFSniperRifle>::IsEnabled( void )
{
	if ( GetWeapon() )
	{
		C_TFSniperRifle *pSniperRifle = GetWeapon();
		if ( pSniperRifle && pSniperRifle->HasFocus() )
			return true;
	}

	return false;
}

template<>
float CHudItemEffectMeterTemp<C_TFSniperRifle>::GetProgress( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		return pPlayer->m_Shared.GetFocusLevel() / 100.0;
	}

	return 0.0;
}

//-----------------------------------------------------------------------------
// C_TFRocketLauncher_Airstrike Specialization
//-----------------------------------------------------------------------------
template<>
int CHudItemEffectMeterTemp<C_TFRocketLauncher_Airstrike>::GetCount( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFRocketLauncher_Airstrike *pRocketLauncher = GetWeapon();
		if ( pRocketLauncher )
			return pPlayer->m_Shared.GetStrikeCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// C_TFRevolver Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFRevolver>::IsEnabled( void )
{
	if ( GetWeapon() )
	{
		C_TFRevolver *pRevolver = GetWeapon();
		if ( pRevolver && pRevolver->HasSapperCrits() )
			return true;
	}

	return false;
}

template<>
int CHudItemEffectMeterTemp<C_TFRevolver>::GetCount( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFRevolver *pRevolver = GetWeapon();
		if ( pRevolver && pRevolver->HasSapperCrits() )
			return pPlayer->m_Shared.GetSapperKillCount();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// C_TFPepBrawlBlaster Specialization
//-----------------------------------------------------------------------------
template<>
float CHudItemEffectMeterTemp<C_TFPepBrawlBlaster>::GetProgress( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && IsEnabled() )
		return pPlayer->m_Shared.GetHypeMeter() / 100;

	return 1.0f;
}


//-----------------------------------------------------------------------------
// C_TFSodaPopper Specialization
//-----------------------------------------------------------------------------
template<>
float CHudItemEffectMeterTemp<C_TFSodaPopper>::GetProgress( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && IsEnabled() )
		return pPlayer->m_Shared.GetHypeMeter() / 100;

	return 1.0f;
}

//-----------------------------------------------------------------------------
// C_TFParticleCannon Specialization
//-----------------------------------------------------------------------------
template<>
float CHudItemEffectMeterTemp<C_TFParticleCannon>::GetProgress( void )
{
	C_TFParticleCannon *pParticleCannon = GetWeapon();
	if ( pParticleCannon )
		return pParticleCannon->GetEnergyPercentage();

	return 1.0f;
}

//-----------------------------------------------------------------------------
// C_TFRaygun Specialization
//-----------------------------------------------------------------------------
template<>
float CHudItemEffectMeterTemp<C_TFRaygun>::GetProgress( void )
{
	C_TFRaygun *pRayGun = GetWeapon();
	if ( pRayGun )
		return pRayGun->GetEnergyPercentage();

	return 1.0f;
}

//-----------------------------------------------------------------------------
// C_TFDRGPomson Specialization
//-----------------------------------------------------------------------------
template<>
float CHudItemEffectMeterTemp<C_TFDRGPomson>::GetProgress( void )
{
	C_TFDRGPomson *pPomson = GetWeapon();
	if ( pPomson )
		return pPomson->GetEnergyPercentage();

	return 1.0f;
}


//-----------------------------------------------------------------------------
// C_TFFlameThrower Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFFlameThrower>::IsEnabled( void )
{
	if ( GetWeapon() )
	{
		C_TFFlameThrower *pFlamethrower = GetWeapon();
		if ( pFlamethrower && pFlamethrower->HasMmmph() )
			return true;
	}

	return false;
}

template<>
float CHudItemEffectMeterTemp<C_TFFlameThrower>::GetProgress( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && IsEnabled() )
		return pPlayer->m_Shared.GetFireRage() / 100;

	return 1.0f;
}

//-----------------------------------------------------------------------------
// C_TFSMG_Charged Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFSMG_Charged>::IsEnabled( void )
{
	if ( tf2v_use_new_cleaners.GetBool() )
		return true;

	return false;
}

template<>
bool CHudItemEffectMeterTemp<C_TFSMG_Charged>::ShouldBeep( void )
{
	if ( tf2v_use_new_cleaners.GetBool() )
		return true;

	return false;
}

template<>
bool CHudItemEffectMeterTemp<C_TFSMG_Charged>::ShouldFlash( void )
{
	if ( tf2v_use_new_cleaners.GetBool() )
		return true;

	return false;
}

template<>
float CHudItemEffectMeterTemp<C_TFSMG_Charged>::GetProgress( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && IsEnabled() )
		return pPlayer->m_Shared.GetCrikeyMeter() / 100;

	return 1.0f;
}

//-----------------------------------------------------------------------------
// C_TFKnife Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFKnife>::IsEnabled( void )
{
	if ( GetWeapon() )
	{
		C_TFKnife *pKnife = GetWeapon();
		if ( pKnife && pKnife->CanExtinguish() )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// C_TFLunchBox Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFLunchBox>::IsEnabled( void )
{
	if ( GetWeapon() )
	{
		C_TFLunchBox *pFood = GetWeapon();
		if ( !tf2v_new_chocolate_behavior.GetBool() && pFood->IsChocolateOrFishcake() )
			return false;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// C_TFPowerupBottle Specialization
//-----------------------------------------------------------------------------
template<>
C_TFPowerupBottle *CHudItemEffectMeterTemp<C_TFPowerupBottle>::GetWeapon( void )
{
	if ( GetItem() == NULL )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			for ( int i = 0; i < pPlayer->GetNumWearables(); ++i )
			{
				C_TFPowerupBottle *pBottle = dynamic_cast<C_TFPowerupBottle *>( pPlayer->GetWearable( i ) );
				if ( pBottle )
				{
					SetItem( pBottle );
					break;
				}
			}
		}

		if ( !GetItem() )
			m_bEnabled = false;
	}

	return static_cast<C_TFPowerupBottle *>( GetItem() );
}

template <>
bool CHudItemEffectMeterTemp<C_TFPowerupBottle>::IsEnabled( void )
{
	CTFPowerupBottle *pPowerupBottle = GetWeapon();
	if ( pPowerupBottle )
		return ( pPowerupBottle->GetNumCharges() > 0 );

	return false;
}

template <>
int CHudItemEffectMeterTemp<C_TFPowerupBottle>::GetCount( void )
{
	CTFPowerupBottle *pPowerupBottle = GetWeapon();
	if ( pPowerupBottle )
		return pPowerupBottle->GetNumCharges();
	
	return 0;
}


//-----------------------------------------------------------------------------
// Killstreak Specialization
//-----------------------------------------------------------------------------
bool CHudItemEffectMeterKillstreak::IsEnabled( void )
{
	if ( tf2v_show_killstreak_counter.GetBool() )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
			return false;

		return true;
	}

	return false;
}

int CHudItemEffectMeterKillstreak::GetCount( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
		return pPlayer->m_Shared.GetKillstreakCount();

	return -1;
}

#define NEW_WEAPON_METER(class, beeps, resource) \
	new CHudItemEffectMeterTemp< class >( "HudItemEffectMeter", beeps, resource )

CHudItemEffects::CHudItemEffects()
	: CAutoGameSystemPerFrame( "HudItemEffects" )
{
}

CHudItemEffects::~CHudItemEffects()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemEffects::Init( void )
{
	ListenForGameEvent( "localplayer_respawn" );
	ListenForGameEvent( "post_inventory_application" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffects::Shutdown( void )
{
	for ( int i = 0; i < m_pEffectBars.Count(); ++i )
	{
		if ( m_pEffectBars[i] )
			delete m_pEffectBars[i].Get();
	}

	StopListeningForAllEvents();
	m_pEffectBars.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffects::Update( float frametime )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	for ( int i = 0; i < m_pEffectBars.Count(); ++i )
	{
		CHudItemEffectMeter *pMeter = m_pEffectBars[i];
		if ( pMeter )
			pMeter->Update( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup elements on player setup
//-----------------------------------------------------------------------------
void CHudItemEffects::FireGameEvent( IGameEvent *event )
{
	if ( !Q_strcmp( event->GetName(), "post_inventory_application" ) )
	{
		int iUserID = event->GetInt( "userid" );
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pPlayer && pPlayer->GetUserID() == iUserID )
		{
			SetPlayer();
		}
	}
	else if ( !Q_strcmp( event->GetName(), "localplayer_respawn" ) )
	{
		SetPlayer();
	}
}

void CHudItemEffects::SetPlayer( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
	{
		return;
	}

	for ( int i = 0; i < m_pEffectBars.Count(); ++i )
	{
		if ( m_pEffectBars[i] )
			delete m_pEffectBars[i].Get();
	}
	m_pEffectBars.RemoveAll();

	AddItemMeter( new CHudItemEffectMeterKillstreak( "HudItemEffectMeter" ) );
	AddItemMeter( new CHudItemEffectMeterTemp<C_TFPowerupBottle>( "HudItemEffectMeter", false, "resource/UI/HudItemEffectMeter_PowerupBottle.res" ) );
	switch ( pPlayer->GetPlayerClass()->GetClassIndex() )
	{
		case TF_CLASS_SCOUT:
			AddItemMeter( NEW_WEAPON_METER( C_TFBat_Wood, true, NULL ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFBat_Giftwrap, true, NULL ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFLunchBox_Drink, true, "resource/UI/HudItemEffectMeter_Scout.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFJarMilk, true, "resource/UI/HudItemEffectMeter_Scout.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFPepBrawlBlaster, true, "resource/UI/HudItemEffectMeter_SodaPopper.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFSodaPopper, true, "resource/UI/HudItemEffectMeter_SodaPopper.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFCleaver, true, "resource/UI/HudItemEffectMeter_Cleaver.res" ) );
			break;
		case TF_CLASS_SNIPER:
			AddItemMeter( NEW_WEAPON_METER( C_TFJar, true, NULL ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFSniperRifle_Decap, false, "resource/UI/HudItemEffectMeter_Sniper.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFSniperRifle, true, "resource/UI/HudItemEffectMeter_Sniperfocus.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFSMG_Charged, false, NULL ) );
			break;
		case TF_CLASS_SOLDIER:
			AddItemMeter( NEW_WEAPON_METER( C_TFBuffItem, true, NULL ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFRocketLauncher_Airstrike, false, "resource/UI/HudItemEffectMeter_Demoman.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFParticleCannon, false, "resource/UI/HudItemEffectMeter_particlecannon.res" ) );
			break;
		case TF_CLASS_DEMOMAN:
			AddItemMeter( NEW_WEAPON_METER( C_TFSword, false, "resource/UI/HudItemEffectMeter_Demoman.res" ) );
			break;
		case TF_CLASS_HEAVYWEAPONS:
			AddItemMeter( NEW_WEAPON_METER( C_TFLunchBox, true, NULL ) );
			break;
		case TF_CLASS_PYRO:
			AddItemMeter( NEW_WEAPON_METER( C_TFFlareGun_Revenge, false, "resource/UI/HudItemEffectMeter_Engineer.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFFlameThrower, true, NULL ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFRaygun, false, "resource/UI/HudItemEffectMeter_raygun.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFJarGas, true, "resource/UI/HudItemEffectMeter_Pyro.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFRocketPack, true, "resource/UI/HudRocketPack.res" ) );
			break;
		case TF_CLASS_SPY:
			AddItemMeter( new CHudItemEffectMeter( "HudItemEffectMeter" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFRevolver, false, "resource/UI/HudItemEffectMeter_Spy.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFKnife, true, "resource/UI/huditemeffectmeter_spyknife.res" ) );
			break;
		case TF_CLASS_ENGINEER:
			AddItemMeter( NEW_WEAPON_METER( C_TFShotgun_Revenge, false, "resource/UI/HudItemEffectMeter_Engineer.res" ) );
			AddItemMeter( NEW_WEAPON_METER( C_TFDRGPomson, false, "resource/UI/HudItemEffectMeter_pomson.res" ) );
			break;
		default:
			break;
	}
}

static CHudItemEffects g_sItemMeters;
