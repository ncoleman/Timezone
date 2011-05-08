/*
 *  Convert localtime from one timezone to another timezone.
 *
 *  Timezone in format of /usr/share/zoneinfo directory structure:
 *	e.g. Australia/Perth, Europe/Rome, Zulu, Israel
 *  Input either:
 *	timezone, localtime is converted to that timezone; or
 *	timezone time timezone, the time in the first timezone is converted to the second timezone.
 *  Output:
 *	a formatted string of the time; or
 *	a list of possible candidate timezones if the supplied one(s) is not recognised.
 *  Usage:
 *	tz Europe/Rome
 *	tz "Europe/Paris" "2004-10-30 06:30" "America/New_York"
 *	tz Australia/Perth "2011-05-06 19:28" America/New_York
 *  Example output:
 *	Sun, 08 May 2011 09:09:57 +0200(CEST)
 *
 *  Compilation:
 *	 cc tz.c -Wall -O2  -o tz
 *
 *  Concept based on http://stackoverflow.com/questions/2413418/how-to-programatically-convert-a-time-from-one-timezone-to-another-in-c
 *  copyright NJC 07 May 2011  
 *
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "timezones.h"

// Change local timezone here (not used in current version)
//#define TIMEZONE "Australia/Perth"

// Change the time format to whatever you desire here
#define TIMEFMTIN "%Y-%m-%d %H:%M"
#define TIMEFMTOUT "%a, %d %b %Y %H:%M:%S %z(%Z)"

#define OVERWRITE 1
#define BUFLEN 100

extern char const * const timezones[];
char buf[BUFLEN];

/*
 * Check a timezone for validity, displaying a list of possible candidates if not found.
 * In that case, the function exits the program since no further progress is possible.
 *
 */
void
find_timezone(char * tz) {
    int i;
    int found = 0;
    char msg[] = " timezone not found.  Possible candidates:";
    // number of elements in an array of strings: sizeof(array)/sizeof*(array)
    int sz_timezones = sizeof(timezones)/sizeof*(timezones);
    regex_t compiled;
    // check if the supplied timezone string is found and return if it is
    for (i=0; i < sz_timezones  ; i++) {
	if (strcmp(timezones[i], tz) == 0) {
	    return;
	}
    }
    // only get to here if timezone was not found
    strcpy(buf,"\"");			// strcpy safe here, first use of buffer
    strncat(buf, tz, strlen(tz));	// not using strlcat because glibc doesn't support it    
    strncat(buf,"\"", 1);
    strncat(buf, msg , strlen(msg));
    puts(buf);
    if (regcomp(&compiled, tz, REG_ICASE|REG_EXTENDED|REG_NOSUB) != 0) {
	puts("Reg ex compilation failed\n") ;
	exit(-1);
    }
    for (i = 0 ; i < sz_timezones ; i++ ) {
	// list possible timezones
	if (regexec(&compiled, timezones[i], 0, NULL, 0) == 0) {
	    found = 1;
	    puts(timezones[i]);
	}
    }
    if (!found) {
	puts("No candidates found. Try searching with a shorter string or an extended regex.");
    }
    exit(1);
}


int main (int argc, char *argv[])
{
    struct tm mytm = {0};
    time_t mytime_t;
    char const * const tz = "TZ";

    mytm.tm_isdst = -1;

    if (argc <= 1) {
	puts("\nNeed at least one timezone.\n");
	exit(1);
    }
    if (argc > 2) {
	find_timezone(argv[1]);
	find_timezone(argv[3]);
	setenv(tz, argv[1], OVERWRITE);
	tzset();
	strptime(argv[2], TIMEFMTIN , &mytm);
	mytime_t = mktime(&mytm);
	setenv(tz, argv[3], OVERWRITE);
    } 
    else {
	find_timezone(argv[1]);
	time(&mytime_t);	// localtime on this machine
	setenv(tz, argv[1], OVERWRITE);
    }
    tzset();
    localtime_r(&mytime_t, &mytm);
    strftime(buf, BUFLEN, TIMEFMTOUT, &mytm);
    puts(buf);

    return 0;
}

