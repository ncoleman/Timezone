/*
 *  Display time converted from one timezone to another timezone.
 *
 *  Timezone in format of /usr/share/zoneinfo directory structure:
 *	e.g. Australia/Sydney, Europe/Rome, Zulu, Israel
 *
 *  Input either:
 *	timezone			local machine's localtime is converted to that timezone; or
 *	timezone time timezone		the time in the first timezone is converted to the second timezone.
 *  Output:
 *	a formatted string of the time; or
 *	a list of possible candidate timezones if the supplied one(s) is not recognised.
 *  Usage:
 *	tz Europe/Rome
 *	tz "Europe/Paris" "2004-10-30 06:30" "America/New_York"
 *	tz Australia/Sydney "2011-05-06 19:28" America/New_York
 *
 *  Examples:
 *	Normal:
 *	    $ tz Australia/Sydney "2011-05-06 19:28" America/New_York
 *		 Fri, 06 May 2011 05:28:00 -0400 (EDT)
 *	Not found:
 *	    $ tz paris
 *		 "paris" timezone not found.  Possible candidates:
 *		 Europe/Paris
 *	Using a regex:
 *	    $ tz "s(ain)?t_"
 *		 "s(ain)?t_" timezone not found.  Possible candidates:
 *		 America/St_Barthelemy
 *		 America/St_Johns
 *		 America/St_Kitts
 *		 America/St_Lucia
 *		 America/St_Thomas
 *		 America/St_Vincent
 *		 Atlantic/St_Helena
 *
 *  Compilation:
 *	 cc tz.c -Wall -O2  -o tz
 *
 *	 To generate the entries in timzone.h for your machine, run `find /usr/share/zoneinfo -type f -ls | cut -d '/'  -f 5-9` and edit
 *	 in your editor to add leading " and trailing ", .   The posix and right directory entries can be deleted, along with zone.tab.
 *	 You will need to regenerate (or manually add) the entries if a new timezone is created in the world.  I imagine this would
 *	 be a very rare event.
 *
 *  Remarks:
 *	Concept based on http://stackoverflow.com/questions/2413418/how-to-programatically-convert-a-time-from-one-timezone-to-another-in-c
 *
 *  Licence:
 *	MIT-style licence (see below for particulars).  Copyright 2011 Nick Coleman 
 *
*/

#define _XOPEN_SOURCE	    /*  glibc2 needs this */
#define _BSD_SOURCE	    /* ditto */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "timezones.h"

// Change the time formats to whatever you desire here
// Time format that user shall input
#define TIMEFMTIN "%Y-%m-%d %H:%M"
// Output time format
#define TIMEFMTOUT "%a, %d %b %Y %H:%M:%S %z (%Z)"

// Constants
#define OVERWRITE 1
#define BUFLEN 100

extern const char *const  timezones[];
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
	exit(1);
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
    // only get to here if timezone was not an exact match
    exit(1);
}


int main (int argc, char *argv[])
{
    struct tm mytm = {0};
    time_t mytime_t;
    const char *const tz = "TZ";

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
		strcpy(buf, "Time format not valid.\nShould be (see strftime options): ");
		strncat(buf, TIMEFMTIN, strlen(TIMEFMTIN));
		puts(buf);
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
