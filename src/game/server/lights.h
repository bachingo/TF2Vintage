//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef LIGHTS_H
#define LIGHTS_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLight : public CPointEntity
{
public:
	DECLARE_CLASS( CLight, CPointEntity );

	bool	KeyValue( const char *szKeyName, const char *szValue );
	void	Spawn( void );
	void	FadeThink( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	void	TurnOn( void );
	void	TurnOff( void );
	void	Toggle( void );

	// Input handlers
	void	InputSetPattern( inputdata_t &inputdata );
	void	InputFadeToPattern( inputdata_t &inputdata );

	void	InputToggle( inputdata_t &inputdata );
	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	int		m_iStyle;
	int		m_iDefaultStyle;
	string_t m_iszPattern;
	char	m_iCurrentFade;
	char	m_iTargetFade;
};

#endif // LIGHTS_H

//-----------------------------------------------------------------------------
// CEnvLight — TF2V extension of the base light_environment entity.
//
// Adds networked sun state (angles, colour, ambient) that sky_tod.cpp drives
// per-frame, plus I/O handlers and the _light key parser used by csm_autospawn.
// The client C_EnvLight receives these via DT_CEnvLight (see c_lights.h/cpp).
//-----------------------------------------------------------------------------
class CEnvLight : public CLight
{
public:
	DECLARE_CLASS( CEnvLight, CLight );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	bool	KeyValue( const char *szKeyName, const char *szValue ); 
	void	Spawn( void );
	void	Think( void );		// 20 Hz — propagates NetworkStateChanged for networked members
	void	FadeThink( void );	// Light-fade helper (carries over from base CLight)

	void	TurnOn( void );
	void	TurnOff( void );
	void	Toggle( void );

	void	InputToggle( inputdata_t &inputdata );
	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

	// Raw sun colour (0-255 per channel) parsed from the Hammer "_light" key.
	// Available after Spawn(); used by csm_autospawn to seed the initial CSM colour.
	// Returns Vector4D(r, g, b, brightness) all in [0, 255] range.
	Vector4D GetLightColor() const
	{
		return Vector4D( m_vecLightRGB.x, m_vecLightRGB.y, m_vecLightRGB.z, m_flLightBrightness );
	}

	// ---- Networked sun state (driven by sky_tod.cpp at runtime) ----
	// These replicate to C_EnvLight; viewrender.cpp reads them for the CSM pass.
	CNetworkVar( QAngle, m_angSunAngles );				// yaw / pitch / roll
	CNetworkVector( m_vecLight );						// direct sun colour (linear 0–1)
	CNetworkVector( m_vecAmbient );						// sky ambient colour (linear 0–1)
	CNetworkVar( bool, m_bCascadedShadowMappingEnabled );

protected:
	// Raw parsed sun colour from BSP — NOT replicated, server-only.
	Vector	m_vecLightRGB;			// R, G, B in [0, 255]
	float	m_flLightBrightness;	// 4th value from Hammer colour field

private:
	// These mirror CLight internals we need for FadeThink/TurnOn/TurnOff.
	// CLight::m_iStyle, m_iDefaultStyle, m_iszPattern, m_iCurrentFade,
	// m_iTargetFade, m_iPitch are declared in the CLight base (lights.cpp).
	// We redeclare m_iPitch here since CLight keeps it private.
	int		m_iPitch;
};

