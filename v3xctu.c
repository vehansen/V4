/*	V3XCTU.C - EXECUTION UTILITIES FOR VICTIM III

	LAST EDITED 6/29/84 BY VICTOR E. HANSEN		*/

#include <signal.h>
#include <time.h>
#include "v3defs.c"
#ifdef POSIX
#ifdef INCSYS
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
 #ifndef LINUX486
  #include <sys/termio.h>
 #endif
#else
#include <types.h>
#endif
#endif

#ifdef WINNT
#include <windows.h>
#endif

#ifdef RS6000AIX
#include <time.h>
#endif

#ifdef LINUX486
#include <asm/ioctls.h>
#endif

/*	G L O B A L   E X E C U T I O N   S Y M B O L S		*/

struct db__process *process ;	/* Points to current process */
struct db__psi *psi ;		/* Points to current psi */
struct db__psi *current_psi ;	/* Used to detect change in psi */
int ctrlp_interrupt ;
jmp_buf xct_continue ;	/* Global execution continue trap */
union val__format tmp_val ;	/* Temp value */
struct db__opr_info *op ;	/* Pointer to V3 predefined function */
extern struct db__parse_info *parse ;
extern int PRS_FUNCTION_COUNT ; /* Number of V3 predefined functions */
extern struct db__predef_table V3_FUNCTIONS[] ;
struct iou__openlist ounits ;	/* List of open units */
int V3_OPCODE_USAGE[300] ;	/* Opcode usage table */
char ttystr[V3_PRS_INPUT_LINE_MAX+1] ;	/* TTY_GETx input buffer */
char ttytrm[V3_TTY_TERMINATOR_MAX+1] ;	/* Holds terminator string after TTY input */
struct num__ref anum ;
int mod_watchdog_id = 0 ;	/* ID of last mod_watchdog() TIMER entry */

char *stru_popcstr() ;
struct db__mod_item *xctu_mod_item_alloc() ;
struct db__psi *xctu_cmd_invoke() ;
char *xctu_pointer() ;
void ctrlp() ;
struct db__module_entry *xctu_call_module_entry(/* format,where,mreupd */) ;
B64INT mthu_cvtii() ;
char *xctu_popptr() ;

int *watch_location = NULL ;
int watch_value ;

/*	P R O C E S S   C R E A T I O N			*/

char months[12][4] =
 { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" } ;
char ucmonths1[12][4] =
 { "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC" } ;
char long_months[12][10] =
 { "January","February","March","April","May","June","July","August",
   "September","October","November","December" } ;
struct db__process *process_init_new()
{
  int i ;

	if (process != NULL) return(process) ;		/* Already been here */
/*	Create new process & psi */
	process = (struct db__process *)v4mm_AllocChunk(sizeof *process,TRUE) ;
	process->name_ob_count = (process->assertion_count = 0) ;
/*	Create fork 0 Stuff */
	process->fork[process->current_fork = 0].stack_base = (V3STACK *)v4mm_AllocChunk(SIZEofSTACK*V3_PROCESS_STACK_SIZE,TRUE) ;
	psi = (struct db__psi *)v4mm_AllocChunk(sizeof *psi,TRUE) ;
	process->fork[0].startup_psi = psi ; process->fork[0].current_psi = psi ;
	strcpy(process->fork[0].name,"BOOT") ;
	psi->reset_stack_ptr = (psi->stack_ptr = (V3STACK *)(process->fork[0].stack_base + V3_PROCESS_STACK_SIZE - 1)) ;
/*	Push an initial EOA onto stack */
	PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
	psi->prior_psi_ptr = NULLV ;
	process->minutes_west = mscu_minutes_west() ;
/*	Set up USA specific country parameters */
	for(i=0;i<12;i++)
	 { strcpy(process->ci.ucMonths[i],ucmonths1[i]) ; strcpy(process->ci.Months[i],months[i]) ;
	   strcpy(process->ci.LongMonths[i],long_months[i]) ;
	 } ;
	process->ci.NumericFldDelim = ',' ;
	process->ci.DecimalPlaceDelim = '.' ;
	process->ci.NegNumPrefix = '-' ;
	process->ci.NegNumSuffix = '-' ;
	process->ci.NegNumLeft = '(' ;
	process->ci.NegNumRight = ')' ;
	process->ci.NumOverflow = '*' ;
	process->ci.NumFill = '*' ;
	process->ci.DashIfZero = '-' ;
	strcpy(process->ci.LeadingDollar,"$") ;
	process->ci.TimeDelim = ':' ;
	process->ci.DefaultDateMask = 0 ;
	process->ci.DefaultDTMask = 0 ;
	process->ci.DefaultMonthMask = 0 ;
	strcpy(process->ci.UnixMonthList,"JanFebMarAprMayJunJulAugSepOctNowDec") ;
	strcpy(process->ci.now,"now") ; strcpy(process->ci.NOW,"NOW") ;
	strcpy(process->ci.today,"today") ; strcpy(process->ci.TODAY,"TODAY") ;
	strcpy(process->ci.tomorrow,"tomorrow") ; strcpy(process->ci.TOMORROW,"TOMORROW") ;
	strcpy(process->ci.yesterday,"yesterday") ; strcpy(process->ci.YESTERDAY,"YESTERDAY") ;
	strcpy(process->ci.yes,"yes") ; strcpy(process->ci.YES,"YES") ;
	strcpy(process->ci.no,"no") ; strcpy(process->ci.NO,"NO") ;
	strcpy(process->ci.lTRUE,"TRUE") ; strcpy(process->ci.lFALSE,"FALSE") ;
	strcpy(process->ci.none,"none") ; strcpy(process->ci.NONE,"NONE") ;
/*	Return pointer to newly created process */
	return(process) ;
}

/*	M A I N   E X E C U T I O N   L O O P		*/

void xctu_main()
{ struct db__package *package_ptr ;	/* Pointer to current package */
   union db__module_code_element element ; /* Code element */
   struct db__psi *ret_psi ;
   union val__mempos *ref ;
   union val__format vf ;
   struct db__formatmempos *fm ;
   struct db__formatptr *fp ;
   extern V3OPEL return_code[] ;
   struct db__package *ptr ;
   int *impure_ptr ;		/* Pointer to current package impure space */
   int *impurelm_ptr ;		/* Pointer to module specific impure space */
   int *pure_ptr ;		/* Pointer to pure */
   int i,j,*intptr ; int *(*mp_ptr) ;

/*	Set up long_jump for xct continuation */
	setjmp(xct_continue) ;

/*	Big loop		*/
	for (;;)
	 { current_psi = psi ;
	   package_ptr = process->package_ptrs[psi->package_index] ;
/*	   Update local symbols for faster run times */
	   if (psi->pure_base != NULLV)
	    { impure_ptr = package_ptr->impure_ptr ; pure_ptr = psi->pure_base ; }
	    else { impure_ptr = package_ptr->impure_ptr ; pure_ptr = package_ptr->pure_ptr ; } ;
	   impurelm_ptr = psi->impurelm_base ;
/*	   Another loop until psi changes */
	   do
	    {	element.all = *(psi->code_ptr++) ;	/* Get next "instruction" */
		switch (element.fld.type)
		 { case CODE_OPERATION:
			if (v3_operations(element.fld.offset)) break ;
/*			It looks like we should return */
			return ;
		   case CODE_PUREIV:
			fm = (struct db__formatmempos *)(pure_ptr+element.fld.offset) ;
			PUSHVI(fm->mempos.all) ; PUSHF(fm->format.all) ; break ;
/*
			PUSHVI(*(intptr = (int *)(pure_ptr+element.fld.offset+1))) ;
			PUSHF(*(intptr = (int *)(pure_ptr+element.fld.offset))) ; break ;
*/
		   case CODE_PUREPP:
			fp = (struct db__formatptr *)(pure_ptr+element.fld.offset) ;
			PUSHMP(fp->ptr) ; PUSHF(fp->format.all) ; break ;
		   case CODE_PUREPV:
			ref = (union val__mempos *)(pure_ptr + element.fld.offset + 1) ;
			switch (ref->fld.value_type)
	 		 { default: v3_error(V3E_BADPUREPV,"XCT","MAINLOOP","BADPUREPV","Invalid address type for PUREPV",(void *)ref->fld.value_type) ;
	   		   case VAL_STATIMP: PUSHMP(impure_ptr + ref->statimp.offset) ; break ;
	   		   case VAL_STATIMPLM: PUSHMP(impurelm_ptr + ref->statimp.offset) ; break ;
	   		   case VAL_STATPURE: PUSHMP(pure_ptr + ref->statpure.offset) ; break ;
			   case VAL_STACK: PUSHMP(psi->stack_space + ref->stack.offset) ; break ;
			   case VAL_ARG: PUSHMP(psi->arg_ptr - ref->arg.offset) ; break ;
	 		 } ;
			PUSHF(*(intptr = (int *)(pure_ptr+element.fld.offset))) ; break ;
		   case CODE_PUREPVLMO:
			ref = (union val__mempos *)(pure_ptr + element.fld.offset + 1) ; PUSHMP((char *)impurelm_ptr + ref->all) ;
			PUSHF(*(intptr = (int *)(pure_ptr+element.fld.offset))) ; break ;
		   case CODE_PUREPVIO:
			ref = (union val__mempos *)(pure_ptr + element.fld.offset + 1) ; PUSHMP((char *)impure_ptr + ref->all) ;
			PUSHF(*(intptr = (int *)(pure_ptr+element.fld.offset))) ; break ;
		   case CODE_PURE:
			PUSHVP(*(intptr = (int *)(pure_ptr+element.fld.offset+1))) ;
			PUSHF(*(intptr = (int *)(pure_ptr+element.fld.offset))) ; break ;
		   case CODE_IMPUREIV:
			PUSHVP(*(intptr = (int *)(impure_ptr+element.fld.offset+1))) ;
			PUSHF(*(intptr = (int *)(impure_ptr+element.fld.offset))) ; break ;
		   case CODE_IMPUREPV:
			ref = (union val__mempos *)(impure_ptr + element.fld.offset + 1) ;
			switch (ref->fld.value_type)
	 		 { default: v3_error(V3E_BADIMPUREPV,"XCT","MAINLOOP","BADIMPUREPV","Invalid address type for IMPUREPV",(void *)ref->fld.value_type) ;
	   		   case VAL_STATIMP: PUSHMP(impure_ptr + ref->statimp.offset) ; break ;
	   		   case VAL_STATPURE: PUSHMP(pure_ptr + ref->statpure.offset) ; break ;
			   case VAL_STACK: PUSHMP(psi->stack_space + ref->stack.offset) ; break ;
			   case VAL_ARG: PUSHMP(psi->arg_ptr - ref->arg.offset) ; break ;
	 		 } ;
			PUSHF(*(intptr = (int *)(impure_ptr+element.fld.offset))) ; break ;
		   case CODE_IMPURE:
			PUSHVP(*(intptr = (int *)(impure_ptr+element.fld.offset+1))) ;
			PUSHF(*(intptr = (int *)(impure_ptr+element.fld.offset))) ; break ;
		   case	CODE_INT_LITERAL:
			PUSHINT(element.fld.offset) ; break ;
		   case CODE_FREQOP:
			switch(element.fld.offset)
			 { default:
				printf("Missing FREQOP entry for opcode=%d\n",element.fld.offset) ;
				v3_operations(element.fld.offset) ; break ;
			   case V3_XCT_PUSHEOA:
				PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ; break ;
	   		   case V3_XCT_EOS:
				psi->stack_ptr = psi->reset_stack_ptr ; break ;
	   		   case V3_XCT_EOSLN:	/* Same as EOS but skips over source line number in next code slot */
				element.all = *(psi->code_ptr++) ; psi->line_number = element.all ; /* Get next "instruction" */
				psi->stack_ptr = psi->reset_stack_ptr ;
				if (watch_location != NULL)
				 { if (*watch_location != watch_value)
				    { printf("Watch variable changed from %d to %d at line number %d\n",
						watch_value,*watch_location,psi->line_number) ;
				      watch_value = *watch_location ;
				      v3_error(V3E_VALCHANGE,"V3WATCH","CHKVAL","VALCHANGE","Watched variable changed",(void *)watch_value) ;
				    } ;
				 } ;
				break ;
	   		   case V3_XCT_IF:
				if (xctu_popint() > 0) { psi->code_ptr++ ; break ; } ;
/*				Have FALSE - jump to proper location */
				if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *(psi->code_ptr) ; }
		 		 else psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *(psi->code_ptr) ;
				break ;
	   		   case V3_XCT_JUMP:
				if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *psi->code_ptr ; }
		 		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *psi->code_ptr ; } ;
				break ;
			   case V3_XCT_DUMMYREF:
				i = *(psi->code_ptr++) ;
				PUSHVP(*(psi->arg_ptr-i+1)) ; PUSHF(*(psi->arg_ptr-i)) ;
				break ;
	   		   case	V3_XCT_PARTIAL:
/*				Get address of top-of-stack & assign as value of second */
				POPF(vf.all) ; POPVP(intptr,(int *)) ;
				POPF(vf.all) ; POPVP(mp_ptr,(int *(*))) ;
				*mp_ptr = intptr ; break ;
			   case V3_XCT_STRUCTPTR:
				POPF(vf.all) ; POPVI(i) ; POPF(j) ; POPVP(mp_ptr,(int *(*))) ;
				if (*mp_ptr == NULLV)
				 v3_error(V3E_STRUCTPTRZERO,"XCT","STRUCTPTR","STRUCTPTRZERO","Pointer cannot be equal to 0",intptr) ;
				PUSHMP(  ((char *)*mp_ptr)+i  ) ; PUSHF(vf.all) ;
				break ;
	   		   case V3_XCT_JUMPC:
/*				JUMPC is a special form of jump from a command */
/*				It uses the full form of: JUMP offset,offset_level,command_level */
/*				 to determine if a psi recovery is necessary */
				i = *(psi->code_ptr) ;	/* i = offset for jump */
				j = *(++psi->code_ptr) ; /* j = level we are jumping to */
/*				If the level we are jumping to is outside of the outer command level then have to restore psi */
				if (j >= *(++psi->code_ptr))
		 		 { if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + i ; }
		    		    else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + i ; } ;
		   		   break ;
		 		 } ;
/*				If here then restore psi to nesting of module defining the command */
				for (ret_psi=psi;ret_psi!=psi->command_psi;ret_psi=ret_psi->prior_psi_ptr)
		 		 { ret_psi->code_ptr = return_code ;	/* Set code_ptr to dummy area for all nested psi levels */
		 		 } ;
				if (psi->pic_base != NULLV) { psi->command_psi->code_ptr = psi->pic_base + i ; }
		 		 else { psi->command_psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + i ; } ;
				break ;
			 } ; break ;
		  } ;
	   } while (current_psi == psi) ;
	 } ;
}

/*	M O D U L E   C A L L			*/

void xctu_call(cmdflag)
  int cmdflag ;					/* If TRUE then from command- just copy current psi (no module entry!) */
{ union val__format mod_ref ;
   struct db__package *package ;
   struct db__module_entry *mentry ;
   struct db__psi *new_psi ;
   struct db__mod_item *itemp ;
   int ac,sb ; V3STACK *lptr ; char *bp ;

/*	Set up new psi environment */
	new_psi = (struct db__psi *)((char *)psi->stack_ptr - (((sizeof *psi) + ALIGN_DOUBLE) & ~ALIGN_DOUBLE) ) ;
/*	Position past EOA to module reference */
	for (ac=0;;ac++)
	 { POPF(mod_ref.all) ; POPVI(sb) ;
	   if (mod_ref.all == V3_FORMAT_EOA) break ;
	 } ;
/*	Pop module reference off of stack */
	if (cmdflag)
	 { mentry = psi->mep ;
	   new_psi->arg_ptr = psi->stack_ptr-(2*SIZEOFSTACKENTRY) ;
	 } else
	 { POPF(mod_ref.all) ; POPVP(bp,(char *)) ;
	   if (mod_ref.fld.type != VFT_MODREF)
	    { v3_error(V3E_UNDEFMOD,"XCT","CALL","UNDEFMOD","Undefined module reference",NULLV) ; return ; } ;
	   mentry = xctu_call_module_entry(mod_ref.all,bp,&new_psi->mre) ;
	   new_psi->arg_ptr = psi->stack_ptr-(3*SIZEOFSTACKENTRY) ;
	 } ;
	new_psi->prior_psi_ptr = psi ;	/* Link old to new */
	new_psi->package_index = mentry->package_id ;
	new_psi->code_ptr = (V3OPEL *)((char *)mentry + mentry->code_offset) ;
	new_psi->pic_base = (mentry->stack_bytes < 0 ? new_psi->code_ptr : NULLV) ;
	new_psi->pure_base = (int *)((char *)mentry + mentry->pure_offset) ;
	new_psi->arg_cnt = ac ;
	new_psi->mep = mentry ;	/* Save for possible command lookup */
	new_psi->command_psi = NULLV ; new_psi->item_list = NULLV ;
	new_psi->command_index = 0 ;
	new_psi->interrupt_index = 0 ;		/* Not in an interrupt */
	new_psi->line_number = 0 ;
	new_psi->impurelm_base = new_psi->mre->impurelm_base ;

/*	Adjust stack pointer for module stack storage */
	lptr = (V3STACK *)(psi = new_psi) ;
	sb = (mentry->stack_bytes < 0 ? -mentry->stack_bytes : mentry->stack_bytes) ;
/*	If stack storage is real big then allocate from heap */
	if (sb > V3_MODULE_DYNAMIC_MAX)
	 { psi->stack_space = (char *)v4mm_AllocChunk(sb,FALSE) ;
	   if (psi->stack_space == 0)
	    v3_error(V3E_INSDYNMEM,"HEAP","ALLOC","INSDYNMEM","Insufficient heap memory for dynamic module invocation",0) ;
/*	   Create module stack item to deallocate this space */
	   itemp = (struct db__mod_item *)xctu_mod_item_alloc() ; itemp->owner_psi = psi ;
	   psi->item_list = itemp ; itemp->prior = NULLV ;
	   itemp->format.all = V3_FORMAT_INTEGER ; itemp->ptr = (char *)psi->stack_space ;
	   itemp->v3_exit_func = V3_FLAGS_MOD_HEAP_FREE ; itemp->name[0] = 0 ;
	   psi->reset_stack_ptr = (psi->stack_ptr = lptr) ;
	   return ;
	 } ;
	psi->reset_stack_ptr = (psi->stack_ptr = (V3STACK *)((char *)lptr - ((sb+ALIGN_MAX) & ~ALIGN_MAX) )) ;
	psi->stack_space = (char *)psi->stack_ptr ;
	return ;
}

/*	xctu_call_module_entry - Find module_entry definition for a module */

struct db__module_entry *xctu_call_module_entry(formatall,ptr,mreupd)
  int formatall ;
  char *ptr ;
  struct db__module_runtime_entry *(*mreupd) ;
{ struct db__module_link_list *mll ;
  struct db__module_runtime_entry *mre ;
  union val__format format ;
  struct db__module_objentry *moe ;
  struct db__module_entry *mep ;
  int hx,hash,trycnt,offset ; char ebuf[100] ;

	format.all = formatall ;
	for(trycnt=0;;trycnt++)
	 { switch (format.fld.mode)
	    { default: v3_error(V3E_INVMODREF,"XCT","CALLMOD","INVMODREF","Invalid module reference",(void *)format.fld.mode) ;
	      case VFM_IMMEDIATE:	v3_error(V3E_V3OPREF,"XCT","CALLMOD","V3OPREF","Cannot reference V3 operator from module call",(void *)0) ;
	      case VFM_MEP:
		mep = (struct db__module_entry *)ptr ;
		hash = prsu_str_hash(mep->name,-1) ;
		for(hx=hash;mreupd!=NULLV;hx++)				/* Search hash table for mre */
		 { hx = hx % V3_PROCESS_MODULE_MAX ;
		   if (process->mht.entry[hx].module_hash == 0)
		    v3_error(V3E_MODHTBAD,"XCT","CALLMOD","MODHTBAD","Module hash table for process is trashed!",(void *)0) ;
		   if (process->mht.entry[hx].module_hash != hash) continue ;
		   if (strcmp(process->mht.entry[hx].name,mep->name) != 0) continue ;
		   if (process->mht.entry[hx].runtime_index == 0)
		    v3_error(V3E_MODHTBAD,"XCT","CALLMOD","MODHTBAD","Module hash table for process is trashed!",(void *)0) ;
		   *mreupd = process->mre[process->mht.entry[hx].runtime_index] ;
		   break ;
		 } ;
		return(mep) ;
	      case VFM_PTR:				/* Pointer to db__module_objentry */
		moe = (struct db__module_objentry *)ptr ;
		for(hx=moe->module_hash;;hx++)
		 { hx = hx % V3_PROCESS_MODULE_MAX ;
		   if (process->mht.entry[hx].module_hash == 0)
		    { if (trycnt == 0) { if (xctu_attempt_v4_load(moe->name)) goto try_again ; } ;
		      sprintf(ebuf,"Undefined module reference1- %s/%d/%d",moe->name,moe->module_hash,hx) ;
		      v3_error(V3E_UNDEFMOD,"XCT","CALLMOD","UNDEFMOD",ebuf,0) ;
		    } ;
		   if (process->mht.entry[hx].module_hash != moe->module_hash) continue ;
		   if (strcmp(process->mht.entry[hx].name,moe->name) != 0) continue ;
		   if (process->mht.entry[hx].runtime_index == 0)
		    { if (trycnt == 0) { if (xctu_attempt_v4_load(moe->name)) goto try_again ; } ;
		      sprintf(ebuf,"Undefined module reference2- %s",moe->name) ;
		      v3_error(V3E_UNDEFMOD,"XCT","CALLMOD","UNDEFMOD",ebuf,0) ;
		    } ;
		   if (mreupd != NULLV) *mreupd = process->mre[process->mht.entry[hx].runtime_index] ;
		   return(process->mre[process->mht.entry[hx].runtime_index]->mentry) ;
		 } ;
	      case VFM_OFFSET:
		if (psi->mep == NULLV)
		 v3_error(V3E_NOTINMOD,"XCT","CALLMOD","NOTINMOD","Cannot reference module index outside of module (i.e. from interpretive)",0) ;
		mre = psi->mre ; offset = (int)ptr ;
		if (offset < 0 || offset >= mre->count)
		 { sprintf(ebuf,"Invalid module (%s - %d) index (%d)",mre->name,mre->count,offset) ;
		   v3_error(V3E_INVMODX,"XCT","CALLMOD","INVMODX",ebuf,(void *)offset) ;
		 } ;
		if (mre->ext_mre[offset] != NULLV)
		 { if (mreupd != NULLV) *mreupd = mre->ext_mre[offset] ;
		   return(mre->ext_mre[offset]->mentry) ;
		 } ;
/*		Module may be undefined or just not yet linked in- try to link */
		mll = (struct db__module_link_list *)((char *)psi->mep + psi->mep->module_link_offset) ;
		hash = prsu_str_hash(mll->entry[offset].name,-1) ;
		for(hx=hash;;hx++)
		 { hx = hx % V3_PROCESS_MODULE_MAX ;
		   if (process->mht.entry[hx].module_hash == 0)
		    { if (trycnt == 0) { if (xctu_attempt_v4_load((char *)&mll->entry[offset])) goto try_again ; } ;
		      sprintf(ebuf,"Undefined module reference3- %s",(char *)mll->entry[offset].name) ;
		      v3_error(V3E_UNDEFMOD,"XCT","CALLMOD","UNDEFMOD",ebuf,0) ;
		    } ;
		   if (process->mht.entry[hx].module_hash != hash) continue ;
		   if (strcmp(process->mht.entry[hx].name,mll->entry[offset].name) != 0) continue ;
		   if (process->mht.entry[hx].runtime_index == 0)
		    { if (trycnt == 0) { if (xctu_attempt_v4_load((char *)&mll->entry[offset])) goto try_again ; } ;
		      sprintf(ebuf,"Undefined module reference4- %s",mll->entry[offset].name) ;
		      v3_error(V3E_UNDEFMOD,"XCT","CALLMOD","UNDEFMOD",ebuf,0) ;
		    } ;
		   mre->ext_mre[offset] = process->mre[process->mht.entry[hx].runtime_index] ;
		   if (mreupd != NULLV) *mreupd = process->mre[process->mht.entry[hx].runtime_index] ;
		   return(process->mre[process->mht.entry[hx].runtime_index]->mentry) ;
		 } ;
	    } ;
try_again: continue ;
	 } ;
}

/*	xctu_attempt_v4_load - Maybe try and load undefined module via V4 database	*/
/*	Call: result = xctu_attempt_v4_load( modname )
	  where result is TRUE if something loaded, FALSE otherwise,
		modname is pointer to module name string to load			*/

int xctu_attempt_v4_load(modname)
  char *modname ;
{ struct V4LEX__TknCtrlBlk tcb ;
  char v4eval[250] ; int i,len ;

	if (strlen(process->v4_eval_on_undefmod) == 0) return(FALSE) ;	/* Nothing to try */
	for(i=0,len=0;;i++)
	 { switch (process->v4_eval_on_undefmod[i])
	    { case 0: 	v4eval[len] = 0 ; goto out_loop ;
	      case '*':	v4eval[len] = 0 ; strcat(v4eval,modname) ; len = strlen(v4eval) ; break ;
	      default:	v4eval[len++] = process->v4_eval_on_undefmod[i] ; break ;
	    } ;
	 } ;
out_loop:
	process->have_V4intercept = TRUE ;			/* Set up to intercept all V4 errors */
	if (setjmp(process->error_intercept) != 0) return(FALSE) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(v4eval),V4LEX_InpMode_String) ;
	v4eval_Eval(&tcb,process->ctx,FALSE,FALSE,FALSE,FALSE,FALSE,NULL) ;	/* Evaluate the argument */
	process->have_V4intercept = FALSE ;
	return(TRUE) ;
}

/*	xctu_call_defer - Handles user modules or V3 primitives */

void xctu_call_defer(modref)
 struct db__formatptr *modref ;
{ char needeoa ;
   union val__format format1,format2 ;
   char *val1,*val2 ;

/*	First see if V3 primitive or user module */
	if (modref->format.fld.type != VFT_MODREF)
	 v3_error(V3E_UNDEFMOD,"MOD","CALL_DEFER","UNDEFMOD","Undefined module reference",NULLV) ;
	switch (modref->format.fld.mode)
	 { default: v3_error(V3E_INVFORMATMODE,"MOD","CALL_DEFER","INVFORMATMODE","Invalid format mode for module call",(void *)modref->format.fld.mode) ;
	   case VFM_MEP:	xctu_call_mod((struct db__module_entry *)modref->ptr) ; return ;
	   case VFM_PTR:
	   case VFM_OFFSET:
		xctu_call_mod(xctu_call_module_entry(modref->format.all,modref->ptr,NULLV)) ;
		return ;
	   case VFM_IMMEDIATE:	break ;		/* Fall thru to handle primitive! */
	 } ;
/*	It's a V3 primitive !	*/
	needeoa = modref->format.fld.decimals ;
/*	Call the V3 handlers */
	v3_operations((int)modref->ptr) ;
/*	Now see if we need to get rid of EOA */
	if (needeoa) return ;	/* All is well */
/*	Have to get rid of EOA on stack */
	POPF(format1.all) ; POPVP(val1,(char *)) ; if (format1.all == V3_FORMAT_EOA) return ;
	for (;;)
	 { POPF(format2.all) ; POPVP(val2,(char *)) ; if (format2.all == V3_FORMAT_EOA) break ; } ;
	PUSHMP(val1) ; PUSHF(format1.all) ;
	return ;
}

/*	xctu_call_mod - Same as xctu_call but module entry passed as argument, not on stack */

void xctu_call_mod(module_entry)
  struct db__module_entry *module_entry ;
{ union val__format mod_ref ;
   struct db__package *package ;
   struct db__mod_item *itemp ;
   struct db__psi *new_psi ;
   int i,j,ac,sb,hash,hx ; V3STACK *lptr ;

/*	Set up new psi environment */
	new_psi = (struct db__psi *)((char *)psi->stack_ptr - (((sizeof *psi) + ALIGN_DOUBLE) & ~ALIGN_DOUBLE) ) ;
	hash = prsu_str_hash(module_entry->name,-1) ;
	for(hx=hash;;hx++)				/* Search hash table for mre */
	 { hx = hx % V3_PROCESS_MODULE_MAX ;
	   if (process->mht.entry[hx].module_hash == 0)
	    v3_error(V3E_MODHTBAD,"XCT","CALLMOD","MODHTBAD","Module hash table for process is trashed!",0) ;
	   if (process->mht.entry[hx].module_hash != hash) continue ;
	   if (strcmp(process->mht.entry[hx].name,module_entry->name) != 0) continue ;
	   if (process->mht.entry[hx].runtime_index == 0)
	    v3_error(V3E_MODHTBAD,"XCT","CALLMOD","MODHTBAD","Module hash table for process is trashed!",0) ;
	   new_psi->mre = process->mre[process->mht.entry[hx].runtime_index] ;
	   break ;
	 } ;
/*	Position past EOA to module reference */
	for (ac=0;;ac++)
	 { POPF(mod_ref.all) ; POPVI(i) ;
	   if (mod_ref.all == V3_FORMAT_EOA) break ;
	 } ;
	new_psi->prior_psi_ptr = psi ;	/* Link old to new */
	new_psi->package_index = module_entry->package_id ;
	new_psi->code_ptr = (V3OPEL *)((char *)module_entry + module_entry->code_offset) ;
	new_psi->pic_base = (module_entry->stack_bytes < 0 ? new_psi->code_ptr : NULLV) ;
	new_psi->pure_base = (int *)((char *)module_entry + module_entry->pure_offset) ;
	new_psi->arg_ptr = psi->stack_ptr ;
	new_psi->arg_cnt = ac ;
	new_psi->arg_ptr = psi->stack_ptr-(2*SIZEOFSTACKENTRY) ;
	new_psi->mep = module_entry ;	/* Save for possible command lookup */
	new_psi->command_psi = NULLV ; new_psi->item_list = NULLV ;
	new_psi->command_index = 0 ;
	new_psi->interrupt_index = 0 ;		/* Not in an interrupt */
	new_psi->line_number = 0 ;
	new_psi->impurelm_base = new_psi->mre->impurelm_base ;

/*	Adjust stack pointer for module stack storage */
	lptr = (V3STACK *)(psi = new_psi) ;
/*	If stack storage is real big then allocate from heap */
	sb = (module_entry->stack_bytes < 0 ? -module_entry->stack_bytes : module_entry->stack_bytes) ;
	if (sb > V3_MODULE_DYNAMIC_MAX)
	 { psi->stack_space = (char *)v4mm_AllocChunk(sb,FALSE) ;
	   if (psi->stack_space == 0)
	    v3_error(V3E_INSDYNMEM,"HEAP","ALLOC","INSDYNMEM","Insufficient heap memory for dynamic module invocation",0) ;
/*	   Create module stack item to deallocate this space */
	   itemp = (struct db__mod_item *)xctu_mod_item_alloc() ; itemp->owner_psi = psi ;
	   psi->item_list = itemp ; itemp->prior = NULLV ;
	   itemp->format.all = V3_FORMAT_INTEGER ; itemp->ptr = (char *)psi->stack_space ;
	   itemp->v3_exit_func = V3_FLAGS_MOD_HEAP_FREE ; itemp->name[0] = 0 ;
	   psi->reset_stack_ptr = (psi->stack_ptr = lptr) ;
	   return ;
	 } ;
	psi->reset_stack_ptr = (psi->stack_ptr = (V3STACK *)((char *)lptr - ((sb+ALIGN_MAX) & ~ALIGN_MAX) )) ;
	psi->stack_space = (char *)psi->stack_ptr ;
	return ;
}

/*	xctu_call_parser - Calls the VICTIM III parser as a module */

void xctu_call_parser(package_id,file_ptr,prompt_string,nest_flag)
  int package_id ;		/* Call parser for this package */
  FILE *file_ptr ;		/* NULLV or file pointer of file to load */
  char *prompt_string	 ;	/* Interactive prompt string or NULLV */
  int nest_flag ;		/* If TRUE then nest parser input */
{ struct db__package *pckp ;
   struct db__parse_info *prsp ;
   struct db__psi *new_psi ;

/*	Make sure package can be updated */
	if ((pckp = process->package_ptrs[package_id]) == NULLV)
	 { printf("Package slot %d not properly initialized- aborting\n",package_id) ; exit(EXIT_FAILURE) ; } ;
	if ((prsp = pckp->parse_ptr) == NULLV)
	 { printf("Package slot %d not initialized with parser information- aborting\n",package_id) ; exit(EXIT_FAILURE) ; } ;
/*	Set up input for parser */
	if (nest_flag) prsu_nest_input(prsp,file_ptr,prompt_string) ;
/*	Set up new psi environment */
	new_psi = (struct db__psi *)((char *)psi->stack_ptr - (((sizeof *psi) + ALIGN_DOUBLE) & ~ALIGN_DOUBLE) ) ;
	new_psi->prior_psi_ptr = psi ;	/* Link old to new */
	new_psi->package_index = package_id ;
	new_psi->code_ptr = NULLV ;
	new_psi->arg_ptr = NULLV ;
	new_psi->arg_cnt = 0 ;
	new_psi->arg_ptr = NULLV ;
	new_psi->mep = NULLV ;
	new_psi->command_psi = NULLV ; new_psi->item_list = NULLV ;
	new_psi->interrupt_index = 0 ;		/* Not in an interrupt */
	new_psi->impurelm_base = NULLV ;

/*	Adjust stack pointer for module stack storage */
	psi = new_psi ; psi->reset_stack_ptr = (psi->stack_ptr = (V3STACK *)psi) ; psi->stack_space = (char *)psi->stack_ptr ;
/*	Finally invoke the parser */
	prsu_parser() ;

	return ;
}

/*	M O D U L E   R E T U R N		*/

int xctu_return()
{ union val__format return_format ;
   struct db__psi *old_psi ;
   char *return_ptr ; unsigned char is_parse=FALSE ;
   extern jmp_buf parse_exit ;	/* For longjmp back into parser */

/*	If we have module items then go see if we should do some things */
	if (psi->item_list != NULLV) xctu_mod_item_exit(psi,TRUE) ;
/*	Pop something off of stack to see if we are returning a value */
	POPF(return_format.all) ; POPVP(return_ptr,(char *)) ;
/*	See if we are returning from parse call */
	if (psi->mep == NULLV) is_parse = TRUE ;
/*	Revert to prior psi environment */
	old_psi = psi ; psi = psi->prior_psi_ptr ;
/*	Zap current_psi so xctu_main doesn't get confused	*/
	current_psi = NULLV ;
	if (psi->code_ptr == NULLV)
	 { psi = old_psi ;
	   if (!is_parse) v3_error(V3E_TOOMANYRETS,"XCT","RETURN","TOOMANYRETS","Too many RETURNs called",0) ;
	 } ;
/*	Do we have a return result ? */
	if (return_format.all != V3_FORMAT_EOA)
	 { PUSHMP(return_ptr) ; PUSHF(return_format.all) ; } ;
/*	If we are returning from parser then kick out to parse_exit */
	if (is_parse) longjmp(parse_exit,3) ;
/*	Return TRUE unless returning from an interrupt */
	return(old_psi->interrupt_index == 0 ? TRUE : FALSE) ;
}

/*	M O D U L E   I T E M   R O U T I N E S			*/

static struct db__mod_item *free_list = 0 ;	/* Pointer to begin of available item blocks */

/*	xctu_mod_item_alloc - Allocates a new db__mod_item block */

struct db__mod_item *xctu_mod_item_alloc()
{ struct db__mod_item *itemp ;

	if (free_list == NULLV)
	 { return((struct db__mod_item *)v4mm_AllocChunk(sizeof *itemp,TRUE)) ; } ;
/*	Have something already available - return it */
	itemp = free_list ; free_list = free_list->prior ;
	return(itemp) ;
}

/*	xctu_mod_item_delete - Deletes an item from the list */

void xctu_mod_item_delete()
{ struct db__mod_item *itemp ;
   char tmpname[V3_PRS_SYMBOL_MAX+1] ;
   int i ;

/*	Look for item in current psi only */
	POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
	if (tmp_val.fld.type == VFT_BININT)
	 { i = xctu_popint() ;		/* Looking for v3_exit_func */
	   for (itemp=psi->item_list;itemp!=NULL;itemp=itemp->prior)
	    { if (itemp->v3_exit_func == i)
	       { itemp->v3_exit_func = 0 ; itemp->name[0] = 0 ; PUSHTRUE ; return ; } ;
	    } ;
	   PUSHFALSE ; return ;	/* Could not find it */
	 } ;
/*	Here to look for user name */
	stru_popcstr(tmpname) ; stru_csuc(tmpname) ;
	for (itemp=psi->item_list;itemp!=NULL;itemp=itemp->prior)
	 { if (strcmp(itemp->name,tmpname) == 0)
	    { itemp->v3_exit_func = 0 ; itemp->name[0] = 0 ; PUSHTRUE ; return ; } ;
	 } ;
	PUSHFALSE ; return ;
}

/*	xctu_mod_item_exit - Called by "return" on module exit */

xctu_mod_item_exit(psip,modcall_flag)
  struct db__psi *psip ;		/* Pointer to psi which is exitting */
  int modcall_flag ;			/* If TRUE then do NOT call module, but do everything else */
{ struct db__mod_item *itemp,*titemp ;
   struct db__dcl_objref hass ;		/* Temp buffer for module_exit object name */
   struct objwh {
      union val__sob obj ;
      char *ptr ;
    } *objrefp ;
   struct nllist {
      struct nllist *next_link ;			/* If nonzero then link to next one */
    } *list ;
   struct nllist *save_link ;
   struct iou__unit *unitp ;

/*	Loop thru all items for specified psi looking for v3_exit_func's */
	for (itemp=psip->item_list;itemp!=NULL;)
	 { switch (itemp->v3_exit_func)
	    { default: break ;
	      case V3_FLAGS_MOD_NOP:
		break ;
	      case V3_FLAGS_MOD_HEAP_FREE:
		if (!modcall_flag) break ;
		itemp->v3_exit_func = V3_FLAGS_MOD_NOP ;	/* Flag so we only do it once (VEH040120) */
		v4mm_FreeChunk(itemp->ptr) ; break ;
	      case V3_FLAGS_MOD_HEAP_FREE_LIST:
		if (!modcall_flag) break ;
		itemp->v3_exit_func = V3_FLAGS_MOD_NOP ;	/* Flag so we only do it once (VEH040120) */
		list = (struct nllist *)itemp->ptr ;
		for(;list != NULL;)
		 { save_link = list->next_link ; v4mm_FreeChunk(list) ; list = save_link ; } ;
		break ;
	      case V3_FLAGS_MOD_MODULE_EXIT:
		if (modcall_flag) break ;
		itemp->v3_exit_func = V3_FLAGS_MOD_NOP ;	/* Flag so we only do it once (VEH040120) */
/*		Have to find object.module_exit object */
		objrefp = (struct objwh *)itemp->ptr ;
		if (sobu_object_dereference(objrefp->obj.all,"MODULE_EXIT") == NULLV) break ;
		hass.count = 1 ; hass.name_ids[0].all = sobu_name_create("MODULE_EXIT",objrefp->obj.fld.package_id) ;
		sobu_push_val(objrefp->obj.all,0,objrefp->ptr,&hass,FALSE,FALSE) ;
		break ;
	      case V3_FLAGS_MOD_UNIT_CLOSE:
   		unitp = (struct iou__unit *)itemp->ptr ;
		if (unitp->handler_mep != NULL)
		 { iou_close((struct iou__unit *)itemp->ptr,NULLV) ;
		   itemp->v3_exit_func = V3_FLAGS_MOD_NOP ;	/* Flag so we only do it once (VEH040120) */
		   break ;
		 } else
		 { if (!modcall_flag) break ;
		   itemp->v3_exit_func = V3_FLAGS_MOD_NOP ;	/* Flag so we only do it once (VEH040120) */
		   iou_close((struct iou__unit *)itemp->ptr,NULLV) ; break ;
		 } ;
//	      case V3_FLAGS_MOD_HANDLE_DELETE:
//		if (!modcall_flag) break ;
//		itemp->v3_exit_func = V3_FLAGS_MOD_NOP ;	/* Flag so we only do it once (VEH040120) */
//		han_Close((int)itemp->ptr) ; break ;
	      case V3_FLAGS_MOD_WATCHDOG:
		if (!modcall_flag) break ;
		itemp->v3_exit_func = V3_FLAGS_MOD_NOP ;	/* Flag so we only do it once (VEH040120) */
/*		Just cancel the timer entry so we don't keep checking up on it */
#ifdef vms
		sys$cantim((int)itemp->ptr,0) ; break ;
#else
		/* HPXXX */
#endif
	    } ;
/*	   Now get rid of the object */
	   titemp = itemp->prior ; if (modcall_flag) xctu_mod_item_free(itemp) ; itemp = titemp ;
	 } ;
}

/*	xctu_mod_item_find - Finds an item & updates second arg */

void xctu_mod_item_find()
{ struct db__psi *tpsi ;
   struct db__mod_item *itemp ;
   char tmpname[V3_PRS_SYMBOL_MAX+1] ;
   int updformat ; char *updwhere ;

/*	Pop off context: 0 or starting location for search */
	itemp = (struct db__mod_item *)xctu_popptr() ;
	if (itemp == NULLV) { itemp = psi->item_list ; tpsi = psi ; }
	 else { tpsi = itemp->owner_psi ; itemp = itemp->prior ; } ;
/*	Now pop off and save argument to be updated */
	POPF(updformat) ; POPVP(updwhere,(char *)) ;
/*	Now get the name of the item to be found */
	stru_popcstr(tmpname) ; stru_csuc(tmpname) ;
	for (;;)
	 { if (itemp == NULLV)
	    { tpsi = tpsi->prior_psi_ptr ;	/* Go back to prior */
	      if (tpsi == NULLV) { PUSHFALSE ; return ; } ;
	      itemp = tpsi->item_list ; continue ;
	    } ;
/*	   Have an item to check - check the name */
	   if (strcmp(tmpname,itemp->name) != 0)
	    { itemp = itemp->prior ; continue ; } ;
/*	   Got a match - update argument & return TRUE */
	   PUSHMP(updwhere) ; PUSHF(updformat) ; PUSHMP(itemp->ptr) ; PUSHF(itemp->format.all) ;
	   v3_operations(V3_XCT_UPDATE+1) ; POPFV ;
	   PUSHMP(itemp) ; PUSHF(V3_FORMAT_POINTER) ; return ;
	 } ;
}

/*	xctu_mod_item_fork - Handles /fork_leave/ & /fork_return/ functions */

void xctu_mod_item_fork(lr_flag)
  int lr_flag ;				/* If TRUE then /leave/ otherwise /return/ */
{ struct db__psi *tpsi ;
   struct db__mod_item *itemp ;
   struct db__dcl_objref hass ;		/* Temp name list */
   struct objwh {
      union val__sob obj ;
      char *ptr ;
    } *objrefp ;

/*	Scan thru all psis & psi items looking for the right stuff */
	for (tpsi=psi;tpsi!=NULL;tpsi=tpsi->prior_psi_ptr)
	 { for (itemp=tpsi->item_list;itemp!=NULL;itemp=itemp->prior)
	    { switch (itemp->v3_exit_func)
	       { default: break ;
		 case V3_FLAGS_MOD_FORK_LEAVE:
			if (!lr_flag) break ;
			objrefp = (struct objwh *)itemp->ptr ;
			if (sobu_object_dereference(objrefp->obj.all,"FORK_LEAVE") == NULLV) break ;
			hass.count = 1 ;
			hass.name_ids[0].all = sobu_name_create("FORK_LEAVE",objrefp->obj.fld.package_id) ;
			sobu_push_val(objrefp->obj.all,0,objrefp->ptr,&hass,FALSE,FALSE) ;
			break ;
		 case V3_FLAGS_MOD_FORK_REENTER:
			if (lr_flag) break ;
			objrefp = (struct objwh *)itemp->ptr ;
			if (sobu_object_dereference(objrefp->obj.all,"FORK_REENTER") == NULLV) break ;
			hass.count = 1 ;
			hass.name_ids[0].all = sobu_name_create("FORK_REENTER",objrefp->obj.fld.package_id) ;
			sobu_push_val(objrefp->obj.all,0,objrefp->ptr,&hass,FALSE,FALSE) ;
			break ;
	       } ;
	    } ;
	 } ;
	return ;
}

/*	xctu_mod_item_free - Frees up an item block		*/

void xctu_mod_item_free(itemp_to_free)
  struct db__mod_item *itemp_to_free ;
{
/*	Just link up to free chain */
	itemp_to_free->prior = free_list ;
	free_list = itemp_to_free ;
	return ;
}

/*	xctu_mod_item_push - Pushes a new item onto psi list	*/

void xctu_mod_item_push()
{ struct db__psi *tpsi ;
   struct db__mod_item *itemp ;
   union val__format vf ;
   struct db__formatptr *fw ;
   int i,*base_ptr ; int *(*mp_ptr) ;

/*	Pop off context: 0 for current psi, -n for prior, +n for explicit pointer */
	POPF(vf.all) ; PUSHF(vf.all) ;
	if (vf.fld.type == VFT_POINTER) { tpsi = (struct db__psi *)xctu_popptr() ; }
	 else { i = xctu_popint() ;
		if (i == 0) { tpsi = psi ; }
		 else { if (i > 0) { v3_error(V3E_NOTPSI,"MOD","ITEM_PUSH","NOTPSI","Third arg must be PSI, 0, or -n",(void *)i) ; }
			 else { for (tpsi=psi;i<0;i++) tpsi = tpsi->prior_psi_ptr ; } ;
		      } ;
	      } ;
/*	Allocate a new item block & fill in details */
	itemp = (struct db__mod_item *)xctu_mod_item_alloc() ;
	itemp->owner_psi = tpsi ; itemp->prior = tpsi->item_list ; tpsi->item_list = itemp ;
	POPF(itemp->format.all) ;
	if (itemp->format.fld.type == VFT_INDIRECT)
	 { POPVP(fw,(struct db__formatptr *)) ; itemp->format.all = fw->format.all ; PUSHMP(fw->ptr) ; } ;
	if (itemp->format.fld.type == VFT_STRUCTPTR)
	 { itemp->format.fld.type = VFT_FIXSTR ;
	   POPVP(mp_ptr,(int *(*))) ; itemp->ptr = (char *)*mp_ptr ;
	 } else if (itemp->format.fld.type == VFT_POINTER) { PUSHF(itemp->format.all) ; itemp->ptr = xctu_popptr() ; }
	 	 else { POPVP(itemp->ptr,(char *)) ; } ;
/*	See if name is string or number (v3_exit_func) */
	POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
	if (tmp_val.fld.type == VFT_BININT)
	 { itemp->v3_exit_func = xctu_popint() ; itemp->name[0] = 0 ; }
	 else { stru_popcstr(itemp->name) ; stru_csuc(itemp->name) ; itemp->v3_exit_func = 0 ; } ;
	return ;
}

/*	xctu_mod_watchdog - Handles mod_watchdog() module call	*/

void xctu_mod_watchdog()
{ struct db__mod_item *itemp ;
   int i ;
   int intu_mod_watchdog() ;

	i = xctu_popint() ;	/* Get max number of seconds */
	/* HPXXX */
}

/*	C O M M A N D   U T I L I T I E S			*/

/*	xctu_cmd_str_convert - Converts string of form:
		"command,command,..."
	 to db__command_list format				*/

void xctu_cmd_str_convert(command_str,list_ptr)
  char *command_str ;			/* Command string */
  struct db__command_search_list *list_ptr ;	/* Updated with results */

{ char *str_ptr ; unsigned short cx ;

/*	Convert command_str to upper case */
	str_ptr = command_str ;
	while (*str_ptr)
	{ if (*str_ptr >= 'a') { *str_ptr = (*str_ptr)-32 ; }
	   else if (*str_ptr == '-') { *str_ptr = '_' ; } ;
	  str_ptr++ ;
/*	  *str_ptr = (*str_ptr >= 'a' ? (*str_ptr)-32 : (*str_ptr == '-' ? '_' : *str_ptr)) ; str_ptr++ ; */
	} ;
/*	Strip out each command, ignore spaces/tabs...	*/
	str_ptr = command_str ; list_ptr->count = 0 ; list_ptr->repeatable = TRUE ;
	do
	 { str_ptr += strspn(str_ptr,"/,; 	") ;
/*	   If command name begins with "*" then disable "repeatability" */
	   if (*str_ptr == '*') { str_ptr++ ; list_ptr->repeatable = FALSE ; } ;
	   if ((cx = strcspn(str_ptr,"	 ;,/")) == 0) break ;
/*	   Have something, length is cx */
	   if (list_ptr->count >= V3_COMMAND_SEARCH_LIST_MAX)
	    v3_error(V3E_CMDMAX,"XCT","CMD_STR_CONVERT","CMDMAX","Too many commands in list",(void *)V3_COMMAND_SEARCH_LIST_MAX) ;
	   strncpy(list_ptr->com[list_ptr->count].name,str_ptr,cx) ;
	   list_ptr->com[list_ptr->count++].name[cx] = NULLV ;
	   list_ptr->com[list_ptr->count - 1].hash = prsu_str_hash(str_ptr,cx) ;
	   str_ptr += cx ;
	 } while (strlen(str_ptr) > 0) ;
/*	All done, list_ptr has been updated */
	return ;
}

/*	xctu_cmd_find - Returns TRUE if one of the command(s) in arg1 are found */
/*	 Second arg is updated with info needed for xctu_cmd_invoke */

int xctu_cmd_find(command_list_ptr,command_info)
  struct db__command_search_list *command_list_ptr ;
  struct db__xct_cmd_info *command_info ;	/* Updated with info if command is found */
{ struct db__psi *tpsi ;
   struct db__command_list *cmdp ;
   struct db__level_ids *lvlp ;
   struct
    { short count ;
      struct { struct db__module_entry *mep ; char command_index ; } cmds[20] ;
    } acl ;				/* Active command list (to avoid nesting) */
   int tpsi_offset ; int i,j,k ; char *t1,*t2 ;

 /*	Search back thru psi nesting for all module commands */
	tpsi = psi ; acl.count = 0 ;
	while (tpsi != NULLV)
	 { if (tpsi->mep == NULLV) goto skip_psi ;		/* Skip over interpretive psi's */
/*	   If this psi is a command invocation then remember so that we don't call it again */
	   if (command_list_ptr->repeatable ? FALSE : (tpsi->command_index != 0))
	    { acl.cmds[acl.count].mep = tpsi->mep ; acl.cmds[acl.count].command_index = tpsi->command_index ; acl.count++ ;
	    } ;
	   if (tpsi->command_psi != NULLV) goto skip_psi ;	/* Don't want to trap within a trap ! */
	   if (tpsi->mep->command_offset == 0) goto skip_psi ;
	   cmdp = (struct db__command_list *)((char *)tpsi->mep + tpsi->mep->command_offset) ;
	   lvlp = (struct db__level_ids *)((char *)tpsi->mep + tpsi->mep->level_id_offset) ;
/*	   Figure out offset from begin of package for this level's psi code pointer */
	   if (tpsi->pic_base != NULLV) { tpsi_offset = tpsi->code_ptr - tpsi->pic_base ; }
	    else { tpsi_offset = tpsi->code_ptr - process->package_ptrs[tpsi->package_index]->code_ptr ; } ;
/*	   Search this module's commands against those in the argument command list */
	   for (i=0;i<command_list_ptr->count;i++)
	    { for (j=0;j<cmdp->count;j++)
	       { BL1: if (command_list_ptr->com[i].hash != cmdp->com[j].hash) continue ;
		 BL2: if (strcmp(command_list_ptr->com[i].name,cmdp->com[j].name) != 0) continue ;
/*		 Name matches - see if command is active via scope of level */
		 BL3: if (tpsi_offset >= cmdp->com[j].begin_code_offset &&
		     tpsi_offset <= cmdp->com[j].end_code_offset)
		      { /*  Looks like this command is valid, make sure we have not already invoked it */
			    for(k=0;k<acl.count;k++)
			     { if (acl.cmds[k].mep == tpsi->mep && acl.cmds[k].command_index == j+1)
				goto not_this_one ;
			     } ;
			    goto found_command ;
		      } ;
		  not_this_one: ;
	       } ;
	    } ;
/*	   Command not matched at this psi - try prior */
skip_psi:
	   tpsi = tpsi->prior_psi_ptr ;
	 } ;
/*	If here then no match was found - return NULLV */
	return(NULLV) ;
/*	Found a match - update second arg with info */
found_command:
	command_info->command_psi = tpsi ; command_info->command_index = j+1 ;
	command_info->cmd_code_offset = cmdp->com[j].cmd_code_offset ;
	return(TRUE) ;
}

/*	xctu_cmd_invoke - Actually invokes a command as a funny kind of module call */

struct db__psi *xctu_cmd_invoke(command_info,listp)
  struct db__xct_cmd_info *command_info ; /* Points to command info */
  struct cmd__args *listp ;		/* NULLV or pointer to temp arg list */
{ union val__format mod_ref ;
   struct db__module_entry *mentry ;
   struct db__psi *new_psi ;
   V3STACK *sp ;
   int i,sb,*lptr ;

/*	Push eoa */
	sp = psi->stack_ptr ;
	PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
/*	Now push any arguments passed in temp list */
	if (listp != NULLV)
	 { for (i=listp->count-1;i>=0;i--) { PUSHVP(listp->arglist[i].ptr) ; PUSHF(listp->arglist[i].format.all) ; } ;
	 } ;
	new_psi = (struct db__psi *)((char *)psi->stack_ptr - (((sizeof *psi) + ALIGN_DOUBLE) & ~ALIGN_DOUBLE) ) ;
	psi->stack_ptr = sp ;			/* Restore stack */
	mentry = command_info->command_psi->mep ;
	new_psi->arg_ptr = psi->stack_ptr-(2*SIZEOFSTACKENTRY) ;
	new_psi->prior_psi_ptr = psi ;	/* Link old to new */
	new_psi->package_index = mentry->package_id ;
	new_psi->pure_base = (int *)((char *)mentry + mentry->pure_offset) ;
	new_psi->mep = mentry ;	/* Save for possible command lookup */
	new_psi->item_list = NULLV ;
	new_psi->interrupt_index = 0 ;		/* Not in an interrupt */
	new_psi->line_number = 0 ;
/*	Adjust stack pointer for module stack storage */
	lptr = (int *)(psi = new_psi) ;
/*	NEW_PSI => PSI below !! */
	sb = (mentry->stack_bytes < 0 ? -mentry->stack_bytes : mentry->stack_bytes) ;
	psi->reset_stack_ptr = (psi->stack_ptr = (V3STACK *)((char *)lptr - ((sb+ALIGN_MAX) & ~ALIGN_MAX) )) ;
	psi->stack_space = (char *)psi->stack_ptr ;
/*	Now add some info before returning so we know it is a command */
	psi->command_psi = command_info->command_psi ;
	psi->pic_base = psi->command_psi->pic_base ;
	psi->command_arg_ptr = psi->arg_ptr ; psi->command_arg_cnt = (listp == NULLV ? 0 : listp->count) ;
	psi->code_ptr = psi->pic_base + command_info->cmd_code_offset ;
	psi->command_index = command_info->command_index ;
	psi->mre = command_info->command_psi->mre ;
	psi->impurelm_base = psi->mre->impurelm_base ;
/*	Now fudge some stuff so local variables & arguments will work as if in prior context */
	psi->arg_ptr = psi->command_psi->arg_ptr ; psi->arg_cnt = psi->command_psi->arg_cnt ;
	psi->stack_space = psi->command_psi->stack_space ;

	return(psi) ;
}

/*	U T I L I T I E S			*/

/*	xctu_popnum - Pops off number and stores in standard form */

void xctu_popnum(nump)
  struct num__ref *nump ;		/* Pointer to standard form */
{ short int *wordp ;
  int *intp ;

	POPF(nump->format.all) ;		/* Save format */
	if (nump->format.fld.mode == VFM_IMMEDIATE)
	 { POPVI(nump->value) ; nump->ptr = &nump->value ; return ; } ;
	if (nump->format.fld.type == VFT_BININT)
	 { POPVP(intp,(int *)) ; nump->dbl_ptr = (double *)intp ;
	   nump->value = *intp ; nump->ptr = (LONGINT *)intp ;
	   return ;
	 } ;
	if (nump->format.fld.type == VFT_BINWORD)
	 { POPVP(wordp,(short int *)) ; nump->dbl_ptr = (double *)wordp ;
	   nump->value = *wordp ; nump->ptr = (LONGINT *)wordp ;
	   return ;
	 } ;
	POPVP(nump->ptr,(LONGINT *)) ; nump->dbl_ptr = (double *)nump->ptr ;
	return ;
}

/*	xctu_poppd - Pops off value and converts to standard packed decimal */

#ifdef SUPPORT_PD
xctu_poppd(pdref)
  struct pd__ref *pdref ;
{ struct num__ref anum ;

	xctu_popnum(&anum) ;	/* Get the number */
	pdref->format = anum.format ;
	switch (anum.format.fld.type)
	 { case VFT_BINWORD:
	   case VFT_BININT:
		mthu_cvtip(*anum.ptr,0,&pdref->str_buf,10,0) ;
		pdref->format.fld.length = 10 ; pdref->ptr = (int *)pdref->str_buf ; return ;
	   case VFT_PACDEC:
		pdref->ptr = anum.ptr ; return ;
	   case VFT_STRINT:
		mthu_cvtsp(anum.ptr,anum.format.fld.length,0,
			   &pdref->str_buf,anum.format.fld.length/2+1,0) ;
		pdref->format.fld.length = anum.format.fld.length/2 + 1 ; pdref->ptr = (int *)pdref->str_buf ;
		return ;
	 } ;
}
#endif

/*	xctu_popptr - Pops POINTER off of stack & returns (char *)	*/

char *xctu_popptr()
{ union val__format vf ;
  char *(*ptr) ; int i ;

	POPF(vf.all) ;
	if (vf.fld.type != VFT_POINTER)
	 { if (vf.all == V3_FORMAT_NULL) { POPVI(i) ; return(NULLV) ; } ;		/* Not a POINTER - allow NULL */
	   if (vf.fld.type == VFT_BININT || vf.fld.type == VFT_BINWORD || vf.fld.type == VFT_BINLONG)
	    { PUSHF(vf.all) ; if (xctu_popint() == 0) return(NULLV) ;			/* Also allow 0 as null pointer */
	    } ;
	  v3_error(V3E_NOTPTR,"XCT","POPPTR","NOTPTR","Value not a POINTER",(void *)vf.fld.type) ;
	 } ;
	POPVP(ptr,(char *(*))) ;
	if (vf.fld.mode == VFM_IMMEDIATE) return((char *)ptr) ;
	return(*ptr) ;
}

/*	xctu_pointer - Converts argument to real pointer */

char *xctu_pointer(refall)
  int refall ;
{ struct db__package *ptr ;
  union val__mempos ref ;
   struct db__psi *xpsi ;
   int i ;

	ref.all = refall ;
	switch (ref.fld.value_type)
	 {
	   case 0:
		return(NULLV) ;
	   case VAL_STATOB:
		return((char *)&process->package_ptrs[ref.statpure.package_id]->ob_bucket_ptr->buf[ref.statpure.offset]) ;
	   case VAL_STATIMPLM:
		ptr = process->package_ptrs[ref.statimp.package_id] ;
		return((char *)(psi->impurelm_base+ref.statimp.offset)) ;
	   case VAL_STATIMP:
		ptr = process->package_ptrs[ref.statimp.package_id] ;
		return((char *)((ptr->impure_ptr)+ref.statimp.offset)) ;
	   case VAL_STATPURE:
		if (psi->pure_base != NULLV) return((char *)psi->pure_base + ref.statpure.offset) ;
		ptr = process->package_ptrs[ref.statpure.package_id] ;
		return((char *)((ptr->pure_ptr)+ref.statpure.offset)) ;
	   case VAL_STACK:
		return((char *)(psi->stack_space + ref.stack.offset)) ;
	   case VAL_ARG:
		return((char *)(psi->arg_ptr - ref.arg.offset)) ;
	   case VAL_STRUCT:
		if (parse != NULLV) return((char *)&parse->struct_buf[ref.statpure.offset]) ;
		v3_error(V3E_NOSTRUCT,"XCT","POPPTR","NOSTRUCT","Do not have pointer to structure buffer???",0) ;
	   case VAL_XARG:
	   case VAL_XSTACK:
		xpsi = psi ;	/* Start looping back for correct context */
		for (i=0;;)
		 { if (xpsi->mep == NULLV ? FALSE : xpsi->mep->module_id == ref.xstack.module_id)
		    { if (++i > ref.xstack.frames) break ; } ;
		   if ((xpsi = xpsi->prior_psi_ptr) != NULLV) continue ;
		   v3_error(V3E_NOMODINSTACK,"XCT","POPPTR","NOMODINSTACK","Could not find module in current frame stack",0) ;
		 } ;
/*		Got proper psi - convert to address */
		if (ref.fld.value_type == VAL_XARG) return((char *)(xpsi->arg_ptr - ref.xarg.offset)) ;
		return((char *)(xpsi->stack_space + ref.xstack.offset)) ;
/*	   xxx - need some extra code here */
	 } ;
/*	If get here then bad value type - generate error	*/
	v3_error(V3E_BADLOCREF,"XCT","POINTER","BADLOCREF","Invalid location reference",(void *)ref.fld.value_type) ;
	return(NULL) ;
}

/*	xctu_num_upd - Updates numeric variable with integer value */

static int pwrs[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 } ;

void xctu_num_upd(int_val,decimals,nump)
  B64INT int_val ;			/* Integer value */
  int decimals ;			/* Number of decimal places */
  struct num__ref *nump ;		/* Points to the numeric var */
{ short int *word_ptr ;
  int *int_ptr ;
  B64INT *long_ptr ;
   union val__v3num *v3n ;

	switch(nump->format.fld.type)
	 { default: v3_error(V3E_INVNUMVAR,"XCT","NUMUPD","INVNUMVAR","Cannot update variable with numeric result",0) ;
	   case VFT_POINTER:
		*nump->ptr = int_val ; break ;
	   case VFT_BININT:
		int_ptr = (int *)nump->dbl_ptr ;
		*int_ptr = mthu_cvtii(int_val,decimals,nump->format.fld.decimals) ; break ;
	   case VFT_BINLONG:
		long_ptr = (B64INT *)nump->dbl_ptr ;
		*long_ptr = mthu_cvtii(int_val,decimals,nump->format.fld.decimals) ; break ;
	   case VFT_BINWORD:
		word_ptr = (short int *)nump->dbl_ptr ;
		*word_ptr = mthu_cvtii(int_val,decimals,nump->format.fld.decimals) ; break ;
	   case VFT_FLOAT:
		mthu_cvtid(int_val,decimals,nump->dbl_ptr) ; break ;
	   case VFT_PACDEC:
		mthu_cvtip(int_val,decimals,nump->ptr,nump->format.fld.length,nump->format.fld.decimals) ; break ;
	   case VFT_STRINT:
		mthu_cvtis(int_val,decimals,(char *)nump->ptr,nump->format.fld.length,nump->format.fld.decimals) ; break ;
	   case VFT_V3NUM:
		v3n = (union val__v3num *)nump->ptr ; v3n->fd.big = 0 ;
		if (decimals == 0) { v3n->fd.dps = 0 ; v3n->fd.num = int_val ; }
		 else { v3n->fd.dps = (int_val%pwrs[decimals])*pwrs[9-decimals] ; v3n->fd.num = int_val/pwrs[decimals] ; } ;
	 } ;
	return ;
}

/*	V 3   E R R O R   H A N D L E R			*/

/*	v3_error - Main error handler			*/

void v3_error(errnum,subsys,module,code,msg,param)
  int errnum ;			/* V3 error number */
  char *subsys ;		/* V3 subsystem generating (ex: PCK) */
  char *module ;		/* V3 module (ex: SAVE) */
  char *code ;			/* Error code (ex: CREATEXCT) */
  char *msg ;
  void *param ;			/* Variable parameter (may be string, int, whatever) */
{ static struct db__error err ;	/* To be filled with error info */
   struct db__command_search_list cmd_list ;
   struct db__xct_cmd_info cmd_info ;
   struct cmd__args err_arglist ;
   struct db__parse_info *prsp ;
   char *ioname ;		/* NOTE: BAD CODE HERE */
   char cmds[150] ;		/* Temp string buffer */
   int i ;

/*	Do special check to see if "XV3 xxx" startup failed */
	if (psi->package_index != V3_PICMOD_PACKAGE_ID)
	 { if (process->package_ptrs[psi->package_index] == NULLV)
	    { printf("\n%V3-F-NOFILE, %s\n",msg) ; psi = 0 ; exit(44) ; } ;
	 } ;
/*`	Perform normal error routine - First fill err structure */
	strcpy(err.subsystem,subsys) ; strcpy(err.v3_module,module) ; strcpy(err.code,code) ;
	strcpy(err.message,msg) ; err.parameter = (int)param ; err.v3_opcode = process->last_v3_opcode ;
	err.errnum = errnum ; process->last_errnum = errnum ;
	if (psi->package_index == V3_PICMOD_PACKAGE_ID) { strcpy(err.package_name,"V3PICMOD") ; }
	 else { strcpy(err.package_name,process->package_ptrs[psi->package_index]->package_name) ; } ;
	if (psi->mep == NULLV) { strcpy(err.module_name,"*interactive*") ; }
	 else strcpy(err.module_name,psi->mre->name) ;
/*	See if we can find a source file line number */
	if (psi->package_index == V3_PICMOD_PACKAGE_ID ? FALSE : process->package_ptrs[psi->package_index] == NULLV) return ;
	err.line_number = psi->line_number ;
/*	Now push error message & pointer onto error arglist */
	err_arglist.count = 2 ;
	err_arglist.arglist[1].format.all = V3_FORMAT_VARSTR ; err_arglist.arglist[1].ptr = err.message ;
	err_arglist.arglist[0].format.all = V3_FORMAT_POINTER ; err_arglist.arglist[0].ptr = (char *)&err ;
/*	See if there might be an error interrupt to invoke */
	if (process->interrupts[V3_FLAGS_INT_ERROR].status == INTERRUPT_ENABLED)
	 intu_call_interrupt(V3_FLAGS_INT_ERROR,&err_arglist) ;
/*	Fine, now create one or more commands to try to execute */
	if (strcmp(subsys,"IO") == 0)
	 { ioname = (char *)param ;
	   strcpy(cmds,ioname) ; strcat(cmds,"_ERROR,") ; strcat(cmds,ioname) ; strcat(cmds,"_") ; strcat(cmds,code) ;
	   strcat(cmds,",") ; strcat(cmds,ioname) ; strcat(cmds,"_") ; strcat(cmds,module) ;
	   xctu_cmd_str_convert(cmds,&cmd_list) ; cmd_list.repeatable = FALSE ;
	   if (xctu_cmd_find(&cmd_list,&cmd_info))
	    { xctu_cmd_invoke(&cmd_info,&err_arglist) ; longjmp(xct_continue,1) ; } ;
	 } ;
	strcpy(cmds,subsys) ; strcat(cmds,"_ERROR,") ; strcat(cmds,subsys) ; strcat(cmds,"_") ; strcat(cmds,code) ;
	strcat(cmds,",") ; strcat(cmds,subsys) ; strcat(cmds,"_") ; strcat(cmds,module) ;
	xctu_cmd_str_convert(cmds,&cmd_list) ; cmd_list.repeatable = FALSE ;
	if (xctu_cmd_find(&cmd_list,&cmd_info))
	 { xctu_cmd_invoke(&cmd_info,&err_arglist) ; longjmp(xct_continue,1) ; } ;
/*	User not read to handle errors - Try for "V3_ERROR" */
	xctu_cmd_str_convert("V3_ERROR",&cmd_list) ; cmd_list.repeatable = FALSE ;
	if (xctu_cmd_find(&cmd_list,&cmd_info))
	 { xctu_cmd_invoke(&cmd_info,&err_arglist) ; longjmp(xct_continue,1) ; } ;
/*	Boy, he's making it difficult - print error message  */
	printf("%%V3-%s-%s, %d: %s (%d/%d)\n - Called from %s within %s.%s.%d\n",
	 err.subsystem,err.code,err.errnum,err.message,(int)param,process->last_v3_opcode,err.v3_module,err.package_name,
	 err.module_name,err.line_number) ;
/*	See if we can print parse input location */
	if (psi->package_index == V3_PICMOD_PACKAGE_ID ? FALSE :
					(prsp = process->package_ptrs[psi->package_index]->parse_ptr) != NULLV)
	 { if (prsp->ilvl[prsp->ilx].file != NULLV)
	    printf(" - input line #%d %s\n",prsp->ilvl[prsp->ilx].current_line,prsp->ilvl[prsp->ilx].input_str) ;
	 } ;
	xctu_cmd_str_convert("V3_UNTRAPPED_ERROR",&cmd_list) ; cmd_list.repeatable = FALSE ;
	if (xctu_cmd_find(&cmd_list,&cmd_info))
	 { xctu_cmd_invoke(&cmd_info,&err_arglist) ; longjmp(xct_continue,1) ; } ;
/*	That's it - do recovery trapping */
	longjmp(process->v3_error_jmp,1) ;
}

/*	Here on V3 Exit		*/

#ifdef V3PCKSHRSEG
#ifdef UNIX
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#endif

void sigint_exit_handler(arg)		/* Here on SIGINT */
  int arg ;
{ void exit_handler() ;

	process = process_init_new() ;
	exit_handler() ;		/* Call standard exit handler */
	exit(process == NULL ? EXIT_SUCCESS : (process->exit_value=EXIT_SUCCESS)) ;			/*  and quit */
}

void exit_handler()
{ 
  static int count = 0 ;
#ifdef ISOCKETS
#ifdef WINNT
  int wx,wres ;
#endif
#endif
#ifdef V3PCKSHRSEG
#ifdef UNIX
   struct shmid_ds buf ;
#endif
   struct db__package *pckp ;
   int i ;
#endif
   struct db__psi *tpsi ;
   char package[64],module[64] ; int cnt ;

	count++ ; if (count > 1) return ;	/* Only want to do this once! */
	signal(SIGILL,SIG_DFL) ;		/* Turn off any trapping */
#ifndef WINNT
	signal(SIGQUIT,SIG_DFL) ; signal(SIGBUS,SIG_DFL) ;
#endif
	signal(SIGFPE,SIG_DFL) ; signal(SIGSEGV,SIG_DFL) ;
	signal(SIGINT,SIG_DFL) ; signal(SIGTERM,SIG_DFL) ;
	process = process_init_new() ;
	if (process == NULL) return ;	/* Never got started! */
	iou_close_all() ;		/* Close all open units */
#ifdef WINNT
	for(wx=0;wx<10;wx++)
	 { WSACleanup() ; wres = WSAGetLastError() ;
	   if (wres == WSANOTINITIALISED) break ;			/* Cleanup until all clean! */
	   if (wres == WSAEINPROGRESS) WSACancelBlockingCall() ;
	 } ;
#endif
#ifdef V3PCKSHRSEG
/*	Maybe go thru all packages & release sharable segments */
	for(i=0;i<V3_PACKAGE_MAX;i++)
	 { if ((pckp = process->package_ptrs[i]) == 0) continue ;	/* No package in this slot */
	   if (pckp->shmid == 0) continue ;				/* No sharable segment */
relseg:
#ifdef UNIX
	   if (shmdt(pckp->shmaddr) == -1) printf("? Err=%s detaching %d @%p\n",v_OSErrString(errno),pckp->shmid,pckp->shmaddr) ;
	   if (shmctl(pckp->shmid,IPC_STAT,&buf) == -1)		/* Can't get info ? */
	    { printf("? Err=%s obtaining info for seg %d\n",v_OSErrString(errno),pckp->shmid) ; continue ; } ;
	   if (buf.shm_nattch <= 0)
	    { if ((int)shmctl(pckp->shmid,IPC_RMID,&buf) == 0) { /* printf("[Removed %d]\n",pckp->shmid) */ ; }
	       else { /* printf("? Err=%d removing %d\n",errno,pckp->shmid) ; */ } ;
	    } ;
#endif
#ifdef WINNT
	   UnmapViewOfFile(pckp->shmaddr) ; CloseHandle(pckp->shmid) ;
#endif
	   process->package_ptrs[i] = NULLV ;
	 } ;
#endif
/*	Did we get a normal exit ? */
	if (process->exit_value & 1) return ;
/*	Don't try to print out ^P info if we never got started */
	if (psi == 0) return ;
	printf("\n%%V3-F-EXIT, Abnormal image termination of V3 (Ctrl/P to follow)\n") ;

/*	No - then try to call Control/P to see where we were on abort */
	ctrlp(FALSE) ;	/* If not called via sys_exit(1) then call control-P handler */
	printf("\nV3-F-Exit, Call Stack at time of termination\n") ;
	for (tpsi=psi,cnt=10;tpsi!=NULLV&&cnt>0;(tpsi=tpsi->prior_psi_ptr,cnt--))
	 { if (process->package_ptrs[tpsi->package_index] == NULLV) { sprintf(package,"PSLOT(%d)",tpsi->package_index) ; }
	    else strcpy(package,process->package_ptrs[tpsi->package_index]->package_name) ;
	   if (tpsi->mep == NULLV) { strcpy(module,"*interactive*") ; }
	    else { strcpy(module,tpsi->mre->name) ; } ;
/*	   Print out the location */
	   printf("   %s.%s.%d\015\012",package,module,tpsi->line_number) ;
	 } ;
	return ;
}

/*	Here on ^P from VMS Systems		*/

#ifdef VMSOS
void vms_ctrlp()
{
	ctrlp(TRUE) ;
}
#endif

/*	Here on ^P from Unix Systems		*/

void unix_ctrlp()
{ void unix_ctrlp() ;

#ifdef UNIX
	ctrlp_interrupt = TRUE ;
	signal(SIGTSTP,unix_ctrlp) ;	/* Reset the signal */
	ioctl(0,TCFLSH,2) ;		/* Flush terminal I/O buffers */
#endif
	ctrlp(TRUE) ;
}

/*	Here on ^P interrupt */
void ctrlp(showstack)
  int showstack ;			/* TRUE to calculate current stack size */
{ int i,size ;
   struct db__psi *tpsi ;
   struct db__parse_info *prsp ;
#ifdef UNIX
   struct tms tmsp ;
#endif
   struct tm *tmp ;
   time_t current_time ;
#ifdef VMSOS
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc ;
#endif
   char buf[200] ; int delta,delta_cpu ;
   char package[50],module[50] ;

	process = process_init_new() ;
/*	First check to see if we should interrupt on ^P */
	switch (process->interrupts[V3_FLAGS_INT_CTRLP].status)
	 { case INTERRUPT_ENABLED:
		intu_call_interrupt(V3_FLAGS_INT_CTRLP,NULLV) ; return ;
	   case INTERRUPT_IGNORE:
		return ;
	   case INTERRUPT_DISABLED:
		break ;
	 } ;
/*	See if we can find a source file line number */
	if (process->package_ptrs[psi->package_index] == NULLV)
	 { sprintf(package,"PSLOT(%d)",psi->package_index) ; }
	 else { strcpy(package,process->package_ptrs[psi->package_index]->package_name) ; } ;
/*	Add up total Stack Bytes used so far */
	for((size=0,tpsi=psi);showstack && tpsi!=0;tpsi=tpsi->prior_psi_ptr)
	 { if (tpsi->mep != NULLV)
	    size += (tpsi->mep->stack_bytes >= 0
			? tpsi->mep->stack_bytes : -tpsi->mep->stack_bytes) ;
	} ;
	if (psi->mep == NULLV) { strcpy(module,"*interactive*") ; }
	 else { strcpy(module,psi->mre->name) ; } ;
	sprintf(buf,"V3 - %s.%s.%d (op=%d,stk=%d)",package,module,psi->line_number,process->last_v3_opcode,size) ;
#ifdef UNIX
	current_time = time((time_t)NULLV) ; tmp = localtime(&current_time) ; times(&tmsp) ;
	delta = current_time - process->delta_time ; process->delta_time = current_time ;
	delta_cpu = (tmsp.tms_utime+tmsp.tms_stime+tmsp.tms_cutime+tmsp.tms_cstime)
			-(process->delta_tms.tms_utime+process->delta_tms.tms_stime+process->delta_tms.tms_cutime+process->delta_tms.tms_cstime) ;
#define T(TM) ((TM*10)/CLOCKS_PER_SEC)/10,((TM*10)/CLOCKS_PER_SEC)%10
	printf("\n%s\n\r     (Time=%d:%02d:%02d, Delta=%02d:%02d, CPU=%d.%d %d.%d %d.%d, Delta=%d.%d)\n\r",
		buf,tmp->tm_hour,tmp->tm_min,tmp->tm_sec,delta/60,delta%60,
		T(tmsp.tms_utime),T(tmsp.tms_stime),T(tmsp.tms_cutime+tmsp.tms_cstime),T(delta_cpu)) ;
	process->delta_tms = tmsp ;
#endif
#ifdef VMSOS
	strdsc.desc = 0 ; strdsc.pointer = &buf ;
	strdsc.length = strlen(buf) ; lib$put_output(&strdsc) ;
#endif
#ifdef WINNT
	printf("\n%s\n",buf) ;
#endif
/*	See if we can print parse input location */
	if (process->package_ptrs[psi->package_index] != NULLV)
	 { if ((prsp = process->package_ptrs[psi->package_index]->parse_ptr) != NULLV)
	    { if (prsp->ilvl[prsp->ilx].file != NULLV)
	       { sprintf(buf,"%s, page %d, module %s + %d -",
		     prsp->ilvl[prsp->ilx].file_name,prsp->ilvl[prsp->ilx].file_page,prsp->module_name,
		     prsp->ilvl[prsp->ilx].current_line) ;
#ifdef VMSOS
	         strdsc.length = strlen(buf) ; lib$put_output(&strdsc) ;
	         sprintf(buf,"%5d	%s",prsp->ilvl[prsp->ilx].total_lines,prsp->ilvl[prsp->ilx].input_str) ;
	         strdsc.length = strlen(buf) ; lib$put_output(&strdsc) ;
#else
		 printf(buf,"%5d	%s\n",prsp->ilvl[prsp->ilx].total_lines,prsp->ilvl[prsp->ilx].input_str) ;
#endif
	       } ;
	    } ;
	 } ;
/*	Call on iou_ctrlp to print out any open V3 files */
	iou_ctrlp(0) ;
	printf("\n") ;
}

/*	v3_signal_quit - Called to do a control-P */

void v3_signal_quit(arg)
  int arg ;
{
#ifdef POSIX
	signal(SIGQUIT,v3_signal_quit) ;
#endif
	ctrlp(FALSE) ;
}


/*	S I G N A L   U T I L I T I E S					*/

/*	v3_signal_fpe - Handles Arithmetic Traps			*/

#ifndef SCPCVAL
#define SCPCVAL scp->sc_pc
#define SCPCZAP scp->sc_pc = NULL
#endif

#ifdef ALPHAOSF
void v3_signal_fpe(sig,code,scp)
  int sig,code ;
  struct sigcontext *scp ;
#else
void v3_signal_fpe(code)
  int code ;
#endif
{ void v3_signal_fpe() ;
#ifdef ALPHAOSF
  struct sigcontext scpx ;
#endif
  char errbuf[250] ;

	signal(SIGFPE,v3_signal_fpe) ;	/* Reset signal */
#ifdef ALPHAOSF
	if (scp == NULLV) { scp = &scpx ; SCPCZAP ; } ;
	sprintf(errbuf,"Arithmetic exception trap, code (%d), pc (%p), v3op (%d)",code,SCPCVAL,process->last_v3_opcode) ;
#else
	sprintf(errbuf,"Arithmetic exception trap, code (%d), v3op (%d)",code,process->last_v3_opcode) ;
#endif
	v3_error(V3E_ARITH,"MTH","SIGNAL","ARITH",errbuf,0) ;
}

/*	v3_signal_ill - Illegal Instruction Trap			*/

#ifdef ALPHAOSF
void v3_signal_ill(sig,code,scp)
  int sig,code ;
  struct sigcontext *scp ;
#else
void v3_signal_ill(code)
  int code ;
#endif
{ void v3_signal_fpe(),v3_signal_ill() ;
#ifdef ALPHAOSF
  struct sigcontext scpx ;
#endif
  char errbuf[200] ;

	signal(SIGILL,v3_signal_ill) ; signal(SIGFPE,v3_signal_fpe) ; /* Reset signal */
#ifdef ALPHAOSF
	if (scp == NULLV) { scp = &scpx ; SCPCZAP ; } ;
	sprintf(errbuf,"Illegal instruction/addressing trap, code (%d), pc (%p), v3op (%d)",code,SCPCVAL,process->last_v3_opcode) ;
#else
	sprintf(errbuf,"Illegal instruction/addressing trap, code (%d), v3op (%d)",code,process->last_v3_opcode) ;
#endif
	if (psi->code_ptr == NULL || process->last_v3_opcode == 0 || psi->mep == 0)
	 { printf("? Fatal parsing error: %s\n",errbuf) ; exit(EXIT_FAILURE) ; } ;
	v3_error(V3E_ILL,"XCT","SIGNAL","ILL",errbuf,0) ;
}

/*	v3_signal_bus - Bus Error */

#ifdef ALPHAOSF
void v3_signal_bus(sig,code,scp)
  int sig,code ;
  struct sigcontext *scp ;
#else
void v3_signal_bus(code)
  int code ;
#endif
{ char buf[100] ;
  void v3_signal_bus() ;
#ifdef ALPHAOSF
  struct sigcontext scpx ;
#endif
  char errbuf[200] ;

#ifdef ALPHAOSF
	signal(SIGBUS,v3_signal_bus) ;	/* Reset signal */
#endif
#ifdef VMSOS
	signal(SIGBUS,v3_signal_bus) ;	/* Reset signal */
#endif
	signal(SIGSEGV,v3_signal_bus) ;
#ifdef ALPHAOSF
	if (scp == NULLV) { scp = &scpx ; SCPCZAP ; } ;
	sprintf(errbuf,"Access violation trap, code (%d), pc (%p), v3op (%d)",code,SCPCVAL,process->last_v3_opcode) ;
#else
	sprintf(errbuf,"Access violation trap, code (%d), v3op (%d)",code,process->last_v3_opcode) ;
#endif
	if (psi->code_ptr == NULL || process->last_v3_opcode == 0 || psi->mep == 0)
	 { printf("? Fatal parsing error: %s\n",errbuf) ; exit(EXIT_FAILURE) ; } ;
	v3_error(V3E_BUS,"XCT","SIGNAL","BUS",errbuf,0) ;
}

/*	I N T E R R U P T   U T I L I T I E S				*/

/*	intu_call_interrupt - Tries to invoke an interrupt	*/

void intu_call_interrupt(int_index,arg_list_ptr)
 int int_index ;			/* Index into process->interrupts */
 struct cmd__args *arg_list_ptr ;	/* NULL or ptr to arg list */
{ struct db__command_search_list csl ;	/* Command search list format */
   struct db__xct_cmd_info ci ;		/* Updated with command info */
   jmp_buf hold_xct_continue ;		/* Hold master for duration of interrupt */
   char tmp[200] ; int i ;
   extern struct iou__unit *v3unitp ;

	if (v3unitp != NULL) v4is_ErrorCleanup((struct V4IS__ParControlBlk *)v3unitp->file_ptr) ;	/* Clean up whatever due to nonfatal V4IS error */
/*	Should we actually try for an interrupt ? */
	if (process->interrupts[int_index].status != INTERRUPT_ENABLED) return ;
/*	Yes, look for the specified command */
	xctu_cmd_str_convert(process->interrupts[int_index].command_to_xct,&csl) ;
	if (!xctu_cmd_find(&csl,&ci))
	 { sprintf(tmp,"Interrupt Failed - Command (%s) not currently active",process->interrupts[int_index].command_to_xct) ;
	   v3_error(V3E_NOCMD,"INT","CALL","NOCMD",tmp,0) ;
	 } ;
/*	Looks good, invoke the command/interrupt */
	psi->stack_ptr -= 64 ;		/* Do this to get around problem of V3MACRO/V3_OPERATIONS trashing stack */
	psi = xctu_cmd_invoke(&ci,arg_list_ptr) ;
/*	Remember that this is an interrupt */
	psi->interrupt_index = int_index ;
	process->interrupts[int_index].status = INTERRUPT_ACTIVE ;
/*	Nest back into xctu_main */
	memcpy(&hold_xct_continue,&xct_continue,sizeof xct_continue) ;
	xctu_main() ;
	memcpy(&xct_continue,&hold_xct_continue,sizeof xct_continue) ;
	psi->stack_ptr += 64 ;		/* Restore the stack pointer */
/*	All done handling of interrupt, reactivate if appropriate */
	if (process->interrupts[int_index].status == INTERRUPT_ACTIVE)
	 process->interrupts[int_index].status = INTERRUPT_ENABLED ;
}

/*	intu_got_alarm - Here on alarm signal */

void intu_got_alarm()
{
	intu_call_interrupt(V3_FLAGS_INT_TIMER,NULLV) ;
}

/*	intu_disable - Called to disable an interrupt */

void intu_disable(int_index)
  int int_index ;			/* Type of interrupt being disabled */
{
/*	Maybe do something special */
	switch (int_index)
	 { case V3_FLAGS_INT_TIMER:
#ifdef POSIX
		alarm(0) ;		/* Turn off any pending alarms */
#endif
		break ;
	   case V3_FLAGS_INT_CTRLC:
		signal(SIGINT,sigint_exit_handler) ;
	 } ;
}

/*	intu_enable - Called to do anything special to enable an interrupt */

void intu_enable(int_index)
  int int_index ;
{ void intu_got_alarm(),intu_got_ctrlc() ;

	switch (int_index)
	 { case V3_FLAGS_INT_CTRLC:
		signal(SIGINT,intu_got_ctrlc) ; break ;
	 } ;
}

/*	intu_got_ctrlc - Here on CONTROL-C Signal */

void intu_got_ctrlc(arg)
  int arg ;
{
	intu_call_interrupt(V3_FLAGS_INT_CTRLC,NULLV) ;
}

/*	intu_ignore - Called to "ignore" an interrupt */

void intu_ignore(int_index)
  int int_index ;			/* Type of interrupt to ignore */
{
/*	Branch to proper handler */
	switch (int_index)
	 { case V3_FLAGS_INT_CTRLC:
		signal(SIGINT,SIG_IGN) ; break ;
	   case V3_FLAGS_INT_TIMER:
#ifdef POSIX
		alarm(0) ;
#endif
		break ;
	 } ;
	process->interrupts[int_index].status = INTERRUPT_IGNORE ;
}

/*	intu_mod_watchdog - Called to check on module cpu overrun	*/

#ifdef vms
intu_mod_watchdog(timer_id)
  int timer_id ;		/* TIMER id used to generate this AST */
{ struct db__mod_item *itemp ;
   struct db__psi *tpsi ;
   struct {
     short int buffer_length,item_code ;
     int buffer_address,return_length_address,end_of_list ;
    } jpi ;
   int cpu,dt,delta_time[2] ;
   globalvalue int JPI$_CPUTIM ;

/*	Now get current cpu time */
	jpi.buffer_length = 4 ; jpi.item_code = JPI$_CPUTIM ;
	jpi.buffer_address = &cpu ; jpi.return_length_address = 0 ; jpi.end_of_list = 0 ;
	sys$getjpi(0,0,0,&jpi,0,0,0) ;
/*	Now go thru all psi's and look for module item corresponding to this watchdog */
	tpsi = psi ; itemp = psi->item_list ;
	for(;;)
	 { if (itemp == NULLV)
	    { tpsi = tpsi->prior_psi_ptr ;
/*	      If can't find module item entry then just return, won't get here again */
	      if (tpsi == NULLV) return ;
	      itemp = tpsi->item_list ; continue ;
	    } ;
/*	   Have an item, is it worth looking at ? */
	   if (itemp->v3_exit_func != V3_FLAGS_MOD_WATCHDOG) { itemp = itemp->prior ; continue ; } ;
	   if ((int)itemp->ptr != timer_id) { itemp = itemp->prior ; continue ; } ;
/*	   Got a match - check out the cpu time */
	   if (cpu < itemp->format.all)
	    { if ((dt = (itemp->format.all - cpu)/100) <= 0) dt = 1 ;
	      if (dt > 420) dt = 420 ; delta_time[0] = -10000000 * dt ; delta_time[1] = -1 ;
	      sys$setimr(0,&delta_time,intu_mod_watchdog,(int)itemp->ptr) ; return ;
	    } ;
/*	   Time exceeded - generate error */
	   v3_error(V3E_MAXCPU,"MOD","WATCHDOG","MAXCPU","Exceeded maximum CPU as per mod_watchdog() call",0) ;
	 } ;
}
#endif

/*	intu_set_timer - Called to set a timer interrupt */

void intu_set_timer(seconds)
  int seconds ;				/* Number of seconds until timer */
{ void intu_got_alarm() ;

/*	Make sure we can do this */
	if (process->interrupts[V3_FLAGS_INT_TIMER].status == INTERRUPT_DISABLED)
	 v3_error(V3E_NOTENA,"INT","SET_TIMER","NOTENA","Cannot call INT_SET_TIMER without enabling /int_timer/ interrupt",NULLV) ;
#ifdef POSIX
	signal(SIGALRM,intu_got_alarm) ;
	alarm(seconds) ;		/* Do it */
#endif
}
