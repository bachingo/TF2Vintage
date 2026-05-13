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
		// --- Static Constants (Scientific Calibration Data) ---
		
		// Anchor: Total Lunar Eclipse of June 26, 2029 (Peak Opposition).
		// This serves as the 'Day 0' for the 7th-order calculation.
		static const uint32 iEclipseEpochUTC = 1877138531;
		
		// Lunar Distance / Speed of Light: Time (in seconds) for light to travel 
		// from the Moon to Earth. Used to sync visual state with physical position.
		static const double fLightTimeSeconds = 1.282;
		
		// Solar Day Refinement: Accounts for the slight drift in the length 
		// of a mean solar day over long historical periods.
		static const float  flSecondsPerDay = 86400.002f;
		
		static const double fTwoPi = 2.0 * M_PI;
		
		// --- ORBITAL PERIODS (Mean Values used for Fundamental Arguments) ---
		static const double fSynodicMonth = 2551442.890;   // New Moon to New Moon (Phase Cycle)
		static const double fAnomalisticMonth = 2380713.12; // Perigee to Perigee (Distance Cycle)
		static const double fDraconicMonth = 2351135.0;     // Node to Node (Ecliptic Crossing)
	
		// --- LIGHT-TIME COMPENSATION ---
		// We calculate the moon's position at (T - 1.282s) because that is 
		// the lunar state currently visible to an observer on Earth's surface.
		const double fElapsedSeconds = ((double)timeCurrent.GetRTime32() - (double)iEclipseEpochUTC) - fLightTimeSeconds;
	
		// 1. Fundamental Arguments (Radians)
		// These represent the mean angular positions of the Moon and Sun.
		// Added phase offsets to align mean positions with the 2029 Epoch.
		const double D       = fmod(fElapsedSeconds + (2.52 * 86400.0), fSynodicMonth) / fSynodicMonth * fTwoPi; 
		const double M       = fmod(fElapsedSeconds + (0.11 * 86400.0), fAnomalisticMonth) / fAnomalisticMonth * fTwoPi; 
		const double M_prime = fmod(2.1 + (0.01720209895 * (fElapsedSeconds / (double)flSecondsPerDay)), fTwoPi); 
		const double F       = fmod(fElapsedSeconds + (1.85 * 86400.0), fDraconicMonth) / fDraconicMonth * fTwoPi;
	
		// 2. 7th-Order Correction ("Wobble")
		// This accounts for gravitational perturbations, primarily from the Sun and Earth's 
		// oblateness, which cause the Moon to speed up and slow down in its orbit.
		// Coefficients represent the amplitude (in days) of gravitational perturbations.
		const double fWobble = 
		// [ELLIPTICAL] Primary orbit correction (Moon speed changing via distance)
		  0.47119 * sin(M)                    // Equation of Center

		// [SOLAR] The Sun's pull changing the shape and timing of the orbit
		+ 0.16512 * sin(2 * D - M)            // Evection (Solar pull on eccentricity)
		+ 0.02106 * sin(2 * D)                // Variation (Gravitational flux)
		- 0.22513 * sin(M_prime)              // Annual Equation (Earth-Sun distance)
		
		// [PARALLACTIC] Corrections for the Sun's finite distance (not infinite)
		- 0.03504 * sin(D)                    // Parallactic Equation (General Sun proximity)
		+ 0.00032 * sin(D - M)                // Parallactic Inequality (Elliptical Sun proximity)
		
		// [PLANETARY] Tugs from other celestial bodies
		+ 0.00702 * sin(2 * D + M)            // Venus/Jupiter resonance
		+ 0.00401 * sin(2 * D - 2 * M_prime)  // Jupiter's long-term perturbation
		
		// [GEODETIC] Physical constraints of the Earth/Moon alignment
		+ 0.00063 * sin(2 * F)                // Reduction to Ecliptic (Orbit tilt correction)
		- 0.01140 * sin(F);                   // Nodal Precession (Earth's equatorial bulge / J2 effect)
	
		// 3. Corrected Cycle Position
		// Translating the mean time into 'Actual' time by applying the gravitational wobble.
		double fCurrentCycleSeconds = fmod(fElapsedSeconds, fSynodicMonth);
		if (fCurrentCycleSeconds < 0) fCurrentCycleSeconds += fSynodicMonth;
	
		double fCorrectedSeconds = fmod(fCurrentCycleSeconds - (fWobble * (double)flSecondsPerDay), fSynodicMonth);
		if (fCorrectedSeconds < 0) fCorrectedSeconds += fSynodicMonth;
	
		// 4. Dynamic Phase Angle
		// Convert corrected seconds into a degree-based position (0 to 360).
		// 0.0 degrees represents the absolute center of the Full Moon peak.
		double fCorrectedDegrees = (fCorrectedSeconds / fSynodicMonth) * 360.0;
	
		// Calculate the shortest angular distance from the Peak (handles the 360/0 wrap).
		double fAngleFromPeak = fCorrectedDegrees;
		if (fAngleFromPeak > 180.0) fAngleFromPeak = 360.0 - fAngleFromPeak;
	
		// Opposition Surge Threshold:
		// 7.0 degrees phase angle defines the beginning of "Opposition Surge": the period where 
		// lunar craters cast no visible shadows and the moon appears to glow.
		static const float fOppositionSurgeThreshold = 7.0f;
	
		return (fAngleFromPeak <= fOppositionSurgeThreshold);
	}

private:
	time_t m_timeInitial ;

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
static CCLunarHoliday		g_Holiday_FullMoon					( "fullmoon",		8, 28, 2007,		29.53058885f,				0.575f );
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
		if (iHolidayIndex == kHoliday_TFBirthday && nCurrentEra < 708))
			return false;
		
		// Halloween first appeared Oct 29, 2009
		if (iHolidayIndex == kHoliday_Halloween && nCurrentEra < TF2V_ERA_DAY_HALLOWEEN_2009))
			return false;
		
		// Christmas first appeared Dec 17, 2010 (Australian Christmas 2010)
		if (iHolidayIndex == kHoliday_Christmas && nCurrentEra < TF2V_DAY_SMISSMAS_2010))
			return false;
		
		// Full Moon first appeared Oct 27, 2011 (Halloween 2011)
		if (iHolidayIndex == kHoliday_FullMoon && nCurrentEra < TF2V_ERA_DAY_HALLOWEEN_2011))
			return false;
		
		// Valentine's first appeared Feb 14, 2012
		if (iHolidayIndex == kHoliday_Valentines && nCurrentEra < 1608))
			return false;
		
		// Meet the Pyro is explicitly 2012 (Pyromania Update)
		if (iHolidayIndex == kHoliday_MeetThePyro && nCurrentEra < TF2V_ERA_DAY_PYROMANIA))
			return false;
		
		// April Fool's first appeared Apr 1, 2014
		if (iHolidayIndex == kHoliday_AprilFools && nCurrentEra < 2389))
			return false;
		
		// End of the Line is explicitly 2014
		if (iHolidayIndex == kHoliday_EOTL && nCurrentEra < TF2V_DAY_MAJOR_END_OF_THE_LINE))
			return false;
		
		// Soldier holiday (Rick May tribute) first appeared Apr 12, 2020
		if (iHolidayIndex == kHoliday_Soldier && nCurrentEra < 4609))
			return false;
		
		// Summer events first appeared Jul 12, 2023
		if (iHolidayIndex == kHoliday_Summer && nCurrentEra < TF2V_DAY_SUMMER_2023))
			return false;
			
		// Special case: Halloween events before they were hardcoded in 2019
		// These have specific start/end dates each year
		if ( iHolidayIndex == kHoliday_Halloween )
		{
			// Manually check the dates for Halloween by year prior to the hardcoding in 2019.
			// Each event lasts approximately 13-14 days
			if ( 
			(nCurrentEra >= 774)  && nCurrentEra < 788))  || // 2009 (Oct 29 - Nov 11)
			(nCurrentEra >= 1137) && nCurrentEra < 1150)) || // 2010 (Oct 27 - Nov 9)
			(nCurrentEra >= 1502) && nCurrentEra < 1516)) || // 2011 (Oct 27 - Nov 10)
			(nCurrentEra >= 1867) && nCurrentEra < 1881)) || // 2012 (Oct 26 - Nov 9)
			(nCurrentEra >= 2235) && nCurrentEra < 2249)) || // 2013 (Oct 29 - Nov 12)
			(nCurrentEra >= 2600) && nCurrentEra < 2615)) || // 2014 (Oct 29 - Nov 13)
			(nCurrentEra >= 2964) && nCurrentEra < 2980)) || // 2015 (Oct 28 - Nov 13)
			(nCurrentEra >= 3323) && nCurrentEra < 3352)) || // 2016 (Oct 21 - Nov 19)
			(nCurrentEra >= 3687) && nCurrentEra < 3701)) || // 2017 (Oct 20 - Nov 3)
			(nCurrentEra >= 4051) && nCurrentEra < 4077)) )  // 2018 (Oct 19 - Nov 14)
			{
				return true;
			}
		}
		else if ( iHolidayIndex == kHoliday_FullMoon ) 
		{
			// Strange instance where Full Moon was active for a week straight in September 2014.
			// Sept 17-24, 2014 = Days 2558-2565
			if (nCurrentEra >= 2558) && nCurrentEra < 2566))
			{
				return true;
			}
		}
		else if ( iHolidayIndex == kHoliday_Summer )
		{
			// Summer events with specific date ranges
			// Each event lasts from mid-July to mid-September
			if (
			(nCurrentEra >= 5778) && nCurrentEra < 5843)) || // 2023 (Jul 12 - Sep 15)
			(nCurrentEra >= 6142) && nCurrentEra < 6209)) || // 2024 (Jul 10 - Sep 15)
			(nCurrentEra >= 6510) && nCurrentEra < 6573)) )  // 2025 (Jul 16 - Sep 17)
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
