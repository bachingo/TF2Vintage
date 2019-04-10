#ifndef TF_WEAPON_STICKBOMB_H
#define TF_WEAPON_STICKBOMB_H
#ifdef _WIN32
#pragma once
#endif


// OTHER THINGS:
// - scripts/tf_weapon_stickbomb.txt
// - scripts/items/items_game.txt
// - materials/
// - models/


#include "tf_weapon_bottle.h"


#ifdef CLIENT_DLL
#define CTFStickBomb C_TFStickBomb
#endif


class CTFStickBomb : public CTFBottle
{
public:
	DECLARE_CLASS( CTFStickBomb, CTFBottle );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CTFStickBomb();
	CTFStickBomb( const CTFStickBomb& ) = delete;
	
	virtual int			GetWeaponID() const { return TF_WEAPON_STICKBOMB; }
	virtual const char	*GetWorldModel() const;
	virtual void		Precache();
	virtual void		Smack();
	virtual void		SwitchBodyGroups();
	virtual void		WeaponRegenerate();
	virtual void		WeaponReset();
	
#ifdef CLIENT_DLL
	virtual int			GetWorldModelIndex();
#endif
	
private:
	CNetworkVar( int, m_iDetonated );
};


#endif // TF_WEAPON_STICKBOMB_H
