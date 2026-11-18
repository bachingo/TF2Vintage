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
// Purpose: Full moon detection. Active for m_fBonusTimeInDays (0.575 = 13.8 hours,
//			which is 7 degrees of lunar motion at its average speed) either side of
//			the exact instant of full moon, giving the same fixed ~27.6 hour window
//			every month, like the original game.
//
//			The instant of full moon is when the Moon's ecliptic longitude is exactly
//			180 degrees from the Sun's. We compute both longitudes for any moment
//			(Moon: Meeus, "Astronomical Algorithms" ch. 47, 59-term series; Sun:
//			VSOP87D Earth series cut to its 20 largest terms), then home in on the
//			nearest opposition by repeated correction.
//			There is no epoch and no month counting, so nothing can drift.
//			Full moon instant is typically within ~7 seconds (worst ~30 s) of JPL DE421;
//			the Moon series is the limit. Leap seconds are handled for any date from 1970
//			on. Only uses <cmath> and the existing CRTime.
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
		static const double fSynodicDays = 29.530588861;
		static const double fMeanDegreesPerDay = 360.0 / fSynodicDays;   // average Moon-from-Sun speed

		const double fNow = (double)timeCurrent.GetRTime32();

		// Find the exact instant of the full moon nearest to now. Start from now, see how
		// far the Moon is from opposition, and step by that angle at the average speed.
		// The real speed is within ~17% of average, so each pass shrinks the error ~6x;
		// 6 passes lands within a couple of seconds.
		double fFullMoon = fNow;
		for ( int i = 0; i < 6; i++ )
		{
			const double fDegreesFromFull = Wrap180( MoonSunElongationDegrees( fFullMoon ) - 180.0 );
			fFullMoon -= ( fDegreesFromFull / fMeanDegreesPerDay ) * 86400.0;
		}

		return fabs( fNow - fFullMoon ) <= (double)m_fBonusTimeInDays * 86400.0;
	}

private:
	static double Wrap360( double fDegrees )
	{
		fDegrees = fmod( fDegrees, 360.0 );
		return ( fDegrees < 0.0 ) ? fDegrees + 360.0 : fDegrees;
	}

	// Wrap an angle to -180..180 degrees
	static double Wrap180( double fDegrees )
	{
		fDegrees = fmod( fDegrees, 360.0 );
		if ( fDegrees > 180.0 ) fDegrees -= 360.0;
		else if ( fDegrees < -180.0 ) fDegrees += 360.0;
		return fDegrees;
	}

	// Terrestrial Time minus UTC, in seconds, for a UTC unix time. The astronomy below runs
	// on Terrestrial Time, which is steady, while UTC is stepped by leap seconds:
	// TT - UTC = 32.184 + (TAI - UTC). One second of error here is 0.5 arcsec of Moon.
	static double TerrestrialMinusUTCSeconds( double fUnixTime )
	{
		// Unix time each leap second took effect, and TAI - UTC in whole seconds from then on.
		// IERS announced no leap second for end of December 2026, and UTC is scheduled to stop
		// using them in 2027, so 37 is expected to be final. If one is ever added, add a row.
		static const int s_LeapSeconds[28][2] = {
			{   63072000, 10 }, {   78796800, 11 }, {   94694400, 12 },
			{  126230400, 13 }, {  157766400, 14 }, {  189302400, 15 },
			{  220924800, 16 }, {  252460800, 17 }, {  283996800, 18 },
			{  315532800, 19 }, {  362793600, 20 }, {  394329600, 21 },
			{  425865600, 22 }, {  489024000, 23 }, {  567993600, 24 },
			{  631152000, 25 }, {  662688000, 26 }, {  709948800, 27 },
			{  741484800, 28 }, {  773020800, 29 }, {  820454400, 30 },
			{  867715200, 31 }, {  915148800, 32 }, { 1136073600, 33 },
			{ 1230768000, 34 }, { 1341100800, 35 }, { 1435708800, 36 },
			{ 1483228800, 37 }
		};

		double fTAIMinusUTC;
		if ( fUnixTime < s_LeapSeconds[0][0] )
		{
			// Before 1972 UTC had no leap seconds; its rate was adjusted instead (8.0 s in 1970).
			fTAIMinusUTC = 8.000082 + ( fUnixTime / 86400.0 ) * 0.002592;
		}
		else
		{
			int i = 27;
			while ( i > 0 && fUnixTime < s_LeapSeconds[i][0] )
				i--;
			fTAIMinusUTC = (double)s_LeapSeconds[i][1];
		}

		return 32.184 + fTAIMinusUTC;
	}

	// Ecliptic longitude of the Moon minus that of the Sun, 0..360 degrees,
	// for a UTC unix time.
	static double MoonSunElongationDegrees( double fUnixTime )
	{
		static const double fDegToRad    = M_PI / 180.0;
		static const double fMoonLightTime = 1.28;   // seconds; we see the Moon where it was this long ago

		// Julian centuries since J2000.0 in Terrestrial Time
		const double fJDE = fUnixTime / 86400.0 + 2440587.5 + TerrestrialMinusUTCSeconds( fUnixTime ) / 86400.0;
		const double T    = ( fJDE - 2451545.0 ) / 36525.0;
		const double Tm   = T - ( fMoonLightTime / 86400.0 ) / 36525.0;

		//---- Moon (Meeus ch. 47) ----
		const double T2 = Tm * Tm, T3 = T2 * Tm, T4 = T3 * Tm;
		const double Lp = Wrap360( 218.3164477 + 481267.88123421 * Tm - 0.0015786 * T2 + T3 / 538841.0 - T4 / 65194000.0 );
		const double D  = Wrap360( 297.8501921 + 445267.1114034  * Tm - 0.0018819 * T2 + T3 / 545868.0 - T4 / 113065000.0 );
		const double M  = Wrap360( 357.5291092 +  35999.0502909  * Tm - 0.0001536 * T2 + T3 / 24490000.0 );
		const double Mp = Wrap360( 134.9633964 + 477198.8675055  * Tm + 0.0087414 * T2 + T3 / 69699.0 - T4 / 14712000.0 );
		const double F  = Wrap360(  93.2720950 + 483202.0175233  * Tm - 0.0036539 * T2 - T3 / 3526000.0 + T4 / 863310000.0 );
		const double E  = 1.0 - 0.002516 * Tm - 0.0000074 * T2;   // Earth orbit eccentricity factor

		// Multiples of D, M, M', F and the longitude coefficient in 0.000001 degrees
		static const int s_Terms[59][5] = {
			{ 0, 0, 1, 0,  6288774}, { 2, 0,-1, 0,  1274027}, { 2, 0, 0, 0,   658314}, { 0, 0, 2, 0,   213618},
			{ 0, 1, 0, 0,  -185116}, { 0, 0, 0, 2,  -114332}, { 2, 0,-2, 0,    58793}, { 2,-1,-1, 0,    57066},
			{ 2, 0, 1, 0,    53322}, { 2,-1, 0, 0,    45758}, { 0, 1,-1, 0,   -40923}, { 1, 0, 0, 0,   -34720},
			{ 0, 1, 1, 0,   -30383}, { 2, 0, 0,-2,    15327}, { 0, 0, 1, 2,   -12528}, { 0, 0, 1,-2,    10980},
			{ 4, 0,-1, 0,    10675}, { 0, 0, 3, 0,    10034}, { 4, 0,-2, 0,     8548}, { 2, 1,-1, 0,    -7888},
			{ 2, 1, 0, 0,    -6766}, { 1, 0,-1, 0,    -5163}, { 1, 1, 0, 0,     4987}, { 2,-1, 1, 0,     4036},
			{ 2, 0, 2, 0,     3994}, { 4, 0, 0, 0,     3861}, { 2, 0,-3, 0,     3665}, { 0, 1,-2, 0,    -2689},
			{ 2, 0,-1, 2,    -2602}, { 2,-1,-2, 0,     2390}, { 1, 0, 1, 0,    -2348}, { 2,-2, 0, 0,     2236},
			{ 0, 1, 2, 0,    -2120}, { 0, 2, 0, 0,    -2069}, { 2,-2,-1, 0,     2048}, { 2, 0, 1,-2,    -1773},
			{ 2, 0, 0, 2,    -1595}, { 4,-1,-1, 0,     1215}, { 0, 0, 2, 2,    -1110}, { 3, 0,-1, 0,     -892},
			{ 2, 1, 1, 0,     -810}, { 4,-1,-2, 0,      759}, { 0, 2,-1, 0,     -713}, { 2, 2,-1, 0,     -700},
			{ 2, 1,-2, 0,      691}, { 2,-1, 0,-2,      596}, { 4, 0, 1, 0,      549}, { 0, 0, 4, 0,      537},
			{ 4,-1, 0, 0,      520}, { 1, 0,-2, 0,     -487}, { 2, 1, 0,-2,     -399}, { 0, 0, 2,-2,     -381},
			{ 1, 1, 1, 0,      351}, { 3, 0,-2, 0,     -340}, { 4, 0,-3, 0,      330}, { 2,-1, 2, 0,      327},
			{ 0, 2, 1, 0,     -323}, { 1, 1,-1, 0,      299}, { 2, 0, 3, 0,      294} };

		double fSigmaL = 0.0;
		for ( int i = 0; i < 59; i++ )
		{
			const double fArg = ( s_Terms[i][0] * D + s_Terms[i][1] * M + s_Terms[i][2] * Mp + s_Terms[i][3] * F ) * fDegToRad;
			double fCoef = (double)s_Terms[i][4];
			const int nM = ( s_Terms[i][1] < 0 ) ? -s_Terms[i][1] : s_Terms[i][1];
			if ( nM == 1 ) fCoef *= E;
			else if ( nM == 2 ) fCoef *= E * E;
			fSigmaL += fCoef * sin( fArg );
		}

		// Venus, Jupiter and flattening of the Earth
		const double A1 = Wrap360( 119.75 + 131.849 * Tm );
		const double A2 = Wrap360(  53.09 + 479264.290 * Tm );
		fSigmaL += 3958.0 * sin( A1 * fDegToRad ) + 1962.0 * sin( ( Lp - F ) * fDegToRad ) + 318.0 * sin( A2 * fDegToRad );

		const double fMoonLongitude = Lp + fSigmaL / 1000000.0;

		//---- Sun (VSOP87D, heliocentric longitude of the Earth, 20 largest terms) ----
		// L = sum of A * cos( B + C * tau ), times tau for the k = 1 rows.
		// tau = Julian millennia from J2000.0. L is in radians.
		const double tau = T / 10.0;
		static const double s_Earth[20][4] = {
		//	  k        A            B           C
			{ 0,  1.753470457, 0.0000000,     0.000000 },
			{ 0,  0.033416565, 4.6692568,  6283.075850 },
			{ 0,  0.000348943, 4.6261024, 12566.151700 },
			{ 0,  0.000034971, 2.7441180,  5753.384885 },
			{ 0,  0.000034176, 2.8288658,     3.523118 },
			{ 0,  0.000031359, 3.6276704, 77713.771468 },
			{ 0,  0.000026762, 4.4180835,  7860.419392 },
			{ 0,  0.000023427, 6.1351624,  3930.209696 },
			{ 0,  0.000013243, 0.7424636, 11506.769770 },
			{ 0,  0.000012732, 2.0370966,   529.690965 },
			{ 0,  0.000011992, 1.1096294,  1577.343542 },
			{ 0,  0.000009903, 5.2326813,  5884.926847 },
			{ 0,  0.000009019, 2.0450544,    26.298320 },
			{ 0,  0.000008572, 3.5084916,   398.149003 },
			{ 0,  0.000007798, 1.1788265,  5223.693920 },
			{ 0,  0.000007531, 2.5333905,  5507.553239 },
			{ 0,  0.000005053, 4.5829256, 18849.227550 },
			{ 0,  0.000004924, 4.2050664,   775.522611 },
			{ 1, 6283.319667475, 0.0000000,     0.000000 },
			{ 1,  0.002060589, 2.6782346,  6283.075850 }
		};

		double fEarthL = 0.0;
		for ( int i = 0; i < 20; i++ )
			fEarthL += s_Earth[i][1] * cos( s_Earth[i][2] + s_Earth[i][3] * tau ) * ( s_Earth[i][0] > 0.5 ? tau : 1.0 );

		// Earth-Sun distance in AU, only needed to size the aberration shift
		const double fSunDistance = 1.00013989
								  + 0.016707   * cos( 3.0984635 + 6283.07585 * tau )
								  + 0.00013956 * cos( 3.0552461 + 12566.1517 * tau );

		// The Sun is opposite the Earth as seen from the Sun. Apparent position is
		// shifted back by annual aberration (20.4955 arcsec / distance).
		const double fSunLongitude = fEarthL / fDegToRad + 180.0 - 0.0056932 / fSunDistance;

		// Nutation moves both longitudes equally, so it cancels in the difference.
		return Wrap360( fMoonLongitude - fSunLongitude );
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
