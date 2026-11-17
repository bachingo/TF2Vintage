//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "rtime.h"
#include "econ_holidays.h"

//-----------------------------------------------------------------------------
// Purpose: Interface that answers the simple question "on the passed-in time,
//			would this holiday be active?". Any caching of calculations is left
//			up to subclasses.
//-----------------------------------------------------------------------------
class IIsHolidayActive
{
public:
	IIsHolidayActive( const char *pszHolidayName ) : m_pszHolidayName( pszHolidayName ) { }
	virtual ~IIsHolidayActive ( ) { }
	virtual bool IsActive( const CRTime& timeCurrent ) = 0;

	const char *GetHolidayName() const { return m_pszHolidayName; }

private:
	const char *m_pszHolidayName;
};

//-----------------------------------------------------------------------------
// Purpose: Always-disabled. Dummy event needed to map to slot zero for "disabled
//			holiday".
//-----------------------------------------------------------------------------
class CNoHoliday : public IIsHolidayActive
{
public:
	CNoHoliday() : IIsHolidayActive( "none" ) { }

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: A holiday that lasts exactly one and only one day.
//-----------------------------------------------------------------------------
class CSingleDayHoliday : public IIsHolidayActive
{
public:
	CSingleDayHoliday( const char *pszName, int iMonth, int iDay )
		: IIsHolidayActive( pszName )
		, m_iMonth( iMonth )
		, m_iDay( iDay )
	{
		//
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return m_iMonth == timeCurrent.GetMonth()
			&& m_iDay == timeCurrent.GetDayOfMonth();
	}

private:
	int m_iMonth;
	int m_iDay;
};

//-----------------------------------------------------------------------------
// Purpose: We want "week long" holidays to encompass at least two weekends,
//			so that players get plenty of time interacting with the holiday
//			features.
//-----------------------------------------------------------------------------
class CWeeksBasedHoliday : public IIsHolidayActive
{
public:
	CWeeksBasedHoliday( const char *pszName, int iMonth, int iDay, int iExtraWeeks )
		: IIsHolidayActive( pszName )
		, m_iMonth( iMonth )
		, m_iDay( iDay )
		, m_iExtraWeeks( iExtraWeeks )
		, m_iCachedCalculatedYear( 0 )
	{
		// We'll calculate the interval the first time we call IsActive().
	}

	void RecalculateTimeActiveInterval( int iYear )
	{
		// Get the date of the holiday.
		tm holiday_tm = { };
		holiday_tm.tm_mday = m_iDay;
		holiday_tm.tm_mon  = m_iMonth - 1;
		holiday_tm.tm_year = iYear - 1900; // convert to years since 1900
		mktime( &holiday_tm );

		// The event starts on the first Friday at least four days prior to the holiday.
		tm start_time_tm( holiday_tm );
		start_time_tm.tm_mday -= 4;							// Move back four days.
		mktime( &start_time_tm );
		int days_offset = start_time_tm.tm_wday - kFriday;	// Find the nearest prior Friday.
		if ( days_offset < 0 )
			days_offset += 7;
		start_time_tm.tm_mday -= days_offset;
		time_t start_time = mktime( &start_time_tm );

		// The event ends on the first Monday after the holiday, maybe plus some additional fudge
		// time.
		tm end_time_tm( holiday_tm );
		days_offset = 7 - (end_time_tm.tm_wday - kMonday);
		if ( days_offset >= 7 )
			days_offset -= 7;
		end_time_tm.tm_mday += days_offset + 7 * m_iExtraWeeks;
		time_t end_time = mktime( &end_time_tm );


		m_timeStart = start_time;
		m_timeEnd = end_time;

		// We're done and our interval data is cached.
		m_iCachedCalculatedYear = iYear;
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		const int iCurrentYear = timeCurrent.GetYear();
		if ( m_iCachedCalculatedYear != iCurrentYear )
			RecalculateTimeActiveInterval( iCurrentYear );

		return timeCurrent.GetRTime32() > m_timeStart
			&& timeCurrent.GetRTime32() < m_timeEnd;
	}

private:
	static const int kMonday = 1;
	static const int kFriday = 5;

	int m_iMonth;
	int m_iDay;
	int m_iExtraWeeks;
	
	// Filled out from RecalculateTimeActiveInterval().
	int m_iCachedCalculatedYear;

	RTime32 m_timeStart;
	RTime32 m_timeEnd;
};

//-----------------------------------------------------------------------------
// Purpose: A holiday that repeats on a certain time interval, like "every N days"
//			or "once every two months".
//-----------------------------------------------------------------------------
class CCyclicalHoliday : public IIsHolidayActive
{
public:
	CCyclicalHoliday( const char *pszName, int iMonth, int iDay, int iYear, float fCycleLengthInDays, float fBonusTimeInDays )
		: IIsHolidayActive( pszName )
		, m_fCycleLengthInDays( fCycleLengthInDays )
		, m_fBonusTimeInDays( fBonusTimeInDays )
	{
		// When is our initial interval?
		tm holiday_tm = { };
		holiday_tm.tm_mday = iDay;
		holiday_tm.tm_mon  = iMonth - 1;
		holiday_tm.tm_year = iYear - 1900; // convert to years since 1900
		m_timeInitial = mktime( &holiday_tm );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		// Days-to-seconds conversion.
		const int iSecondsPerDay = 24 * 60 * 60;

		// Convert our cycle/buffer times to seconds.
		const int iCycleLengthInSeconds = (int)(m_fCycleLengthInDays * iSecondsPerDay);
		const int iBufferTimeInSeconds  = (int)(m_fBonusTimeInDays * iSecondsPerDay);

		// How long has it been since we started this cycle?
		int iSecondsIntoCycle = (timeCurrent.GetRTime32() - m_timeInitial) % iCycleLengthInSeconds;

		// If we're within the buffer period right after the start of a cycle, we're active.
		if ( iSecondsIntoCycle < iBufferTimeInSeconds )
			return true;

		// If we're within the buffer period towards the end of a cycle, we're active.
		if ( iSecondsIntoCycle > iCycleLengthInSeconds - iBufferTimeInSeconds )
			return true;

		// Alas, normal mode for us.
		return false;
	}

private:
	time_t m_timeInitial ;

	float m_fCycleLengthInDays;
	float m_fBonusTimeInDays;
};

//-----------------------------------------------------------------------------
// Purpose: A hilariously overkill, way over the top method of finding full moons.
//			Zeroed from the June 2029 lunar eclipse (Syzygy)
//			Should have a 3s accuracy for the next 100 years.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Full moon detection. Active while the Moon is within 7 degrees of
//			opposition (the "opposition surge" window, craters shadowless).
//
//			Method: Meeus, "Astronomical Algorithms" ch. 49 (phases of the Moon).
//			Predicts the true instant of each full moon directly, rather than
//			counting mean months from one anchor. Good to about a minute for
//			thousands of years, so it needs no epoch and will not drift.
//			Only uses <cmath> (sin/cos/floor) and the existing CRTime.
//-----------------------------------------------------------------------------
class CCLunarHoliday : public IIsHolidayActive
{
public:
	CCLunarHoliday( const char *pszName, int iMonth, int iDay, int iYear, float fCycleLengthInDays, float fBonusTimeInDays )
		: IIsHolidayActive( pszName )
		, m_fCycleLengthInDays( fCycleLengthInDays )
		, m_fBonusTimeInDays( fBonusTimeInDays )
	{
		// When is our initial interval?
		tm holiday_tm = { };
		holiday_tm.tm_mday = iDay;
		holiday_tm.tm_mon  = iMonth - 1;
		holiday_tm.tm_year = iYear - 1900; // convert to years since 1900
		m_timeInitial = mktime( &holiday_tm );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		static const double fSynodicDays      = 29.530588861;   // mean new moon to new moon
		static const double fJDE_K0           = 2451550.09766;  // mean new moon nearest 2000-01-06 (k = 0)
		static const double fUnixEpochJD      = 2440587.5;      // Julian Date of 1970-01-01 00:00 UTC
		static const double fSurgeDegrees     = 7.0;            // phase angle where opposition surge begins

		const double fNow = (double)timeCurrent.GetRTime32();
		const double fJD  = fNow / 86400.0 + fUnixEpochJD;

		// Full moons are k = n + 0.5. Start at the one at/after the mean estimate
		// and also test its neighbours, since the true time can differ by ~14 hours.
		const double fKBase = floor( ( fJD - fJDE_K0 ) / fSynodicDays ) + 0.5;

		for ( int i = -1; i <= 1; i++ )
		{
			double fMoonAnomaly = 0.0;
			const double fFullMoonTime = FullMoonUnixTime( fKBase + i, fMoonAnomaly );

			// The Moon's apparent speed away from the Sun at full moon swings from
			// ~10.8 to ~14.2 deg/day depending on its distance (anomaly). Convert
			// the 7 degree threshold into a half-width in days using that speed.
			const double fDegreesPerDay = 12.47 + 1.68 * cos( fMoonAnomaly );
			const double fHalfWidthSeconds = ( fSurgeDegrees / fDegreesPerDay ) * 86400.0;

			if ( fabs( fNow - fFullMoonTime ) <= fHalfWidthSeconds )
				return true;
		}

		return false;
	}

private:
	// Returns the UTC unix time of the true full moon number k (k = n + 0.5),
	// and the Moon's mean anomaly at that moment (radians).
	static double FullMoonUnixTime( double k, double &fMoonAnomalyOut )
	{
		static const double fDegToRad = M_PI / 180.0;

		const double T  = k / 1236.85;
		const double T2 = T * T;

		// Mean full moon (Terrestrial Time, Julian Ephemeris Day)
		double fJDE = 2451550.09766 + 29.530588861 * k + 0.00015437 * T2;

		const double E  = 1.0 - 0.002516 * T - 0.0000074 * T2;  // Earth orbit eccentricity factor

		const double M  = fmod( 2.5534   + 29.10535670  * k, 360.0 ) * fDegToRad;  // Sun's mean anomaly
		const double Mp = fmod( 201.5643 + 385.81693528 * k + 0.0107582 * T2, 360.0 ) * fDegToRad; // Moon's mean anomaly
		const double F  = fmod( 160.7108 + 390.67050284 * k - 0.0016118 * T2, 360.0 ) * fDegToRad; // Moon's argument of latitude
		const double Om = fmod( 124.7746 - 1.56375588   * k, 360.0 ) * fDegToRad;  // Longitude of ascending node

		// Periodic corrections in days (Meeus Table 49.A, full moon column)
		double fCorr =
			  -0.40614 * sin( Mp )
			+  0.17302 * E * sin( M )
			+  0.01614 * sin( 2 * Mp )
			+  0.01043 * sin( 2 * F )
			+  0.00734 * E * sin( Mp - M )
			-  0.00515 * E * sin( Mp + M )
			+  0.00209 * E * E * sin( 2 * M )
			-  0.00111 * sin( Mp - 2 * F )
			-  0.00057 * sin( Mp + 2 * F )
			+  0.00056 * E * sin( 2 * Mp + M )
			-  0.00042 * sin( 3 * Mp )
			+  0.00042 * E * sin( M + 2 * F )
			+  0.00038 * E * sin( M - 2 * F )
			-  0.00024 * E * sin( 2 * Mp - M )
			-  0.00017 * sin( Om )
			-  0.00007 * sin( Mp + 2 * M )
			+  0.00004 * sin( 2 * Mp - 2 * F )
			+  0.00004 * sin( 3 * M )
			+  0.00003 * sin( Mp + M - 2 * F )
			+  0.00003 * sin( 2 * Mp + 2 * F )
			-  0.00003 * sin( Mp + M + 2 * F )
			+  0.00003 * sin( Mp - M + 2 * F )
			-  0.00002 * sin( Mp - M - 2 * F )
			-  0.00002 * sin( 3 * Mp + M );

		// Planetary arguments (Meeus: additional corrections, days)
		static const double s_A[14][2] = {
			{ 299.77,  0.107408 }, { 251.88,  0.016321 }, { 251.83, 26.651886 }, { 349.42, 36.412478 },
			{  84.66, 18.206239 }, { 141.74, 53.303771 }, { 207.14,  2.453732 }, { 154.84,  7.306860 },
			{  34.52, 27.261239 }, { 207.19,  0.121824 }, { 291.34,  1.844379 }, { 161.72, 24.198154 },
			{ 239.56, 25.513099 }, { 331.55,  3.592518 } };
		static const double s_Amp[14] = {
			0.000325, 0.000165, 0.000164, 0.000126, 0.000110, 0.000062, 0.000060,
			0.000056, 0.000047, 0.000042, 0.000040, 0.000037, 0.000035, 0.000023 };

		for ( int i = 0; i < 14; i++ )
		{
			double fArg = s_A[i][0] + s_A[i][1] * k;
			if ( i == 0 ) fArg -= 0.009173 * T2;
			fCorr += s_Amp[i] * sin( fmod( fArg, 360.0 ) * fDegToRad );
		}

		fJDE += fCorr;

		fMoonAnomalyOut = Mp;

		// Terrestrial Time -> UTC (Delta-T ~ 69 s, drifts well under a second a year),
		// then Julian Date -> unix seconds.
		static const double fDeltaTSeconds = 69.2;
		return ( fJDE - 2440587.5 ) * 86400.0 - fDeltaTSeconds;
	}

	time_t m_timeInitial;

	float m_fCycleLengthInDays;
	float m_fBonusTimeInDays;
};

//-----------------------------------------------------------------------------
// Purpose: A pseudo-holiday that is active when either of its child holidays
//			is active. Works through pointers but does not manage memory.
//-----------------------------------------------------------------------------
class COrHoliday : public IIsHolidayActive
{
public:
	COrHoliday( const char *pszName, IIsHolidayActive *pA, IIsHolidayActive *pB )
		: IIsHolidayActive( pszName )
		, m_pA( pA )
		, m_pB( pB )
	{
		Assert( pA );
		Assert( pB );
		Assert( pA != pB );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return m_pA->IsActive( timeCurrent )
			|| m_pB->IsActive( timeCurrent );
	}

private:
	IIsHolidayActive *m_pA;
	IIsHolidayActive *m_pB;
};

//-----------------------------------------------------------------------------
// Purpose: Holiday that is defined by a start and end date
//-----------------------------------------------------------------------------
class CDateBasedHoliday : public IIsHolidayActive
{
public:
	CDateBasedHoliday( const char *pszName, const char *pszStartTime, const char *pszEndTime )
		: IIsHolidayActive( pszName )
	{
		m_rtStartTime =	 CRTime::RTime32FromString( pszStartTime );
		m_rtEndTime = CRTime::RTime32FromString( pszEndTime );
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		return ( ( timeCurrent >= m_rtStartTime ) && ( timeCurrent <= m_rtEndTime ) );
	}

	RTime32 GetEndRTime() const
	{
		return m_rtEndTime.GetRTime32();
	}


private:
	CRTime m_rtStartTime;
	CRTime m_rtEndTime;
};

//-----------------------------------------------------------------------------
// Purpose: Holiday that is defined by a start and end date with no year specified
//-----------------------------------------------------------------------------
class CDateBasedHolidayNoSpecificYear : public IIsHolidayActive
{
public:
	CDateBasedHolidayNoSpecificYear( const char *pszName, const char *pszStartTime, const char *pszEndTime )
		: IIsHolidayActive( pszName )
		, m_pszStartTime( pszStartTime )
		, m_pszEndTime( pszEndTime )
		, m_iCachedYear( -1 )
	{
	}

	virtual bool IsActive( const CRTime& timeCurrent )
	{
		const int iYear = timeCurrent.GetYear();

		if ( iYear != m_iCachedYear )
		{
			char m_szStartTime[k_RTimeRenderBufferSize];
			char m_szEndTime[k_RTimeRenderBufferSize];

			V_sprintf_safe( m_szStartTime, "%d-%s", iYear, m_pszStartTime );
			V_sprintf_safe( m_szEndTime, "%d-%s", iYear, m_pszEndTime );

			m_iCachedYear = iYear;
			m_rtCachedStartTime = CRTime::RTime32FromString( m_szStartTime );
			m_rtCachedEndTime = CRTime::RTime32FromString( m_szEndTime );
		}

		return ( ( timeCurrent >= m_rtCachedStartTime ) && ( timeCurrent <= m_rtCachedEndTime ) );
	}

	RTime32 GetEndRTime() const
	{
		return m_rtCachedEndTime.GetRTime32();
	}

private:
	const char *m_pszStartTime;
	const char *m_pszEndTime;

	int m_iCachedYear;
	CRTime m_rtCachedStartTime;
	CRTime m_rtCachedEndTime;
};

//-----------------------------------------------------------------------------
// Purpose: Actual holiday implementation objects.
//-----------------------------------------------------------------------------

static CNoHoliday			g_Holiday_NoHoliday;

static CDateBasedHolidayNoSpecificYear	g_Holiday_TF2Birthday	( "birthday",	"08-23", "08-25" );

static CDateBasedHolidayNoSpecificYear	g_Holiday_Halloween		( "halloween",	"10-01", "11-08" );

static CDateBasedHolidayNoSpecificYear	g_Holiday_ChristmasPart1( "christmas1", "12-01", "12-31 23:59:59" );
static CDateBasedHolidayNoSpecificYear	g_Holiday_ChristmasPart2( "christmas2", "01-01", "01-08" );
static COrHoliday	g_Holiday_Christmas		( "christmas", &g_Holiday_ChristmasPart1, &g_Holiday_ChristmasPart2 );

static CDateBasedHolidayNoSpecificYear	g_Holiday_ValentinesDay	( "valentines",	"02-13", "02-15" );

static CDateBasedHoliday	g_Holiday_MeetThePyro				( "meet_the_pyro",	"2012-06-26", "2012-07-05" );
														   /*					starting date		cycle length in days	bonus time in days on both sides */
static CCLunarHoliday		g_Holiday_FullMoon					( "fullmoon",		10, 12, 2011,		29.53058885f,				0.575f );
																								 // TF2V: This is set for the first full moon before TF2's beta release, using the proper synodical moon calculation. Fun fact: This was a lunar eclipse!
static COrHoliday			g_Holiday_HalloweenOrFullMoon		( "halloween_or_fullmoon",	&g_Holiday_Halloween,	&g_Holiday_FullMoon );

static COrHoliday			g_Holiday_HalloweenOrFullMoonOrValentines	( "halloween_or_fullmoon_or_valentines",	&g_Holiday_HalloweenOrFullMoon,	&g_Holiday_ValentinesDay );

static CDateBasedHolidayNoSpecificYear	g_Holiday_AprilFools	( "april_fools",	"03-31", "04-02" );

static CDateBasedHoliday	g_Holiday_EndOfTheLine				( "eotl_launch",	"2014-12-03", "2015-01-05" );

static CDateBasedHoliday	g_Holiday_CommunityUpdate			( "community_update", "2015-09-01", "2015-11-05" );

static CDateBasedHolidayNoSpecificYear	g_Holiday_Soldier		( "soldier", "04-12", "04-14" );

// only setup for 2025 right now...need to figure out how we want future events to run and maybe remove the year
static CDateBasedHoliday	g_Holiday_Summer( "summer", "2025-07-16", "2025-09-16" );

// ORDER NEEDS TO MATCH enum EHoliday
static IIsHolidayActive *s_HolidayChecks[] =
{
	&g_Holiday_NoHoliday,							// kHoliday_None
	&g_Holiday_TF2Birthday,							// kHoliday_TFBirthday
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
	&g_Holiday_Soldier,								// kHoliday_Soldier
	&g_Holiday_Summer,								// kHoliday_Summer
};

COMPILE_TIME_ASSERT( ARRAYSIZE( s_HolidayChecks ) == kHolidayCount );

#include "tf_gamerules.h"
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool EconHolidays_IsHolidayActive( int iHolidayIndex, const CRTime& timeCurrent )
{
	if ( iHolidayIndex < 0 || iHolidayIndex >= kHolidayCount )
		return false;

	Assert( s_HolidayChecks[iHolidayIndex] );
	if ( !s_HolidayChecks[iHolidayIndex] )
		return false;
	
	uint32_t timeHolidayTest;
	
	if ( TFGameRules() && TFGameRules()->GetTF2VEra() )
	{
		// We check this twice: once with the Era, the second below with the timestamp.
		int nCurrentEra = TFGameRules()->GetTF2VEra();
		// We already have the IsAnachronistic, IsContemporary, and IsBetween, but this saves us some time checking.
		
		
		// TF2V: Prevent cyclic holidays firing off earlier than they were added.

		// TF2 Birthday first appeared on Aug 24, 2009
		if (iHolidayIndex == kHoliday_TFBirthday && nCurrentEra < 708)
			return false;
		
		// Halloween first appeared Oct 29, 2009
		if (iHolidayIndex == kHoliday_Halloween && nCurrentEra < TF2V_DAY_HALLOWEEN_2009)
			return false;
		
		// Christmas first appeared Dec 17, 2010 (Australian Christmas 2010)
		if (iHolidayIndex == kHoliday_Christmas && nCurrentEra < TF2V_DAY_SMISSMAS_2010)
			return false;
		
		// Full Moon first appeared Oct 27, 2011 (Halloween 2011)
		if (iHolidayIndex == kHoliday_FullMoon && nCurrentEra < TF2V_DAY_HALLOWEEN_2011)
			return false;
		
		// Valentine's first appeared Feb 14, 2012
		if (iHolidayIndex == kHoliday_Valentines && nCurrentEra < 1608)
			return false;
		
		// Meet the Pyro is explicitly 2012 (Pyromania Update)
		if (iHolidayIndex == kHoliday_MeetThePyro && nCurrentEra < TF2V_DAY_MAJOR_PYROMANIA)
			return false;
		
		// April Fool's first appeared Apr 1, 2014
		if (iHolidayIndex == kHoliday_AprilFools && nCurrentEra < 2389)
			return false;
		
		// End of the Line is explicitly 2014
		if (iHolidayIndex == kHoliday_EOTL && nCurrentEra < TF2V_DAY_MAJOR_END_OF_THE_LINE)
			return false;
		
		// Soldier holiday (Rick May tribute) first appeared Apr 12, 2020
		if (iHolidayIndex == kHoliday_Soldier && nCurrentEra < 4609)
			return false;
		
		// Summer events first appeared Jul 12, 2023
		if (iHolidayIndex == kHoliday_Summer && nCurrentEra < TF2V_DAY_SUMMER_2023)
			return false;
			
		// Special case: Halloween events before they were hardcoded in 2019
		// These have specific start/end dates each year
		if ( iHolidayIndex == kHoliday_Halloween )
		{
			// Manually check the dates for Halloween by year prior to the hardcoding in 2019.
			// Each event lasts approximately 13-14 days
			if ( 
			((nCurrentEra >= 774 ) && (nCurrentEra < 788 ))  || // 2009 (Oct 29 - Nov 11)
			((nCurrentEra >= 1137) && (nCurrentEra < 1150)) || // 2010 (Oct 27 - Nov 9)
			((nCurrentEra >= 1502) && (nCurrentEra < 1516)) || // 2011 (Oct 27 - Nov 10)
			((nCurrentEra >= 1867) && (nCurrentEra < 1881)) || // 2012 (Oct 26 - Nov 9)
			((nCurrentEra >= 2235) && (nCurrentEra < 2249)) || // 2013 (Oct 29 - Nov 12)
			((nCurrentEra >= 2600) && (nCurrentEra < 2615)) || // 2014 (Oct 29 - Nov 13)
			((nCurrentEra >= 2964) && (nCurrentEra < 2980)) || // 2015 (Oct 28 - Nov 13)
			((nCurrentEra >= 3323) && (nCurrentEra < 3352)) || // 2016 (Oct 21 - Nov 19)
			((nCurrentEra >= 3687) && (nCurrentEra < 3701)) || // 2017 (Oct 20 - Nov 3)
			((nCurrentEra >= 4051) && (nCurrentEra < 4077)) )  // 2018 (Oct 19 - Nov 14)
			// Consistent with rules from 2019-2025
			((nCurrentEra >= 6954) && (nCurrentEra < 6985)) || // 2026 (Oct 1 - Nov 1)
			{
				return true;
			}
		}
		else if ( iHolidayIndex == kHoliday_FullMoon ) 
		{
			// Strange instance where Full Moon was active for a week straight in September 2014.
			// Sept 17-24, 2014 = Days 2558-2565
			if ((nCurrentEra >= 2558) && (nCurrentEra < 2566))
			{
				return true;
			}
		}
		else if ( iHolidayIndex == kHoliday_Summer )
		{
			// Summer events with specific date ranges
			// Each event lasts from mid-July to mid-September
			if (
			((nCurrentEra >= 5778) && (nCurrentEra < 5843)) || // 2023 (Jul 12 - Sep 15)
			((nCurrentEra >= 6142) && (nCurrentEra < 6209)) || // 2024 (Jul 10 - Sep 15)
			((nCurrentEra >= 6510) && (nCurrentEra < 6573)) )  // 2025 (Jul 16 - Sep 17)
			((nCurrentEra >= 6868) && (nCurrentEra < 6938)) )  // 2026 (Jul 17 - Sep 15)
			{
				return true;
			}
		}
		
		
		// Make our own faked current time based off the day TF2V is set as.
		// Since the Era function is saved as days from 09/16/2007, we simply replace our own YY/MM/DD with it.
		
		const uint32_t EPOCH_OFFSET = 1189900800; 
		const uint32_t SECONDS_PER_DAY = 86400;
			
		// 2. Get the current system time to extract HH:MM
		time_t now = time(0);
		struct tm *now_tm = gmtime(&now);

			// 3. Calculate seconds contributed by today's HH:MM:SS
		uint32_t secondsToday = (now_tm->tm_hour * 3600) + 
								(now_tm->tm_min * 60) + 
								 now_tm->tm_sec;
									 
		// 4. Combine: Offset + (Custom Days * 86400) + Current Time
		timeHolidayTest = EPOCH_OFFSET + (TFGameRules()->GetTF2VEra() * SECONDS_PER_DAY) + secondsToday;
	}
	else	// Just grab today's current time. Boring.
		timeHolidayTest = timeCurrent.GetRTime32();
	
	// TF2V: Prevent cyclic holidays firing off earlier than they were added.
	// Kind of gross because we use harcoded seconds here for the comparison.
	if (iHolidayIndex == kHoliday_TFBirthday && timeHolidayTest < 1250985600) // Birthday wasn't introduced until 2009
		return false;
	if (iHolidayIndex == kHoliday_Halloween && timeHolidayTest < 1256774400) // Halloween wasn't introduced until 2009
		return false;
	if (iHolidayIndex == kHoliday_Christmas && timeHolidayTest < 1292544000) // Christmas wasn't introduced until 2010
		return false;
	if (iHolidayIndex == kHoliday_FullMoon && timeHolidayTest < 1319673600)  // Full Moon wasn't introduced until 2011
		return false;
	if (iHolidayIndex == kHoliday_Valentines && timeHolidayTest < 1329091200)  // Valentine's wasn't introduced until 2012
		return false;
	if (iHolidayIndex == kHoliday_MeetThePyro && timeHolidayTest < 1340668800)  // Pyromania is explicitly 2012
		return false;
	if (iHolidayIndex == kHoliday_AprilFools && timeHolidayTest < 1396310400) // April Fool's wasn't introduced until 2014
		return false;
	if (iHolidayIndex == kHoliday_EOTL && timeHolidayTest < 1417564800) 		// End of the Line is explicitly 2014
		return false;
	if (iHolidayIndex == kHoliday_Soldier && timeHolidayTest < 1586649600) // Rick May's still alive! (Before April 2020, at least.)
		return false;
	if (iHolidayIndex == kHoliday_Summer && timeHolidayTest < 1689120000) // Summer wasn't introduced until 2023
		return false;
		
	// We're officially going from "Kind of gross" to "Extremely gross" now.
	if ( iHolidayIndex == kHoliday_Halloween )
	{
		// Manually check the dates for Halloween by year prior to the hardcoding in 2019.
		if ( 
		(timeHolidayTest >= 1256774400 && timeHolidayTest <= 1257811199) || // 2009
        (timeHolidayTest >= 1288137600 && timeHolidayTest <= 1289260799) || // 2010
        (timeHolidayTest >= 1319673600 && timeHolidayTest <= 1320623999) || // 2011
        (timeHolidayTest >= 1351209600 && timeHolidayTest <= 1352419199) || // 2012
        (timeHolidayTest >= 1383004800 && timeHolidayTest <= 1384214399) || // 2013
        (timeHolidayTest >= 1414540800 && timeHolidayTest <= 1415836799) || // 2014
        (timeHolidayTest >= 1445990400 && timeHolidayTest <= 1447372799) || // 2015
        (timeHolidayTest >= 1477008000 && timeHolidayTest <= 1479340799) || // 2016
        (timeHolidayTest >= 1508976000 && timeHolidayTest <= 1510185599) || // 2017
        (timeHolidayTest >= 1539907200 && timeHolidayTest <= 1542239999) )  // 2018
		{
			return true;
		}
	}
	else if ( iHolidayIndex == kHoliday_FullMoon ) 
	{
		// Strange instance where Full Moon was active for a week straight in September 2014.
		if (timeHolidayTest >= 1410912000 && timeHolidayTest <= 1411516800)
		{
			return true;
		}
	}
	else if ( iHolidayIndex == kHoliday_Summer )
	{
		// Do the same thing for summer. As it ends September 15 constantly, hopefully they'll lock this logic.
		if (
		(timeHolidayTest >= 1689120000 && timeHolidayTest <= 1694822399) || // 2023
        (timeHolidayTest >= 1721260800 && timeHolidayTest <= 1726444799) || // 2024
        (timeHolidayTest >= 1753315200 && timeHolidayTest <= 1757980799) )  // 2025
		{
			return true;
		}
	}
	

	return s_HolidayChecks[iHolidayIndex]->IsActive( timeHolidayTest );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	EconHolidays_GetHolidayForString( const char* pszHolidayName )
{
	for ( int iHoliday = 0; iHoliday < kHolidayCount; ++iHoliday )
	{
		Assert( s_HolidayChecks[iHoliday] );
		if ( s_HolidayChecks[iHoliday] &&
			 0 == Q_stricmp( pszHolidayName, s_HolidayChecks[iHoliday]->GetHolidayName() ) )
		{
			return iHoliday;
		}
	}

	return kHoliday_None;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *EconHolidays_GetActiveHolidayString()
{
	CRTime timeNow;
	timeNow.SetToCurrentTime();
	timeNow.SetToGMT( true );

	for ( int iHoliday = 0; iHoliday < kHolidayCount; iHoliday++ )
	{
		if ( EconHolidays_IsHolidayActive( iHoliday, timeNow ) )
		{
			Assert( s_HolidayChecks[iHoliday] );
			return s_HolidayChecks[iHoliday]->GetHolidayName();
		}
	}

	// No holidays currently active.
	return NULL;
}

#if defined(TF_CLIENT_DLL) || defined(TF_DLL) || defined(TF_GC_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
RTime32 EconHolidays_TerribleHack_GetHalloweenEndData()
{
	return g_Holiday_Halloween.GetEndRTime();
}
#endif // defined(TF_CLIENT_DLL) || defined(TF_DLL) || defined(TF_GC_DLL)
