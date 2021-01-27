//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef RTIME_H
#define RTIME_H
#ifdef _WIN32
#pragma once
#endif

#include "platform.h"
#include <time.h>

enum
{
	k_ERTimeRenderBufSize = 25
};

enum class ETimeUnit
{
	None,
	Second,
	Minute,
	Hour,
	Day,
	Week,
	Month,
	Year,
	Forever
};

//-----------------------------------------------------------------------------
// Purpose: Encapsulation of real world time clocks
//-----------------------------------------------------------------------------
class CRTime
{
	static RTime32 sm_nTimeCur;
	static char sm_rgchLocalTimeCur[16];
	static char sm_rgchLocalDateCur[16];
	static RTime32 sm_nTimeLastSystemTimeUpdate;

public:
	CRTime();
	CRTime( RTime32 const &nTime );

	int					CSecsPassed() const;
	int					GetDayOfYear() const;
	int					GetDayOfMonth() const;
	int					GetDayOfWeek() const;
	int					GetHour() const;
	int					GetISOWeekOfYear() const;
	int					GetMinute() const;
	int					GetMonth() const;
	int					GetSecond() const;
	int					GetYear() const;
	const char*			Render( char( &buf )[k_ERTimeRenderBufSize] ) const;

	static bool			BIsLeapYear( int nYear );
	static ETimeUnit	FindTimeBoundaryCrossings( RTime32 nTime1, RTime32 nTime2, bool *pbWeekChanged );
	static char*		PchTimeCur() { return sm_rgchLocalTimeCur; }
	static char*		PchDateCur() { return sm_rgchLocalDateCur; }
	static const char*	Render( RTime32 nTime, char( &buf )[k_ERTimeRenderBufSize] );
	static RTime32		RTime32BeginningOfDay( RTime32 nTime );
	static RTime32		RTime32BeginningOfNextDay( RTime32 nTime );
	static RTime32		RTime32DateAdd( RTime32 nTime, int nAmount, ETimeUnit eTimeAmountType );
	static RTime32		RTime32FirstDayOfMonth( RTime32 nTime );
	static RTime32		RTime32FirstDayOfNextMonth( RTime32 rTime32 );
	static RTime32		RTime32FromFmtString( const char *fmt, const char *pszValue );
	static RTime32		RTime32FromHTTPDateString( const char *pszValue );
	static RTime32		RTime32FromRFC3339UTCString( const char *pszValue );
	static RTime32		RTime32FromString( const char *pszValue );
	static const char*	RTime32ToDayString( RTime32 nTime, char (&buf)[k_ERTimeRenderBufSize], bool bUseGMT = false );
	static const char*	RTime32ToRFC3339UTCString( RTime32 nTime, char( &buf )[k_ERTimeRenderBufSize] );
	static const char*	RTime32ToString( RTime32 nTime, char( &buf )[k_ERTimeRenderBufSize], bool bNoPunct = false, bool bNoTime = false );
	static RTime32		RTime32LastDayOfMonth( RTime32 nTime );
	static RTime32		RTime32LastDayOfNextMonth( RTime32 nTime );
	static RTime32		RTime32MonthAddChooseDay( int nNthDayOfMonth, RTime32 nStartDate, int nMonthsToAdd );
	static RTime32		RTime32NthDayOfMonth( RTime32 nTime, int nDay );
	static RTime32		RTime32TimeCur() { return sm_nTimeCur; }
	static void			SetSystemClock( RTime32 nCurrentTime );
	static void			UpdateRealTime();

	unsigned			GetStartTime() const { return m_nStartTime; }
	void				SetStartTime( RTime32 nTime ) { m_nStartTime = nTime; }
	void				SetGMT( bool bUseGMT ) { m_bGMT = bUseGMT;}
	bool				BIsGMT() const { return m_bGMT; }

	struct tm*			GetGMTComplTMTime( void ) const;

private:
	RTime32 m_nStartTime;
	bool m_bGMT;
};

#endif