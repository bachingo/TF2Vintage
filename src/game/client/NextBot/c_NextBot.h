#ifndef C_NEXTBOT_H
#define C_NEXTBOT_H
#ifdef _WIN32
#pragma once
#endif


#include "c_basecombatcharacter.h"

class C_NextBotCombatCharacter : public C_BaseCombatCharacter
{
	DECLARE_CLASS( C_NextBotCombatCharacter, C_BaseCombatCharacter )
public:
	DECLARE_CLIENTCLASS()

	C_NextBotCombatCharacter();

	virtual bool IsNextBot( void ) OVERRIDE { return true; }

	virtual void Spawn( void ) OVERRIDE;
	virtual void UpdateClientSideAnimation( void ) OVERRIDE;

	virtual ShadowType_t ShadowCastType( void );
	void UpdateShadowLOD( void );

private:
	C_NextBotCombatCharacter( C_NextBotCombatCharacter const& ); // not defined, not accessible

	ShadowType_t m_nShadowType;
	CountdownTimer m_shadowUpdateTimer;
};


#endif // C_NEXTBOT_H
