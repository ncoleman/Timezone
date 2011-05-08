/*
 *  Convert localtime from one timezone to another timezone.
 *  Timezone in format of /usr/share/zoneinfo directory structure:
 *  e.g. Australia/Perth, Europe/Rome, Zulu, Israel
 *  Two types of input:
 *	timezone, in which case localtime is converted to that timezone
 *	timezone time timezone, in which case the time in the first timezone is converted to the second timezone.
 *  Output:
 *	a formatted string of the time.
 *
 *  based on http://stackoverflow.com/questions/2413418/how-to-programatically-convert-a-time-from-one-timezone-to-another-in-c
 *  NJC 07/05/11  
 *
 * TODO  
 *	Automate local timezone, from link in /etc/localtime.
 *	Do error checking on input timezones, perhaps by iterating through files
 *	in /usr/share/zoneinfo to check on a match with a filename with the input timezone.
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>

#include "timezones.h"

/* alternative to hard-coding local timezone is to use readlink from unistd.h
#include <unistd.h>
readlink()
*/
// Change local timezone here
#define TIMEZONE "Australia/Perth"

// Change the time format to whatever you desire here
#define TIMEFMTIN "%Y-%m-%d %H:%M"
#define TIMEFMTOUT "%a, %d %b %Y %H:%M:%S %z(%Z)"

#define OVERWRITE 1
#define BUFLEN 100

extern char const * const timezones[];

/*
 * Check a timezone for validity, displaying a list of possible candidates if not found.
 * In that case, the function exits the program since no further progress is possible.
 *
 */
void
find_timezone(char * tz) {
    int i;
    int found = 0;
    char buf[BUFLEN];
    regex_t compiled;
    // number of elements in an array of strings: sizeof(array)/sizeof*(array)
    for (i=0; i < sizeof(timezones)/sizeof*(timezones)  ; i++) {
	if (strcmp(timezones[i], tz) == 0) {
	    return;
	}
    }
    // only get to here is timezone was not found.
    strcpy(buf, tz);
    strcat(buf, " timezone not found.  Possible candidates:");
    puts(buf);
    if (regcomp(&compiled, tz, REG_ICASE) != 0) {
	puts("Reg ex compilation failed\n") ;
	exit(-1);
    }
    for (i = 0 ; i < 500 ; i++ ) {
	// list possible timezones
	if (regexec(&compiled, timezones[i], 0, NULL, 0) == 0) {
	    found = 1;
	    puts(timezones[i]);
	}
    }
    if (!found) {
	puts("No candidates found. Try using a shorter string or a simple regex.");
    }
    puts("Finished.");
    exit(1);
}


int main (int argc, char *argv[])
{
    struct tm mytm = {0};
    time_t mytime_t;
    char buf[BUFLEN];
    char const * const tz = "TZ";
    char const * const mytz = TIMEZONE;

    mytm.tm_isdst = -1;
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

/* Usage
/tz Europe/Rome
./tz "Europe/Paris" "2004-10-30 06:30" "America/New_York"
./tz Australia/Perth "2011-05-06 19:28" America/New_York
*/
