// Purpose: VR Hand Rendering Layer
// Renders VR hands/weapons on a separate layer from the world scene.
// World depth is preserved for occlusion while allowing independent
// control over the hand rendering pass (zNear, effects, sniper scope, etc.)

#ifndef VR_HAND_RENDER_H
#define VR_HAND_RENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "iclientrenderable.h"

// Call at the start of the main world scene pass to begin collecting
// hand renderables that should be deferred to the hand layer pass.
void VRHandLayer_BeginWorldPass();

// Call at the end of the main world scene pass.
void VRHandLayer_EndWorldPass();

// Returns true if the VR hand layer is enabled and we're currently
// in the world rendering pass (meaning hands should skip drawing
// and register themselves for the hand layer instead).
bool VRHandLayer_ShouldSkipDraw();

// Returns true if we're currently drawing the hand layer pass
// (hands should draw normally in this pass).
bool VRHandLayer_IsHandPass();

// Register a renderable to be drawn during the hand layer pass.
// Called from DrawModel of hand/weapon entities when they detect
// they should skip world-pass drawing.
void VRHandLayer_AddRenderable(IClientRenderable *pRenderable);

// Get the collected renderables for drawing.
int VRHandLayer_GetRenderableCount();
IClientRenderable *VRHandLayer_GetRenderable(int index);

// Clear all collected renderables after drawing.
void VRHandLayer_ClearRenderables();

// Check if the VR hand layer system is enabled.
bool VRHandLayer_IsEnabled();

// Get the zNear override for the hand layer (0 = use world zNear).
float VRHandLayer_GetZNearOverride();

// Get the zFar override for the hand layer (0 = use world zFar).
float VRHandLayer_GetZFarOverride();

// Set whether the hand pass is currently active (used internally by DrawVRHands).
void VRHandLayer_SetHandPassActive(bool bActive);

// Particle deferral: weapon/hand particles skip the world translucent pass
// and render in the hand layer pass for correct depth ordering.
class C_BaseEntity;
void VRHandLayer_AddParticleOwner(C_BaseEntity *pEntity);
bool VRHandLayer_IsParticleOwner(C_BaseEntity *pEntity);
void VRHandLayer_AddDeferredParticle(IClientRenderable *pParticle);
int VRHandLayer_GetDeferredParticleCount();
IClientRenderable *VRHandLayer_GetDeferredParticle(int index);
void VRHandLayer_ClearDeferredParticles();

#endif // VR_HAND_RENDER_H
