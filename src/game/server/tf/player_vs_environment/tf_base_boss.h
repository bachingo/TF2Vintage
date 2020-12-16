//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BASE_BOSS_H
#define TF_BASE_BOSS_H

#ifdef _WIN32
#pragma once
#endif

#include "NextBot/NextBot.h"
#include "NextBot/NextBotGroundLocomotion.h"


class CTFBaseBossLocomotion : public NextBotGroundLocomotion
{
public:
	CTFBaseBossLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { }
	virtual ~CTFBaseBossLocomotion() { }

	virtual float GetRunSpeed( void ) const OVERRIDE;
	virtual float GetStepHeight( void ) const OVERRIDE { return 100.0f; }
	virtual float GetMaxJumpHeight( void ) const OVERRIDE { return 100.0f; }

	virtual void FaceTowards( const Vector &target ) OVERRIDE;
};


class CTFBaseBoss : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CTFBaseBoss, NextBotCombatCharacter );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFBaseBoss();
	virtual ~CTFBaseBoss();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void Touch( CBaseEntity *pOther );
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void UpdateOnRemove();
	virtual int UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_ALWAYS ); }
	virtual void UpdateCollisionBounds( void ) {}
	virtual bool IsRemovedOnReset( void ) const { return false; }

	void BossThink( void );

	void SetMaxSpeed( float value ) { m_speed = value; }
	virtual float GetMaxSpeed( void ) const { return m_speed; }
	void SetInitialHealth( int value ) { m_initialHealth = value; }
	int GetInitialHealth( void ) { return m_initialHealth; }
	void SetCurrencyValue( int value ) { m_nCurrencyValue = value; }
	virtual int GetCurrencyValue( void ) { return 125; /*Why doesn't this return m_nCurrencyValue?*/ }
	void SetResolvePlayerCollisions( bool bResolve ) { m_bResolvePlayerCollisions = bResolve; }

	virtual CTFBaseBossLocomotion *GetLocomotionInterface( void ) const	{ return m_locomotor; }

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetSpeed( inputdata_t &inputdata );
	void InputSetHealth( inputdata_t &inputdata );
	void InputSetMaxHealth( inputdata_t &inputdata );
	void InputAddHealth( inputdata_t &inputdata );
	void InputRemoveHealth( inputdata_t &inputdata );

	COutputEvent m_outputOnHealthBelow90Percent;
	COutputEvent m_outputOnHealthBelow80Percent;
	COutputEvent m_outputOnHealthBelow70Percent;
	COutputEvent m_outputOnHealthBelow60Percent;
	COutputEvent m_outputOnHealthBelow50Percent;
	COutputEvent m_outputOnHealthBelow40Percent;
	COutputEvent m_outputOnHealthBelow30Percent;
	COutputEvent m_outputOnHealthBelow20Percent;
	COutputEvent m_outputOnHealthBelow10Percent;
	COutputEvent m_outputOnKilled;

protected:
	virtual void ModifyDamage(CTakeDamageInfo *) {}
	void ResolvePlayerCollision( CTFPlayer *pPlayer );

private:
	int m_initialHealth;
	CNetworkVar( float, m_lastHealthPercentage );
	string_t m_modelString;
	float m_speed;
	int m_startDisabled;
	bool m_bEnabled;
	int m_nDamagePoseParameter;
	int m_nCurrencyValue;

	bool m_bResolvePlayerCollisions;

	CTFBaseBossLocomotion *m_locomotor;
};

#endif