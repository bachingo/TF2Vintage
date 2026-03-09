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

class CEnvLight : public CLight
{
public:
	DECLARE_CLASS( CEnvLight, CLight );
	DECLARE_DATADESC();

	bool	KeyValue( const char *szKeyName, const char *szValue ); 
	void	Spawn( void );

	void	FadeThink(void);

	void	TurnOn(void);
	void	TurnOff(void);
	void	Toggle(void);

	void	InputToggle(inputdata_t& inputdata);
	void	InputTurnOn(inputdata_t& inputdata);
	void	InputTurnOff(inputdata_t& inputdata);

	// Sun pitch (degrees, positive = down). Set via the "pitch" key.
	int		m_iPitch;

	// Raw RGBA sun color as parsed from the "_light" key (0-255 per channel).
	// Stored so csm_autospawn (and anything else) can read it without
	// needing friend access or a separate lookup.
	// w/a is the brightness scalar (the 4th value in the Hammer color field).
	// Returns the sun color as (r, g, b, brightness) in 0-255 range.
	Vector4D GetLightColor() const
	{
		return Vector4D( m_vecLightRGB.x, m_vecLightRGB.y, m_vecLightRGB.z, m_flLightBrightness );
	}

protected:
	// Raw sun color (0-255 per channel) from the "_light" BSP key.
	Vector		m_vecLightRGB;		// R, G, B
	float		m_flLightBrightness;	// 4th value (Hammer brightness scalar)

private:
	int		m_iStyle;
	int		m_iDefaultStyle;
	string_t m_iszPattern;
	char	m_iCurrentFade;
	char	m_iTargetFade;
};

#endif // LIGHTS_H
