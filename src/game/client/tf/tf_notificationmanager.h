#ifndef TF_NOTIFICATIONMANAGER_H
#define TF_NOTIFICATIONMANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "igamesystem.h"
#include "GameEventListener.h"
#include "steam/steam_api.h"
#include "steam/isteamhttp.h"

class CTFNotificationManager;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFNotificationManager : public CAutoGameSystemPerFrame, public CGameEventListener, public ISteamMatchmakingServerListResponse
{
public:
	CTFNotificationManager();
	~CTFNotificationManager();

	// Methods of IGameSystem
	virtual bool Init();
	virtual char const *Name() { return "CTFNotificationManager"; }
	// Gets called each frame
	virtual void Update(float frametime);

	// Methods of CGameEventListener
	virtual void FireGameEvent(IGameEvent *event);

	virtual char*GetVersionString();

	uint32 GetServerFilters(MatchMakingKeyValuePair_t **pFilters);
	// Server has responded ok with updated data
	virtual void ServerResponded(HServerListRequest hRequest, int iServer);
	// Server has failed to respond
	virtual void ServerFailedToRespond(HServerListRequest hRequest, int iServer){}
	// A list refresh you had initiated is now 100% completed
	virtual void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response);

	void UpdateServerlistInfo();
	gameserveritem_t GetServerInfo(int index);

private:
	bool		m_bInited;

	ISteamHTTP*			m_SteamHTTP;
	HTTPRequestHandle	m_httpRequest;

	bool				bCompleted;

	float				fUpdateLastCheck;;	

	HServerListRequest hRequest;
	CUtlMap<int, gameserveritem_t> m_mapServers;
	CUtlVector<MatchMakingKeyValuePair_t> m_vecServerFilters;
};

CTFNotificationManager *GetNotificationManager();
#endif // TF_NOTIFICATIONMANAGER_H
