//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_ROSTER_H
#define TF_BOT_ROSTER_H

#ifdef _WIN32
#pragma once
#endif

class CTFBotRoster : public CPointEntity
{
	DECLARE_CLASS( CTFBotRoster, CPointEntity );
public:
	DECLARE_DATADESC();

	CTFBotRoster( void );
	virtual ~CTFBotRoster() {}

	void InputSetAllowScout( inputdata_t &inputdata );
	void InputSetAllowSniper( inputdata_t &inputdata );
	void InputSetAllowSoldier( inputdata_t &inputdata );
	void InputSetAllowDemoman( inputdata_t &inputdata );
	void InputSetAllowMedic( inputdata_t &inputdata );
	void InputSetAllowHeavy( inputdata_t &inputdata );
	void InputSetAllowPyro( inputdata_t &inputdata );
	void InputSetAllowSpy( inputdata_t &inputdata );
	void InputSetAllowEngineer( inputdata_t &inputdata );

	bool IsClassAllowed( int iBotClass ) const;
	bool IsClassChangeAllowed( void ) const;
	char const *GetTeamName( void ) const;

private:
	string_t m_teamName;
	bool m_bAllowClassChanges;
	bool m_bAllowedClasses[ TF_LAST_NORMAL_CLASS + 1 ];
};
#endif