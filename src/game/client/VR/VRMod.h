#ifndef TF_VRMOD_H
#define TF_VRMOD_H

#include "cbase.h"
#include <stdint.h>
#include "materialsystem/imaterialsystem.h"

// Globals (defined in VRMod.cpp — keep declarations here so multiple TUs can include this header)
extern int VRMod_Started;
extern ITexture *RenderTarget_VRMod;
extern ITexture *RenderTarget_VRMod_GUI;
extern float g_horizontalFOVLeft;
extern float g_horizontalFOVRight;
extern float g_aspectRatioLeft;
extern float g_aspectRatioRight;

// Functions shared with viewrender.cpp and others
void VRMOD_SubmitSharedTexture();

void VRMOD_UpdatePosesAndActions();

void VRMOD_UtilSetOrigin(Vector pos);

void VRMOD_UtilHandleTracking();

void VRMOD_Process_input();

QAngle VRMOD_GetViewAngle();

Vector VRMOD_GetViewOriginLeft();

Vector VRMOD_GetViewOriginRight();

int VRMOD_GetRecWidth();

int VRMOD_GetRecHeight();

Vector VRMOD_GetRecommendedViewmodelAbsPos();

QAngle VRMOD_GetRecommendedViewmodelAbsAngle();

QAngle VRMOD_GetRightControllerAbsAngle();

Vector VRMOD_GetRightControllerAbsPos();

void VRMOD_SetSpawnPlayerHMDAngles();

Vector VRMOD_GetPlayerForward();

Vector VRMOD_GetPlayerRight();

Vector VRMOD_GetPlayerUp();

void RenderHUDQuad(bool bBlackout, bool bTranslucent);

inline bool UseVRMod( void ) { return VRMod_Started == 1; }

#if defined( CLIENT_DLL )
void RenderVRCrosshair();
void VRMOD_DrawLaserPointer();
void RenderGestureQuads(int NumItems);
#endif

#endif // TF_VRMOD_H
