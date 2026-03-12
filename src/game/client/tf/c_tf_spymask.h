//=============================================================================
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_SPYMASK_H
#define C_TF_SPYMASK_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class C_TFSpyMask : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_TFSpyMask, C_BaseAnimating );

	C_TFSpyMask();

	bool	InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup );
	virtual int		InternalDrawModel( int flags );
	virtual bool	ShouldDraw( void );
	virtual int		GetSkin( void );
};

#endif // C_TF_SPYMASK_H
