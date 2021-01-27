//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef C_TF_MVM_BOSS_HEALTH_USER_H
#define C_TF_MVM_BOSS_HEALTH_USER_H

#ifdef _WIN32
#pragma once
#endif

DECLARE_AUTO_LIST( ITFMvMBossProgressUserAutoList );

class C_TFMvMBossProgressUser : public ITFMvMBossProgressUserAutoList
{
public:
	virtual const char* GetBossProgressImageName() const { return NULL; }
	virtual float GetBossStatusProgress() const { return 0.f; }
};

#endif
