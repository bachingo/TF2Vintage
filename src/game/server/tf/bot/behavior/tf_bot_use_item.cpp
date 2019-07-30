#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_use_item.h"

CTFBotUseItem::CTFBotUseItem( CTFWeaponBase *item )
{
	m_hItem = item;
}

CTFBotUseItem::~CTFBotUseItem()
{
}


const char *CTFBotUseItem::GetName( void ) const
{
	return "UseItem";
}


ActionResult<CTFBot> CTFBotUseItem::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->PushRequiredWeapon(m_hItem);
	m_InitialDelay.Start(0.25f + (m_hItem->m_flNextPrimaryAttack - gpGlobals->curtime));
	
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUseItem::Update( CTFBot *me, float dt )
{
	if (m_hItem == nullptr || me->m_Shared.GetActiveTFWeapon() == nullptr) {
		return Action<CTFBot>::Done("NULL item");
	}
	
	if (m_InitialDelay.HasStarted())
	{
		if (m_InitialDelay.IsElapsed())
		{
			me->PressFireButton();
			m_InitialDelay.Invalidate();
		}
		
		return Action<CTFBot>::Continue();
	}
	
	if (me->m_Shared.InCond(TF_COND_TAUNTING)) {
		return Action<CTFBot>::Continue();
	}
	
	return Action<CTFBot>::Done("Item used");
}

void CTFBotUseItem::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->PopRequiredWeapon();
}
