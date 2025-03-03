#include "cbase.h"
#include "filesystem.h"
#include "vstdlib/coroutine.h"
#include "econ_networking.h"
#include "econ_networking_messages.h"
#include "steam/steamclientpublic.h"
#include "steam/isteamuser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CUtlVector<CUtlMemoryPool*> s_vecMsgPools;
struct WorkItem_t
{
	IMessageHandler *m_pHandler;
	CSmartPtr<CNetPacket> m_pPacket;
};

// Queue is the wrong word
// TODO: Threaded job manager
bool QueueEconNetworkMessageWork( IMessageHandler *pHandler, CSmartPtr<CNetPacket> const &pPacket )
{
	if ( pHandler == nullptr || pPacket == nullptr )
		return false;

	const auto WorkThread = [ ] ( void *pvParam ) {
		WorkItem_t *pWork = (WorkItem_t *)pvParam;
		CNetPacket *pPacket = pWork->m_pPacket.GetObject();

		while ( !pWork->m_pHandler->ProcessMessage( pPacket ) )
			ThreadSleep(25);
	};

	WorkItem_t work{pHandler, pPacket};
	HCoroutine hCoroutine = Coroutine_Create( WorkThread, &work );
	Coroutine_Continue( hCoroutine, "process packet" );

	return true;
}

////-----------------------------------------------------------------------------
//// Purpose: 
////-----------------------------------------------------------------------------
//class CServerHelloHandler : public IMessageHandler
//{
//public:
//	CServerHelloHandler() {}
//
//	virtual bool ProcessMessage( CNetPacket *pPacket )
//	{
//		CProtobufMsg<CServerHelloMsg> msg( pPacket );
//
//		CSteamID serverID( msg->remote_steamid() );
//		DbgVerify( serverID.BGameServerAccount() );
//		
//	#if defined( CLIENT_DLL )
//		if ( !engine->IsConnected() )
//			return true;
//
//		uint unVersion = 0;
//		if ( msg->has_version() )
//		{
//			FileHandle_t fh = g_pFullFileSystem->Open( "version.txt", "r", "MOD" );
//			if ( fh && g_pFullFileSystem->Size( fh ) > 0 )
//			{
//				char version[48];
//				g_pFullFileSystem->ReadLine( version, sizeof( version ), fh );
//				unVersion = CRC32_ProcessSingleBuffer( version, Q_strlen( version ) + 1 );
//
//				if ( msg->version() != unVersion )
//					Warning( "The server you are trying to connect to is running a different version of the game.\n" );
//			}
//
//			g_pFullFileSystem->Close( fh );
//		}
//
//		CProtobufMsg<CClientHelloMsg> reply;
//		reply->set_version( unVersion );
//
//		CSteamID playerID = SteamUser()->GetSteamID();
//		reply->set_remote_steamid( playerID.ConvertToUint64() );
//
//		const int nLength = reply->ByteSize();
//		CArrayAutoPtr<byte> array( new byte[ nLength ]() );
//		reply->SerializeWithCachedSizesToArray( array.Get() );
//
//		g_pNetworking->SendMessage( serverID, k_EClientHelloMsg, array.Get(), nLength );
//	#endif
//
//		return true;
//	}
//};
//REG_ECON_MSG_HANDLER( CServerHelloHandler, k_EServerHelloMsg, ServerHelloMsg );
//
////-----------------------------------------------------------------------------
//// Purpose: 
////-----------------------------------------------------------------------------
//class CClientHelloHandler : public IMessageHandler
//{
//public:
//	CClientHelloHandler() {}
//
//	virtual bool ProcessMessage( CNetPacket *pPacket )
//	{
//	#if defined( GAME_DLL )
//		FileHandle_t fh = filesystem->Open("version.txt", "r", "MOD");
//		if ( fh && filesystem->Size( fh ) > 0 )
//		{
//			CProtobufMsg<CClientHelloMsg> msg( pPacket );
//
//			char version[48];
//			filesystem->ReadLine( version, sizeof( version ), fh );
//
//			uint32 unVersion = CRC32_ProcessSingleBuffer( version, Q_strlen(version)+1 );
//			if ( unVersion > 0 && msg->has_version() )
//			{
//				if ( msg->version() != unVersion )
//				{
//					engine->ServerCommand( UTIL_VarArgs( 
//						"kickid %d \"The server you are trying to connect to is running a\\n different version of the game.\"\n",
//						UTIL_PlayerBySteamID( msg->remote_steamid() )->GetUserID() ) 
//					);
//				}
//			}
//		}
//
//		filesystem->Close( fh );
//	#endif
//
//		return true;
//	}
//};
//REG_ECON_MSG_HANDLER( CClientHelloHandler, k_EClientHelloMsg, ClientHelloMsg );



bool CEconNetMsg::Process( void )
{
	if ( m_eMsgType == 0 )
		return false;

	if ( m_pPacket->Hdr().m_eMsgType != m_eMsgType )
		return false;

	g_pNetworking->RecvMessage( m_pPacket->Hdr().m_ulSourceID, m_eMsgType, m_pPacket->Data(), m_pPacket->Size() );
	return true;
}

bool CEconNetMsg::ReadFromBuffer( bf_read &buffer )
{
	m_eMsgType = buffer.ReadShort();
	uint32 nSize = buffer.ReadLong();

	CArrayAutoPtr<byte> array( new byte[nSize]() );
	buffer.ReadBytes( array.Get(), nSize );

	m_pPacket->InitFromMemory( array.Get(), nSize );

	return !buffer.IsOverflowed();
}

bool CEconNetMsg::WriteToBuffer( bf_write &buffer )
{
	buffer.WriteUBitLong( GetType(), 6 ); // More magic numbers, all other types are encoded in 6 bits
	buffer.WriteShort( m_eMsgType );
	buffer.WriteLong( m_pPacket->Size() );
	buffer.WriteBytes( m_pPacket->Data(), m_pPacket->Size() );
	return !buffer.IsOverflowed();
}

const char *CEconNetMsg::ToString( void ) const
{
	static char szString[32];
	V_sprintf_safe( szString, "%s: type %d", GetName(), m_pPacket->Hdr().m_eMsgType );
	return szString;
}

bool CEconNetMsg::BIncomingMessageForProcessing( double dblNetTime, int numBytes )
{
	return false;
}

size_t CEconNetMsg::GetSize() const
{
	return m_pPacket->Size();
}
