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
class CHudItemEffectMeter : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudItemEffectMeter, EditablePanel );

public:
	CHudItemEffectMeter( Panel *pParent, const char *pElementName );

	virtual void    ApplySchemeSettings( IScheme *scheme );
	virtual void    PerformLayout( void );
	virtual bool	ShouldDraw( void );
	
	virtual void    Update( C_TFPlayer *pPlayer );

	virtual int     GetCount( void ) const              { return -1; }
	virtual float   GetProgress( void ) const;
	virtual const char* GetLabelText( void ) const      { return "#TF_Cloak"; }
	virtual const char* GetResourceName( void ) const   { return "resource/UI/HudItemEffectMeter.res"; }
	virtual bool    ShouldBeep( void ) const;
	virtual bool    ShouldFlash( void ) const           { return false; }
	virtual bool    IsEnabled( void ) override          { return m_bEnabled; }

	virtual C_BaseEntity* GetItem( void ) const         { return m_hItem.Get(); }
	void            SetItem( C_BaseEntity *pItem ) const{ const_cast<EHANDLE &>( m_hItem ) = pItem; }

private:
	ContinuousProgressBar *m_pEffectMeter;
	CExLabel *m_pEffectMeterLabel;

	CPanelAnimationVarAliasType( int, m_iXOffset, "x_offset", "0", "proportional_float" );

protected:
	bool m_bEnabled;
	EHANDLE m_hItem;
	float m_flOldCharge;
};


template<typename Class>
class CHudItemEffectMeterTemp : public CHudItemEffectMeter
{
	DECLARE_CLASS( CHudItemEffectMeterTemp<Class>, CHudItemEffectMeter );
public:
	CHudItemEffectMeterTemp( Panel *pParent, const char *pElementName )
		: CHudItemEffectMeter( pParent, pElementName )
	{
	}

	virtual void    Update( C_TFPlayer *pPlayer ) override;

	virtual int     GetCount( void ) const { return -1; }
	virtual float   GetProgress( void ) const
	{
		return CHudItemEffectMeter::GetProgress();
	}
	virtual const char* GetLabelText( void ) const
	{
		return CHudItemEffectMeter::GetLabelText();
	}
	virtual const char* GetResourceName( void ) const
	{
		return CHudItemEffectMeter::GetResourceName();
	}
	virtual bool    ShouldBeep( void ) const
	{
		return CHudItemEffectMeter::ShouldBeep();
	}
	virtual bool    ShouldFlash( void ) const { return false; }
	virtual bool    IsEnabled( void ) override;

	virtual Class*  GetWeapon( void ) const { return nullptr; }
};


class CHudItemEffects : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudItemEffects, EditablePanel );
public:
	CHudItemEffects( const char *pElementName );
	~CHudItemEffects();

	virtual bool ShouldDraw( void );
	virtual void OnTick( void );
	virtual void ApplySchemeSettings( IScheme *scheme );

	virtual void FireGameEvent( IGameEvent *event );

	inline void AddItemMeter( CHudItemEffectMeter *pMeter )
	{
		DHANDLE<CHudItemEffectMeter> hndl;
		hndl = pMeter;
		if (hndl)
		{
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
CHudItemEffectMeter::CHudItemEffectMeter( Panel *pParent, const char *pElementName )
	: BaseClass( pParent, pElementName )
{
	m_pEffectMeter = new ContinuousProgressBar( this, "ItemEffectMeter" );
	m_pEffectMeterLabel = new CExLabel( this, "ItemEffectMeterLabel", "" );

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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::PerformLayout( void )
{
	if (m_pEffectMeterLabel)
	{
		wchar_t *pszLocalized = g_pVGuiLocalize->Find( GetLabelText() );
		if (pszLocalized)
			m_pEffectMeterLabel->SetText( pszLocalized );
		else
			m_pEffectMeterLabel->SetText( GetLabelText() );
	}

	int iNumPanels = 0;
	CHudItemEffectMeter *pLastMeter = nullptr;
	for (int i=0; i<CHudItemEffects::m_pEffectBars.Count(); ++i)
	{
		CHudItemEffectMeter *pMeter = CHudItemEffects::m_pEffectBars[i];
		if (pMeter)
		{
			iNumPanels += (int)pMeter->IsEnabled();
			pLastMeter = pMeter;
		}
	}

	if (iNumPanels > 1 && pLastMeter)
	{
		int x = 0, y = 0;
		GetPos( x, y );
		SetPos( ( x - m_iXOffset * ( iNumPanels - 1 ) ), y );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemEffectMeter::ShouldDraw( void )
{
	// Investigate for optimzations that may have happened to this logic
	/*bool bResult = false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pPlayer && pPlayer->IsAlive() && IsEnabled())
	{
		bResult = CHudElement::ShouldDraw();
		if (bResult ^ IsVisible())
		{
			SetVisible( bResult );
			if (bResult)
				InvalidateLayout( false, true );
		}
	}*/

	return true; //bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffectMeter::Update( C_TFPlayer *pPlayer )
{
	if (!pPlayer || !pPlayer->IsAlive())
	{
		m_flOldCharge = 1.0f;
		return;
	}

	if (m_pEffectMeter && IsEnabled())
	{
		if (!IsVisible())
		{
			SetVisible( true );
			m_pEffectMeter->SetFgColor( COLOR_WHITE );
		}

		if (GetCount() >= 0)
			SetDialogVariable( "progresscount", GetCount() );

		float flCharge = GetProgress();
		m_pEffectMeter->SetProgress( flCharge );

		// Play a ding when full charged.
		if (m_flOldCharge < 1.0f && flCharge >= 1.0f && ShouldBeep())
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "TFPlayer.Recharged" );
		}

		if (ShouldFlash())
		{
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
float CHudItemEffectMeter::GetProgress( void ) const
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pPlayer && pPlayer->IsAlive())
		return pPlayer->m_Shared.GetShieldChargeMeter() / 100.0f;

	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemEffectMeter::ShouldBeep( void ) const
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pPlayer && pPlayer->IsAlive())
	{
		C_TFWeaponInvis *pWatch = static_cast<C_TFWeaponInvis *>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
		if (pWatch && !pWatch->HasFeignDeath())
			return false;
	}

	return true;
}


template<typename Class>
void CHudItemEffectMeterTemp<Class>::Update( C_TFPlayer *pPlayer )
{
	CHudItemEffectMeter::Update( pPlayer );
}

template<typename Class>
bool CHudItemEffectMeterTemp<Class>::IsEnabled( void )
{
	if (GetWeapon())
		return true;

	return CHudItemEffectMeter::IsEnabled();
}


template<>
C_TFSword *CHudItemEffectMeterTemp<C_TFSword>::GetWeapon( void ) const
{
	if (!GetItem())
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if (pPlayer)
			SetItem( dynamic_cast<C_TFSword *>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_SWORD ) ) );
	}

	return static_cast<C_TFSword *>( GetItem() );
}

template<>
bool CHudItemEffectMeterTemp<C_TFSword>::IsEnabled( void )
{
	if (GetWeapon())
		return true;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pPlayer)
	{
		if (pPlayer->m_Shared.HasDemoShieldEquipped())
			return true;
	}

	return CHudItemEffectMeter::IsEnabled();
}

template<>
const char *CHudItemEffectMeterTemp<C_TFSword>::GetResourceName( void ) const
{
	return "resource/UI/HudItemEffectMeter_Demoman.res";
}

template<>
const char *CHudItemEffectMeterTemp<C_TFSword>::GetLabelText( void ) const
{
	C_TFSword *pSword = GetWeapon();

	return pSword ? pSword->GetEffectLabelText() : "";
}

template<>
int CHudItemEffectMeterTemp<C_TFSword>::GetCount( void ) const
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (pPlayer)
	{
		C_TFSword *pSword = GetWeapon();
		if (pSword && pSword->CanDecapitate())
			return pPlayer->m_Shared.GetDecapitationCount();
	}

	return -1;
}


template<>
C_TFBat_Wood *CHudItemEffectMeterTemp<C_TFBat_Wood>::GetWeapon( void ) const
{
	if (!GetItem())
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if (pPlayer)
			SetItem( dynamic_cast<C_TFBat_Wood *>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_BAT_WOOD ) ) );
	}

	return static_cast<C_TFBat_Wood *>( GetItem() );
}

template<>
const char *CHudItemEffectMeterTemp<C_TFBat_Wood>::GetLabelText( void ) const
{
	C_TFBat_Wood *pBat = GetWeapon();

	return pBat ? pBat->GetEffectLabelText() : "";
}

template<>
float CHudItemEffectMeterTemp<C_TFBat_Wood>::GetProgress( void ) const
{
	C_TFBat_Wood *pBat = GetWeapon();

	return pBat ? pBat->GetEffectBarProgress() : 0.0f;
}


template<>
C_TFLunchBox_Drink *CHudItemEffectMeterTemp<C_TFLunchBox_Drink>::GetWeapon( void ) const
{
	if (!GetItem())
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if (pPlayer)
			SetItem( dynamic_cast<C_TFLunchBox_Drink *>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX_DRINK ) ) );
	}

	return static_cast<C_TFLunchBox_Drink *>( GetItem() );
}

template<>
const char *CHudItemEffectMeterTemp<C_TFLunchBox_Drink>::GetResourceName( void ) const
{
	return "resource/UI/HudItemEffectMeter_Scout.res";
}

template<>
const char *CHudItemEffectMeterTemp<C_TFLunchBox_Drink>::GetLabelText( void ) const
{
	C_TFLunchBox_Drink *pDrink = GetWeapon();

	return pDrink ? pDrink->GetEffectLabelText() : "";
}

template<>
float CHudItemEffectMeterTemp<C_TFLunchBox_Drink>::GetProgress( void ) const
{
	C_TFLunchBox_Drink *pDrink = GetWeapon();

	return pDrink ? pDrink->GetEffectBarProgress() : 0.0f;
}



DECLARE_HUDELEMENT( CHudItemEffects );

CHudItemEffects::CHudItemEffects( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudItemEffects" )
{
	SetParent( g_pClientMode->GetViewport() );

	// Create effect bars for primary, secondary, melee and pda slots.
	/*for (int i = 0; i < TF_PLAYER_WEAPON_COUNT; i++)
	{
		CHudItemEffectMeter *pMeter = new CHudItemEffectMeter( this, "HudItemEffectMeter" );
		pMeter->SetSlot( i );
		m_pEffectBars.AddToTail( pMeter );
	}*/

	ListenForGameEvent( "post_inventory_application" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

CHudItemEffects::~CHudItemEffects()
{
	for (int i=0; i<m_pEffectBars.Count(); ++i)
	{
		if (m_pEffectBars[i])
			delete m_pEffectBars[i].Get();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffects::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudItemEffects::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer || !pPlayer->IsAlive())
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudItemEffects::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	bool bUpdateLayout = false;
	for (int i = 0; i < m_pEffectBars.Count(); i++)
	{
		CHudItemEffectMeter *pMeter = m_pEffectBars[i];
		if (pMeter)
		{
			bool bWasVisible = pMeter->IsVisible();
			pMeter->Update( pPlayer );
			bool bVisible = pMeter->IsVisible();

			if (bVisible != bWasVisible)
			{
				bUpdateLayout = true;
			}
		}
	}

	if (bUpdateLayout)
	{
		InvalidateLayout( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup elements on player setup
//-----------------------------------------------------------------------------
void CHudItemEffects::FireGameEvent( IGameEvent *event )
{
	if (!Q_strcmp( event->GetName(), "post_inventory_application" ))
	{
		int iUserID = event->GetInt( "userid" );
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

		if (pPlayer && pPlayer->GetUserID() == iUserID)
		{
			for (int i=0; i<m_pEffectBars.Count(); ++i)
			{
				if (m_pEffectBars[i])
					delete m_pEffectBars[i].Get();
			}

			switch (pPlayer->GetPlayerClass()->GetClassIndex())
			{
				case TF_CLASS_SCOUT:
					AddItemMeter( new CHudItemEffectMeterTemp<C_TFBat_Wood>( this, "HudItemEffectMeter" ) );
					AddItemMeter( new CHudItemEffectMeterTemp<C_TFLunchBox_Drink>( this, "HudItemEffectMeter" ) );
					break;
				case TF_CLASS_DEMOMAN:
					AddItemMeter( new CHudItemEffectMeterTemp<C_TFSword>( this, "HudItemEffectMeter" ) );
					break;
				default:
					break;
			}
		}
	}
}
