#include "cbase.h"
#include "tf_modalstack.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui/IInput.h>
#include "ienginevgui.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPanelModalStack *TFModalStack( void )
{
    static CPanelModalStack g_ModalStack;
    return &g_ModalStack;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPanelModalStack::IsEmpty( void ) const
{
    return m_Dialogs.IsEmpty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::PopModal( Panel *pDialog )
{
    FOR_EACH_VEC_BACK( m_Dialogs, i )
    {
        if ( m_Dialogs[i] == pDialog->GetVPanel() )
            PopModal( i );
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::PopModal( int nIndex )
{
    VPanelHandle hModal = m_Dialogs[ nIndex ];
    m_Dialogs.Remove( nIndex );

    VPANEL hPanel = vgui::input()->GetAppModalSurface();
    if ( !hPanel || hModal == hPanel )
    {
        if ( !m_Dialogs.IsEmpty() )
            vgui::input()->SetAppModalSurface( m_Dialogs.Head() );
        else
            vgui::input()->SetAppModalSurface( 0 );
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::PushModal( Panel *pDialog )
{
    VPanelHandle handle;
    handle.Set( pDialog->GetVPanel() );

    FOR_EACH_VEC( m_Dialogs, i )
    {
        if ( m_Dialogs[i] == handle )
        {
            Assert( false );
            return;
        }
    }

    m_Dialogs.AddToHead( handle );

    pDialog->RequestFocus();
    pDialog->MoveToFront();

    vgui::input()->SetAppModalSurface( pDialog->GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPanelHandle CPanelModalStack::Top( void )
{
    if ( !m_Dialogs.IsEmpty() )
        return m_Dialogs.Head();

    return VPanelHandle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPanelModalStack::Update( void )
{
    if ( m_Dialogs.IsEmpty() )
        return;

    if ( !enginevgui->IsGameUIVisible() )
        return;
}
