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

public:
	CEconEntity();
	virtual ~CEconEntity() {};

#ifdef CLIENT_DLL
	virtual void OnPreDataChanged( DataUpdateType_t );
	virtual void OnDataChanged( DataUpdateType_t );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual bool IsOverridingViewmodel( void ) const;

	C_ViewmodelAttachmentModel *GetViewmodelAddon( void ) const;

	virtual int InternalDrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual void ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	virtual bool GetAttachment( char const *pszName, Vector &absOrigin ) { return BaseClass::GetAttachment( pszName, absOrigin ); }
	virtual bool GetAttachment( char const *pszName, Vector &absOrigin, QAngle &absAngles ) { return BaseClass::GetAttachment( pszName, absOrigin, absAngles ); }
	virtual bool GetAttachment( int iAttachment, Vector &absOrigin );
	virtual bool GetAttachment( int iAttachment, Vector &absOrigin, QAngle &absAngles );
	virtual bool GetAttachment( int iAttachment, matrix3x4_t &matrix );

	virtual void UpdateAttachmentModels( void );
	virtual bool AttachmentModelsShouldBeVisible( void ) const { return true; }

	IMaterial *GetMaterialOverride( int iIndex ) const;
	virtual void SetMaterialOverride( int iIndex, const char *pszMaterial );
	virtual void SetMaterialOverride( int iIndex, CMaterialReference &material );

	struct AttachedModelData_t
	{
		int  modeltype;
		const model_t *model;
	};
	CUtlVector<AttachedModelData_t> m_aAttachments;

	CMaterialReference m_aMaterials[4];
#endif

	virtual int TranslateViewmodelHandActivity( int iActivity ) { return iActivity; }

	virtual void PlayAnimForPlaybackEvent(wearableanimplayback_t iPlayback) {};

	void SetItem( CEconItemView const &pItem );
	CEconItemView *GetItem() const { return m_AttributeManager.GetItem(); }
	bool HasItemDefinition() const;
	int GetItemID() const;

	virtual void GiveTo( CBaseEntity *pEntity );

	virtual CAttributeManager *GetAttributeManager( void ) { return &m_AttributeManager; }
	virtual CAttributeContainer *GetAttributeContainer( void ) { return &m_AttributeManager; }
	virtual CBaseEntity *GetAttributeOwner( void ) { return NULL; }
	virtual CAttributeList *GetAttributeList( void ) { return &m_AttributeList; }
	virtual void ReapplyProvision( void );
	void InitializeAttributes( void );

#ifdef GAME_DLL
	void UpdateModelToClass( void );
#endif

	virtual void UpdatePlayerBodygroups( void );

	virtual void UpdateOnRemove( void );

protected:
	EHANDLE m_hOldOwner;

private:
	CNetworkVarEmbedded( CAttributeContainer, m_AttributeManager );

	CAttributeList m_AttributeList;
#ifdef CLIENT_DLL
	CHandle<C_ViewmodelAttachmentModel> m_hAttachmentParent;
#endif
};

#ifdef CLIENT_DLL
inline C_ViewmodelAttachmentModel *C_EconEntity::GetViewmodelAddon( void ) const
{
	return m_hAttachmentParent;
}

inline IMaterial *C_EconEntity::GetMaterialOverride( int iIndex ) const
{
	if ( iIndex < 0 || iIndex > 4 )
		return NULL;

	return m_aMaterials[ iIndex ];
}

void DrawEconEntityAttachedModels( C_BaseAnimating *pAnimating, C_EconEntity *pEconEntity, ClientModelRenderInfo_t const *pInfo, int iModelType );
#endif

#endif
