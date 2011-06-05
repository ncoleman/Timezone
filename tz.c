/*
 * Timezone Converter
 * ==================
 * 
 * Display time converted from one timezone to another timezone, with daylight saving handled automatically.
 * 
 * See README for full documentation.
 *
 * Command line options:
 *     -s	    Suppress all messages except for the datetime.  Possible use is for scripts that don't 
 *		    care about warnings or errors.
 *		    This flag means that an invalid timezone or regex will result in no output.  
 *		    The return value can be tested to detect this.
 *     -f "xxxx"    Format customised output using strftime formatting.  See man 3 strftime.
 *
 * Input either:
 *     timezone			Local machine's localtime is converted to that timezone; or
 *     timezone time timezone	The time in the first timezone is converted to the second timezone; or
 *     regex			Timezones are searched using the regex (which can be a simple string).	
 * 
 * Return value:
 *     0	timezone(s) match exactly (perfect match) with the internal list; no errors
 *     1	at least one of the timezones was substituted because there was only one 
 *		match in the internal list (i.e. imperfect match); no errors
 *     2	an error occurred: too many timezone candidates to automatically choose one, time format wrong, regex invalid.
 *
 *  Licence:
 *	MIT-style licence (see below for particulars).  Copyright 2011 Nick Coleman 
 *	
 *  Compilation:
 *	make
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
#include <unistd.h>


/* Change the time formats to whatever you desire here.  You can use two formats for input.
 * I have set up format 1 for strict formatting, and format 2 for loose, unreadable but quick typing.
 */
// Input time formats
#define TIMEFMTINP1 "%Y-%m-%d %H:%M"
#define TIMEFMTINP2 "%Y%m%d%H%M"
// Output time format
#define TIMEFMTOUT "%a, %d %b %Y %H:%M:%S %z (%Z)"

// Constants
#define OVERWRITE 1
#define BUFLEN 100

char buf[BUFLEN] = "\0";			    // general purpose
char tz1[BUFLEN] = "\0";			    // for first timezone string
char tz2[BUFLEN] = "\0";			    // for second timezone string
int imperfect_match = 0;			    // flag to indicate a regex substitution occurred
int suppress = 0;				    // flag to suppress any warning or error messages


/*
 * Check a timezone for validity, displaying a list of possible candidates if not found.
 * In that case, the function exits the program with appropriate error code set, 
 * since no further progress is possible.
 *
 * Returns with timezone in tz1[] if perfect match or one possible match, doesn't return
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
    // TODO NJC 05/15/11  perhaps merge this loop with the next one. Is malloc() (to hold timezones found so far)
    // quicker than two loops or slower?
    for (i=0; i < sz_timezones  ; i++) {
	if (strcmp(timezones[i], tz) == 0) {
	    strncpy(tz1, timezones[i], BUFLEN);
	    return;
	}
    }
    // only get to here if timezone was not found
    // print error msg
    if (!suppress)
	printf("%s timzezone not found.  Possible candidates:\n", tz);
    if (regcomp(&compiled, tz, REG_ICASE|REG_EXTENDED|REG_NOSUB) != 0) {
	// regex compilation failed, probably because tz (argv) string is not a valid regex
	if (!suppress)
	    printf("Search failed: %s is an invalid regex\n", tz);
	exit(2);
    }
    // search for regex match on array of timezones
    for (i = 0 ; i < sz_timezones ; i++ ) {
	// list possible timezones
	if (regexec(&compiled, timezones[i], 0, NULL, 0) == 0) {
	    // regex match found, so print the matching timezone
	    // if this is the first one found, copy it to buffer for possible re-use
	    if (++found == 1)
		strncpy(buf, timezones[i], BUFLEN);
	    if (!suppress)
		printf("%s\n",timezones[i]);
	}
    }
    if (!found && !suppress) {
	printf("No candidates found. Try searching with a shorter string or an extended regex.\n");
    }
    if (found == 1) {
	// found only one possible candidate, so use it
	strncpy(tz1, buf, BUFLEN);		    // use tz1, main() will switch if necessary
	imperfect_match |= 1;			    // set imperfect if not already
	return; 
    }
    // only get to here if timezone was not found nor substituted
    exit(2);
}


int main (int argc, char **argv)
{
    struct tm mytm = {0};
    time_t mytime_t;
    const char *const tz = "TZ";
    char tmfmtout[BUFLEN] = "\0";

    mytm.tm_isdst = -1;
    strncpy(tmfmtout, TIMEFMTOUT, BUFLEN);

    extern char *optarg;
    extern int opterr;
    extern int optind;
    extern int optopt;
    extern int optreset;
    int ch;
    while ((ch=getopt(argc, argv, "sf:")) != -1) {
	switch(ch) {
	    case '?':
		printf("Invalid option or missing argument\n");
		exit(2);
	    case 's':
		suppress = 1;
		break;
	    case 'f':
		strncpy(tmfmtout, argv[optind - 1], BUFLEN);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    switch (argc) {
	case 0:
	    // no input, print error msg and exit
	    if (!suppress)
		puts("Need at least one timezone.\n");
	    exit(2);
	case 1:
	    // single timezone supplied
	    find_timezone(argv[0]);
	    time(&mytime_t);	// localtime on this machine
	    setenv(tz, tz1, OVERWRITE);
	    break;
	case 3:
	    // timezone time timezone supplied
	    // find timezone in reverse order so tz1 and tz2 are set up correctly
	    find_timezone(argv[2]);
	    strncpy(tz2, tz1, BUFLEN);
	    find_timezone(argv[0]);
	    setenv(tz, tz1, OVERWRITE);
	    tzset();
	    // try first format time input string
	    if (strptime(argv[1], TIMEFMTINP1 , &mytm) == NULL) {
		// failed, try second format
		if (strptime(argv[1], TIMEFMTINP2, &mytm) == NULL) {
		    // still failed, error msg and exit.
		    if (!suppress)
			printf("Time format not valid.\nShould be (see man 3 strftime): %s or %s\n", TIMEFMTINP1, TIMEFMTINP2);
		    exit(2);
		}
	    }
	    mytime_t = mktime(&mytm);
	    setenv(tz, tz2, OVERWRITE);
	    break;
	default:
	    // unknown gibberish supplied
	    if (!suppress)
		puts("Invalid number of arguments.  Need <timezone> [<datetime> <timezone>]");
	    exit(2);
    }
    // do the actual timezone conversion
    tzset();
    localtime_r(&mytime_t, &mytm);
    strftime(buf, BUFLEN, tmfmtout, &mytm);
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
