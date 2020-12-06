//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tier1//strtools.h"
#include "econ_holidays.h"
#include "rtime.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
abstract_class IIsHolidayActive
{
public:
	IIsHolidayActive( const char *pszHolidayName ) : m_pszHolidayName( pszHolidayName ) { }
	virtual ~IIsHolidayActive() { }
	virtual bool IsActive( CRTime const& timeCurrent ) = 0;

	const char *GetHolidayName() const { return m_pszHolidayName; }

private:
	const char *m_pszHolidayName;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNoHoliday : public IIsHolidayActive
{
public:
	CNoHoliday() : IIsHolidayActive( "none" ) { }

	virtual bool IsActive( CRTime const& timeCurrent )
	{
		return false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class COrHoliday : public IIsHolidayActive
{
public:
	COrHoliday( const char *pszName, IIsHolidayActive *pA, IIsHolidayActive *pB )
		: IIsHolidayActive( pszName ), m_pA( pA ), m_pB( pB ) { }

	virtual bool IsActive( CRTime const& timeCurrent )
	{
		return m_pA->IsActive( timeCurrent ) || m_pB->IsActive( timeCurrent );
	}

private:
	IIsHolidayActive *m_pA;
	IIsHolidayActive *m_pB;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDateBasedHoliday : public IIsHolidayActive
{
public:
	CDateBasedHoliday( const char *pszName, const char *pszStartTime, const char *pszEndTime )
		: IIsHolidayActive( pszName )
	{
		m_StartTime = CRTime::RTime32FromString( pszStartTime );
		m_EndTime = CRTime::RTime32FromString( pszEndTime );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return ( ( timeCurrent.GetStartTime() >= m_StartTime.GetStartTime() ) && 
				 ( timeCurrent.GetStartTime() <= m_EndTime.GetStartTime() ) );
	}

	RTime32 GetEndTime() const
	{
		return m_EndTime.GetStartTime();
	}

private:
	CRTime m_StartTime;
	CRTime m_EndTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDateBasedHolidayNoSpecificYear : public IIsHolidayActive
{
public:
	CDateBasedHolidayNoSpecificYear( const char *pszName, const char *pszStartTime, const char *pszEndTime )
		: IIsHolidayActive( pszName ), m_pszStartTime( pszStartTime ), m_pszEndTime( pszEndTime ), m_iYear( -1 ) { }

	virtual bool IsActive( CRTime const& timeCurrent )
	{
		const int iYear = timeCurrent.GetYear();

		if ( iYear != m_iYear )
		{
			char szStartTime[k_ERTimeRenderBufSize];
			char szEndTime[k_ERTimeRenderBufSize];

			V_sprintf_safe( szStartTime, "%d-%s", iYear, m_pszStartTime );
			V_sprintf_safe( szEndTime, "%d-%s", iYear, m_pszEndTime );

			m_iYear = iYear;
			m_StartTime = CRTime::RTime32FromString( szStartTime );
			m_EndTime = CRTime::RTime32FromString( szEndTime );
		}

		return ( ( timeCurrent.GetStartTime() >= m_StartTime.GetStartTime() ) && 
				 ( timeCurrent.GetStartTime() <= m_EndTime.GetStartTime() ) );
	}

	RTime32 GetEndTime() const
	{
		return m_EndTime.GetStartTime();
	}

private:
	const char *m_pszStartTime;
	const char *m_pszEndTime;

	int m_iYear;
	CRTime m_StartTime;
	CRTime m_EndTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCyclicalHoliday : public IIsHolidayActive
{
public:
	CCyclicalHoliday( const char *pszName, int iMonth, int iDay, int iYear, float fCycleLengthInDays, float fFudgeTimeInDays )
		: IIsHolidayActive( pszName ), m_flCycleLength( fCycleLengthInDays ), m_flFudgeTime( fFudgeTimeInDays )
	{
		struct tm tm;
		Q_memset( &tm, 0, sizeof( tm ) );
		tm.tm_year = iYear - 1900; // years since 1900
		tm.tm_mon = iMonth - 1;
		tm.tm_mday = iDay;
		m_nStartTime = mktime( &tm );
	}

	virtual bool IsActive( CRTime const &timeCurrent )
	{
		const int iCycleLength = m_flCycleLength * 60 * 60 * 24; // in seconds a day
		const int iBufferLength = m_flFudgeTime * 60 * 60 * 24; // in seconds a day
		const int iSecondsPassedInCycle = ( timeCurrent.GetStartTime() - m_nStartTime ) % iCycleLength;

		if ( iSecondsPassedInCycle < iBufferLength )
			return true;
		if ( iSecondsPassedInCycle > (iCycleLength - iBufferLength) )
			return true;

		return false;
	}
private:
	time_t m_nStartTime;

	float m_flCycleLength;
	float m_flFudgeTime;
};

static CNoHoliday g_Holiday_NoHoliday{};
static CDateBasedHolidayNoSpecificYear g_Holiday_TF2Birthday( "tf_birthday", "08-23", "08-25" );
static CDateBasedHolidayNoSpecificYear g_Holiday_TF2VBirthday( "vintage_birthday", "07-03", "07-05" );
static COrHoliday g_Holiday_Birthday( "birthday", &g_Holiday_TF2Birthday, &g_Holiday_TF2VBirthday );
static CDateBasedHolidayNoSpecificYear g_Holiday_Halloween( "halloween", "10-23", "11-02" );
static CDateBasedHolidayNoSpecificYear g_Holiday_Christmas( "christmas", "12-20", "01-01" );
static CDateBasedHolidayNoSpecificYear g_Holiday_ValentinesDay( "valentines", "02-13", "02-15" );
static CDateBasedHoliday g_Holiday_MeetThePyro( "meet_the_pyro", "2012-06-26", "2012-07-05" );
static CCyclicalHoliday g_Holiday_FullMoon( "fullmoon", 5, 21, 2016, 29.53f, 1.0f );
static COrHoliday g_Holiday_HalloweenOrFullMoon( "halloween_or_fullmoon", &g_Holiday_Halloween, &g_Holiday_FullMoon );
static COrHoliday g_Holiday_HalloweenOrFullMoonOrValentines( "halloween_or_fullmoon_or_valentines", &g_Holiday_HalloweenOrFullMoon, &g_Holiday_ValentinesDay );
static CDateBasedHolidayNoSpecificYear g_Holiday_AprilFools( "april_fools", "03-31", "04-02" );
static CDateBasedHoliday g_Holiday_EndOfTheLine( "eotl_launch", "2014-12-03", "2015-01-05" );
static CDateBasedHoliday g_Holiday_CommunityUpdate( "community_update", "2015-09-01", "2015-11-05" );
static CDateBasedHoliday g_Holiday_LoveAndWar( "bread_update", "2014-06-18", "2014-07-09" );
static CDateBasedHolidayNoSpecificYear g_Holiday_SoldierMemorial( "soldier_memorial", "04-30", "06-01" );

static IIsHolidayActive *s_HolidayChecks[] =
{
	&g_Holiday_NoHoliday,							// kHoliday_None
	&g_Holiday_Birthday,							// kHoliday_TF2Birthday
	&g_Holiday_Halloween,							// kHoliday_Halloween
	&g_Holiday_Christmas,							// kHoliday_Christmas
	&g_Holiday_CommunityUpdate,						// kHoliday_CommunityUpdate
	&g_Holiday_EndOfTheLine,						// kHoliday_EOTL
	&g_Holiday_ValentinesDay,						// kHoliday_Valentines
	&g_Holiday_MeetThePyro,							// kHoliday_MeetThePyro
	&g_Holiday_FullMoon,							// kHoliday_FullMoon
	&g_Holiday_HalloweenOrFullMoon,					// kHoliday_HalloweenOrFullMoon
	&g_Holiday_HalloweenOrFullMoonOrValentines,		// kHoliday_HalloweenOrFullMoonOrValentines
	&g_Holiday_AprilFools,							// kHoliday_AprilFools
	&g_Holiday_LoveAndWar,							// kHoliday_BreadUpdate
	&g_Holiday_SoldierMemorial,						// kHoliday_SoldierMemorial
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_HolidayChecks ) == kHolidayCount );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool EconHolidays_IsHolidayActive( int eHoliday, CRTime const &timeCurrent )
{
	if ( eHoliday < 0 || eHoliday >= kHolidayCount )
		return false;

	if ( !s_HolidayChecks[eHoliday] )
		return false;

	return s_HolidayChecks[eHoliday]->IsActive( timeCurrent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RTime32 EconHolidays_TerribleHack_GetHalloweenEndData( void )
{
	return g_Holiday_Halloween.GetEndTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int EconHolidays_GetHolidayForString( char const *pszHolidayName )
{
	for ( int iHoliday = 0; iHoliday < kHolidayCount; iHoliday++ )
	{
		if ( !s_HolidayChecks[iHoliday] )
			continue;
		if ( FStrEq( pszHolidayName, s_HolidayChecks[iHoliday]->GetHolidayName() ) )
			return iHoliday;
	}

	return kHoliday_None;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *EconHolidays_GetActiveHolidayString( void )
{
	CRTime timeCurrent;
	timeCurrent.SetGMT( true );

	for ( int iHoliday = 0; iHoliday < kHolidayCount; iHoliday++ )
	{
		if ( EconHolidays_IsHolidayActive( iHoliday, timeCurrent ) )
		{
			return s_HolidayChecks[iHoliday]->GetHolidayName();
		}
	}

	return nullptr;
}
