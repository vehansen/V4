#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include <windows.h>

#define VERSION "1"

#define EXITABORT EXIT_FAILURE
#define EXITOK EXIT_SUCCESS


#define TRUE 1
#define FALSE 0

typedef int DCLLOG ;
typedef int DCLINT ;

/*	Character string declarations */
typedef char DCLUC ;
#define UCEOL '\n'
#define UCEOS '\0'
#define UCsizeof sizeof
#define UCcopy(DST,SRC) strcpy_s(DST,UCsizeof(DST),SRC)
#define UCncopy(DST,SRC,LEN) strncpy_s(DST,UCsizeof(DST),SRC,LEN)
#define UCcat(DST,SRC) strcat_s(DST,UCsizeof(DST),SRC)
#define UCstrstr(TGT,STR) strstr(TGT,STR)
#define UClit(STR) STR
#define UCin(LIST,CHAR) (UCstrchr(LIST,CHAR) != NULL)
#define UCstrchr(LIST,CHAR) strchr(LIST,CHAR)
#define UCstrrchr(STR,CHAR) strrchr(STR,CHAR)
#define UCzap(STR) STR[0] = UCEOS
#define UCcmp(STR1,STR2) strcmp(STR1,STR2)
#define UCicmp(STR1,STR2) _stricmp(STR1,STR2)
#define UClen(STR) strlen(STR)
#define UCGetModuleFileName GetModuleFileNameA
#define UCWS(CHAR) ((CHAR) == UClit(' ') || (CHAR) == UClit('\t'))
#define UCMT(STR) ((STR[0]) == UCEOS)

/*	Name tables	*/
#define NAME_MAX 64
struct ntableNE {
    DCLUC name[NAME_MAX] ;
    DCLUC nameUC[NAME_MAX] ;
} ;
typedef struct ntableNE DCLNTBE ;
struct ntable {				/* Normalization table */
  DCLINT num ;				/* Current number of entry[] elements */
  DCLINT max ;				/* Max number of entry[] elements */
  DCLNTBE entry[0] ;
};
typedef struct ntable DCLNTB ;

/*	Main environment structure */
enum levelTypes { isct, list, domod, expression, comment, tally, ifmod, rptmod, enummod, macroexp } ;
typedef enum levelTypes DCLLT ;
enum tokenTypes { eof, eol, number, dim, punc, identifier, string, intmod, wspacelead, wspace, tag, comment1, com, comBegin, comCont, comEnd, unknown, vbt, macro, command } ;
typedef enum tokenTypes DCLTT ;
#define LVL_MAX 100
#define LINE_MAX 8192*2
#define TOKEN_MAX 8192

struct env {
  FILE *file ;				/* Input file */
  FILE *ofile ;				/* Output file */
  FILE *cfile ;				/* Changes file */
  DCLINT inpLineNum ;			/* Input line number */
  DCLUC inpLine[LINE_MAX] ;		/* Input line */
  DCLUC outLine[LINE_MAX] ;		/* Output line */
  DCLUC *ip ;				/* Input pointer */
  DCLTT tokenType ;			/* Current toekn type */
  DCLTT priorTokenType ;		/* Prior toekn type */
  DCLUC token[TOKEN_MAX] ;		/* Current token value */
  DCLINT wsCount ;				/* If tokenType = wpace then number of spaces */
  DCLLOG inComment ;			/* Within comment */
  DCLLOG inVBT ;			/* Within <vbt> ... </vbt> section */
  DCLLOG keepQuote ;			/* If TRUE then don't replace single quote strings with double quotes */
  DCLLOG keepEchoD ;			/* If TRUE then don't force EchoD() to LH margin */
  DCLNTB *intmods ;			/* Internal modules */
  DCLNTB *dims ;			/* Dimension names */
  DCLNTB *ids ;				/* Identifiers (NId) */
  DCLNTB *macros ;			/* Macro names */
  DCLUC lastMacro[NAME_MAX] ;		/* Last macro invoked */
  DCLNTB *tags ;			/* Tag names */
  DCLNTB *commands ;			/* Command & option names */
  DCLINT lvlX ;
  DCLINT curPL ;				/* Nesting level of '()' */
  struct {
    DCLLT lType ;
    DCLINT indent ;
    DCLINT pl ;				/* Paren level */
    DCLINT lineNum ;			/* Line number starting level */
    DCLUC macro[NAME_MAX] ;		/* If a macro level then the name */
   } lvl[LVL_MAX] ;
} ;
typedef struct env DCLENV ;

/*	TABIT - macro that outputs tabs/spaces to match leading INDENT */
#define TABIT(INDENT,ZAP,ADJUST) \
 { DCLINT i, tabs, spaces ; \
   tabs = (INDENT) >> 3 ; \
   spaces = (INDENT) & 0x7 ; if (tabs == 0 && spaces > 0) spaces -= ADJUST ; \
   if (ZAP) { UCzap(env->outLine) ; } ; \
   for(i=0;i<tabs;i++) { UCcat(env->outLine,UClit("\t")) ; } ; \
   for(i=0;i<spaces;i++) { UCcat(env->outLine,UClit(" ")) ; } ; \
 }

/*	Prototypes */
void nextToken(DCLENV *) ;
DCLUC *getV4HomePath(DCLUC *) ;

/*	ntableLoad - loads name/dictionary table & sorts it */
int ntableLoadCmp(t1,t2)
  DCLNTBE *t1,*t2 ;
{ return(UCcmp(t1->nameUC,t2->nameUC)) ; }

DCLNTB *ntableLoad(fileName)
  DCLUC *fileName ;
{
  DCLNTB *nt ;
  FILE *f ;
  DCLUC buf[2048] ;
  DCLINT lines, i ;

	if (fopen_s(&f,fileName,"r") != 0)
	 { printf("? Could not open table %s\n",fileName) ; exit(EXITABORT) ; } ;
/*	Count number of lines to know what to allocate */
	for(lines=0;;lines++)
	 { if (fgets(buf,UCsizeof(buf),f) == NULL) break ; } ;
	fclose(f) ;

	nt = malloc(sizeof *nt + lines * sizeof nt->entry[0]) ;
	nt->num = lines ; nt->max = lines ;

	if (fopen_s(&f,fileName,"r") != 0)
	 { printf("? Could not reopen table %s\n",fileName) ; exit(EXITABORT) ; } ;
	for(lines=0;;lines++)
	 { if (fgets(buf,UCsizeof(buf),f) == NULL) break ;
	   for(i=0;buf[i]>=UClit(' ');i++)
	    { nt->entry[lines].name[i] = buf[i] ; nt->entry[lines].nameUC[i] = toupper(buf[i]) ;
	    } ; nt->entry[lines].name[i] = UCEOS ; nt->entry[lines].nameUC[i] = UCEOS ;
	 } ;
	fclose(f) ;
	qsort(&nt->entry[0],(size_t)nt->num,(size_t)sizeof nt->entry[0],(_CoreCrtNonSecureSearchSortCompareFunction)ntableLoadCmp) ;

	return(nt) ;
}

/*	ntableSearch - performs binary search on sorted name/dictionary table */
DCLUC *ntableSearch(env,ntbl,str)
  DCLENV *env ;
  DCLNTB *ntbl ;
  DCLUC *str ;
{ DCLINT first, last, i, j ;
  DCLUC uc[NAME_MAX] ;

	for(i=0;;i++) { uc[i] = toupper(str[i]) ; if (uc[i] == UCEOS) break ; } ;
	for(first=0,last=ntbl->num-1;;)
	 { i = (last + first) / 2 ;  j = UCcmp(uc,ntbl->entry[i].nameUC) ;
	   if (j == 0) break ; if (j < 0) { last = i - 1 ; } else { first = i + 1 ; } ;
	   if (first > last) { i = ntbl->num + 1 ; break ; } ;
	 } ;
	return(i > ntbl->num ? NULL : ntbl->entry[i].name) ;
}

/*	Looks at string and determines indentation from leading tabs & spaces */
DCLINT indentation(line)
  DCLUC *line ;
{ DCLINT ind ; DCLLOG ok ;

	for(ind=0,ok=TRUE;ok;line++)
	 { switch(*line)
	    { default:		ind++ ;  break ;
	      case UCEOS:	ok = FALSE ; break ;
	      case UClit(' '):	ind++ ; break ;
	      case UClit('\t'):	ind = (ind & 0xfffff8) + 8 ; break ;
	    } ;
	 } ;
	return(ind) ;
}

DCLINT main(argc,argv)
 DCLINT argc ;
 char *argv[] ;
{
  DCLENV *env ;
  DCLINT ax ; DCLLOG notEOF ;

	env = malloc(sizeof *env) ; memset(env,0,sizeof *env) ;
/*	Parse the command line */
	for(ax=1;ax<argc;ax++)
	 { DCLUC *arg ;

	   arg = argv[ax] ;
	   if (arg[0] != UClit('-'))
	    { if (env->file == NULL)
	       { if(fopen_s(&env->file,arg,UClit("r")) != 0)
		  { printf("? Could not open input file: %s\n",arg) ; return ; } ;
	       } else if (env->ofile == NULL)
	        { if(fopen_s(&env->ofile,arg,UClit("w")) != 0)
		  { printf("? Could not open output file: %s\n",arg) ; return ; } ;
		} else { printf("? Input & output files already declared: %s\n",arg) ; exit(EXITABORT) ; } ;
	      continue ;
	    } ;
	   switch(arg[1])
	    { default:		printf("? Unknown switch: %s\n",arg) ; exit(EXITABORT) ;
	      case UClit('c'):
		if (ax == argc - 1) { printf("? Missing value after argument: %s\n",arg) ; exit(EXITABORT) ; } ;
		arg = argv[++ax] ;
		if (fopen_s(&env->cfile,arg,UClit("w")) != 0)
		  { printf("? Could not open changes file: %s\n",arg) ; return ; } ;
		break ;
	      case UClit('D'):
		if (ax == argc - 1) { printf("? Missing value after argument: %s\n",arg) ; exit(EXITABORT) ; } ;
		env->dims = ntableLoad(argv[++ax]) ;
		break ;
	      case UClit('e'):
		env->keepEchoD = TRUE ;
		break ;
	      case UClit('h'):
		{ FILE *hf ;
		  printf("xv4pp (V4 PrettyPrint) v%s Help\n\n",VERSION) ;
		  if(fopen_s(&hf,getV4HomePath(UClit("v4ppHelp.v4i")),UClit("r")) != 0)
		   { printf("? Could not open help file\n") ; exit(EXITABORT) ; } ;
		  for(;;)
		   { DCLUC buf[2000] ;
		     if (fgets(buf,UCsizeof(buf),hf) == NULL) break ;
		     printf(buf) ;
		   } ; fclose(hf) ;
		}
		exit(EXITOK) ;
	      case UClit('M'):
		if (ax == argc - 1) { printf("? Missing value after argument: %s\n",arg) ; exit(EXITABORT) ; } ;
		env->macros = ntableLoad(argv[++ax]) ;
		break ;
	      case UClit('N'):
		if (ax == argc - 1) { printf("? Missing value after argument: %s\n",arg) ; exit(EXITABORT) ; } ;
		env->ids = ntableLoad(argv[++ax]) ;
		break ;
	      case UClit('O'):
		env->macros = ntableLoad(UClit("v4ppmacrolist.v4i")) ;
		env->ids = ntableLoad(UClit("v4ppnidlist.v4i")) ;
		env->dims = ntableLoad(UClit("v4ppdimlist.v4i")) ;
		break ;
	      case UClit('q'):
		env->keepQuote = TRUE ;
		break ;
	    } ;

	 } ;

/*	If change file then output header */
	if (env->cfile != NULL)
	 { DCLUC cline[2000], month[4], timebuf[50] ; DCLINT i ;
	   SYSTEMTIME st ;
	   static char Months[37]=UClit("JanFebMarAprMayJunJulAugSepOctNovDec") ;
	   GetLocalTime(&st) ;
	   UCncopy(month,&Months[(st.wMonth-1)*3],3) ; month[3] = '\0' ;
	   sprintf_s(timebuf,UCsizeof(timebuf),UClit("%02d-%3s %02d:%02d:%02d"),st.wDay,month,st.wHour,st.wMinute,st.wSecond) ;
	   UCzap(cline) ; for(i=1;i<argc;i++) { UCcat(cline,argv[i]) ; UCcat(cline,UClit(" ")) ; } ;
	   fprintf(env->cfile,"\tV4 Pretty Print (v%s) Change File - %s\n\tCommand line: %s\n\n",VERSION,timebuf,cline) ;
	 } ;

/*	Load up V4 tag and intmod lists */
	env->tags = ntableLoad(getV4HomePath(UClit("v4pptaglist.v4i"))) ;
	env->intmods = ntableLoad(getV4HomePath(UClit("v4ppintmodlist.v4i"))) ;
	env->commands = ntableLoad(getV4HomePath(UClit("v4ppcmdlist.v4i"))) ;
	
	env->priorTokenType = eol ;
	for(notEOF=TRUE;notEOF;)
	 { DCLNTB *ntb ;
	   nextToken(env) ;
	   ntb = NULL ;
	   switch(env->tokenType)
	    { 
	      case eof:		notEOF = FALSE ; break ;
	      case eol:		if (env->ofile != NULL) fprintf(env->ofile,"%s\n",env->outLine) ;
				UCzap(env->outLine) ; env->priorTokenType = env->tokenType ;
				continue ;
	      case comment1:	UCcopy(env->outLine,env->token) ;
				if (env->ofile != NULL) fprintf(env->ofile,"%s",env->outLine) ;
				UCzap(env->outLine) ; env->priorTokenType = env->tokenType ;
				env->ip = NULL ;
				continue ;
	      case intmod:	ntb = env->intmods ; break ;
	      case tag:		ntb = env->tags ; break ;
	      case command:	ntb = env->commands ; break ;
	      case macro:	ntb = env->macros ; UCcopy(env->lastMacro,env->token) ; break ;
	      case identifier:	ntb = env->ids ; break ;
	      case dim:		ntb = env->dims ; break ;
	      case comBegin:	if (env->priorTokenType == wspacelead || env->priorTokenType == eol)
				 { UCcopy(env->outLine,UClit("/*")) ;
				   TABIT((env->lvlX <= 0 ? 8 : env->lvl[env->lvlX].indent),FALSE,2)
				   for(;UCWS(*env->ip);env->ip++) {} ;
				   continue ;
				 } ;
				break ;
	      case punc:	switch(env->token[0])
				 { case UClit('['):	env->lvlX ++ ; env->lvl[env->lvlX].lineNum = env->inpLineNum ;
							env->lvl[env->lvlX].lType = isct ; env->lvl[env->lvlX].indent = (env->lvlX <= 1 ? 0 : env->lvl[env->lvlX-1].indent+1) ;
							env->lvl[env->lvlX].pl = (env->lvlX <= 1 ? 0 : env->lvl[env->lvlX-1].pl) ;
							break ;
				   case UClit(']'):	if (env->lvl[env->lvlX].lType == isct) env->lvlX-- ; break ;
				   case UClit('('):	if (env->lvl[env->lvlX].lType == isct || env->lvl[env->lvlX].lType == list)
							 { DCLLOG chg=FALSE ;
							   env->lvlX++ ; env->lvl[env->lvlX].lineNum = env->inpLineNum ;
							   if (UCMT(env->lastMacro)) { env->lvl[env->lvlX].lType = list ; }
							    else { env->lvl[env->lvlX].lType = macroexp ; UCcopy(env->lvl[env->lvlX].macro,env->lastMacro) ; UCzap(env->lastMacro) ;
								 } ;
							   env->lvl[env->lvlX].pl = env->curPL ; env->lvl[env->lvlX].indent = indentation(env->outLine) + 2 ;
							   for(;UCWS(*env->ip);env->ip++) { chg = TRUE ; } ;
							   if (env->cfile != NULL && chg) fprintf(env->cfile,"%d: removed spaces after '('\n",env->inpLineNum) ;
							 } ;
							if (env->priorTokenType == intmod)		/* If intmod then strip any spaces after '(' */
							 { DCLLOG chg=FALSE ;
							   for(;UCWS(*env->ip);env->ip++) { chg = TRUE ; } ;
							   if (env->cfile != NULL && chg) fprintf(env->cfile,"%d: removed spaces after '('\n",env->inpLineNum) ;
							 } ;
							break ;
				   case UClit(')'):	if (env->lvl[env->lvlX].lType == list)
							 { if (env->priorTokenType == wspacelead)
							    { TABIT(env->lvl[env->lvlX].indent - 2,TRUE,0) ;
							    } ;
							   env->lvlX-- ;
							 } ;
							break ;
				   case UClit('<'):	if (*env->ip == UClit('/') && env->lvl[env->lvlX].lType == macroexp)
							 { DCLUC buf[NAME_MAX],save,*p ;
							   p = UCstrchr(env->ip,UClit('>')) ;
							   if (p == NULL) break ;
							   save = *p ; *p = UCEOS ; UCcopy(buf,env->ip+1) ; *p = save ;
							   if (UCicmp(buf,env->lvl[env->lvlX].macro) == 0)
							    { env->lvlX-- ; env->curPL-- ; } ;
							 } ;
							break ;
				 } ;
				break ;
	    } ;
	   if (ntb != NULL)
	    { DCLUC *fix = ntableSearch(env,ntb,env->token) ; 
	      if (fix != NULL)
	       { if (env->cfile != NULL && UCcmp(env->token,fix) != 0) fprintf(env->cfile,"%d: %s -> %s\n",env->inpLineNum,env->token,fix) ; UCcopy(env->token,fix) ; } ;
	    } ;
/*	   If leading whitespace then maybe change to handle indenting */
	   if (env->tokenType == wspacelead && env->lvlX > 0)
	    { DCLINT indent = env->lvl[env->lvlX].indent ;
/*	      If this is a line of just closing parens then may want to adjust the indentation back to match starting paren */
	      if (*env->ip == UClit(')'))
	       { switch(env->lvlX <= 0 ? -1 : env->lvl[env->lvlX].lType)
		  { default:		if (indent > 0) indent-- ; break ;
		    case -1:		break ;
		    case ifmod:		break ;
		  }
	       } ;
	      TABIT(indent,TRUE,0)
	      env->priorTokenType = env->tokenType ;
	      continue ;
	    } ;
/* CI - checks for a module and then if found sets indent (with option to round up to tab) */
#define CI(TAG,LTYPE,INDENT,TAB) \
  if (env->tokenType == intmod && UCicmp(env->token,UClit(TAG)) == 0) \
   { DCLINT indent ; \
     env->lvlX++ ; env->lvl[env->lvlX].lineNum = env->inpLineNum ; \
     env->lvl[env->lvlX].pl = env->curPL + 1 ; \
     indent = indentation(env->outLine) + INDENT ; \
     if(TAB) { if (((indent + 7) & 0xfffff8) - indent < 3) indent = (indent + 7) & 0xfffff8 ; } ; \
     env->lvl[env->lvlX].indent = indent ; \
     env->lvl[env->lvlX].lType = LTYPE ; \
   }

/*	   Check for begin of 'Do()' and other modules */
	   if (env->tokenType == intmod && UCicmp(env->token,UClit("Do")) == 0)
	    { env->lvlX++ ; env->lvl[env->lvlX].lineNum = env->inpLineNum ;
/*	      If first level (i.e. Do after isct) then set indentation to 1 */
	      if (env->lvlX == 1 && (env->priorTokenType == wspacelead || env->priorTokenType == eol))
	       { UCcopy(env->outLine,UClit(" ")) ;
	       } ;
	      env->lvl[env->lvlX].pl = env->curPL + 1 ;			/* Add one because next token will be '(' */
	      env->lvl[env->lvlX].indent = indentation(env->outLine) + UClen(env->token) + 1 ;
	      env->lvl[env->lvlX].lType = domod ;
	    } else CI("Tally",tally,3,TRUE)
	      else CI("Enum",enummod,1,FALSE)
	      else CI("If",ifmod,1,FALSE)
	      else CI("Rpt",rptmod,4,TRUE)
	      else CI("TEQ",rptmod,4,TRUE)
	      else CI("Str",rptmod,4,TRUE)
	      else CI("HTTP",rptmod,5,TRUE)
	      else CI("DB",rptmod,3,TRUE)
	      else CI("JSON",rptmod,3,TRUE)
	      else if (env->token[0] == UClit(')') && (env->lvlX <= 0 ? FALSE : env->curPL < env->lvl[env->lvlX].pl))
	       { env->lvlX-- ;
	       } ;
/*	   Have 'EchoD()' always start in second column */
	   if (!env->keepEchoD && env->priorTokenType == wspacelead && UCcmp(env->token,UClit("EchoD")) == 0)
	    { if (env->cfile != NULL && indentation(env->outLine) != 1) fprintf(env->cfile,"%d: moved EchoD() to LH margin\n",env->inpLineNum) ;
	      UCcopy(env->outLine,UClit(" ")) ;
	    } ;
	   UCcat(env->outLine,env->token) ;
	   env->priorTokenType = env->tokenType ;
	 } ;
	if (env->ofile != NULL) fclose(env->ofile) ;
	if (env->cfile != NULL) fclose(env->cfile) ;
}

DCLLOG nextLine(env)
  DCLENV *env ;
{ 
	if (fgets(env->inpLine,UCsizeof(env->inpLine),env->file) == NULL) return(FALSE) ;
	env->ip = env->inpLine ; env->inpLineNum++ ;
	return(TRUE) ;
}

void nextToken(env)
  DCLENV *env ;
{
	if (env->ip == NULL)
	 { if (!nextLine(env))
	    { UCzap(env->token) ; env->tokenType = eof ; return ; } ;
	 } ;
	if (*env->ip == UCEOL || *env->ip == UCEOS)
	 { UCzap(env->token) ; env->tokenType = eol ; env->ip = NULL ; return ; } ;

/*	In <vbt> ... </vbt> section ? */
	if (env->inVBT)
	 { if (strncmp(env->ip,UClit("</vbt>"),6) == 0) env->inVBT = FALSE ;
	   UCcopy(env->token,env->ip) ; env->tokenType = vbt ; env->ip = NULL ;
	   return ;
	 } ;

/*	Check for comments */
	if (env->inComment)
	 { DCLUC save, *end = UCstrstr(env->ip,UClit("*/")) ;
	   if (end != NULL)
	    { end += 2 ; save = *end ; *end = UCEOS ; UCcopy(env->token,env->ip) ; *end = save ;
	      env->ip = end ; env->tokenType = comEnd ; env->inComment = FALSE ;
	      return ;
	    } else
	    { UCcopy(env->token,env->ip) ; env->tokenType = comCont ; env->ip = NULL ; return ; } ;
	 } ;
	if (*env->ip == UClit('!') && env->ip == env->inpLine)
	 { DCLUC *p = env->ip ;
	   env->tokenType = comment1 ; UCcopy(env->token,env->inpLine) ; env->ip = NULL ;
/*	   If there are 1 or more spaces after the '!' then indent per current level */
	   if (UCWS(*(p+1)) && env->lvlX > 0)
	    { UCcopy(env->token,UClit("!")) ;
	      TABIT(env->lvl[env->lvlX].indent,FALSE,1)
	      for(p++;UCWS(*p);p++) { } ;
	      UCcat(env->token,p) ;
	    } ;
	   return ;
	 } ;
	if (*env->ip == UClit('/') && *(env->ip+1) == UClit('*'))
	 { UCcopy(env->token,UClit("/*")) ; env->ip += 2 ;
	   env->tokenType = comBegin ; env->inComment = TRUE ;
	   return ;
	 } ;

/*	Check for white spaces */
	if (UCWS(*env->ip))
	 { DCLINT spaces ; DCLLOG ok ; DCLUC save,*ptr ; env->tokenType = (env->ip == env->inpLine ? wspacelead : wspace) ;
	   
	   for(ptr=env->ip,ok=TRUE,spaces=0;ok;)
	    { switch(*ptr)
	       { default:		ok = FALSE ; break ;
	         case UClit(' '):	spaces++ ; ptr++ ;  break ;
		 case UClit('\t'):	spaces = (spaces & 0x7) + 8 ; ptr++ ;  break ;
	       } ;
	    } ;
	   env->wsCount = spaces ;
	   save = *ptr ; *ptr = UCEOS ; UCcopy(env->token,env->ip) ; *ptr = save ;  env->ip = ptr ;
	   return ;
	 } ;

/*	Check for numeric literal */
	if (UCin(UClit("^0123456789"),*env->ip))
	 { DCLUC save, *ptr, *list = UClit("0123456789._") ;
	   ptr = env->ip ;
	   if (*ptr == UClit('^'))
	    { switch(*(++ptr))
	       { case UClit('B'): case UClit('b'):	ptr++ ; list = UClit("01") ; break ;
	         case UClit('O'): case UClit('o'):	ptr++ ; list = UClit("01234567") ; break ;
		 case UClit('X'): case UClit('x'):	ptr++ ; list = UClit("01234567890AaBbCcDdEeFf") ; break ;
	       } ;
	      for(;UCin(list,*ptr);ptr++) {} ;
	      save = *ptr ; *ptr = UCEOS ;
	      UCcopy(env->token,env->ip) ; *ptr = save ; env->ip = ptr ;
	      env->tokenType = number ; return ;
	    } ;
	 } ;
/*	Check for string literal & maybe replace single with double */
	if (!env->keepQuote && (*env->ip == UClit('"') || *env->ip == UClit('\'')))
	 { DCLUC *ptr, *tkn ; DCLLOG dblQ ; DCLLOG ok ;
/*	   Want to convert single quote literal to double quote */
	   dblQ = (*env->ip == UClit('"')) ;
	   if (!dblQ && env->cfile != NULL) fprintf(env->cfile,"%d: string changed to double quote\n",env->inpLineNum) ;
	   env->token[0] = UClit('"') ; tkn = &env->token[1] ;
	   for(ok=TRUE,ptr = (env->ip + 1);ok && *ptr!=UCEOS;ptr++)
	    { switch(*ptr)
	       { default:		*tkn++ = *ptr ; break ;
	         case UClit('\\'):	*tkn++ = *ptr++ ; *tkn++ = *ptr ; break ;
		 case UClit('"'):	if (dblQ) { ok = FALSE ; break ; } else { *tkn++ = UClit('\\') ; *tkn++ = *ptr ; } ; break ;
		 case UClit('\''):	if (!dblQ) { ok = FALSE ; } else { *tkn++ = *ptr ; } ; break ;
	       } ;
	    } ;
	   env->ip = ptr ; *tkn++ = UClit('\"') ; *tkn = UCEOS ; env->tokenType = string ; return ;
	 } ;
	if (env->keepQuote && (*env->ip == UClit('"') || *env->ip == UClit('\'')))
	 { DCLUC *ptr, *tkn, q ; DCLLOG ok ;
	   q = *env->ip ;
	   env->token[0] = q ; tkn = &env->token[1] ;
	   for(ok=TRUE,ptr = (env->ip + 1);ok && *ptr!=UCEOS;ptr++)
	    { if (*ptr == '\\') { *tkn++ = *ptr++ ; *tkn++ = *ptr ; }
	       else if (*ptr == q) { *tkn++ = *ptr ; ok = FALSE ; }
	       else { *tkn++ = *ptr ; } ;
	    } ;
	   env->ip = ptr ; *tkn = UCEOS ; env->tokenType = string ; return ;
	 } ;
/*	Check for alpha-numeric identifier */
	if (UCin(UClit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_$"),*env->ip))
	 { DCLUC save, *ptr, *list = UClit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_$0123456789") ;
	   for(ptr=env->ip;*ptr != UCEOS && UCin(list,*ptr);ptr++) { } ;
	   save = *ptr ; *ptr = UCEOS ; UCcopy(env->token,env->ip) ; *ptr = save ; env->ip = ptr ;
/*	   Check next characters to determine what kind of Id we have */
	   switch(*ptr)
	    { default:			env->tokenType = (env->lvlX == 0 && (env->priorTokenType == wspacelead || env->priorTokenType == eol) ? command : identifier) ; return ;
	      case UClit(':'):		env->tokenType = (*(ptr+1) == ':' ? tag : dim) ; return ;
	      case UClit('('):		env->tokenType = (env->lvlX == 0 ? macro : (ntableSearch(env,env->intmods,env->token) != NULL ? intmod : macro)) ;
					return ;
	      case UClit('?'):		env->tokenType = tag ; return ;
	      case UClit('+'):
	      case UClit('.'):
	      case UClit('*'):
	      case UClit('~'):		env->tokenType = dim ; return ;
	    } ;
	 } ;

	if (*env->ip== UClit('('))
	 { env->curPL++ ; }
	 else if (*env->ip == UClit(')'))
	  { env->curPL-- ; } ;
/*	Check for ':=' and remove before & after spaces */
	if (*env->ip == UClit(':') && *(env->ip+1) == UClit('='))
	 { DCLINT i ; DCLLOG chg = FALSE ;
	   for(i=UClen(env->outLine)-1;UCWS(env->outLine[i]) && i >= 0;i--)
	    { env->outLine[i] = UCEOS ; chg = TRUE ; } ;
	   for(env->ip+=2;UCWS(*env->ip);env->ip++) { chg = TRUE ;  } ;
	   if (env->cfile != NULL && chg)
	    fprintf(env->cfile,"%d: Removed spacing around ':='\n",env->inpLineNum) ;
	   UCcopy(env->token,UClit(":=")) ; env->tokenType = punc ;
	   return ;
	 } ;
	switch(*env->ip)
	 { 
	   case UCEOL:			env->tokenType = eol ; env->ip = NULL ; return ;
	   case UClit('<'):		if (strncmp(env->ip,UClit("<vbt>"),5) == 0)
					 { UCcopy(env->token,UClit("<vbt>")) ; env->ip += 5 ; env->tokenType = vbt ; env->inVBT = TRUE ;
					   return ;
					 } ;
	   case UClit(':'):
	   case UClit('!'):		
	   case UClit('#'):
	   case UClit('&'):
	   case UClit('|'):
	   case UClit('~'):
	   case UClit('`'):
	   case UClit('?'):
	   case UClit('@'):
	   case UClit('('):
	   case UClit(')'):
	   case UClit('*'):
	   case UClit('+'):
	   case UClit(','):
	   case UClit('-'):
	   case UClit('.'):
	   case UClit('/'):
	   case UClit('\\'):
	   case UClit(';'):
	   case UClit('>'):
	   case UClit('='):
	   case UClit('['):
	   case UClit(']'):
	   case UClit('{'):
	   case UClit('}'):
	   case UClit('%'):
	   default:			env->token[0] = *env->ip ; env->token[1] = UCEOS ; env->ip++ ; env->tokenType = punc ; return ;
	 } ;
	UCzap(env->token) ; env->tokenType = unknown ; return ;
}

/*	v_GetV4HomePath - Returns path to V4 Home Directory		*/
/*	Call: path = v_GetV4HomePath( file )
	  where path is string path name to file (includes file),
		file is string name of file				*/

DCLUC *getV4HomePath(file)
  DCLUC *file ;
{ DCLUC xv4Path[2000], *b ;
  static DCLUC respath[2000] ;

	UCzap(respath) ;
	if (UCGetModuleFileName(NULL,xv4Path,UCsizeof(xv4Path)))
	 { b = UCstrrchr(xv4Path,UClit('\\')) ;		/* Search for right-most backslash */
	   if (b != NULL) { *(b+1) = UCEOS ; UCcopy(respath,xv4Path) ; } ;
	 } ;
	UCcat(respath,file) ;
	return(respath) ;
}
