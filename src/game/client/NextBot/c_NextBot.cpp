#include "cbase.h"
#include "c_NextBot.h"
#include "engine/ivdebugoverlay.h"


IMPLEMENT_CLIENTCLASS( C_NextBotCombatCharacter, DT_NextBot, NextBotCombatCharacter )

BEGIN_RECV_TABLE( C_NextBotCombatCharacter, DT_NextBot )
END_RECV_TABLE()

C_NextBotCombatCharacter::C_NextBotCombatCharacter()
{
}
