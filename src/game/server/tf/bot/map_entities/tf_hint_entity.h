//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_HINT_ENTITY_H
#define TF_HINT_ENTITY_H
#ifdef _WIN32
#pragma once
#endif


DECLARE_AUTO_LIST( ITFBotHintEntityAutoList );

class CBaseTFBotHintEntity : public CPointEntity, public ITFBotHintEntityAutoList
{
	DECLARE_CLASS( CBaseTFBotHintEntity, CPointEntity );
public:
	enum HintType : int
	{
		TELEPORTER_EXIT = 0,
		SENTRY_GUN      = 1,
		ENGINEER_NEST   = 2,
	};

	CBaseTFBotHintEntity();
	virtual ~CBaseTFBotHintEntity();

	DECLARE_DATADESC();

	virtual HintType GetHintType() const = 0;

	void InputEnable( inputdata_t& inputdata );
	void InputDisable( inputdata_t& inputdata );

	bool OwnerObjectFinishBuilding() const;
	bool OwnerObjectHasNoOwner() const;

	bool IsDisabled( void ) const { return m_isDisabled; }

protected:
	bool m_isDisabled;
	// 368 CHandle<T> (unused)
};

#endif
