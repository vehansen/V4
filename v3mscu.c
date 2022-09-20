/*	V3MCSU.C - MISCELLANEOUS ROUTINES FOR VICTIM III

	LAST EDITED 7/23/84 BY VICTOR E. HANSEN		*/

#include <time.h>
#include "v3defs.c"
#ifdef HPUX
#include <utmp.h>
#endif

/*	External linkages				*/
extern struct db__parse_info *parse ;
extern struct db__process *process ;
extern struct db__psi *psi ;

extern short int julian[] ;
extern short int julian_leap[] ;

/*	D A T E   M O D U L E S					*/

/*	mscu_date_info - Updates date table with info about a date  */
/*	If date__table not passed as argument, then pop off of V3 stack */

void mscu_date_info(dt)
  struct v3__date_table *dt ;
{  union val__format format ;
   int ud,id,idt,century,day,month,year ;

/*	First see what we got */
	if (dt == NULL)
	 { POPF(format.all) ; POPVP(dt,(struct v3__date_table *)) ;
/*   	   Make sure we are in sync with caller */
	   if (format.fld.length != sizeof *dt)
	    v3_error(V3E_INVDISIZE,"DATE","INFO","INVDISIZE","Argument to \'date_info\' module is incorrect length",(void *)sizeof *dt) ;
	 } ;
/*	Which input argument was set ? */
	if (dt->inp_universal != 0)
	 { if (dt->inp_universal < 0 || dt->inp_universal > 70200)
	    v3_error(V3E_INVUD,"DATE","INFO","INVUD","Invalid universal date",(void *)dt->inp_universal) ;
	   year = ((dt->inp_universal+678955)*4)/1460 ;
	   for (;;)
	    { ud = mscu_ymd_to_ud(year,1,1) ;
	      if (ud > dt->inp_universal) { year-- ; continue ; } ;
	      if (dt->inp_universal - ud < ((year % 4) == 0 ? 366 : 365)) break ;
	      year-- ;
	    } ;
	   id = year * 1000 + dt->inp_universal - ud + 1 ; goto julian_entry ;
	 } ;
	if (dt->inp_date_time != 0)
	 { idt = dt->inp_date_time / 86400 + 44240 ;
	   if (idt < 0 || idt > 70200)
	    v3_error(V3E_INVDTD,"DATE","INFO","INVDTD","Invalid universal date in date_time",(void *)idt) ;
	   year = ((idt+678955)*4)/1460 ;
	   for (;;)
	    { ud = mscu_ymd_to_ud(year,1,1) ;
	      if (ud > idt) { year-- ; continue ; } ;
	      if (idt - ud < ((year % 4) == 0 ? 366 : 365)) break ;
	      year-- ;
	    } ;
	   id = year * 1000 + idt - ud + 1 ; goto julian_entry ;
	 } ;
	if (dt->inp_julian != 0)
	 { id = dt->inp_julian ;
julian_entry:
	   year = id/1000 ; id %= 1000 ;
	   if ((year % 4) == 0)
	    { for (month=1;month<12;month++)
	       { if (id <= julian_leap[month]) break ; } ;
	      day = id-julian_leap[month-1] ; goto got_mdy ;
	    } else
	    { for (month=1;month<12;month++)
	       { if (id <= julian[month]) break ; } ;
	      day = id-julian[month-1] ; goto got_mdy ;
	    } ;
	 } ;
	if (dt->inp_mmddyy)
	 { month = dt->inp_mmddyy/10000 ; day = (dt->inp_mmddyy/100) % 100 ;
	   year = dt->inp_mmddyy % 100 ; goto got_mdy ;
	 } ;
	if (dt->inp_yymmdd)
	 { year = dt->inp_yymmdd/10000 ; month = (dt->inp_yymmdd/100) % 100 ;
	   day = dt->inp_yymmdd % 100 ; goto got_mdy ;
	 } ;
	if (dt->inp_umonth)
	 { year = 1850 + dt->inp_umonth/12 ; month = (dt->inp_umonth % 12) + 1 ; day = 1 ;
	   goto got_mdy ;
	 } ;
	v3_error(V3E_NODATE,"DATE","INFO","NODATE","No date given in any of the \'inp_xxx\' elements",0) ;
/*	Got everything we need - start filling in junk */
got_mdy:
	if (year < 100) { year += 2000 ; } ;
	dt->universal = mscu_ymd_to_ud(year,month,day) ;
	dt->umonth = (year-1850)*12 + month - 1 ;
	if ((dt->date_time = (dt->universal - 44240) * 86400) < 0) dt->date_time = 0 ;
	century = year/100 ; year %=100 ;
	dt->mmddyy = month*10000 + day*100 + year ;
	dt->yymmdd = year*10000 + month*100 + day ;
	dt->month = month ; dt->year = year ; dt->day = day ; dt->century = century ;
/*	Now may twiddle day for julian stuff */
	if ((year % 4) == 0 && month > 2) day++ ;
	dt->julian = year*1000 + julian[month-1] + day ;
	dt->day_of_year = julian[month-1] + day ;
	dt->days_in_year = (year % 4 == 0 ? 365 : 364) ;
	dt->day_of_week = ((dt->universal+1) % 7)+1 ;
/*	That's about it */
	return ;
}

/*	V I C T I M   F L A G   L O O K U P   R O U T I N E	*/

/*	Flag table	*/

struct {
  char name[V3_FLAG_NAME_MAX+1] ;
  int value ;
 } flag_table[] =
    {	{"AFTER",V3_FLAGS_HAN_AFTER},
        {"ALL_BPS",0},
	{"ALL_MODULES",0},
	{"AM_PM",V3_FLAGS_EDT_AM_PM},
	{"APPEND",V3_FLAGS_IOF_APPEND},
	{"ASSERTION_CHANGES",V3_FLAGS_V3_ASSERTION_CHANGES},
	{"ASTERISK_COUNT",V3_FLAGS_V3_ASTERISK_COUNT},
	{"ASTERISK_FILL",V3_FLAGS_EDT_ASTERISK_FILL},
	{"BEFORE",V3_FLAGS_HAN_BEFORE},
	{"BEGINS",V3_FLAGS_STR_BEGINS},
	{"BLANK_IF_ZERO",V3_FLAGS_EDT_BLANK_IF_ZERO},
	{"BLINK",V3_FLAGS_SCR_BLINK},
	{"BLOCKING",V3_FLAGS_IOM_BLOCKING},
	{"BOF",V3_FLAGS_IOPG_BOF},
	{"BOLD",V3_FLAGS_SCR_BOLD},
	{"BOX",V3_FLAGS_SYS_BOX},
	{"C",V3_FLAGS_IOF_C},
	{"CACHE",V3_FLAGS_IOPG_CACHE},
	{"CARRIAGE_RETURN",V3_FLAGS_IOR_CARRIAGE_RETURN},
	{"CENTER",V3_FLAGS_EDT_CENTER},
	{"CENTURY",V3_FLAGS_EDT_CENTURY},
	{"CHECKPOINT",V3_FLAGS_IOM_CHECKPOINT},
	{"CHECKSUM_TABLE",V3_FLAGS_V3_CHECKSUM_TABLE},
	{"CLEAR_AFTER_FIRST",V3_FLAGS_TTY_CLEAR_AFTER_FIRST},
	{"CLIENT",V3_FLAGS_IOF_CLIENT},
	{"COLON_KW",V4LEX_RPPFlag_ColonKW},
	{"COMMAND_LINE",V3_FLAGS_SYS_COMMAND_LINE},
	{"COMMAS",V3_FLAGS_EDT_COMMAS},
	{"COMPARE",V3_FLAGS_STR_COMPARE},
	{"COMPRESSED",V3_FLAGS_IOR_COMPRESSED},
	{"CONCAT",V3_FLAGS_EDT_CONCAT},
	{"COUNTRY_INFO",V3_FLAGS_V3_COUNTRY_INFO},
	{"CPU_TIME",V3_FLAGS_SYS_CPU_TIME},
	{"CREATE",V3_FLAGS_IOF_CREATE},
	{"CREATE_IF",V3_FLAGS_IOF_CREATE_IF},
	{"CURRENT_MODULE",0},
	{"CURRENT_PACKAGE",V3_FLAGS_V3_CURRENT_PACKAGE},
	{"DASH_IF_ZERO",V3_FLAGS_EDT_DASH_IF_ZERO},
	{"DATA_ONLY",V3_FLAGS_IOPG_DATAONLY},
	{"DATA_TYPE",V3_FLAGS_V3_DATA_TYPE},
	{"DATE",V3_FLAGS_SYS_DATE},
	{"DATE_TIME",V3_FLAGS_EDT_DATE_TIME},
	{"DDMMMYY",V3_FLAGS_EDT_DDMMMYY},
	{"DDMMYY",V3_FLAGS_EDT_DDMMYY},
	{"DECIMAL_PLACES",V3_FLAGS_V3_DECIMAL_PLACES},
	{"DELETE",V3_FLAGS_IOF_DELETE},
	{"DELETE_FILE",V3_FLAGS_IOF_DELETE_FILE},
	{"DIRECTORY_PATH",V3_FLAGS_SYS_DIRECTORY_PATH},
	{"DISCONNECTONCLOSE",V3_FLAGS_IOF_DISCONNECTonCLOSE},
	{"DISK_AVAILABLE",V3_FLAGS_SYS_DISK_AVAILABLE},
	{"DISK_USAGE",V3_FLAGS_SYS_DISK_USAGE},
	{"DOUBLE_HEIGHT",V3_FLAGS_SCR_DOUBLE_HEIGHT},
	{"DOUBLE_WIDTH",V3_FLAGS_SCR_DOUBLE_WIDTH},
	{"DT_ALPHA",VFT_FIXSTR},
	{"DT_DECIMAL",VFT_STRINT},
	{"DT_FLOAT",VFT_FLOAT},
	{"DT_INT",VFT_BININT},
	{"DT_LONG",VFT_BINLONG},
	{"DT_OBJECT",VFT_OBJECT},
	{"DT_OBJREF",VFT_OBJREF},
	{"DT_PACKED",VFT_PACDEC},
	{"DT_SHORT",VFT_BINWORD},
	{"DT_STRING",VFT_VARSTR},
	{"ENDOFLIST",V3_FLAGS_HAN_ENDOFLIST},
	{"ENVIRONMENT",V3_FLAGS_SYS_ENVIRONMENT},
	{"EOF",V3_FLAGS_IOPG_EOF},
	{"EOL_TERM",V4LEX_RPPFlag_EOL},
	{"ERROR",V3_FLAGS_V3_ERROR},
	{"ERROR_IF_LOADED",0},
	{"EXACT",V3_FLAGS_STR_EXACT},
	{"EXECUTABLE",V3_FLAGS_IOM_EXECUTABLE},
	{"EXISTS",V3_FLAGS_IOF_EXISTS},
	{"EXTEND",V3_FLAGS_EDT_EXTEND},
	{"FALSE",FALSE},
	{"FILEBINARY",V3_FLAGS_IOPG_FILEBINARY},
	{"FILETEXT",V3_FLAGS_IOPG_FILETEXT},
	{"FILL_BINARY",V3_FLAGS_IOM_FILL_BUF_BINARY},
	{"FILL_TEXT",V3_FLAGS_IOM_FILL_BUF_TEXT},
	{"FIXED_LENGTH",V3_FLAGS_IOR_FIXED_LENGTH},
	{"FLOATING_DOLLAR",V3_FLAGS_EDT_FLOATING_DOLLAR},
	{"FLUSH",V3_FLAGS_TTY_FLUSH},
	{"FORCE_KW",V3_FLAGS_TKN_FORCE_KW},
	{"FORCE_NEW_LINE",V3_FLAGS_TKN_FORCE_NEW_LINE},
	{"FORK_LEAVE",V3_FLAGS_MOD_FORK_LEAVE},
	{"FORK_REENTER",V3_FLAGS_MOD_FORK_REENTER},
	{"FORTRAN",V3_FLAGS_IOR_FORTRAN},
	{"FREEMEMORY",V3_FLAGS_HAN_FREEMEMORY},
	{"GET",V3_FLAGS_IOF_GET},
	{"GET_BUF",V3_FLAGS_IOM_GET_BUF},
	{"GET_MODE",V3_FLAGS_IOM_GET_MODE},
	{"HANDLER_MODULE",V3_FLAGS_IOM_HANDLER_MODULE},
	{"HANDLE_DELETE",V3_FLAGS_MOD_HANDLE_DELETE},
	{"HASHED",V3_FLAGS_IOF_HASHED},
	{"HEAP_FREE",V3_FLAGS_MOD_HEAP_FREE},
	{"HEAP_FREE_LIST",V3_FLAGS_MOD_HEAP_FREE_LIST},
	{"HEX",V3_FLAGS_EDT_HEX},
	{"HISTORY",V3_FLAGS_EDT_HISTORY},
	{"IGNORE_LOCK",V3_FLAGS_IOPG_IGNORE_LOCK},
	{"IMPLY_AND",V4LEX_RPPFlag_ImplyAnd},
	{"INDEXED",V3_FLAGS_IOF_INDEXED},
	{"INETSOCKET",V3_FLAGS_IOF_INETSOCKET},
	{"INFO",V3_FLAGS_HAN_INFO},
	{"INITIALIZE",V3_FLAGS_IOM_INIT},
	{"INT_BROADCAST_RCV",V3_FLAGS_INT_BROADCAST_RCV},
	{"INT_CTRLC",V3_FLAGS_INT_CTRLC},
	{"INT_CTRLP",V3_FLAGS_INT_CTRLP},
	{"INT_CTRLY",V3_FLAGS_INT_CTRLY},
	{"INT_ERROR",V3_FLAGS_INT_ERROR},
	{"INT_MAILBOX_RCV",V3_FLAGS_INT_MAILBOX_RCV},
	{"INT_TIMER",V3_FLAGS_INT_TIMER},
	{"IO_COUNT",V3_FLAGS_SYS_IO_COUNT},
	{"IS_ACTIVE_USER",V3_FLAGS_SYS_IS_ACTIVE_USER},
	{"IS_BATCH_JOB",V3_FLAGS_SYS_IS_BATCH_JOB},
	{"IS_GUI",V3_FLAGS_SYS_IS_GUI},
	{"IS_OPEN",V3_FLAGS_IOM_IS_OPEN},
	{"JOBID",V3_FLAGS_V3_SET_JOBID},
	{"JULIAN",V3_FLAGS_EDT_JULIAN},
	{"KEYED",V3_FLAGS_IOPG_KEYED},
	{"KEYWORD",V3_FLAGS_STR_KEYWORD},
	{"LEFT_JUSTIFY",V3_FLAGS_EDT_LEFT_JUSTIFY},
	{"LINK",V3_FLAGS_HAN_LINK},
	{"LOCK_WAIT",V3_FLAGS_IOPG_LOCK_WAIT},
	{"LONG_DATE",V3_FLAGS_EDT_LONG_DATE},
	{"LOWER_CASE",V3_FLAGS_EDT_LOWER_CASE},
	{"MANUAL_LOCK",V3_FLAGS_IOM_MANUAL_LOCK},
	{"MANUAL_LOCKWAIT",V3_FLAGS_IOM_MANUAL_LOCKWAIT},
	{"MANUAL_UNLOCK",V3_FLAGS_IOM_MANUAL_UNLOCK},
	{"MAX_LENGTH",V3_FLAGS_V3_MAX_LENGTH},
	{"MDY",V3_FLAGS_EDT_MDY},
	{"MMDDYY",V3_FLAGS_EDT_MMDDYY},
	{"MODULE_EXIT",V3_FLAGS_MOD_MODULE_EXIT},
	{"NEW_EOF",V3_FLAGS_IOPG_NEW_EOF},
	{"NEW_FILE",V3_FLAGS_IOF_NEW_FILE},
	{"NEW_RECORD",V3_FLAGS_IOPG_NEW_RECORD},
	{"NEXT",V3_FLAGS_IOPG_NEXT},
	{"NEXTNUM1",V3_FLAGS_IOPG_NEXTNUM1},
	{"NEXT_FILE",V3_FLAGS_SYS_NEXT_FILE},
	{"NOBLOCK", V3_FLAGS_IOF_NOBLOCK},
	{"NOCASE",V3_FLAGS_STR_NOCASE},
	{"NOECHO",V3_FLAGS_TTY_NOECHO},
	{"NOERROR",V3_FLAGS_EDT_NOERROR},
	{"NOERROR_IF_LOADED",1},
	{"NOHANDLER",V3_FLAGS_IOF_NOHANDLER},
	{"NOLOCK",V3_FLAGS_IOPG_NOLOCK},
	{"NOPAD",V3_FLAGS_EDT_NOPAD},
	{"NORMALIZE",V3_FLAGS_EDT_NORMALIZE},
	{"NOSYNONYMS",V3_FLAGS_STR_NOSYN},
	{"NOYEAR",V3_FLAGS_EDT_NOYEAR},
	{"OBJECT_PACKAGE",V3_FLAGS_V3_OBJECT_PACKAGE},
	{"OBSOLETE",V3_FLAGS_IOPG_OBSOLETE},
	{"OCTAL",V3_FLAGS_EDT_OCTAL},
	{"ONE_DATE_TIME_DAY",V3_FLAGS_EDT_ONE_DATE_TIME_DAY},
	{"OPEN_FILES",V3_FLAGS_V3_OPEN_FILES},
	{"OS",V3_FLAGS_SYS_OS},
	{"OS_ID",V3_FLAGS_SYS_OS_ID},
	{"OVERPUNCH",V3_FLAGS_EDT_OVERPUNCH},
	{"PACKAGE_FILENAME",V3_FLAGS_V3_PACKAGE_FILENAME},
	{"PACKAGE_ID",V3_FLAGS_V3_PACKAGE_ID},
	{"PACKAGE_LOADED",V3_FLAGS_V3_PACKAGE_LOADED},
	{"PACKAGE_NAME",V3_FLAGS_V3_PACKAGE_NAME},
	{"PARENT",V3_FLAGS_HAN_PARENT},
	{"PAREN_IF_NEG",V3_FLAGS_EDT_PAREN_IF_NEG},
	{"PARSE_ERRORS",V3_FLAGS_V3_PARSE_ERRORS},
	{"PARTIAL",V3_FLAGS_IOPG_PARTIAL},
	{"PASSALL",V3_FLAGS_TTY_PASSALL},
	{"PATTERN",V3_FLAGS_STR_PATTERN},
	{"PEERNAME",V3_FLAGS_IOM_PEERNAME},
	{"POINTER",V3_FLAGS_HAN_POINTER},
	{"POSITION",V3_FLAGS_IOPG_POSITION},
	{"POSITIVE",V3_FLAGS_EDT_POSITIVE},
	{"PRAGMA_GET_SEQ",V3_FLAGS_IOPRAG_GET_SEQ},
	{"PRAGMA_PUT_MASS",V3_FLAGS_IOPRAG_PUT_MASS},
	{"PRAGMA_PUT_SEQ",V3_FLAGS_IOPRAG_PUT_SEQ},
	{"PROCESS_ID",V3_FLAGS_SYS_PROCESS_ID},
	{"PROCESS_SIZE",V3_FLAGS_SYS_PROCESS_SIZE},
	{"PROMPT",V3_FLAGS_TKN_PROMPT},
	{"PUNCTUATION",V3_FLAGS_EDT_PUNCTUATION},
	{"PUSH_CUR",V3_FLAGS_TKN_PUSH_CUR},
	{"PUSH_FILE",V3_FLAGS_TKN_PUSH_FILE},
	{"PUSH_PARENS",V4LEX_RPPFlag_PushParens},
	{"PUSH_STDIN",V3_FLAGS_TKN_PUSH_STDIN},
	{"PUSH_STRING",V3_FLAGS_TKN_PUSH_STRING},
	{"PUT",V3_FLAGS_IOF_PUT},
	{"PUT_BUF",V3_FLAGS_IOM_PUT_BUF},
	{"PUT_MODE",V3_FLAGS_IOM_PUT_MODE},
	{"QUESTION_MARK",V3_FLAGS_TTY_QUESTION_MARK},
	{"QUOTED",V3_FLAGS_EDT_QUOTED},
	{"REAL_DATE",V3_FLAGS_SYS_REAL_DATE},
	{"REAL_TIME_OF_DAY",V3_FLAGS_SYS_REAL_TIME_OF_DAY},
	{"RECONNECT",V3_FLAGS_IOM_RECONNECT},
	{"RELATIVE",V3_FLAGS_IOF_RELATIVE},
	{"RELOCK",V3_FLAGS_IOPG_RELOCK},
	{"RESET",V3_FLAGS_IOPG_RESET},
	{"RET_EOL",V3_FLAGS_TKN_RET_EOL},
	{"REVERSE_VIDEO",V3_FLAGS_SCR_REVERSE_VIDEO},
	{"RIGHT_HAND_SIGN",V3_FLAGS_EDT_RIGHT_HAND_SIGN},
	{"RIGHT_JUSTIFY",V3_FLAGS_EDT_RIGHT_JUSTIFY},
	{"RPTCAT",0xFF000000},
	{"SCAN",V3_FLAGS_STR_SCAN},
	{"SCORE",V3_FLAGS_SCR_UNDER_LINE},
	{"SECONDS",V3_FLAGS_EDT_SECONDS},
	{"SEMI_TERM",V4LEX_RPPFlag_Semi},
	{"SEQUENTIAL",V3_FLAGS_IOF_SEQUENTIAL},
	{"SEQUENTIAL_INPUT",V3_FLAGS_IOM_SEQ_INPUT},
	{"SEQUENTIAL_OUTPUT",V3_FLAGS_IOM_SEQ_OUTPUT},
	{"SERVER",V3_FLAGS_IOF_SERVER},
	{"SET_DEVICE",V3_FLAGS_TTY_SET_DEVICE},
	{"SET_KEYS",V3_FLAGS_IOM_SET_KEYS},
	{"SET_KEYSV",V3_FLAGS_IOM_SET_KEYSV},
	{"SOS",V3_FLAGS_IOR_SOS},
	{"STARTUP_OPTIONS",V3_FLAGS_V3_STARTUP_OPTIONS},
	{"TABLE_NUMBER",4},
	{"TABLE_TEXT",2},
	{"TERMINAL_NAME",V3_FLAGS_SYS_TERMINAL_NAME},
	{"TEXT_INPUT",V3_FLAGS_IOM_TEXT_INPUT},
	{"TEXT_OUTPUT",V3_FLAGS_IOM_TEXT_OUTPUT},
	{"TIME_OF_DAY",V3_FLAGS_SYS_TIME_OF_DAY},
	{"TOP_OF_PAGE",200},
	{"TRIM_LEFT",V3_FLAGS_EDT_TRIM_LEFT},
	{"TRIM_RIGHT",V3_FLAGS_EDT_TRIM_RIGHT},
	{"TRUE",TRUE},
	{"TRUE_FALSE",V3_FLAGS_EDT_TRUE_FALSE},
	{"TRUNCATE",V3_FLAGS_TTY_TRUNCATE},
	{"TYPE",V3_FLAGS_HAN_TYPE},
	{"TYPEAHEAD_COUNT",V3_FLAGS_TTY_TYPEAHEAD_COUNT},
	{"UNDEFINED_MODULE",V3_FLAGS_V3_SET_UNDEF_MODULE},
	{"UNDERLINE",V3_FLAGS_SCR_UNDER_LINE},
	{"UNIT_CLOSE",V3_FLAGS_MOD_UNIT_CLOSE},
	{"UNIVERSAL",V3_FLAGS_EDT_UNIVERSAL},
	{"UNIXSOCKET",V3_FLAGS_IOF_UNIXSOCKET},
	{"UNLOCK",V3_FLAGS_IOM_UNLOCK},
	{"UNTIL_DELIMITER",V3_FLAGS_EDT_UNTIL_DELIMITER},
	{"UPDATE",V3_FLAGS_IOF_UPDATE},
	{"UPDATE_EXISTING",V3_FLAGS_IOPG_UPDATE_EXISTING},
	{"UPPER_CASE",V3_FLAGS_EDT_UPPER_CASE},
	{"UPPER_CASE_ALT",V3_FLAGS_EDT_UPPER_CASE_ALT},
	{"USER_ID",V3_FLAGS_SYS_USER_ID},
	{"USER_NAME",V3_FLAGS_SYS_USER_NAME},
	{"V4IS",V3_FLAGS_IOF_V4IS},
	{"VARIABLE_LENGTH",V3_FLAGS_IOR_VARIABLE_LENGTH},
	{"VERSION",V3_FLAGS_V3_VERSION},
	{"VMSRFA",V3_FLAGS_IOPG_VMSRFA},
	{"WATCH_LOCATION",V3_FLAGS_V3_SET_WATCH_LOCATION},
	{"X_FALSE",V3_FLAGS_EDT_X_FALSE},
	{"X_TRUE",V3_FLAGS_EDT_X_TRUE},
	{"YES_NO",V3_FLAGS_EDT_YES_NO},
	{"YYMMDD",V3_FLAGS_EDT_YYMMDD},
	{"ZERO_FILL",V3_FLAGS_EDT_ZERO_FILL},
	{"ZERO_IF_ERROR",V3_FLAGS_EDT_ZERO_IF_ERROR},
    } ;

#define MSC_FLAG_TABLE_COUNT 249

/*	mscu_flag_lookup - Does lookup on internal flags & returns flag value */

int mscu_flag_lookup(flag)
  char *flag ;
{ int i=0,j=MSC_FLAG_TABLE_COUNT,k,cr ;

/*	Perform binary search on the table */
	for (;;)
	 { if (i > j)
	    { for(i=parse->user_flag_count-1;i>=0;i--)
	       { if (strcmp(parse->user_flags[i].name,flag) == 0) return(parse->user_flags[i].value) ; } ;
	      if (!prs_attempt_struct_load(flag))
	       prsu_error(ERRP,"UNDEFFLAG","Undefined \'/xxx/\' symbol/flag") ;
	      prsu_nxt_token(FALSE) ; prsu_dcl(FALSE) ;
	      for(i=parse->user_flag_count-1;i>=0;i--)
	       { if (strcmp(parse->user_flags[i].name,flag) == 0) return(parse->user_flags[i].value) ; } ;
/*	      Flag not defined in V3 or user tables */
	      prsu_error(ERRP,"UNDEFFLAG","Undefined \'/xxx/\' symbol/flag") ;
	    } ;
	   k = (i+j)/2 ; cr = strcmp(flag,flag_table[k].name) ;
	   switch (cr < 0 ? 1 : (cr > 0 ? 2 : cr))
	    { case 1:	/* Symbol before current table entry */
		j = k-1 ; break ;
	      case 0:	/* Got a match */
		return(flag_table[k].value) ;
	      case 2:	/* Symbol after current entry */
		i = k+1 ; break ;
	    } ;
	 } ;
}

/*	V E C T O R   O P E R A T I O N S			*/

/*	Define some common pointers, etc. */


/*	vec_add - Adds two vectors of identical type */

void vec_add()
{ int i ;
  B64INT *lptr1,*lptr2 ;
  int ival,*iptr1,*iptr2,len ;
  union val__format format ;
  char bval,*bptr1,*bptr2 ;
  short int wval,*wptr1,*wptr2 ;

	i = xctu_popint() ;	/* Get length */
	POPF(format.all) ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","ADD","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BINLONG:
		POPVP(lptr2,(B64INT *)) ; POPF(format.all) ; POPVP(lptr1,(B64INT *)) ;
		for(;i>0;i--) { *(lptr1++) += *(lptr2++) ; } ;
		break ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ; POPF(format.all) ; POPVP(iptr1,(int *)) ;
		for(;i>0;i--) { *(iptr1++) += *(iptr2++) ; } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ; POPF(format.all) ; POPVP(wptr1,(short int *)) ;
		for(;i>0;i--) { *(wptr1++) += *(wptr2++) ; } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; POPF(format.all) ; POPVP(bptr1,(char *)) ;
		for(;i>0;i--) { *(bptr1++) += *(bptr2++) ; } ;
		break ;
	 } ;
	return ;
}

/*	vec_add_constant - Adds constant value to vector */

void vec_add_constant()
{ int i ;
  B64INT *lptr1,*lptr2 ;
  int ival,*iptr1,*iptr2,len ;
  union val__format format ;
  char bval,*bptr1,*bptr2 ;
  short int wval,*wptr1,*wptr2 ;

	i = xctu_popint() ;	/* Get length */
	POPF(format.all) ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","ADD","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BINLONG:
		POPVP(lptr2,(B64INT *)) ; ival = xctu_popint() ;
		for(;i>0;i--) { *(lptr2++) += ival ; } ;
		break ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ; ival = xctu_popint() ;
		for(;i>0;i--) { *(iptr2++) += ival ; } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ; wval = xctu_popint() ;
		for(;i>0;i--) { *(wptr2++) += wval ; } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; bval = xctu_popint() ;
		for(;i>0;i--) { *(bptr2++) += bval ; } ;
		break ;
	 } ;
	return ;
}

/*	vec_copy - Copies one vector over another (with conversions) */

void vec_copy()
{ int i ;
  int ival,*iptr1,*iptr2,len ;
  int lval,*lptr1,*lptr2 ;
  union val__format format ;
  char bval,*bptr1,*bptr2 ;
  short int wval,*wptr1,*wptr2 ;

	i = xctu_popint() ;	/* Get length */
	POPF(format.all) ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","ADD","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ; POPF(format.all) ;
		switch (format.fld.type)
		 { default:
			v3_error(V3E_INVVECDT,"VEC","COPY","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
		   case VFT_BININT:
			POPVP(iptr1,(int *)) ;
			for(;i>0;i--) { *(iptr1++) = *(iptr2++) ; } ; break ;
		   case VFT_BINWORD:
			POPVP(wptr1,(short int *)) ;
			for(;i>0;i--) { *(wptr1++) = *(iptr2++) ; } ; break ;
	   	   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			POPVP(bptr1,(char *)) ;
			for(;i>0;i--) { *(bptr1++) = *(iptr2++) ; } ; break ;
		 } ;
		break ;
	   case VFT_BINLONG:
		POPVP(lptr2,(int *)) ; POPF(format.all) ;
		switch (format.fld.type)
		 { default:
			v3_error(V3E_INVVECDT,"VEC","COPY","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
		   case VFT_BINLONG:
			POPVP(lptr1,(int *)) ;
			for(;i>0;i--) { *(lptr1++) = *(lptr2++) ; } ; break ;
		   case VFT_BININT:
			POPVP(iptr1,(int *)) ;
			for(;i>0;i--) { *(iptr1++) = *(lptr2++) ; } ; break ;
		   case VFT_BINWORD:
			POPVP(wptr1,(short int *)) ;
			for(;i>0;i--) { *(wptr1++) = *(lptr2++) ; } ; break ;
	   	   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			POPVP(bptr1,(char *)) ;
			for(;i>0;i--) { *(bptr1++) = *(lptr2++) ; } ; break ;
		 } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ; POPF(format.all) ;
		switch (format.fld.type)
		 { default:
			v3_error(V3E_INVVECDT,"VEC","COPY","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
		   case VFT_BININT:
			POPVP(iptr1,(int *)) ;
			for(;i>0;i--) { *(iptr1++) = *(wptr2++) ; } ; break ;
		   case VFT_BINWORD:
			POPVP(wptr1,(short int *)) ;
			for(;i>0;i--) { *(wptr1++) = *(wptr2++) ; } ; break ;
	   	   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			POPVP(bptr1,(char *)) ;
			for(;i>0;i--) { *(bptr1++) = *(wptr2++) ; } ; break ;
		 } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; POPF(format.all) ;
		switch (format.fld.type)
		 { default:
			v3_error(V3E_INVVECDT,"VEC","COPY","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
		   case VFT_BININT:
			POPVP(iptr1,(int *)) ;
			for(;i>0;i--) { *(iptr1++) = *(bptr2++) ; } ; break ;
		   case VFT_BINWORD:
			POPVP(wptr1,(short int *)) ;
			for(;i>0;i--) { *(wptr1++) = *(bptr2++) ; } ; break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			POPVP(bptr1,(char *)) ;
			for(;i>0;i--) { *(bptr1++) = *(bptr2++) ; } ; break ;
		 } ;
		break ;
	 } ;
	return ;
}

/*	vec_find_constant - Finds constant value in vector, 0 if not found */

void vec_find_constant()
{ int i,j ;
  int *lptr2 ;
  int ival,*iptr1,*iptr2,len ;
  union val__format format ;
  char bval,*bptr1,*bptr2 ;
  short int wval,*wptr1,*wptr2 ;


	j = (i = xctu_popint()) ;	/* Get length */
	POPF(format.all) ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","FIND_CONSTANT","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BINLONG:
		POPVP(lptr2,(int *)) ; ival = xctu_popint() ;
		for(;i>0;i--) { if (*(lptr2++) == ival) break ; } ;
		break ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ; ival = xctu_popint() ;
		for(;i>0;i--) { if (*(iptr2++) == ival) break ; } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ; wval = xctu_popint() ;
		for(;i>0;i--) { if (*(wptr2++) == wval) break ; } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; bval = xctu_popint() ;
		for(;i>0;i--) { if (*(bptr2++) == bval) break ; } ;
		break ;
	 } ;
	PUSHINT(i>0 ? j-i+1 : 0) ; return ;
}

/*	vec_set - Sets all elements of a vector to a constant */

void vec_set()
{ int i ;
  B64INT *lptr2 ;
  int ival,*iptr1,*iptr2,len ;
  union val__format format ;
  char bval,*bptr1,*bptr2 ;
  short int wval,*wptr1,*wptr2 ;

	i = xctu_popint() ;	/* Get length */
	POPF(format.all) ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","ADD","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BINLONG:
		POPVP(lptr2,(B64INT *)) ; ival = xctu_popint() ;
		for(;i>0;i--) { *(lptr2++) = ival ; } ;
		break ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ; ival = xctu_popint() ;
		for(;i>0;i--) { *(iptr2++) = ival ; } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ; wval = xctu_popint() ;
		for(;i>0;i--) { *(wptr2++) = wval ; } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; bval = xctu_popint() ;
		for(;i>0;i--) { *(bptr2++) = bval ; } ;
		break ;
	 } ;
	return ;
}

/*	vec_span - Spans vector until element not found */

void vec_span()
{ int i ; int ac ;
  int *lptr2 ;
  int ival,*iptr1,*iptr2,len ;
  union val__format format ;
  char bval,*bptr1,*bptr2 ;
  short int wval,*wptr1,*wptr2 ;

	i = xctu_popint() ;	/* Get length */
	POPF(format.all) ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","ADD","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BINLONG:
		POPVP(lptr2,(int *)) ; ival = xctu_popint() ;
		for(ac=0;i>0;(ac++,i--))
		 { if (*(lptr2++) != ival) { PUSHINT(ac) ; goto span_done ; } ; } ;
		break ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ; ival = xctu_popint() ;
		for(ac=0;i>0;(ac++,i--))
		 { if (*(iptr2++) != ival) { PUSHINT(ac) ; goto span_done ; } ; } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ; wval = xctu_popint() ;
		for(ac=0;i>0;(ac++,i--))
		 { if (*(wptr2++) != wval) { PUSHINT(ac) ; goto span_done ; } ; } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; bval = xctu_popint() ;
		for(ac=0;i>0;(ac++,i--))
		 { if (*(bptr2++) != bval) { PUSHINT(ac) ; goto span_done ; } ; } ;
		break ;
	 } ;
/*	If here then hit end of vector */
	PUSHINT(-1) ;
span_done:
	return ;
}

/*	vec_sum - Sums all elements in a vector & returns sum */

void vec_sum()
{ int i,j ;
  B64INT *lptr2 ;
  int *iptr2 ; short int *wptr2 ; char *bptr2 ;
  union val__format format ;
  union val__format format1 ;

	j = 0 ; i = xctu_popint() ;	/* Zap sum & get length */
	POPF(format.all) ; format1.all = V3_FORMAT_INTEGER ; format1.fld.decimals = format.fld.decimals ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","SUM","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BINLONG:
		POPVP(lptr2,(B64INT *)) ;	for(;i>0;i--) { j += *(lptr2++) ; } ;
		break ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ;	for(;i>0;i--) { j += *(iptr2++) ; } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ;	for(;i>0;i--) { j += *(wptr2++) ; } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; for(;i>0;i--) { j += *(bptr2++) ; } ;
		break ;
	 } ;
	PUSHVI(j) ; PUSHF(format1.all) ; return ;
}

/*	vec_swap - Swaps contents of two vectors	*/

void vec_swap()
{ int i ;
  B64INT lval,*lptr1,*lptr2 ;
  int ival,*iptr1,*iptr2,len ;
  union val__format format ;
  char bval,*bptr1,*bptr2 ;
  short int wval,*wptr1,*wptr2 ;

	i = xctu_popint() ;	/* Get length */
	POPF(format.all) ;
	switch(format.fld.type)
	 { default:
		v3_error(V3E_INVVECDT,"VEC","ADD","INVVECDT","Invalid datatype for vector operation",(void *)format.fld.type) ;
	   case VFT_BINLONG:
		POPVP(lptr2,(B64INT *)) ; POPF(format.all) ; POPVP(lptr1,(B64INT *)) ;
		for(;i>0;i--)
		 { lval = *lptr1 ; *(lptr1++) = *lptr2 ; *(lptr2++) = lval ; } ;
		break ;
	   case VFT_BININT:
		POPVP(iptr2,(int *)) ; POPF(format.all) ; POPVP(iptr1,(int *)) ;
		for(;i>0;i--)
		 { ival = *iptr1 ; *(iptr1++) = *iptr2 ; *(iptr2++) = ival ; } ;
		break ;
	   case VFT_BINWORD:
		POPVP(wptr2,(short int *)) ; POPF(format.all) ; POPVP(wptr1,(short int *)) ;
		for(;i>0;i--) { wval = *wptr1 ; *(wptr1++) = *wptr2 ; *(wptr2++) = wval ; } ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		POPVP(bptr2,(char *)) ; POPF(format.all) ; POPVP(bptr1,(char *)) ;
		for(;i>0;i--)
		 { bval = *bptr1 ; *(bptr1++) = *bptr2 ; *(bptr2++) = bval ; } ;
		break ;
	 } ;
	return ;
}
