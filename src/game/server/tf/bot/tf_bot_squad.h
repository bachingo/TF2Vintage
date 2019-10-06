//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_SQUAD_H
#define TF_BOT_SQUAD_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotInterface.h"
#include "ehandle.h"

class CTFBot;

class CTFBotSquad : public INextBotEventResponder
{
public:
	CTFBotSquad();
	virtual ~CTFBotSquad();

	virtual INextBotEventResponder *FirstContainedResponder( void ) const override;
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *prev ) const override;

	struct Iterator
	{
		CTFBot *bot;
		int index;
	};

	void CollectMembers( CUtlVector<CTFBot *> *members ) const;

	Iterator GetFirstMember( void ) const;
	Iterator GetNextMember( const Iterator& it ) const;

	int GetMemberCount( void ) const;
	CTFBot *GetLeader( void ) const;

	void Join( CTFBot *bot );
	void Leave( CTFBot *bot );

	float GetMaxSquadFormationError( void ) const;
	float GetSlowestMemberIdealSpeed( bool include_leader ) const;
	float GetSlowestMemberSpeed( bool include_leader ) const;

	bool IsInFormation( void ) const;
	bool ShouldSquadLeaderWaitForFormation( void ) const;

	void DisbandAndDeleteSquad( void );

	CUtlVector< CHandle<CTFBot> > m_hMembers;
	CHandle<CTFBot> m_hLeader;
	float m_flFormationSize;
	bool m_bShouldPreserveSquad;
};

#endif