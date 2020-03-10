//========= Copyright Valve Corporation, All rights reserved. ==============================//
//
// Purpose: Intercepts game events for VScript and call OnGameEvent_<eventname>.
//
//==========================================================================================//

#ifndef SCRIPT_GAME_EVENT_LISTENER_H
#define SCRIPT_GAME_EVENT_LISTENER_H
#ifdef _WIN32
#pragma once
#endif

#include "gameeventlistener.h"
#include <igamesystem.h>

template<typename T, typename I = unsigned short>
class CCopyableStringMap : public CUtlMap<char const *, T, I>
{
public:
	virtual ~CCopyableStringMap();
	explicit CCopyableStringMap( int growSize = 0, int initSize = 0 )
		: CUtlMap<char const *, T, I>( growSize, initSize, CaselessStringLessThan ) {}
	CCopyableStringMap( CCopyableStringMap const &map );
	CCopyableStringMap<T, I> &operator=( const CCopyableStringMap<T, I> &other );
};
typedef CCopyableStringMap<KeyValues::types_t> ParamMap_t;

template<typename T, typename I>
inline CCopyableStringMap<T, I>::~CCopyableStringMap()
{
	for ( int i=0; i < MaxElement(); ++i )
	{
		if ( !IsValidIndex( i ) )
			continue;

		delete[] Key( i );
	}
}

// Used to intercept game events for VScript and call OnGameEvent_<eventname>.
class CScriptGameEventListener : public CGameEventListener, public CBaseGameSystem
{
public:
	virtual ~CScriptGameEventListener();

	virtual void SetVScriptEventValues( IGameEvent *event, HSCRIPT table );

	enum
	{
		TYPE_LOCAL, 
		TYPE_STRING,
		TYPE_FLOAT,
		TYPE_LONG,
		TYPE_SHORT,
		TYPE_BYTE,
		TYPE_BOOL,
	};

public: // IGameEventListener Interface

	virtual void FireGameEvent( IGameEvent * event );
	
public: // CBaseGameSystem overrides

	virtual bool Init();

private:
	typedef struct
	{
		char const *m_szEventName;
		ParamMap_t m_EventParams;
	} GameEvents_t;
	CUtlDict<GameEvents_t> m_GameEvents;
};

extern CScriptGameEventListener *ScriptGameEventListener();

#endif
