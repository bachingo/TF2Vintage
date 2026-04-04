// Purpose: VR Hand entity - client-only animated hands driven by OpenXR hand tracking

#ifndef C_TFVR_HAND_H
#define C_TFVR_HAND_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "c_baseanimating.h"
#include "openxr/openxr.h"
#include "econ/ihasowner.h"

class C_TFPlayer;
class COpenXRHandTracker;
class C_TFWeaponBase;
class CVRWatchPanel;

// Hand side enum
enum VRHandSide
{
	VR_HAND_LEFT = 0,
	VR_HAND_RIGHT = 1
};

// Medigun fire animation state (fire_on -> fire_loop -> fire_off)
enum MedigunFireState
{
	MEDIGUN_FIRE_IDLE = 0,
	MEDIGUN_FIRE_ON,
	MEDIGUN_FIRE_LOOP,
	MEDIGUN_FIRE_OFF,
};

// Draw animation scope - controls which bones are driven by the draw animation
// Higher values animate more of the arm chain
enum VRDrawAnimScope
{
	VR_DRAW_ANIM_NONE = 0,        // No draw animation
	VR_DRAW_ANIM_WEAPON_BONE,     // Only weapon_bone (weapon shifts/rotates, hand stays idle)
	VR_DRAW_ANIM_WRIST,           // Wrist rotation + fingers + weapon_bone (hand pinned to controller)
	VR_DRAW_ANIM_FULL_ARM,        // Full arm chain follows the animation (hand can displace from controller)
};

//-----------------------------------------------------------------------------
// Purpose: Client-side VR hand entity that renders a single animated hand
//          driven by OpenXR hand tracking data
//          NOTE: This now represents a SINGLE hand (left OR right)
//          Implements IHasOwner so material proxies (cloak, etc.) can find owner
//-----------------------------------------------------------------------------
class C_TFVRHand : public C_BaseAnimating, public IHasOwner
{
	DECLARE_CLASS(C_TFVRHand, C_BaseAnimating);
	DECLARE_CLIENTCLASS();

public:
	C_TFVRHand();
	virtual ~C_TFVRHand();

	// IHasOwner interface - allows material proxies to find owner for cloak effects
	virtual CBaseEntity *GetOwnerViaInterface(void) OVERRIDE;

	// Spawning and lifecycle (spawns two separate hand entities)
	static void SpawnVRHands(C_TFPlayer *pPlayer);
	static void RemoveVRHands(C_TFPlayer *pPlayer);
	
	bool Initialize(C_TFPlayer *pOwner, VRHandSide handSide);
	void Shutdown();

	// Spawn override
	virtual void Spawn() override;

	// Update methods called every frame
	virtual void ClientThink() override;
	void Update(); // Manual update method
	void UpdateHandTransform();
	void UpdateHandTransformFresh(); // Uses freshly sampled XR pose
	void UpdateHandBones();
	
	// Helper to get hand joint transforms as matrix
	bool GetWristTransform(VMatrix& outTransform);
	bool GetPalmTransform(VMatrix& outTransform);  // Palm joint - canonical reference for consistent offsets

	// Bone setup override to position hand bones
	virtual bool SetupBones(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime) override;

	// Suppress animation events on the hand — only allow through:
	//   - Bread Bite fire animations (attack sounds baked into animation)
	//   - Bread creature weapons (draw + idle sounds driven by the hand)
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options ) override
	{
		if ((m_bIsBreadBite && m_bPlayingFireAnim) || m_bAnimateIdle)
			BaseClass::FireEvent(origin, angles, event, options);
	}
	virtual void DoAnimationEvents( CStudioHdr *pStudio ) override
	{
		if ((m_bIsBreadBite && m_bPlayingFireAnim) || m_bAnimateIdle)
			BaseClass::DoAnimationEvents(pStudio);
	}

	// Rendering control
	virtual bool ShouldDraw() override;
	virtual int DrawModel(int flags) override;
	virtual ShadowType_t ShadowCastType() override;  // Always cast shadows
	virtual bool ShouldReceiveProjectedTextures(int flags) override;  // Always receive shadows
	virtual bool IsTransparent() override;  // Returns true when owner is cloaking
	virtual bool GetShadowCastDistance(float *pDist, ShadowType_t shadowType) const override;
	
	// Override to ensure hands are always in PVS
	virtual void GetRenderBounds(Vector& mins, Vector& maxs) override;

	// Bone mapping
	void SetupBoneMapping();
	bool MapOpenXRJointToBone(XrHandJointEXT joint, int &boneIndex);
	
	// Apply finger tracking to bones
	void ApplyFingerTracking(matrix3x4_t *pBoneToWorldOut, int nMaxBones);
	
	// Hide the opposite hand (scale to zero)
	void HideOppositeHand(matrix3x4_t *pBoneToWorldOut, int nMaxBones, CStudioHdr *pStudioHdr);

	// Accessors
	C_TFPlayer* GetOwnerPlayer() const { return m_hOwnerPlayer.Get(); }
	VRHandSide GetHandSide() const { return m_handSide; }
	bool IsLeftHand() const { return m_handSide == VR_HAND_LEFT; }
	bool IsRightHand() const { return m_handSide == VR_HAND_RIGHT; }
	COpenXRHandTracker* GetHandTracker() const { return m_pHandTracker; }
	int GetHandBoneIndex() const { return m_iHandBone; }
	
	// Weapon management
	void EquipWeapon(C_TFWeaponBase *pWeapon);
	void UnequipWeapon();
	C_TFWeaponBase* GetHeldWeapon() const { return m_hHeldWeapon.Get(); }
	C_BaseAnimating* GetRenderWeapon() const { return m_hRenderWeapon.Get(); }
	void UpdateWeaponTransform();
	void UpdateSkins();  // Sync skins for team colors, crit effects, etc.
	void UpdateCritBoostEffect();  // Update crit electricity effect
	
	// Position weapon using already-computed bone matrices (called from SetupBones)
	void PositionWeaponFromBones(matrix3x4_t *pBoneToWorldOut, int nMaxBones);
	
	// Get the weapon's muzzle position and angles in world space
	bool GetWeaponMuzzlePositionAndAngles(Vector &outPos, QAngle &outAngles);
	
	// Get cached weapon bone world transform (for overlays to avoid bone cache issues)
	bool GetCachedWeaponBoneTransform(matrix3x4_t &outTransform) const;
	
	// Weapon pose override
	void ApplyWeaponPose(matrix3x4_t *pBoneToWorldOut, int nMaxBones, C_TFWeaponBase *pWeaponOverride = NULL, int seqOverride = -1, float cycleOverride = 0.0f);
	
	// Fire animation - trigger weapon fire animation
	void PlayWeaponFireAnimation();
	void PlayWeaponAltFireAnimation();
	
	// Draw animation - played when weapon is equipped (deploy/holster)
	void PlayWeaponDrawAnimation();
	bool IsPlayingDrawAnim() const { return m_bPlayingDrawAnim; }
	
	// Charge animation - played while holding attack to charge (sticky launcher, huntsman, etc.)
	// Some weapons have two phases (e.g. huntsman: bw_charge -> bw_shake at max charge)
	void PlayWeaponChargeAnimation();
	void PlayWeaponChargeAnimation2();  // Second phase (e.g. max-charge shake)
	void StopWeaponChargeAnimation();
	bool IsPlayingChargeAnim() const { return m_bPlayingChargeAnim; }
	
	// Medigun fire animation state machine (fire_on -> fire_loop -> fire_off)
	void UpdateMedigunFireAnimation();
	
	// Flamethrower fire animation - driven by weapon fire state
	void UpdateFlamethrowerFireAnimation();
	
	// Two-handed weapon support
	// bUseCurrentAnimation: false = use idle (for stable weapon rotation), true = use current (for visual positioning)
	bool GetOffHandGripTarget(Vector &outPos, QAngle &outAngles, bool bUseCurrentAnimation = false);
	float GetTwoHandBlendAmount() const { return m_flTwoHandBlend; }
	void SetTwoHandBlendAmount(float blend) { m_flTwoHandBlend = blend; }
	bool IsTwoHanding() const { return m_flTwoHandBlend > 0.01f; }
	
	// Offhand grip - when grip button is held and within range
	bool IsOffhandGripActive() const { return m_bOffhandGripActive; }
	bool WasOffhandGripActive() const { return m_bWasOffhandGripActive; }  // For blend-out tracking
	float GetGripRotationBlend() const { return m_flGripRotationBlend; }   // Separate blend for weapon rotation
	const Vector& GetOffhandGripForward() const { return m_vecOffhandGripForward; }
	const Vector& GetOffhandGripUp() const { return m_vecOffhandGripUp; }
	
	// Animation state getters (for cross-hand animation sampling)
	int GetIdleSequenceIndex() const { return m_iIdleSequence; }
	bool IsPlayingFireAnim() const { return m_bPlayingFireAnim; }
	bool ShouldAnimateIdle() const { return m_bAnimateIdle; }
	int GetOffHandBoneIndex() const { return m_iOffHandBone; }

private:
	// Which hand this entity represents
	VRHandSide m_handSide;
	
	// Owner
	CHandle<C_TFPlayer> m_hOwnerPlayer;

	// Held weapon
	CHandle<C_TFWeaponBase> m_hHeldWeapon;  // The actual weapon (for mechanics/firing)
	CHandle<C_BaseAnimating> m_hRenderWeapon;  // Separate render-only entity for visuals
	int m_iLastEquippedWeaponID;  // Weapon ID at equip time (survives entity recreation)

	// Hand tracking
	COpenXRHandTracker *m_pHandTracker;
	bool m_bHandTrackingValid;
	
	// Shutdown flag
	bool m_bShuttingDown;
	
	// Last known player class - to detect class changes
	int m_iLastPlayerClass;

	// Bone index for hand root
	int m_iHandBone;

	// Bone mapping: OpenXR joint index -> Source bone index
	// -1 means no corresponding bone in the model
	// These map ALL XR_HAND_JOINT_COUNT_EXT joints (including fingers)
	int m_BoneMapping[XR_HAND_JOINT_COUNT_EXT];
	bool m_bBoneMappingSetup;

	// Tracking state for this controller
	bool m_bControllerTracked;
	Vector m_vecLastValidPosition;
	QAngle m_angLastValidAngles;

	// Model info
	char m_szModelName[MAX_PATH];
	bool m_bHasGunslinger;         // Engineer has Gunslinger equipped (robot right hand)
	
	// Fire animation
	int m_iFireSequence;           // Fire animation sequence index (fire_loop for medigun)
	int m_iAltFireSequence;        // Alt-fire (secondary attack) animation sequence index
	int m_iIdleSequence;           // Idle animation sequence to return to
	bool m_bAnimateIdle;           // Weapon model drives its own vm_weapon idle (skip bone merge for those)
	bool m_bLoopIdleOnHand;        // Hand idle animation should advance (e.g. sapper screen animation)
	bool m_bPlayingFireAnim;       // Currently playing fire animation
	float m_flFireAnimStartTime;   // When fire animation started
	
	// Draw animation (played on weapon equip/deploy)
	int m_iDrawSequence;              // Draw animation sequence index on hand model
	bool m_bPlayingDrawAnim;          // Currently playing draw animation
	float m_flDrawAnimStartTime;      // When draw animation started
	VRDrawAnimScope m_eDrawAnimScope; // Which bones the draw animation drives
	
	// Charge animation (sticky launcher, huntsman, loose cannon, etc.)
	int m_iChargeSequence;         // Charge/pullback animation sequence index
	int m_iChargeSequence2;        // Second charge phase (e.g. huntsman max-charge shake)
	bool m_bPlayingChargeAnim;     // Currently playing charge animation
	
	// Medigun fire animation state machine
	MedigunFireState m_eMedigunFireState;
	int m_iFireOnSequence;         // fire_on sequence (healing beam starts)
	int m_iFireOffSequence;        // fire_off sequence (healing beam ends)
	bool m_bMedigunWasHealing;     // Previous frame healing state for edge detection
	
	// Flamethrower fire animation state tracking
	bool m_bFlamethrowerWasFiring;   // Previous frame firing state for edge detection
	float m_flFlamethrowerFireBlend; // Blend weight: 0.0 = idle, 1.0 = fully in fire pose
	
	// Melee swing animation cycling (a -> b -> c -> a...)
	int m_iMeleeSwingIndex;        // Current swing variant (0=a, 1=b, 2=c)
	char m_szMeleeSwingBase[64];   // Base swing animation name (e.g., "b_swing_")
	int m_iMeleeSwingCount;        // Number of swing variants available (usually 3)
	
	// VR physical swing edge detection (triggers hand fire animation)
	bool m_bPrevVRSwingActive;     // Previous frame m_bVRSwingActive
	
	// Bread Bite: crit sequence + idle cycling on the hand model
	int m_iBreadBiteCritSeq;       // breadglove_swing_crit on the hand
	int m_iBreadBiteIdleSeqs[3];   // breadglove_idle_A/B/C on the hand
	bool m_bIsBreadBite;           // current weapon is the Bread Bite
	bool m_bBreadCreaturePin;      // pin hand to controller during creature idle (jars)
	float m_flBreadBiteIdleStartTime; // when the current idle variant started playing

	// Bread Bite animation crossfade
	int m_iBBCrossfadeFromSeq;      // sequence we're blending away from (-1 = none)
	float m_flBBCrossfadeFromCycle; // cycle of that sequence at transition time
	float m_flBBCrossfadeStart;     // gpGlobals->curtime when crossfade began
	int m_iBBLastSampledSeq;        // previous frame's sequence (for change detection)
	float m_flBBLastSampledCycle;   // previous frame's cycle
	float m_flBBLastCrossfadeCheck; // last curtime we ran crossfade detection
	
	// Backstab ready animation (spy knife only)
	int m_iBackstabUpSequence;          // knife_backstab_up (raise transition)
	int m_iBackstabDownSequence;        // knife_backstab_down (lower transition)
	int m_iBackstabIdleSequence;        // knife_backstab_idle (hold pose)
	int m_iBackstabAttackSequence;      // knife_backstab (the actual stab attack)
	bool m_bBackstabReady;
	bool m_bBackstabAttacking;          // true = playing the backstab attack animation (cooldown)
	float m_flBackstabCycle;            // 0→1 through up/down anims
	bool m_bBackstabRaising;            // true = playing knife_backstab_up / holding idle
	bool m_bBackstabLowering;           // true = playing knife_backstab_down
	float m_flLastBackstabUpdateTime;   // per-instance guard so SetupBones multi-calls don't multiply cycle
	matrix3x4_t m_matIdleWeaponBoneLocal;
	matrix3x4_t m_matIdleWeaponBoneWorld;
	bool m_bHasIdleWeaponBone;
public:
	bool IsBackstabReady() const { return m_bBackstabReady; }
	bool GetIdleWeaponBoneTransform( Vector &outPos, QAngle &outAng ) const;
	
	// Cached transform from idle hand bone to VR controller (calculated once)
	matrix3x4_t m_matIdleHandBoneTransform;  // Hand bone transform from idle pose (model space)
	Vector m_vecIdleHandBoneLocalPos;        // Hand bone LOCAL position from idle pose (parent space)
	bool m_bHandBoneOffsetValid;             // Whether the offset has been calculated
	
	// Two-handed weapon support
	float m_flTwoHandBlend;  // 0.0 = free hand, 1.0 = fully gripping weapon
	int m_iOffHandBone;      // Bone index for the off-hand (bip_hand_L) on weapon hand's model
	int m_iOffHandMiddleFingerBone;  // Bone index for bip_middle_0_L (for bind pose offset calc)
	
	// Offhand grip state - active when grip button held + within range
	bool m_bOffhandGripActive;         // Is the offhand currently gripping the weapon
	bool m_bWasOffhandGripActive;      // Was grip active last frame (for blend-out tracking)
	float m_flGripRotationBlend;       // Separate blend for weapon rotation (0=no rotation, 1=full grip rotation)
	Vector m_vecOffhandGripForward;    // Desired forward direction (toward offhand)
	Vector m_vecOffhandGripUp;         // Up reference from weapon hand controller
	Vector m_vecCachedGripDelta;       // Cached initial pivot axis from grip start
	Vector m_vecCachedGripYAxis;       // Cached initial weapon Y-axis from grip start
	
	// Aim stabilization - counteracts grip/trigger-induced palm movement
	// Uses controller aim pose as stable reference; derives palm from controller movement
	// so grip/trigger finger changes don't shift the hand root or weapon aim
	matrix3x4_t m_matAimRefPalmOffset;   // Palm transform relative to controller (captured at equip)
	bool m_bAimRefValid;                 // Whether reference has been captured
	
	// Cached idle muzzle offset for pistols (to keep aim stable during fire anim)
	Vector m_vIdleMuzzleOffset;        // Muzzle offset relative to weapon_bone in idle pose
	QAngle m_angIdleMuzzleAngles;      // Muzzle angles relative to weapon_bone in idle pose
	bool m_bIdleMuzzleOffsetValid;     // Whether the offset has been calculated
	int m_iCachedMuzzleWeaponID;       // Weapon ID this was cached for (invalidate on weapon change)
	
	// Crit boost effect - attached to hand for proper timing
	CSmartPtr<CNewParticleEffect> m_pCritBoostEffect;
	bool m_bCritBoostActive;
	
	// Cached weapon bone world transform - set during PositionWeaponFromBones
	// This is used by overlays to avoid stale bone cache issues
	matrix3x4_t m_matWeaponBoneWorld;
	bool m_bWeaponBoneWorldValid;
	
	// Cached muzzle position - set during PositionWeaponFromBones for effects
	Vector m_vecCachedMuzzlePos;
	QAngle m_angCachedMuzzleAngles;
	bool m_bCachedMuzzleValid;
	
	// =========================================================================
	// LEFT HAND WEARABLES - spy watches, scout balls, etc.
	// These are attached to the LEFT hand when the RIGHT hand equips certain weapons
	// =========================================================================
	
	// Spy watch (cloak weapon's wrist model)
	// Shown on left hand whenever a Spy has a cloak weapon equipped
	// Position is driven by weapon_bone_L in SetupBones
	CHandle<C_BaseAnimating> m_hLeftHandWatch;
	matrix3x4_t m_matWatchOffset;
	bool m_bWatchOffsetValid;
	CVRWatchPanel *m_pWatchPanel;
	
	// Scout ball (Sandman baseball, Wrap Assassin ornament)
	// Conditionally shown based on ammo availability
	// Ball position is updated directly in SetupBones() to avoid lag
	CHandle<C_BaseAnimating> m_hLeftHandBall;
	int m_iLastBallAmmo;  // Track ammo changes to update ball visibility
	
	// Demoman shield (Chargin' Targe, Splendid Screen, Tide Turner, etc.)
	// Shown on left hand whenever the Demoman has a shield equipped
	CHandle<C_BaseAnimating> m_hLeftHandShield;
	matrix3x4_t m_matShieldOffset;  // weapon_targe relative to bip_hand_L from c_demo_arms cm_idle
	bool m_bShieldOffsetValid;
	
	// NOTE: attach_to_hands weapons (boxing gloves, etc.) contain BOTH hands
	// in a single model mesh, so they don't need special left-hand handling.
	
public:
	// Left hand wearable management
	void CreateWatchModel(const char *pszWatchModel);
	void RemoveLeftHandWatch();
	void UpdateLeftHandWatch();   // Called each frame to check watch state
	void AttachBallToLeftHand(const char *pszBallModel);
	void RemoveLeftHandBall();
	void UpdateLeftHandBall();    // Called each frame to check ammo
	void AttachShieldToLeftHand(const char *pszShieldModel, int nSkin);
	void RemoveLeftHandShield();
	void UpdateLeftHandShield();  // Called each frame to check shield state
};

// Global functions
void UpdateVRHands();
void CleanupAllVRHands();

// Helper to get the opposite hand
C_TFVRHand* GetOppositeVRHand(C_TFVRHand *pHand);

// Accessors for the local player's hands
C_TFVRHand* GetLocalPlayerLeftHand();
C_TFVRHand* GetLocalPlayerRightHand();

#endif // C_TFVR_HAND_H

