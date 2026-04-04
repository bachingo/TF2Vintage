//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined( USERCMD_H )
#define USERCMD_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "utlvector.h"
#include "imovehelper.h"
#include "checksum_crc.h"


class bf_read;
class bf_write;

class CEntityGroundContact
{
public:
	int					entindex;
	float				minheight;
	float				maxheight;
};

class CUserCmd
{
public:
	CUserCmd()
	{
		Reset();
	}

	virtual ~CUserCmd() { };

	void Reset()
	{
		command_number = 0;
		tick_count = 0;
		viewangles.Init();
		postFullBodyIKDeltaOrigin.Init();
		playerToHmdOrigin.Init();
		playerToHmdAngles.Init();
		clientEyePosition.Init();
		leftControllerOrigin.Init();
		leftControllerAngles.Init();
		rightControllerOrigin.Init();
		rightControllerAngles.Init();
		vrIKHandPosL.Init();
		vrIKHandAngL.Init();
		vrIKHandPosR.Init();
		vrIKHandAngR.Init();
		vrThrowVelocity.Init();
		vrThrowOrigin.Init();
		vrThrowAngles.Init();
		vrThrowAngVel.Init();
		vrMeleeGripSpeed = 0.0f;
		vrMeleeGripSpeedLeft = 0.0f;
		vrBallAimActive = false;
		vrPhysicalCrouch = false;
		forwardmove = 0.0f;
		sidemove = 0.0f;
		upmove = 0.0f;
		buttons = 0;
		impulse = 0;
		weaponselect = 0;
		weaponsubtype = 0;
		random_seed = 0;
#ifdef GAME_DLL
		server_random_seed = 0;
#endif
		mousedx = 0;
		mousedy = 0;

		hasbeenpredicted = false;
#if defined( HL2_DLL ) || defined( HL2_CLIENT_DLL )
		entitygroundcontact.RemoveAll();
#endif
	}

	CUserCmd& operator =( const CUserCmd& src )
	{
		if ( this == &src )
			return *this;

		command_number		= src.command_number;
		tick_count			= src.tick_count;
		viewangles			= src.viewangles;
		postFullBodyIKDeltaOrigin = src.postFullBodyIKDeltaOrigin;
		playerToHmdOrigin = src.playerToHmdOrigin;
		playerToHmdAngles = src.playerToHmdAngles;
		clientEyePosition = src.clientEyePosition;
		leftControllerOrigin = src.leftControllerOrigin;
		leftControllerAngles = src.leftControllerAngles;
		rightControllerOrigin = src.rightControllerOrigin;
		rightControllerAngles = src.rightControllerAngles;
		vrIKHandPosL		= src.vrIKHandPosL;
		vrIKHandAngL		= src.vrIKHandAngL;
		vrIKHandPosR		= src.vrIKHandPosR;
		vrIKHandAngR		= src.vrIKHandAngR;
		vrThrowVelocity		= src.vrThrowVelocity;
		vrThrowOrigin		= src.vrThrowOrigin;
		vrThrowAngles		= src.vrThrowAngles;
		vrThrowAngVel		= src.vrThrowAngVel;
		vrMeleeGripSpeed	= src.vrMeleeGripSpeed;
		vrMeleeGripSpeedLeft = src.vrMeleeGripSpeedLeft;
		vrBallAimActive		= src.vrBallAimActive;
		vrPhysicalCrouch	= src.vrPhysicalCrouch;
		forwardmove			= src.forwardmove;
		sidemove			= src.sidemove;
		upmove				= src.upmove;
		buttons				= src.buttons;
		impulse				= src.impulse;
		weaponselect		= src.weaponselect;
		weaponsubtype		= src.weaponsubtype;
		random_seed			= src.random_seed;
#ifdef GAME_DLL
		server_random_seed = src.server_random_seed;
#endif
		mousedx				= src.mousedx;
		mousedy				= src.mousedy;

		hasbeenpredicted	= src.hasbeenpredicted;

#if defined( HL2_DLL ) || defined( HL2_CLIENT_DLL )
		entitygroundcontact			= src.entitygroundcontact;
#endif

		return *this;
	}

	CUserCmd( const CUserCmd& src )
	{
		*this = src;
	}

	CRC32_t GetChecksum( void ) const
	{
		CRC32_t crc;

		CRC32_Init( &crc );
		CRC32_ProcessBuffer( &crc, &command_number, sizeof( command_number ) );
		CRC32_ProcessBuffer( &crc, &tick_count, sizeof( tick_count ) );
		CRC32_ProcessBuffer( &crc, &viewangles, sizeof( viewangles ) );    
		CRC32_ProcessBuffer( &crc, &postFullBodyIKDeltaOrigin, sizeof( postFullBodyIKDeltaOrigin ) );
		CRC32_ProcessBuffer( &crc, &playerToHmdOrigin, sizeof( playerToHmdOrigin ) );
		CRC32_ProcessBuffer( &crc, &playerToHmdAngles, sizeof( playerToHmdAngles ) );
		CRC32_ProcessBuffer( &crc, &leftControllerOrigin, sizeof( leftControllerOrigin ) );
		CRC32_ProcessBuffer( &crc, &leftControllerAngles, sizeof( leftControllerAngles ) );
		CRC32_ProcessBuffer( &crc, &rightControllerOrigin, sizeof( rightControllerOrigin ) );
		CRC32_ProcessBuffer( &crc, &rightControllerAngles, sizeof( rightControllerAngles ) );
		CRC32_ProcessBuffer( &crc, &vrIKHandPosL, sizeof( vrIKHandPosL ) );
		CRC32_ProcessBuffer( &crc, &vrIKHandAngL, sizeof( vrIKHandAngL ) );
		CRC32_ProcessBuffer( &crc, &vrIKHandPosR, sizeof( vrIKHandPosR ) );
		CRC32_ProcessBuffer( &crc, &vrIKHandAngR, sizeof( vrIKHandAngR ) );
		CRC32_ProcessBuffer( &crc, &vrThrowVelocity, sizeof( vrThrowVelocity ) );
		CRC32_ProcessBuffer( &crc, &vrThrowOrigin, sizeof( vrThrowOrigin ) );
		CRC32_ProcessBuffer( &crc, &vrThrowAngles, sizeof( vrThrowAngles ) );
		CRC32_ProcessBuffer( &crc, &vrThrowAngVel, sizeof( vrThrowAngVel ) );
		CRC32_ProcessBuffer( &crc, &vrMeleeGripSpeed, sizeof( vrMeleeGripSpeed ) );
		CRC32_ProcessBuffer( &crc, &vrMeleeGripSpeedLeft, sizeof( vrMeleeGripSpeedLeft ) );
		CRC32_ProcessBuffer( &crc, &vrBallAimActive, sizeof( vrBallAimActive ) );
		CRC32_ProcessBuffer( &crc, &vrPhysicalCrouch, sizeof( vrPhysicalCrouch ) );
		CRC32_ProcessBuffer( &crc, &forwardmove, sizeof( forwardmove ) );   
		CRC32_ProcessBuffer( &crc, &sidemove, sizeof( sidemove ) );      
		CRC32_ProcessBuffer( &crc, &upmove, sizeof( upmove ) );         
		CRC32_ProcessBuffer( &crc, &buttons, sizeof( buttons ) );		
		CRC32_ProcessBuffer( &crc, &impulse, sizeof( impulse ) );        
		CRC32_ProcessBuffer( &crc, &weaponselect, sizeof( weaponselect ) );	
		CRC32_ProcessBuffer( &crc, &weaponsubtype, sizeof( weaponsubtype ) );
		CRC32_ProcessBuffer( &crc, &random_seed, sizeof( random_seed ) );
		CRC32_ProcessBuffer( &crc, &mousedx, sizeof( mousedx ) );
		CRC32_ProcessBuffer( &crc, &mousedy, sizeof( mousedy ) );
		CRC32_Final( &crc );

		return crc;
	}

	// Allow command, but negate gameplay-affecting values
	void MakeInert( void )
	{
		viewangles = vec3_angle;
		playerToHmdOrigin.Init();
        playerToHmdAngles.Init();
		leftControllerOrigin.Init();
		leftControllerAngles.Init();
		rightControllerOrigin.Init();
		rightControllerAngles.Init();
		vrIKHandPosL.Init();
		vrIKHandAngL.Init();
		vrIKHandPosR.Init();
		vrIKHandAngR.Init();
		vrThrowVelocity.Init();
		vrThrowOrigin.Init();
		vrThrowAngles.Init();
		vrThrowAngVel.Init();
		vrMeleeGripSpeed = 0.0f;
		vrMeleeGripSpeedLeft = 0.0f;
		vrBallAimActive = false;
		vrPhysicalCrouch = false;
		forwardmove = 0.f;
		sidemove = 0.f;
		upmove = 0.f;
		buttons = 0;
		impulse = 0;
	}

	// These functions are needed for exposing CUserCmd to VScript.
	int		command_number;
	
	// the tick the client created this command
	int		tick_count;
	
	// Player instantaneous view angles.
	QAngle	viewangles;     
	
	// HMD Tracking
	Vector	postFullBodyIKDeltaOrigin;
	Vector	playerToHmdOrigin;
	QAngle	playerToHmdAngles;
	Vector	clientEyePosition;  // Direct eye position from client for collision detection
	
	// VR Controller Tracking for weapon shooting
	Vector	leftControllerOrigin;
	QAngle	leftControllerAngles;
	Vector	rightControllerOrigin;
	QAngle	rightControllerAngles;

	// VR IK: raw controller grip positions for third-person arm IK (always grip pose, never muzzle)
	Vector	vrIKHandPosL;
	QAngle	vrIKHandAngL;
	Vector	vrIKHandPosR;
	QAngle	vrIKHandAngR;

	// VR physical throw (set on grip/trigger release for throwable weapons)
	Vector	vrThrowVelocity;
	Vector	vrThrowOrigin;		// player-relative offset (reconstructed on server)
	QAngle	vrThrowAngles;		// hand orientation at moment of release
	Vector	vrThrowAngVel;		// angular velocity of hand (deg/sec, as Vector)

	// VR melee: grip speed computed client-side in tracking space (u/s)
	float	vrMeleeGripSpeed;
	float	vrMeleeGripSpeedLeft;

	// VR ball aim: true when offhand trigger is held with a ball-launching bat
	bool	vrBallAimActive;

	// VR physical crouch: true when player is physically crouching (HMD below threshold)
	bool	vrPhysicalCrouch;

	// Intended velocities
	//	forward velocity.
	float	forwardmove;   
	//  sideways velocity.
	float	sidemove;      
	//  upward velocity.
	float	upmove;         
	// Attack button states
	int		buttons;		
	// Impulse command issued.
	byte    impulse;        
	// Current weapon id
	int		weaponselect;	
	int		weaponsubtype;

	int		random_seed;	// For shared random functions
#ifdef GAME_DLL
	int		server_random_seed; // Only the server populates this seed
#endif

	short	mousedx;		// mouse accum in x from create move
	short	mousedy;		// mouse accum in y from create move

	// Client only, tracks whether we've predicted this command at least once
	bool	hasbeenpredicted;

	// Back channel to communicate IK state
#if defined( HL2_DLL ) || defined( HL2_CLIENT_DLL )
	CUtlVector< CEntityGroundContact > entitygroundcontact;
#endif

};

void ReadUsercmd( bf_read *buf, CUserCmd *move, CUserCmd *from );
void WriteUsercmd( bf_write *buf, const CUserCmd *to, const CUserCmd *from );

#endif // USERCMD_H
