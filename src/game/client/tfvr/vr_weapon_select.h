//=============================================================================
// TF2VR - VR Weapon Selection Menu
// Radial weapon selection menu for VR controllers
//=============================================================================

#ifndef VR_WEAPON_SELECT_H
#define VR_WEAPON_SELECT_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Panel.h"
#include "c_tf_player.h"
#include "tf_weaponbase.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class C_TFWeaponBase;

//-----------------------------------------------------------------------------
// CVRWeaponSelectPanel - VGUI Panel that renders the radial weapon menu
//-----------------------------------------------------------------------------
class CVRWeaponSelectPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CVRWeaponSelectPanel, vgui::Panel);

public:
	CVRWeaponSelectPanel(vgui::Panel *parent, const char *panelName);
	virtual ~CVRWeaponSelectPanel();

	virtual void Paint() override;
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;

	// Update hand position in normalized coordinates (0-1)
	void UpdateHandPosition(float u, float v);
	
	// Get the currently selected slot (-1 if none/center)
	int GetSelectedSlot() const { return m_nSelectedSlot; }
	void SetNumQuadrants(int num) { m_nNumQuadrants = num; }
	
	
	// Convert a slot index to screen coordinates for drawing
	void SlotToCoords(int slot, int &x, int &y);
	
	// Get slot from hand position (in pixel coords)
	int GetSlotFromPosition(int x, int y);
	
	// Draw a single quadrant
	void DrawQuadrant(int slot, bool selected, int centerX, int centerY);
	
	// Draw weapon icon and name in a quadrant
	void DrawWeaponInQuadrant(C_TFWeaponBase *pWeapon, int slot, bool selected, int centerX, int centerY);
	
	// Get weapon in a specific slot
	C_TFWeaponBase* GetWeaponInSlot(int slot);

	// Slot layout configuration
	// Slot angles: 0=12 o'clock (primary), 1=9 o'clock (secondary), 2=3 o'clock (melee), 3=6 o'clock (PDA)
	float GetSlotAngle(int slot);
	
	// Member variables
	int m_nSelectedSlot;          // Currently selected slot (-1 = center/none)
	int m_nNumQuadrants;          // Number of active quadrants (3, 4, or 5)
	int m_handX, m_handY;         // Hand position in panel pixel coords
	
	// Textures
	int m_nQuadrantSelectedTexId;
	int m_nQuadrantUnselectedTexId;
	
	// Layout configuration
	int m_nQuadrantRadius;        // Distance from center to quadrant center
	int m_nQuadrantSize;          // Size of each quadrant texture
	int m_nCenterDeadzone;        // Radius of center deadzone
	
	// Fonts for weapon names (different sizes)
	vgui::HFont m_hWeaponNameFont;         // Currently selected font
	vgui::HFont m_hWeaponNameFontSmallest;
	vgui::HFont m_hWeaponNameFontSmall;
	vgui::HFont m_hWeaponNameFontMedium;
	vgui::HFont m_hWeaponNameFontLarge;
};

//-----------------------------------------------------------------------------
// CVRWeaponSelectManager - Manages the weapon selection menu lifecycle
//-----------------------------------------------------------------------------
class CVRWeaponSelectManager
{
public:
	CVRWeaponSelectManager();
	~CVRWeaponSelectManager();
	
	// Initialize/shutdown
	bool Initialize();
	void Shutdown();
	
	// Per-frame update
	void Update(float deltaTime);
	
	// Render the menu in 3D space
	void Render();
	
	// Open/close the menu
	void OpenMenu();
	void CloseMenu();
	
	// Check if menu is currently open
	bool IsMenuOpen() const { return m_bMenuOpen; }
	
	// Reset state (e.g., on respawn)
	void ResetState();

private:
	// Calculate the menu transform in world space
	bool CalculateMenuTransform(VMatrix &transform);
	
	// Project hand position onto menu plane
	void UpdateHandPositionOnMenu();
	
	// Handle weapon switching
	void HandleWeaponSelection();
	
	// Get number of quadrants for current class
	int GetNumQuadrantsForClass(int classIndex);

	// State
	bool m_bInitialized;
	bool m_bMenuOpen;
	
	// The panel
	CVRWeaponSelectPanel *m_pPanel;
	
	// Menu positioning (playspace-anchored)
	float m_flMenuSize;           // World size of menu
	Vector m_vecMenuPlayspacePos; // Position relative to playspace origin (stays fixed)
	float m_flMenuYaw;            // Yaw to face toward head when opened (stays fixed)
	
	// Cursor tracking in playspace (immune to head-relative corrections)
	Vector m_vecHandPlayspacePosAtOpen; // Raw controller playspace position when menu opened
	Vector m_vecMenuRightPlayspace;     // Menu's "right" direction in playspace
	Vector m_vecMenuUpPlayspace;        // Menu's "up" direction in playspace
	
	// Selection state
	int m_nLastSelectedSlot;      // Last selected slot (for change detection)
	bool m_bSelectionMade;        // Whether a selection was made this frame
	
	// Panel dimensions
	int m_nPanelPixelWidth;
	int m_nPanelPixelHeight;
	float m_flPanelWorldWidth;
	float m_flPanelWorldHeight;
};

// Global accessor
extern CVRWeaponSelectManager *g_pVRWeaponSelectManager;

#endif // VR_WEAPON_SELECT_H
