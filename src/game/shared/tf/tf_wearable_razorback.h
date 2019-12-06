#ifndef TF_WEARABLE_RAZORBACK_H
#define TF_WEARABLE_RAZORBACK_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_wearable.h"

#if defined CLIENT_DLL
#define CTFWearableRazorBack C_TFWearableRazorBack
#endif

class CTFWearableRazorBack : public CTFWearable
{
	DECLARE_CLASS(CTFWearableRazorBack, CTFWearable)
public:
	DECLARE_NETWORKCLASS()

	CTFWearableRazorBack();
	virtual ~CTFWearableRazorBack();

	virtual void	Precache(void);

#ifdef GAME_DLL
	virtual void	Equip(CBasePlayer *pPlayer);
	virtual void	UnEquip(CBasePlayer *pPlayer);

#endif

private:

	CTFWearableRazorBack(const CTFWearableRazorBack&) {}
};


extern CTFWearableRazorBack *GetEquippedRazorBack(CTFPlayer *pPlayer);

#endif
