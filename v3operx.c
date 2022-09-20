/*	V3OPERX.C - Handles seldom-used V3 "Operations"

	Created 22-Dec-94 By Victor E. Hansen			*/

#define NEED_SOCKET_LIBS 1
#include <time.h>
#include "v3defs.c"
#ifdef UNIX
#ifdef INCSYS
#include <sys/types.h>
#include <sys/param.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <glob.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#else
#include <types.h>
#include <glob.h>
#include <param.h>
#include <times.h>
#include <stat.h>
#include <stdlib.h>
#include <signal.h>
#endif
#endif
#ifdef WINNT
#include <winnetwk.h>
#endif

/*	G L O B A L   E X E C U T I O N   S Y M B O L S		*/

struct db__psi *xctu_cmd_invoke() ;
extern struct V4C__Context *ctx ;
extern struct db__process *process ;	/* Points to current process */
extern struct db__psi *psi ;		/* Points to current psi */
extern int PRS_FUNCTION_COUNT ; /* Number of V3 predefined functions */
extern struct db__predef_table V3_FUNCTIONS[] ;
extern struct iou__openlist ounits ;
extern int termSocket ;
extern int V3_OPCODE_USAGE[300] ;	/* Opcode usage table */
extern char ttystr[V3_PRS_INPUT_LINE_MAX+1] ;	/* TTY_GETx input buffer */
extern char ttytrm[V3_TTY_TERMINATOR_MAX+1] ;	/* Holds terminator string after TTY input */
extern int v3_param_date ;	/* If nonnegative then V3 "date" */
extern int v3_param_time ;	/* If nonnegative then V3 "time" */

#ifdef WINNT
#include <windows.h>
#include <io.h>
#endif
#ifdef VMSOS
#include <stat.h>
#endif

char *stru_popcstr() ;
struct db__module_entry *pcku_module_defined() ;

void v3_operationsx(code)
  int code ;			/* Operation code - See V3_XCT_xxx */
{ int ac,*base_ptr,*int_ptr ; unsigned int ui ;
#ifdef UNIX
   struct stat stat_buf ;
#endif /* POSIX */
#ifdef WINNT
WIN32_FIND_DATA FileData ;
SYSTEMTIME systime ;
#endif
#ifdef VMSOS
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc,strdsc2 ;
   struct stat stat_buf ;
#endif
struct sys__user_id	/* Format of 12 byte user id */
 { 
#ifdef WINNT
   int pid ;
#else
   int pid ;			/* Process id */
#endif
   int date_time[2] ;		/* Universal date & time of login */
 } user_id ;
struct sys__user_id *user_id_ptr ;
/*	Set up structure for VAX job information		*/
#ifdef VMSOS
struct {
  short int buffer_length ;		/* Length of buffer */
  short int item_code ;			/* Code of what we want- JPI$_xxx */
  int buffer_address ;			/* Where VMS is to put result */
  int return_length_address ;
  int end_of_list ;
 } jpi_table ;
globalvalue int JPI$_LOGINTIM,JPI$_DIRIO,JPI$_FREP0VA,JPI$_PID ;
globalvalue int SYI$_BOOTTIME ;
#endif
   struct iou__fileinfo *ifi ;	/* For sys_info(/next_file/,xxx) */
#ifdef UNIX
   glob_t *globt ;
#endif
   static char bigbuf[4096] ;	/* Big print buffer */
   struct db__command_search_list cmd_list ;
   struct db__xct_cmd_info cmd_info ;
   struct cmd__args cmd_alist ;	/* List of args for command invocation */
   LONGINT i,j,k ;
   V3STACK *bsp ;			/* Base stack pointer */
   int *(*mp_ptr) ;			/* Pointer to a pointer */
   extern int *watch_location,watch_value ;
   time_t tt ;
   struct db__package *pckp ;
   struct db__parse_info *prsp ;
   struct db__module_entry *mentry ;
   union val__v3num v3n ;
   struct str__ref strref ;	/* String reference */
   char *str_ptr,tmpstr[200],tmpstr1[200] ;		/* Temp string */
   static char *cbb ;
   char *bp1,*bp2 ;
   FILE *fp ;
   struct db__formatptr *fw ;
   union val__format tmp_val ;	/* Temp value */
   struct VRPP__Xct *vrx ;
   struct V4LEX__TknCtrlBlk *tcb ;
   struct V4LEX__RevPolParse *rpp ;

	switch(code)
	 { default:
		v3_error(V3E_BADCODE,"XCT","OPERATIONS","BADCODE","Invalid code value in v3_operations",(void *)code) ;
	   case V3_XCT_CB_PUT:
		if (cbb == NULL) cbb = (char *)stru_alloc_bytes(15000) ;	/* Get temp buffer */
		stru_popcstr(cbb) ;
#ifdef WINNT
		{ HGLOBAL hc ;
		  OpenClipboard(NULL) ; EmptyClipboard() ;
		  hc = GlobalAlloc(GMEM_DDESHARE,strlen(cbb)+1) ;
		  bp1 = GlobalLock(hc) ; memcpy(bp1,cbb,strlen(cbb)+1) ; GlobalUnlock(hc) ;
		  SetClipboardData(CF_TEXT,hc) ;
		  CloseClipboard() ;
		}
#endif
		break ;
	   case V3_XCT_CMD_ARG_NUM:
		if (psi->command_psi == NULLV)
		 v3_error(V3E_ONLYCMD,"CMD","ARG_NUM","ONLYCMD","CMD_xxx only allowed within command",NULLV) ;
		PUSHINT(psi->command_arg_cnt) ; break ;
	   case V3_XCT_CMD_ARG_VALUE:
		if (psi->command_psi == NULLV)
		 v3_error(V3E_ONLYCMD,"CMD","ARG_VALUE","ONLYCMD","CMD_xxx only allowed within command",NULLV) ;
		i = xctu_popint() ;
		if (i < i || i > psi->command_arg_cnt)
		 v3_error(V3E_BADCMDARG,"CMD","ARG_VALUE","BADCMDARG","Argument to CMD_ARG_VALUE is out of range",(void *)psi->command_arg_cnt) ;
		PUSHVP(*(psi->command_arg_ptr-2*i+3)) ; PUSHF(*(psi->command_arg_ptr-(2*(i-1)))) ; break ;
	   case V3_XCT_CMD_TEST:
		xctu_cmd_str_convert(stru_popcstr(tmpstr),&cmd_list) ;
		PUSHINT(xctu_cmd_find(&cmd_list,&cmd_info)) ; break ;
	   case V3_XCT_CMD_XCT:
/*		Pop off any arguments into cmd_alist */
		cmd_alist.count = 0 ;
		for (;;)
		 { POPF(cmd_alist.arglist[cmd_alist.count].format.all) ;
		   if (cmd_alist.arglist[cmd_alist.count].format.all == V3_FORMAT_EOA) { POPVI(i) ; break ; } ;
		   POPVP(cmd_alist.arglist[cmd_alist.count++].ptr,(char *)) ;
		 } ;
/*		Last item should be first argument, put back on stack */
		cmd_alist.count -= 1 ; REPUSH ; REPUSH ;
		xctu_cmd_str_convert(stru_popcstr(tmpstr),&cmd_list) ;
		if (xctu_cmd_find(&cmd_list,&cmd_info) == FALSE) break ;
		xctu_cmd_invoke(&cmd_info,(cmd_alist.count > 0 ? &cmd_alist : NULLV)) ; break ;
	   case V3_XCT_LOAD:
		if (process->package_ptrs[psi->package_index]->parse_ptr == NULLV)
		 v3_error(V3E_NOLOAD,"LOAD","OPERATIONS","NOLOAD","Cannot LOAD from xct-only package",NULLV) ;
		str_ptr = stru_popcstr(tmpstr) ;
		if ((fp = fopen((char *)v3_logical_decoder(tmpstr,TRUE),"r")) == NULLV)
		 { if (strchr(tmpstr,'.') != NULLV)
		    v3_error(V3E_NOFILETOLOAD,"LOAD","OPERATIONS","NOFILETOLOAD","Could not LOAD file",tmpstr) ;
/*		   No extension - append ".v3" */
		   strcat(tmpstr,".v3") ; strcpy(tmpstr,(char *)v3_logical_decoder(tmpstr,TRUE)) ;
		   if ((fp = fopen(tmpstr,"r")) == NULLV)
		    v3_error(V3E_NOFILETOLOAD,"LOAD","OPERATIONS","NOFILETOLOAD","Could not LOAD file",tmpstr) ;
		 } ;
		prsu_nest_input(process->package_ptrs[psi->package_index]->parse_ptr,fp,tmpstr) ;
		break ;
	   case V3_XCT_PRINT:
	   case V3_XCT_TTY_PUT:
		for (ac=0;;ac++) { POPF(tmp_val.all) ; POPVI(i) ; if (tmp_val.all == V3_FORMAT_EOA) break ; } ;
/*		Know how many args, now print out in proper order */
		bsp = (psi->stack_ptr-SIZEOFSTACKENTRY) ; bigbuf[0] = 0 ;
		for (j=1;j<=ac;j++)
		 { psi->stack_ptr = bsp-(SIZEOFSTACKENTRY*j) ;
		   POPF(tmp_val.all) ;
/*		   What do we got ? */
retry_indirect: /* (Here on retry of indirect) */
		   switch (tmp_val.fld.type)
		    { default: POPVI(i) ;
			sprintf(tmpstr,"%% Unknown value (%d) type to print",tmp_val.fld.type) ;
			strcat(bigbuf,tmpstr) ; break ;
		      case VFT_OBJREF:
			POPVP(int_ptr,(int *)) ; sprintf(tmpstr,"(obj_%s)",sobu_name_str(*int_ptr)) ;
			strcat(bigbuf,tmpstr) ; break ;
		      case VFT_OBJECT:
			POPVI(i) ; sprintf(tmpstr,"(obj_%s)",sobu_name_str(tmp_val.all)) ;
			strcat(bigbuf,tmpstr) ; break ;
		      case VFT_FIXSTRSK:
		      case VFT_FIXSTR: POPVP(bp1,(char *)) ;
			i = strlen(bigbuf) ;
		  	strncat(bigbuf,bp1,tmp_val.fld.length) ; bigbuf[i+tmp_val.fld.length] = 0 ;
			break ;
		      case VFT_VARSTR: POPVP(bp1,(char *)) ; strcat(bigbuf,bp1) ; break ;
		      case VFT_POINTER:
			POPVP(mp_ptr,(int *(*))) ;
			if (tmp_val.fld.mode == VFM_IMMEDIATE) { sprintf(tmpstr,"^X%p",mp_ptr) ; }
			 else { sprintf(tmpstr,"^X%p",*mp_ptr) ; } ;
			strcat(bigbuf,tmpstr) ; break ;
		      case VFT_BINWORD:
		      case VFT_BININT:
		      case VFT_BINLONG:
		      case VFT_STRINT:
		      case VFT_PACDEC:
		      case VFT_V3NUM:
			PUSHF(tmp_val.all) ; i = v3num_popv3n(&v3n) ;
			v3num_itx(&v3n,tmpstr,i) ; strcat(bigbuf,tmpstr) ; break ;
		      case VFT_FLOAT:
			PUSHF(tmp_val.all) ; v3num_popv3n(&v3n) ;
			sprintf(tmpstr,"%g",v3n.fp.dbl) ; strcat(bigbuf,tmpstr) ; break ;
		      case VFT_INDIRECT:
			POPVP(fw,(struct db__formatptr *)) ;
			switch (fw->format.fld.type)
			 { default: PUSHMP(fw->ptr) ; tmp_val.all = fw->format.all ; goto retry_indirect ;
			   case VFT_VARSTR:	strcat(bigbuf,fw->ptr) ; break ;
			   case VFT_FIXSTRSK:
			   case VFT_FIXSTR:
				i = strlen(bigbuf) ; strncat(bigbuf,fw->ptr,fw->format.fld.length) ;
				bigbuf[i+fw->format.fld.length] = 0 ; break ;
			 } ;
			break ;
		    } ;
		 } ;
		psi->stack_ptr = bsp + SIZEOFSTACKENTRY ;
#ifdef ISOCKETS
		if (termSocket != 0)
		 { if ((i = send(termSocket,bigbuf,strlen(bigbuf),0)) < strlen(bigbuf))
		    { sprintf(tmpstr,"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),strlen(bigbuf),i,termSocket) ;
		      v3_error(V3E_SOCKETSEND,"TTY","GET","SOCKETSEND",tmpstr,0) ;
		    } ;
		   if (code == V3_XCT_PRINT) { send(termSocket,"\n",strlen("\n"),0) ; } ;
		   break ;
		 } ;
#endif
#ifdef VMS
		if (code == V3_XCT_PRINT) { puts(bigbuf) ; } else { printf("%s",bigbuf) ; } ;
#else
#ifdef WINNT
		if (code == V3_XCT_PRINT) strcat(bigbuf,"\r\n") ;
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),bigbuf,strlen(bigbuf),&i,NULL) ;
#else
		if (code == V3_XCT_PRINT) { fputs(bigbuf,stdout) ; fputs("\n",stdout) ; }
		 else fputs(bigbuf,stdout) ;
#endif
#endif
		break ;
	   case V3_XCT_PCK_COMPILE:
		str_ptr = stru_popcstr(tmpstr) ; k = xctu_popint() ;
/*		1) create package, 2) load file, 3) save V3X	*/
		strcpy(tmpstr1,tmpstr) ; strcat(tmpstr1,".v3") ; strcpy(tmpstr1,(char *)v3_logical_decoder(tmpstr1,TRUE)) ;
		fp = fopen(tmpstr1,"r") ;
		if (fp == NULLV) v3_error(V3E_NOFILETOCOMP,"PCK","COMPILE","NOFILETOCOMP","Could not access V3 file",tmpstr1) ;
/*		If package name prefaced with a logical then strip off */
		if ((bp1 = (char *)strchr(str_ptr,':')) != 0) { str_ptr = bp1 + 1 ; }
		 else { if ((bp1 = (char *)strchr(str_ptr,']')) != 0) str_ptr = bp1 + 1 ; } ;
		pcku_parse_init(k,str_ptr) ; pckp = (struct db__package *)pcku_find(str_ptr,TRUE) ;
		prsp = pckp->parse_ptr ; prsp->in_pck_compile = TRUE ;
		xctu_call_parser(pckp->package_id,fp,tmpstr1,TRUE) ;
		if (prsp == NULLV)
		 v3_error(V3E_NOSAVE,"PCK","SAVE","NOSAVE","Cannot save xct-only package",pckp->package_name) ;
/*		If any parser errors then abort (don't save) */
		if (prsp->error_flag > 0) exit(process->exit_value=44) ;
		strcat(str_ptr,".v3x") ; pcku_save(pckp,str_ptr) ;
		if (prsp->ctx != NULLV) v4ctx_AreaClose(prsp->ctx,-1) ;	/* Close any areas */
		exit(process->exit_value=EXIT_SUCCESS) ;
	   case	V3_XCT_PCK_DCL_FILENAME:
		str_ptr = stru_popcstr(tmpstr) ; i = xctu_popint() ;
		if (i < 1 || i > V3_PROCESS_SYSTEM_PACKAGES_MAX)
		 v3_error(V3E_INVPCKNUM,"PCK","DCL_FILENAME","INVPCKNUM","Package number is out of range",(void *)i) ;
		strncpy(process->system_packages[i-1].file_name,str_ptr,V3_PROCESS_PCK_FILENAME_MAX-1) ;
		break ;
	   case V3_XCT_PCK_ENTER:
/*		package_enter(name,prompt)	*/
		str_ptr = stru_popcstr(tmpstr) ;
		pckp = (struct db__package *)pcku_find(stru_popcstr(tmpstr1),TRUE) ;
		xctu_call_parser(pckp->package_id,NULLV,str_ptr,TRUE) ; break ;
	   case V3_XCT_PCK_LOAD:
		str_ptr = stru_popcstr(tmpstr) ;		/* File name */
		if ((fp = fopen((char *)v3_logical_decoder(tmpstr,TRUE),"r")) == NULLV)
		 { strcat(tmpstr,".v3") ; fp = fopen((char *)v3_logical_decoder(tmpstr,TRUE),"r") ; } ;
		if (fp == NULLV) v3_error(V3E_PCKLOADNOFILE,"PCK","LOAD","PCKLOADNOFILE","Could not access file to load",tmpstr) ;
		pckp = (struct db__package *)pcku_find(stru_popcstr(tmpstr1),TRUE) ;
		xctu_call_parser(pckp->package_id,fp,tmpstr,TRUE) ; break ;
	   case V3_XCT_PCK_NEW:
		str_ptr = stru_popcstr(tmpstr) ;		/* Package name */
		i = xctu_popint() ;
		pcku_parse_init(i,str_ptr) ; break ;
	   case V3_XCT_PCK_OLD_PARSE:
		pcku_parse_load(stru_popcstr(tmpstr)) ; break ;
	   case V3_XCT_PCK_OLD_SYSTEM:
		i = xctu_popint() ;
		if (i < 1 || i > V3_PROCESS_SYSTEM_PACKAGES_MAX)
		 v3_error(V3E_INVPCKNUM,"PCK","OLD_SYSTEM","INVPCKNUM","Package number is out of range",(void *)i) ;
		if (strlen(process->system_packages[i-1].file_name) == 0)
		 v3_error(V3E_NODCLNAME,"PCK","OLD_SYSTEM","NODCLNAME","No filename has been declared for this package slot",(void *)0) ;
/*		If package slot in use then nothing to do */
		if (process->package_ptrs[i] != NULLV) break ;
		pcku_xct_load(process->system_packages[i-1].file_name,TRUE) ;
		break ;
	   case V3_XCT_PCK_OLD_XCT:
		i = xctu_popint() ;	/* Get error-if-loaded flag */
		pckp = (struct db__package *)pcku_xct_load(stru_popcstr(tmpstr),i) ;
		PUSHINT(pckp->package_id) ; break ;
	   case V3_XCT_PCK_SAVE:
		str_ptr = stru_popcstr(tmpstr) ;
		pckp = (struct db__package *)pcku_find(stru_popcstr(tmpstr1),TRUE) ;
		pcku_save(pckp,str_ptr) ; break ;
	   case V3_XCT_PCK_UNLOADA:
		if ((pckp = (struct db__package *)pcku_find(stru_popcstr(tmpstr),FALSE)) == NULLV)
/*		   Could not find package - skip over next opcode (PCK_UNLOADB) */
		 { psi->code_ptr++ ; pcku_find(tmpstr,TRUE) ; } ;
/*		Push package pointer onto stack for call to UNLOADB */
		PUSHMP(pckp) ; PUSHF(0) ; PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
/*		See if we got "PACKAGE_UNLOAD" routine */
		if ((mentry = pcku_module_defined("PACKAGE_UNLOAD")) != NULLV) xctu_call_mod(mentry) ;
		break ;
	   case V3_XCT_PCK_UNLOADB:
/*		Here to do actual unload */
		POPF(tmp_val.all) ;
		if (tmp_val.all == V3_FORMAT_EOA) { POPVI(i) ; POPF(i) ; } ;
		POPVI(i) ; base_ptr = (int *)i ;
		pckp = (struct db__package *)*base_ptr ; 	/* POP off pointer to package */
		pcku_unload(pckp) ; break ;
	   case V3_XCT_SYS_CHAIN:
#ifdef VMSOS
		str_ptr = stru_popcstr(tmpstr) ;
		strdsc.length = strlen(str_ptr) ; strdsc.desc = 0 ; strdsc.pointer = str_ptr ;
		lib$do_command(&strdsc) ;
#endif
		v3_error(V3E_NOCHAIN,"SYS","CHAIN","NOCHAIN","Could not chain to progam",NULLV) ;
	   case V3_XCT_SYS_LOGOUT:
		process->exit_value = 1 ; exit_handler() ;	/* First cleanup as much as possible */
		if (termSocket != 0)
		 { printf("Not allowed to execute sys_logout() with -w startup switch, just EXIT'ing\n") ;
		   exit(process->exit_value) ;
		 } ;
#ifdef VMSOS
		strdsc.pointer = "LOGOUT" ; strdsc.length = strlen(strdsc.pointer) ; strdsc.desc = 0 ;
		lib$do_command(&strdsc) ;
#endif
#ifdef POSIX
		kill(0,9) ;
#endif
#ifdef WINNT
		ExitWindowsEx(EWX_LOGOFF,0) ;
#endif
		v3_error(V3E_NOLOGOUT,"SYS","LOGOUT","NOLOGOUT","Could not logout process family",NULLV) ;
	   case V3_XCT_SYS_DELETE:
	   	POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
	   	if (tmp_val.all == V3_FORMAT_INTEGER)	/* Maybe got sys_delete(file,/noerror/) */
	   	 { j = xctu_popint() ; } else { j = 0 ; }
		stru_popcstr(tmpstr) ; i = remove((char *)v3_logical_decoder(tmpstr,TRUE)) ;
		if (j == V3_FLAGS_EDT_NOERROR)
		 { if (i == -1) { PUSHFALSE ; break ; } ;
		 } else
		 { if (i == -1) v3_error(V3E_NODELETE,"SYS","DELETE","NODELETE","Error in sys_delete",(void *)errno) ; } ;
		PUSHTRUE ; break ;
	   case V3_XCT_SYS_EXIT:
		process->exit_value = 1 ; exit_handler() ;	/* First cleanup as much as possible */
/*		Look for an argument */
		POPF(tmp_val.all) ;
		if (tmp_val.all == V3_FORMAT_EOA) { process->exit_value = 1 ; exit(EXIT_SUCCESS) ; } ;
/*		Got an argument, best be integer value */
		PUSHF(tmp_val.all) ; exit(process->exit_value = xctu_popint()) ;
	   case V3_XCT_SYS_RENAME:
	   	POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
	   	if (tmp_val.all == V3_FORMAT_INTEGER)	/* Maybe got sys_rename(file,/noerror/) */
	   	 { j = xctu_popint() ; } else { j = 0 ; }
		stru_popcstr(tmpstr) ; strcpy(tmpstr,v3_logical_decoder(tmpstr,FALSE)) ; stru_popcstr(tmpstr1) ;
#ifdef WINNT
		remove(tmpstr) ;		/* Make sure target file does not exist on NT */
#endif
		i = rename((char *)v3_logical_decoder(tmpstr1,TRUE),tmpstr) ;
		if (j == V3_FLAGS_EDT_NOERROR)
		 { if (i == -1) { PUSHFALSE ; break ; } ;
		 } else
		 { if (i == -1) v3_error(V3E_NORENAME,"SYS","RENAME","NORENAME","Error in sys_rename",(void *)errno) ; } ;
		PUSHTRUE ; break ;
	   case V3_XCT_SYS_SLEEP:
		i = xctu_popint() ;
		if (i < 0)		/* If negative then in milliseconds */
		 { HANGLOOSE(-i) ; break ; } ;
#ifdef WINNT
		Sleep(i*1000) ;
#endif
#ifdef POSIX
		sleep(i) ;
#endif
		break ;
	   case V3_XCT_SYS_SPAWN:
/*		Did we get an argument string ? */
		POPF(tmp_val.all) ;
		if (tmp_val.all == V3_FORMAT_EOA)
		 { POPVI(i) ;
//		   if (!v_SpawnProcess(NULL,NULL,V_SPAWNPROC_Wait,tmpstr,NULL,NULL,UNUSED))
//		    v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(tmpstr),0) ;
		   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = tmpstr ; sa.fileId = UNUSED ;
 		     if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(tmpstr),0) ;
		   }
		   PUSHINT(TRUE) ; break ;
		 } ;
		PUSHF(tmp_val.all) ; stru_popcstr(bigbuf) ;
//		if (!v_SpawnProcess(NULL,ASCretUC(bigbuf),V_SPAWNPROC_Wait,tmpstr,NULL,NULL,UNUSED))
//		 v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(tmpstr),0) ;
		{ struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = ASCretUC(bigbuf) ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = tmpstr ; sa.fileId = UNUSED ;
 		  if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(tmpstr),0) ;
		}
		POPF(i) ; POPVI(i) ; PUSHINT(TRUE) ; break ;
	   case V3_XCT_SYS_SUBMIT:
#ifdef VMSOS
		vax_queue("SYS$BATCH") ; break ;
#endif
		sys_queue("SYS$BATCH") ; break ;
	   case V3_XCT_SYS_PRINT:
#ifdef VMSOS
		vax_queue("SYS$PRINT") ; break ;
#endif
		sys_queue("SYS$PRINT") ; break ;
	   case V3_XCT_SYS_INFO:
/*		Pop off args until EOA				*/
		for (i=0;;i++)
		 { POPF(tmp_val.all) ; POPVI(j) ;
		   if (tmp_val.all == V3_FORMAT_EOA) break ;
		 } ; bsp = psi->stack_ptr ; REPUSH ; REPUSH ;
		j = xctu_popint() ; process->last_v3_opcode = V3_XCT_SYS_INFO*100000 + j ;
		switch (j)
		 { default:
/*			Have special test for /date_time/ (it's a big value!) */
			if (j == V3_FLAGS_EDT_DATE_TIME)
			 {
/*			   Adjust for time zone & maybe daylight savings, finally adjust for V3 /date_time/ */
			   tt = time(NULLV) - (60*mscu_minutes_west()) ;
			   tt -= TIMEOFFSETSEC ; i = (int)tt ; break ;
			 } ;
			v3_error(V3E_BADSYSINFOARG,"SYS","INFO","BADSYSINFOARG","Invalid argument to SYS_INFO",(void *)j) ;
		   case V3_FLAGS_SYS_BOX:
			psi->stack_ptr = bsp ; PUSHMP(V3_CONFIG_BOX) ; PUSHF(V3_FORMAT_VARSTR) ; goto sys_info_end ;
		   case V3_FLAGS_SYS_COMMAND_LINE:
			psi->stack_ptr = bsp ; PUSHMP(&process->v3_start_command_line) ; PUSHF(V3_FORMAT_VARSTR) ;
			goto sys_info_end ;
		   case V3_FLAGS_SYS_CPU_TIME:
			if (CLOCKS_PER_SEC < 5000) { i = (100*clock()) / CLOCKS_PER_SEC ; }
			 else { i = clock() / (CLOCKS_PER_SEC / 100) ; } ;
			psi->stack_ptr = bsp ; PUSHVI(i) ; PUSHF(V3_FORMAT_INTEGER2) ;
			goto sys_info_end ;
		   case V3_FLAGS_SYS_DATE: /* Return current date - Universal format */
			if (v3_param_date >= 0) { i = v3_param_date ; }
			 else { tt = time(NULLV)-(60*mscu_minutes_west()) ; i = TIMEOFFSETDAY + tt/(60*60*24) ; } ;
			break ;
		   case V3_FLAGS_SYS_DIRECTORY_PATH:
			bp1 = (char *)stru_alloc_bytes(150) ;	/* Get temp buffer */
#ifdef WINNT
			GetCurrentDirectory(50,bp1) ;
#endif
#ifdef UNIX
			bp2 = getenv("PWD") ;		/* Get current value of current working directory */
			if (bp2 == NULL) bp2 = "." ;	/* If no value then default to "./" */
			strcpy(bp1,bp2) ;		/* Copy path into temp string buffer */
#endif
#ifdef VMSOS
			str_ptr = getenv("PATH") ;
			if (str_ptr == NULL) str_ptr = "[]" ;
			strcpy(bp1,str_ptr) ;
#endif
			psi->stack_ptr = bsp ; PUSHMP(bp1) ; PUSHF(V3_FORMAT_VARSTR) ;
			goto sys_info_end ;
		   case	V3_FLAGS_SYS_DISK_AVAILABLE:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popcstr(tmpstr) ;
#ifdef WINNT
			GetDiskFreeSpace((strlen(tmpstr) > 0 ? tmpstr : NULL),&i,&j,&k,&ac) ;
			i = i*j*k ;		/* Free = SectorsPerCluster*BytesPerSector*FreeClusters */
#endif
#ifdef SU486
			i = 0 ;
#endif
#ifdef HPUX
			i = 0 ;
#endif
#ifdef ULTRIX
			i = 0 ;
#endif
#ifdef RS6000AIX
			i = 0 ;
#endif
#ifdef ALPHAOSF
			i = 0 ;
#endif
#ifdef LINUX486
			i = 0 ;
#endif
#ifdef VMSOS
			strdsc.length = strlen(tmpstr) ; strdsc.desc = 0 ; strdsc.pointer = tmpstr ;
			i = v3_vms_disk_available(&strdsc) ;
#endif
			break ;
		   case	V3_FLAGS_SYS_DISK_USAGE:
#ifdef VMSOS
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popcstr(tmpstr) ;
			strdsc.length = strlen(tmpstr) ; strdsc.desc = 0 ; strdsc.pointer = tmpstr ;
			i = v3_vms_disk_usage(&strdsc) ; break ;
#else
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popcstr(tmpstr) ;
			i = 0 ; break ;
#endif
		   case V3_FLAGS_SYS_ENVIRONMENT:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popcstr(tmpstr) ;
			for(i=0;tmpstr[i]!='\0';i++) { tmpstr[i] = tolower(tmpstr[i]) ; } ;
			for(i=0;;i++)
			 { char *def,*bp,name[64] ; int len ;
			   extern char **startup_envp ;
			   def = startup_envp[i] ;	/* def = pointer to next item in environment */
			   if (def == NULL) { bp1 = "" ; break ; } ;
			   bp = strchr(def,'=') ; if (bp == NULL) bp = &def[strlen(def)] ;
			   len = bp - def ; if (len >= sizeof name) len = (sizeof name) - 1 ;
			   strncpy(name,def,len) ; name[len] = '\0' ; for(j=0;name[j]!='\0';j++) { name[j] = tolower(name[j]) ; } ;
			   if (strcmp(name,tmpstr) == 0) { bp1 =  (*bp == '=' ? bp+1 : "") ; break ; } ;
			 } ;
			psi->stack_ptr = bsp ; PUSHMP(bp1) ; PUSHF(V3_FORMAT_VARSTR) ;
			goto sys_info_end ;
		   case V3_FLAGS_SYS_IO_COUNT:
#ifdef VMSOS
			jpi_table.buffer_length = 4 ; jpi_table.item_code = JPI$_DIRIO ;
			jpi_table.end_of_list = 0 ; jpi_table.return_length_address = 0 ;
			jpi_table.buffer_address = &i ; sys$getjpiw(0,0,0,&jpi_table,0,0,0) ;
			break ;
#else
			i = 0 ; /* HPXXX */
			break ;
#endif
		   case V3_FLAGS_SYS_IS_ACTIVE_USER:
#ifdef WINNT
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popstr(&strref) ;
			user_id_ptr = (struct sys__user_id *)strref.ptr ;
			GetExitCodeProcess((HANDLE)user_id_ptr->pid,&i) ;
			i = (i == STILL_ACTIVE) ;
			break ;
#endif
#ifdef VMSOS
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popstr(&strref) ; user_id_ptr = strref.ptr ;
/*			if LH of pid not same as ours (i.e. on different node) then assume still logged in */
/*			if ( (user_id_ptr->pid & 0xFFFF0000) != (getpid() & 0xFFFF0000) ) { i = TRUE ; break ; } ; */
memset(&user_id,0,12) ;
			jpi_table.buffer_length = 8 ; jpi_table.item_code = JPI$_LOGINTIM ;
			jpi_table.buffer_address = &user_id.date_time ;
			jpi_table.end_of_list = 0 ; jpi_table.return_length_address = 0 ;
			i = sys$getjpiw(0,&user_id_ptr->pid,0,&jpi_table,0,0,0) ;
			if (user_id_ptr->date_time[0] == user_id.date_time[0] && user_id_ptr->date_time[1] == user_id.date_time[1])
			 { i = TRUE ; } else { i = FALSE ; } ;
			break ;
#endif
#ifdef POSIX
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popstr(&strref) ;
			user_id_ptr = (struct sys__user_id *)strref.ptr ;
			i = (kill(user_id_ptr->pid,0) == 0) ;	/* Return TRUE if pid still valid */
			break ;
#endif
			i = TRUE ;
			break ;

		   case V3_FLAGS_SYS_IS_BATCH_JOB:
			i = !v_IsAConsole(stdin) ;
			break ;
		   case V3_FLAGS_SYS_IS_GUI:
			i = FALSE ;				/* Assume no GUI unless... */
#ifdef WINNT
			i = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&ac) ;
#endif
			break ;
		   case V3_FLAGS_SYS_NEXT_FILE:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popstr(&strref) ;
			ifi = (struct iou__fileinfo *)strref.ptr ;
#ifdef VMSOS
			if (ifi->internal1 == 0)		/* First call ? */
			 { strcpy(ifi->directorypath,ifi->filename) ; } ;
			strdsc.length = sizeof(ifi->filename)-2 ; strdsc.desc = 0 ; strdsc.pointer = ifi->filename ;
			strdsc2.length = sizeof(ifi->directorypath) ; strdsc2.desc = 0 ; strdsc2.pointer = ifi->directorypath ;
			i = LIB$FIND_FILE(&strdsc2,&strdsc,&ifi->internal1,0,0,0,0) ;
			if ((i & 1) == 0)			/* Failed ? */
			 { LIB$FIND_FILE_END(&ifi->internal1) ; i = -i ; break ; } ;
			ifi->filename[strdsc.length] = 0 ;
			bp1 = (char *)strchr(ifi->filename,' ') ; if (bp1 != NULL) *bp1 = 0 ;
			stat(ifi->filename,&stat_buf) ;
			ifi->filebytes = stat_buf.st_size ;
			ifi->updatedt = stat_buf.st_mtime - (60*mscu_minutes_west()) - TIMEOFFSETSEC ;
			i = TRUE ; break ;
#endif
#ifdef UNIX
			if (ifi->internal2 == NULL)		/* First time ? */
			 { globt = (glob_t *)v4mm_AllocChunk(sizeof *globt,TRUE) ;
			   ifi->internal2 = (char *)globt ;
			   strcpy(ifi->filename,(char *)v3_logical_decoder(ifi->filename,FALSE)) ;
			   glob(ifi->filename,0,NULL,globt) ;	/* Get list of files */
			   ifi->internal1 = 0 ;
			 } ;
			globt = (glob_t *)ifi->internal2 ;
			if (ifi->internal1 >= globt->gl_pathc)
			 { globfree(globt) ; v4mm_FreeChunk(globt) ; i = FALSE ; break ; } ;
			strcpy(ifi->filename,globt->gl_pathv[ifi->internal1++]) ;
			stat(ifi->filename,&stat_buf) ;
			ifi->filebytes = stat_buf.st_size ;
			ifi->updatedt = stat_buf.st_mtime - (60*mscu_minutes_west()) - TIMEOFFSETSEC ;
			ifi->filetype = ((stat_buf.st_mode & S_IFDIR) != 0 ? 1 : 0) ;	/* Is it a directory? */
			i = TRUE ; break ;
#endif
#ifdef WINNT
			strcpy(tmpstr,ifi->filename) ;
			if (ifi->internal1 == 0)
			 { for(i=1;;i++)
			    { bp1 = (char *)v3_logical_decoderx(ifi->filename,FALSE,i,NULL) ;
			      if (bp1 == NULL) goto no_more_files ;	/* Could not find any matches */
			      strcpy(ifi->filename,bp1) ;
			      bp1 = strrchr(ifi->filename,'\\') ;	/* Look for directory path */
			      if (bp1 == NULL) { ifi->directorypath[0] = 0 ; }
			       else { *bp1 = 0 ; strcpy(ifi->directorypath,ifi->filename) ; *bp1 = '\\' ; strcat(ifi->directorypath,"\\") ; } ;
			      ifi->internal1 = FindFirstFile(ifi->filename,&FileData) ;
			      if (ifi->internal1 == INVALID_HANDLE_VALUE) { strcpy(ifi->filename,tmpstr) ; continue ; } ;
			      break ;
			    } ;
			 } else
			 { if (!FindNextFile(ifi->internal1,&FileData))
			    { FindClose(ifi->internal1) ; i = FALSE ; break ; } ;
			 } ;
			strcpy(ifi->filename,ifi->directorypath) ; strcat(ifi->filename,FileData.cFileName) ;
			ifi->filebytes = FileData.nFileSizeLow ;
			FileTimeToLocalFileTime(&FileData.ftLastWriteTime,&FileData.ftLastAccessTime) ;
			FileTimeToSystemTime(&FileData.ftLastAccessTime,&systime) ;
			ifi->filetype = (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? 1 : 0 ;
			ifi->updatedt = mscu_ymd_to_ud(systime.wYear,systime.wMonth,systime.wDay) ;
			ifi->updatedt = (ifi->updatedt - 44240)*86400 + systime.wHour*3600 + systime.wMinute*60 + systime.wSecond ;
			i = TRUE ; break ;
#endif
no_more_files:
			i = FALSE ; break ;
		   case V3_FLAGS_SYS_OS:
			psi->stack_ptr = bsp ;
#ifdef WINNT
			if (GetVersion() < 0x80000000) { PUSHMP("WINNT") ; } else { PUSHMP("WINDOWS") ; } ;
#else
			PUSHMP(V3_CONFIG_OS) ;
#endif
			PUSHF(V3_FORMAT_VARSTR) ; goto sys_info_end ;
		   case V3_FLAGS_SYS_OS_ID:
#ifdef VMSOS
			jpi_table.buffer_length = 8 ; jpi_table.item_code = SYI$_BOOTTIME ;
			jpi_table.buffer_address = &user_id.date_time ; sys$getsyi(0,0,0,&jpi_table,0,0,0) ;
			i = user_id.date_time[0]+user_id.date_time[1] ; break ;
#endif
#ifdef WINNT
			i = (time(NULL) - ((GetCurrentTime()+500)/1000)) ;
			i = (i + 5) / 10 ; break ;
#endif
#ifdef UNIX
			if (stat("/usr/bin/reboot.time",&stat_buf) == -1) stat_buf.st_mtime = 0 ;
			i = stat_buf.st_mtime ;
			break ;
#endif
		   case V3_FLAGS_SYS_PROCESS_ID:
#ifdef WINNT
			i = WINNT_PID ; break ;
#endif
#ifdef POSIX
			i = getpid() ; break ;
#endif
		   case V3_FLAGS_SYS_PROCESS_SIZE:
#ifdef VMSOS
			jpi_table.buffer_length = 4 ; jpi_table.item_code = JPI$_FREP0VA ;
			jpi_table.end_of_list = 0 ; jpi_table.return_length_address = 0 ;
			jpi_table.buffer_address = &i ; sys$getjpiw(0,0,0,&jpi_table,0,0,0) ;
			break ;
#endif
			/* HPXXX */ i = 0 ;
			break ;
		   case V3_FLAGS_SYS_REAL_DATE: /* Return current date - Universal format */
			tt = time(NULLV) - (60*mscu_minutes_west()) ; i = TIMEOFFSETDAY + tt/(60*60*24) ; break ;
		   case	V3_FLAGS_SYS_REAL_TIME_OF_DAY:
			tt = time(NULLV)-(60*mscu_minutes_west()) ; i = tt % (60*60*24) ; break ;
		   case V3_FLAGS_SYS_TERMINAL_NAME:
#ifdef WINNT
			bp1 = (char *)stru_alloc_bytes(50) ;	/* Get temp buffer */
			ac = 50 ; WNetGetUser(NULL,bp1,&ac) ;	/* Defined in mpr.lib! */
#endif
#ifdef POSIX
			bp1 = ctermid(NULLV) ;
#endif
			tmp_val.all = V3_FORMAT_VARSTR ; tmp_val.fld.length = strlen(bp1) ;
			psi->stack_ptr = bsp ; PUSHMP(bp1) ; PUSHF(tmp_val.all) ;
			goto sys_info_end ;
		   case V3_FLAGS_SYS_TIME_OF_DAY: /* Current time */
			if (v3_param_time >= 0) { i = v3_param_time ; break ; } ;
			tt = time(NULLV)-(60*mscu_minutes_west()) ; i = tt % (60*60*24) ;
			break ;
		   case V3_FLAGS_SYS_USER_ID:
#ifdef WINNT
			user_id.pid = WINNT_PID ; user_id.date_time[0] = time(0) ;
			bp1 = (char *)stru_alloc_bytes(12) ; memcpy(bp1,&user_id,12) ;
#endif
#ifdef VMSOS
			jpi_table.buffer_length = 4 ; jpi_table.item_code = JPI$_PID ;
			jpi_table.end_of_list = 0 ; jpi_table.return_length_address = 0 ;
			jpi_table.buffer_address = &user_id.pid ; sys$getjpiw(0,0,0,&jpi_table,0,0,0) ;
user_id.pid = getpid() ;
			jpi_table.buffer_length = 8 ; jpi_table.item_code = JPI$_LOGINTIM ;
			jpi_table.buffer_address = &user_id.date_time ; i = sys$getjpiw(0,0,0,&jpi_table,0,0,0) ;
			bp1 = (char *)stru_alloc_bytes(12) ; memcpy(bp1,&user_id,12) ;
			psi->stack_ptr = bsp ; PUSHMP(bp1) ;
			tmp_val.all = V3_FORMAT_FIXSTR ; tmp_val.fld.length = 12 ; PUSHF(tmp_val.all) ;
			goto sys_info_end ;
#endif
#ifdef POSIX
			user_id.pid = getpid() ; user_id.date_time[0] = time(0) ;
			bp1 = (char *)stru_alloc_bytes(12) ; memcpy(bp1,&user_id,12) ;
#endif
			psi->stack_ptr = bsp ; PUSHMP(bp1) ;
			tmp_val.all = V3_FORMAT_FIXSTR ; tmp_val.fld.length = 12 ; PUSHF(tmp_val.all) ;
			goto sys_info_end ;
		   case V3_FLAGS_SYS_USER_NAME:
			bp1 = NULLV ;
#ifdef VMSOS
			bp1 = cuserid(NULL) ;
#endif
#ifdef WINNT
			bp1 = (char *)stru_alloc_bytes(50) ;	/* Get temp buffer */
			ac = 50 ; GetUserName(bp1,&ac) ;
#endif
#ifdef UNIX
			bp1 = getlogin() ;
#endif
#ifdef POSIX
			if (bp1 == NULLV) { sprintf(tmpstr,"Batch:%d",getpid()) ; bp1 = tmpstr ; } ;
#endif
			if (bp1 == NULLV) bp1 = "*Unknown*" ;
			tmp_val.all = V3_FORMAT_VARSTR ; tmp_val.fld.length = strlen(bp1) ;
			psi->stack_ptr = bsp ; PUSHMP(bp1) ; PUSHF(tmp_val.all) ;
			goto sys_info_end ;
		 } ;
		psi->stack_ptr = bsp ; PUSHINT(i) ; break ;
sys_info_end:
		break ;
	   case V3_XCT_SYS_SET_PARAMETER:
		bsp = psi->stack_ptr + (2*SIZEOFSTACKENTRY) ; POPFV ; i = xctu_popint() ;
		psi->stack_ptr = bsp - (2*SIZEOFSTACKENTRY) ;
		switch (i)
		 { default: v3_error(V3E_BADSYSSETPARAMVAL,"SYS","SET_PARAMETER","BADSYSSETPARAMVAL","Invalid argument to SYS_SET_PARAMETER",(void *)i) ;
		   case V3_FLAGS_SYS_DIRECTORY_PATH:
			str_ptr = stru_popcstr(tmpstr) ;
#ifdef WINNT
			SetCurrentDirectory(str_ptr) ;
#else
#ifdef POSIX
			if (chdir(str_ptr) == -1)
			 v3_error(V3E_INVDIR,"SYS","SET_PARAMETER","INVDIR","Invalid directory name specification",NULL) ;
			break ;
#else
			v3_error(V3E_NOTIMP,"SYS","SET_PARAMETER","NOTIMP","Feature not imnplemented",NULL) ;
#endif
#endif
		 } ;
		psi->stack_ptr = bsp ; break ;
	   case V3_XCT_TKN_NEXT:
		i = xctu_popint() ; stru_popstr(&strref) ;
		if (stru_sslen(&strref) != sizeof *tcb)
		 v3_error(V3E_INVTCBLEN,"TKN","NEXT","INVTCBLEN","Invalid length for token-control-block",(void *)0) ;
		PUSHINT(v4lex_NextTkn((struct V4LEX__TknCtrlBlk *)strref.ptr,i)) ;
		break ;
	   case V3_XCT_TKN_PARSE:
		i = xctu_popint() ;
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_FIXSTR || tmp_val.fld.type == VFT_FIXSTRSK)
		 { stru_popstr(&strref) ;
		   if (stru_sslen(&strref) != sizeof *tcb)
		    v3_error(V3E_INVTCBLEN,"TKN","PARSE","INVTCBLEN","Invalid length for token-control-block",(void *)0) ;
		 } else { strref.ptr = (char *)xctu_popptr() ; } ;
		bp1 = strref.ptr ;	/* Pointer to tcb */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_FIXSTR || tmp_val.fld.type == VFT_FIXSTRSK)
		 { stru_popstr(&strref) ;
		   if (stru_sslen(&strref) != sizeof *rpp)
		    v3_error(V3E_INVRPPLEN,"TKN","PARSE","INVRPPLEN","Invalid length for parse-block",0) ;
		 } else { strref.ptr = (char *)xctu_popptr() ; } ;
		PUSHINT(v4lex_RevPolParse((struct V4LEX__RevPolParse *)strref.ptr,(struct V4LEX__TknCtrlBlk *)bp1,i,ctx)) ;
		break ;
	   case V3_XCT_TKN_PARSE_OPT:
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_FIXSTR || tmp_val.fld.type == VFT_FIXSTRSK)
		 { stru_popstr(&strref) ;
		   if (stru_sslen(&strref) != sizeof *rpp)
		    v3_error(V3E_INVRPPLEN,"TKN","PARSEOPT","INVRPPLEN","Invalid length for parse-block",0) ;
		 } else { strref.ptr = (char *)xctu_popptr() ; } ;
		bp1 = strref.ptr ;	/* Pointer to src */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_FIXSTR || tmp_val.fld.type == VFT_FIXSTRSK)
		 { stru_popstr(&strref) ;
		   if (stru_sslen(&strref) != sizeof *rpp)
		    v3_error(V3E_INVRPPLEN,"TKN","PARSEOPT","INVRPPLEN","Invalid length for parse-block",0) ;
		 } else { strref.ptr = (char *)xctu_popptr() ; } ;
//		v4lex_OptRevPol(strref.ptr,bp1) ;
		break ;
	   case V3_XCT_TKN_SET:
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_FIXSTR || tmp_val.fld.type == VFT_FIXSTRSK || tmp_val.fld.type == VFT_VARSTR)
		 { stru_popcstr(tmpstr) ; } ;
		i = xctu_popint() ;			/* Get /set-code/ */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_FIXSTR || tmp_val.fld.type == VFT_FIXSTRSK)
		 { stru_popstr(&strref) ;
		   if (stru_sslen(&strref) != sizeof *tcb)
		    v3_error(V3E_INVTCBLEN,"TKN","SET","INVTCBLEN","Invalid length for token-control-block",(void *)sizeof *tcb) ;
		 } else { strref.ptr = (char *)xctu_popptr() ; } ;
		switch (i)
		 { default: v3_error(V3E_INVSETARG,"XCT","TKN_SET","INVSETARG","Invalid set-code (arg #2) in TKN_SET",(void *)i) ;
		   case V3_FLAGS_IOM_INIT:		v4lex_InitTCB((struct V4LEX__TknCtrlBlk *)strref.ptr,0) ; break ;
		   case V3_FLAGS_TKN_PROMPT:		v4lex_SetPrompt((struct V4LEX__TknCtrlBlk *)strref.ptr,ASCretUC(tmpstr)) ; break ;
		   case V3_FLAGS_TKN_PUSH_FILE:
			{ struct UC__File UCFile ; UCCHAR errmsg[256] ;
			  if (!v_UCFileOpen(&UCFile,ASCretUC(tmpstr),UCFile_Open_Read,TRUE,errmsg,0))
			   v3_error(V3E_NOFILE,"XCT","TKN_PUSH","NOFILE","Could not access include file",(void *)0) ;
			  v4lex_NestInput((struct V4LEX__TknCtrlBlk *)(strref.ptr),&UCFile,ASCretUC(tmpstr),i) ; break ;
			}
		   case V3_FLAGS_TKN_PUSH_STDIN:	v4lex_NestInput((struct V4LEX__TknCtrlBlk *)strref.ptr,0,UClit("V4TKN>"),i) ; break ;
		   case V3_FLAGS_TKN_PUSH_STRING:	v4lex_NestInput((struct V4LEX__TknCtrlBlk *)strref.ptr,0,ASCretUC(tmpstr),i) ; break ;
		   case V3_FLAGS_TKN_FORCE_NEW_LINE:	break ;
		 } ;
		break ;
	   case V3_XCT_TTY_GET:
	   case V3_XCT_TTY_GETC:
/*		Update j with input length */
		j = (code == V3_XCT_TTY_GETC ? 1 : sizeof ttystr) ;
/*		Do we have a prompt ?	*/
		POPF(tmp_val.all) ;
		if (tmp_val.all == V3_FORMAT_EOA)
		 { POPVI(i) ; iou_input_tty("",ttystr,j,FALSE) ; }
		 else
		 { PUSHF(tmp_val.all) ; iou_input_tty(stru_popcstr(tmpstr),ttystr,j,FALSE) ;
		   POPFV ;	/* Get rid of EOA */
		 } ;
/*		Return string result */
		PUSHMP(&ttystr) ;
		tmp_val.fld.type = VFT_FIXSTR ; tmp_val.fld.mode = VFM_PTR ; tmp_val.fld.length = strlen(ttystr) ;
		PUSHF(tmp_val.all) ; break ;
	   case V3_XCT_TTY_MISC:
#ifdef VMSOS
		vax_tty_misc() ; break ;
#endif
		iou_tty_misc() ;
		break ;
	   case V3_XCT_V3_XCTRPPEXP:
		stru_popstr(&strref) ;
		if (stru_sslen(&strref) != sizeof *vrx)
		 v3_error(V3E_INVRPPSIZE,"V3","XCTRPPEXP","INVRPPSIZE","Invalid size of argument",(void *)sizeof *vrx) ;
		vrpp_Execute(ctx,(struct VRPP__Xct *)strref.ptr) ;
		break ;
	 } ;
}
