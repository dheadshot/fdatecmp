#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <WinBase.h> //Lib=kernel32
/*
 * SYSTEMTIME: https://msdn.microsoft.com/en-us/library/ms724950.aspx
 * FileTimeToSystemTime: https://msdn.microsoft.com/en-us/library/ms724280.aspx
 * WIN32_FILE_ATTRIBUTE_DATA: https://msdn.microsoft.com/en-us/library/aa365739.aspx
 * GetFileAttributesEx: https://msdn.microsoft.com/en-us/library/aa364946.aspx
 * File Attribute Constants: https://msdn.microsoft.com/en-us/library/gg258117.aspx
 * GetTimeZoneInformation: https://msdn.microsoft.com/en-us/library/windows/desktop/ms724421(v=vs.85).aspx
 * TIME_ZONE_INFORMATION: https://msdn.microsoft.com/en-us/library/windows/desktop/ms725481(v=vs.85).aspx
*/

WIN32_FILE_ATTRIBUTE_DATA fad;
SYSTEMTIME ct, lat, lmt, spectime;
TIME_ZONE_INFORMATION tz;
LONG tzbias = 0, mtzbias, htzbias;
char majorver[]="1", minorver[]="00", revisionver[]="0002";

int daysinmonth(WORD year, WORD month)
{
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) return 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    if (month == 2)
    {
        if (year % 4 != 0) return 28;
        if (year % 100 != 0) return 29;
        if (year % 400 != 0) return 28;
        return 29;
    }
    return 0;
}

long cleversgn(long n, long low, long high)
{
    if (n<low) return (0-((n-1)/(high+1-low)));
    if (n>high) return ((n+1)/(high+1-low));
    return 0;
}

int converttimes(SYSTEMTIME *atime, LONG biashours, LONG biasmins)
{
    long ati = 0, atdim = 0;
    ati = ((long) (atime->wMinute)) + ((long) biasmins);
    if (ati<0) atime->wMinute = ati+60;
    else if (ati>59) atime->wMinute = ati - 60;
    else atime->wMinute = ati;
    ati = cleversgn(ati,0,59);
    ati += ((long) (atime->wHour)) + ((long) biashours);
    if (ati<0) atime->wHour = ati+24;
    else if (ati>23) atime->wHour = ati - 24;
    else atime->wHour = ati;
    ati = cleversgn(ati,0,23);
    ati += ((long) (atime->wDay));
    atdim = ((long) (daysinmonth(atime->wYear, atime->wMonth)));
    if (atdim == 0) return 0;
    if (ati<1) atime->wDay = ati+atdim;
    else if (ati>atdim) atime->wDay = ati - atdim;
    else atime->wDay = ati;
    ati = cleversgn(ati,1,atdim);
    ati += ((long) (atime->wMonth));
    if (ati<1) atime->wMonth = ati+12;
    else if (ati>12) atime->wMonth = ati - 12;
    else atime->wMonth = ati;
    ati = cleversgn(ati,1,12);
    ati += ((long) (atime->wYear));
    if (ati<1601) atime->wYear = 1601;
    else if (ati>30827) atime->wYear = 30827;
    else atime->wYear = ati;
    return 1;
}

int main(int argc, char *argv[])
{
    int tzdst;
	SYSTEMTIME *chosentime = NULL;
    if (argc != 4)
    {
        printf("DHSC File Date Compare\nVersion: %s.%s.%s\n",majorver,minorver,revisionver);
		printf("Usage:\n  %s <File> <TimeType> <CompareDate>\nTimeTypes:\n  C\tCreated Date", argv[0]);
		printf("  M\tModified Date\n  A\tAccessed date\nCompareDate Format:\n  YYYY-MM-DD HH:MM:SS.fff\n");
		printf("Returns:\n  0\tTimes are the same\n  1\tFile is newer\n  2\tFile is older\n  10\tAn error has occurred\n");
        return 0;
    }
	switch( toupper(argv[2][0]))
	{
	    case 'A':
		  chosentime = &lat;
		break;
		
		case 'C':
		  chosentime = &ct;
		break;
		
		case 'M':
		  chosentime = &lmt;
		break;
		
		default:
	      fprintf(stderr,"Invalid Timetype '%s'!  Must be 'A', 'C' or 'M'!\n", argv[2]);
		  return 10;
		break;
	}
	memset(&spectime,0,sizeof(SYSTEMTIME));
	if (sscanf(argv[3],"%lu-%u-%u %u:%u:%u.%lu",&(spectime.wYear),&(spectime.wMonth),&(spectime.wDay),
	                                            &(spectime.wHour),&(spectime.wMinute),&(spectime.wSecond),
			   								    &(spectime.wMilliseconds)) == EOF)
	{
	    fprintf(stderr,"Input Error.\n");
		return 10;
	}
    tzdst = GetTimeZoneInformation(&tz);
    if (tzdst == TIME_ZONE_ID_UNKNOWN || tzdst == TIME_ZONE_ID_STANDARD)
    {
        tzbias = 0-(tz.Bias+tz.StandardBias);
    }
    else if (tzdst == TIME_ZONE_ID_DAYLIGHT)
    {
        tzbias = 0-(tz.Bias+tz.DaylightBias);
    }
    else
    {
        fprintf(stderr,"Error getting timezone data!\n");
        return 10;
    }
    htzbias = tzbias/60;
    mtzbias = tzbias % 60;
	if (GetFileAttributesEx(argv[1], GetFileExInfoStandard, &fad))
    {
        if (FileTimeToSystemTime(&(fad.ftCreationTime),&ct) == FALSE)
        {
            printf("Error Converting Time!\n");
            return 10;
        }
        if (converttimes(&ct,htzbias,mtzbias) == 0)
        {
            printf("Error Adjusting Time!\n");
            return 10;
        }
        if (FileTimeToSystemTime(&(fad.ftLastAccessTime),&lat) == FALSE)
        {
            printf("Error Converting Time!\n");
            return 10;
        }
        if (converttimes(&lat,htzbias,mtzbias) == 0)
        {
            printf("Error Adjusting Time!\n");
            return 10;
        }
        if (FileTimeToSystemTime(&(fad.ftLastWriteTime),&lmt) == FALSE)
        {
            printf("Error Converting Time!\n");
            return 10;
        }
        if (converttimes(&lmt,htzbias,mtzbias) == 0)
        {
            printf("Error Adjusting Time!\n");
            return 10;
        }
        
		long acmp = 0;
		acmp = chosentime->wYear - spectime.wYear;
		if (acmp<0) return 2;
		if (acmp>0) return 1;
		acmp = chosentime->wMonth - spectime.wMonth;
		if (acmp<0) return 2;
		if (acmp>0) return 1;
		acmp = chosentime->wDay - spectime.wDay;
		if (acmp<0) return 2;
		if (acmp>0) return 1;
		acmp = chosentime->wHour - spectime.wHour;
		if (acmp<0) return 2;
		if (acmp>0) return 1;
		acmp = chosentime->wMinute - spectime.wMinute;
		if (acmp<0) return 2;
		if (acmp>0) return 1;
		acmp = chosentime->wSecond - spectime.wSecond;
		if (acmp<0) return 2;
		if (acmp>0) return 1;
		acmp = chosentime->wMilliseconds - spectime.wMilliseconds;
		if (acmp<0) return 2;
		if (acmp>0) return 1;
		
        return 0;
    }
    else
    {
        fprintf(stderr,"There was an error retrieving file attributes!\n");
        return 10;
    }
}