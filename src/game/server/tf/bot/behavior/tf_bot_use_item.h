#ifndef TF_BOT_USE_ITEM_H
#define TF_BOT_USE_ITEM_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFWeaponBase;

class CTFBotUseItem : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotUseItem, Action<CTFBot> )
public:
	CTFBotUseItem( CTFWeaponBase *item );
	virtual ~CTFBotUseItem();
	
	virtual const char *GetName( void ) const OVERRIDE;
	
	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;
	
private:
	CHandle<CTFWeaponBase> m_hItem;
	CountdownTimer m_InitialDelay;
};

#endif