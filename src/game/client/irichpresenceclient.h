#ifndef IRICHPRESENCE_H
#define IRICHPRESENCE_H
#ifdef _WIN32
#pragma once
#endif

//--------------------------------------------------------------------
// Purpose: Discord Rich Presence interface
//--------------------------------------------------------------------
abstract_class IRichPresenceClient
{
public:
	virtual bool		InitPresence( void ) = 0;
	virtual void		ResetPresence( void ) = 0;
	virtual void		UpdatePresence( void ) = 0;
	virtual void		SetLevelName( char const *pMapName ) = 0;
	virtual char const *GetMatchSecret( void ) const = 0; // Return NULL if a request isn't allowed
	virtual char const *GetJoinSecret( void ) const = 0; // Return NULL if a request isn't allowed
	virtual char const *GetSpectateSecret( void ) const = 0; // Return NULL if a request isn't allowed
	virtual unsigned char const *GetEncryptionKey( void ) const = 0;
};

extern IRichPresenceClient *rpc;
#endif