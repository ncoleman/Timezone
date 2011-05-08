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
 *
 *  Licence:
 *	MIT-style licence (see below),  which basically means you can do what you want.  Copyright 2011 Nick Coleman 
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
    // print error msg
    puts(buf);
    if (regcomp(&compiled, tz, REG_ICASE|REG_EXTENDED|REG_NOSUB) != 0) {
	// regex compilation failed, probably because tz (argv) string is not a valid regex
	strcpy(buf, "Search failed: ");
	strncat(buf, tz, strlen(tz));
	strncat(buf, " is an invalid regex.", 22);
	puts(buf) ;
	exit(-1);
    }
    // search for regex match on array of timezones
    for (i = 0 ; i < sz_timezones ; i++ ) {
	// list possible timezones
	if (regexec(&compiled, timezones[i], 0, NULL, 0) == 0) {
	    // regex match found, so print the matching timezone
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

    switch (argc) {
	case 1:
	    puts("Need at least one timezone.\nExample:\nAsia/Tokyo\tor\nEurope/Paris \"2011-01-01 12:00\" America/New_York\nTo find a timezone, use a regex.");
	    exit(1);
	case 2:
	    // single timezone supplied
	    find_timezone(argv[1]);
	    time(&mytime_t);	// localtime on this machine
	    setenv(tz, argv[1], OVERWRITE);
	    break;
	case 4:
	    // timezone time timezone supplied
	    find_timezone(argv[1]);
	    find_timezone(argv[3]);
	    setenv(tz, argv[1], OVERWRITE);
	    tzset();
	    if (strptime(argv[2], TIMEFMTIN , &mytm) == NULL) {
		puts("Time format not valid.");
		exit(1);
	    }
	    mytime_t = mktime(&mytm);
	    setenv(tz, argv[3], OVERWRITE);
	    break;
	default:
	    // unknown gibberish supplied
	    puts("Invalid number of arguments.  Need <timezone> [<datetime> <timezone>]");
	    exit(1);
    }
    // do the actual timezone conversion
    tzset();
    localtime_r(&mytime_t, &mytm);
    strftime(buf, BUFLEN, TIMEFMTOUT, &mytm);
    puts(buf);

    return 0;
}

/*
Copyright (c) 2011 Nicholas Coleman

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
