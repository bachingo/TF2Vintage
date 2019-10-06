//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_squad.h"

CTFBotSquad::CTFBotSquad()
{
	m_flFormationSize = -1.0f;
	m_bShouldPreserveSquad = false;
}

CTFBotSquad::~CTFBotSquad()
{
}


INextBotEventResponder *CTFBotSquad::FirstContainedResponder( void ) const
{
	if ( m_hMembers.IsEmpty() )
		return nullptr;

	return m_hMembers[0];
}

INextBotEventResponder *CTFBotSquad::NextContainedResponder( INextBotEventResponder *prev ) const
{
	CHandle<CTFBot> hPrev;

	if ( prev != nullptr )
	{
		CTFBot *bot_prev = static_cast<CTFBot *>( prev );
		if ( bot_prev != nullptr )
			hPrev = bot_prev;
	}

	int idx = m_hMembers.Find( hPrev );
	if ( idx != -1 )
	{
		return m_hMembers[idx];
	}

	return nullptr;
}


void CTFBotSquad::CollectMembers( CUtlVector<CTFBot *> *members ) const
{
	members->RemoveAll();

	FOR_EACH_VEC( m_hMembers, i )
	{
		CTFBot *bot = m_hMembers[i];
		if ( bot && bot->IsAlive() )
		{
			members->AddToTail( m_hMembers[i] );
		}
	}
}


CTFBotSquad::Iterator CTFBotSquad::GetFirstMember( void ) const
{
	FOR_EACH_VEC( m_hMembers, i )
	{
		CTFBot *bot = m_hMembers[i];
		if ( bot && bot->IsAlive() )
		{
			return {
				m_hMembers[i],
				i,
			};
		}
	}

	return {
		nullptr,
		-1,
	};
}

CTFBotSquad::Iterator CTFBotSquad::GetNextMember( const Iterator& it ) const
{
	for ( int i = it.index + 1; i < m_hMembers.Count(); ++i )
	{
		CTFBot *bot = m_hMembers[i];
		if ( bot && bot->IsAlive() )
		{
			return {
				m_hMembers[i],
				i,
			};
		}
	}

	return {
		nullptr,
		-1,
	};
}

int CTFBotSquad::GetMemberCount( void ) const
{
	int count = 0;

	FOR_EACH_VEC( m_hMembers, i )
	{
		CTFBot *bot = m_hMembers[i];
		if ( bot && bot->IsAlive() )
			++count;
	}

	return count;
}

CTFBot *CTFBotSquad::GetLeader( void ) const
{
	return m_hLeader;
}


void CTFBotSquad::Join( CTFBot *bot )
{
	if ( m_hMembers.IsEmpty() )
		m_hLeader = bot;

	m_hMembers.AddToTail( bot );
}

void CTFBotSquad::Leave( CTFBot *bot )
{
	int idx = m_hMembers.Find( bot );
	if ( m_hMembers.IsValidIndex( idx ) )
		m_hMembers.Remove( idx );

	CTFBot *leader = m_hLeader;
	if ( bot == leader )
	{
		m_hLeader = nullptr;

		if ( m_bShouldPreserveSquad )
		{
			CUtlVector<CTFBot *> members;
			this->CollectMembers( &members );

			if ( !members.IsEmpty() )
				m_hLeader = members[0];
		}
	}

	if ( this->GetMemberCount() == 0 )
		this->DisbandAndDeleteSquad();
}


float CTFBotSquad::GetMaxSquadFormationError( void ) const
{
	float error = 0.0f;

	/* exclude squad leader */
	for ( int i = 1; i < m_hMembers.Count(); ++i )
	{
		CTFBot *member = m_hMembers[i];
		if ( member == nullptr || !member->IsAlive() )
			continue;

		error = Max( error, member->m_flFormationError );
	}

	return error;
}

float CTFBotSquad::GetSlowestMemberIdealSpeed( bool include_leader ) const
{
	float speed = FLT_MAX;

	for ( int i = ( include_leader ? 0 : 1 ); i < m_hMembers.Count(); ++i )
	{
		CTFBot *member = m_hMembers[i];
		if ( member == nullptr || !member->IsAlive() )
			continue;

		speed = Min( speed, member->GetPlayerClass()->GetMaxSpeed() );
	}

	return speed;
}

float CTFBotSquad::GetSlowestMemberSpeed( bool include_leader ) const
{
	float speed = FLT_MAX;

	for ( int i = ( include_leader ? 0 : 1 ); i < m_hMembers.Count(); ++i )
	{
		CTFBot *member = m_hMembers[i];
		if ( member == nullptr || !member->IsAlive() )
			continue;

		speed = Min( speed, member->MaxSpeed() );
	}

	return speed;
}


bool CTFBotSquad::IsInFormation( void ) const
{
	/* exclude squad leader */
	for ( int i = 1; i < m_hMembers.Count(); ++i )
	{
		CTFBot *member = m_hMembers[i];
		if ( member == nullptr || !member->IsAlive() )
			continue;

		if ( member->m_bIsInFormation )
			continue;

		if ( member->GetLocomotionInterface()->IsStuck() )
			continue;

		if ( member->IsPlayerClass( TF_CLASS_MEDIC ) )
			continue;

		if ( member->m_flFormationError > 0.75f )
			return false;
	}

	return true;
}

bool CTFBotSquad::ShouldSquadLeaderWaitForFormation( void ) const
{
	/* exclude squad leader */
	for ( int i = 1; i < m_hMembers.Count(); ++i )
	{
		CTFBot *member = m_hMembers[i];
		if ( member == nullptr || !member->IsAlive() )
			continue;

		if ( member->m_flFormationError < 1.0f )
			continue;

		if ( member->m_bIsInFormation )
			continue;

		if ( member->GetLocomotionInterface()->IsStuck() )
			continue;

		if ( !member->IsPlayerClass( TF_CLASS_MEDIC ) )
			return true;
	}

	return false;
}


void CTFBotSquad::DisbandAndDeleteSquad( void )
{
	FOR_EACH_VEC( m_hMembers, i )
	{
		CTFBot *bot = m_hMembers[i];
		if ( bot != nullptr )
			bot->m_pSquad = nullptr;
	}

	delete this;
}
