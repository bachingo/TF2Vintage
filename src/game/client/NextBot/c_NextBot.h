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

private:
	C_NextBotCombatCharacter( const C_NextBotCombatCharacter & ); // not defined, not accessible
};


#endif // C_NEXTBOT_H
