#ifndef ENTITY_SOLDIER_STATUE_H
#define ENTITY_SOLDIER_STATUE_H

#ifdef _WIN32
#pragma once
#endif


class CEntitySoldierStatue : public CBaseAnimating
{
	DECLARE_CLASS( CEntitySoldierStatue, CBaseAnimating );
public:

	virtual ~CEntitySoldierStatue() {}

	virtual void Precache();
	virtual void Spawn();

	void		StatueThink( void );

private:
	CountdownTimer m_voiceLineTimer;
};

#endif
