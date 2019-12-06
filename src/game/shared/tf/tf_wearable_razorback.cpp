#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

#include "tf_gamerules.h"
#include "tf_wearable_razorback.h"


IMPLEMENT_NETWORKCLASS_ALIASED(TFWearableRazorBack, DT_TFWearableRazorBack);

BEGIN_NETWORK_TABLE(CTFWearableRazorBack, DT_TFWearableRazorBack)
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(tf_wearable_razorback, CTFWearableRazorBack);
PRECACHE_REGISTER(tf_wearable_razorback);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWearableRazorBack::CTFWearableRazorBack()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWearableRazorBack::~CTFWearableRazorBack()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableRazorBack::Precache(void)
{
	BaseClass::Precache();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableRazorBack::Equip(CBasePlayer *pPlayer)
{
	BaseClass::Equip(pPlayer);

	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (pTFPlayer) pTFPlayer->m_Shared.SetDemoShieldEquipped(true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableRazorBack::UnEquip(CBasePlayer *pPlayer)
{
	BaseClass::UnEquip(pPlayer);

	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (pTFPlayer) pTFPlayer->m_Shared.SetDemoShieldEquipped(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWearableRazorBack *GetEquippedRazorBack(CTFPlayer *pPlayer)
{
	for (int i = 0; i<pPlayer->GetNumWearables(); ++i)
	{
		CTFWearableRazorBack *pShield = dynamic_cast<CTFWearableRazorBack *>(pPlayer->GetWearable(i));
		if (pShield)
			return pShield;
	}

	return nullptr;
}
#endif
