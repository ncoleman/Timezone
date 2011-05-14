/*
 * Timezone Converter
 * ==================
 * 
 * Display time converted from one timezone to another timezone.
 * 
 * Input either:
 *     timezone			Local machine's localtime is converted to that timezone; or
 *     timezone time timezone	The time in the first timezone is converted to the second timezone; or
 *     regex			Timezones are searched using the regex (which can be a simple string).	
 * 
 * Input Format:
 *     timezone in the format of the /usr/share/zoneinfo directory structure (e.g.
 * 	Australia/Sydney, Europe/Rome, Zulu, Israel), or an extended regex.  If the regex
 * 	matches only one timezone from the internal list, that timezone is
 * 	susbstituted automatically on the basis that the user should not have to retype
 * 	it again.  The return value can be used to detect if this substitution has
 * 	occurred.
 * 	The timezone is case-sensitive, but the regex search is case-insensitive.  
 * 	This lets you do quick data entry of "paris" since the regex search
 * 	will substitute the correct value of Europe/Paris.
 * 	The regex does partial matches automatically.  You don't need to specify ".*foo.*",
 * 	foo will work fine.
 *     time in either of two strftime formats (which can be changed in #define in the source code):
 * 	"%Y-%m-%d %H:%M" or %Y%m%d%H%M (the second for quick and dirty typing, 
 * 	sacrificing legibility).
 * 
 * Output:
 *     formatted string of the time; or
 *     list of possible candidate timezones.
 * 
 * Return value:
 *     0	timezone(s) match exactly (perfect match) with the internal list; no errors
 *     1	at least one of the timezones was substituted because there was only one 
 * 	match in the internal list (i.e. imperfect match); no errors
 *     2	an error occurred: too many timezone candidates to automatically choose one, time format wrong, regex invalid.
 * 
 * Usage:
 *     tz paris
 *     tz "s(aint)?_"
 *     tz Europe/Rome
 *     tz "Europe/Paris" "2004-10-30 06:30" "America/New_York"
 *     tz Australia/Sydney 201105061928 America/New_York
 *
 *  Remarks:
 *	Concept based on http://stackoverflow.com/questions/2413418/how-to-programatically-convert-a-time-from-one-timezone-to-another-in-c
 *
 *  Licence:
 *	MIT-style licence (see below for particulars).  Copyright 2011 Nick Coleman 
 *	
 *  Compilation:
 *	cc -o tz tz.c timezones.c	
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


/* Change the time formats to whatever you desire here.  You can use two formats for input.
 * I set up format 1 for strict formatting, and format 2 for loose, unreadable but quick typing.
 */
// Time format that user shall input
#define TIMEFMTINP1 "%Y-%m-%d %H:%M"
#define TIMEFMTINP2 "%Y%m%d%H%M"
// Output time format
#define TIMEFMTOUT "%a, %d %b %Y %H:%M:%S %z (%Z)"

// Constants
#define OVERWRITE 1
#define BUFLEN 100

char buf[BUFLEN];				    // general purpose
char tz1[BUFLEN];				    // for first timezone string
char tz2[BUFLEN];				    // for second timezone string
int imperfect_match = 0;			    // flag to indicate a regex substitution occurred


/*
 * Check a timezone for validity, displaying a list of possible candidates if not found.
 * In that case, the function exits the program since no further progress is possible.
 *
 * Returns with timezone in buf[] if perfect match or one possible match, doesn't return
 * if multiple or no match.
 *
 */
void
find_timezone(char * tz) {
    int i;
    int found = 0;
    extern const char *const timezones[];
    extern const int sz_timezones;
    regex_t compiled;
    // check if the supplied timezone string is found and return if it is
    for (i=0; i < sz_timezones  ; i++) {
	if (strcmp(timezones[i], tz) == 0) {
	    strcpy(tz1, timezones[i]);
	    return;
	}
    }
    // only get to here if timezone was not found
    // print error msg
    printf("%s timzezone not found.  Possible candidates:", tz);
    if (regcomp(&compiled, tz, REG_ICASE|REG_EXTENDED|REG_NOSUB) != 0) {
	// regex compilation failed, probably because tz (argv) string is not a valid regex
	printf("\nSearch failed: %s is an invalid regex\n", tz);
	exit(2);
    }
    // search for regex match on array of timezones
    for (i = 0 ; i < sz_timezones ; i++ ) {
	// list possible timezones
	if (regexec(&compiled, timezones[i], 0, NULL, 0) == 0) {
	    // regex match found, so print the matching timezone
	    // if this is the first one found, copy it to buffer for possible re-use
	    if (++found == 1)
		strcpy(buf, timezones[i]);
	    printf("\n%s",timezones[i]);
	}
    }
    if (!found) {
	printf("\nNo candidates found. Try searching with a shorter string or an extended regex.");
    }
    if (found == 1) {
	// found only one possible candidate, so use it
	printf(" is the only candidate, so using it.\n");
	strcpy(tz1,buf);		    // use tz1, main() will switch if necessary
	imperfect_match |= 1;		    // set imperfect if not already
	return; 
    }
    printf("\n");
    // only get to here if timezone was not found nor substituted
    exit(2);
}


int main (int argc, char *argv[])
{
    struct tm mytm = {0};
    time_t mytime_t;
    const char *const tz = "TZ";

    mytm.tm_isdst = -1;

    switch (argc) {
	case 1:
	    // no input, print error msg and exit
	    puts("Need at least one timezone.\nExample:\nAsia/Tokyo\tor\nEurope/Paris \"2011-01-01 12:00\" America/New_York\nTo find a timezone, use a regex.");
	    exit(2);
	case 2:
	    // single timezone supplied
	    find_timezone(argv[1]);
	    time(&mytime_t);	// localtime on this machine
	    setenv(tz, tz1, OVERWRITE);
	    break;
	case 4:
	    // timezone time timezone supplied
	    // find timezone in reverse order so tz1 and tz2 are set up correctly
	    find_timezone(argv[3]);
	    strcpy(tz2, tz1);
	    find_timezone(argv[1]);
	    setenv(tz, tz1, OVERWRITE);
	    tzset();
	    // try first format time input string
	    if (strptime(argv[2], TIMEFMTINP1 , &mytm) == NULL) {
		// failed, try second format
		if (strptime(argv[2], TIMEFMTINP2, &mytm) == NULL) {
		    // still failed, error msg and exit.
		    printf("Time format not valid.\nShould be (see man 3 strftime): %s or %s\n", TIMEFMTINP1, TIMEFMTINP2);
		    exit(2);
		}
	    }
	    mytime_t = mktime(&mytm);
	    setenv(tz, tz2, OVERWRITE);
	    break;
	default:
	    // unknown gibberish supplied
	    puts("Invalid number of arguments.  Need <timezone> [<datetime> <timezone>]");
	    exit(2);
    }
    // do the actual timezone conversion
    tzset();
    localtime_r(&mytime_t, &mytm);
    strftime(buf, BUFLEN, TIMEFMTOUT, &mytm);
    puts(buf);

    return imperfect_match;				    // 0 is no substitutions, 1 is at least one substitution
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
