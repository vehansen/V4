/*	V3DRIVER.C - DRIVER PROGRAM FOR V3 SYSTEM

	Last edited 7/23/84 by Victor E. Hansen		*/

#include <signal.h>
#include <time.h>
#include <string.h>
#include "v3defs.c"
#ifdef UNIX
#ifdef INCSYS
#include <sys/times.h>
#else
#include <times.h>
#endif
#include <termio.h>
#endif /* POSIX */
#ifdef ALPHAOSF
#include <sys/sysinfo.h>
#include <sys/proc.h>
#endif

struct db__package *pcku_xct_load() ;
struct db__process *process_init_new() ;
struct db__psi *xctu_cmd_invoke() ;
struct db__module_entry *pcku_module_defined( /* module */ ) ;
void ctrlp() ;
struct V4DPI__Point *v4dpi_IsctEval() ;
struct V4C__Context *v3v4_GetV4ctx() ;

/*	Set up for ^P trapping */
#ifdef VMSOS
struct {
  int zeros ;
  int mask ;
 } ctrl ;
static short int channel ;
static struct {
  short int len,desc ;
  char *ptr ;
 } devdesc ;
globalvalue IO$_SETMODE,IO$M_OUTBAND ;
#endif


/*	Global Pointer to Startup Environment */

char **startup_envp ;

main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
{
   struct db__process *process ;
   struct db__package *pckp ;
   struct db__parse_info *prsp ;
   struct V4C__Context *ctx ;
   struct db__module_entry *mep ;
   int i,len ; char tbuf[150],startup_package[150] ; char *bp1,*bp2 ;
   void ctrlp() ; void exit_handler(),sigint_exit_handler() ; void v3_signal_ill(),v3_signal_fpe(),v3_signal_bus() ;
#ifdef VMSOS
   void vms_ctrlp() ;
   void v3_signal_quit() ; void vax_signal_ill() ; void vax_signal_fpe() ;
#endif
#ifdef WINNT
   void v3_signal_quit() ;
#endif
#ifdef POSIX
   void v3_signal_quit() ; void unix_ctrlp() ;
#endif
#ifdef ALPHAOSF
  int uacbuf[2] ;
#endif

#ifdef ALPHAOSF
	uacbuf[0] = SSIN_UACPROC ; uacbuf[1] = UAC_NOPRINT | UAC_SIGBUS | UAC_NOFIX ; /* Want unaligned access to blow big time! */
	setsysinfo(SSI_NVPAIRS,&uacbuf,1,0,0) ;
#endif
#ifdef VMSOS
/*	Initialize sys$input on channel */
	devdesc.len = 11 ; devdesc.desc = 0 ; devdesc.ptr = "SYS$COMMAND" ; sys$assign(&devdesc,&channel,0,0) ;
/*	Set up qio for ^P interrupt */
	ctrl.mask = 0x10000 ;	/* Set to interrupt on ^P */
	sys$qiow(0,channel,IO$_SETMODE | IO$M_OUTBAND,0,0,0,vms_ctrlp,&ctrl,0,0,0,0) ;
#endif

	process = process_init_new() ;
/*	Set up exit handler & ^Y(^C) handler */
	atexit(exit_handler) ; signal(SIGINT,sigint_exit_handler) ; signal(SIGTERM,sigint_exit_handler) ;
/*	Save the startup command line */
	process->v3_start_command_line[0] = NULLV ; startup_package[0] = 0 ;
	for(i=0;i<argc;i++)
	 { strncat(process->v3_start_command_line,argv[i],(sizeof process->v3_start_command_line)-1) ; strncat(process->v3_start_command_line," ",(sizeof process->v3_start_command_line)-1) ;
	   if (i == 1) strcpy(startup_package,argv[i]) ;
	 } ;

/*	Save startup environment */
	startup_envp = envp ;
#ifdef UNIX
	signal(SIGTSTP,unix_ctrlp) ;
	process->delta_time = time(NULLV) ;		/* Get starting time */
	times(&process->delta_tms) ;			/*  and cpu */
#endif
/*	Set up special signal handlers */
#ifdef VMSOS
	signal(SIGILL,vax_signal_ill) ; signal(SIGFPE,vax_signal_fpe) ;
	signal(SIGQUIT,v3_signal_quit) ; signal(SIGBUS,v3_signal_bus) ;
#else
	signal(SIGILL,v3_signal_ill) ; signal(SIGFPE,v3_signal_fpe) ;
#endif
#ifdef ALPHAOSF
	signal(SIGSEGV,v3_signal_bus) ;
#endif
#ifdef UNIX
	signal(SIGQUIT,v3_signal_quit) ; signal(SIGBUS,v3_signal_bus) ; signal(SIGSEGV,v3_signal_bus) ;
	setvbuf(stdout,NULLV,_IONBF,0) ;		/* Disable all buffering on controlling terminal */
#endif
#ifdef WINNT
	signal(SIGABRT,v3_signal_fpe) ; signal(SIGTERM,v3_signal_quit) ;
#endif

/*	Check for xv3 switches */
	for(i=1;i<argc;)
	 { if (*(argv[i]) != '-')
	    { strcpy(process->v3_start_command_line,argv[0]) ; strcpy(startup_package,argv[i]) ;
	      for(;i<argc;i++) { strcat(process->v3_start_command_line," ") ; strcat(process->v3_start_command_line,argv[i]) ; } ;
	      break ;
	    } ;
	   startup_package[0] = 0 ;
	   i += v3_driver_option_handler(argv[i],(i+1 < argc ? argv[i+1] : NULL)) ;
	 } ;
/*	Did we get a DCL argument - a package to execute ? */
	if (strlen(startup_package))
	 { switch (setjmp(process->v3_error_jmp))
	    { case 0: break ;
	      default: exit(process->exit_value=EXIT_SUCCESS) ;
	    } ;
	   process->ctx = v3v4_GetV4ctx() ;
	   pckp = (struct db__package *)pcku_xct_load(startup_package,TRUE) ;	/* Go get it */
/*	   Look for startup symbol in global table */
	   if (pckp->startup_module[0] == 0) v3_error(V3E_NOSTART,"DRVR","MAIN","NOSTART","No startup module declared for package",0) ;
	   if ((mep = pcku_module_defined(pckp->startup_module)) == NULLV)
	    v3_error(V3E_STARTUPMOD,"DRVR","MAIN","STARTUPMOD","Startup module not found in package",0) ;
/*	   Start up the procedure */
	   xctu_call_mod(mep) ; xctu_main() ;
	   exit(process->exit_value=EXIT_SUCCESS) ;
	 } ;
/*	Init a process & the interpretive package	*/
	pcku_parse_init(V3_INTERPRETIVE_PACKAGE_ID,"BOOT") ;
/*	Now loop forever	*/
	for (;;)
	 { switch (setjmp(process->v3_error_jmp))
	    { case 0: xctu_call_parser(1,0,"V3>",TRUE) ; break ;
	      case 1: xctu_call_parser(1,0,"V3>",FALSE) ; break ;
	    } ;
	 } ;
}

int v3_driver_option_handler(sarg,varg)
  char *sarg ;
  char *varg ;
{
  struct db__process *process ;
  struct db__package *pckp ;
  struct db__parse_info *prsp ;
  struct V4C__Context *ctx ;
  struct V4DPI__Point point,isctpt,*dpt ;
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4IS__ParControlBlk pcb ;
  struct V4C__AreaHInfo ahi ;
  FILE *fp ;
  int len,capc,port ; char tbuf[150],fnbuf[150] ; char *bp1,*bp2 ;

/*	Branch to proper switch handler		*/
#define CHKV if (varg == NULL) { printf("? Switch (%s) requires an argument\n",sarg) ; exit(EXIT_FAILURE) ; } ;
#define CTXINIT ctx = v3v4_GetV4ctx() ;
	capc = FALSE ;
	switch (*(sarg+1))
	 { default:
		printf("? Invalidi startup switch- %s\n",sarg) ; exit(EXIT_FAILURE) ;
	   case 'a':		/* -a areaname */
		CTXINIT CHKV
		memset(&ahi,0,sizeof ahi) ;
		ahi.RelHNum = 0 ; ahi.ExtDictUpd = TRUE ; ahi.IntDictUpd = TRUE ; ahi.BindingsUpd = TRUE ;
		memset(&pcb,0,sizeof pcb) ;
		strcpy(pcb.V3name,"areau") ; UCstrcpy(pcb.UCFileName,ASCretUC(varg)) ;
		if (UCstrchr(pcb.UCFileName,'.') == NULL) UCstrcat(pcb.UCFileName,UClit(".v4a")) ;
		pcb.AccessMode = -1 ; pcb.OpenMode = V4IS_PCB_OM_Read ;
		v4is_Open(&pcb,NULL,NULL) ;
		if (v4ctx_AreaAdd(ctx,&pcb,&ahi,NULL) == UNUSED) { printf("? %s - %s",varg,UCretASC(ctx->ErrorMsgAux)) ; exit(EXIT_FAILURE) ; } ;
		return(2) ;
	   case 'C':		/* -C v3program */
		capc = TRUE ;
	   case 'c':		/* -c v3program */
		CHKV strcpy(fnbuf,varg) ;
		if (strchr(fnbuf,'.') == NULL) strcat(fnbuf,".v3") ;
		strcpy(fnbuf,(char *)v3_logical_decoder(fnbuf,TRUE)) ;
		fp = fopen(fnbuf,"r") ;
		if (fp == NULLV)
		 { strcpy(tbuf,"v3_include:") ;		/* Try via "v3_include:" */
	           strcat(tbuf,fnbuf) ;
	           strcpy(tbuf,(char *)v3_logical_decoder(tbuf,TRUE)) ; strcpy(fnbuf,tbuf) ;
	           fp = fopen(fnbuf,"r") ;
	           if (fp == NULLV) { printf("? Error (%s) accessing file: %s\n",v_OSErrString(errno),fnbuf) ; exit(EXIT_FAILURE) ; } ;
		 } ;
		if (capc) printf("Compiling: %s\n",fnbuf) ;
/*		If package name prefaced with a logical then strip off */
		bp2 = varg ;
		if ((bp1 = (char *)strchr(bp2,':')) != 0) { bp2 = bp1 + 1 ; }
		 else { if ((bp1 = (char *)strchr(bp2,']')) != 0) bp2 = bp1 + 1 ; } ;
		process = process_init_new() ;
		switch (setjmp(process->v3_error_jmp))
		 { case 0: break ;
		   default: exit(process->exit_value=EXIT_SUCCESS) ;
		 } ;
		pcku_parse_init(0,bp2) ; pckp = (struct db__package *)pcku_find(bp2,TRUE) ;
		prsp = pckp->parse_ptr ; prsp->in_pck_compile = TRUE ; prsp->ctx = v3v4_GetV4ctx() ; process->ctx = prsp->ctx ;
		xctu_call_parser(pckp->package_id,fp,fnbuf,TRUE) ;
/*		If any parser errors then abort (don't save) */
		if (prsp->error_flag > 0) exit(process->exit_value=44) ;
		strcpy(fnbuf,bp2) ; bp2 = fnbuf ;
		strcat(bp2,".v3x") ; pcku_save(pckp,bp2) ;
		if (prsp->ctx != NULLV) v4ctx_AreaClose(prsp->ctx,-1) ;	/* Close any areas */
		exit(process->exit_value=EXIT_SUCCESS) ;
	   case 'e':		/* -e "v4eval statement" */
		CTXINIT CHKV v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(varg),V4LEX_InpMode_String) ;
		v4eval_Eval(&tcb,v3v4_GetV4ctx(),FALSE,FALSE,FALSE,FALSE,FALSE,NULL) ;
		return(2) ;
	   case 'f':		/* -f includefile */
		CHKV strcpy(tbuf,varg) ; if (strchr(tbuf,'.') == NULL) strcat(tbuf,".v3s") ;
		strcpy(tbuf,(char *)v3_logical_decoder(tbuf,TRUE)) ; fp = fopen(tbuf,"r") ;
		if (fp == NULLV)
		 { printf("? Error (%s) accessing -f file (%s) to load symbols\n",v_OSErrString(errno),tbuf) ; exit(EXIT_FAILURE) ; } ;
		for(;;)
		 { if (fgets(tbuf,sizeof tbuf,fp) == NULL) break ;
		   if (tbuf[0] == '!') continue ; if (tbuf[0] == '/') continue ;
		   len = strlen(tbuf) ; if (tbuf[len-1] == '\n') tbuf[len-1] = 0 ;
		   bp1 = strchr(tbuf,' ') ;
		   if (bp1 == NULL) { v3_driver_option_handler(tbuf,NULL) ; }
		    else { *bp1 = 0 ; v3_driver_option_handler(tbuf,bp1+1) ; } ;
		 } ;
		fclose(fp) ; return(2) ;
	   case 'h':
		printf("\n\
V3 (v%d.%d) command line switches\n\
  -a file		Link in the V4 area spcified by file\n\
  -c package		Compile package into package.v3x\n\
  -C package		  same as above except output filename\n\
  -e \"V4 statement\"	Evaluates the V4 statement\n\
  -f file		Reads all lines in file.v4s and treats each\n\
			 line as a V3 switch (useful for setting up\n\
			 a list of symbols)\n\
  -l file		Compile the file into current V3 runtime\n\
  -o optionlist		Saves optionlist for reference via-\n\
			  v3_info(/startup_options/)\n\
  -p dim:point		Adds point to V4 context\n\
  -P			Turn off ^P trapping\n\
  -s sym=value		Assigns value to sym(bol)\n\
			  Symbol may be used as a logical\n\
			  Note: overrides any operating system values\n\
  -t \"string\"		Next request for terminal input gets string\n\
			  Use of multiple -t switches allowed\n\
  -v 			Outputs the current V3/V4 version numbers\n\
  -w node.port		Handle all \"terminal\" I/O through socket\n\n",
		V3_VERSION_MAJOR,V3_VERSION_MINOR) ;
		return(1) ;
	   case 'l':			/* -l v3file */
		process = process_init_new() ; pcku_parse_init(V3_INTERPRETIVE_PACKAGE_ID,"BOOT") ;
		CHKV bp1 = varg ;
		if ((fp = fopen((char *)v3_logical_decoder(bp1,TRUE),"r")) == NULLV)
		 { if (strchr(bp1,'.') != NULLV)
		   { printf("? Could not access file (%s) to load\n",bp1) ; exit(EXIT_FAILURE) ; } ;
/*		   No extension - append ".v3" */
		   strcpy(tbuf,bp1) ; bp1 = tbuf ; strcat(tbuf,".v3") ; strcpy(tbuf,(char *)v3_logical_decoder(tbuf,TRUE)) ;
		   if ((fp = fopen(tbuf,"r")) == NULLV)
		    { printf("? Could not access file (%s) to load\n",tbuf) ; exit(EXIT_FAILURE) ; } ;
		 } ;
		process->ctx = v3v4_GetV4ctx() ; process->package_ptrs[V3_INTERPRETIVE_PACKAGE_ID]->parse_ptr->ctx = v3v4_GetV4ctx() ;
		prsu_nest_input(process->package_ptrs[V3_INTERPRETIVE_PACKAGE_ID]->parse_ptr,NULL,"V3>") ;
		prsu_nest_input(process->package_ptrs[V3_INTERPRETIVE_PACKAGE_ID]->parse_ptr,fp,bp1) ;
		for (;;)
		 { switch (setjmp(process->v3_error_jmp))
		    { case 0: xctu_call_parser(1,0,"V3>",FALSE) ; break ;
		      case 1: xctu_call_parser(1,0,"V3>",FALSE) ; break ;
		    } ;
		 } ;
	   case 'o':		/* -o optionlist */
		process = process_init_new() ;
		CHKV strcpy(process->v3_startup_option_list,varg) ; return(2) ;
	   case 'p':		/* -p dim:point */
		CTXINIT CHKV v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(varg),V4LEX_InpMode_String) ;
		v4dpi_PointParse(v3v4_GetV4ctx(),&isctpt,&tcb,0) ;
		if (isctpt.PntType != V4DPI_PntType_Isct) { dpt = &isctpt ; }
		 else { dpt = v4dpi_IsctEval(&point,&isctpt,v3v4_GetV4ctx(),0,NULL,NULL) ; } ;
		v4ctx_FrameAddDim(v3v4_GetV4ctx(),V4C_FrameId_Real,dpt,0,0) ;
		return(2) ;
	   case 'P':
#ifdef SIGTSTP
		signal(SIGTSTP,SIG_IGN) ; return(1) ;
#endif
	   case 's':			/* -s sym=value */
		CHKV strcpy(tbuf,varg) ;
		if ((bp1 = (char *)strchr(tbuf,'=')) == NULL)
		 { printf("? Invalid -s syntax (%s) - Missing equal sign\n",tbuf) ; exit(EXIT_FAILURE) ; } ;
		*bp1 = 0 ; vlog_StoreSymValue(ASCretUC(tbuf),ASCretUC(bp1+1)) ;
		return(2) ;
	   case 't':		/* -t "next terminal input value" */
		CHKV iou_input_tty(varg,NULL,0,0) ;
		return(2) ;
	   case 'v':			/* -v */
		printf("V3 Compiler/Runtime v%d.%d - 2009..2012\n",V3_VERSION_MAJOR,V3_VERSION_MINOR) ;
		printf("V4 %d.%d - \"Data that thinks like us\"\nProtected by U.S. Patent 6,470,490 - MKS Inc. (2002..2012)\n",V4IS_MajorVersion,V4IS_MinorVersion) ;
		return(1) ;
#ifdef ISOCKETS
	   case 'w':			/* -w node.port - bind terminal io to socket */
		CHKV ; strcpy(tbuf,varg) ;
		bp1 = strrchr(tbuf,'.') ;			/* Look for .portnum */
		if (bp1 == NULL) { printf("? Invalid host.port (%s) - Missing port number\n",tbuf) ; exit(EXIT_FAILURE) ; } ;
		*bp1 = 0 ;					/* Terminate hostname with null */
		port = strtol(bp1+1,&bp2,10) ;	if (*bp2 != 0) port = -1 ;
		if (port < 1000 || port > 9999)
		 { printf("? Error in port number (%s) - Not a number or out of range\n",bp1+1) ; exit(EXIT_FAILURE) ; } ;
		iou_OpenTerminalSocket(tbuf,port,FALSE) ;	/* Open terminal socket for duration of xv3 execution */
#ifdef WINNT
		printf("V3 Process (%x) handling all terminal I/O via socket!\n",WINNT_PID) ;
#endif
		return(2) ;
#endif
	 } ;
}

/*	Logical Device Decoder */


char *v3_logical_decoderx(cspec,exists,index,errmsg)
  char *cspec ;
  LOGICAL exists ;			/* TRUE if looking for existing file, FALSE for new file */
  int index ;			/* If > 0 then return index'th path in logical definitions */
  UCCHAR *errmsg ;		/* If not NULL then updated with any error messages */
{ static char ffn[200],normspec[200] ;
   char *b,*b1,*ns,*spec ;
   char *comma,*colon ; char firstffn[300],path[500],npath[500] ; UCCHAR upath[500] ;
   int i,cnt ; FILE *tf ;

#ifdef VMSOS
	return(cspec) ;						/* VMS does all this for us */
#endif
/*	Convert to lowercase */
#ifdef WINNT
	for((b=normspec,spec=cspec);*spec!='\0';spec++) { *(b++) = tolower(*spec) ; } ; *b = '\0' ;
	ns = strstr(normspec,"  .") ;				/* Any spaces before extension? */
	if (ns != NULL)
	 { for(;*ns == ' ';ns--) {} ;				/* Backup to last non-space */
	   for(i=TRUE,b1=path,b=normspec;;)
	    { if (!i && *b == '.') i = TRUE ;
	      if (i) *(b1++) = *b ; if (*b == '\0') break ;
	      if (b == ns) i = FALSE ; b++ ;
	    } ; strcpy(normspec,path) ;
	 } ;
	spec = normspec ;
	if (*(spec+1) == ':') return(index > 1 ? NULL : spec) ;		/* Don't decode if single letter device: c:xxx */
#endif
#ifdef UNIX
	for((b=normspec,spec=cspec);*spec!=NULL;spec++) { if (*spec > ' ') *(b++) = tolower(*spec) ; } ;
	spec = normspec ; *b = NULL ;
#endif
	if (*spec == '$') spec++ ;				/* Get rid of possible leading $ */
	if ((b = (char *)strrchr(spec,';')) != NULL) *b = '\0' ;	/* Strip of trailing ";xxx" - VMS generation numbers */
/*	First see if we have a device (xxx:file) */
	if ((colon = (char *)strchr(spec,':')) == 0)
#ifdef UNIX
	 return(spec) ;	/* No device - just return argument */
#endif
#ifdef WINNT
	 return(index > 1 ? NULL : spec) ;	/* No device - just return argument */
#endif
/*	Got a colon - lets see if we can decode it */
	strncpy(ffn,spec,colon-spec) ; ffn[colon-spec] = 0 ;	/* Copy up to the colon */
/*	Now zip-zop thru the environment to see we have an path */
	if (!v_UCGetEnvValue(ASCretUC(ffn),upath,UCsizeof(upath)))
	 { 
	   if (errmsg != NULL)
	    { v_Msg(NULL,errmsg,"LogDecNoLog",ffn,cspec) ; return(NULL) ; } ;
	   sprintf(path,"Could not evaluate logical (%s) in file (%s)",ffn,cspec) ;
	   v3_error(V3E_NOLOGICAL,"VCOM","LOGEXP","NOLOGICAL",path,0) ;
	 } ; UCstrcpyToASC(path,upath) ;
/*	Loop thru all directories specified */
	ZS(firstffn) ;
	if (index > 0) exists = TRUE ;			/* Set to TRUE to force sequence through logical defs */
	for(cnt=1;;cnt++)
	 { if (strlen(path) == 0)			/* If nothing in path, return current file */
	    { if (index > 0) return(NULL) ;		/* If looking for n'th then return null (end of list) */
	      if (firstffn[0] != '\0') strcpy(ffn,firstffn) ;
	      return(ffn) ;
	    } ;
	   if ((comma = (char *)strchr(path,',')) == 0) /* Look for "," in path */
	    {
	      if (path[strlen(path)-1] == ':')		/* Got a nested logical ? */
	       { path[strlen(path)-1] = 0 ;
	         if (!v_UCGetEnvValue(ASCretUC(path),upath,UCsizeof(upath)))
	          { if (errmsg != NULL) { v_Msg(NULL,errmsg,"LogDecNested",path,cspec) ; return(NULL) ; } ;
		    sprintf(ffn,"Could not evaluate nested logical (%s)",path) ;
	            v3_error(V3E_NONESTEDLOGICAL,"VCOM","LOGEXP","NONESTEDLOGICAL",ffn,0) ;
	          } ; UCstrcpyToASC(ffn,upath) ;
	       } else { strcpy(ffn,path) ; } ;
#ifdef WINNT
	      if (ffn[strlen(ffn)-1] != '\\') strcat(ffn,"\\") ;
#endif
#ifdef UNIX
	      if (ffn[strlen(ffn)-1] != '/') strcat(ffn,"/") ;
#endif
	      strcat(ffn,colon+1) ;	/* And remainder of file spec */
#ifdef WINNT
	      i = strlen(ffn) ; if (ffn[i-1] == '\\') ffn[i-1] = 0 ;	/* Don't return with trailing "/" */
#endif
#ifdef UNIX
	      i = strlen(ffn) ; if (ffn[i-1] == '/') ffn[i-1] = 0 ;	/* Don't return with trailing "/" */
#endif
	      return(index > cnt ? NULL : ffn) ;
	    } ;
	   strncpy(ffn,path,comma-path) ; ffn[comma-path] = 0 ; strcpy(path,&path[1+comma-path]) ;
	   if (ffn[strlen(ffn)-1] == ':')		/* Got a nested logical ? */
	    { ffn[strlen(ffn)-1] = 0 ;
	      if (!v_UCGetEnvValue(ASCretUC(ffn),upath,UCsizeof(upath)))
	       { if (errmsg != NULL) { v_Msg(NULL,errmsg,"LogDecNested",path,cspec) ; return(NULL) ; } ;
	         sprintf(npath,"Could not evaluate nested logical (%s)",ffn) ;
	         v3_error(V3E_NOLOGICAL,"VCOM","LOGEXP","NOLOGICAL",npath,0) ;
	       } ; UCstrcpyToASC(npath,upath) ;
	      strcpy(ffn,npath) ;
	    } ;
#ifdef WINNT
	   if (ffn[strlen(ffn)-1] != '\\') strcat(ffn,"\\") ;
#endif
#ifdef UNIX
	   if (ffn[strlen(ffn)-1] != '/') strcat(ffn,"/") ;
#endif
	   strcat(ffn,colon+1) ;	/* And remainder of file spec */
/*	   Try and read this file to make sure it exists */
	   if (index > 0 && cnt == index) return(ffn) ;
	   if ((tf=fopen(ffn,"r")) != NULL) { fclose(tf) ; return(ffn) ; } ;
	   if (!exists)
	    { if (firstffn[0] == '\0') strcpy(firstffn,ffn) ;	/* Save first path in case we don't find file */
	    } ;
	 } ;
}

char *v3_logical_decoder(cspec,exists)	/* Decodes & returns string to expanded file name */
  char *cspec ;
  int exists ;			/* TRUE if looking for existing file, FALSE for new file */
{
	return(v3_logical_decoderx(cspec,exists,0,NULL)) ;
}

char *v_LogicalDecoder(cspec,exists,errmsg)
  char *cspec ;
  int exists ;			/* TRUE if looking for existing file, FALSE for new file */
  UCCHAR *errmsg ;		/* Updated with error msg, routine returns NULL */
{
	return(v3_logical_decoderx(cspec,exists,0,errmsg)) ;
}


#ifdef VMSOS
struct vms__dsc {
  short int length ;		/* Length of value */
  short int desc ;		/* Descriptive info */
  char *pointer ;		/* Pointer to value */
 } ;

/*	vms_input_tty - Inputs from user terminal (SYS$INPUT) */
vms_input_tty(prompt,result,maxlen,parse_flag)
  char *prompt ;		/* Prompt string */
  char *result ;		/* Result string to be updated */
  int maxlen ;			/* Maximum input length */
  int parse_flag ;		/* TRUE iff called from V3 parser otherwise from runtime */
{ struct vms__dsc pdsc,rdsc ;
   char errbuf[250] ;
   short int input_len ; int status ;

/*	Construct string descriptors for vms routines */
	pdsc.length = strlen(prompt) ; pdsc.pointer = prompt ; pdsc.desc = 0 ;
	rdsc.length = maxlen ; rdsc.pointer = result ; rdsc.desc = 0 ;
/*	Call vms routine to do input */
	status = LIB$GET_INPUT(&rdsc,&pdsc,&input_len) ;
/*	Did we get an error, if so then if from parser then exit if EOF */
	if (status != 1)
	 { if (parse_flag) exit(status == RMS$_EOF ? 1 : status) ;
/*	   Running a V3 program, generate a V3 error */
	   if (status == RMS$_EOF) v3_error(V3E_EOF,"TTY","GET","EOF","End of file reached (^Z)",0) ;
	   v3_error(V3E_GET,"TTY","GET","GET",mscu_vms_errmsg(status,errbuf),0) ;
	 } ;
/*	Make sure result ends with a null */
	*(result+input_len) = 0 ;
	return(TRUE) ;
}
#endif