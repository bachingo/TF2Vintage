//========= Copyright (C) 2018, CSProMod Team, All rights reserved. =========//
//
// Purpose: provide world light related functions to the client
//
// As the engine provides no access to brush/model data (brushdata_t, model_t),
// we hence have no access to dworldlight_t. Therefore, we manually extract the
// world light data from the BSP itself, before entities are initialised on map
// load.
//
// To find the brightest light at a point, all world lights are iterated.
// Lights whose radii do not encompass our sample point are quickly rejected,
// as are lights which are not in our PVS, or visible from the sample point.
// If the sky light is visible from the sample point, then it shall supersede
// all other world lights.
//
// Written: November 2011
// Author: Saul Rennison
//
//===========================================================================//

#include "cbase.h"
#include "worldlight.h"
#include "bspfile.h"
#include "filesystem.h"

#include "tier0/memdbgon.h"

//static IVEngineServer *g_pEngineServer = NULL;

extern int GetClusterForOrigin( const Vector& org );
extern int GetPVSForCluster( int clusterIndex, int outputpvslength, byte* outputpvs );
extern bool CheckOriginInPVS( const Vector& org, const byte* checkpvs, int checkpvssize );

//-----------------------------------------------------------------------------
// Singleton exposure
//-----------------------------------------------------------------------------
static CWorldLights s_WorldLights;
CWorldLights* g_pWorldLights = &s_WorldLights;

//-----------------------------------------------------------------------------
// Purpose: calculate intensity ratio for a worldlight by distance
// Author: Valve Software
//-----------------------------------------------------------------------------
static float Engine_WorldLightDistanceFalloff( const dworldlight_t& wl, const Vector& delta )
{
	switch ( wl.type )
	{
		case emit_surface:
			// Cull out stuff that's too far
			if ( wl.radius != 0 )
			{
				if ( DotProduct( delta, delta ) > ( wl.radius * wl.radius ) )
					return 0.0f;
			}

			return InvRSquared( delta );

		case emit_skylight:
		case emit_skyambient:
			return 1.f;

		case emit_quakelight:
		{
			// X - r;
			const float falloff = wl.linear_attn - FastSqrt( DotProduct( delta, delta ) );
			if ( falloff < 0 )
				return 0.f;

			return falloff;
		}

		case emit_point:
		case emit_spotlight:	// directional & positional
		{
			const float dist2 = DotProduct( delta, delta );
			const float dist = FastSqrt( dist2 );

			// Cull out stuff that's too far
			if ( wl.radius != 0 && dist > wl.radius )
				return 0.f;

			return 1.f / ( wl.constant_attn + wl.linear_attn * dist + wl.quadratic_attn * dist2 );
		}
	}

	return 1.f;
}

//-----------------------------------------------------------------------------
// Purpose: initialise game system and members
//-----------------------------------------------------------------------------
CWorldLights::CWorldLights() : CAutoGameSystemPerFrame( "World lights" )
{
	m_nWorldLights = 0;
	m_pWorldLights = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: clear worldlights, free memory
//-----------------------------------------------------------------------------
void CWorldLights::Clear()
{
	m_nWorldLights = 0;

	if ( m_pWorldLights )
	{
		delete[] m_pWorldLights;
		m_pWorldLights = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: get all world lights from the BSP
//-----------------------------------------------------------------------------
void CWorldLights::LevelInitPreEntity()
{
	// Open map
	FileHandle_t hFile = g_pFullFileSystem->Open( VarArgs( "maps/%s.bsp", MapName() ), "rb" );
	if ( !hFile )
	{
		Warning( "CWorldLights: unable to open map\n" );
		return;
	}

	// Read the BSP header. We don't need to do any version checks, etc. as we
	// can safely assume that the engine did this for us
	dheader_t hdr;
	g_pFullFileSystem->Read( &hdr, sizeof( hdr ), hFile );

	// Grab the light lump and seek to it
	const lump_t& lightLump = hdr.lumps[g_pMaterialSystemHardwareConfig->GetHDREnabled() && engine->MapHasHDRLighting() ? LUMP_WORLDLIGHTS_HDR : LUMP_WORLDLIGHTS];

	// If we can't divide the lump data into a whole number of worldlights,
	// then the BSP format changed and we're unaware
	if ( lightLump.filelen % sizeof( dworldlight_t ) )
	{
		Warning( "CWorldLights: broken world light lump with size of %d, epcected to be multiple of %u\n", lightLump.filelen, sizeof( dworldlight_t ) );

		// Close file
		g_pFullFileSystem->Close( hFile );
		return;
	}

	g_pFullFileSystem->Seek( hFile, lightLump.fileofs, FILESYSTEM_SEEK_HEAD );

	// Allocate memory for the worldlights
	m_nWorldLights = lightLump.filelen / sizeof( dworldlight_t );
	m_pWorldLights = new dworldlight_t[m_nWorldLights];

	// Read worldlights then close
	g_pFullFileSystem->Read( m_pWorldLights, lightLump.filelen, hFile );
	g_pFullFileSystem->Close( hFile );

	DevMsg( "CWorldLights: load successful (%d lights at 0x%p)\n", m_nWorldLights, m_pWorldLights );
}

//-----------------------------------------------------------------------------
// Purpose: find the brightest light source at a point
//-----------------------------------------------------------------------------
bool CWorldLights::GetBrightestLightSource( const Vector& vecPosition, Vector& vecLightPos, Vector& vecLightBrightness ) const
{
	if ( !m_nWorldLights || !m_pWorldLights )
		return false;

	// Default light position and brightness to zero
	vecLightBrightness.Init();
	vecLightPos.Init();

	// Find the size of the PVS for our current position
	const int nCluster = GetClusterForOrigin( vecPosition );
	const int nPVSSize = GetPVSForCluster( nCluster, 0, NULL );

	// Get the PVS at our position
	byte* pvs = new byte[nPVSSize];
	GetPVSForCluster( nCluster, nPVSSize, pvs );

	// Iterate through all the worldlights
	for ( int i = 0; i < m_nWorldLights; ++i )
	{
		const dworldlight_t& light = m_pWorldLights[i];

		// Skip skyambient
		if ( light.type == emit_skyambient )
			continue;

		// Handle sun
		if ( light.type == emit_skylight )
		{
			if ( light.intensity.LengthSqr() <= vecLightBrightness.LengthSqr() )
				continue;

			const Vector& pos = vecPosition - light.normal * MAX_TRACE_LENGTH;

			trace_t tr;
			UTIL_TraceLine( vecPosition, pos, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );

			if ( !tr.DidHit() )
				continue;

			if ( !( tr.surface.flags & SURF_SKY ) && !( tr.surface.flags & SURF_SKY2D ) )
				continue;

			vecLightBrightness = light.intensity;
			vecLightPos = pos;
			continue;
		}

		// Calculate square distance to this worldlight
		const Vector& vecDelta = light.origin - vecPosition;
		const float flDistSqr = vecDelta.LengthSqr();
		const float flRadiusSqr = light.radius * light.radius;

		// Skip lights that are out of our radius
		if ( light.type != emit_spotlight && flRadiusSqr > 0 && flDistSqr >= flRadiusSqr )
			continue;

		// Is it out of our PVS?
		if ( !CheckOriginInPVS( light.origin, pvs, nPVSSize ) )
			continue;

		// Calculate intensity at our position
		const float flRatio = Engine_WorldLightDistanceFalloff( light, vecDelta );
		Vector vecIntensity = light.intensity * flRatio;

		// Is this light more intense than the one we already found?
		if ( vecIntensity.LengthSqr() <= vecLightBrightness.LengthSqr() )
			continue;

		// Can we see the light?
		trace_t tr;
		const Vector& vecAbsStart = vecPosition + Vector( 0, 0, 30 );
		UTIL_TraceLine( vecAbsStart, light.origin, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );

		if ( tr.DidHit() )
			continue;

		vecLightPos = light.origin;
		vecLightBrightness = vecIntensity;
	}

	delete[] pvs;
	return !vecLightBrightness.IsZero();
}