/*	vLogUtil - Simple logging utility that reads stdin and appends to log file & stdout with date-time stamp prefix	*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

char *logFileName(prefix)
  char *prefix ;
{ SYSTEMTIME st ;
  static char filename[2048] ;
  
	GetLocalTime(&st) ;
	sprintf(filename,"%s%04d%02d%02d.log",prefix,st.wYear,st.wMonth,st.wDay) ;
	return(filename) ;
}
void main(int argc, char *argv[])
{ SYSTEMTIME st ;
  FILE *fp ;
  static char Months[37]="JanFebMarAprMayJunJulAugSepOctNovDec" ;
  char timebuf[50],lbuf[2048], Month[4] ; char logFilePrefix[2048] ;

/*	First command line argument is log file prefix (directory/name) */
	if (argc > 1)
	 { strncpy(logFilePrefix,argv[1],sizeof logFilePrefix - 1) ; logFilePrefix[sizeof logFilePrefix - 1] = '\0' ; }
	 else { strcpy(logFilePrefix,"log") ; } ;

	fp = fopen(logFileName(logFilePrefix),"a") ;		/* Open up log file */
	for(;;)
	 { char lBuf[4096] ;
	   if (!fgets(lBuf,sizeof lBuf,stdin)) break ;
	   GetLocalTime(&st) ; strncpy(Month,&Months[(st.wMonth-1)*3],3) ; Month[3] = '\0' ;
	   sprintf(timebuf,"%02d-%3s %02d:%02d:%02d",st.wDay,Month,st.wHour,st.wMinute,st.wSecond) ;
	   printf("%s %s",timebuf,lBuf) ;	
	   if (fp != NULL) fprintf(fp,"%s %s",timebuf,lBuf) ;
	 } ;
	if (fp != NULL) fclose(fp) ;
}
