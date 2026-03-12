//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_CONFIRM_DIALOG_H
#define TF_CONFIRM_DIALOG_H

#include <vgui_controls/EditablePanel.h>
#include "inputsystem/InputEnums.h"

class CExButton;
class CTFSpectatorGUIHealth;

//-----------------------------------------------------------------------------
class CConfirmDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CConfirmDialog, vgui::EditablePanel );
public:
	CConfirmDialog( vgui::Panel *pParent );
	virtual ~CConfirmDialog() { }

	virtual void			ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void			OnCommand( char const *pszCommand ) OVERRIDE;
	virtual void			OnKeyCodePressed( ButtonCode_t button ) OVERRIDE;
	virtual void			OnKeyCodeTyped( ButtonCode_t button ) OVERRIDE;
	virtual void			OnSizeChanged( int nNewWide, int nNewTall ) OVERRIDE;
	virtual void			PerformLayout( void ) OVERRIDE;

	virtual wchar_t const *	GetText( void ) = 0;

	virtual GameActionSet_t GetPreferredActionSet( void ) const { return GAME_ACTION_SET_MENUCONTROLS; }
	virtual char const *	GetResFile( void );
	virtual GameActionSet_t GetActionSet( void ) const { return GAME_ACTION_SET_MENUCONTROLS; }
	virtual char const *	GetCancelActionName( void ) const { return NULL; }
	virtual char const *	GetConfirmActionName( void ) const { return NULL; }

	void					FinishUp( void );
	void					Show( bool );
	void					SetIconImage( char const *pszIcon );

protected:
	CExButton *m_pConfirmButton;
	CExButton *m_pCancelButton;
	vgui::ImagePanel *m_pIcon;
};

//-----------------------------------------------------------------------------
class CTFGenericConfirmDialog : public CConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CTFGenericConfirmDialog, CConfirmDialog );
public:
	CTFGenericConfirmDialog( const char *pszTitle, const char *pszTextKey, const char *pszConfirmBtnText,
							 const char *pszCancelBtnText, void (*callback)( bool, void * ), vgui::Panel *pParent );
	CTFGenericConfirmDialog( const char *pszTitle, const wchar_t *pszText, const char *pszConfirmBtnText,
							 const char *pszCancelBtnText, void (*callback)( bool, void * ), vgui::Panel *pParent );
	virtual ~CTFGenericConfirmDialog();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void	OnCommand( const char *pszCommand ) OVERRIDE;
	virtual void	PerformLayout( void ) OVERRIDE;

	virtual const wchar_t *GetText();

	void			AddStringToken( const char *pToken, const wchar_t *pValue );
	void			SetContext( void *pContext );
	void			SetKeyValues( KeyValues *pKeyValues );

private:
	const char *m_pszTitle;
	const char *m_pszTextKey;
	const char *m_pszConfirmText;
	const char *m_pszCancelText;

	KeyValues *m_pKeyValues;
	wchar_t m_wszBuffer[1024];
	void (*m_pCallback)( bool, void * );
	void *m_pContext;
};

CTFGenericConfirmDialog *ShowConfirmDialog( const char *pTitle, const char *pText, const char *pConfirmBtnText, 
											const char *pCancelBtnText, void (*callback)( bool, void * ), 
											vgui::Panel *parent = NULL, void *pContext = NULL, const char *pSound = NULL );

//-----------------------------------------------------------------------------
class CTFMessageBoxDialog : public CTFGenericConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CTFMessageBoxDialog, CTFGenericConfirmDialog );
public:
	CTFMessageBoxDialog( const char *pszTitle, const char *pszText, const char *pszConfirmBtnText, void (*callback)( bool, void * ), vgui::Panel *pParent )
		: BaseClass( pszTitle, pszText, pszConfirmBtnText, NULL, callback, pParent ) {}
	CTFMessageBoxDialog( const char *pszTitle, const wchar_t *pszText, const char *pszConfirmBtnText, void (*callback)( bool, void * ), vgui::Panel *pParent )
		: BaseClass( pszTitle, pszText, pszConfirmBtnText, NULL, callback, pParent ) {}
	virtual ~CTFMessageBoxDialog() { }

	virtual const char *GetResFile() OVERRIDE;
};


CTFMessageBoxDialog *ShowMessageBox( const char *pszTitle, const char *pszText, const char *pszConfirmBtnText = "#GameUI_OK", 
									 void (*callback)( bool, void * ) = NULL, vgui::Panel *pParent = NULL, void *pContext = NULL );
CTFMessageBoxDialog *ShowMessageBox( const char *pszTitle, const wchar_t *pszText, const char *pszConfirmBtnText = "#GameUI_OK", 
									 void (*callback)( bool, void * ) = NULL, vgui::Panel *pParent = NULL, void *pContext = NULL );
CTFMessageBoxDialog *ShowMessageBox( const char *pszTitle, const char *pszText, KeyValues *pKeyValues, const char *pszConfirmBtnText = "#GameUI_OK", 
									 void (*callback)( bool, void * ) = NULL, vgui::Panel *pParent = NULL, void *pContext = NULL );

//-----------------------------------------------------------------------------
class CTFReviveDialog : public CTFMessageBoxDialog
{
	DECLARE_CLASS_SIMPLE( CTFReviveDialog, CTFMessageBoxDialog );
public:
	CTFReviveDialog( const char *pszTitle, const char *pszText, const char *pszConfirmBtnText, void (*callback)( bool, void * ), vgui::Panel *pParent );
	virtual ~CTFReviveDialog() { }

	virtual void OnTick() OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	virtual const char *GetResFile() OVERRIDE { return "Resource/UI/ReviveDialog.res"; }
	void SetOwner( CBaseEntity *pEntity );

	CTFSpectatorGUIHealth *m_pTargetHealth;
	CHandle< C_BaseEntity >	m_hEntity;
	float m_flPrevHealth;
};


CTFReviveDialog *ShowRevivePrompt( CBaseEntity *pOwner, const char *pszTitle = "#TF_Prompt_Revive_Title",  const char *pszText = "#TF_Prompt_Revive_Message",
								   const char *pszConfirmBtnText = "#TF_Prompt_Revive_Cancel", void (*callback)( bool, void * ) = NULL,
								   vgui::Panel *pParent = NULL, void *pContext = NULL );
#endif