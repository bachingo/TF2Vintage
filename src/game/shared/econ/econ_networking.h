#ifndef ECON_NETWORKING_H
#define ECON_NETWORKING_H
#ifdef _WIN32
#pragma once
#endif


#include "steam/isteamclient.h"
#include "steam/isteamnetworking.h"
#include "tier1/mempool.h"
#include "inetchannelinfo.h"

#include <tier0/valve_minmax_off.h>
//#include "econ_messages.pb.h"
#include <tier0/valve_minmax_on.h>


typedef uint32 MsgType_t;

#define ECON_SERVER_PORT 27050
#define SERVER_TIMEOUT	20

#define svc_EconMsg	35	// Horrible magic number, but 32 appears to be the last real message
						// 36 is the maximum index as it's encoded in just 6 bits

/* Peer To Peer connection definitions */
const int k_nInvalidPort = INetChannelInfo::TOTAL;
const int k_nClientPort = INetChannelInfo::TOTAL + 1; // Client <-> Client port
const int k_nServerPort = INetChannelInfo::TOTAL + 2; // Client <-> Server port

//-----------------------------------------------------------------------------
// Purpose: Interface for sending economy data over the wire to clients
//-----------------------------------------------------------------------------
abstract_class IEconNetworking
{
public:
	virtual bool Init( void ) = 0;
	virtual void Shutdown( void ) = 0;
	virtual void Update( float frametime ) = 0;
#if defined(GAME_DLL)
	virtual void OnClientConnected( CSteamID const &id ) = 0;
	virtual void OnClientDisconnected( CSteamID const &steamID ) = 0;
#endif
	virtual bool SendMessage( CSteamID const &targetID, MsgType_t eMsg, void *pubData, uint32 cubData ) = 0;
	virtual void RecvMessage( CSteamID const &remoteID, MsgType_t eMsg, void const *pubData, uint32 const cubData ) = 0;
};

extern IEconNetworking *g_pNetworking;


#endif
