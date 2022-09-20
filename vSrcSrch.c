#define WINDOWS defined _WIN32 || defined _WIN64

/*	To build on linux-
	  cc -m32 -O3 -o xv4rpp -DLINUX486 -DINCLUDE_VCOMMON -w v4rpp.c vcommon.c -lm
*/

#include <stdio.h>
#if WINDOWS
#include <io.h>
#include <ctype.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
//#include <audiomediatype.h>

/*	Define UCxxx in case we ever want to move back to V4 standard & Unicode */
#define UCEOS '\0'
#if WINDOWS
#define UCstrcpy_s strcpy_s
#define UCstrtol strtol
#define UCstrstr strstr
#define UCstrchr strchr
#define UCstrncmp strncmp
#define UCstrlen strlen
#define UCstrcat_s strcat_s
#define UCstricmp _stricmp
#else
#define UCstrcpy_s(DST,LEN,SRC) strcpy(DST,SRC)
#define UCstrtol strtol
#define UCstrstr strstr
#define UCstrchr strchr
#define UCstrncmp strncmp
#define UCstrlen strlen
#define UCstrcat_s(DST,LEN,SRC) strcat(DST,SRC)
#define UCstricmp strcasecmp
#endif
#define UClit(_lit) _lit
#define UCempty(_str) (_str[0] == '\0')
#define UCnotempty(_str) (_str[0] != '\0')
#define UCsizeof(element) ((sizeof element)/sizeof(UCCHAR))
#define ZUS(_str) _str[0] = UCEOS
#define UCtoupper(ucchar) toupper(ucchar)
typedef char UCCHAR ;
typedef int COUNTER  ;
typedef int INDEX ;
typedef int LOGICAL ;

#define V_FileName_Max 1024
#define VSS_PatternMax 512

#define TRUE 1
#define FALSE 0
#define UNUSED -1
#define EXITABORT EXIT_FAILURE
#define EXITOK EXIT_SUCCESS

#define EXITErr exit(EXITABORT)
#define EXITOk exit(EXITOK)

#define SRCSRCH_VERSION "2.0"

/*	vuc_StrStrIC - Searches for pattern in string ignoring case
	  NOTE: pattern assumed to be converted to UPPER CASE!		*/

UCCHAR *vuc_StrStrIC(str,pat)
  UCCHAR *str,*pat ;
{ int plen ; UCCHAR save, *b, *e ;

	plen = UCstrlen(pat) - 1 ;
	for(b=&str[plen];;)
	 { e = UCstrchr(b,pat[plen]) ;	/* Search for last character in pattern */
	   for(e=b;;e++)
	    { if (*e == UCEOS) { e = NULL ; break ; } ;
	      if (UCtoupper(*e) == pat[plen]) break ;
	    } ;
	   if (e == NULL) return(NULL) ;	/* Did not find pat in str */
	   save = *(e+1) ; *(e+1) = UCEOS ;
	   if (UCstricmp(&e[-plen],pat) == 0) { *(e+1) = save ; return(e-plen) ; } ;
	   *(e+1) = save ; b = e + 1 ;
	 } ;
}

void showHelp() ;
void formatOutputLine(UCCHAR *, INDEX, UCCHAR *, UCCHAR *, LOGICAL) ;

UCCHAR **startup_envp ;
#ifdef WINNT
wmain(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  UCCHAR *argv[] ;		/* Argument values */
  UCCHAR **envp ;			/* Environment pointer */
#else
main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
#endif
{ 
#ifndef USEV4FILE
  FILE *fp ;
#endif

#define UCFile_GetBufMaxChars 2000000
#define SEARCH_WINDOW_LINES 25
#define MAX_MATCHES 25
#define MAX_LBUF_SIZE 1024
  struct s__matches {
    COUNTER count ;
    struct {
      COUNTER lines ;
      COUNTER startLine ;
      UCCHAR *lbuf[SEARCH_WINDOW_LINES] /* [MAX_LBUF_SIZE] */ ;
     } match[MAX_MATCHES] ;
  } matches ;
  UCCHAR patText[VSS_PatternMax], fnInput[V_FileName_Max], curFunc[64], curJobNum[V_FileName_Max], curDateTime[64], matchFunc[128], urlLogPrefix[512], totCPU[32] ;
  UCCHAR *arg, *b, *b1, *ucb, *ucbuf[SEARCH_WINDOW_LINES] /* [UCFile_GetBufMaxChars] */ ;
#define FILEBUF_SIZE 1048576
  char *fileBuf ; char *cip ; LOGICAL hitEOF ;
  INDEX i,before,after, doAfter, curLine, matchLine ; LOGICAL onlyOnce, onlyFile, ignoreCase, prettyPrint, searchAJAX ; COUNTER ilines,errCnt,totMatches, totFileMatches ;

/*	Allocate & initialize variables, buffers, etc. */
	fileBuf = malloc(FILEBUF_SIZE+1) ;
	for (i=0;i<MAX_MATCHES;i++)
	 { INDEX j ;
	   for (j=0;j<SEARCH_WINDOW_LINES;j++)
	    { matches.match[i].lbuf[j] = malloc(MAX_LBUF_SIZE+1) ; } ;
	 } ;
	for (i=0;i<SEARCH_WINDOW_LINES;i++)
	 { ucbuf[i] = malloc(UCFile_GetBufMaxChars) ; } ;

	curLine = 0 ; ZUS(fnInput) ; onlyOnce = FALSE ; onlyFile = FALSE ; searchAJAX = FALSE ; ignoreCase = FALSE ; prettyPrint = FALSE ; ZUS(urlLogPrefix) ; ZUS(totCPU) ; ZUS(matchFunc) ;
	before = 0 ; after = 0 ; totMatches = 0 ; totFileMatches = 0 ; ZUS(patText) ;

#define NA if (*(++arg) == 0) arg = argv[++i] ; if (arg == NULL) goto err_arg ; if (*arg == UClit('-')) goto err_arg ;
#define NUM(VAR) { UCCHAR *delim ; VAR = UCstrtol(arg,&delim,10) ; if (*delim != UCEOS) goto err_int ; } ;
	startup_envp = envp ;
	for(i=1;i<argc;i++)
	 { arg = argv[i] ;					/* Next argument */
	   if (*arg == UClit('-')) { arg++ ; }
	    else { if (UCempty(fnInput)) { UCstrcpy_s(fnInput,V_FileName_Max,arg) ; continue ; }
		   printf("? Invalid command line - double input file\n") ; EXITErr ;
		 } ;
	   switch (*arg)
	    { default:	printf("? Invalid command line option: %s\n",arg) ; EXITErr ;
	      case UClit('a'):				/* -a num - show num lines after */
		NA NUM(after) break ;
	      case UClit('b'):				/* -b num - show num lines before */
		NA NUM(before) ; break ;
	      case UClit('d'):				/* Format for pretty display */
		prettyPrint = TRUE ; break ;
	      case UClit('f'):				/* -f matchfunc to search for */
		NA UCstrcpy_s(matchFunc,UCsizeof(matchFunc),arg) ; break ;
	      case UClit('h'):
		showHelp() ; EXITOk ;
	      case UClit('i'):				/* -i - ignore case */
		ignoreCase = TRUE ; break ;
	      case UClit('j'):				/* -j - Look at AJAX result files instead of logs */
		searchAJAX = TRUE ; break ;
	      case UClit('m'):				/* -f - only output matching filename */
		onlyFile = TRUE ; onlyOnce = TRUE ; break ;
	      case UClit('p'):				/* -p patternToSearchFor */
		NA UCstrcpy_s(patText,UCsizeof(patText),arg) ; break ;
	      case UClit('s'):				/* -s - only show pattern once per file */
		onlyOnce = TRUE ; break ;
	      case UClit('u'):				/* -u urlprefix - for linking to full log file */
		NA UCstrcpy_s(urlLogPrefix,UCsizeof(urlLogPrefix),arg) ; break ;
	    } ;
	 } ;
	if (UCempty(patText)) { printf("? Missing a pattern\n") ; EXITErr ; } ;
	if (ignoreCase)
	 { INDEX i ; for(i=0;patText[i]!=UCEOS;i++) { patText[i] = UCtoupper(patText[i]) ; } ; } ;
	if (onlyFile && (before != 0 || after != 0))
	 { before = 0 ; after = 0 ;
	   printf("Setting -b & -a to zero with -m option\n") ;
	 } ;
	if (before < 0 || after < 0 || (before + after + 1) >= SEARCH_WINDOW_LINES)
	 {  printf("? Before+After window cannot exceed %1d lines\n",SEARCH_WINDOW_LINES) ; EXITErr ; } ;
/*	Open input- if no file then read from stdin */
	if (UCempty(fnInput))
	 { 
#if WINDOWS
	   fp = stdin ; _setmode(_fileno(stdin), _O_BINARY) ;
#else
	   fp = freopen(NULL,"rb",stdin) ;
#endif
	 } else
	 { char name[V_FileName_Max] ;
	   for(i=0;i<V_FileName_Max;i++) { name[i] = fnInput[i] ; if (fnInput[i] == UCEOS) break ; } ;
#if WINDOWS
	   if (fopen_s(&fp,name,"rb") != 0)
	    { printf("? Could not locate input file: %s\n",fnInput) ; EXITErr ; } ;
#else
	   if ((fp = fopen(name,"rb")) == NULL)
	    { printf("? Could not locate input file: %s\n",fnInput) ; EXITErr ; } ;
#endif
	 } ;
	matches.count = 0 ; matchLine = UNUSED ; curLine = 0 ; ZUS(curFunc) ; ZUS(curJobNum) ; doAfter = 0 ;
	if (prettyPrint)
	 { printf("\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\">\n\
<head>\n\
<style type='text/css'>\n\
.title { text-align:center; font-weight:bold; font-size:larger; }\n\
.fileHeader { font-weight:bold; }\n\
.footer { border-top:1px dotted black; padding-top:.4em; border-bottom:2px solid black; padding-bottom:.4em; font-weight:bold; }\n\
.matchesInFile { margin-left:.5 in; background-color: #f0f0f0; padding:.2em; }\n\
.pattern { color:blue; }\n\
.errs { color:red; font-weight:bold; }\n\
</style>\n\
</head>\n\
<body>\n") ;
	  printf("<p class='title'>xvsrcSrch v%s - Pattern **%s** - Func:%s</p>\n",SRCSRCH_VERSION,patText,(UCempty(matchFunc) ? UClit("any") : matchFunc)) ;
	 } ;

	fileBuf[0] = '\0' ; cip = fileBuf ; hitEOF = FALSE ;
	for(ilines=0,errCnt=0;;ilines++)
	 { ucb = ucbuf[(curLine++)%SEARCH_WINDOW_LINES] ;
	   ucb[0] = '\0' ;
	   for(;;)
	    { char *ep ;
	      ep = strchr(cip,'\n') ;
/*	      If doing ajax then don't have any new lines in file, replaced leading "{" with newline so put it back in here */
	      if (searchAJAX)
	       { if (*cip == UClit('"')) UCstrcat_s(ucb,UCFile_GetBufMaxChars,UClit("{")) ;
	       } ;
	      if (ep != NULL) { *ep = '\0' ; UCstrcat_s(ucb,UCFile_GetBufMaxChars,cip) ; cip = ep + 1 ; break ; }
	       { COUNTER num ; INDEX i ;
	         UCstrcat_s(ucb,UCFile_GetBufMaxChars,cip) ;
		 num = fread(fileBuf,1,FILEBUF_SIZE,fp) ;
		 if (num <= 0) { hitEOF = TRUE ; break ; } ;
		 fileBuf[num] = '\0' ;
		 if (searchAJAX)
		  { UCCHAR *bp, *p ;
		    for(bp=fileBuf;;)
		     { p = UCstrchr(bp,UClit('{')) ;
		       if (p == NULL) break ;
		       bp = p + 1 ;
/*		       Replace leading "{" with newline because there are none in the input stream */
		       if (UCstrncmp(p,UClit("{\"meta\":"),8) != 0) continue ;
		       if (p != fileBuf) *p = '\n' ;
/*		       Now search for end with next 'line' being begin of V4 log */
		       p = UCstrstr(p+1,UClit("}V4 1")) ;
		       if (p == NULL) continue ;
		       *(p+1) = '\n' ;
		     } ;
		  } else
		  { for(i=0;i<num;i++) { if(fileBuf[i] <= 26 && fileBuf[i] != '\n') fileBuf[i] = ' ' ; } ;
		  } ;
		 cip = fileBuf ;
		 } ;
	     } ;
	   if (hitEOF) break ;
/*	   Is this an AJAX file? */
	   if (UCstrncmp(ucb,UClit("{\"meta\":"),8) == 0)
	    { if (!searchAJAX) continue ;
/*	      Also matching on func? */
	      if (UCnotempty(matchFunc))
	       { UCCHAR *bp ; INDEX i ;
	         bp = UCstrstr(ucb,"\"func\":\"") ;
		 if (bp != NULL)
		  { bp += 9 ;
		    for(i=0;(i < 1 ? TRUE : *bp!=UClit('"'))&&i<UCsizeof(curFunc)-1;bp++) { curFunc[i++] = *bp ; } ;
		    curFunc[i++] = UCEOS ;
		    if (UCstricmp(curFunc,matchFunc) != 0) continue ;
		  } ;
	       } ;
	      if (ignoreCase)
	       { if (vuc_StrStrIC(ucb,patText) == NULL) continue ;
	       } else { if (UCstrstr(ucb,patText) == NULL) continue ; } ;
	      totMatches++ ; totFileMatches++ ;
	      if (prettyPrint)
	       { UCCHAR obuf2[UCFile_GetBufMaxChars+64] ;
		 formatOutputLine(obuf2,UCFile_GetBufMaxChars,ucb,patText,ignoreCase) ;
	         if (prettyPrint) printf("<p class='matchesInFile'>%s</p>\n",obuf2) ;
	       } else
	       { printf("%s\n",ucb) ; } ;
	      continue ;
	    } else
	    { if (searchAJAX) continue ;
	    } ;

/*	   Looking at a log file - check for certain patterns */
	   if (UCstrstr(ucb,UClit("Data that thinks")) != NULL)
	    { matches.count = 0 ; matchLine = UNUSED ; curLine = 0 ; ZUS(curFunc) ; ZUS(curJobNum) ; continue ; } ;
	   if (UCstrstr(ucb,UClit("CommonPreEval")) != NULL)
	    { b = UCstrchr(ucb,',') ; if (b != NULL) b1 = UCstrchr(b,'"') ;
	      if (b != NULL && b1 != NULL)
	       { *b1 = UCEOS ; UCstrcpy_s(curJobNum,UCsizeof(curJobNum),b+1) ; } ;
	      if (UCempty(curFunc))
	       { b = UCstrchr(ucb,'"') ; if (b != NULL) b1 = UCstrchr(b,',') ;
		 if (b != NULL && b1 != NULL)
		  { *b1 = UCEOS ; UCstrcpy_s(curFunc,UCsizeof(curFunc),b+1) ; } ;
	       } ;
	    } ;
	   if (b = UCstrstr(ucb,UClit("Context ADV ajaxMod:")))
	    { INDEX j ; b = UCstrchr(ucb,':') + 1 ;
	      for(i=0,j=0;;i++)
	       { if (b[i] == UClit(' ')) continue ;
	         if (b[i] <= 26) break ;
		 curFunc[j++] = b[i] ;
	       } ; curFunc[j++] = UCEOS ;
//	     UCstrcpy_s(curFunc,UCsizeof(curFunc),b+1) ;
	    } ;
	   if (matchLine == UNUSED)
	    { if (ignoreCase)
	       { if (vuc_StrStrIC(ucb,patText) != NULL) { matchLine = curLine + after ; } ;
	       } else { if (UCstrstr(ucb,patText) != NULL) { matchLine = curLine + after ; } ; } ;
	    } ;
/*	   If current line is matchLine then save info in match structure */
	   if (curLine == matchLine || (matchLine != UNUSED && UCstrncmp(ucb,UClit("V4Exit- "),8) == 0))
	    { INDEX sx,ex,lx ; 
	      matchLine = UNUSED ;
	      matches.count++ ;
	      if (onlyOnce && matches.count > 1) continue ;
/*	      Save the matching line and any before/after lines */
	      if (matches.count > MAX_MATCHES) continue ;
	      sx = (curLine - (before + 1 + after) < 0 ? 0 : curLine - (before + 1 + after)) ; ex = curLine ;
	      matches.match[matches.count-1].startLine = sx + 1 ;
	      matches.match[matches.count-1].lines = ex - sx ;
	      for(lx=0;sx<=ex;sx++)
	       { ucbuf[sx % SEARCH_WINDOW_LINES][MAX_LBUF_SIZE-1] = UCEOS ;
	         if (sx > curLine) break ;
	         UCstrcpy_s(matches.match[matches.count-1].lbuf[lx++],MAX_LBUF_SIZE,ucbuf[sx % SEARCH_WINDOW_LINES]) ;
	       } ;
	    } ;
	   if (UCstrncmp(ucb,UClit("V4Exit- "),8) == 0 || UCstrncmp(ucb,UClit("V4ExitErr- "),11) == 0)
	    { INDEX mx, lx ; char errs[128] ;
/*	      Here is where we dump out any/all matches */
	      if (matches.count <= 0) continue ;
	      if (UCnotempty(matchFunc) && UCstricmp(matchFunc,curFunc) != 0) continue ;
	      totFileMatches++ ; totMatches += matches.count ;
	      b = UCstrstr(ucb,UClit("CPU:")) ; if (b != NULL) b1 = UCstrchr(b+1,',') ;
	      if (b != NULL && b1 != NULL)
	       { *b1 = UCEOS ; UCstrcpy_s(totCPU,UCsizeof(totCPU),b+4) ;
	       } ;
	      ZUS(errs) ; b = UCstrstr(ucb,UClit("Errs:")) ; if (b != NULL) b1 = UCstrchr(b+1,',') ;
	      if (b != NULL && b1 != NULL)
	       { *b1 = UCEOS ; UCstrcpy_s(errs,UCsizeof(errs),(prettyPrint ? ", <span class='errs'>" : UClit(", "))) ; UCstrcat_s(errs,UCsizeof(errs),b) ; if (prettyPrint) UCstrcat_s(errs,UCsizeof(errs),UClit("</span>")) ;
	       } ;
	      b = UCstrchr(ucb,' ') ; if (b != NULL) b1 = UCstrchr(b,',') ;
	      if (b != NULL && b1 != NULL)
	       { *b1 = UCEOS ; UCstrcpy_s(curDateTime,UCsizeof(curDateTime),b+1) ; 
	       } ;
	      if (UCempty(curFunc))
	       { sprintf_s(curFunc,UCsizeof(curFunc),"? (line #%d)",ilines-after) ; } ;
	      if (prettyPrint)
	       { printf("<p class='fileHeader'>Function: %s, Matches=%d JobNumber: <a href='%s%s.%s'>%s</a>, Date-Time: %s%s</p>\n",curFunc,matches.count,urlLogPrefix,curJobNum,totCPU,curJobNum,curDateTime,errs) ;
	       } else
	       { printf("==== Function: %s, Matches=%d JobNumber: %s, Date-Time: %s%s\n",curFunc,matches.count,curJobNum,curDateTime,errs) ;
	       } ;
	      if (onlyFile) continue ;
	      for(mx=0;mx<matches.count && mx < MAX_MATCHES;mx++)
	       { 
		 if (prettyPrint) printf("<p class='matchesInFile'>\n") ;
	         for (lx=0;lx<matches.match[mx].lines;lx++)
	          { if (prettyPrint)
		     { char obuf2[MAX_LBUF_SIZE+64] ;
		       formatOutputLine(obuf2,MAX_LBUF_SIZE+64,matches.match[mx].lbuf[lx],patText,ignoreCase) ;
		       printf("   %d. %s<br/>\n",matches.match[mx].startLine+lx,obuf2) ;
		     } else
		     { printf("   %d. %s\n",matches.match[mx].startLine+lx,matches.match[mx].lbuf[lx]) ; } ;
		  } ;
		 if (prettyPrint) { printf("</p>\n") ; }
		  else { printf("----\n") ; } ;
	         if (onlyOnce) break ;
	       } ;
	    } ;
	 } ;
	if (fp != stdin) fclose(fp) ;
	if (prettyPrint)
	 { printf("<p class='footer'>xvsrcSrch scanned %d lines, %d matches found in %d files</p>\n",ilines,totMatches,totFileMatches) ;
	 } else { printf("xvsrcSrch scanned %d lines, %d matches found in %d files\n",ilines,totMatches,totFileMatches) ; } ;

	if (prettyPrint) { printf("</body>\n</html>\n") ; } ;

	EXITOk ;

err_arg: printf("? Invalid command line switch: %s\n",arg) ; EXITErr ;

err_int: printf("? Argument not an integer: %s\n",arg) ; EXITErr ;
}

void formatOutputLine(out,outMax,in,patText,ignoreCase)
  UCCHAR *out ;
  INDEX outMax ;
  UCCHAR *in ;
  UCCHAR *patText ;
  LOGICAL ignoreCase ;
{
  char *bp, obuf[UCFile_GetBufMaxChars+64] ;
  INDEX ix, ox ;
#define STARTSPAN "\x14"
#define ENDSPAN "\x15"
#define APPENDOBUF2(_str) { INDEX _i ; for(_i=0;_str[_i]!=UCEOS;_i++) { if (ox < outMax-1) out[ox++] = _str[_i] ; } ; } ;

	bp = (ignoreCase ? vuc_StrStrIC(in,patText) : UCstrstr(in,patText)) ;
	if (bp != NULL)
	 { char save = *bp ; *bp = UCEOS ; UCstrcpy_s(obuf,UCsizeof(obuf),in) ; *bp = save ;
	   UCstrcat_s(obuf,UCFile_GetBufMaxChars+64,STARTSPAN) ;
	   save = *(bp + UCstrlen(patText)) ; *(bp + UCstrlen(patText)) = UCEOS ; UCstrcat_s(obuf,UCFile_GetBufMaxChars,bp) ; *(bp + UCstrlen(patText)) = save ;
	   UCstrcat_s(obuf,UCFile_GetBufMaxChars+64,ENDSPAN) ;
	   UCstrcat_s(obuf,UCFile_GetBufMaxChars+64,bp+UCstrlen(patText)) ;
	 } else
	 { UCstrcpy_s(obuf,UCsizeof(obuf),in) ; } ;
/*	Now look for any (HTML) tags */
	for(ix=0,ox=0;;ix++)
	 { if (obuf[ix] == '<') { APPENDOBUF2("&lt;") ; continue ; } ;
	   if (obuf[ix] == *STARTSPAN) { APPENDOBUF2("<span class='pattern'>") ; continue ; } ;
	   if (obuf[ix] == *ENDSPAN) { APPENDOBUF2("</span>") ; continue ; } ;
	   if (ox < outMax-1) out[ox++] = obuf[ix] ;
	   if (obuf[ix] == UCEOS) break ;
	 } ;
	obuf[ix++] = UCEOS ;
}

void showHelp()
{
	printf("xvSrcSrch version %s\n",SRCSRCH_VERSION) ;
	printf("Usage: xvSrcSrch filename [optional switches]\n\
  -a num	Show num lines after match\n\
  -b num	Show num lines before match\n\
  -d		Format for easy-to-ready display\n\
  -f func	Only look in logs running specified fund\n\
  -h		Output this help message\n\
  -i		Ignore case when matching\n\
  -j		Search AJAX result files instead of log files\n\
  -m		Only output matching filename(s)\n\
  -p 'pattern'	Search for specified pattern\n\
  -s		Show only 1 (single) instance of matched line(s) per file\n\
  -u url	URL (prefix) for linking to full log file\n") ;
}