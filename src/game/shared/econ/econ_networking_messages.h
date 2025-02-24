#ifndef ECON_NETWORKING_MESSAGES_H
#define ECON_NETWORKING_MESSAGES_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/mempool.h"
#include "tier1/smartptr.h"
#include "inetmessage.h"

struct MsgHdr_t
{
	MsgType_t m_eMsgType;	// Message type
	uint32 m_unMsgSize;		// Size of message without header
	uint64 m_ulSourceID;
	uint64 m_ulTargetID;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNetPacket : public IRefCounted
{
	DECLARE_FIXEDSIZE_ALLOCATOR_MT( CNetPacket );
public:
	CNetPacket()
	{
		m_pMsg = NULL;
		m_Hdr.m_eMsgType = k_EInvalidMsg;
		m_cRefCount = 1;
	}

	void *Data( void ) const { return (byte *)m_pMsg; }
	byte *MutableData( void ) { return (byte *)m_pMsg + sizeof(MsgHdr_t); }
	uint32 Size( void ) const { return m_Hdr.m_unMsgSize + sizeof(MsgHdr_t); }
	MsgHdr_t const &Hdr( void ) const { return m_Hdr; }

protected:
	virtual ~CNetPacket()
	{
		Assert( m_cRefCount == 0 );
		Assert( m_pMsg == NULL );
	}

	friend class CEconNetworking;
	friend class CEconNetMsg;
	void Init( uint32 size, MsgType_t eMsg );
	void InitFromMemory( void const *pMemory, uint32 size );

private:
	void *m_pMsg;
	MsgHdr_t m_Hdr;

	friend class CRefCountAccessor;
	virtual int AddRef( void ) OVERRIDE;
	virtual int Release( void ) OVERRIDE;
	volatile uint m_cRefCount;
};


class CEconNetMsg : public INetMessage
{
public:
	CEconNetMsg( CSmartPtr<CNetPacket> const &pPacket ) : m_pPacket( pPacket ), m_eMsgType( pPacket->Hdr().m_eMsgType ) {}
	CEconNetMsg( void ) : m_pPacket( new CNetPacket() ), m_eMsgType( 0 ) {}

	// Inherited via INetMessage
	virtual void SetNetChannel( INetChannel *netchan ) OVERRIDE { m_pNetChan = netchan; }
	virtual void SetReliable( bool state ) OVERRIDE				{ m_bReliable = state; }
	virtual bool Process( void ) OVERRIDE;
	virtual bool ReadFromBuffer( bf_read &buffer ) OVERRIDE;
	virtual bool WriteToBuffer( bf_write &buffer ) OVERRIDE;
	virtual bool IsReliable( void ) const OVERRIDE				{ return m_bReliable; }
	virtual int GetType( void ) const OVERRIDE					{ return svc_EconMsg; }
	virtual int GetGroup( void ) const OVERRIDE					{ return k_nServerPort; }
	virtual const char *GetName( void ) const OVERRIDE			{ return "svc_EconMsg"; }
	virtual INetChannel *GetNetChannel( void ) const OVERRIDE	{ return m_pNetChan; }
	virtual const char *ToString( void ) const OVERRIDE;

private:
	bool m_bReliable;
	INetChannel *m_pNetChan;
	MsgType_t m_eMsgType;
	CSmartPtr<CNetPacket> m_pPacket;
};

//-----------------------------------------------------------------------------
// Purpose: Interface for processing network packets
//-----------------------------------------------------------------------------
abstract_class IMessageHandler
{
public:
	virtual ~IMessageHandler() {}
	virtual bool ProcessMessage( CNetPacket *pPacket ) = 0;
};


void RegisterEconNetworkMessageHandler( MsgType_t eMsg, IMessageHandler *pHandler );
#define REG_ECON_MSG_HANDLER( classType, msgType, msgName ) \
		static classType s_##msgName##Handler; \
		static class CReg##classType \
		{ \
		public: \
			CReg##classType() { RegisterEconNetworkMessageHandler( msgType, &s_##msgName##Handler ); } \
		} g_Reg##classType

bool QueueEconNetworkMessageWork( IMessageHandler *pHandler, CSmartPtr<CNetPacket> const &pPacket );


template< class TProtoMsg >
class CProtobufMsg
{
	static CUtlMemoryPool *sm_MsgPool;
	static bool sm_bRegisteredPool;
public:
	CProtobufMsg()
		: m_pPacket( NULL )
	{
		m_pBody = AllocMsg();
	}
	CProtobufMsg( CNetPacket *pPacket )
		: m_pPacket( pPacket )
	{
		m_pBody = AllocMsg();
		Assert( m_pBody );

		CRefCountAccessor::AddRef( m_pPacket );
		m_pBody->ParseFromArray( m_pPacket->MutableData(), m_pPacket->Size() - sizeof(MsgHdr_t) );
	}
	virtual ~CProtobufMsg()
	{
		if ( m_pBody )
		{
			FreeMsg( m_pBody );
		}

		if ( m_pPacket )
		{
			CRefCountAccessor::Release( m_pPacket );
		}
	}

	TProtoMsg &Body( void ) { return *m_pBody; }
	TProtoMsg const &Body( void ) const { return *m_pBody; }
	TProtoMsg *operator->() { return m_pBody; }

protected:
	TProtoMsg *AllocMsg( void )
	{
		if ( !sm_bRegisteredPool )
		{
			Assert( sm_MsgPool == NULL );
			sm_MsgPool = new CUtlMemoryPool( sizeof( TProtoMsg ), 1 );

			sm_bRegisteredPool = true;
		}

		if ( sm_MsgPool->Count() > 0 )
			return (TProtoMsg *)sm_MsgPool->Alloc();

		TProtoMsg *pMsg = (TProtoMsg *)sm_MsgPool->Alloc();
		Construct<TProtoMsg>( pMsg );
		return pMsg;
	}

	void FreeMsg( TProtoMsg *pObj )
	{
		pObj->Clear();
		sm_MsgPool->Free( pObj );
	}

private:
	CNetPacket *m_pPacket;
	TProtoMsg *m_pBody;

	// Copying is illegal
	CProtobufMsg( const CProtobufMsg& );
	CProtobufMsg& operator=( const CProtobufMsg& );
};

template< typename TProtoMsg >
CUtlMemoryPool *CProtobufMsg<TProtoMsg>::sm_MsgPool;
template< typename TProtoMsg >
bool CProtobufMsg<TProtoMsg>::sm_bRegisteredPool;

#endif
