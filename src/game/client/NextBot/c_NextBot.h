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

	virtual bool IsNextBot( void ) override { return true; }

	virtual void Spawn( void ) override;
	virtual void UpdateClientSideAnimation( void ) override;

private:
	C_NextBotCombatCharacter( C_NextBotCombatCharacter const& ); // not defined, not accessible
};


#endif // C_NEXTBOT_H
