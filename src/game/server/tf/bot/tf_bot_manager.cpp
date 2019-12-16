//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "tf_bot_manager.h"
#include "tf_bot.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#include "fmtstr.h"


extern ConVar tf_bot_difficulty;
extern ConVar tf_bot_prefix_name_with_difficulty;

ConVar tf_bot_quota( "tf_bot_quota", "0", FCVAR_NONE, "Determines the total number of TF bots in the game." );
ConVar tf_bot_quota_mode( "tf_bot_quota_mode", "normal", FCVAR_NONE, "Determines the type of quota. Allowed values: 'normal', 'fill', and 'match'. If 'fill', the server will adjust bots to keep N players in the game, where N is bot_quota. If 'match', the server will maintain a 1:N ratio of humans to bots, where N is bot_quota." );
ConVar tf_bot_join_after_player( "tf_bot_join_after_player", "1", FCVAR_NONE, "If nonzero, bots wait until a player joins before entering the game.", true, 0.0f, true, 1.0f );
ConVar tf_bot_auto_vacate( "tf_bot_auto_vacate", "1", FCVAR_NONE, "If nonzero, bots will automatically leave to make room for human players.", true, 0.0f, true, 1.0f );
ConVar tf_bot_offline_practice( "tf_bot_offline_practice", "0", FCVAR_NONE, "Tells the server that it is in offline practice mode.", true, 0.0f, true, 1.0f );
ConVar tf_bot_melee_only( "tf_bot_melee_only", "0", FCVAR_GAMEDLL, "If nonzero, TFBots will only use melee weapons", true, 0.0f, true, 1.0f );

// this whole function seems unnecessary,
// why not have them in a database that can be changed be the end user,
// and why do a loop to figure out how big you made this list,
// and this isn't even random, it starts at a random spot, but goes linearly thereafter
const char *GetRandomBotName( void )
{
	static const char *const nameList[] = {
		"Chucklenuts",
		"CryBaby",
		"WITCH",
		"ThatGuy",
		"Still Alive",
		"Hat-Wearing MAN",
		"Me",
		"Numnutz",
		"H@XX0RZ",
		"The G-Man",
		"Chell",
		"The Combine",
		"Totally Not A Bot",
		"Pow!",
		"Zepheniah Mann",
		"THEM",
		"LOS LOS LOS",
		"10001011101",
		"DeadHead",
		"ZAWMBEEZ",
		"MindlessElectrons",
		"TAAAAANK!",
		"The Freeman",
		"Black Mesa",
		"Soulless",
		"CEDA",
		"BeepBeepBoop",
		"NotMe",
		"CreditToTeam",
		"BoomerBile",
		"Someone Else",
		"Mann Co.",
		"Dog",
		"Kaboom!",
		"AmNot",
		"0xDEADBEEF",
		"HI THERE",
		"SomeDude",
		"GLaDOS",
		"Hostage",
		"Headful of Eyeballs",
		"CrySomeMore",
		"Aperture Science Prototype XR7",
		"Humans Are Weak",
		"AimBot",
		"C++",
		"GutsAndGlory!",
		"Nobody",
		"Saxton Hale",
		"RageQuit",
		"Screamin' Eagles",
		"Ze Ubermensch",
		"Maggot",
		"CRITRAWKETS",
		"Herr Doktor",
		"Gentlemanne of Leisure",
		"Companion Cube",
		"Target Practice",
		"One-Man Cheeseburger Apocalypse",
		"Crowbar",
		"Delicious Cake",
		"IvanTheSpaceBiker",
		"I LIVE!",
		"Cannon Fodder",
		"trigger_hurt",
		"Nom Nom Nom",
		"Divide by Zero",
		"GENTLE MANNE of LEISURE",
		"MoreGun",
		"Tiny Baby Man",
		"Big Mean Muther Hubbard",
		"Force of Nature",
		"Crazed Gunman",
		"Grim Bloody Fable",
		"Poopy Joe",
		"A Professional With Standards",
		"Freakin' Unbelievable",
		"SMELLY UNFORTUNATE",
		"The Administrator",
		"Mentlegen",
		"Archimedes!",
		"Ribs Grow Back",
		"It's Filthy in There!",
		"Mega Baboon",
		"Kill Me",
		"Glorified Toaster with Legs"
	};
	static int nameCount = 0;
	static int nameIndex = 0;

	if ( nameCount == 0 )
	{
		/*for (int i=0; nameList[i]; ++i)
			++nameCount;*/
		nameCount = ARRAYSIZE( nameList );
		nameIndex = RandomInt( 0, nameCount );
	}

	const char *name = nameList[nameIndex];

	++nameIndex;
	if ( nameIndex >= nameCount )
		nameIndex = 0;

	return name;
}

bool UTIL_KickBotFromTeam( int teamNum )
{
	// a ForEachPlayer functor goes here

	// find a dead guy first, so we don't disrupt combat
	for ( int i=1; i<gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );
		if ( player == nullptr || FNullEnt( player->edict() ) || !player->IsPlayer() || !player->IsConnected() )
			continue;

		CTFBot *bot = dynamic_cast<CTFBot *>( player );
		if ( bot == nullptr )
			continue;

		if ( player->GetTeamNumber() != teamNum || player->IsAlive() )
			continue;

		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", engine->GetPlayerUserId( player->edict() ) ) );
		return true;
	}
	
	// go with anyone otherwise
	for ( int i=1; i<gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );
		if ( player == nullptr || FNullEnt( player->edict() ) || !player->IsPlayer() || !player->IsConnected() )
			continue;

		CTFBot *bot = dynamic_cast<CTFBot *>( player );
		if ( bot == nullptr )
			continue;

		if ( player->GetTeamNumber() != teamNum )
			continue;

		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", engine->GetPlayerUserId( player->edict() ) ) );
		return true;
	}

	return false;
}

const char *DifficultyToName( int iSkillLevel )
{
	switch ( iSkillLevel )
	{
		default:
		case CTFBot::EASY:
			return "Easy ";
		case CTFBot::NORMAL:
			return "Normal ";
		case CTFBot::HARD:
			return "Hard ";
		case CTFBot::EXPERT:
			return "Expert ";
	}
}
int NameToDifficulty( const char *pszSkillName )
{
	if ( !Q_stricmp( pszSkillName, "expert" ) )
		return CTFBot::EXPERT;
	else if ( !Q_stricmp( pszSkillName, "hard" ) )
		return CTFBot::HARD;
	else if ( !Q_stricmp( pszSkillName, "normal" ) )
		return CTFBot::NORMAL;

	return CTFBot::EASY;
}

void CreateBotName( int iTeamNum, int iClassIdx, int iSkillLevel, char *out, int outlen )
{
	char szName[64] = "", szRelationship[16] = "", szDifficulty[8] = "";
	if ( TFGameRules()->IsInTraining() )
	{
		int iHumanTeam = TFGameRules()->GetAssignedHumanTeam();
		if ( iHumanTeam != TEAM_ANY )
		{
			const wchar_t *szLRelations = nullptr;
			if ( iTeamNum == iHumanTeam )
			{
				szLRelations = g_pVGuiLocalize->Find( "#TF_Bot_Title_Friendly" );
			}
			else
			{
				szLRelations = g_pVGuiLocalize->Find( "#TF_Bot_Title_Enemy" );
			}

			if ( szLRelations )
				g_pVGuiLocalize->ConvertUnicodeToANSI( szLRelations, szRelationship, sizeof( szRelationship ) );
		}

		const wchar_t *szLClassname = nullptr;
		if ( ( iClassIdx-1 ) < TF_LAST_NORMAL_CLASS )
		{
			szLClassname = g_pVGuiLocalize->Find( g_aPlayerClassNames[iClassIdx] );
		}
		else
		{
			szLClassname = g_pVGuiLocalize->Find( "#TF_Bot_Generic_ClassName" );
		}

		if ( szLClassname )
			g_pVGuiLocalize->ConvertUnicodeToANSI( szLClassname, szName, sizeof( szName ) );
	}
	else
	{
		Q_strncpy( szName, GetRandomBotName(), sizeof( szName ) );
	}

	if ( tf_bot_prefix_name_with_difficulty.GetBool() )
	{
		Q_strncpy( szDifficulty, DifficultyToName( iSkillLevel ), sizeof( szDifficulty ) );
	}

	Q_strncpy( out, CFmtStr( "%s%s%s", szDifficulty, szRelationship, szName ), outlen );
}


CTFBotManager sTFBotManager;
CTFBotManager &TheTFBots()
{
	return sTFBotManager;
}


CTFBotManager::CTFBotManager()
{
	NextBotManager::SetInstance( this );

	m_flQuotaChangeTime = 0.0f;
}

CTFBotManager::~CTFBotManager()
{
	NextBotManager::SetInstance( NULL );
}


void CTFBotManager::Update()
{
	MaintainBotQuota();

	NextBotManager::Update();
}


void CTFBotManager::OnMapLoaded()
{
	LoadBotNames();

	NextBotManager::OnMapLoaded();
}

void CTFBotManager::OnRoundRestart()
{
	NextBotManager::OnRoundRestart();
}

void CTFBotManager::OnLevelShutdown()
{
	m_BotNames.Purge();
	m_flQuotaChangeTime = 0.0f;

	if ( IsInOfflinePractice() )
		RevertOfflinePracticeConvars();
}


bool CTFBotManager::IsInOfflinePractice() const
{
	return tf_bot_offline_practice.GetBool();
}

void CTFBotManager::SetIsInOfflinePractice( bool set )
{
	tf_bot_offline_practice.SetValue( set );
}

void CTFBotManager::RevertOfflinePracticeConvars()
{
	tf_bot_quota.Revert();
	tf_bot_quota_mode.Revert();
	tf_bot_auto_vacate.Revert();
	tf_bot_difficulty.Revert();
	tf_bot_offline_practice.Revert();
}


void CTFBotManager::OnForceAddedBots( int count )
{
	tf_bot_quota.SetValue( Min( gpGlobals->maxClients, count + tf_bot_quota.GetInt() ) );
	m_flQuotaChangeTime = gpGlobals->curtime + 1.0f;
}

void CTFBotManager::OnForceKickedBots( int count )
{
	tf_bot_quota.SetValue( Max( 0, tf_bot_quota.GetInt() - count ) );
	m_flQuotaChangeTime = gpGlobals->curtime + 2.0f;
}


bool CTFBotManager::IsAllBotTeam( int teamNum )
{
	CTeam *team = GetGlobalTeam( teamNum );
	if ( team == nullptr )
		return false;

	if ( team->GetNumPlayers() > 0 )
	{
		for ( int i=0; i<team->GetNumPlayers(); ++i )
		{
			CBasePlayer *player = team->GetPlayer( i );
			if ( player->IsNetClient() && !player->IsFakeClient() )
				return false;
		}
	}
	else
	{
		return teamNum != TFGameRules()->GetAssignedHumanTeam();
	}

	return true;
}


bool CTFBotManager::IsMeleeOnly() const
{
	return tf_bot_melee_only.GetBool();
}


void CTFBotManager::MaintainBotQuota()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( TheNavMesh->IsGenerating() || g_fGameOver ) return;
	if ( !TFGameRules() || TFGameRules()->IsInTraining() ) return;
	if ( !engine->IsDedicatedServer() && !UTIL_GetListenServerHost() ) return;
	if ( gpGlobals->curtime < m_flQuotaChangeTime ) return;

	m_flQuotaChangeTime = gpGlobals->curtime + 0.25f;

	int nPlayersTotal = 0;
	int nTFBotsTotal  = 0;
	int nTFBotsTeamed = 0;
	int nHumansTeamed = 0;
	int nHumansSpec   = 0;

	for ( int i = 1; i < gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );

		if ( player == nullptr || FNullEnt( player->edict() ) )
			continue;
		if ( !player->IsPlayer() || !player->IsConnected() )
			continue;

		CTFBot *bot = dynamic_cast<CTFBot *>( player );
		if ( bot != nullptr )
		{
			++nTFBotsTotal;
			if ( bot->GetTeamNumber() == TF_TEAM_RED || bot->GetTeamNumber() == TF_TEAM_BLUE )
			{
				++nTFBotsTeamed;
			}
		}
		else
		{
			if ( player->GetTeamNumber() == TF_TEAM_RED || player->GetTeamNumber() == TF_TEAM_BLUE )
			{
				++nHumansTeamed;
			}
			else if ( player->GetTeamNumber() == TEAM_SPECTATOR )
			{
				++nHumansSpec;
			}
		}

		++nPlayersTotal;
	}

	int nHumansTotal = nPlayersTotal - nTFBotsTotal;

	int nDesired = tf_bot_quota.GetInt();

	if ( FStrEq( tf_bot_quota_mode.GetString(), "fill" ) )
	{
		nDesired = Max( 0, nDesired - nHumansTeamed );
	}
	else if ( FStrEq( tf_bot_quota_mode.GetString(), "match" ) )
	{
		nDesired = Max( 0, tf_bot_quota.GetInt() * nHumansTeamed );
	}

	if ( tf_bot_join_after_player.GetBool() )
	{
		if ( nHumansTeamed == 0 )
		{
			nDesired = 0;
		}
	}

	if ( tf_bot_auto_vacate.GetBool() )
	{
		nDesired = Min( nDesired, gpGlobals->maxClients - ( nHumansTotal + 1 ) );
	}
	else
	{
		nDesired = Min( nDesired, gpGlobals->maxClients - nHumansTotal );
	}

	if ( nDesired > nTFBotsTeamed )
	{
		if ( !TFGameRules()->WouldChangeUnbalanceTeams( TF_TEAM_BLUE, TEAM_UNASSIGNED ) ||
			 !TFGameRules()->WouldChangeUnbalanceTeams( TF_TEAM_RED, TEAM_UNASSIGNED ) )
		{
			extern ConVar tf_bot_force_class;

			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>( GetRandomBotName() );

			if ( bot != nullptr )
			{
				bot->HandleCommand_JoinTeam( "auto" );

				const char *szClassname;
				if ( FStrEq( tf_bot_force_class.GetString(), "" ) )
				{
					szClassname = bot->GetNextSpawnClassname();
				}
				else
				{
					szClassname = tf_bot_force_class.GetString();
				}

				bot->HandleCommand_JoinClass( szClassname );

				char szName[256];
				CreateBotName( bot->GetTeamNumber(), bot->GetPlayerClass()->GetClassIndex(), tf_bot_difficulty.GetInt(), szName, sizeof( szName ) );
				engine->SetFakeClientConVarValue( bot->edict(), "name", szName );
			}
		}
	}
	else if ( nDesired < nTFBotsTeamed )
	{
		if ( !UTIL_KickBotFromTeam( TEAM_UNASSIGNED ) )
		{
			CTeam *team_red = GetGlobalTeam( TF_TEAM_RED );
			CTeam *team_blu = GetGlobalTeam( TF_TEAM_BLUE );

			int team_kick = TF_TEAM_BLUE;
			if ( team_red->GetNumPlayers() >= team_blu->GetNumPlayers() )
			{
				team_kick = TF_TEAM_RED;
				if ( team_red->GetNumPlayers() <= team_blu->GetNumPlayers() )
				{
					team_kick = TF_TEAM_BLUE;
					if ( team_red->GetScore() >= team_blu->GetScore() )
					{
						team_kick = TF_TEAM_RED;
						if ( team_red->GetScore() <= team_blu->GetScore() )
						{
							team_kick = RandomInt( 0, 1 ) == 1 ? TF_TEAM_RED : TF_TEAM_BLUE;
						}
					}
				}
			}

			if ( !UTIL_KickBotFromTeam( team_kick ) )
			{
				UTIL_KickBotFromTeam( team_kick == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED );
			}
		}
	}
}

const char *CTFBotManager::GetRandomBotName()
{
	static char szName[64];
	if( m_BotNames.Count() == 0 )
		return "Unnamed";

	Q_strncpy( szName,
			   STRING( m_BotNames[ RandomInt( 0, m_BotNames.Count() - 1 ) ] ),
			   sizeof szName );

	return szName;
}

void CTFBotManager::ReloadBotNames()
{
	m_BotNames.RemoveAll();
	LoadBotNames();
}

#define BOT_NAMES_FILE	"scripts/tf_bot_names.txt"

bool CTFBotManager::LoadBotNames()
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_OTHER_FILESYSTEM );

	if ( g_pFullFileSystem == nullptr )
		return false;

	KeyValues *pBotNames = new KeyValues( "BotNames" );
	if ( !pBotNames->LoadFromFile( g_pFullFileSystem, BOT_NAMES_FILE, "MOD" ) )
	{
		Warning( "CTFBotManager: Could not load %s", BOT_NAMES_FILE );
		pBotNames->deleteThis();
		return false;
	}

	for ( KeyValues *pSubData = pBotNames->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
	{
		if ( FStrEq( pSubData->GetString(), "" ) )
			continue;

		string_t iName = AllocPooledString( pSubData->GetString() );
		if ( m_BotNames.Find( iName ) == m_BotNames.InvalidIndex() )
			m_BotNames[ m_BotNames.AddToTail() ] = iName;
	}

	return true;
}

void CC_ReloadBotNames( const CCommand &args )
{
	TheTFBots().ReloadBotNames();
}

ConCommand tf_bot_names_reload( "tf_bot_names_reload", CC_ReloadBotNames, "Reload all names for TFBots.", FCVAR_CHEAT );
