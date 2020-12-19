#ifndef TF_LOGIC_ENTITIES_H
#define TF_LOGIC_ENTITIES_H

#include "GameEventListener.h"
class CTeamControlPoint;
class CPointEntity;


class CArenaLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CArenaLogic, CPointEntity );
	DECLARE_DATADESC();

	virtual void	Spawn( void );
	virtual int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
	void			OnCapEnabled( void );
	//void			ArenaLogicThink( void );

	void			InputRoundActivate( inputdata_t &inputdata );

	float		m_flTimeToEnableCapPoint;
	bool		m_bCapUnlocked;
	COutputEvent m_OnArenaRoundStart;
	COutputEvent m_OnCapEnabled;
};

class CKothLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CKothLogic, CPointEntity );
	DECLARE_DATADESC();

	CKothLogic();

	virtual void	InputAddBlueTimer( inputdata_t &inputdata );
	virtual void	InputAddRedTimer( inputdata_t &inputdata );
	virtual void	InputAddGreenTimer( inputdata_t &inputdata );
	virtual void	InputAddYellowTimer( inputdata_t &inputdata );
	virtual void	InputSetBlueTimer( inputdata_t &inputdata );
	virtual void	InputSetRedTimer( inputdata_t &inputdata );
	virtual void	InputSetGreenTimer( inputdata_t &inputdata );
	virtual void	InputSetYellowTimer( inputdata_t &inputdata );
	virtual void	InputRoundSpawn( inputdata_t &inputdata );
	virtual void	InputRoundActivate( inputdata_t &inputdata );

private:
	int m_iTimerLength;
	int m_iUnlockPoint;

};

class CCPTimerLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CCPTimerLogic, CPointEntity );
	DECLARE_DATADESC();

	CCPTimerLogic();

	virtual void	Spawn( void );
	virtual int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
	void			Think( void );
	void			InputRoundSpawn( inputdata_t &inputdata );

	COutputEvent m_onCountdownStart;
	COutputEvent m_onCountdown15SecRemain;
	COutputEvent m_onCountdown10SecRemain;
	COutputEvent m_onCountdown5SecRemain;
	COutputEvent m_onCountdownEnd;

private:

	string_t					m_iszControlPointName;
	int							m_nTimerLength;
	CHandle<CTeamControlPoint>	m_hPoint;

	bool						m_bRestartTimer;
	CountdownTimer				m_TimeRemaining;

	bool						m_bOn15SecRemain;
	bool						m_bOn10SecRemain;
	bool						m_bOn5SecRemain;

};

class CMultipleEscortLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CMultipleEscortLogic, CPointEntity );

	virtual int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
};

class CMedievalLogic : public CPointEntity
{
public:
	DECLARE_CLASS( CMedievalLogic, CPointEntity );

	virtual int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
};

class CHybridMap_CTF_CP : public CPointEntity
{
public:
	DECLARE_CLASS( CHybridMap_CTF_CP, CPointEntity );

	virtual int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
};

// Barebone for now, just so maps don't complain
class CTFHolidayEntity : public CPointEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFHolidayEntity, CPointEntity );
	DECLARE_DATADESC();

	CTFHolidayEntity();
	virtual ~CTFHolidayEntity() { }

	virtual int		UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_ALWAYS ); }
	virtual void	FireGameEvent( IGameEvent *event );

	void			InputHalloweenSetUsingSpells( inputdata_t &inputdata );
	void			InputHalloweenTeleportToHell( inputdata_t &inputdata );

	int				GetHolidayType( void ) const { return m_nHolidayType; }
	bool			ShouldAllowHaunting( void ) const { return m_nAllowHaunting != 0; }

private:
	int m_nHolidayType;
	int m_nTauntInHell;
	int m_nAllowHaunting;
};

class CLogicOnHoliday : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicOnHoliday, CLogicalEntity );
	DECLARE_DATADESC();

	void InputFire( inputdata_t & );

private:
	COutputEvent m_IsAprilFools;
	COutputEvent m_IsFullMoon;
	COutputEvent m_IsHalloween;
	COutputEvent m_IsNothing;
	COutputEvent m_IsSmissmas;
	COutputEvent m_IsTFBirthday;
	COutputEvent m_IsValentines;
};

#endif // TF_LOGIC_ENTITIES_H