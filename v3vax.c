/*	V3VAX - VAX/VMS Specific Routines

	Created edited 10/22/93 by Victor E. Hansen		*/

#include signal
#include "v3defs.c"
#include sjcdef

static short tty_channel = 0 ;	/* Channel for TTY modules */
static char tty_device_name[100] ; /* Device name for TTY modules */

#define IO_SET_STATIC 1		/* To initialize "static" IO unit */
#define IO_GET_STATIC 2		/* For GET static */
#define IO_PUT_STATIC 4		/* For PUT static */
#define IO_UPDATE_STATIC 8	/* For PUT/Update Static */
#define IO_NULL_STATIC 16	/* To handle NL: device on Unix */

extern char *compact_buffer ;
extern struct db__process *process ;
extern struct db__psi *psi ;
extern struct iou__openlist ounits ;

struct vms__dsc {
  short int length ;		/* Length of value */
  short int desc ;		/* Descriptive info */
  char *pointer ;		/* Pointer to value */
 } ;

/*	S I G N A L   U T I L I T I E S					*/

/*	vax_signal_fpe - Handles Arithmetic Traps			*/

void vax_signal_fpe(sigint,type)
  int sigint,type ;
 { char msg[30],buf[100] ;
   void vax_signal_fpe() ;

	signal(SIGFPE,vax_signal_fpe) ;	/* Reset signal */
	switch(type)
	 { default: sprintf(msg,"* Unknown FPE trap=%d *",type) ; break ;
	   case FPE_INTOVF_TRAP: strcpy(msg,"Integer Overflow") ; break ;
	   case FPE_INTDIV_TRAP: strcpy(msg,"Integer Division by Zero") ; break ;
	   case FPE_FLTOVF_TRAP: strcpy(msg,"Floating Overflow") ; break ;
	   case FPE_FLTDIV_TRAP: strcpy(msg,"Floating/decimal division by zero") ; break ;
	   case FPE_FLTUND_TRAP: strcpy(msg,"Floating underflow") ; break ;
	   case FPE_DECOVF_TRAP: strcpy(msg,"Decimal overflow") ; break ;
	   case FPE_FLTOVF_FAULT: strcpy(msg,"Floating overflow fault") ; break ;
	   case FPE_FLTDIV_FAULT: strcpy(msg,"Floating divide by zero fault") ; break ;
	   case FPE_FLTUND_FAULT: strcpy(msg,"Floating underflow fault") ; break ;
	 } ;
	sprintf(buf,"%s (%d)",msg,process->last_v3_opcode) ; v3_error(V3E_ARITH,"MTH","SIGNAL","ARITH",buf,0) ;
 } ;

/*	vax_signal_ill - Illegal Instruction Trap			*/

void vax_signal_ill(type)
  int type ;
 { char msg[30],buf[100] ;
   void vax_signal_fpe(),vax_signal_ill() ;

	signal(SIGILL,vax_signal_ill) ; signal(SIGFPE,vax_signal_fpe) ; /* Reset signal */
	switch(type)
	 { default: sprintf(msg,"* Unknown ILL trap=%d *",type) ; break ;
	   case ILL_PRIVIN_FAULT: strcpy(msg,"Reserved instruction") ; break ;
	   case ILL_RESOP_FAULT: strcpy(msg,"Reserved operand") ; break ;
	   case ILL_RESAD_FAULT: strcpy(msg,"Reserved addressing") ; break ;
	 } ;
	sprintf(buf,"%s (%d)",msg,process->last_v3_opcode) ; v3_error(V3E_ILL,"XCT","SIGNAL","ILL",buf,0) ;
}

/*	I / O   T O   R M S   I N T E R F A C E			*/


/*	Open RMS File				*/
vms_open(unitp)
  struct iou__unit *unitp ;		/* Pointer to the I/O unit */
{
   struct FAB *fabp ; struct RAB *rabp ; struct NAM *namp ;
   char tmp[10],errbuf[250] ;
   char xnam[V3_IO_FILENAME_MAX] ;	/* Holds expanded file name */
   int rms_res ;			/* Result of system call */
   short int i ;

/*	Allocate & init rms structures */
	fabp = malloc(sizeof *fabp) ;
 *fabp = cc$rms_fab ;
 rabp = malloc(sizeof *rabp) ;
 *rabp = cc$rms_rab ;
	namp = malloc(sizeof *namp) ; *namp = cc$rms_nam ;
	namp->nam$l_rsa = &xnam ; namp->nam$b_rss = V3_IO_FILENAME_MAX ;
	rabp->rab$l_fab = fabp ; fabp->fab$l_nam = namp ;
/*	See if we have file name defaults */
	if (unitp->name_dflt_format.all == 0)
	 { fabp->fab$l_dna = NULL ; fabp->fab$b_dns = 0 ; }
	 else { fabp->fab$l_dna = unitp->name_dflt_ptr ;
		fabp->fab$b_dns =
		 (unitp->name_dflt_format.fld.type == VFT_FIXSTR || unitp->name_dflt_format.fld.type == VFT_FIXSTRSK ?
				     unitp->name_dflt_format.fld.length : strlen(fabp->fab$l_dna)) ;
	      } ;
/*	Link fab address to user's unit */
	unitp->file_ptr = NULL ; unitp->rab_pointer = rabp ;
/*	Set up access */
#define ACC(FLAG,EXT) if ((unitp->file_info & FLAG) != 0) fabp->fab$b_fac |= EXT
	ACC(V3_FLAGS_IOF_GET,FAB$M_GET) ; ACC(V3_FLAGS_IOF_PUT,FAB$M_PUT) ;
	ACC(V3_FLAGS_IOF_UPDATE,FAB$M_UPD) ; ACC(V3_FLAGS_IOF_DELETE,FAB$M_DEL) ; ACC(V3_FLAGS_IOF_DELETE,FAB$M_TRN) ;
/*	Pick up the file name */
	fabp->fab$l_fna = &unitp->file_name ; fabp->fab$b_fns = strlen(unitp->file_name) ;
/*	Max record length */
	fabp->fab$w_mrs = unitp->file_record_length ;
/*	File organization */
#define ORG(FLAG,ORG) if ((unitp->file_info & FLAG) != 0) fabp->fab$b_org = ORG
	ORG(V3_FLAGS_IOF_INDEXED,FAB$C_IDX) ; ORG(V3_FLAGS_IOF_RELATIVE,FAB$C_REL) ;
	ORG(V3_FLAGS_IOF_SEQUENTIAL,FAB$C_SEQ) ;
/*	File Processing Options */
#define FPO(FLAG,FOP) if ((unitp->file_info & FLAG) != 0) fabp->fab$l_fop |= FOP
	FPO(V3_FLAGS_IOF_CREATE_IF,FAB$M_CIF) ; FPO(V3_FLAGS_IOF_DELETE_FILE,FAB$M_DLT) ;
	FPO(V3_FLAGS_IOF_SEQUENTIAL,FAB$M_SQO) ;
/*	Now take care of the record format */
#define REC(FLAG,RFORM) if ((unitp->record_info & FLAG) != 0) fabp->fab$b_rfm = RFORM
	REC(V3_FLAGS_IOR_FIXED_LENGTH,FAB$C_FIX) ; REC(V3_FLAGS_IOR_VARIABLE_LENGTH,FAB$C_VAR) ;
/*	Handle the record attributes */
#define RAT(FLAG,RAP) if ((unitp->record_info & FLAG) != 0) fabp->fab$b_rat = RAP
	RAT(V3_FLAGS_IOR_CARRIAGE_RETURN,FAB$M_CR) ; RAT(V3_FLAGS_IOR_FORTRAN,FAB$M_FTN) ;
/*	Does user want an SOS type file ? */
	if ((unitp->record_info & V3_FLAGS_IOR_SOS) != 0)
	 { fabp->fab$b_fsz = 2 ; fabp->fab$b_rfm = FAB$C_VFC ; } ;
/*	File Sharing */
#define SHR(FLAG,SHF) if ((unitp->share_info & FLAG) != 0) fabp->fab$b_shr |= SHF
	SHR(V3_FLAGS_IOS_PUT,FAB$M_PUT) ; SHR(V3_FLAGS_IOS_GET,FAB$M_GET) ; SHR(V3_FLAGS_IOS_DELETE,FAB$M_DEL) ;
	SHR(V3_FLAGS_IOS_UPDATE,FAB$M_UPD) ; SHR(V3_FLAGS_IOS_NONE,FAB$M_NIL) ;
/*	See if we should add in any pragmatic info */
	if (unitp->pragmas & V3_FLAGS_IOPRAG_GET_SEQ)
	 { rabp->rab$b_mbc = 50 ; rabp->rab$b_mbf = 3 ; rabp->rab$l_rop |= RAB$M_RAH ; } ;
	if (unitp->pragmas & V3_FLAGS_IOPRAG_PUT_SEQ)
	 { fabp->fab$w_deq = 50 ; fabp->fab$l_fop |= FAB$M_TEF ;
	   rabp->rab$b_mbc = 50 ; rabp->rab$b_mbf = 3 ; rabp->rab$l_rop |= RAB$M_WBH ;
	 } ;
/*	See if we should link up any XAB buffers */
	fabp->fab$l_xab = unitp->xabp ;
/*	Ready to roll - try to open the file */
	if ((unitp->file_info & (V3_FLAGS_IOF_CREATE | V3_FLAGS_IOF_CREATE_IF)) != 0)
	 { rms_res = sys$create(fabp) ; } else { rms_res = sys$open(fabp) ; } ;
	if ((rms_res & 1) == 0) v3_error(V3E_FILEOPEN,"IO","OPEN","FILEOPEN",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
/*	File is open - do the connect */
	if ((unitp->file_info & V3_FLAGS_IOF_APPEND) && (unitp->file_info & V3_FLAGS_IOF_SEQUENTIAL)) rabp->rab$l_rop |= RAB$M_EOF ;
	rms_res = sys$connect(rabp) ;
	if ((rms_res & 1) == 0) v3_error(V3E_FILEOPEN,"IO","OPEN","FILEOPEN",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
	xnam[namp->nam$b_rsl] = 0 ; strcpy(unitp->file_name,xnam) ;
/*	All is well */
	if (ounits.count < V3_IO_UNIT_LIST)
	 { ounits.unit[ounits.count].unitp = unitp ; strcpy(ounits.unit[ounits.count++].file_name,unitp->file_name) ; } ;
}

vms_close(unitp)
  struct iou__unit *unitp ;		/* Pointer to the I/O unit */
{
   short int i ;
   int rms_res ; char errbuf[250] ;

/*	Must be an RMS file */
	if (unitp->rab_pointer != NULL)
	 { rms_res = sys$close(unitp->rab_pointer->rab$l_fab) ;
	   if ( ((rms_res & 1) != 1) && (rms_res != RMS$_FAB) )
	    v3_error(V3E_CLOSEFAIL,"IO","CLOSE","CLOSEFAIL",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
/*	   RMS file is closed - release some stuff */
	   free(unitp->rab_pointer->rab$l_fab->fab$l_nam) ; free(unitp->rab_pointer->rab$l_fab) ;
	   free(unitp->rab_pointer) ; unitp->rab_pointer = NULL ; unitp->file_ptr = -1 ;
	   return ;
	 } ;
}

vms_get(unitp,ptr,len,flags,getformall)
  struct iou__unit *unitp ;		/* Pointer to the I/O unit */
  char *ptr ;
  int len,flags,getformall ;
{ char errbuf[250] ;
   union val__format get_format ;	/* Format of get buffer */
   union val__format key_format ;	/* Format of key buffer */
   struct RAB *rabp ;
   int rms_res ;			/* Updated with result of rms call */
   int i ;
   char *cptr ; int clen ;		/* Pointer & length if compaction */

/*	Should we force an EOF ? */
	if (unitp->max_gets != 0)
	 { if (unitp->get_count >= unitp->max_gets) v3_error(V3E_EOF,"IO","GET","EOF","End of file reached (as per unit.max_gets)",unitp) ;
	 } ;
/*	Have a static I/O unit ? */
	if (unitp->is_static & IO_GET_STATIC)
	 { rabp = unitp->rab_pointer ; rabp->rab$l_rfa0 = unitp->vms_rfa4 ; rabp->rab$w_rfa4 = unitp->vms_rfa2 ;
/*	   If skipping records then skip them */
	   if (unitp->skip_gets != 0)
	    { if (rabp->rab$b_rac == RAB$C_SEQ) { for(i=0;i<unitp->skip_gets;i++) { rms_res = sys$get(rabp) ; } ; } ;
	    } ;
	   get_format.all = unitp->dflt_get_format.all ; ptr = unitp->last_pointer ;
	   rms_res = sys$get(rabp) ; unitp->get_count ++ ; goto rms_static_entry ;
	 } ;
	unitp->last_pointer = ptr ;	/* Save "last buffer pointer" in case of error dump */
	get_format.all = getformall ;
/*	See if we have record compaction */
	if (unitp->record_info & V3_FLAGS_IOR_COMPRESSED)
	 { cptr = ptr ; clen = len ; unitp->is_static = 0 ;
	   if (compact_buffer == 0) compact_buffer = malloc(V3_IO_COMPACT_BUF) ;
	   ptr = compact_buffer ; len = V3_IO_COMPACT_BUF ;
	 } ;
/*	What kind of file is this ? */
	if (unitp->file_ptr != NULL)
	 { if (fgets(ptr,len,unitp->file_ptr) == NULL)
	    v3_error(V3E_EOF,"IO","GET","EOF","End of file reached",unitp) ;
	   unitp->get_count++ ; unitp->get_length = strlen(ptr) ; goto got_record ;
	 } ;
/*	Best be an rms file */
	if ((rabp = unitp->rab_pointer) == NULL) v3_error(V3E_NOTOPEN,"IO","GET","NOTOPEN","Unit is not open",unitp) ;
	rabp->rab$l_rop = 0 ; rabp->rab$l_rfa0 = unitp->vms_rfa4 ; rabp->rab$w_rfa4 = unitp->vms_rfa2 ;
#define ROP(FLAG,OPT) if ((flags & FLAG) != 0) rabp->rab$l_rop |= OPT
	ROP(V3_FLAGS_IOPG_EOF,RAB$M_EOF) ; ROP(V3_FLAGS_IOPG_NOLOCK,RAB$M_NLK) ; ROP(V3_FLAGS_IOPG_LOCK_WAIT,RAB$M_WAT) ;
	ROP(V3_FLAGS_IOPG_PARTIAL,RAB$M_KGE) ; ROP(V3_FLAGS_IOPG_IGNORE_LOCK,RAB$M_RRL) ;
	if (unitp->pragmas & V3_FLAGS_IOPRAG_GET_SEQ) rabp->rab$l_rop |= RAB$M_RAH ;
/*	What kind of access ? */
	if (flags == 0) v3_error(V3E_NOGETMODE,"IO","GET","NOGETMODE","No get_mode (/bof/, /keyed/, /next/, ...) specified",unitp) ;
/*	Got an RMS File */
#define TAC(FLAG,OPT) if ((flags & FLAG) != 0) rabp->rab$b_rac = OPT
	TAC(V3_FLAGS_IOPG_KEYED,RAB$C_KEY) ; TAC(V3_FLAGS_IOPG_NEXT,RAB$C_SEQ) ; TAC(V3_FLAGS_IOPG_VMSRFA,RAB$C_RFA) ;
	TAC(V3_FLAGS_IOPG_DATAONLY,RAB$C_SEQ) ;
/*	Link user buffer to rab */
	rabp->rab$l_ubf = ptr ; rabp->rab$w_usz = len ;
/*	If SOS file then link to get_count otherwise just increment */
	if ((unitp->record_info & V3_FLAGS_IOR_SOS) == 0) { unitp->get_count++ ; }
	 else { rabp->rab$l_rhb = &unitp->get_count ; unitp->is_static = 0 ; } ;
/*	Figure out key info */
	if ((unitp->file_info & V3_FLAGS_IOF_INDEXED) != 0)
/*	   Doing keyed reference */
	 { if (unitp->get_key > 0) { rabp->rab$b_krf = unitp->get_key-1 ; unitp->get_key = 0 ; }
	    else { rabp->rab$b_krf = (unitp->dflt_get_key > 0 ? unitp->dflt_get_key-1 : 0) ; } ;
	   if (unitp->key_format.all != 0)
	    { key_format.all = unitp->key_format.all ; unitp->key_format.all = 0 ;
	      rabp->rab$l_kbf = unitp->key_ptr ;
	    } else
	    { key_format.all = unitp->dflt_key_format.all ; rabp->rab$l_kbf = unitp->dflt_key_ptr ;
	    } ;
	   rabp->rab$b_ksz = (key_format.fld.type == VFT_VARSTR ? strlen(rabp->rab$l_kbf) : key_format.fld.length) ;
	 } else
	 { rabp->rab$l_kbf = &unitp->get_key ; rabp->rab$b_ksz = 4 ;
	 } ;
	if ((flags & V3_FLAGS_IOPG_POSITION) != 0)
 	 { if ((flags & V3_FLAGS_IOPG_BOF) != 0)
	    { rms_res = sys$rewind(rabp) ; } else { rms_res = sys$find(rabp) ; } ;
	   unitp->is_static = 0 ;
	 } else
	 { if ((flags & V3_FLAGS_IOPG_BOF) != 0) rms_res = sys$rewind(rabp) ;
/*	   If unit is static then remember to save time on future calls */
	   if (unitp->is_static & IO_SET_STATIC) unitp->is_static |= IO_GET_STATIC ;
/*	   If skipping records then skip them */
	   if (unitp->skip_gets != 0)
	    { if (rabp->rab$b_rac == RAB$C_SEQ)
	       { for(i=0;i<unitp->skip_gets;i++) { rms_res = sys$get(rabp) ; } ; } ;
	    } ;
	   rms_res = sys$get(rabp) ;
	 } ;
rms_static_entry:
	if ((rms_res & 1) != 0) { unitp->get_length = rabp->rab$w_rsz ; goto got_record ; } ;
/*	Got an error - see if we can figure it out */
	if (rms_res == RMS$_EOF) v3_error(V3E_EOF,"IO","GET","EOF","End of file reached",unitp) ;
	if (rms_res == RMS$_RLK) v3_error(V3E_LOCKED,"IO","GET","LOCKED","Record is locked",unitp) ;
	if (rms_res == RMS$_RTB) v3_error(V3E_RECTOOBIG,"IO","GET","RECTOOBIG","Record too big for buffer",unitp) ;
/*	Just generate read error */
	v3_error(V3E_GET,"IO","GET","GET",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
/*	Here to do final processing on record */
got_record:
/*	Should we expand a compressed record ? */
	if (unitp->record_info & V3_FLAGS_IOR_COMPRESSED)
	 { ptr = cptr ; unitp->get_length = data_expand(cptr,compact_buffer,unitp->get_length) ; } ;
/*	If variable length record then append null */
	if (get_format.fld.type == VFT_VARSTR) *(ptr+unitp->get_length) = 0 ;
/*	Save the VMS/RMS RFA */
	unitp->vms_rfa4 = rabp->rab$l_rfa0 ; unitp->vms_rfa2 = rabp->rab$w_rfa4 ;
	return ;
}

/*	vms_put - Writes a record */

vms_put(unitp,ptr,len,flags)
  struct iou__unit *unitp ;		/* Pointer to the I/O unit */
  char *ptr ;
  int len,flags ;
{ union val__format put_format ;	/* Put buffer format desc */
   struct RAB *rabp ;
   char errbuf[250] ;
   int rms_res ;
   int clen ; char *cptr ;		/* For compaction */

/*	Should we be shadowing this unit ? */
	if (unitp->shadow_flag != 0) v3_error(V3E_NOTSHADOW,"IO","PUT","NOTSHADOW","Attempted io_put without first shadowing",unitp) ;
	unitp->shadow_flag = unitp->shadow_index ;	/* Copy over to ensure that V3 code clears before next io_put ! */
/*	Do we have a static IO unit ? */
	if (unitp->is_static & IO_PUT_STATIC)
	 { rabp = unitp->rab_pointer ; unitp->put_count++ ;
	   if (unitp->is_static & IO_UPDATE_STATIC) { rms_res = sys$update(rabp) ; } else { rms_res = sys$put(rabp) ; } ;
	   if ((rms_res & 1) == 0) v3_error(V3E_PUTERR,"IO","PUT","PUTERR",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
	   return ;
	 } ;
/*	Should we chop off trailing spaces ? */
	if (flags & V3_FLAGS_IOPG_TRUNCATE) { for (;len>0 && *(ptr+len-1)==' ';len--) ; } ;
/*	See if we have record compaction */
	if (unitp->record_info & V3_FLAGS_IOR_COMPRESSED)
	 { if (compact_buffer == 0) compact_buffer = malloc(V3_IO_COMPACT_BUF) ;
	   len = data_compress(compact_buffer,ptr,len,unitp->header_bytes,V3_IO_COMPACT_BUF) ; ptr = compact_buffer ;
	   unitp->is_static = 0 ;
	 } ;
	if ((rabp = unitp->rab_pointer) == NULL)
	 v3_error(V3E_NOTOPEN,"IO","PUT","NOTOPEN","Unit is not open",unitp) ;
/*	Best be writing to RMS file */
	rabp->rab$l_rbf = ptr ; rabp->rab$w_rsz = len ;
/*	Check for deletes */
	if (flags & (V3_FLAGS_IOPG_OBSOLETE | V3_FLAGS_IOPG_DELETE))
	 { unitp->put_count++ ;
	   if (((rms_res = sys$delete(rabp)) & 1) == 0) v3_error(V3E_DELFAIL,"IO","PUT","DELFAIL",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
	   return ;
	 } ;
/*	Check for truncation */
	if (flags & V3_FLAGS_IOPG_NEW_EOF)
	 { if (((rms_res = sys$truncate(rabp)) & 1) == 0) v3_error(V3E_TRNFAIL,"IO","PUT","TRNFAIL",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
	   return ;
	 } ;
/*	Figure out some flags */
	rabp->rab$l_rop = 0 ;
#define ROP(FLAG,OPT) if ((flags & FLAG) != 0) rabp->rab$l_rop |= OPT
	ROP(V3_FLAGS_IOPG_EOF,RAB$M_EOF) ; ROP(V3_FLAGS_IOPG_NOLOCK,RAB$M_NLK) ; ROP(V3_FLAGS_IOPG_LOCK_WAIT,RAB$M_WAT) ;
	if (unitp->pragmas & V3_FLAGS_IOPRAG_PUT_SEQ) rabp->rab$l_rop |= RAB$M_WBH ;
/*	Are we updating or is it a new record ? */
	if ((flags & V3_FLAGS_IOPG_NEW_RECORD) == 0) rabp->rab$l_rop |= RAB$M_UIF ;
/*	What kind of access ? */
#define TAC(FLAG,OPT) if ((flags & FLAG) != 0) rabp->rab$b_rac = OPT
	TAC(V3_FLAGS_IOPG_KEYED,RAB$C_KEY) ; TAC(V3_FLAGS_IOPG_NEXT,RAB$C_SEQ) ;
/*	If an SOS file then use put_count as line number, otherwise increment it */
	if ((unitp->record_info & V3_FLAGS_IOR_SOS) == 0) { unitp->put_count++ ; }
	 else { rabp->rab$l_rhb = &unitp->put_count ; } ;
/*	Ready to write the record */
	if (unitp->is_static & IO_SET_STATIC)
	 { unitp->is_static |= IO_PUT_STATIC ; if (flags & V3_FLAGS_IOPG_UPDATE_EXISTING) unitp->is_static |= IO_UPDATE_STATIC ;
	 } ;
/*	Are we doing a PUT or UPDATE ? */
	if (flags & V3_FLAGS_IOPG_UPDATE_EXISTING) { rms_res = sys$update(rabp) ; } else { rms_res = sys$put(rabp) ; } ;
	if (rms_res == RMS$_REX) v3_error(V3E_EXISTS,"IO","PUT","EXISTS","Record already exists with specified key",unitp) ;
	if ((rms_res & 1) == 0) v3_error(V3E_PUTERR,"IO","PUT","PUTERR",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
/*	Save the VMS/RMS RFA */
	unitp->vms_rfa4 = rabp->rab$l_rfa0 ; unitp->vms_rfa2 = rabp->rab$w_rfa4 ;
/*	Looks good */
	return ;
 }

/*	U S E R   T T Y   R O U T I N E S			*/

/*	vax_tty_misc - Handles the tty_misc() module		*/

vax_tty_misc()
 { struct {
     short int status ;		/* IOSB status */
     short int data_len ;	/* Number data bytes input */
     short int terminator ;	/* Terminating byte */
     short int term_len ;	/* Number of terminating bytes */
    } iosb ;
   struct {
     short int bytes ;		/* Number of bytes typeahead */
     char first_byte ;
     char reserved[5] ;
   } char_buf ;			/* TTY characteristics buffer */
   struct {
     short int len ;
     short int type ;
     char *pointer ;
    } chan_desc ;
   int func_code,res ;
   short int ac ; int *base_ptr ;
   union val__format format ;
   char temp_name[250] ;		/* Temp device name */
   globalvalue int IO$_SENSEMODE,IO$M_TYPEAHDCNT ;

/*	If first time then open up the channel */
	if (tty_channel == 0)
	 { if (tty_device_name[1] == 0) strcpy(tty_device_name,"SYS$INPUT") ;
	   chan_desc.pointer = &tty_device_name ; chan_desc.len = strlen(tty_device_name) ; chan_desc.type = 0 ;
	   sys$assign(&chan_desc,&tty_channel,0,0) ;
	 } ;
/*	See how many arguments */
	for (ac=0;;ac++)
	 { POPF(format.all) ; POPVP(base_ptr,(int *)) ; if (format.all == V3_FORMAT_EOA) break ; } ;
/*	ac = number of arguments, base_ptr = current stack pointer */
	base_ptr = psi->stack_ptr ;
/*	And the function code */
	psi->stack_ptr = base_ptr - 4 ; func_code = xctu_popint() ; psi->stack_ptr = base_ptr ;
/*	Now branch on function code */
	if (func_code == V3_FLAGS_TTY_TYPEAHEAD_COUNT)
	 { sys$qiow(0,tty_channel,IO$_SENSEMODE | IO$M_TYPEAHDCNT,&iosb,0,0,&char_buf,8,0,0,0,0) ;
	   PUSHINT(char_buf.bytes) ;
	 } else if (func_code == V3_FLAGS_TTY_SET_DEVICE)
	 { if (tty_channel != 0) { sys$dassgn(tty_channel) ; tty_channel = 0 ; } ;
/*	   Get the new device name */
	   psi->stack_ptr = base_ptr - 6 ; stru_popcstr(temp_name) ; psi->stack_ptr = base_ptr ;
	   chan_desc.pointer = &temp_name ; chan_desc.len = strlen(temp_name) ; chan_desc.type = 0 ;
	   if (chan_desc.len == 0) { strcpy(temp_name,"SYS$INPUT") ; chan_desc.len = 9 ; } ;
	   res = sys$assign(&chan_desc,&tty_channel,0,0) ;
	   if ((res & 1) == 0) v3_error(V3E_INVDEVNAM,"TTY","MISC","INVDEVNAM","Invalid device name",res) ;
	   strcpy(tty_device_name,temp_name) ;	/* Looks good, copy in name */
	 } else if (func_code == V3_FLAGS_TTY_FLUSH)
	 {	/* Not yet implemented on VAX/VMS */
	 } else	v3_error(V3E_INVFUNC,"TTY","MISC","INVFUNC","Invalid TTY_MISC function",func_code) ;
}

/*	T T Y   I N P U T   ( C R T )   R O U T I N E		*/

/*	vax_scr_get - Reads input from crt screen		*/

vax_scr_get(buffer,length,flags,time,mask)
  char *buffer ;		/* Pointer to user buffer for input */
  int length ;			/* Number of bytes to input */
  int flags ;			/* User flags */
  int time ;			/* Number of seconds to wait */
  char *mask ;			/* NULL or data input mask */
{ struct {
     short int status ;		/* IOSB status */
     short int data_len ;	/* Number data bytes input */
     short int terminator ;	/* Terminating byte */
     short int term_len ;	/* Number of terminating bytes */
    } iosb ;

   globalvalue int IO$_READVBLK,IO$_WRITEVBLK,IO$_SENSEMODE,IO$M_TYPEAHDCNT,
	IO$M_ESCAPE,IO$M_NOFILTR,IO$M_TIMED,IO$M_TRMNOECHO,IO$M_NOECHO,
	SS$_HANGUP,SS$_TIMEOUT ;

   char *input_ptr ;		/* Pointer into buffer */
   char *esc_start,*ep ;	/* Start of escape sequence (after escape) & temp pointer */
   short int input_len ;	/* Number bytes to input (remaining) */
   int looper,func,vms_res ; short int i,j ;
   struct {
     short int len ;
     short int type ;
     char *pointer ;
    } chan_desc ;
   struct {
     int mask_size ;
     int *ptr ;
     int mask[4] ;
    } read_term ;
   struct {
     short int bytes ;		/* Number of bytes typeahead */
     char first_byte ;
     char reserved[5] ;
   } char_buf ;			/* TTY characteristics buffer */

/*	If first time then open up the channel */
	if (tty_channel == 0)
	 { if (tty_device_name[1] == 0) strcpy(tty_device_name,"SYS$INPUT") ;
	   chan_desc.pointer = &tty_device_name ; chan_desc.len = strlen(tty_device_name) ; chan_desc.type = 0 ;
	   sys$assign(&chan_desc,&tty_channel,0,0) ;
	 } ;

/*	Set up some pointers & lengths */
	input_ptr = buffer ; input_len = length ;
	read_term.mask_size = 16 ; read_term.ptr = &read_term.mask ;
	read_term.mask[0] = 0x3FFFDFF ;	/* Terminate on all control except tab */
	read_term.mask[1] = (read_term.mask[2] = 0) ;
	read_term.mask[3] = 0x80000000 ; /* Terminate on delete */
/*	If interrupt on question-mark then set bit */
	if ((flags & V3_FLAGS_TTY_QUESTION_MARK) != 0) read_term.mask[1] = 0x80000000 ;

/*	If got the /passall/ flag then do special read */
	if (flags & V3_FLAGS_TTY_PASSALL)
	 { read_term.mask[0] = -1 ; read_term.mask[1] = -1 ; read_term.mask[2] = -1 ; read_term.mask[3] = -1 ;
	   func = IO$_READVBLK | IO$M_TRMNOECHO | IO$M_NOECHO | IO$M_NOFILTR | IO$M_ESCAPE ;
	   if (time > 0) func |= IO$M_TIMED ;
	   vms_res = sys$qiow(0,tty_channel,func,&iosb,0,0,input_ptr,99,time,&read_term,0,0) ;
	   if ((vms_res & 1) == 0) exit(EXIT_SUCCESS) ;
	   if (iosb.status == SS$_HANGUP) exit(EXIT_SUCCESS) ;
	   input_ptr += iosb.data_len + iosb.term_len ;
	   if (iosb.status == SS$_TIMEOUT) goto got_timeout ;
	   *input_ptr = 0 ;
	   return(input_ptr-buffer) ;
	 } ;

/*	If we got the clear_after_first flag then do special */
	if ((flags & V3_FLAGS_TTY_CLEAR_AFTER_FIRST) != 0)
	 { func = IO$_READVBLK | IO$M_ESCAPE | IO$M_NOFILTR  | IO$M_TRMNOECHO ;
	   if (time > 0) func |= IO$M_TIMED ;
	   for (;;)
	    { vms_res = sys$qiow(0,tty_channel,func,&iosb,0,0,input_ptr,1,time,&read_term,0,0) ;
	      if ((vms_res & 1) == 0) exit(EXIT_SUCCESS) ;
	      if (iosb.status == SS$_HANGUP) exit(EXIT_SUCCESS) ;
/*	      Did we get anything ? */
	      if (iosb.status == SS$_TIMEOUT) goto got_timeout ;
	      if (iosb.data_len > 0)
	       { input_ptr++ ; input_len-- ;
/*		 Clear out the rest of the field */
		 for (i=1;i<=input_len;i++)
		  { sys$qiow(0,tty_channel,IO$_WRITEVBLK,0,0,0," ",1,0,0,0,0) ; } ;
		 for (i=1;i<=input_len;i++)
		  { sys$qiow(0,tty_channel,IO$_WRITEVBLK,0,0,0,"\010",1,0,0,0,0) ; } ;
		 break ;
	       } ;
/*	      Got a terminator */
	      if (*input_ptr == 27) goto got_escape ;
	      if (*input_ptr == 21 || *input_ptr == 127) continue ;
/*	      Got a valid terminator - end input */
	      *input_ptr = iosb.terminator ; *(input_ptr+1) = 0 ;
	      return(input_ptr - buffer) ;
	   } ;
	 } ;
/*	Set up main input loop */
	for (looper=0;;looper++)
	 { if (looper > 2500) v3_error(V3E_LOOPMAX,"TTY","GET","LOOPMAX","Terminal input loop error",0) ;
	   func = IO$_READVBLK | IO$M_ESCAPE | IO$M_NOFILTR | IO$M_TRMNOECHO ;
	   if (time > 0) func |= IO$M_TIMED ;
	   if ((flags & V3_FLAGS_TTY_NOECHO) != 0) func |= IO$M_NOECHO ;
	   vms_res = sys$qiow(0,tty_channel,func,&iosb,0,0,input_ptr,input_len,time,&read_term,0,0) ;
	   if ((vms_res & 1) == 0) exit(EXIT_SUCCESS) ;
	   if (iosb.status == SS$_HANGUP) exit(EXIT_SUCCESS) ;
/*	   What did we get ? */
	   input_ptr += iosb.data_len ; input_len -= iosb.data_len ;
	   if (iosb.status == SS$_TIMEOUT) goto got_timeout ;
/*	   Assume ok for now */
	   if (iosb.term_len == 0 && (flags & V3_FLAGS_TTY_TRUNCATE))
	    { func |= IO$M_NOECHO ;
/*	      Read until user inputs a terminator */
	      for (;;)
	       { vms_res = sys$qiow(0,tty_channel,func,&iosb,0,0,input_ptr,1,time,&read_term,0,0) ;
	         if ((vms_res & 1) == 0) exit(EXIT_SUCCESS) ;
		 if (iosb.status == SS$_HANGUP) exit(EXIT_SUCCESS) ;
		 if (iosb.status == SS$_TIMEOUT) goto got_timeout ;
/*		 Did we get a terminator ? */
		 if (iosb.term_len > 0) break ;
/*		 No - ring bell & continue in this loop */
		 sys$qiow(0,tty_channel,IO$_WRITEVBLK,0,0,0,"\007",1,0,0,0,0) ;
	       } ;
	    } ;
/*	   Got something - check out the terminator */
	   if (iosb.term_len > 0 && iosb.terminator == 0 && *input_ptr == 27) goto got_escape ;
	   if (iosb.term_len == 0) iosb.terminator = 13 ;	/* If ran out of buffer then fudge in a return */
	   switch (iosb.terminator)
	    { case 21:	/* Here for ^U (erase entire field) */
		i = input_ptr - buffer ; /* i = number bytes input */
		input_ptr -= i ; input_len += i ;
		if (flags & V3_FLAGS_TTY_NOECHO) break ;
		for (j=1;j<=i;j++)
		 { sys$qiow(0,tty_channel,IO$_WRITEVBLK,0,0,0,"\010 \010",3,0,0,0,0) ; } ;
		break ;
	      case 127:	/* Here for delete */
/*		Make sure we don't delete past left end */
		if (input_ptr <= buffer) break ;
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 sys$qiow(0,tty_channel,IO$_WRITEVBLK,0,0,0,"\010 \010",3,0,0,0,0) ;
		input_ptr -= 1 ; input_len += 1 ; break ;
	      case 27:	/* Here for an escape */
		goto got_escape ;
	      case 0:	/* Break - handle as end-of-input */
	      default: 	/* Normal terminator - all done */
/*		Insert the terminator and null */
		*input_ptr = iosb.terminator ; *(input_ptr+1) = 0 ;
		return(input_ptr - buffer) ;
	    } ;
	 } ;
/*	Here to handle time-outs */
got_timeout:
	return(-1) ;
/*	Here to handle escape sequences */
got_escape:
/*	Read remainder of escape sequence */
	input_len = input_ptr - buffer ; esc_start = input_ptr + 1 ; input_ptr += iosb.term_len ;
got_escape_continue:
	for (looper=0;;looper++)
	 { if (looper > 50) v3_error(V3E_BADESC,"TTY","GET","BADESC","Invalid input escape sequence",0) ;
	   ep = esc_start ; *input_ptr = 0 ;
	   if (*ep == '[')
	    { for(ep++;*ep>=0x30 && *ep<=0x3F;ep++) ; } ;
	   if (*ep == ';' || *ep == '?' || *ep == 'O') ep++ ;
	   for(;*ep>=0x20 && *ep<=0x2F;ep++) ;
/*	   If end of escape sequence then next should be between 30 & 7E */
	   if (*ep >= 0x30 && *ep <= 0x7E) break ;
/*	   Not end of sequence, read on */
/*	   See how many more bytes are in escape sequence */
	   sys$qiow(0,tty_channel,IO$_SENSEMODE | IO$M_TYPEAHDCNT,&iosb,0,0,&char_buf,8,0,0,0,0) ;
	   if ((vms_res & 1) == 0) exit(EXIT_SUCCESS) ;
	   if (iosb.status == SS$_HANGUP) exit(EXIT_SUCCESS) ;
	   if (char_buf.bytes <= 0) char_buf.bytes = 1 ;	/* Make sure at least one byte read */
	   sys$qiow(0,tty_channel,IO$_READVBLK | IO$M_TRMNOECHO | IO$M_NOECHO,&iosb,0,0,input_ptr,char_buf.bytes,0,0,0,0) ;
	   if ((vms_res & 1) == 0) exit(EXIT_SUCCESS) ;
	   if (iosb.status == SS$_HANGUP) exit(EXIT_SUCCESS) ;
/*	   See if end of escape sequence */
	   input_ptr += char_buf.bytes ;
	 } ;
	return(input_len) ;
}

/*	S Y S T E M   Q U E U E   M O D U L E S			*/

struct tmp__itmlst			/* Format of VMS item-list */
 { short int buflen,code ;
   int *bufadr,*lenadr ;
 } ;

vax_queue(dflt_queue_name)
   char *dflt_queue_name ;
 { struct v3__queue_info *qi ;
   struct str__ref sr ;
   struct tmp__itmlst itms[20] ;
   struct {
     short int status ;		/* IOSB status */
     short int data_len ;	/* Number data bytes input */
     short int terminator ;	/* Terminating byte */
     short int term_len ;	/* Number of terminating bytes */
    } iosb ;
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc ;
   int cnt,qnum,i,res,vax_datetime[2] ; char server[50],buf[250],*p ;

/*	Link up to the queue information block */
	stru_popstr(&sr) ; qi = sr.ptr ;
/*	See if going through xvrc ? */
	p = strrchr(qi->parameters[0],']') ;
	if (qi->parameters[0][0] == '[' && p != NULL)			/* Handle queing via vrc ? */
	 { *p = 0 ; strcpy(server,&qi->parameters[0][1]) ;
	   sprintf(buf,"xvrc -n %s -o \"%s\" -f %s",server,p+1,qi->filename) ;
	   strdsc.pointer = &buf ; strdsc.length = strlen(buf) ;
	   strdsc.desc = 0 ; res = LIB$SPAWN(&strdsc,0,0,0,0,0,&i) ;
	   if ((res & 1) == 0) v3_error(V3E_VMSERR,"SYS","SPAWN","VMSERR",mscu_vms_errmsg(res,buf)) ;
	   return ;
	 } ;
/*	Set up itemlist */
	for(i=0;i<20;i++) { itms[i].code = 0 ; itms[i].lenadr = 0 ; itms[i].buflen = 0 ; itms[i].bufadr = 0 ; } ;
/*	Queue name & file name are mandatory */
	cnt = 0 ; itms[cnt].code = 134 ; /* SJC$_QUEUE */
	if (strlen(qi->queuename) == 0) { itms[cnt].buflen = strlen(dflt_queue_name) ; itms[cnt].bufadr = dflt_queue_name ; }
	 else { itms[cnt].buflen = strlen(qi->queuename) ; itms[cnt].bufadr = &(qi->queuename) ; } ;
	cnt++ ; itms[cnt].code = 42 ; /* SJC$_FILE_SPECIFICATION */
	itms[cnt].buflen = strlen(qi->filename) ; itms[cnt].bufadr = &(qi->filename) ;
	if (strlen(qi->job_name) > 0)
	 { cnt++ ; itms[cnt].code = 79 ; /* SJC$_JOB_NAME */
	   itms[cnt].bufadr = &(qi->job_name) ; itms[cnt].buflen = strlen(qi->job_name) ;
	 } ;
	if (strlen(qi->log_name) > 0)
	 { cnt++ ; itms[cnt].code = 98 ; /* SJC$_LOG_SPECIFICATION */
	   itms[cnt].bufadr = &qi->log_name ; itms[cnt].buflen = strlen(qi->log_name) ;
	 } ;
/*	If we have date-time then convert to VAX format */
	if (qi->submit_date_time != 0)
	 { cnt++ ; itms[cnt].code = 3 ; /* SJC$_AFTER_TIME */
	   v3_cnv_dt_sysdt(qi->submit_date_time,&vax_datetime) ;	/* Convert it */
	   itms[cnt].buflen = 8 ; itms[cnt].bufadr = &vax_datetime ;
	 } ;
	if (qi->copies != 0)
	 { cnt++ ; itms[cnt].code = 35 ; /* SJC$_FILE_COPIES */ itms[cnt].buflen = 4 ; itms[cnt].bufadr = &(qi->copies) ; } ;
	if (qi->start_page != 0)
	 { cnt++ ; itms[cnt].code = 46 ; /* SJC$_FIRST_PAGE */ itms[cnt].buflen = 4 ; itms[cnt].bufadr = &(qi->start_page) ; } ;
	if (qi->end_page != 0)
	 { cnt++ ; itms[cnt].code = 91 ; /* SJC$_LAST_PAGE */ itms[cnt].buflen = 4 ; itms[cnt].bufadr = &(qi->end_page) ; } ;
	if (qi->delete > 0)
	 { cnt++ ; itms[cnt].code =  SJC$_DELETE_FILE ; } ;
	if (strlen(qi->operator) > 0)
	 { cnt++ ; itms[cnt].code = 110 ; /* SJC$_OPERATOR_REQUEST */ itms[cnt].buflen = strlen(qi->operator) ;
	   itms[cnt].bufadr = &(qi->operator) ;
	 } ;
	if (qi->burst > 0) { cnt++ ; itms[cnt].code = 33 ; /* SJC$_FILE_BURST_ONE */ }
	 else { if (qi->burst < 0) { cnt++ ; itms[cnt].code = 34 ; /* SJC$_NO_FILE_BURST */ } ; } ;
	if (qi->banner > 0) { cnt++ ; itms[cnt].code = 36 ; /* SJC$_FILE_FLAG */ }
	 else { if (qi->banner < 0) { cnt++ ; itms[cnt].code = 38 ; /* SJC$_NO_FILE_FLAG */ } ; } ;
	if (qi->notify > 0) { cnt++ ; itms[cnt].code = 108 ; /* SJC$_NOTIFY */ } ;
	if (qi->passall > 0) { cnt++ ; itms[cnt].code = 128 ; /* SJC$_PASSALL */ } ;
	if (strlen(qi->forms) > 0)
	 { cnt++ ; itms[cnt].code = 54 ; /* SJC_$FORM_NAME */ itms[cnt].buflen = strlen(qi->forms) ;
	   itms[cnt].bufadr = &(qi->forms) ;
	 } ;
	cnt++ ; itms[cnt].code = 101 ; /* SJC$_NO_LOG_SPOOL */
	if (strlen(qi->setup) > 0)
	 { cnt++ ; itms[cnt].code = 40 ; /* SJC$_FILE_SETUP_MODULES */
	   itms[cnt].buflen = strlen(qi->setup) ; itms[cnt].bufadr = &(qi->setup) ;
	 } ;
	if (strlen(qi->notes) > 0)
	 { cnt++ ; itms[cnt].code = 106 ; /* SJC$_NOTE */
	   itms[cnt].buflen = strlen(qi->notes) ; itms[cnt].bufadr = &(qi->notes) ;
	 } ;
	if (qi->paging < 0) { cnt++ ; itms[cnt].code = 118 ; /* SJC$_NO_PAGINATE */ } ;
	for(i=0;i<8;i++)
	 { if (qi->parameters[i][0] != 0)
	    { cnt++ ; itms[cnt].code = 119+i ; /* SJC$_PARAMETER_n */ ;
	      itms[cnt].buflen = strlen(&(qi->parameters[i][0])) ; itms[cnt].bufadr = &(qi->parameters[i][0]) ;
	    } ;
	 } ;
	if (qi->hold > 0 ) { cnt++ ; itms[cnt].code = 71 ; /* SJC$_HOLD */ } ;
	cnt++ ; itms[cnt].code = 31 ; /* SJC$_ENTRY_NUMBER_OUTPUT */ itms[cnt].buflen = 4 ; itms[cnt].bufadr = &qnum ;

	res = sys$sndjbcw(0,19,0,&itms,&iosb,0,0) ; /* SJC$_ENTER_FILE */
	if ((res & 1) != 1) qnum = -res ;
	PUSHINT(qnum) ;
}

/*	S E G M E N T   R O U T I N E S			*/

#define SEG_LIST_MAX 25

struct seg__list	/* List of current open shared segments */
 { short count ;		/* Number of shared segments currently open */
   struct
    { char *pointer ;		/* Pointer to the segment in this process memory */
      int end_address ;		/* Pointer to last page in this segment */
      int lock_id ;		/* If nonzero then lock-id */
      char name[50] ;		/* Name of the segment */
    } segment[SEG_LIST_MAX] ;
 } ;
static struct seg__list *seglist = 0 ;

/*	iou_seg_create( buffer , name ) - Creates a segment file from buffer */

iou_seg_create()
{ char name[250] ;		/* Holds segment file name */
   struct str__ref sstr ;
   FILE *file_ptr ;

	stru_popcstr(name) ;		/* Get the segment name */
	stru_popstr(&sstr) ;		/* And buffer */
	strcat(name,".SEG") ;
	if ((file_ptr = fopen(name,"w")) == NULLV)
	 v3_error(V3E_CRESEGFILE,"SEG","CREATE","CRESEGFILE","Could not create segment file",name) ;
	fwrite(sstr.ptr,sstr.format.fld.length,1,file_ptr) ;
	fclose(file_ptr) ;
}

/*	iou_seg_lock( buffer , access ) - Attempts to lock a buffer for read/write access */

iou_seg_lock()
 { char errbuf[150],msgbuf[150] ;
   struct str__ref sstr ;
   struct vms__dsc ndsc ;
   struct vms_itmlst
    { short l1,ic1 ; int buf1,ret1 ;
      short l2,ic2 ; int buf2,ret2 ;
      int zilch ;
    } itmlst ;
   char resnam[50] ; int pid ;
#define LKI_BLOCKING 0x207
#define LKI__L_PID 4
   struct vms__lksb		/* Lock status block */
    { short vms_cond,reserved ;
      int lock_id ;
    } lksb ;
  int flags,i ;
#define LCK_K_EXMODE 5
#define LCK_K_PRMODE 3
#define LCK_M_NOQUEUE 4
/*  globalvalue int LCK$K_EXMODE,LCK$K_PRMODE ; */

	flags = xctu_popint() ;	/* Pop off access flags */
	stru_popstr(&sstr) ;		/* And buffer */
/*	Look for the buffer in the seglist table */
	for(i=0;i<seglist->count;i++) { if (sstr.ptr == seglist->segment[i].pointer) break ; } ;
	if (i >= seglist->count)
	 v3_error(V3E_NOSEGMENT,"SEG","UNLOCK","NOSEGMENT","Segment not entered into V3 table via SEG_SHARE",0) ;
/*	Try and set the lock */
	ndsc.length = strlen(seglist->segment[i].name) ; ndsc.desc = 0 ; ndsc.pointer = &(seglist->segment[i].name) ;
	flags = sys$enqw(0,((flags & (V3_FLAGS_IOF_UPDATE+V3_FLAGS_IOF_PUT)) != 0 ? LCK_K_EXMODE : LCK_K_PRMODE),&lksb,
			 ((flags & V3_FLAGS_IOPG_LOCK_WAIT) == 0 ? LCK_M_NOQUEUE : 0),&ndsc,0,0,0,0,0,0) ;
	if ((flags & 1) == 0)
	 { mscu_vms_errmsg(flags,errbuf) ;
/*	   Find the process blocking the lock */
	   for(;;)
	    { lksb.lock_id = -1 ; itmlst.l1 = 4 ; itmlst.ic1 = 0x100 ; itmlst.buf1 = &pid ; itmlst.ret1 = 0 ;
	      itmlst.l2 = 31 ; itmlst.ic2 = 0x201 ; itmlst.buf2 = &resnam ; itmlst.ret2 = 0 ; itmlst.zilch = 0 ;
	      flags = sys$getlkiw(0,&lksb.lock_id,&itmlst,0,0,0,0) ;
	      if ((flags & 1) == 0) { pid = -1 ; break ; } ;
	      if (strcmp(ndsc.pointer,&resnam) == 0) break ;	/* Kick out if resource name matches */
	    } ;
	   sprintf(msgbuf,"%s (PID=%x)",errbuf,pid) ;
	   v3_error(V3E_NOLOCK,"SEG","LOCK","NOLOCK",msgbuf,0) ; return ;
	 } ;
/*	Got the lock - remember */
	seglist->segment[i].lock_id = lksb.lock_id ;
 } ;

/*	iou_seg_share( name , access ) - Sets up shared segment */

iou_seg_share()
 { char name[150],errbuf[150] ;
   char xnam[V3_IO_FILENAME_MAX] ;	/* Holds expanded file name */
   int flags,bytes,i,write ;
   int sec_address[2],ret_address[2] ;
   struct FAB seg_fab ;		/* To access V3X file via mapped section */
   struct NAM seg_nam ;		/* XAB for file name */
   struct vms__dsc ndsc ;
   globalvalue int SEC$M_CRF,SEC$M_GBL,SEC$M_WRT ;
   globalvalue int PRT$C_UR ;	/* Protection for pages: user read */

/*	If first call then allocate seglist */
	if (seglist == 0) seglist = calloc(1,sizeof *seglist) ;

	flags = xctu_popint() ;	/* Pop off access flags */
	write = (flags & (V3_FLAGS_IOF_UPDATE+V3_FLAGS_IOF_PUT)) != 0 ;
	stru_popcstr(name) ;		/* Get the segment name */

/*	Open the file via rms for user-only access */
	seg_fab = cc$rms_fab ; seg_nam = cc$rms_nam ;
	seg_fab.fab$l_fna = &name ; seg_fab.fab$b_fns = strlen(name) ; seg_fab.fab$l_dna = ".seg" ; seg_fab.fab$b_dns = 4 ;
	seg_fab.fab$b_fac = FAB$M_GET ; if (write) seg_fab.fab$b_fac |= (FAB$M_PUT + FAB$M_UPD) ;
	seg_fab.fab$l_fop = FAB$M_UFO ; seg_fab.fab$l_nam = &seg_nam ;
	seg_fab.fab$b_shr = FAB$M_UPI + FAB$M_GET + FAB$M_PUT + FAB$M_UPD ;
	seg_nam.nam$l_rsa = &xnam ; seg_nam.nam$b_rss = V3_IO_FILENAME_MAX ;
	flags = sys$open(&seg_fab) ;
/*	Did we open it ok ? */
	if (!(flags & 1))
	 { v3_error(V3E_FILEOPEN,"SEG","SHARE","FILEOPEN",mscu_vms_errmsg(flags,errbuf),0) ; } ;
/*	Copy file name from full name specs */
	strncpy(seglist->segment[seglist->count].name,seg_nam.nam$l_name,seg_nam.nam$b_name) ;
/*	Allocate memory for this file */
	bytes = (seg_fab.fab$l_alq*V3_VMS_PAGELET_SIZE + 2*V3_VMS_PAGE_SIZE) ;
	sec_address[1] = (sec_address[0] = ((malloc(bytes) + V3_VMS_PAGE_SIZE) & ~(V3_VMS_PAGE_SIZE-1))) ;
	sec_address[1] += V3_VMS_PAGE_SIZE-1 ;

	ndsc.length = strlen(seglist->segment[seglist->count].name) ; ndsc.desc = 0 ;
	ndsc.pointer = &(seglist->segment[seglist->count].name) ;
/*	Now try to map everything into a global section */
	flags = sys$crmpsc(&sec_address,&ret_address,0,(write ? SEC$M_GBL+SEC$M_WRT : SEC$M_GBL),
			   &ndsc,0,0,seg_fab.fab$l_stv,seg_fab.fab$l_alq,0,0,0) ;
	if (!(flags & 1))
	 { v3_error(V3E_CRMPSCFAIL,"SEG","SHARE","CRMPSCFAIL",mscu_vms_errmsg(flags,errbuf),0) ; } ;
	seglist->segment[seglist->count].pointer = ret_address[0] ; seglist->segment[seglist->count].end_address = ret_address[1] ;
	seglist->count ++ ;
	PUSHMP(ret_address[0]) ; PUSHF(V3_FORMAT_POINTER) ; return ;
 } ;

/*	iou_seg_unlock( buffer ) - Unlocks buffer previously locked via seg_lock */

iou_seg_unlock()
 { char errbuf[150] ;
   struct str__ref sstr ;
   struct vms__lksb		/* Lock status block */
    { short vms_cond,reserved ;
      int lock_id ;
    } lksb ;
  int i,flags ;

	stru_popstr(&sstr) ;		/* Get the buffer location */
/*	Look for the buffer in the seglist table */
	for(i=0;i<seglist->count;i++) { if (sstr.ptr == seglist->segment[i].pointer) break ; } ;
	if (i >= seglist->count || (seglist->segment[i].lock_id == 0))
	 v3_error(V3E_NOSEGMENT,"SEG","UNLOCK","NOSEGMENT","Segment not entered into V3 table via SEG_SHARE",0) ;
/*	Release the lock */
	flags = sys$deq(seglist->segment[i].lock_id,0,0,0) ;
	if ((flags & 1) == 0) v3_error(V3E_NOLOCK,"SEG","UNLOCK","NOLOCK",mscu_vms_errmsg(flags,errbuf),0) ;
/*	No more lock - remember */
	seglist->segment[i].lock_id = 0 ;
 } ;

/*	iou_seg_update( buffer ) - Writes out all updated pages within a segment to disk */

iou_seg_update()
 { char errbuf[150] ;
   struct str__ref sstr ;
  int i,flags ;

	stru_popstr(&sstr) ;		/* Get the buffer location */
/*	Look for the buffer in the seglist table */
	for(i=0;i<seglist->count;i++) { if (sstr.ptr == seglist->segment[i].pointer) break ; } ;
	if (i >= seglist->count)
	 v3_error(V3E_NOSEGMENT,"SEG","UPDATE","NOSEGMENT","Segment not entered into V3 table via SEG_SHARE",0) ;
/*	Make call to update */
	flags = sys$updsecw(&seglist->segment[i].pointer,0,0,0,0,0,0,0) ;
	if ((flags & 1) == 0) v3_error(V3E_NOUPDATE,"SEG","UPDATE","NOUPDATE",mscu_vms_errmsg(flags,errbuf),0) ;
 } ;

/*	P A C K A G E  /  E X E C U T E - O N L Y   R O U T I N E S	*/
/*	( V A X  /  V M S   V E R S I O N				*/

/*	vax_xct_load - Loads package for execute-only (no parse) */
/*	  (returns pointer to new package)			*/

vax_xct_load(file_name,flag)
  char *file_name ;		/* Name of file to be loaded */
  int flag ;			/* If TRUE then no error if package already loaded- just ignore this request */
 { struct db__package *pckp ;
   struct pf__package_header *ph ;	/* Package header */
   struct db__module_entry *me ;
   FILE *file_ptr ;
   char tstr[200] ; short int i ; int bytes ;
   struct FAB v3x_fab ;		/* To access V3X file via mapped section */
   struct NAM v3x_nam ;		/* XAB for file name */
   char xnam[V3_IO_FILENAME_MAX+1] ;	/* Holds file name */
   int sec_address[2] ;		/* Holds address range of section */
   int ret_address[2] ;
   int package_ident[2] ;
   int vms_res ; char errbuf[250] ;
   unsigned char vmslot ;
   int nss,ss ; int initial_address ;
   struct {
     short int length,desc ;
     char *pointer ;
    } sp ;
   globalvalue int SEC$M_CRF,SEC$M_GBL,SEC$M_WRT ;
   globalvalue int PRT$C_UR ;	/* Protection for pages: user read */

/*	Open the file via rms for user-only access */
	v3x_fab = cc$rms_fab ; v3x_nam = cc$rms_nam ;
	v3x_fab.fab$l_fna = file_name ; v3x_fab.fab$b_fns = strlen(file_name) ;
	v3x_fab.fab$l_dna = ".v3x" ; v3x_fab.fab$b_dns = 4 ;
	v3x_fab.fab$l_fop = FAB$M_UFO ;
	v3x_fab.fab$l_nam = &v3x_nam ; v3x_nam.nam$l_rsa = &xnam ; v3x_nam.nam$b_rss = V3_IO_FILENAME_MAX ;
	vms_res = sys$open(&v3x_fab) ;
/*	Did we open it ok ? */
	if (!(vms_res & 1))
	 { v3_error(V3E_FILEOPEN,"PCK","OLD_XCT","FILEOPEN",mscu_vms_errmsg(vms_res,errbuf),0) ; } ;
/*	Now find a free slot in virtual memory */
	for(vmslot=0;process->vms_vm_slots[vmslot]!=0;vmslot++) ;
	bytes = (v3x_fab.fab$l_alq*V3_VMS_PAGELET_SIZE + 2*V3_VMS_PAGE_SIZE) ;
	sec_address[1] = (sec_address[0] = ((malloc(bytes) + V3_VMS_PAGE_SIZE) & ~(V3_VMS_PAGE_SIZE-1))) ;
	sec_address[1] += V3_VMS_PAGE_SIZE-1 ;
/*	Now try to map first page (with package header) into memory section */
	vms_res = sys$crmpsc(&sec_address,&ret_address,0,SEC$M_CRF+SEC$M_WRT,0,0,0,v3x_fab.fab$l_stv,
			V3_VMS_PAGE_SIZE/V3_VMS_PAGELET_SIZE,0,0,0) ;
	if (!(vms_res & 1))
	 { v3_error(V3E_CRMPSC_HDR,"PCK","OLD_XCT","CRMPSC_HDR",mscu_vms_errmsg(vms_res,errbuf),0) ; } ;
	initial_address = ret_address[0] ; ph = ret_address[0] ;
/*	Looks good, link up pointers & go */
	if ((pckp = process->package_ptrs[ph->package_id]) != NULL)
	 { if (!flag)
	    v3_error(V3E_INUSE,"PCK","XCT_LOAD","INUSE","Package slot already in use",ph->package_id) ;
/*	   Undo what we just did ! */
	   sys$deltva(&ret_address,0,0) ; sys$dassgn(v3x_fab.fab$l_stv) ;
	   return(pckp) ;
	 } ;
	if (ph->v3_version_major != V3_VERSION_MAJOR)
	 v3_error(V3E_BADVER,"PCK","XCT_LOAD","BADVER","Version mismatch",ph->v3_version_major) ;
	if (ph->is_parse)
	 v3_error(V3E_HDRNOTXCT,"PCK","XCT_LOAD","HDRNOTXCT","File not in valid xct load format",tstr) ;
/*	Figure out extent of nonsharable section */
	nss = ph->total_nonshare_bytes / V3_VMS_PAGE_SIZE ; if (nss == 0) nss = 1 ;
	if (nss > 1)
	 { sec_address[1] = initial_address + (nss - 1)*V3_VMS_PAGE_SIZE + (V3_VMS_PAGE_SIZE-1) ;
	   sec_address[0] = initial_address + V3_VMS_PAGE_SIZE ;
	   vms_res = sys$crmpsc(&sec_address,&ret_address,0,SEC$M_CRF+SEC$M_WRT,0,0,0,v3x_fab.fab$l_stv,
			(nss-1)*(V3_VMS_PAGE_SIZE/V3_VMS_PAGELET_SIZE),1+(V3_VMS_PAGE_SIZE/V3_VMS_PAGELET_SIZE),0,0) ;
	   if (!(vms_res & 1))
	    { v3_error(V3E_CRMPSC_NONSHARE,"PCK","OLD_XCT","CRMPSC_NONSHARE",mscu_vms_errmsg(vms_res,errbuf),0) ; } ;
	 } ;
/*	Figure out package "ident" from its checksum */
	package_ident[0] = 1 ; package_ident[1] = ph->package_checksum ;
/*	Now map in the sharable section */
	ss = 1 + (ph->total_xct_bytes - ph->total_nonshare_bytes) / V3_VMS_PAGE_SIZE ;
	sec_address[0] = ret_address[1] + 1 ;
	sec_address[1] = sec_address[0] + (ss - 1)*V3_VMS_PAGE_SIZE + (V3_VMS_PAGE_SIZE-1) ;
	sp.length = strlen(ph->package_name) ; sp.desc = 0 ; sp.pointer = &ph->package_name ;
	vms_res = sys$crmpsc(&sec_address,&ret_address,0,SEC$M_GBL,&sp,&package_ident,0,v3x_fab.fab$l_stv,
			ss*(V3_VMS_PAGE_SIZE/V3_VMS_PAGELET_SIZE),1+(nss*(V3_VMS_PAGE_SIZE/V3_VMS_PAGELET_SIZE)),0,0) ;
	if (!(vms_res & 1))
	 { v3_error(V3E_CRMPSC_SHARE,"PCK","OLD_XCT","CRMPSC_SHARE",mscu_vms_errmsg(vms_res,errbuf),0) ; } ;
/*	Package looks ok, allocate & start filling in the pieces */
	process->package_ptrs[ph->package_id] = (pckp = calloc(1,sizeof *pckp)) ;
/*	Copy info from package header */
	pckp->package_id = ph->package_id ; strcpy(pckp->package_name,ph->package_name) ;
	pckp->id_obj = ph->id_obj ; strcpy(pckp->startup_module,ph->startup_module) ;
	pckp->total_xct_bytes = ph->total_xct_bytes ; pckp->total_nonshare_bytes = ph->total_nonshare_bytes ;
	pckp->vms_addresses[0] = initial_address ; pckp->vms_addresses[1] = ret_address[1] ;
	pckp->vms_slot = vmslot ; pckp->vms_fab = v3x_fab ;
	process->vms_vm_slots[vmslot] = 1 ;
/*	Copy in the full VMS file name */
	xnam[v3x_nam.nam$b_rsl] = 0 ; strcpy(pckp->file_name,xnam) ;
/*	Link up the pointers to all pieces of the package */
#define PHL(PTR,FO) pckp->PTR = initial_address + ph->FO.loc ;
	PHL(code_ptr,code) ; PHL(impure_ptr,impure) ; PHL(pure_ptr,pure) ; PHL(checksum_ptr,checksum) ;
	PHL(ob_bucket_ptr,ob_bucket) ; PHL(name_table_ptr,name_table) ; PHL(skeleton_ptr,skeletons) ;
	for (i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++)
	 { PHL(name_ob_ptrs[i],name_ob_pairs[i]) ; } ;
/*	Got everything loaded - check out global/external symbols */
	pcku_load_package(pckp) ;
/*	Maybe redo object search list */
	sobu_calculate_name_ob_list() ;
/*	Twiddle the skeletons before setting page protections */
	for(i=0;i<pckp->skeleton_ptr->count;i++)
	 { sobu_load_skeleton(pckp->skeleton_ptr,&pckp->skeleton_ptr->buf[pckp->skeleton_ptr->index[i]],NULL) ; } ;
	return(pckp) ;
}