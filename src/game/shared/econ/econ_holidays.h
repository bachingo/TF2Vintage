//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef ECON_HOLIDAYS_H
#define ECON_HOLIDAYS_H
#ifdef _WIN32
#pragma once
#endif

char const *EconHolidays_GetActiveHolidayString( void );
int	EconHolidays_GetHolidayForString( char const *pszHolidayName );
bool EconHolidays_IsHolidayActive( int nHoliday, class CRTime const& timeCurrent );
RTime32 EconHolidays_TerribleHack_GetHalloweenEndData( void );

#endif // ECON_HOLIDAYS_H
