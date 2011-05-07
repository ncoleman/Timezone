// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001
// NJC 10/10/09 added usage() options() and changed main()
// to add ability to specify a date on the command line: dd mm yy

#include <stdio.h>
#include <math.h>
#include <stdlib.h> 
#include <time.h>
#include <unistd.h>

// NJC 03/10/09 hard-coded my lat, long, and timezone

#define LAT -31.9250000
#define LONG 115.81583333
#define ZONE 8

#define TRUE 1
#define FALSE !TRUE

// NJC 05/05/11 added getopt parsing

extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;
extern int optreset;

int
getopt(int argc, char * const *argv, const char *optstring);

int abbrev_output = FALSE;

double pi = 3.14159;
double degs;
double rads;

double L,g,daylen;
double SunDia = 0.53;     // Sunradius degrees

double AirRefr = 34.0/60.0; // athmospheric refraction degrees //

//   Get the days to J2000
//   h is UT in decimal hours
//   FNday only works between 1901 to 2099 - see Meeus chapter 7
double FNday (int y, int m, int d, float h) {
    long int luku = - 7 * (y + (m + 9)/12)/4 + 275*m/9 + d;
    // type casting necessary on PC DOS and TClite to avoid overflow
    luku+= (long int)y*367;
    return (double)luku - 730531.5 + h/24.0;
};

//   the function below returns an angle in the range
//   0 to 2*pi

double FNrange (double x) {
    double b = 0.5*x / pi;
    double a = 2.0*pi * (b - (long)(b));
    if (a < 0) a = 2.0*pi + a;
    return a;
};

// Calculating the hourangle
double f0(double lat, double declin) {
    double fo,dfo;
    // Correction: different sign at S HS
    dfo = rads*(0.5*SunDia + AirRefr); if (lat < 0.0) dfo = -dfo;
    fo = tan(declin + dfo) * tan(lat*rads);
    if (fo>0.99999) fo=1.0; // to avoid overflow //
    fo = asin(fo) + pi/2.0;
    return fo;
};

// Calculating the hourangle for twilight times
double f1(double lat, double declin) {
    double fi,df1;
    // Correction: different sign at S HS
    df1 = rads * 6.0; if (lat < 0.0) df1 = -df1;
    fi = tan(declin + df1) * tan(lat*rads);
    if (fi>0.99999) fi=1.0; // to avoid overflow //
    fi = asin(fi) + pi/2.0;
    return fi;
};

//   Find the ecliptic longitude of the Sun
double FNsun (double d) {
    //   mean longitude of the Sun
    L = FNrange(280.461 * rads + .9856474 * rads * d);
    //   mean anomaly of the Sun
    g = FNrange(357.528 * rads + .9856003 * rads * d);
    //   Ecliptic longitude of the Sun
    return FNrange(L + 1.915 * rads * sin(g) + .02 * rads * sin(2 * g));
};

// Display decimal hours in hours and minutes
void showhrmn(double dhr) {
    int hr,mn;
    hr=(int) dhr;
    mn = (dhr - (double) hr)*60;
    printf("%02d:%02d",hr,mn);
};

int
usage(char *argv[]) {
    printf("Usage: %s [-s][-d dd mm yy]\n\t\t-s\tshort form: sunrise sunset time only\n\t\t-d\tdate: dd mm yy  (low digit years are this century, high digit years are previous century)\nProvides sun ephemeris for today or a specified date.\n ", argv[0]);
    exit(1);
}

// Process command line NJC 10/10/09 
//NJC 05/05/11  not needed on BSD, already declared in time.h  -> void strptime(const char *, const char *, struct tm *);
void
options(int argc, char *argv[], struct tm *p){


    int ch, day, month, year;

    while ((ch = getopt(argc, argv, "sd:h")) != -1 ) {
	switch(ch) {
	    case 's':
		abbrev_output = TRUE;
		break;
	    case 'd':
		// Simple date error checking, doesn't check for valid day of month.
		day = atoi(argv[optind - 1]);	// because optind is set up for the *next* run through getopt
		month = atoi(argv[optind]);
		year = atoi(argv[optind+1]);
		if (  day > 31 || month > 12 || year > 99 || \
			    day < 1 || month < 1 || year < 0 ){
		    printf("Error: invalid date in %i %i %i\n", day, month, year);
		    usage(argv);
		}
		// strptime 3 times to avoid mucking around with concatenating argv[1]-[3]
		strptime(argv[optind - 1], "%d", p);	    // because optind is set up the *next8 run through getopt 
		strptime(argv[optind], "%m", p); 
		strptime(argv[optind + 1], "%y", p); 
		break;	
	    case 'h':
		usage(argv);
		/* not reached */
	    default:
		usage(argv);
	}
    }

}

int main(int argc, char *argv[]){

    double y,m,day,h,latit,longit;
    float inlat,inlon,intz;
    double tzone,d,lambda;
    double obliq,alpha,delta,LL,equation,ha,hb,twx;
    double twam,altmax,noont,settm,riset,twpm;
    time_t sekunnit;
    struct tm *p;

    degs = 180.0/pi;
    rads = pi/180.0;
    //  get the date and time from the user
    // read system date and extract the year

    /** First get time **/
    time(&sekunnit);

    /** Next get localtime **/
    p=localtime(&sekunnit);

    // and change it to use command-line supplied date NJC 10/10/09 
    if ( argc > 1 )
	options(argc, argv, p) ;

    y = p->tm_year;
    // this is Y2K compliant method
    y+= 1900;
    m = p->tm_mon + 1;

    day = p->tm_mday;

    h = 12;

    //printf("year %4d month %2d\n",(int)y,(int)m); 
    //printf("Input latitude, longitude and timezone\n");
    //scanf("%f", &inlat); scanf("%f", &inlon); 
    //scanf("%f", &intz);
    inlat = LAT;
    inlon = LONG;
    intz = ZONE;
    latit = (double)inlat; longit = (double)inlon;
    tzone = (double)intz;

    // testing
    // m=6; day=10;

    d = FNday(y, m, day, h);

    //   Use FNsun to find the ecliptic longitude of the Sun
    lambda = FNsun(d);

    //   Obliquity of the ecliptic
    obliq = 23.439 * rads - .0000004 * rads * d;

    //   Find the RA and DEC of the Sun
    alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
    delta = asin(sin(obliq) * sin(lambda));

    // Find the Equation of Time in minutes
    // Correction suggested by David Smith
    LL = L - alpha;
    if (L < pi) LL += 2.0*pi;
    equation = 1440.0 * (1.0 - LL / pi/2.0);
    ha = f0(latit,delta);
    hb = f1(latit,delta);
    twx = hb - ha;  // length of twilight in radians
    twx = 12.0*twx/pi;              // length of twilight in hours
    //printf("ha= %.2f   hb= %.2f \n",ha,hb);
    // Conversion of angle to hours and minutes //
    daylen = degs*ha/7.5;
	if (daylen<0.0001) {daylen = 0.0;}
    // arctic winter     //

    riset = 12.0 - 12.0 * ha/pi + tzone - longit/15.0 + equation/60.0;
    settm = 12.0 + 12.0 * ha/pi + tzone - longit/15.0 + equation/60.0;
    noont = riset + 12.0 * ha/pi;
    altmax = 90.0 + delta * degs - latit; 
    // Correction for S HS suggested by David Smith
    // to express altitude as degrees from the N horizon
    if (latit < delta * degs) altmax = 180.0 - altmax;

    twam = riset - twx;     // morning twilight begin
    twpm = settm + twx;     // evening twilight end

    if (riset > 24.0) riset-= 24.0;
    if (settm > 24.0) settm-= 24.0;

    if (abbrev_output) {
	// sunrise, sunset, newline
	showhrmn(riset); putchar(' ');
	showhrmn(settm); puts("");
    }
    else {
	//puts("\n Sunrise and set");
	//puts("===============");

	printf("  year  : %d \n",(int)y);
	printf("  month : %d \n",(int)m);
	printf("  day   : %d \n\n",(int)day);
	printf("Days since Y2K :  %d \n",(int)d);

	printf("Latitude :  %3.1f, longitude: %3.1f, timezone: %3.1f \n",(float)latit,(float)longit,(float)tzone);
	printf("Declination   :  %.2f \n",delta * degs);
	printf("Daylength     : "); showhrmn(daylen); puts(" hours \n");
	printf("Civil twilight: ");
	showhrmn(twam); puts("");
	printf("Sunrise       : ");
	showhrmn(riset); puts("");

	printf("Sun altitude ");
	// Amendment by D. Smith
	printf(" %.2f degr",altmax);
	printf(latit>=0.0 ? " South" : " North");
	printf(" at noontime "); showhrmn(noont); puts("");
	printf("Sunset        : ");
	showhrmn(settm);  puts("");
	printf("Civil twilight: ");
	showhrmn(twpm);  puts("\n");
    }

    return 0;
}


