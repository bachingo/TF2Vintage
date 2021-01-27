//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef ENTITY_MERASMUS_TRICKORTREAT_PROP_H
#define ENTITY_MERASMUS_TRICKORTREAT_PROP_H
#ifdef _WIN32
#pragma once
#endif

DECLARE_AUTO_LIST( ITFMerasmusTrickOrTreatPropAutoList )
class CTFMerasmusTrickOrTreatProp : public CBaseAnimating, public ITFMerasmusTrickOrTreatPropAutoList
{
	DECLARE_CLASS( CTFMerasmusTrickOrTreatProp, CBaseAnimating );
public:

	

	CTFMerasmusTrickOrTreatProp();
	virtual ~CTFMerasmusTrickOrTreatProp() {}

	static CTFMerasmusTrickOrTreatProp *Create( Vector const &vecOrigin, QAngle const &vecAngles );
	static char const  *GetRandomPropModelName( void );

	virtual void		Spawn( void );
	virtual void		Event_Killed( CTakeDamageInfo const &info );
	virtual int			OnTakeDamage( CTakeDamageInfo const &info );

	virtual void		Touch( CBaseEntity *pOther ) OVERRIDE;

	void				SpawnTrickOrTreatItem( void );
};

#endif