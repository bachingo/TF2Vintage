//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "rtime.h"
#include "platform.h"
#include "commonmacros.h"
#include "tier1/strtools.h"

#ifdef _WIN32
#include "winlite.h"
#else
#include <sys/time.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

RTime32 CRTime::sm_nTimeCur = 0;
char CRTime::sm_rgchLocalTimeCur[16]{};
char CRTime::sm_rgchLocalDateCur[16]{};
RTime32 CRTime::sm_nTimeLastSystemTimeUpdate = 0;


CRTime::CRTime()
{
	if ( sm_nTimeCur == 0 )
		sm_nTimeCur = time( NULL );

	m_nStartTime = sm_nTimeCur;
	m_bGMT = false;
}

CRTime::CRTime( RTime32 const &nTime )
	: m_nStartTime(nTime), m_bGMT(false)
{
}

//-----------------------------------------------------------------------------
// Purpose: Get seconds since epoch
//-----------------------------------------------------------------------------
int CRTime::CSecsPassed() const
{
	return ( sm_nTimeCur - m_nStartTime );
}

//-----------------------------------------------------------------------------
// Purpose: Get current day of the year [0, 365]
//-----------------------------------------------------------------------------
int CRTime::GetDayOfYear() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_yday;
}

//-----------------------------------------------------------------------------
// Purpose: Get current day of the month [1, 31]
//-----------------------------------------------------------------------------
int CRTime::GetDayOfMonth() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_mday;
}

//-----------------------------------------------------------------------------
// Purpose: Get current day of the week [0, 6]
//-----------------------------------------------------------------------------
int CRTime::GetDayOfWeek() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_wday;
}

//-----------------------------------------------------------------------------
// Purpose: Get current hour [0, 23]
//-----------------------------------------------------------------------------
int CRTime::GetHour() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_hour;
}

//-----------------------------------------------------------------------------
// Purpose: Get ISO standardized week of the year
//-----------------------------------------------------------------------------
int CRTime::GetISOWeekOfYear() const
{
	return GetDayOfYear() - ( 1 + GetDayOfWeek() ) / 7;
}

//-----------------------------------------------------------------------------
// Purpose: Get current minute [0, 59]
//-----------------------------------------------------------------------------
int CRTime::GetMinute() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_min;
}

//-----------------------------------------------------------------------------
// Purpose: Get current month [0, 11]
//-----------------------------------------------------------------------------
int CRTime::GetMonth() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_mon;
}

//-----------------------------------------------------------------------------
// Purpose: Get current second [0, 59]
//-----------------------------------------------------------------------------
int CRTime::GetSecond() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_sec;
}

//-----------------------------------------------------------------------------
// Purpose: Get current year (years since 1900, so 1900 is added)
//-----------------------------------------------------------------------------
int CRTime::GetYear() const
{
	struct tm *pCurTime = GetGMTComplTMTime();
	return pCurTime->tm_year + 1900;
}

//-----------------------------------------------------------------------------
// Purpose: Render time (Sun Jan 01 00:00:00 1970\0)
//-----------------------------------------------------------------------------
const char *CRTime::Render( char( &buf )[k_ERTimeRenderBufSize] ) const
{
	return Render( sm_nTimeCur, buf );
}

//-----------------------------------------------------------------------------
// Purpose: Helper for leap years
//-----------------------------------------------------------------------------
bool CRTime::BIsLeapYear( int nYear )
{
	// Every 4 years, unless it is a century. Or if it is every 4th century
	if ( ( nYear % 4 == 0 && nYear % 100 != 0) || nYear % 400 == 0)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return a unit length of time which 2 times have crossed (year, month, day etc.)
//-----------------------------------------------------------------------------
ETimeUnit CRTime::FindTimeBoundaryCrossings( RTime32 nTime1, RTime32 nTime2, bool *pWeekChanged )
{
	time_t time1 = nTime1;
	time_t time2 = nTime2;
	struct tm tmp;

	struct tm *pTime1 = Plat_localtime( &time1, &tmp );
	if ( !pTime1 )
		return ETimeUnit::Forever;

	struct tm *pTime2 = Plat_localtime( &time2, &tmp );
	if ( !pTime2 )
		return ETimeUnit::Forever;

	*pWeekChanged = false;

	const int nSecondsAWeek = 60 * 60 * 24 * 7; // seconds in a minute * minutes in an hour * hours in a day * days in a week
	// If the difference is more than 6 days, we crossed a week boundary
	if ( ( nTime1 > nTime2 && ( nTime1 - nTime2 ) > nSecondsAWeek )
		 || ( nTime2 > nTime1 && ( nTime2 - nTime1 ) > nSecondsAWeek ) )
	{
		*pWeekChanged = true;
	}
	else if ( pTime1->tm_yday != pTime2->tm_yday )
	{
		// Or one time was Friday and the other later time was Sunday
		if ( nTime2 > nTime1 )
		{
			if ( pTime2->tm_wday <= pTime1->tm_wday )
				*pWeekChanged = true;
		}
		else
		{
			if ( pTime1->tm_wday <= pTime2->tm_wday )
				*pWeekChanged = true;
		}
	}

	// Larger boundries will consider smaller boundries as crossed as well
	// so travel from largest to shortest

	if ( pTime1->tm_year != pTime2->tm_year )
		return ETimeUnit::Year;

	if ( pTime1->tm_mon != pTime2->tm_mon )
		return ETimeUnit::Month;

	// If the week changed, return that now
	if ( *pWeekChanged )
		return ETimeUnit::Week;

	if ( pTime1->tm_yday != pTime2->tm_yday )
		return ETimeUnit::Day;

	if ( pTime1->tm_hour != pTime2->tm_hour )
		return ETimeUnit::Hour;

	// If DST changed but somehow hours didn't, check that here
	if ( pTime1->tm_isdst != pTime2->tm_isdst )
		return ETimeUnit::Hour;

	if ( pTime1->tm_min != pTime2->tm_min )
		return ETimeUnit::Minute;

	if ( pTime1->tm_sec != pTime2->tm_sec )
		return ETimeUnit::Second;

	// Nothing changed
	return ETimeUnit::None;
}

//-----------------------------------------------------------------------------
// Purpose: Render time (Sun Jan 01 00:00:00 1970\0)
//-----------------------------------------------------------------------------
const char *CRTime::Render( RTime32 nTime, char( &buf )[k_ERTimeRenderBufSize] )
{
	if ( !buf )
		return nullptr;

	char szTime[32];
	if ( !Plat_ctime( (time_t *)&nTime, szTime, sizeof( szTime ) ) )
		return nullptr;

	// Trim \n from result
	szTime[25] = '\0';

	if ( nTime == 0x7FFFFFFF )
		Q_strncpy( buf, "Infinite time value", k_ERTimeRenderBufSize );
	else if ( nTime == 0U )
		Q_strncpy( buf, "Nil time value", k_ERTimeRenderBufSize );
	else if ( nTime < 10U )
		Q_strncpy( buf, "Invalid time value", k_ERTimeRenderBufSize );
	else
		Q_strncpy( buf, szTime, k_ERTimeRenderBufSize );

	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32BeginningOfDay( RTime32 nTime )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	// Midnight is technically the begining
	pCurTime->tm_hour = 0;
	pCurTime->tm_min = 0;
	pCurTime->tm_sec = 0;
	// Compute DST
	pCurTime->tm_isdst = -1;

	return mktime( pCurTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32BeginningOfNextDay( RTime32 nTime )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	// Increasing the month day will allow it to roll over
	pCurTime->tm_mday += 1;
	// Midnight is technically the begining
	pCurTime->tm_hour = 0;
	pCurTime->tm_min = 0;
	pCurTime->tm_sec = 0;
	// Compute DST
	pCurTime->tm_isdst = -1;

	return mktime( pCurTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32DateAdd( RTime32 nTime, int nAmount, ETimeUnit eTimeAmountType )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	switch ( eTimeAmountType )
	{
		default:
			break;
		case ETimeUnit::Forever:
			return 0x7FFFFFFF;
		case ETimeUnit::Year:
			pCurTime->tm_year += nAmount;
			break;
		case ETimeUnit::Month:
			pCurTime->tm_mon += nAmount;
			break;
		case ETimeUnit::Week:
			pCurTime->tm_mday += 7 * nAmount;
			break;
		case ETimeUnit::Day:
			pCurTime->tm_mday += nAmount;
			break;
		case ETimeUnit::Hour:
			pCurTime->tm_hour += nAmount;
			break;
		case ETimeUnit::Minute:
			pCurTime->tm_min += nAmount;
			break;
		case ETimeUnit::Second:
			pCurTime->tm_sec += nAmount;
			break;
	}

	// Compute DST
	pCurTime->tm_isdst = -1;
	return mktime( pCurTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32FirstDayOfMonth( RTime32 nTime )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	// Month days start at 1, else it rolls back
	pCurTime->tm_mday = 1;
	// Midnight is technically the begining
	pCurTime->tm_hour = 0;
	pCurTime->tm_min = 0;
	pCurTime->tm_sec = 0;
	// Compute DST
	pCurTime->tm_isdst = -1;

	return mktime( pCurTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32FirstDayOfNextMonth( RTime32 nTime )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	pCurTime->tm_mon += 1;
	// Month days start at 1, else it rolls back
	pCurTime->tm_mday = 1;
	// Midnight is technically the begining
	pCurTime->tm_hour = 0;
	pCurTime->tm_min = 0;
	pCurTime->tm_sec = 0;
	// Compute DST
	pCurTime->tm_isdst = -1;

	return mktime( pCurTime );
}

//-----------------------------------------------------------------------------
// Purpose: Given a custom (but valid) time format, return a time value
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32FromFmtString( const char *fmt, const char *pszValue )
{
	int nFmtLen = Q_strlen( fmt );
	int nValueLen = Q_strlen( pszValue );
	// Format should match the value
	if ( nFmtLen != nValueLen || nFmtLen < 4 )
		return 0;

	struct tm tm;
	Q_memset( &tm, 0, sizeof( tm ) );
	tm.tm_isdst = -1;

	char szValue[64];
	V_strcpy_safe( szValue, pszValue );

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32FromHTTPDateString( const char *pszValue )
{
	// these seem to not be used, and the strptime function is very complicated,
	// so I'm opting out of recreating them
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32FromRFC3339UTCString( const char *pszValue )
{
	// these seem to not be used, and the strptime function is very complicated,
	// so I'm opting out of recreating them
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Given a string of "YYYY-MM-DD hh:mm:ss", return a time value
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32FromString( const char *pszValue )
{
	struct tm tm;
	Q_memset( &tm, 0, sizeof( tm ) );
	tm.tm_isdst = -1;

	char szValue[64];
	V_strcpy_safe( szValue, pszValue );

	// This is a headache
	// enough space for the year plus null
	char num[5];

	num[0] = szValue[0]; 
	num[1] = szValue[1]; 
	num[2] = szValue[2]; 
	num[3] = szValue[3]; 
	num[4] = '\0';
	tm.tm_year = strtol( num, NULL, 10 ) - 1900;
	// skip over '-'
	num[0] = szValue[5]; 
	num[1] = szValue[6]; 
	num[2] = '\0';
	tm.tm_mon = strtol( num, NULL, 10 ) - 1;
	// skip over '-'
	num[0] = szValue[8]; 
	num[1] = szValue[9]; 
	num[2] = '\0';
	tm.tm_mday = strtol( num, NULL, 10 );

	// if it's got a time
	if ( szValue[10] != '\0' )
	{
		// skip over ' ' or 'T'
		num[0] = szValue[11]; 
		num[1] = szValue[12]; 
		num[2] = '\0';
		tm.tm_hour = strtol( num, NULL, 10 );
		// skip over ':'
		num[0] = szValue[14]; 
		num[1] = szValue[15]; 
		num[2] = '\0';
		tm.tm_min = strtol( num, NULL, 10 );
		// skip over ':'
		num[0] = szValue[17]; 
		num[1] = szValue[18]; 
		num[2] = '\0';
		tm.tm_sec = strtol( num, NULL, 10 );
	}

	return mktime( &tm );
}

//-----------------------------------------------------------------------------
// Purpose: Given a time value, return the day portion
//-----------------------------------------------------------------------------
const char *CRTime::RTime32ToDayString( RTime32 nTime, char( &buf )[k_ERTimeRenderBufSize], bool bUseGMT )
{
	if ( !buf )
		return nullptr;

	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = bUseGMT ? Plat_gmtime( &nCurTime, &tmp ) : Plat_localtime( &nCurTime, &tmp );

	strftime( buf, sizeof( buf ), "%b %d", pCurTime );
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Give a UTC time, return RFC3339 formatted time
//-----------------------------------------------------------------------------
const char *CRTime::RTime32ToRFC3339UTCString( RTime32 nTime, char( &buf )[k_ERTimeRenderBufSize] )
{
	if ( !buf )
		return nullptr;

	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return "NIL";

	if ( nTime == 0U )
		return "NIL";
	else if ( nTime == 0x7FFFFFFF )
		return "Infinite time value";
	else if ( nTime < 10U )
		return "Invalid time value";

	V_sprintf_safe( buf, "%04u-%02u-%02uT%02u:%02u:%02uZ",
					pCurTime->tm_year+1900, pCurTime->tm_mon+1, pCurTime->tm_mday,
					pCurTime->tm_hour, pCurTime->tm_min, pCurTime->tm_sec );

	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Given a time, return "YYYY-MM-DD (hh:mm:ss)"
//-----------------------------------------------------------------------------
const char *CRTime::RTime32ToString( RTime32 nTime, char( &buf )[k_ERTimeRenderBufSize], bool bNoPunct, bool bNoTime )
{
	if ( !buf )
		return nullptr;

	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return "NIL";

	if ( nTime == 0U )
		return "NIL";
	else if ( nTime == 0x7FFFFFFF )
		return "Infinite time value";
	else if ( nTime < 10U )
		return "Invalid time value";

	if ( bNoTime )
	{
		V_sprintf_safe( buf, bNoPunct ? "%04u%02u%02u" : "%04u-%02u-%02u",
						pCurTime->tm_year+1900, pCurTime->tm_mon+1, pCurTime->tm_mday );
	}
	else
	{
		V_sprintf_safe( buf, bNoPunct ? "%04u%02u%02u%02u%02u%02u" : "%04u-%02u-%02u %02u:%02u:%02u",
						pCurTime->tm_year+1900, pCurTime->tm_mon+1, pCurTime->tm_mday, 
						pCurTime->tm_hour, pCurTime->tm_min, pCurTime->tm_sec );
	}

	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32LastDayOfMonth( RTime32 nTime )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	pCurTime->tm_mon += 1;
	// Zeroth day of the month rolls it back to day of last month
	pCurTime->tm_mday = 0;
	// Midnight is technically the begining
	pCurTime->tm_hour = 0;
	pCurTime->tm_min = 0;
	pCurTime->tm_sec = 0;
	// Compute DST
	pCurTime->tm_isdst = -1;

	return mktime( pCurTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32LastDayOfNextMonth( RTime32 nTime )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	pCurTime->tm_mon += 2;
	// Zeroth day of the month rolls it back to day of last month
	pCurTime->tm_mday = 0;
	// Midnight is technically the begining
	pCurTime->tm_hour = 0;
	pCurTime->tm_min = 0;
	pCurTime->tm_sec = 0;
	// Compute DST
	pCurTime->tm_isdst = -1;

	return mktime( pCurTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32MonthAddChooseDay( int nNthDayOfMonth, RTime32 nStartDate, int nMonthsToAdd )
{
	RTime32 nFirstDayOfMonth = RTime32FirstDayOfMonth( nStartDate );
	RTime32 nFirstDayOfDesiredMonth = RTime32DateAdd( nFirstDayOfMonth, nMonthsToAdd, ETimeUnit::Month );
	return RTime32NthDayOfMonth( nFirstDayOfDesiredMonth, nNthDayOfMonth );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 CRTime::RTime32NthDayOfMonth( RTime32 nTime, int nDay )
{
	time_t nCurTime = nTime;
	struct tm tmp;
	struct tm *pCurTime = Plat_localtime( &nCurTime, &tmp );
	if ( !pCurTime )
		return 0;

	pCurTime->tm_mday = nDay;
	// Midnight is technically the begining
	pCurTime->tm_hour = 0;
	pCurTime->tm_min = 0;
	pCurTime->tm_sec = 0;
	// Compute DST
	pCurTime->tm_isdst = -1;

	int nCurMonth = pCurTime->tm_mon;
	time_t nTimeThen = mktime( pCurTime );

	// Handle improper usage, just return last day
	if ( pCurTime->tm_mon != nCurMonth )
	{
		// Zeroth day trickery
		pCurTime->tm_mday = 0;
		pCurTime->tm_isdst = -1;

		nTimeThen = mktime( pCurTime );
	}

	return nTimeThen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRTime::SetSystemClock( RTime32 nCurrentTime )
{
#ifdef _WIN32
	FILETIME fileTime{};
	SYSTEMTIME systemTime{};
	uint64 ulTemp = ( ( (uint64)nCurrentTime ) * 10000000 ) + 116444736000000000;
	fileTime.dwLowDateTime = (DWORD)ulTemp;
	fileTime.dwHighDateTime = ulTemp >> 32;

	if ( !FileTimeToSystemTime( &fileTime, &systemTime ) )
		return;

	SetSystemTime( &systemTime );
	sm_nTimeCur = nCurrentTime;
#endif // _WIN32
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRTime::UpdateRealTime()
{
	sm_nTimeCur = time( NULL );

	if ( ( sm_nTimeCur - sm_nTimeLastSystemTimeUpdate ) >= 1 )
	{
	#ifdef _WIN32
		SYSTEMTIME systemTimeLocal;
		GetLocalTime( &systemTimeLocal );
		GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systemTimeLocal, "HH:mm:ss", sm_rgchLocalTimeCur, sizeof( sm_rgchLocalTimeCur ) );
		GetDateFormat( LOCALE_USER_DEFAULT, 0, &systemTimeLocal, "MM/dd/yy", sm_rgchLocalDateCur, sizeof( sm_rgchLocalDateCur ) );
	#endif // _WIN32

		sm_nTimeLastSystemTimeUpdate = sm_nTimeCur;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return a tm struct filled with current time respecting GMT
//-----------------------------------------------------------------------------
struct tm *CRTime::GetGMTComplTMTime( void ) const
{
	time_t nCurTime = m_nStartTime;
	struct tm garbage;
	return m_bGMT ? Plat_gmtime( &nCurTime, &garbage ) : Plat_localtime( &nCurTime, &garbage );
}
