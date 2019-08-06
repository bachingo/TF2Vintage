//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef ECON_ENTITY_H
#define ECON_ENTITY_H
#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )
#define CEconEntity C_EconEntity
#endif

#include "ihasattributes.h"
#include "econ_item_view.h"
#include "attribute_manager.h"

#ifdef CLIENT_DLL
#include "c_tf_viewmodeladdon.h"
#endif

struct wearableanimplayback_t
{
	int iStub;
};

//-----------------------------------------------------------------------------
// Purpose: BaseCombatWeapon is derived from this in live tf2.
//-----------------------------------------------------------------------------
class CEconEntity : public CBaseAnimating, public IHasAttributes
{
	DECLARE_CLASS( CEconEntity, CBaseAnimating );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif

public:
	CEconEntity();
	~CEconEntity();

#ifdef CLIENT_DLL
	virtual void OnPreDataChanged( DataUpdateType_t );
	virtual void OnDataChanged( DataUpdateType_t );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual C_ViewmodelAttachmentModel *GetViewmodelAddon( void ) { return m_hAttachmentParent; }

	virtual int InternalDrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual void ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	virtual void UpdateAttachmentModels( void );
	virtual bool AttachmentModelsShouldBeVisible( void ) const { return true; }

	virtual void SetMaterialOverride( int iTeam, const char *pszMaterial );
	virtual void SetMaterialOverride( int iTeam, CMaterialReference &material );

	struct AttachedModelData_t
	{
		int  modeltype;
		const model_t *model;
	};
	CUtlVector<AttachedModelData_t> m_aAttachments;

	CMaterialReference m_MaterialOverrides[TF_TEAM_COUNT];
#endif

	virtual int TranslateViewmodelHandActivity( int iActivity ) { return iActivity; }

	virtual void PlayAnimForPlaybackEvent(wearableanimplayback_t iPlayback) {};

	virtual void SetItem( CEconItemView &newItem );
	CEconItemView *GetItem();
	virtual bool HasItemDefinition() const;
	virtual int GetItemID();

	virtual void GiveTo( CBaseEntity *pEntity );

	virtual CAttributeManager *GetAttributeManager() { return &m_AttributeManager; }
	virtual CAttributeContainer *GetAttributeContainer() { return &m_AttributeManager; }
	virtual CBaseEntity *GetAttributeOwner() { return NULL; }
	virtual void ReapplyProvision( void );

	void UpdatePlayerModelToClass( void );

	virtual void UpdatePlayerBodygroups( void );

	virtual void UpdateOnRemove( void );

protected:
	EHANDLE m_hOldOwner;
	CEconItemView m_Item;

private:
	CNetworkVarEmbedded( CAttributeContainer, m_AttributeManager );

#ifdef CLIENT_DLL
	CHandle<C_ViewmodelAttachmentModel> m_hAttachmentParent;
#endif
};

#ifdef CLIENT_DLL
void DrawEconEntityAttachedModels( C_BaseAnimating *pAnimating, C_EconEntity *pEconEntity, ClientModelRenderInfo_t const *pInfo, int iModelType );
#endif

#endif
