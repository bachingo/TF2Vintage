//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_MODAL_STACK_H
#define TF_MODAL_STACK_H

#include "tier1/utlvector.h"
namespace vgui
{
	class VPanelHandle;
	class Panel;
};


class CPanelModalStack
{
public:
	bool			IsEmpty( void ) const;
	void			PopModal( vgui::Panel *pDialog );
	void			PushModal( vgui::Panel *pDialog );
	vgui::VPanelHandle Top( void );
	void			Update( void );

private:
	void		PopModal( int nIndex );

	CUtlVector<vgui::VPanelHandle> m_Dialogs;
};

extern CPanelModalStack *TFModalStack( void );

#endif