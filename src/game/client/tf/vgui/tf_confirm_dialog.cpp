//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_confirm_dialog.h"
#include "tf_controls.h"
#include "tf_modalstack.h"
#include "tf_spectatorgui.h"
#include <vgui_controls/CheckButton.h>
#include "ienginevgui.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmDialog::CConfirmDialog( vgui::Panel *parent )
	: BaseClass( parent, "ConfirmDialog" )
{
	m_pCancelButton = NULL;
	m_pConfirmButton = NULL;
	m_pIcon = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFile(), "GAME" );

	SetBorder( pScheme->GetBorder( "EconItemBorder" ) );
	SetDialogVariable( "text", GetText() );

	m_pConfirmButton = FindControl<CExButton>( "ConfirmButton" );
	m_pCancelButton = FindControl<CExButton>( "CancelButton" );
	m_pIcon = FindControl<vgui::ImagePanel>( "Icon" );
	
	/*if ( input->IsSteamControllerActive() )
	{
		dynamic_cast<CSCHintIcon *>( FindChildByName( "ConfirmButtonHintIcon" ) )->SetAction( GetConfirmActionName(), GetActionSet() );
		dynamic_cast<CSCHintIcon *>( FindChildByName( "CancelButtonHintIcon" ) )->SetAction( GetCancelActionname(), GetActionSet() );
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::OnCommand( char const *pszCommand )
{
	if ( !Q_strnicmp( pszCommand, "cancel", 6 ) )
	{
		FinishUp();
		PostMessage( GetParent(), new KeyValues( "ConfirmDlgResult", "confirmed", 0 ) );
	}
	else if ( !Q_strnicmp( pszCommand, "confirm", 7 ) )
	{
		FinishUp();
		PostMessage( GetParent(), new KeyValues( "ConfirmDlgResult", "confirmed", 1 ) );
	}
	else
	{
		engine->ClientCmd( pszCommand );
	}

	BaseClass::OnCommand( pszCommand );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::OnKeyCodePressed( ButtonCode_t button )
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( button );
	if ( nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_F2 || nButtonCode == STEAMCONTROLLER_B )
	{
		OnCommand( "cancel" );
	}
	else if ( nButtonCode == KEY_ENTER || nButtonCode == KEY_SPACE || nButtonCode == KEY_XBUTTON_A || nButtonCode == STEAMCONTROLLER_F1 || nButtonCode == STEAMCONTROLLER_A )
	{
		OnCommand( "confirm" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( button );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::OnKeyCodeTyped( ButtonCode_t button )
{
	if ( button == KEY_ESCAPE )
	{
		OnCommand( "cancel" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( button );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::OnSizeChanged( int nNewWide, int nNewTall )
{
	int nX = 0, nY = 0;

	if ( m_pCancelButton )
	{
		m_pCancelButton->GetPos( nX, nY );
		m_pCancelButton->SetPos( nX, nNewTall - m_pCancelButton->GetTall() - YRES( 15 ) );
	}

	if ( m_pConfirmButton )
	{
		m_pConfirmButton->GetPos( nX, nY );
		m_pConfirmButton->SetPos( nX, nNewTall - m_pConfirmButton->GetTall() - YRES( 15 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::PerformLayout( void )
{
	BaseClass::PerformLayout();

	vgui::Label *pLabel = FindControl<vgui::Label>( "ExplanationLabel" );
	if ( pLabel )
	{
		pLabel->SizeToContents();

		int nTall = pLabel->GetTall();
		int iY = pLabel->GetYPos();
		
		pLabel->SetTall( YRES( ScreenHeight() ) + iY + nTall + m_pConfirmButton->GetTall() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CConfirmDialog::GetResFile( void )
{
	// if ( input->IsSteamControllerActive() )
	//	return "Resource/UI/econ/ConfirmDialog_SC.res";

	return "Resource/UI/econ/ConfirmDialog.res";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::FinishUp( void )
{
	SetVisible( false );
	MarkForDeletion();

	TFModalStack()->PopModal( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::Show( bool bPopup )
{
	SetVisible( true );
	SetKeyBoardInputEnabled( true );
	if ( bPopup ) MakePopup();
	MoveToFront();

	InvalidateLayout( true, true );

	/*if ( input->IsSteamControllerActive() )
		SetMouseInputEnabled( false );
	else*/
		SetMouseInputEnabled( true );

	TFModalStack()->PushModal( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::SetIconImage( const char *pszIcon )
{
	if ( m_pIcon )
	{
		m_pIcon->SetImage( pszIcon );
		m_pIcon->SetVisible( ( pszIcon ? true : false ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog::CTFGenericConfirmDialog( const char *pszTitle, const char *pszTextKey, const char *pszConfirmBtnText, const char *pszCancelBtnText, void(*callback)( bool, void * ), vgui::Panel *pParent )
	: CConfirmDialog( pParent ), m_pszTitle(pszTitle), m_pszTextKey(pszTextKey), m_pszConfirmText(pszConfirmBtnText), m_pszCancelText(pszCancelBtnText), m_pCallback(callback), m_pKeyValues(NULL), m_pContext(NULL)
{
	if ( pParent == NULL )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
		SetScheme( scheme );
		SetProportional( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog::CTFGenericConfirmDialog( const char *pszTitle, const wchar_t *pwszText, const char *pszConfirmBtnText, const char *pszCancelBtnText, void(*callback)( bool, void * ), vgui::Panel *pParent )
	: CConfirmDialog( pParent ), m_pszTitle(pszTitle), m_pszTextKey(NULL), m_pszConfirmText(pszConfirmBtnText), m_pszCancelText(pszCancelBtnText), m_pCallback(callback), m_pKeyValues(NULL), m_pContext(NULL)
{
	if ( pParent == NULL )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
		SetScheme( scheme );
		SetProportional( true );
	}

	V_wcscpy_safe( m_wszBuffer, pwszText );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog::~CTFGenericConfirmDialog()
{
	if ( m_pKeyValues )
	{
		m_pKeyValues->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pConfirmButton && m_pszConfirmText )
	{
		m_pConfirmButton->SetText( m_pszConfirmText );
	}

	if ( m_pCancelButton && m_pszCancelText )
	{
		m_pCancelButton->SetText( m_pszCancelText );
	}

	SetXToRed( m_pConfirmButton );
	SetXToRed( m_pCancelButton );

	CExLabel *pTitle = FindControl<CExLabel>( "TitleLabel" );
	if ( pTitle ) pTitle->SetText( m_pszTitle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::OnCommand( const char *pszCommand )
{
	if ( !V_strnicmp( pszCommand, "cancel", 6 ) || !V_strnicmp( pszCommand, "confirm", 7 ) )
	{
		FinishUp();

		if ( m_pCallback )
		{
			m_pCallback( V_strnicmp( pszCommand, "confirm", 7 ) == 0,
						 m_pContext );
		}

		return;
	}

	BaseClass::OnCommand( pszCommand );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int iWX = 0, iWY = 0, nWWide = 0, nWTall = 0;
	vgui::surface()->GetWorkspaceBounds( iWX, iWY, nWWide, nWTall );

	int nWide = 0, nTall = 0;
	GetSize( nWide, nTall );

	SetPos( iWX + ( nWWide - nWide ) / 2, iWY + ( nWTall - nTall ) / 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const wchar_t *CTFGenericConfirmDialog::GetText()
{
	if ( m_pszTextKey )
		g_pVGuiLocalize->ConstructString( m_wszBuffer, ARRAYSIZE( m_wszBuffer ), m_pszTextKey, m_pKeyValues );

	return m_wszBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::AddStringToken( const char *pszToken, const wchar_t *pwszValue )
{
	if ( !m_pKeyValues )
	{
		m_pKeyValues = new KeyValues( "GenericConfirmDialog" );
	}
	m_pKeyValues->SetWString( pszToken, pwszValue );

	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::SetContext( void *pContext )
{
	m_pContext = pContext;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::SetKeyValues( KeyValues *pKeyValues )
{
	if ( m_pKeyValues )
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = pKeyValues->MakeCopy();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFMessageBoxDialog::GetResFile()
{
	// if ( input->IsSteamControllerActive() )
	//	return "Resource/UI/econ/MessageBoxDialog_SC.res";
	
	return "Resource/UI/econ/MessageBoxDialog.res";
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFReviveDialog::CTFReviveDialog( const char *pszTitle, const char *pszText, const char *pszConfirmBtnText, void(*callback)( bool, void * ), vgui::Panel *pParent )
	: BaseClass( pszTitle, pszText, pszConfirmBtnText, callback, pParent )
{
	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_pTargetHealth->SetAllowAnimations( false );
	m_pTargetHealth->HideHealthBonusImage();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );

	OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveDialog::OnTick()
{
	BaseClass::OnTick();

	if ( !m_pTargetHealth || !m_hEntity )
		return;

	float flHealth = m_hEntity->GetHealth();
	if ( flHealth != m_flPrevHealth )
	{
		float flMaxHealth = m_hEntity->GetMaxHealth();
		m_pTargetHealth->SetHealth( flHealth, flMaxHealth, flMaxHealth );
		m_flPrevHealth = flHealth;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Skip base class
//-----------------------------------------------------------------------------
void CTFReviveDialog::PerformLayout()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveDialog::SetOwner( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		m_hEntity = pEntity;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog *ShowConfirmDialog( const char *pszTitle, const char *pszText, const char *pszConfirmBtnText, 
											const char *pszCancelBtnText, void (*callback)( bool, void * ), vgui::Panel *pParent, 
											void *pContext, const char *pszSound )
{
	CTFGenericConfirmDialog *pDialog = vgui::SETUP_PANEL( new CTFGenericConfirmDialog( pszTitle, pszText, pszConfirmBtnText, pszCancelBtnText, callback, pParent ) );
	if ( pDialog )
	{
		if ( pContext )
			pDialog->SetContext( pContext );

		if ( pszSound && pszSound[0] )
			vgui::surface()->PlaySound( pszSound );

		pDialog->Show( true );
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowMessageBox( const char *pszTitle, const char *pszText, const char *pszConfirmBtnText, 
									 void (*callback)( bool, void * ), vgui::Panel *pParent, void *pContext )
{
	CTFMessageBoxDialog *pDialog = vgui::SETUP_PANEL( new CTFMessageBoxDialog( pszTitle, pszText, pszConfirmBtnText, callback, pParent ) );
	if ( pDialog )
	{
		if ( pContext )
			pDialog->SetContext( pContext );

		pDialog->Show( true );
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowMessageBox( const char *pszTitle, const wchar_t *pszText, const char *pszConfirmBtnText, 
									 void (*callback)( bool, void * ), vgui::Panel *pParent, void *pContext )
{
	CTFMessageBoxDialog *pDialog = vgui::SETUP_PANEL( new CTFMessageBoxDialog( pszTitle, pszText, pszConfirmBtnText, callback, pParent ) );
	if ( pDialog )
	{
		if ( pContext )
			pDialog->SetContext( pContext );

		pDialog->Show( true );
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowMessageBox( const char *pszTitle, const char *pszText, KeyValues *pKeyValues, const char *pConfirmBtnText, 
									 void (*callback)( bool, void * ), vgui::Panel *pParent, void *pContext )
{
	CTFMessageBoxDialog *pDialog = ShowMessageBox( pszTitle, pszText, pConfirmBtnText, callback, pParent, pContext );
	if ( pDialog )
	{
		if ( pKeyValues )
		{
			pDialog->SetKeyValues( pKeyValues );
			pDialog->SetDialogVariable( "text", pDialog->GetText() );
		}
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFReviveDialog *ShowRevivePrompt( CBaseEntity *pOwner, const char *pTitle, const char *pText, const char *pConfirmBtnText,
								   void (*callback)( bool, void * ), vgui::Panel *pParent, void *pContext )
{
	CTFReviveDialog *pDialog = vgui::SETUP_PANEL( new CTFReviveDialog( pTitle, pText, pConfirmBtnText, callback, pParent ) );
	if ( pDialog )
	{
		if ( pContext )
		{
			pDialog->SetContext( pContext );
		}

		pDialog->SetOwner( pOwner );
		pDialog->Show( true );
	}

	return pDialog;
}
