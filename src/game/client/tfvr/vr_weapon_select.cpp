//=============================================================================
// TF2VR - VR Weapon Selection Menu
// Radial weapon selection menu for VR controllers
//=============================================================================

#include "cbase.h"
#include "vr_weapon_select.h"
#include "vr_world_ui_queue.h"
#include "c_tf_player.h"
#include "tf_weaponbase.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui/IScheme.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "hud.h"
#include "iclientmode.h"
#include "tier0/vprof.h"
#include "openxr_manager.h"
#include "tf_gamerules.h"
#include "econ/econ_item_view.h"
#include "econ/econ_item_system.h"
#include "econ/attribute_manager.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "client_virtualreality.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// Global instance
//=============================================================================
CVRWeaponSelectManager *g_pVRWeaponSelectManager = nullptr;

//=============================================================================
// ConVars
//=============================================================================
ConVar tfvr_weapon_select_enabled("tfvr_weapon_select_enabled", "1", FCVAR_ARCHIVE,
	"Enable VR radial weapon selection menu");
ConVar tfvr_weapon_select_instant("tfvr_weapon_select_instant", "1", FCVAR_ARCHIVE,
	"Switch weapons instantly when pointing at them (0 = switch on release)");
ConVar tfvr_weapon_select_distance("tfvr_weapon_select_distance", "0", FCVAR_ARCHIVE,
	"Forward distance of weapon select menu from palm in units (0 = centered on palm)");
ConVar tfvr_weapon_select_offset_right("tfvr_weapon_select_offset_right", "0", FCVAR_ARCHIVE,
	"Right offset of weapon select menu from palm in units");
ConVar tfvr_weapon_select_offset_up("tfvr_weapon_select_offset_up", "0", FCVAR_ARCHIVE,
	"Up offset of weapon select menu from palm in units");
ConVar tfvr_weapon_select_use_grip("tfvr_weapon_select_use_grip", "0", FCVAR_ARCHIVE,
	"Use grip pose (1) or aim pose (0) for menu positioning");
ConVar tfvr_weapon_select_scale("tfvr_weapon_select_scale", "1.0", FCVAR_ARCHIVE,
	"Scale of the weapon select menu (1.0 = default size)");
ConVar tfvr_weapon_select_quadrant_radius("tfvr_weapon_select_quadrant_radius", "240", FCVAR_ARCHIVE,
	"Distance from center to quadrant centers in pixels");
ConVar tfvr_weapon_select_deadzone("tfvr_weapon_select_deadzone", "80", FCVAR_ARCHIVE,
	"Radius of center deadzone in pixels");
ConVar tfvr_weapon_select_debug("tfvr_weapon_select_debug", "0", FCVAR_ARCHIVE,
	"Show debug info for weapon selection");
ConVar tfvr_weapon_select_text_offset("tfvr_weapon_select_text_offset", "70", FCVAR_ARCHIVE,
	"Vertical offset of weapon name text below icon center (pixels)");
ConVar tfvr_weapon_select_text_size("tfvr_weapon_select_text_size", "3", FCVAR_ARCHIVE,
	"Font size for weapon names: 0=smallest, 1=small, 2=medium, 3=large (TF2 font)");

//=============================================================================
// CVRWeaponSelectPanel Implementation
//=============================================================================

CVRWeaponSelectPanel::CVRWeaponSelectPanel(vgui::Panel *parent, const char *panelName)
	: vgui::Panel(parent, panelName)
{
	m_nSelectedSlot = -1;
	m_nNumQuadrants = 4;
	m_handX = 0;
	m_handY = 0;
	
	m_nQuadrantSelectedTexId = -1;
	m_nQuadrantUnselectedTexId = -1;
	
	m_nQuadrantRadius = 240;
	m_nQuadrantSize = 256;
	m_nCenterDeadzone = 80;
	
	m_hWeaponNameFont = vgui::INVALID_FONT;
	
	SetPaintBackgroundEnabled(false);
}

CVRWeaponSelectPanel::~CVRWeaponSelectPanel()
{
}

void CVRWeaponSelectPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	// Load textures
	m_nQuadrantSelectedTexId = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile(m_nQuadrantSelectedTexId, 
		"vgui/weapon_select/weapon_select_quadrant_selected", true, false);
	
	m_nQuadrantUnselectedTexId = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile(m_nQuadrantUnselectedTexId, 
		"vgui/weapon_select/weapon_select_quadrant_unselected", true, false);
	
	// Explicitly load TF2's ClientScheme for proper font access
	// The default scheme passed to ApplySchemeSettings may not have TF2 fonts
	vgui::HScheme hClientScheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL),
		"resource/ClientScheme.res",
		"ClientScheme"
	);
	vgui::IScheme *pClientScheme = vgui::scheme()->GetIScheme(hClientScheme);
	
	if (!pClientScheme)
	{
		Warning("VR Weapon Select: Failed to load ClientScheme, using fallback\n");
		pClientScheme = pScheme; // Fall back to passed scheme
	}
	
	// Load fonts for weapon names from ClientScheme
	// HudSelectionText is the TF2 weapon selection font (uses TF2 custom font)
	m_hWeaponNameFont = pClientScheme->GetFont("HudSelectionText", true);
	const char *mainFontUsed = "HudSelectionText";
	if (m_hWeaponNameFont == vgui::INVALID_FONT)
	{
		m_hWeaponNameFont = pClientScheme->GetFont("HudFontMediumSmall", true);
		mainFontUsed = "HudFontMediumSmall (fallback)";
	}
	if (m_hWeaponNameFont == vgui::INVALID_FONT)
	{
		m_hWeaponNameFont = pClientScheme->GetFont("Default", true);
		mainFontUsed = "Default (fallback)";
	}
	
	// Load additional sizes for ConVar selection
	m_hWeaponNameFontSmallest = pClientScheme->GetFont("HudFontSmallest", true);
	if (m_hWeaponNameFontSmallest == vgui::INVALID_FONT)
		m_hWeaponNameFontSmallest = pClientScheme->GetFont("DefaultSmall", true);
	
	m_hWeaponNameFontSmall = pClientScheme->GetFont("HudFontSmall", true);
	if (m_hWeaponNameFontSmall == vgui::INVALID_FONT)
		m_hWeaponNameFontSmall = pClientScheme->GetFont("Default", true);
	
	m_hWeaponNameFontMedium = pClientScheme->GetFont("HudFontMediumSmall", true);
	if (m_hWeaponNameFontMedium == vgui::INVALID_FONT)
		m_hWeaponNameFontMedium = pClientScheme->GetFont("HudFontSmall", true);
	
	// HudSelectionText is the authentic TF2 weapon select font
	m_hWeaponNameFontLarge = pClientScheme->GetFont("HudSelectionText", true);
	const char *largeFontUsed = "HudSelectionText";
	if (m_hWeaponNameFontLarge == vgui::INVALID_FONT)
	{
		m_hWeaponNameFontLarge = pClientScheme->GetFont("HudFontMedium", true);
		largeFontUsed = "HudFontMedium (fallback)";
	}
	
	if (tfvr_weapon_select_debug.GetBool())
	{
		Msg("VR Weapon Select Font Debug:\n");
		Msg("  Using ClientScheme: %s\n", pClientScheme != pScheme ? "YES" : "NO (fallback)");
		Msg("  Main font: %s (handle=%d, valid=%s)\n", mainFontUsed, m_hWeaponNameFont, 
			m_hWeaponNameFont != vgui::INVALID_FONT ? "YES" : "NO");
		Msg("  Large font: %s (handle=%d, valid=%s)\n", largeFontUsed, m_hWeaponNameFontLarge,
			m_hWeaponNameFontLarge != vgui::INVALID_FONT ? "YES" : "NO");
		
		if (m_hWeaponNameFontLarge != vgui::INVALID_FONT)
		{
			int fontTall = vgui::surface()->GetFontTall(m_hWeaponNameFontLarge);
			Msg("  Large font height: %d pixels\n", fontTall);
		}
	}
}

float CVRWeaponSelectPanel::GetSlotAngle(int slot)
{
	// Slot layout:
	// Slot 0 = Primary = 12 o'clock = 0 degrees (top)
	// Slot 1 = Secondary = 9 o'clock = 270 degrees (left)  
	// Slot 2 = Melee = 3 o'clock = 90 degrees (right)
	// Slot 3 = PDA/Watch = 6 o'clock = 180 degrees (bottom)
	
	switch (slot)
	{
		case 0: return 0.0f;     // 12 o'clock - Primary
		case 1: return 270.0f;   // 9 o'clock - Secondary
		case 2: return 90.0f;    // 3 o'clock - Melee
		case 3: return 180.0f;   // 6 o'clock - PDA/Watch
		case 4: return 180.0f;   // 6 o'clock - Engineer PDA2 (will be split later)
		default: return 0.0f;
	}
}

void CVRWeaponSelectPanel::SlotToCoords(int slot, int &x, int &y)
{
	int centerX = GetWide() / 2;
	int centerY = GetTall() / 2;
	
	float angle = GetSlotAngle(slot);
	float angleRad = DEG2RAD(angle - 90.0f); // -90 because 0 degrees should be up
	
	float radius = (float)m_nQuadrantRadius;
	
	x = centerX + (int)(cosf(angleRad) * radius);
	y = centerY + (int)(sinf(angleRad) * radius);
}

int CVRWeaponSelectPanel::GetSlotFromPosition(int x, int y)
{
	int centerX = GetWide() / 2;
	int centerY = GetTall() / 2;
	
	float dx = (float)(x - centerX);
	float dy = (float)(y - centerY);
	float dist = sqrtf(dx * dx + dy * dy);
	
	// Check if in center deadzone
	if (dist < (float)m_nCenterDeadzone)
	{
		return -1; // Center = no selection / keep current
	}
	
	// Calculate angle from center
	float angle = RAD2DEG(atan2f(dy, dx)) + 90.0f; // +90 because up should be 0
	if (angle < 0) angle += 360.0f;
	if (angle >= 360.0f) angle -= 360.0f;
	
	// Map angle to slot based on quadrant layout
	// Each quadrant covers 90 degrees
	// Slot 0 (Primary) = 315-45 degrees (centered on 0/360)
	// Slot 1 (Secondary) = 225-315 degrees (centered on 270)
	// Slot 2 (Melee) = 45-135 degrees (centered on 90)
	// Slot 3 (PDA) = 135-225 degrees (centered on 180)
	
	if (angle >= 315.0f || angle < 45.0f)
	{
		return 0; // Primary (12 o'clock)
	}
	else if (angle >= 45.0f && angle < 135.0f)
	{
		return 2; // Melee (3 o'clock)
	}
	else if (angle >= 135.0f && angle < 225.0f)
	{
		if (m_nNumQuadrants >= 4)
			return 3; // PDA (6 o'clock)
		else
			return -1; // No slot here for 3-quadrant classes
	}
	else // 225-315
	{
		return 1; // Secondary (9 o'clock)
	}
}

C_TFWeaponBase* CVRWeaponSelectPanel::GetWeaponInSlot(int slot)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return nullptr;
	
	// TF2 weapon slots:
	// 0 = Primary
	// 1 = Secondary
	// 2 = Melee
	// 3 = PDA (Engineer: Build, Spy: Disguise Kit)
	// 4 = PDA2 (Engineer: Destroy)
	// 5 = Building (Engineer)
	
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if (!pWeapon)
			continue;
		
		C_TFWeaponBase *pTFWeapon = dynamic_cast<C_TFWeaponBase*>(pWeapon);
		if (!pTFWeapon)
			continue;
		
		if (pTFWeapon->GetSlot() == slot)
			return pTFWeapon;
	}
	
	return nullptr;
}

void CVRWeaponSelectPanel::UpdateHandPosition(float u, float v)
{
	// Convert normalized coords to pixel coords
	m_handX = (int)(u * (float)GetWide());
	m_handY = (int)(v * (float)GetTall());
	
	// Clamp to panel bounds
	m_handX = clamp(m_handX, 0, GetWide());
	m_handY = clamp(m_handY, 0, GetTall());
	
	// Update selected slot
	m_nSelectedSlot = GetSlotFromPosition(m_handX, m_handY);
}

void CVRWeaponSelectPanel::DrawQuadrant(int slot, bool selected, int centerX, int centerY)
{
	int texId = selected ? m_nQuadrantSelectedTexId : m_nQuadrantUnselectedTexId;
	
	if (texId < 0)
		return;
	
	int qx, qy;
	SlotToCoords(slot, qx, qy);
	
	// Calculate rotation angle for this quadrant
	float angle = GetSlotAngle(slot);
	
	// Size of quadrant
	int halfSize = m_nQuadrantSize / 2;
	
	// Set up drawing
	vgui::surface()->DrawSetTexture(texId);
	vgui::surface()->DrawSetColor(255, 255, 255, 255);
	
	// Draw rotated quad
	// Calculate rotated corners
	float angleRad = DEG2RAD(angle);
	float cosA = cosf(angleRad);
	float sinA = sinf(angleRad);
	
	// Define quad corners (before rotation)
	float corners[4][2] = {
		{-halfSize, -halfSize},  // Top-left
		{halfSize, -halfSize},   // Top-right
		{halfSize, halfSize},    // Bottom-right
		{-halfSize, halfSize}    // Bottom-left
	};
	
	// Rotate and translate corners
	vgui::Vertex_t verts[4];
	for (int i = 0; i < 4; i++)
	{
		float rx = corners[i][0] * cosA - corners[i][1] * sinA;
		float ry = corners[i][0] * sinA + corners[i][1] * cosA;
		verts[i].m_Position.x = qx + rx;
		verts[i].m_Position.y = qy + ry;
	}
	
	// UV coordinates
	verts[0].m_TexCoord.Init(0, 0);
	verts[1].m_TexCoord.Init(1, 0);
	verts[2].m_TexCoord.Init(1, 1);
	verts[3].m_TexCoord.Init(0, 1);
	
	vgui::surface()->DrawTexturedPolygon(4, verts);
}

void CVRWeaponSelectPanel::DrawWeaponInQuadrant(C_TFWeaponBase *pWeapon, int slot, bool selected, int centerX, int centerY)
{
	if (!pWeapon)
		return;
	
	int qx, qy;
	SlotToCoords(slot, qx, qy);
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	bool bIconDrawn = false;
	
	// First, try to get the item's inventory image (for custom items like Force-a-Nature)
	CAttributeContainer *pAttrContainer = pWeapon->GetAttributeContainer();
	CEconItemView *pItemView = pAttrContainer ? pAttrContainer->GetItem() : nullptr;
	if (pItemView && pItemView->IsValid())
	{
		const char *pszInventoryImage = pItemView->GetInventoryImage();
		if (pszInventoryImage && pszInventoryImage[0])
		{
			// Load the inventory image as a material
			IMaterial *pMaterial = materials->FindMaterial(pszInventoryImage, TEXTURE_GROUP_VGUI, false);
			if (pMaterial && !pMaterial->IsErrorMaterial())
			{
				// Get the image size from the item data
				int iPosition[2] = {0, 0};
				int iSize[2] = {128, 82}; // Default backpack icon size
				pItemView->GetInventoryImageData(iPosition, iSize);
				
				// Scale to fit in quadrant
				float maxSize = (float)m_nQuadrantSize * 0.8f;
				float scale = MIN(maxSize / (float)iSize[0], maxSize / (float)iSize[1]);
				int scaledWidth = (int)(iSize[0] * scale);
				int scaledHeight = (int)(iSize[1] * scale);
				
				// Draw using material system surface
				int textureId = vgui::surface()->DrawGetTextureId(pszInventoryImage);
				if (textureId == -1)
				{
					textureId = vgui::surface()->CreateNewTextureID();
					g_pMatSystemSurface->DrawSetTextureMaterial(textureId, pMaterial);
				}
				
				Color col = selected ? Color(255, 255, 255, 255) : Color(200, 200, 200, 200);
				vgui::surface()->DrawSetColor(col);
				vgui::surface()->DrawSetTexture(textureId);
				vgui::surface()->DrawTexturedRect(
					qx - scaledWidth / 2, qy - scaledHeight / 2,
					qx + scaledWidth / 2, qy + scaledHeight / 2);
				
				bIconDrawn = true;
			}
		}
	}
	
	// Fallback to base weapon sprites if no inventory image was drawn
	if (!bIconDrawn)
	{
		const CHudTexture *pTexture = nullptr;
		
		if (pPlayer)
		{
			// Use team-colored icon
			if (pPlayer->GetTeamNumber() == TF_TEAM_BLUE)
				pTexture = pWeapon->GetSpriteActive();
			else
				pTexture = pWeapon->GetSpriteInactive();
		}
		
		if (!pTexture)
			pTexture = pWeapon->GetSpriteInactive();
		
		if (pTexture)
		{
			// Draw weapon icon centered on quadrant
			int iconWidth = pTexture->Width();
			int iconHeight = pTexture->Height();
			
			// Scale icon to fit in quadrant (1.2x for 1024 panel resolution)
			float scale = 1.2f;
			int scaledWidth = (int)(iconWidth * scale);
			int scaledHeight = (int)(iconHeight * scale);
			
			Color col = selected ? Color(255, 255, 255, 255) : Color(200, 200, 200, 200);
			
			pTexture->DrawSelf(qx - scaledWidth / 2, qy - scaledHeight / 2, 
				scaledWidth, scaledHeight, col);
		}
	}
	
	// Draw weapon name below icon
	if (m_hWeaponNameFont != vgui::INVALID_FONT)
	{
		const wchar_t *pName = nullptr;
		wchar_t wszName[128];
		wszName[0] = L'\0';
		
		// First, try to get the item's actual name (works for custom items like Force-a-Nature)
		if (pItemView && pItemView->IsValid())
		{
			pName = pItemView->GetItemName();
		}
		
		if (pName && pName[0])
		{
			V_wcsncpy(wszName, pName, sizeof(wszName));
		}
		else
		{
			// Fallback to base weapon's print name
			const char *pszPrintName = pWeapon->GetPrintName();
			if (pszPrintName && pszPrintName[0])
			{
				// GetPrintName returns a localization token like "#TF_Weapon_Revolver"
				const wchar_t *pLocalizedName = g_pVGuiLocalize->Find(pszPrintName);
				if (pLocalizedName)
				{
					V_wcsncpy(wszName, pLocalizedName, sizeof(wszName));
				}
				else
				{
					// Fallback to raw string (strip # if present)
					const char *pRawName = pszPrintName[0] == '#' ? pszPrintName + 1 : pszPrintName;
					g_pVGuiLocalize->ConvertANSIToUnicode(pRawName, wszName, sizeof(wszName));
				}
			}
		}
		
		if (wszName[0])
		{
			// Select font size based on ConVar
			// Default (3) uses HudSelectionText - the authentic TF2 weapon select font
			vgui::HFont font = m_hWeaponNameFontLarge; // Default to TF2 font
			int textSizeOption = tfvr_weapon_select_text_size.GetInt();
			switch (textSizeOption)
			{
				case 0: font = m_hWeaponNameFontSmallest; break;
				case 1: font = m_hWeaponNameFontSmall; break;
				case 2: font = m_hWeaponNameFontMedium; break;
				case 3: font = m_hWeaponNameFontLarge; break;  // HudSelectionText (TF2 font)
				default: font = m_hWeaponNameFontLarge; break; // Default to TF2 font
			}
			
			if (font == vgui::INVALID_FONT)
				font = m_hWeaponNameFont;
			
			int textW, textH;
			vgui::surface()->GetTextSize(font, wszName, textW, textH);
			
			// Get text offset from ConVar
			int textOffset = tfvr_weapon_select_text_offset.GetInt();
			
			int textX = qx - textW / 2;
			int textY = qy + textOffset;
			
			// Draw drop shadow first (offset for visibility at higher resolution)
			vgui::surface()->DrawSetTextFont(font);
			vgui::surface()->DrawSetTextColor(Color(0, 0, 0, selected ? 255 : 180));
			vgui::surface()->DrawSetTextPos(textX + 3, textY + 3);
			vgui::surface()->DrawPrintText(wszName, wcslen(wszName));
			
			// Draw main text
			Color textCol = selected ? Color(255, 255, 255, 255) : Color(180, 180, 180, 200);
			vgui::surface()->DrawSetTextColor(textCol);
			vgui::surface()->DrawSetTextPos(textX, textY);
			vgui::surface()->DrawPrintText(wszName, wcslen(wszName));
		}
	}
}

void CVRWeaponSelectPanel::Paint()
{
	VPROF_BUDGET("CVRWeaponSelectPanel::Paint", VPROF_BUDGETGROUP_OTHER_VGUI);
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;
	
	// Update layout from convars
	m_nQuadrantRadius = tfvr_weapon_select_quadrant_radius.GetInt();
	m_nCenterDeadzone = tfvr_weapon_select_deadzone.GetInt();
	
	int centerX = GetWide() / 2;
	int centerY = GetTall() / 2;
	
	// Draw quadrants
	for (int slot = 0; slot < m_nNumQuadrants && slot < 4; slot++)
	{
		bool selected = (slot == m_nSelectedSlot);
		
		// Draw quadrant background
		DrawQuadrant(slot, selected, centerX, centerY);
		
		// Draw weapon in quadrant
		C_TFWeaponBase *pWeapon = GetWeaponInSlot(slot);
		DrawWeaponInQuadrant(pWeapon, slot, selected, centerX, centerY);
	}
	
	// Draw hand cursor
	vgui::surface()->DrawSetColor(255, 255, 255, 200);
	vgui::surface()->DrawOutlinedCircle(m_handX, m_handY, 8, 16);
	vgui::surface()->DrawFilledRect(m_handX - 2, m_handY - 2, m_handX + 2, m_handY + 2);
	
	// Debug: draw center deadzone
	if (tfvr_weapon_select_debug.GetBool())
	{
		vgui::surface()->DrawSetColor(255, 255, 0, 100);
		vgui::surface()->DrawOutlinedCircle(centerX, centerY, m_nCenterDeadzone, 32);
	}
}

//=============================================================================
// CVRWeaponSelectManager Implementation
//=============================================================================

CVRWeaponSelectManager::CVRWeaponSelectManager()
{
	m_bInitialized = false;
	m_bMenuOpen = false;
	m_pPanel = nullptr;
	
	m_flMenuSize = 1.0f;
	m_vecMenuPlayspacePos = vec3_origin;
	m_flMenuYaw = 0.0f;
	
	m_vecHandPlayspacePosAtOpen = vec3_origin;
	m_vecMenuRightPlayspace = Vector(0, 1, 0);
	m_vecMenuUpPlayspace = Vector(0, 0, 1);
	
	m_nLastSelectedSlot = -1;
	m_bSelectionMade = false;
	
	m_nPanelPixelWidth = 1024;
	m_nPanelPixelHeight = 1024;
	m_flPanelWorldWidth = 20.0f;
	m_flPanelWorldHeight = 20.0f;
}

CVRWeaponSelectManager::~CVRWeaponSelectManager()
{
	Shutdown();
}

bool CVRWeaponSelectManager::Initialize()
{
	if (m_bInitialized)
		return true;
	
	// Create the panel
	m_pPanel = new CVRWeaponSelectPanel(nullptr, "VRWeaponSelectPanel");
	m_pPanel->SetBounds(0, 0, m_nPanelPixelWidth, m_nPanelPixelHeight);
	m_pPanel->SetVisible(false);
	
	m_bInitialized = true;
	DevMsg("VR Weapon Select Manager: Initialized\n");
	
	return true;
}

void CVRWeaponSelectManager::Shutdown()
{
	if (m_pPanel)
	{
		m_pPanel->DeletePanel();
		m_pPanel = nullptr;
	}
	
	m_bInitialized = false;
	m_bMenuOpen = false;
}

void CVRWeaponSelectManager::ResetState()
{
	m_bMenuOpen = false;
	m_nLastSelectedSlot = -1;
	m_bSelectionMade = false;
}

int CVRWeaponSelectManager::GetNumQuadrantsForClass(int classIndex)
{
	// Determine number of weapon slots based on class
	switch (classIndex)
	{
		case TF_CLASS_SPY:
			return 4; // Primary, Secondary (Sapper), Melee, PDA (Disguise Kit)
		case TF_CLASS_ENGINEER:
			return 5; // Primary, Secondary, Melee, PDA1 (Build), PDA2 (Destroy)
		default:
			return 3; // Primary, Secondary, Melee
	}
}

void CVRWeaponSelectManager::OpenMenu()
{
	if (!m_bInitialized || !tfvr_weapon_select_enabled.GetBool())
		return;
	
	if (m_bMenuOpen)
		return;
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer || !pPlayer->IsAlive())
		return;
	
	// Check if weapon switching is allowed
	if (!pPlayer->IsAllowedToSwitchWeapons())
		return;
	
	m_bMenuOpen = true;
	m_nLastSelectedSlot = -1;
	m_bSelectionMade = false;
	
	// Set number of quadrants based on class
	int classIndex = pPlayer->GetPlayerClass()->GetClassIndex();
	int numQuadrants = GetNumQuadrantsForClass(classIndex);
	m_pPanel->SetNumQuadrants(numQuadrants);
	
	// Get hand position for menu placement
	Vector handPos = pPlayer->EyePosition(); // Fallback
	QAngle handAngles(0, 0, 0);
	
	if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
	{
		VMatrix handPose;
		bool gotPose = false;
		
		// Choose between grip pose and aim pose based on ConVar
		if (tfvr_weapon_select_use_grip.GetBool())
		{
			gotPose = g_pOpenXRManager->GetRightControllerGripPose(handPose);
		}
		
		if (!gotPose)
		{
			// Use aim pose (more predictable positioning)
			gotPose = g_pOpenXRManager->GetRightControllerPose(handPose);
		}
		
		if (gotPose)
		{
			handPos = handPose.GetTranslation();
			MatrixAngles(handPose.As3x4(), handAngles);
		}
	}
	
	// Get offset values from ConVars (relative to player orientation)
	float forwardOffset = tfvr_weapon_select_distance.GetFloat();
	float rightOffset = tfvr_weapon_select_offset_right.GetFloat();
	float upOffset = tfvr_weapon_select_offset_up.GetFloat();
	
	// Calculate player orientation vectors for offset calculation
	QAngle playerAngles = pPlayer->GetAbsAngles();
	Vector playerForward, playerRight, playerUp;
	AngleVectors(QAngle(0, playerAngles[YAW], 0), &playerForward, &playerRight, &playerUp);
	
	// Calculate menu position: hand position + directional offsets (relative to player facing)
	Vector menuPos = handPos;
	menuPos += playerForward * forwardOffset;
	menuPos += playerRight * rightOffset;
	menuPos += Vector(0, 0, 1) * upOffset; // World up for vertical offset
	
	// Calculate playspace world matrix using proper matrix math
	VMatrix headWorldMatrix = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
	VMatrix headPlayspaceMatrix = g_pOpenXRManager->GetMideyePose();
	VMatrix playspaceToHead = headPlayspaceMatrix.InverseTR();
	VMatrix playspaceWorldMatrix = headWorldMatrix * playspaceToHead;
	
	// Store menu position in playspace coordinates
	// Transform world menu position to playspace coordinates
	VMatrix worldToPlayspace = playspaceWorldMatrix.InverseTR();
	Vector menuPlayspacePos = worldToPlayspace * menuPos;
	m_vecMenuPlayspacePos = menuPlayspacePos;
	
	// Calculate yaw so menu faces toward the head
	Vector headWorldPos = headWorldMatrix.GetTranslation();
	Vector toHead = headWorldPos - menuPos;  // Direction FROM menu TO head
	toHead.z = 0; // Only use horizontal direction
	VectorNormalize(toHead);
	
	// Convert direction to yaw angle - menu faces toward head
	QAngle facingAngles;
	VectorAngles(toHead, facingAngles);
	m_flMenuYaw = facingAngles[YAW];
	
	// Store raw controller playspace position for head-independent cursor tracking.
	// GetRightControllerPose applies a head-relative aim correction that shifts the
	// controller's apparent position when the head rotates. By tracking deltas from
	// the raw playspace position, cursor movement is purely physical hand movement.
	VMatrix handRawPlayspace;
	if (g_pOpenXRManager->GetRightControllerPoseRaw(handRawPlayspace))
	{
		m_vecHandPlayspacePosAtOpen = handRawPlayspace.GetTranslation();
	}
	else
	{
		m_vecHandPlayspacePosAtOpen = vec3_origin;
	}
	
	// Compute menu orientation in playspace for cursor projection
	VMatrix headPlayspace = g_pOpenXRManager->GetMideyePose();
	Vector headPlayspacePos = headPlayspace.GetTranslation();
	Vector toHeadPlayspace = headPlayspacePos - m_vecHandPlayspacePosAtOpen;
	toHeadPlayspace.z = 0;
	VectorNormalize(toHeadPlayspace);
	
	Vector menuForwardPlayspace = toHeadPlayspace;
	VectorVectors(menuForwardPlayspace, m_vecMenuRightPlayspace, m_vecMenuUpPlayspace);
	m_vecMenuRightPlayspace = -m_vecMenuRightPlayspace;
	
	// Update panel size from convars - use scale directly, not multiplied by distance
	m_flMenuSize = tfvr_weapon_select_scale.GetFloat();
	
	// Fixed base size of 50 units, scaled by the scale ConVar
	float baseSize = 50.0f;
	float aspectRatio = (float)m_nPanelPixelWidth / (float)m_nPanelPixelHeight;
	m_flPanelWorldHeight = baseSize * m_flMenuSize;
	m_flPanelWorldWidth = m_flPanelWorldHeight * aspectRatio;
	
	if (tfvr_weapon_select_debug.GetBool())
	{
		DevMsg("VR Weapon Select: Menu opened, playspacePos=(%.1f, %.1f, %.1f), yaw=%.1f, quadrants=%d\n", 
			m_vecMenuPlayspacePos.x, m_vecMenuPlayspacePos.y, m_vecMenuPlayspacePos.z, m_flMenuYaw, numQuadrants);
	}
}

void CVRWeaponSelectManager::CloseMenu()
{
	if (!m_bMenuOpen)
		return;
	
	m_bMenuOpen = false;
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	
	// If using non-instant mode, switch weapon on close
	if (!tfvr_weapon_select_instant.GetBool() && m_pPanel)
	{
		int selectedSlot = m_pPanel->GetSelectedSlot();
		if (selectedSlot >= 0)
		{
			char cmd[32];
			Q_snprintf(cmd, sizeof(cmd), "slot%d", selectedSlot + 1);
			engine->ClientCmd(cmd);
			
			// Play weapon selected sound
			if (pPlayer)
			{
				pPlayer->EmitSound("Player.WeaponSelected");
			}
		}
	}
	
	// Play close sound
	if (pPlayer)
	{
		pPlayer->EmitSound("Player.WeaponSelectionClose");
	}
	
	if (tfvr_weapon_select_debug.GetBool())
	{
		DevMsg("VR Weapon Select: Menu closed\n");
	}
}

bool CVRWeaponSelectManager::CalculateMenuTransform(VMatrix &transform)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return false;
	
	// Calculate current playspace world matrix using proper matrix math
	VMatrix headWorldMatrix = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
	VMatrix headPlayspaceMatrix = g_pOpenXRManager->GetMideyePose();
	VMatrix playspaceToHead = headPlayspaceMatrix.InverseTR();
	VMatrix playspaceWorldMatrix = headWorldMatrix * playspaceToHead;
	
	// Transform stored playspace position to current world position
	Vector menuCenter = playspaceWorldMatrix * m_vecMenuPlayspacePos;
	
	// Menu faces toward head using stored yaw (fixed when opened)
	Vector menuForward;
	AngleVectors(QAngle(0, m_flMenuYaw, 0), &menuForward, nullptr, nullptr);
	
	// Calculate right and up vectors
	Vector menuRight, menuUp;
	VectorVectors(menuForward, menuRight, menuUp);
	
	// Flip right vector for correct orientation
	menuRight = -menuRight;
	
	// DrawPanelIn3DSpace draws from top-left corner, so offset to center the panel
	// Move left by half width and up by half height to get top-left from center
	Vector menuTopLeft = menuCenter - menuRight * (m_flPanelWorldWidth * 0.5f) + menuUp * (m_flPanelWorldHeight * 0.5f);
	
	// Build transform matrix
	transform.Identity();
	transform[0][0] = menuRight.x;  transform[0][1] = menuUp.x;  transform[0][2] = menuForward.x;
	transform[1][0] = menuRight.y;  transform[1][1] = menuUp.y;  transform[1][2] = menuForward.y;
	transform[2][0] = menuRight.z;  transform[2][1] = menuUp.z;  transform[2][2] = menuForward.z;
	transform.SetTranslation(menuTopLeft);
	
	return true;
}

void CVRWeaponSelectManager::UpdateHandPositionOnMenu()
{
	if (!m_pPanel || !g_pOpenXRManager || !g_pOpenXRManager->IsActive())
		return;
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;
	
	// Use raw playspace positions for cursor tracking. This avoids drift caused
	// by the head-relative aim pose correction in GetRightControllerPose, which
	// shifts the controller's apparent world position when the head rotates.
	VMatrix handRawPlayspace;
	if (!g_pOpenXRManager->GetRightControllerPoseRaw(handRawPlayspace))
		return;
	
	Vector currentHandPlayspace = handRawPlayspace.GetTranslation();
	Vector deltaPlayspace = currentHandPlayspace - m_vecHandPlayspacePosAtOpen;
	
	// Project the playspace delta onto the menu plane using stored playspace orientation
	float u = DotProduct(deltaPlayspace, m_vecMenuRightPlayspace) / m_flPanelWorldWidth + 0.5f;
	float v = 0.5f - DotProduct(deltaPlayspace, m_vecMenuUpPlayspace) / m_flPanelWorldHeight;
	
	// Clamp to valid range
	u = clamp(u, 0.0f, 1.0f);
	v = clamp(v, 0.0f, 1.0f);
	
	m_pPanel->UpdateHandPosition(u, v);
	
	if (tfvr_weapon_select_debug.GetBool())
	{
		static float lastDebugTime = 0;
		if (gpGlobals->curtime - lastDebugTime > 0.5f)
		{
			DevMsg("VR Weapon Select: Hand UV=(%.2f, %.2f), delta=(%.1f,%.1f,%.1f)\n",
				u, v, deltaPlayspace.x, deltaPlayspace.y, deltaPlayspace.z);
			lastDebugTime = gpGlobals->curtime;
		}
	}
}

void CVRWeaponSelectManager::HandleWeaponSelection()
{
	if (!m_pPanel || !tfvr_weapon_select_instant.GetBool())
		return;
	
	int selectedSlot = m_pPanel->GetSelectedSlot();
	
	// Only process if selection changed
	if (selectedSlot != m_nLastSelectedSlot)
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		
		if (selectedSlot >= 0)
		{
			// Switch to selected weapon slot
			char cmd[32];
			Q_snprintf(cmd, sizeof(cmd), "slot%d", selectedSlot + 1);
			engine->ClientCmd(cmd);
			
			// Play weapon selection sound only when hovering onto a slot
			if (pPlayer)
			{
				pPlayer->EmitSound("Player.WeaponSelectionMoveSlot");
			}
			
			if (tfvr_weapon_select_debug.GetBool())
			{
				DevMsg("VR Weapon Select: Switching to slot %d\n", selectedSlot + 1);
			}
		}
	}
	
	m_nLastSelectedSlot = selectedSlot;
}

void CVRWeaponSelectManager::Update(float deltaTime)
{
	if (!m_bInitialized)
		return;
	
	// Check for weapon select button
	bool bWeaponSelectHeld = false;
	if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
	{
		bWeaponSelectHeld = g_pOpenXRManager->IsButtonPressed("weapon_select_hold");
	}
	
	// Handle menu open/close
	if (bWeaponSelectHeld && !m_bMenuOpen)
	{
		OpenMenu();
	}
	else if (!bWeaponSelectHeld && m_bMenuOpen)
	{
		CloseMenu();
	}
	
	// Update menu state if open
	if (m_bMenuOpen && m_pPanel)
	{
		UpdateHandPositionOnMenu();
		HandleWeaponSelection();
	}
}

// Priority for weapon select menu (should be on top of most things)
static const int PRIORITY_WEAPON_SELECT = 250;

void CVRWeaponSelectManager::Render()
{
	VPROF("CVRWeaponSelectManager::Render");
	
	if (!m_bInitialized || !m_bMenuOpen || !m_pPanel)
		return;
	
	if (!tfvr_weapon_select_enabled.GetBool())
		return;
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer || !pPlayer->IsAlive())
		return;
	
	VMatrix menuTransform;
	if (!CalculateMenuTransform(menuTransform))
		return;
	
	// Queue for distance-sorted rendering
	bool bWasVisible = m_pPanel->IsVisible();
	
	if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
	{
		g_pVRWorldUIQueue->QueuePanel(m_pPanel, menuTransform,
		                              m_nPanelPixelWidth, m_nPanelPixelHeight,
		                              m_flPanelWorldWidth, m_flPanelWorldHeight,
		                              PRIORITY_WEAPON_SELECT, true, bWasVisible);
	}
	else
	{
		// Fallback: render immediately
		m_pPanel->SetVisible(true);
		g_pMatSystemSurface->DisableClipping(true);
		g_pMatSystemSurface->DrawPanelIn3DSpace(
			m_pPanel->GetVPanel(),
			menuTransform,
			m_nPanelPixelWidth,
			m_nPanelPixelHeight,
			m_flPanelWorldWidth,
			m_flPanelWorldHeight
		);
		g_pMatSystemSurface->DisableClipping(false);
		m_pPanel->SetVisible(false);
	}
}
