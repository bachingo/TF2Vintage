#ifndef TF_BOT_USE_TELEPORTER_H
#define TF_BOT_USE_TELEPORTER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CObjectTeleporter;

class CTFBotUseTeleporter : public Action<CTFBot>
{
public:
	enum UseHowType
	{
		USE_IMMEDIATE,
		USE_WAIT
	};
	
	CTFBotUseTeleporter(CObjectTeleporter *teleporter, UseHowType how = USE_IMMEDIATE);
	virtual ~CTFBotUseTeleporter();
	
	virtual const char *GetName( void ) const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *me, Action<CTFBot> *priorAction) override;
	virtual ActionResult<CTFBot> Update(CTFBot *me, float dt) override;
	
private:
	bool IsTeleporterAvailable( void ) const;
	
	CHandle<CObjectTeleporter> m_hTele;
	UseHowType m_iHow;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePath;
	bool m_bTeleported;
};

#endif