//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
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
	virtual const char* GetLabelText( void )            { return "#TF_Cloak"; }
	virtual const char* GetResourceName( void )         { return "resource/UI/HudItemEffectMeter.res"; }
	virtual bool    ShouldBeep( void );
	virtual bool    ShouldFlash( void )                 { return false; }
	virtual bool    IsEnabled( void ) override          { return m_bEnabled; }

	virtual C_TFWeaponBase* GetItem( void ) const       { return m_hItem; }
	void            SetItem( C_TFWeaponBase *pItem )    { m_hItem = pItem; }

private:
	ContinuousProgressBar *m_pEffectMeter;
	CExLabel *m_pEffectMeterLabel;

	CPanelAnimationVarAliasType( int, m_iXOffset, "x_offset", "0", "proportional_int" );

	CHandle<C_TFWeaponBase> m_hItem;
	float m_flOldCharge;

protected:
	bool m_bEnabled;
};


template<typename Class>
class CHudItemEffectMeterTemp : public CHudItemEffectMeter
{
	DECLARE_CLASS( CHudItemEffectMeterTemp<Class>, CHudItemEffectMeter );
public:
	CHudItemEffectMeterTemp( const char *pElementName, const char *pResourceName = nullptr )
		: CHudItemEffectMeter( pElementName ), m_pszResource( pResourceName )
	{
	}

	virtual int     GetCount( void )          { return -1; }
	virtual float   GetProgress( void );
	virtual const char* GetLabelText( void );
	virtual const char* GetResourceName( void );
	virtual bool    ShouldBeep( void );
	virtual bool    ShouldFlash( void )       { return false; }
	virtual bool    IsEnabled( void );

	virtual Class*  GetWeapon( void );

private:
	const char *m_pszResource;
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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::PerformLayout( void )
{
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
	// Investigate for optimzations that may have happened to this logic
	bool bResult = false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsAlive() && IsEnabled() )
	{
		bResult = CHudElement::ShouldDraw();
	}

	if ( bResult ^ IsVisible() )
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
			int r = 10 * ( (int)( gpGlobals->realtime * 10.0f ) % 10 ) - 96;
			m_pEffectMeter->SetFgColor( Color( r, 0, 0, 255 ) );
		}
		else
		{
			m_pEffectMeter->SetFgColor( COLOR_WHITE );
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
	Assert( dynamic_cast<C_TFWeaponBase *>( GetWeapon() ) );

	C_TFWeaponBase *pWeapon = GetWeapon();
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
	Assert( dynamic_cast<C_TFWeaponBase *>( GetWeapon() ) );

	C_TFWeaponBase *pWeapon = GetWeapon();
	if ( pWeapon )
		return pWeapon->GetEffectLabelText();

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
bool CHudItemEffectMeterTemp<Class>::ShouldBeep( void )
{
	return CHudItemEffectMeter::ShouldBeep();
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
			return pRevenge->GetCount();
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
			return pPlayer->m_Shared.GetKillstreakCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// C_TFRevolver_Dex Specialization
//-----------------------------------------------------------------------------
template<>
int CHudItemEffectMeterTemp<C_TFRevolver_Dex>::GetCount( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFRevolver_Dex *pRevenge = GetWeapon();
		if ( pRevenge )
			return pPlayer->m_Shared.GetSapperKillCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// C_TFPepBrawlBlaster Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFPepBrawlBlaster>::IsEnabled( void )
{
	if ( GetWeapon() )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// C_TFSodaPopper Specialization
//-----------------------------------------------------------------------------
template<>
bool CHudItemEffectMeterTemp<C_TFSodaPopper>::IsEnabled( void )
{
	if ( GetWeapon() )
		return true;

	return false;
}



CHudItemEffects::CHudItemEffects()
	: CAutoGameSystemPerFrame( "CHudItemEffects" )
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

	switch ( pPlayer->GetPlayerClass()->GetClassIndex() )
	{
		case TF_CLASS_SCOUT:
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFBat_Wood>( "HudItemEffectMeter" ) );
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFLunchBox_Drink>( "HudItemEffectMeter", "resource/UI/HudItemEffectMeter_Scout.res" ) );
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFJarMilk>( "HudItemEffectMeter", "resource/UI/HudItemEffectMeter_Scout.res" ) );
			AddItemMeter(new CHudItemEffectMeterTemp<C_TFPepBrawlBlaster>("HudItemEffectMeter", "resource/UI/HudItemEffectMeter_sodapopper.res") );
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFSodaPopper>( "HudItemEffectMeter", "resource/UI/HudItemEffectMeter_sodapopper.res" ) );
			break;
		case TF_CLASS_SNIPER:
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFJar>( "HudItemEffectMeter" ) );
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFSniperRifle_Decap>( "HudItemEffectMeter", "resource/UI/HudItemEfectMeter_Sniper.res" ) );
			break;
		case TF_CLASS_SOLDIER:
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFBuffItem>( "HudItemEffectMeter" ) );
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFRocketLauncher_Airstrike>( "HudItemEffectMeter" ) );
			break;
		case TF_CLASS_DEMOMAN:
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFSword>( "HudItemEffectMeter", "resource/UI/HudItemEffectMeter_Demoman.res" ) );
			break;
		case TF_CLASS_HEAVYWEAPONS:
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFLunchBox>( "HudItemEffectMeter" ) );
			break;
		case TF_CLASS_SPY:
			AddItemMeter( new CHudItemEffectMeter( "HudItemEffectMeter" ) );
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFRevolver_Dex>( "HudItemEffectMeter", "resource/UI/HudItemEffectMeter_Spy.res" ) );
			break;
		case TF_CLASS_ENGINEER:
			AddItemMeter( new CHudItemEffectMeterTemp<C_TFShotgun_Revenge>( "HudItemEffectMeter", "resource/UI/HudItemEffectMeter_Engineer.res" ) );
			break;
		default:
			break;
	}
}

static CHudItemEffects g_pItemMeters;
