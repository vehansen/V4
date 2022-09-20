/*	V3OPER.C - Handles all V3 "Operations"

	LAST EDITED 6/29/84 BY VICTOR E. HANSEN		*/

#include <time.h>
#include "v3defs.c"
#include <math.h>
#ifdef UNIX
#ifdef INCSYS
#include <sys/types.h>
#include <sys/param.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <glob.h>
#include <sys/wait.h>
#else
#include <types.h>
#include <glob.h>
#include <param.h>
#include <times.h>
#include <stat.h>
#endif
#endif

/*	G L O B A L   E X E C U T I O N   S Y M B O L S		*/

extern struct db__process *process ;	/* Points to current process */
extern struct db__psi *psi ;		/* Points to current psi */
extern struct db__parse_info *parse ;
extern int *watch_location,watch_value ;
extern int PRS_FUNCTION_COUNT ; /* Number of V3 predefined functions */
double nint() ;
extern struct db__predef_table V3_FUNCTIONS[] ;
extern struct iou__openlist ounits ;
extern int termSocket ;
extern int V3_OPCODE_USAGE[300] ;	/* Opcode usage table */
double dpowers[] = {1.0,10.0,100.0,1000.0,10000.0,100000.0,1000000.0,10000000.0,100000000.0,1000000000.0} ;
int powers[] = {1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000} ;
int scale[] = { 1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1 } ;
extern char ttystr[V3_PRS_INPUT_LINE_MAX+1] ;	/* TTY_GETx input buffer */
extern char ttytrm[V3_TTY_TERMINATOR_MAX+1] ;	/* Holds terminator string after TTY input */
int v3_param_date = -1 ;	/* If nonnegative then V3 "date" */
int v3_param_time = -1 ;	/* If nonnegative then V3 "time" */
union val__sob obj_skel_objlr ; /* Last skeleton object referenced */

#ifdef HPUX					/* HPUX don't like OPINST ?? */
V3OPEL return_code[] =
  { V3_XCT_RETURN1, V3_XCT_RETURN } ;
#else
V3OPEL return_code[] =
  { OPINST(V3_XCT_RETURN1), OPINST(V3_XCT_RETURN) } ;	/* Code to force returns from commands */
#endif

struct {
  int count ;
  union val__v3num buf[V3_MTH_PD_LIST_MAX] ;
 } v3num_list ;

union val__v3num v3n1,v3n2,*v3np ;

/*	Declare the format of a bit map				*/

struct bitmap__def {
  int bits ;			/* Total number of bits */
  int word_array[10000] ;		/* Word array */
 } ;

char *stru_popcstr() ;
struct db__mod_item *xctu_mod_item_alloc() ;
struct db__module_entry *pcku_module_defined(/* name */) ;
struct db__module_entry *xctu_call_module_entry() ;
char *xctu_popptr() ;

/*	V 3   O P E R A T I O N S			*/

int v3_operations(code)
  int code ;			/* Operation code - See V3_XCT_xxx */
{ int ac,*base_ptr,*int_ptr ; short int *word_ptr ; unsigned int ui ;
   LONGINT i,j,k ;
   V3STACK *bsp ;			/* Base stack pointer */
   V3STACK stackel ;
   int *(*mp_ptr) ;			/* Pointer to a pointer */
   double *dbl_ptr ;
   struct db__package *pckp ;
   struct db__psi *ret_psi ;
   struct db__module_entry *mentry ;
   struct db__module_objentry *moe ;
   union val__sob sob ;
   struct str__ref strref ;	/* String reference */
   char *str_ptr,tmpstr[200],tmpstr1[200] ;		/* Temp string */
   struct bitmap__def *bm1,*bm2 ;	/* Bitmap defs */
   char errstr1[30],errstr2[30] ;
   int row,col,attr ; char *bp1,*bp2 ; short int *wp1 ; LONGINT *long_ptr ;
   unsigned char *up1, *up2 ;
   FILE *fp ;
   struct db__formatptr mod ;
   struct db__formatptr *fw,*fw1 ;
   union {
      int integer ;
      float floating ;
    } intfloat ;
   union val__format tmp_val ;	/* Temp value */
   struct db__branch_info *binfo ;
   struct db__opr_info *op ;	/* Pointer to V3 predefined function */
   struct num__ref anum ;
/*	Some things for subscripting	*/
   int sslist[V3_PRS_SUBSCRIPT_DIM_MAX] ;	/* Holds subscript values */
   int offset ;
   struct db__dim_info *sslp ;	/* Pointer to subscripting dim length info */
   union val__format sssym_format ;	/* Main symbol format */
   union val__format format ;
   struct db__do_loop_info *dlip ;	/* Pointer to do-loop info */
   static double dbl_temp1 ;

/*	Some stuff for skeleton references */
   struct db__pck_skeletons *pckskel ;
   struct db__prs_skeleton *skel ;
   struct db__skeleton_ref *skelref ;
   
	process->last_v3_opcode = code ;		/* Save for possible error checking */
	switch(code)
	 { default:
#ifdef v3v4
		if (code >= V3_V4_OPOFFSET) { v3_v4_handler(code) ; break ; }
#endif
		v3_operationsx(code) ;			/* Try seldom-used codes */
		break ;
	   case V3_XCT_AND:
		POPIJ ; if (i > 0 ? j > 0 : FALSE) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_OR:
		POPIJ ; if (i > 0 ? TRUE : j > 0) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_NOT:
		i = xctu_popint() ; if (i > 0) { PUSHFALSE ; } else { PUSHTRUE ; } ; break ;
	   case V3_XCT_ASSERTION_ADD:
		PUSHINT(i = sobu_assertion_add(stru_popcstr(tmpstr))) ; break ;
	   case V3_XCT_ASSERTION_DELETE:
		PUSHINT(i = sobu_assertion_delete(stru_popcstr(tmpstr))) ; break ;
	   case V3_XCT_ASSERTION_TEST:
		PUSHINT(i = sobu_assertion_test(stru_popcstr(tmpstr))) ; break ;
	   case V3_XCT_DATE_INFO:
		mscu_date_info(NULL) ; break ;
	   case V3_XCT_DBG_BREAK:
		dbg_break() ; break ;
	   case V3_XCT_DBG_CLEARBP:
		dbg_clearbp() ; break ;
	   case V3_XCT_DBG_SETBP:
		dbg_setbp() ; break ;
	   case V3_XCT_DBG_SHOW_CALLS:
		dbg_show_calls() ; break ;
	   case V3_XCT_STOPXCT:
		return(FALSE) ;
	   case V3_XCT_EOS:
		psi->stack_ptr = psi->reset_stack_ptr ; break ;
	   case V3_XCT_EOSLN:	/* Same as EOS but skips over source line number in next code slot */
		psi->stack_ptr = psi->reset_stack_ptr ; psi->code_ptr++ ; break ;
	   case V3_XCT_IF:
		i = xctu_popint() ;
		if (i > 0)
		 { psi->code_ptr++ ; break ; } ;
/*		Have FALSE - jump to proper location */
		i = *(psi->code_ptr) ; /* i = index within package */
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + i ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + i ; } ;
		break ;
	   case V3_XCT_JUMP:
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *psi->code_ptr ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *psi->code_ptr ; } ;
		break ;
	   case V3_XCT_JUMPC:
/*		JUMPC is a special form of jump from a command */
/*		It uses the full form of: JUMP offset,offset_level,command_level */
/*		 to determine if a psi recovery is necessary */
		i = *(psi->code_ptr) ;	/* i = offset for jump */
		j = *(++psi->code_ptr) ; /* j = level we are jumping to */
/*		If the level we are jumping to is outside of the outer command level then have to restore psi */
		if (j >= *(++psi->code_ptr))
/*		   Just a normal jump within command environment */
		 { if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + i ; }
		    else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + i ; } ;
		   break ;
		 } ;
/*		If here then restore psi to nesting of module defining the command */
		for (ret_psi=psi;ret_psi!=psi->command_psi;ret_psi=ret_psi->prior_psi_ptr)
		 { ret_psi->code_ptr = return_code ;	/* Set code_ptr to dummy area for all nested psi levels */
		 } ;
		if (psi->pic_base != NULLV) { psi->command_psi->code_ptr = psi->pic_base + i ; }
		 else { psi->command_psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + i ; } ;
		break ;
	   case V3_XCT_BIT_AND:
		POPIJ ; PUSHINT(i & j) ; break ;
	   case V3_XCT_BIT_COMP:
		i = ~xctu_popint() ; PUSHINT(i) ; break ;
	   case V3_XCT_BIT_CLEAR:
		i = ~xctu_popint() ; i = (i & xctu_popint()) ; PUSHINT(i) ; break ;
	   case V3_XCT_BIT_EQV:
		i = ~xctu_popint() ; i = (i ^ xctu_popint()) ; PUSHINT(i) ; break ;
	   case V3_XCT_BIT_GET_BYTE:
		i = xctu_popint() ; ac = xctu_popint() ;
		ac >>= (8*i) ; PUSHINT(ac & 0xFF) ; break ;
	   case V3_XCT_BIT_INVERT:
		i = xctu_popint() ;
		ac = ((i >> 24) & 0XFF) | (i << 24) | ((i >> 8) & 0XFF00) | ((i << 8) & 0XFF0000) ;
		PUSHINT(ac) ; break ;
	   case V3_XCT_BIT_OR:
		POPIJ ; PUSHINT(i | j) ; break ;
	   case V3_XCT_BIT_PACK2:
		i = xctu_popint() ; i = (i & 0XFFFF) | (xctu_popint() << 16) ;
		PUSHINT(i) ; break ;
	   case V3_XCT_BIT_ROTATE:
		v3_error(V3E_ROTATENYI,"BIT","OPERATIONS","ROTATENYI","BIT_ROTATE not yet implemented",NULLV) ;
	   case V3_XCT_BIT_SET:
		i = xctu_popint() | xctu_popint() ; PUSHINT(i) ; break ;
	   case V3_XCT_BIT_SHIFT:
		i = xctu_popint() ;
		if (i < 0) { j = (xctu_popint() << -i) ; }
		 else { j = (xctu_popint() >> i) ; } ;
		PUSHINT(j) ; break ;
	   case V3_XCT_BIT_TEST_ALL:
		i = xctu_popint() ; j = xctu_popint() ;
		if ( (j & i) != i) { PUSHFALSE ; } else { PUSHTRUE ; } ;
		break ;
	   case V3_XCT_BIT_TEST_ANY:
		if ((xctu_popint() & xctu_popint()) != 0)
		 { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_BIT_TEST_NONE:
		if ((xctu_popint() & xctu_popint()) == 0)
		 { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_BIT_TEST_NTH:
		i = xctu_popint() ; j = xctu_popint() ;
		PUSHINT((j & 1 << (i-1)) == 0 ? FALSE : TRUE) ; break ;
	   case V3_XCT_BIT_UNPACK_LH:
		i = xctu_popint() >> 16 ; PUSHINT(i) ; break ;
	   case V3_XCT_BIT_UNPACK_RH:
		i = xctu_popint() & 0XFFFF ; PUSHINT(i) ; break ;
	   case V3_XCT_BIT_XOR:
		i = xctu_popint() ^ xctu_popint() ; PUSHINT(i) ; break ;
	   case V3_XCT_BITMAP_AND:
		stru_popstr(&strref) ; bm2 = (struct bitmap__def *)strref.ptr ;
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
/*		Figure out how many words to and together */
		i = ((bm1->bits < bm2->bits ? bm1->bits : bm2->bits) + 31)/32 ;
		if (bm2->bits < bm1->bits)		/* If second is smaller than first have to clear out trailing in first */
		 { i = (bm2->bits+1+31)/32 ; j = (bm1->bits+31)/32 ;
		   for(;i<j;i++) { bm1->word_array[i] = 0 ; } ;
		   i = bm2->bits ;
		 } else { i = bm1->bits ; } ;
		i = (i+31)/32 ;
/*		Now and all bits together */
		for(j=0;j<i;j++) { bm1->word_array[j] &=  bm2->word_array[j] ; } ;
		break ;
	   case V3_XCT_BITMAP_BYTES:
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
		PUSHINT(sizeof(bm1->bits)+(bm1->bits+7)/8) ; break ;
	   case V3_XCT_BITMAP_CLEAR:
		i = xctu_popint() ;	/* Get the bit to clear */
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
/*		Set one bit or all ? */
		if (i <= 0)
		 { i = (bm1->bits+31)/32 ;
		   for(j=0;j<i;j++) { bm1->word_array[j] = 0 ; } ;
		   break ;
		 } ;
		i-- ; bm1->word_array[i/32] &= ~(1 << (i % 32)) ;
		break ;
	   case V3_XCT_BITMAP_COMP:
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
/*		Figure out how many words to compliment */
		i = (bm1->bits+31)/32 ;
/*		Now compliment all bits */
		for(j=0;j<i;j++) { bm1->word_array[j] = ~bm1->word_array[j] ; } ;
		break ;
	   case V3_XCT_BITMAP_COUNT:
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ; j = 0 ;
		for(i=0;i<bm1->bits;i++)
		 { if ((bm1->word_array[i/32] & (1 << (i%32))) != 0) j++ ; } ;
		PUSHINT(j) ; break ;
	   case V3_XCT_BITMAP_DCL:
		i = xctu_popint() ;	/* Get the number of bits */
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
/*		Make sure the bits will fit in the string */
		if ( ((strref.format.fld.length-2)/4)*32 < i )
		 v3_error(V3E_TOOMNYBITS,"BITMAP","DCL","TOOMNYBITS","Too many bits for length of string in BITMAP_DCL",(void *)i) ;
		bm1->bits = i ; break ;
	   case V3_XCT_BITMAP_EXTEND:
		i = xctu_popint() ;	/* Get the bit to set */
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
		if (i > bm1->bits)	/* If extending, then make sure all intervening bits are clear */
		 { for(j=bm1->bits;j<i;j++) { bm1->word_array[j/32] &= ~(1 << (j % 32)) ; } ;
		   bm1->bits = i ;
		 } ;
		i-- ; bm1->word_array[i/32] |= (1 << (i % 32)) ;
		break ;
	   case V3_XCT_BITMAP_NEXT_CLEAR:
		i = xctu_popint() ; stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
		ui = ~(bm1->word_array[j = i/32]) ; ui = ui >> (i % 32) ;
/*		Loop until we get a bit */
		for(;i<=bm1->bits;)
		 { for(;ui != 0;) { i++ ; if (ui & 1) goto bitmap_found_clear ; ui = ui >> 1 ; } ;
		   i = (++j)*32 ; ui = ~(bm1->word_array[j]) ;
		 } ;
bitmap_found_clear:
		if (i > bm1->bits) i = 0 ;
		PUSHINT(i) ; break ;
	   case V3_XCT_BITMAP_NEXT_SET:
		i = xctu_popint() ; stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
		ui = bm1->word_array[j = i/32] ; ui = ui >> (i % 32) ;
/*		Loop until we get a bit */
		for(;i<=bm1->bits;)
		 { for(;ui != 0;) { i++ ; if (ui & 1) goto bitmap_found_set ; ui = ui >> 1 ; } ;
		   i = (++j)*32 ; ui = bm1->word_array[j] ;
		 } ;
bitmap_found_set:
		if (i > bm1->bits) i = 0 ;
		PUSHINT(i) ; break ;
	   case V3_XCT_BITMAP_OR:
		stru_popstr(&strref) ; bm2 = (struct bitmap__def *)strref.ptr ; stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
/*		Figure out how many words to or together */
		i = ((bm1->bits < bm2->bits ? bm1->bits : bm2->bits) + 31)/32 ;
/*		Now or all bits together */
		for(j=0;j<i;j++) { bm1->word_array[j] |=  bm2->word_array[j] ; } ;
		break ;
	   case V3_XCT_BITMAP_SET:
		i = xctu_popint() ;	/* Get the bit to set */
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
/*		Clear one bit or all ? */
		if (i <= 0)
		 { i = (bm1->bits+31)/32 ;
		   for(j=0;j<i;j++) { bm1->word_array[j] = -1 ; } ;
		   break ;
		 } ;
		if (i > bm1->bits) { PUSHINT(0) ; }
		 else { i-- ; bm1->word_array[i/32] |= (1 << (i % 32)) ; PUSHINT(i+1) ; } ;
		break ;
	   case V3_XCT_BITMAP_TEST:
		i = xctu_popint() ;	/* Get the bit to test */
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
		if (i < 1 || i > bm1->bits) v3_error(V3E_INVBIT,"BITMAP","TEST","INVBIT","Bit index out of range for map",(void *)i) ;
		i-- ; PUSHINT(((bm1->word_array[i/32] & (1 << (i % 32))) != 0 ? TRUE : FALSE)) ;
		break ;
	   case V3_XCT_BITMAP_TESTDF:
		i = xctu_popint() ;	/* Get the bit to test */
		stru_popstr(&strref) ; bm1 = (struct bitmap__def *)strref.ptr ;
		if (i < 1 || i > bm1->bits) { PUSHFALSE ; }
		 else { i-- ; PUSHINT(((bm1->word_array[i/32] & (1 << (i % 32))) != 0 ? TRUE : FALSE)) ; } ;
		break ;
	   case V3_XCT_DUMMYREF:
/*		Next item in code best be offset via arg list in stack */
		i = *(psi->code_ptr++) ;
		PUSHVP(*(psi->arg_ptr-i+1)) ; PUSHF(*(psi->arg_ptr-i)) ;
		break ;
	   case V3_XCT_DUMMYREF_CMD:
		i = *(psi->code_ptr++) ;
		PUSHVP(*(psi->command_arg_ptr-i+1)) ; PUSHF(*(psi->command_arg_ptr-i)) ; break ;
	   case V3_XCT_DUMMYREFX:	/* Here to handle [module].dummy references */
		POPF(i) ; POPVP(fw,(struct db__formatptr *)) ;
		PUSHMP(fw->ptr) ; PUSHF(fw->format.all) ; break ;
	   case V3_XCT_OBJREF:
		POPF(i) ; POPVP(fw,(struct db__formatptr *)) ;
/*		Call routine with object, object pointer & "has" reference */
		if (psi->pure_base != NULLV)
		 { sobu_push_val(fw->format.all,0,fw->ptr,(struct db__dcl_objref *)(psi->pure_base + *(psi->code_ptr++)),FALSE,FALSE) ;
		 } else
		 { sobu_push_val(fw->format.all,0,fw->ptr,
			      (struct db__dcl_objref *)(process->package_ptrs[psi->package_index]->pure_ptr + *(psi->code_ptr++)),FALSE,FALSE) ;
		 } ;
		break ;
	   case V3_XCT_OBJREFSS:	/* Same as OBJREF except need subscripting info also */
		POPF(i) ; POPVP(fw,(struct db__formatptr *)) ;
/*		Call routine with object, object pointer & "has" reference */
		if (psi->pure_base != NULLV)
		 { sobu_push_val(fw->format.all,0,fw->ptr,(struct db__dcl_objref *)(psi->pure_base + *(psi->code_ptr++)),FALSE,TRUE) ;
		 } else
		 { sobu_push_val(fw->format.all,0,fw->ptr,
			      (struct db__dcl_objref *)(process->package_ptrs[psi->package_index]->pure_ptr + *(psi->code_ptr++)),FALSE,TRUE) ;
		 } ;
		break ;
	   case V3_XCT_INT_DISABLE:
		i = xctu_popint() ; intu_disable(i) ;
		process->interrupts[i].status = INTERRUPT_DISABLED ; process->interrupts[i].command_to_xct[0] = NULLV ;
		break ;
	   case V3_XCT_INT_ENABLE:
		stru_popcstr(tmpstr) ; i = xctu_popint() ;
		process->interrupts[i].status = INTERRUPT_ENABLED ;
		strcpy(process->interrupts[i].command_to_xct,tmpstr) ;
		intu_enable(i) ; break ;
	   case V3_XCT_INT_IGNORE:	i = xctu_popint() ; intu_ignore(i) ; break ;
	   case V3_XCT_INT_SET_TIMER:	i = xctu_popint() ; intu_set_timer(i) ; break ;
	   case V3_XCT_IO_CLOSE:	stru_popstr(&strref) ; iou_close((struct iou__unit *)strref.ptr,NULLV) ; break ;
	   case V3_XCT_IO_GET:		stru_popstr(&strref) ; iou_get((struct iou__unit *)strref.ptr) ; break ;
	   case V3_XCT_IO_MISC:		iou_misc() ; break ;
	   case V3_XCT_IO_OPEN:		stru_popstr(&strref) ; iou_open((struct iou__unit *)strref.ptr) ; break ;
	   case V3_XCT_IO_PUT:		stru_popstr(&strref) ; iou_put((struct iou__unit *)strref.ptr) ; break ;
	   case V3_XCT_ITX_ALPHA:	i = xti_alpha(FALSE) ; PUSHINT(i) ; break ;
	   case V3_XCT_ITX_DATE:	i = itx_date() ; PUSHINT(i) ; break ;
	   case V3_XCT_ITX_DATE_TIME:	i = itx_date_time() ; PUSHINT(i) ; break ;
	   case V3_XCT_ITX_INT:		i = itx_int() ; PUSHINT(i) ; break ;
	   case V3_XCT_ITX_GEN:		itx_gen() ; break ;
	   case V3_XCT_ITX_LOGICAL:	i = itx_logical() ; PUSHINT(i) ; break ;
	   case V3_XCT_ITX_MONTH:	i = itx_month() ; PUSHINT(i) ; break ;
	   case V3_XCT_ITX_POINTER:	i = itx_pointer() ; PUSHINT(i) ; break ;
	   case V3_XCT_ITX_TIME:	i = itx_time() ; PUSHINT(i) ; break ;
	   case V3_XCT_PUSHEOA:		PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ; break ;
	   case V3_XCT_MOD_APPLY:
/*		Pop off args until EOA */
		for (ac=0;;ac+=2)
		 { POPF(tmp_val.all) ; POPVI(i) ; if (tmp_val.all == V3_FORMAT_EOA) break ; } ;
/*		Was last (first) argument a string or MODREF ? */
		tmp_val.all = *(psi->stack_ptr-(2*SIZEOFSTACKENTRY)) ;
		switch(tmp_val.fld.type)
		 { default:
		    v3_error(V3E_ARGNOTMOD,"MOD","APPLY","ARGNOTMOD","First arg to MOD_APPLY NOT a module reference",0) ;
		   case VFT_MODREF:
			mod.format.all = *(psi->stack_ptr - (2*SIZEOFSTACKENTRY)) ; mod.ptr = (char *)*(psi->stack_ptr-3) ;
			break ;
		   case VFT_POINTER:
			mod.format.fld.type = VFT_MODREF ; mod.format.fld.mode = VFM_MEP ; mod.ptr = xctu_popptr() ;
			break ;
		   case VFT_INDIRECT:
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
		   case VFT_VARSTR:
			psi->stack_ptr -= (2*SIZEOFSTACKENTRY) ; stru_popcstr(tmpstr) ; psi->stack_ptr += SIZEOFSTACKENTRY ;
/*			Convert to upper case */
			for (i=0;tmpstr[i] != 0;i++)
			 { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ; } ;
/*			Got a string - lookup symbol */
			if ((mentry = pcku_module_defined(tmpstr)) != NULLV)
			 { mod.format.fld.type = VFT_MODREF ; mod.format.fld.mode = VFM_MEP ; mod.ptr = (char *)mentry ;
			 } else
/*			   Not a user symbol - see if V3 predefined */
			 { if ((op = (struct db__opr_info *)prsu_predefined_lookup(tmpstr,V3_FUNCTIONS,PRS_FUNCTION_COUNT)) == NULLV)
			    { sprintf(tmpstr1,"Name (%s) in mod_apply() not an active module",tmpstr) ;
			      v3_error(V3E_NOTDEF,"MOD","APPLY","NOTDEF",tmpstr1,0) ;
			    } ;
			   mod.format.fld.type = VFT_MODREF ; mod.format.fld.mode = VFM_IMMEDIATE ;
			   mod.format.fld.decimals = op->end_of_arg_list ; mod.ptr = (char *)op->reference ;
			 } ;
		 } ;
/*		Now recreate the stack with just the arguments */
		bsp = psi->stack_ptr - (2*SIZEOFSTACKENTRY) ; psi->stack_ptr -= SIZEOFSTACKENTRY ;
		for (i=2;i<ac;i++) { *(--psi->stack_ptr) = *(--bsp) ; } ;
/*		And finally call the module */
		xctu_call_defer(&mod) ; break ;
	   case V3_XCT_MOD_CURRENT_PSI:
		PUSHMP(psi) ; PUSHF(V3_FORMAT_POINTER) ; break ;
	   case V3_XCT_MOD_DEFINED:
/*		Is argument a string or MODREF ? */
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type != VFT_MODREF)
		 { if (tmp_val.fld.type == 0)
		    { POPVI(i) ; PUSHFALSE ; break ; } ;
		   PUSHF(tmp_val.all) ; stru_popcstr(tmpstr) ;
/*		   Convert to upper case */
		   for (i=0;tmpstr[i] != 0;i++)
		    { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ; } ;
/*		   Got a string - lookup symbol */
		   if ((mentry = pcku_module_defined(tmpstr)) != NULLV)
		    { PUSHINT(mentry->package_id > 0 ? mentry->package_id : 9998) ;
		      break ;
		    } else
/*		      Not a user symbol - see if V3 predefined */
		    { if ((op = (struct db__opr_info *)prsu_predefined_lookup(tmpstr,V3_FUNCTIONS,PRS_FUNCTION_COUNT)) == NULLV)
		       { PUSHFALSE ; break ; } ;
/*		      Module is V3 Primitive module */
		      PUSHINT(9999) ; break ;
		    } ;
		 } ;
/*		Here then first arg was already module reference */
		POPVP(bp1,(char *)) ;
		if (tmp_val.fld.mode == VFM_IMMEDIATE) { PUSHINT(1000) ; }
		 else { if ((mentry = xctu_call_module_entry(tmp_val.all,bp1,NULLV)) == NULLV) { PUSHFALSE ; break ; } ;
			PUSHINT(mentry->package_id > 0 ? mentry->package_id : 9998) ;
		      } ;
		break ;
	   case V3_XCT_MOD_ENTRY_HANDLE:
/*		Is argument a string or MODREF ? */
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type != VFT_MODREF)
		 { if (tmp_val.fld.type == 0)
		    { POPVI(i) ; PUSHFALSE ; break ; } ;
		   PUSHF(tmp_val.all) ; stru_popcstr(tmpstr) ;
/*		   Convert to upper case */
		   for (i=0;tmpstr[i] != 0;i++)
		    { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ; } ;
/*		   Got a string - lookup symbol */
		   if ((mentry = pcku_module_defined(tmpstr)) != NULLV)
		    { PUSHMP(mentry) ; PUSHF(V3_FORMAT_POINTER) ; break ;
		    } else
/*		      Not a user symbol - see if V3 predefined */
		    { if ((op = (struct db__opr_info *)prsu_predefined_lookup(tmpstr,V3_FUNCTIONS,PRS_FUNCTION_COUNT)) == NULLV)
		       { PUSHFALSE ; break ; } ;
/*		      Module is V3 Primitive module */
		      PUSHINT(op->reference) ; break ;
		    } ;
		 } ;
/*		Here then first arg was already module reference */
		POPVP(bp1,(char *)) ;
		if (tmp_val.fld.mode == VFM_IMMEDIATE) { PUSHINT(1000) ; }
		 else { if ((mentry = xctu_call_module_entry(tmp_val.all,bp1,NULLV)) == NULLV) { PUSHFALSE ; break ; } ;
		        PUSHMP(mentry) ; PUSHF(V3_FORMAT_POINTER) ;
		      } ;
		break ;
	   case V3_XCT_MOD_ITEM_DELETE:
		xctu_mod_item_delete() ; break ;
	   case V3_XCT_MOD_ITEM_FIND:
		xctu_mod_item_find() ; break ;
	   case V3_XCT_MOD_ITEM_PUSH:
		xctu_mod_item_push() ; break ;
	   case V3_XCT_MOD_NAME:
/*		Was last (first) argument a string or MODREF ? */
		POPF(mod.format.all) ;
		if (mod.format.fld.type != VFT_MODREF)
		 v3_error(V3E_NOTMODREF,"MOD","NAME","NOTMODREF","Argument to MOD_NAME is not a module reference",(void *)mod.format.all) ;
		if (mod.format.fld.mode == VFM_PTR)
		 { POPVP(moe,(struct db__module_objentry *)) ;
/*		   Return string name of module */
		   PUSHMP(&moe->name) ; PUSHF(V3_FORMAT_VARSTR) ;
		 } else
/*		   Module is a V3 primitive, just return the opcode */
		 { bp1 = (char *)stru_alloc_bytes(15) ; POPVI(i) ;
		   sprintf(bp1,"*V3OP_%d*",i) ;
		   PUSHMP(bp1) ; PUSHF(V3_FORMAT_VARSTR) ;
		 } ;
		break ;
	   case V3_XCT_MOD_PRIOR_PSI:
		PUSHMP(psi->prior_psi_ptr) ; PUSHF(V3_FORMAT_POINTER) ; break ;
	   case V3_XCT_MOD_UNLOAD:
		stru_popcstr(tmpstr) ;
/*		Convert to upper case */
		for (i=0;tmpstr[i] != 0;i++)
		 { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ; } ;
		if ((mentry = pcku_module_defined(tmpstr)) == NULLV) { PUSHINT(0) ; break ; } ;	/* Not define? */
		PUSHINT(pcku_module_unload(mentry)) ; break ;
	   case V3_XCT_MOD_WATCHDOG:
		xctu_mod_watchdog() ; break ;
	   case V3_XCT_MODCALL:
		xctu_call(FALSE) ; break ;
	   case V3_XCT_RETURN:
		return(xctu_return()) ;
	   case V3_XCT_RETURN1:
/*		Here to handle possible exit routine calls to other modules */
		if (psi->item_list != NULLV) xctu_mod_item_exit(psi,FALSE) ;
		break ;
	   case V3_XCT_BRANCH:
		binfo = (struct db__branch_info *)(psi->pure_base + *(psi->code_ptr++)) ;
/*		Pop off branch index number */
		i = xctu_popint() ;
/*		Is this a "PATH" or "PATH(value)" branch ? */
		if (binfo->count < 0)
		 { i -= binfo->min_value ;	/* Convert index */
/*		   If index out of range or is value of undefined path then fall thru */
		   if (i < 0 || i > -binfo->count) break ;
		   if ((i = binfo->path_offsets[i]) == 0) break ;
/*		   Got a valid value, update "pc" to it */
		   if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + i ; }
		    else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + i ; } ;
		   break ;
		 } ;
/*		Have a regular "PATH", if out of range then drop thru */
		if ( i < 1 || i > binfo->count) break ;
/*		In range - update psi */
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + binfo->path_offsets[i-1] ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + binfo->path_offsets[i-1] ; } ;
		break ;
	   case V3_XCT_UPDATE:
/*		This is integer form of update */
		i = xctu_popint() ; POPF(tmp_val.all) ;
		switch (tmp_val.fld.type)
		 { case VFT_BININT:
			POPVP(int_ptr,(int *)) ; *int_ptr = i ; break ;
		   case VFT_BINLONG:
			POPVP(long_ptr,(LONGINT *)) ; *long_ptr = i ; break ;
		   case VFT_BINWORD:
			POPVP(word_ptr,(short int *)) ; *word_ptr = i ; break ;
		   default:
			v3_error(V3E_INVINTUPD,"XCT","UPDATE","INVINTUPD","Invalid datatype for integer update",0) ;
		 } ;
		REPUSH ; break ;
	   case V3_XCT_UPDATE+1:
xct_update:
/*		Get format of dst */
		tmp_val.all = NEXTSF ;
		switch (tmp_val.fld.type)
		 { case VFT_BININT:
			v3num_popv3n(&v3n1) ;
			POPF(tmp_val.all) ; POPVP(base_ptr,(int *)) ;
			if (v3n1.fp.marker == V3NUM_FPMARK)
			 { *base_ptr = nint(v3n1.fp.dbl * dpowers[tmp_val.fld.decimals]) ; }
			 else { if (v3n1.fd.big != 0)
				 { if (v3n1.fd.big > 3) v3_error(V3E_TOOBIGTOUPD,"XCT","UPDATE","TOOBIGTOUPD","Number too big for integer update",0) ;
				   *base_ptr = v3n1.fd.big * 1000000000 + v3n1.fd.num ; break ;
				 } ;
				if (tmp_val.fld.decimals == 0)
				 { *base_ptr = v3n1.fd.num ; if (v3n1.fd.dps >= 500000000) (*base_ptr)++ ; }
				 else { *base_ptr = v3n1.fd.num * powers[tmp_val.fld.decimals] ;
					*base_ptr += (v3n1.fd.dps / scale[tmp_val.fld.decimals]) ;
				      } ;
			      } ;
			break ;
		   case VFT_BINLONG:
			v3num_popv3n(&v3n1) ;
			POPF(tmp_val.all) ; POPVP(long_ptr,(LONGINT *)) ;
			if (v3n1.fp.marker == V3NUM_FPMARK)
			 { *long_ptr = nint(v3n1.fp.dbl * dpowers[tmp_val.fld.decimals]) ; }
			 else { if (v3n1.fd.big != 0)
				 { if (v3n1.fd.big > 3) v3_error(V3E_TOOBIG,"XCT","UPDATE","TOOBIG","Number too big for integer update",0) ;
				   *long_ptr = (LONGINT)v3n1.fd.big * (LONGINT)1000000000 + (LONGINT)v3n1.fd.num ; break ;
				 } ;
				if (tmp_val.fld.decimals == 0)
				 { *long_ptr = (LONGINT)v3n1.fd.num ; if (v3n1.fd.dps >= 500000000) (*long_ptr)++ ; }
				 else { *long_ptr = (LONGINT)v3n1.fd.num * (LONGINT)powers[tmp_val.fld.decimals] ;
					*long_ptr += (v3n1.fd.dps / scale[tmp_val.fld.decimals]) ;
				      } ;
			      } ;
			break ;
		   case VFT_BINWORD:
			v3num_popv3n(&v3n1) ;
			POPF(tmp_val.all) ; POPVP(word_ptr,(short int *)) ;
			if (v3n1.fp.marker == V3NUM_FPMARK)
			 { *word_ptr = nint(v3n1.fp.dbl * dpowers[tmp_val.fld.decimals]) ; }
			 else { if (v3n1.fd.big != 0)
				 { sprintf(tmpstr,"Number (%d %d %d) too big for 16-bit integer update",
						v3n1.fd.big,v3n1.fd.num,v3n1.fd.dps) ;
				   v3_error(V3E_TOOBIG,"XCT","UPDATE","TOOBIG",tmpstr,0) ;
				 } ;
				if (tmp_val.fld.decimals == 0)
				 { *word_ptr = v3n1.fd.num ; if (v3n1.fd.dps >= 500000000) (*word_ptr)++ ; }
				 else { *word_ptr = v3n1.fd.num * powers[tmp_val.fld.decimals] ;
					*word_ptr += (v3n1.fd.dps / scale[tmp_val.fld.decimals]) ;
				      } ;
			      } ;
			break ;
		   case VFT_POINTER:
			bp1 = xctu_popptr() ;					/* Pop pointer off of stack */
			POPF(tmp_val.all) ; POPVP(mp_ptr,(int *(*))) ;		/* Pop pointer to pointer value */
			*mp_ptr = (int *)bp1 ;					/*  and update */
			break ;
		   case VFT_FLOAT:
			xctu_popnum(&anum) ; POPF(tmp_val.all) ; POPVP(dbl_ptr,(double *)) ;
			mthu_cvtatr(&anum,dbl_ptr) ;
			break ;
			switch (anum.format.fld.type)
			 { default: v3_error(V3E_INVREALUPD,"XCT","UPDATE","INVREALUPD","Invalid value for REAL update",0) ;
			   case VFT_BINWORD:
				word_ptr = (short int *)anum.ptr ;
				mthu_cvtid(*word_ptr,anum.format.fld.decimals,dbl_ptr) ; break ;
			   case VFT_BINLONG:
				mthu_cvtid(*anum.ptr,anum.format.fld.decimals,dbl_ptr) ; break ;
			   case VFT_BININT:
				int_ptr = (int *)anum.ptr ;
				mthu_cvtid(*int_ptr,anum.format.fld.decimals,dbl_ptr) ; break ;
			   case VFT_FLOAT:
				*dbl_ptr = *anum.dbl_ptr ; break ;
			   case VFT_PACDEC:
				mthu_cvtid(mthu_cvtpi(anum.ptr,anum.format.fld.length,0,0),anum.format.fld.decimals,dbl_ptr) ;
				break ;
			   case VFT_STRINT:
				mthu_cvtid(mthu_cvtsi((char *)anum.ptr,anum.format.fld.length,anum.format.fld.decimals,anum.format.fld.decimals),anum.format.fld.decimals,dbl_ptr) ;
				break ;
			 } ;
			break ;
		   case VFT_STRINT:
			xctu_popnum(&anum) ; POPF(tmp_val.all) ; POPVP(str_ptr,(char *)) ;
			switch (anum.format.fld.type)
			 { default: v3_error(V3E_INVSTRINT,"XCT","UPDATE","INVSTRINT","Invalid value for DEC field",0) ;
			   case VFT_FLOAT:
				v3n1.fp.dbl = *anum.dbl_ptr ; v3n1.fp.marker = V3NUM_FPMARK ;
				mthu_cvtvs(&v3n1,(char *)str_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_BINWORD:
				word_ptr = (short int *)anum.ptr ;
				mthu_cvtis((LONGINT)*word_ptr,anum.format.fld.decimals,str_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_BINLONG:
				mthu_cvtis((LONGINT)*anum.ptr,anum.format.fld.decimals,str_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_BININT:
				int_ptr = (int *)anum.ptr ;
				mthu_cvtis((LONGINT)*int_ptr,anum.format.fld.decimals,str_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_STRINT:
				mthu_cvtss((char *)anum.ptr,anum.format.fld.length,anum.format.fld.decimals,
					   str_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_V3NUM:
				mthu_cvtvs((union val__v3num *)anum.ptr,str_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_PACDEC:
				mthu_cvtps(anum.ptr,anum.format.fld.length,anum.format.fld.decimals,
					   str_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			  } ;
			break ;
		   case VFT_V3NUM:
			v3num_popv3n(&v3n1) ; POPF(tmp_val.all) ; POPVP(v3np,(union val__v3num *)) ;
			*v3np = v3n1 ; break ;
		   case VFT_PACDEC:
			xctu_popnum(&anum) ; POPF(tmp_val.all) ; POPVP(base_ptr,(int *)) ;
			switch (anum.format.fld.type)
			 { default: v3_error(V3E_INVPACDEC,"XCT","UPDATE","INVPACDEC","Invalid value for IP field",0) ;
			   case VFT_BINWORD:
				word_ptr = (short int *)anum.ptr ;
				mthu_cvtip(*word_ptr,anum.format.fld.decimals,base_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_BINLONG:
				mthu_cvtip(*anum.ptr,anum.format.fld.decimals,base_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_BININT:
				int_ptr = (int *)anum.ptr ;
				mthu_cvtip(*int_ptr,anum.format.fld.decimals,base_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_STRINT:
				mthu_cvtsp(anum.ptr,anum.format.fld.length,anum.format.fld.decimals,
					   base_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			   case VFT_PACDEC:
				mthu_cvtpp(anum.ptr,anum.format.fld.length,anum.format.fld.decimals,
					   base_ptr,tmp_val.fld.length,tmp_val.fld.decimals) ;
				break ;
			 } ;
			break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			stru_updfix() ; break ;
		   case VFT_VARSTR:
			stru_updvar() ; break ;
		   case VFT_INDIRECT:
			stru_updind() ; break ;
		   case VFT_STRUCTPTR:
/*			Are we updating with pointer or string (for value) */
			POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
			switch(tmp_val.fld.type)
			 { case VFT_POINTER:
				bp1 = xctu_popptr() ; POPF(tmp_val.all) ; POPVP(mp_ptr,(int *(*))) ; *mp_ptr = (int *)bp1 ;
				break ;
			   case VFT_VARSTR:
			   case VFT_FIXSTRSK:
			   case VFT_FIXSTR:
			   case VFT_STRUCTPTR:
				stru_updfix() ; break ;
			   case VFT_BINLONG:
			   case VFT_BININT:		/* Only allow update of 0 (null pointer) */
				i = xctu_popint() ;
				if (i == 0) { POPF(tmp_val.all) ; POPVP(mp_ptr,(int *(*))) ; *mp_ptr = NULLV ; break ; } ;
			   default:
				v3_error(V3E_INVSTRPTRVAL,"XCT","UPDATE","INVSTRPTRVAL",
					  "Can only update structure pointer with string or pointer",(void *)tmp_val.fld.type) ;
			 } ;
			break ;
		   case VFT_OBJREF:
/*			Update first or second word depending on value -
			 if object then update first (format)
			 if numeric then update second (pointer)	*/
			POPF(tmp_val.all) ; POPVP(fw1,(struct db__formatptr *)) ; POPF(j) ; POPVP(fw,(struct db__formatptr *)) ;
			if (tmp_val.fld.type == VFT_POINTER)
			 { PUSHMP(fw1) ; PUSHF(tmp_val.all) ; base_ptr = (int *)xctu_popptr() ;
			   fw->ptr = (char *)base_ptr ;
			 } else
			 { if (tmp_val.fld.type != VFT_OBJECT)
			    { if (tmp_val.fld.type != VFT_OBJREF)
			       v3_error(V3E_UPDOBJREF,"XCT","UPDATE","UPDOBJREF",
					"Can only update OBJREF with pointer, object, or another OBJREF",(void *)tmp_val.fld.type) ;
			      fw->format.all = fw1->format.all ; fw->ptr = fw1->ptr ;
			    }
			    else
			    { fw->format.all = tmp_val.all ; fw->ptr = (char *)fw1 ; } ;
			 } ;
			break ;
		   default: v3_error(V3E_BADUPDATE,"XCT","UPDATE","BADUPDATE","Invalid datatype for update",(void *)tmp_val.fld.type) ;
		 } ;
/*		Push result back onto stack */
		REPUSH ; break ;
	   case V3_XCT_EQUAL_EQUAL:
		stru_share() ; break ;
	   case V3_XCT_MTH_ABS:
		i = xctu_popint() ; PUSHINT( (i<0 ? -i : i) ) ; break ;
	   case V3_XCT_MTH_DIGITS:
		i = xctu_popint() ;
		if (i > 0xFFFF) { j = i >> 16 ; i &= 0xFFFF ; }
		 else j = i ;
		k = xctu_popint() ;
/*		j = first digit, i = last, k = number */
		PUSHINT( (k/powers[j-1]) % powers[i-j+1]) ; break ;
	   case V3_XCT_MTH_FTOI:
		i = xctu_popint() ; intfloat.integer = xctu_popint() ;
/*		Scale the floating point number */
		if (i < 0) { for(;i<0;i++) { intfloat.floating /= 10.0 ; } ; }
		 else { for(;i>0;i--) { intfloat.floating *= 10.0 ; } ; } ;
		i = intfloat.floating ; PUSHINT(i) ; break ;
	   case V3_XCT_MTH_MAX:
	   case 82:			/* Temp to get everything compiled */
		i = xctu_popint() ;
		for (;;)
		 { POPF(tmp_val.all) ; if (tmp_val.all == V3_FORMAT_EOA) break ;
		   PUSHF(tmp_val.all) ; j = xctu_popint() ; if (j > i) i = j ;
		 } ;
		POPVI(j) ; PUSHINT(i) ; break ;
	   case V3_XCT_MTH_MAX+1:
		v3num_popv3n(&v3n2) ; REPUSH ; POPF(format.all) ; POPVI(stackel) ;
		for (;;)
		 { POPF(tmp_val.all) ; if (tmp_val.all == V3_FORMAT_EOA) break ;
		   PUSHF(tmp_val.all) ; v3num_popv3n(&v3n1) ;
		   if (v3num_cmp(&v3n1,&v3n2) > 0) { v3n2 = v3n1 ; REPUSH ; POPF(format.all) ; POPVI(stackel) ; } ;
		 } ;
		POPVI(j) ; PUSHVP(stackel) ; PUSHF(format.all) ; break ;
	   case V3_XCT_MTH_MIN:
	   case 83:			/* Temp to get everything compiled */
		i = xctu_popint() ;
		for (;;)
		 { POPF(tmp_val.all) ; if (tmp_val.all == V3_FORMAT_EOA) break ;
		   PUSHF(tmp_val.all) ; j = xctu_popint() ; if (j < i) i = j ;
		 } ;
		POPVI(j) ; PUSHINT(i) ; break ;
	   case V3_XCT_MTH_MIN+1:
		v3num_popv3n(&v3n2) ; REPUSH ; POPF(format.all) ; POPVI(stackel) ;
		for (;;)
		 { POPF(tmp_val.all) ; if (tmp_val.all == V3_FORMAT_EOA) break ;
		   PUSHF(tmp_val.all) ; v3num_popv3n(&v3n1) ;
		   if (v3num_cmp(&v3n1,&v3n2) < 0) { v3n2 = v3n1 ; REPUSH ; POPF(format.all) ; POPVI(stackel) ; } ;
		 } ;
		POPVI(j) ; PUSHVP(stackel) ; PUSHF(format.all) ; break ;
	   case V3_XCT_MTH_MOD:
		i = xctu_popint() ; j = xctu_popint() % i ; PUSHINT(j) ; break ;
	   case V3_XCT_MTH_MOD1:
		i = xctu_popint() ; j = xctu_popint() ; j = (j < 0 ? -j : j) % i ;
		PUSHINT( (j == 0 ? i : j) ) ; break ;
	   case V3_XCT_MTH_RANDOM:
		i = xctu_popint() ;
		PUSHINT(rand() % i) ; break ;
	   case V3_XCT_MTH_RANDOM_SEED:
		srand(xctu_popint()) ; break ;
	   case V3_XCT_NIL:
		PUSHVI(NULLV) ; PUSHF(V3_FORMAT_NULL) ; break ;
	   case V3_XCT_DOLOOP1:	/* Do-loop setup for LOOP1 */
/*		Code of form: var, init, :=, max, [inc,] loop1/2, inc_flag, loop3, info_ptr, out, out_level, body */
		i = *(psi->code_ptr++) ;	/* i = have_inc flag */
		dlip = (struct db__do_loop_info *)(psi->stack_space + *(++psi->code_ptr)) ;
/*		Get the increment */
		dlip->inc.int_val = (i ? xctu_popint() : 1) ;
		dlip->max.int_val = xctu_popint() ;	/* Get the max value */
		POPF(tmp_val.all) ; dlip->type = tmp_val.fld.type ; POPVP(dlip->var.int_val,(int *)) ;
		psi->code_ptr += 3 ; break ;
	   case V3_XCT_DOLOOP2: /* Do-loop setup for LOOP */
		i = *(psi->code_ptr++) ; dlip = (struct db__do_loop_info *)(psi->stack_space + *(++psi->code_ptr)) ;
		dlip->inc.int_val = (i ? xctu_popint() : 1) ; dlip->max.int_val = xctu_popint() ;
		POPF(tmp_val.all) ; dlip->type = tmp_val.fld.type ; POPVP(dlip->var.int_val,(int *)) ;
		psi->code_ptr += 3 ;
/*		Now have to test */
		i = (dlip->type == VFT_BINWORD ? *dlip->var.short_val : *dlip->var.int_val) ;
		if (dlip->inc.int_val < 0) { if (i >= dlip->max.int_val) break ; }
		 else { if (i <= dlip->max.int_val) break ; } ;
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *(psi->code_ptr-2) ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *(psi->code_ptr-2) ; } ;
		break ;
	   case V3_XCT_DOLOOP3: /* Test/Increment for do-loop */
		dlip = (struct db__do_loop_info *)(psi->stack_space + *psi->code_ptr) ; psi->code_ptr += 3 ;
/*		Get current value & update with increment */
		if (dlip->type == VFT_BINWORD)
		 { i = (*dlip->var.short_val += dlip->inc.int_val) ; }
		 else { i = (*dlip->var.int_val += dlip->inc.int_val) ; } ;
		if (dlip->inc.int_val < 0) { if (i < dlip->max.int_val) goto loop_end ; }
		 else { if (i > dlip->max.int_val) goto loop_end ; } ;
/*		Test ok, just continue */
		break ;
loop_end:	/* Here to kick out of loop */
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *(psi->code_ptr-2) ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *(psi->code_ptr-2) ; } ;
		break ;
	   case V3_XCT_REPEAT1:
/*		This routine pops of repeat value & saves in stack space for module invocation */
		base_ptr = (int *)(psi->stack_space + *(psi->code_ptr++)) ;
		*base_ptr = 1+xctu_popint() ;
		break ;
	   case V3_XCT_REPEAT2:
/*		This routine decrements repeat value & loops until <= 0 */
		base_ptr = (int *)(psi->stack_space + *(psi->code_ptr++)) ;
		if (--(*base_ptr) > 0)
		 { psi->code_ptr += 2 ;	/* Skip over jump out offset */
		   break ;
		 } ;
/*		Have to jump out of loop */
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *psi->code_ptr ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *psi->code_ptr ; } ;
		break ;
	   case V3_XCT_REPEAT3:
/*		This is similar to REPEAT1 but is called for LOOP1(n) */
		base_ptr = (int *)(psi->stack_space + *(psi->code_ptr++)) ;
		if ((*base_ptr = 1+xctu_popint()) < 2) *base_ptr = 2 ;
		break ;
	   case V3_XCT_LOOP_TEST:
/*		This is sort-of an "if" for LOOP statements */
/*		Code is: TEST,jump-index for fail,0,jump-index for ok,0 */
		if (xctu_popint() > 0) psi->code_ptr += 2 ;
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *psi->code_ptr ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *psi->code_ptr ; } ;
		break ;
	   case V3_XCT_LOOP_TEST_TRUE:
/*		Same as above but for "loop(xx;;xx)" construct with no test - always takes "true" path */
		if (psi->pic_base != NULLV) { psi->code_ptr = psi->pic_base + *(psi->code_ptr+2) ; }
		 else { psi->code_ptr = process->package_ptrs[psi->package_index]->code_ptr + *(psi->code_ptr+2) ; } ;
		break ;
	   case V3_XCT_NULL:
		POPF(tmp_val.all) ;
/*		Return TRUE if argument is nil or argument is pointer with no value */
		if (tmp_val.all == V3_FORMAT_NULL) { POPVI(i) ; PUSHTRUE ; break ; } ;
		if (tmp_val.fld.type == VFT_POINTER)
		 { PUSHF(tmp_val.all) ; bp1 = (char *)xctu_popptr() ; if (bp1 == NULLV) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		 } else { POPVI(i) ; PUSHFALSE ; } ;
		break ;
	   case V3_XCT_FORK_DELETE:
		fork_delete() ; break ;
	   case V3_XCT_FORK_NEW:
		fork_new() ; break ;
	   case V3_XCT_FORK_SWITCH:
		POPF(tmp_val.all) ;
		if (tmp_val.all == V3_FORMAT_EOA) { POPVI(i) ; } else { PUSHF(tmp_val.fld.type) ; } ;
		fork_switch() ; xctu_mod_item_fork(FALSE) ; break ;
	   case V3_XCT_FORK_SWITCH1:
/*		Called first to handle "/fork_leave/" on item stack */
		PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
		xctu_mod_item_fork(TRUE) ; break ;
	   case	V3_XCT_ZAP_OBJ_SKEL_OBJLR:
		obj_skel_objlr.all = NULLV ; break ;
	   case	V3_XCT_EVAL_SKELETON_SS:
		i = 1 ; goto skel_entry ;	/* Just set flag & enter below */
	   case V3_XCT_EVAL_SKELETON:
		i = 0 ;				/* 0 means no subscripting */
skel_entry:
		pckp = process->package_ptrs[psi->package_index] ;
		pckskel = pckp->skeleton_ptr ;
		if (psi->pure_base != NULLV)
		 { skelref = (struct db__skeleton_ref *)(psi->pure_base + *(psi->code_ptr++)) ; }
		 else { skelref = (struct db__skeleton_ref *)(pckp->pure_ptr + *(psi->code_ptr++)) ; } ;
		skel = (struct db__prs_skeleton *)&(pckskel->buf[pckskel->index[skelref->skel_index]]) ;
/*		If element index is 0 then referencing entire structure */
		if (skelref->el_index == -1)
		 { PUSHMP(xctu_pointer(skelref->base_where.all)) ;
		   tmp_val.all = V3_FORMAT_STRUCTPTR ; tmp_val.fld.length = skel->alloc_bytes ; PUSHF(tmp_val.all) ;
		   break ;
		 } ;
/*		Handle subscripts ? */
		if (i)
		 { PUSHMP(&(skel->el[skelref->el_index].dims)) ; PUSHF(0) ; } ;
		mp_ptr = (int *(*))xctu_pointer(skelref->base_where.all) ;
		if (!skel->el[skelref->el_index].defined)
		 v3_error(V3E_UNDEDEL,"XCT","EVAL_SKELETON","UNDEDEL","Undefined skeleton element reference",(void *)skelref->el_index) ;
		PUSHMP(skel->el[skelref->el_index].offset + (char *)(*mp_ptr)) ;
		tmp_val.all = skel->el[skelref->el_index].format.all ;
/*
		if (tmp_val.fld.type == VFT_FIXSTR) tmp_val.fld.type = VFT_FIXSTRSK ; /* Substitute special code for alpha's */
		PUSHF(tmp_val.all) ;
/*		Remember what element we just referenced */
		obj_skel_objlr.all = skel->el[skelref->el_index].el_object.all ;
		break ;

/*	A R I T H M E T I C  &   M A T H E M A T I C A L	*/

	   case V3_XCT_MTH_DIV:
		i = xctu_popint() ; i = xctu_popint()/i ; PUSHINT(i) ; break ;
	   case V3_XCT_MTH_DIV+1:
		v3np = (union val__v3num *)&v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(&v3n2) ; i |= v3num_popv3n(v3np) ;
		v3num_mdas(v3np,&v3n2,2) ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ; break ;
	   case V3_XCT_MTH_EQUAL:
		POPIJ ; if (i==j) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_MTH_EQUAL+1:
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ; if (tmp_val.fld.type != VFT_POINTER) { tmp_val.all = NEXTSF ; } ;
		if (tmp_val.fld.type == VFT_POINTER)
		 { bp1 = xctu_popptr() ; bp2 = xctu_popptr() ;
		   if (bp1 == bp2) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		   break ;
		 } ;
		v3num_popv3n(&v3n2) ; v3num_popv3n(&v3n1) ;
		if (v3num_cmp(&v3n1,&v3n2) == 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_MTH_GREATER:
		POPIJ ; if (i > j) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_MTH_GREATER+1:
		v3num_popv3n(&v3n2) ; v3num_popv3n(&v3n1) ;
		if (v3num_cmp(&v3n1,&v3n2) > 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_MTH_GREATEREQUAL:
		POPIJ ; if (i>=j) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_MTH_GREATEREQUAL+1:
		v3num_popv3n(&v3n2) ; v3num_popv3n(&v3n1) ;
		if (v3num_cmp(&v3n1,&v3n2) >= 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_MTH_LESS:
		POPIJ ; if (i < j) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_MTH_LESS+1:
		v3num_popv3n(&v3n2) ; v3num_popv3n(&v3n1) ;
		if (v3num_cmp(&v3n1,&v3n2) < 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_MTH_LESSEQUAL:
		POPIJ ; if (i <= j) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_MTH_LESSEQUAL+1:
		v3num_popv3n(&v3n2) ; v3num_popv3n(&v3n1) ;
		if (v3num_cmp(&v3n1,&v3n2) <= 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_MTH_LOGE:
		xctu_popnum(&anum) ; dbl_temp1 = log(*anum.dbl_ptr) ;
		PUSHMP(&dbl_temp1) ; PUSHF(V3_FORMAT_FLOAT) ; break ;
	   case V3_XCT_MTH_LOG10:
		xctu_popnum(&anum) ; dbl_temp1 = log10(*anum.dbl_ptr) ;
		PUSHMP(&dbl_temp1) ; PUSHF(V3_FORMAT_FLOAT) ; break ;
	   case V3_XCT_MTH_MINUS:
		POPIJ ; PUSHINT(i-j) ; break ;
	   case V3_XCT_MTH_MINUS+1:
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;			/* Check for possible pointer difference */
		if (tmp_val.fld.type == VFT_POINTER)
		 { bp1 = xctu_popptr() ; bp2 = xctu_popptr() ; PUSHINT(bp2-bp1) ; break ; } ;
		v3np = (union val__v3num *)&v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(&v3n2) ;
		i |= v3num_popv3n(v3np) ;
		v3num_mdas(v3np,&v3n2,4) ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ; break ;
	   case V3_XCT_MTH_MINUSEQUAL:
		i = xctu_popint() ; POPF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_BINWORD)
		 { POPVP(word_ptr,(short int *)) ; *word_ptr -= i ; }
		 else { POPVP(base_ptr,(int *)) ; *base_ptr -= i ; } ;
		REPUSH ; break ;
	   case V3_XCT_MTH_MINUSEQUAL+1:
		v3np = &v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(&v3n2) ; i |= v3num_popv3n(v3np) ; REPUSH ;
		v3num_mdas(v3np,&v3n2,4) ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ;
		goto xct_update ;
	   case V3_XCT_MTH_MULTEQUAL:
		i = xctu_popint() ; break ;
	   case V3_XCT_MTH_MULTEQUAL+1:
		break ;
	   case V3_XCT_MTH_MULTIPLY:
		i = xctu_popint()*xctu_popint() ; PUSHINT(i) ; break ;
	   case V3_XCT_MTH_MULTIPLY+1:
		v3np = &v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(&v3n2) ; i |= v3num_popv3n(v3np) ;
		v3num_mdas(v3np,&v3n2,1) ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ; break ;
	   case V3_XCT_MTH_NOTEQUAL:
		POPIJ ; if (i != j) { PUSHTRUE ; } else { PUSHFALSE ; } ; break ;
	   case V3_XCT_MTH_NOTEQUAL+1:
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ; if (tmp_val.fld.type != VFT_POINTER) { tmp_val.all = NEXTSF ; } ;
		if (tmp_val.fld.type == VFT_POINTER)
		 { bp1 = xctu_popptr() ; bp2 = xctu_popptr() ;
		   if (bp1 != bp2) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		   break ;
		 } ;
		v3num_popv3n(&v3n2) ; v3num_popv3n(&v3n1) ;
		if (v3num_cmp(&v3n1,&v3n2) != 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
		break ;
	   case V3_XCT_MTH_PERCENT:
		v3np = &v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(&v3n2) ; i |= v3num_popv3n(v3np) ;
/*		See if second number is 0, if so then return 0 */
		if (v3n2.fp.marker == V3NUM_FPMARK ? v3n2.fp.dbl == 0 : v3n2.fd.big == 0 & v3n2.fd.num == 0 & v3n2.fd.dps == 0)
		 { v3np->fd.big = 0 ; v3np->fd.num = 0 ; v3np->fd.dps = 0 ; }
/*		 Multiply by 100 & divide to get percentage */
		 else { v3n1.fp.marker = V3NUM_FPMARK ; v3n1.fp.dbl = 1E2 ;
			v3num_mdas(v3np,&v3n1,1) ; v3num_mdas(v3np,&v3n2,2) ;
		      } ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ; break ;
	   case V3_XCT_MTH_PLUS:
		i = xctu_popint()+xctu_popint() ; PUSHINT(i) ; break ;
	   case V3_XCT_MTH_PLUS+1:
		v3np = &v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(&v3n2) ;
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;			/* Check for possible pointer difference */
		if (tmp_val.fld.type == VFT_POINTER)
		 { REPUSH ; i = xctu_popint() ; bp1 = xctu_popptr() ;
		   PUSHMP(bp1+i) ; PUSHF(V3_FORMAT_POINTER) ;
		   break ;
		 } ;
		i |= v3num_popv3n(v3np) ;
		v3num_mdas(v3np,&v3n2,3) ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ; break ;
	   case V3_XCT_MTH_PLUSEQUAL:
		i = xctu_popint() ; POPF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_BINWORD)
		 { POPVP(word_ptr,(short int *)) ; *word_ptr += i ; }
		 else { POPVP(base_ptr,(int *)) ; *base_ptr += i ; } ;
		REPUSH ; break ;
	   case V3_XCT_MTH_PLUSEQUAL+1:
		v3np = &v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(&v3n2) ; i |= v3num_popv3n(v3np) ; REPUSH ;
		v3num_mdas(v3np,&v3n2,3) ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ;
		goto xct_update ;
	   case V3_XCT_MTH_POWER:
		xctu_popnum(&anum) ; dbl_temp1 = *anum.dbl_ptr ;
		xctu_popnum(&anum) ; dbl_temp1 = pow(*anum.dbl_ptr,dbl_temp1) ;
		PUSHMP(&dbl_temp1) ; PUSHF(V3_FORMAT_FLOAT) ; break ;
	   case V3_XCT_MTH_SQRT:
		xctu_popnum(&anum) ;
		mthu_cvtatr(&anum,&dbl_temp1) ;
		dbl_temp1 = sqrt(dbl_temp1) ; PUSHMP(&dbl_temp1) ; PUSHF(V3_FORMAT_FLOAT) ; break ;
	   case V3_XCT_MTH_UNARYMINUS:
		i = xctu_popint() ; PUSHINT(-i) ; break ;
	   case V3_XCT_MTH_UNARYMINUS+1:
		v3np = (union val__v3num *)&v3num_list.buf[(v3num_list.count++) % V3_MTH_PD_LIST_MAX] ;
		i = v3num_popv3n(v3np) ;
		if (v3np->fp.marker == V3NUM_FPMARK) { v3np->fp.dbl = -v3np->fp.dbl ; }
		 else { v3np->fd.big = -v3np->fd.big ; v3np->fd.num = -v3np->fd.num ; v3np->fd.dps = -v3np->fd.dps ; } ;
		format.all = V3_FORMAT_V3NUM ; format.fld.decimals = i ;
		PUSHMP(v3np) ; PUSHF(format.all) ; break ;
	   case V3_XCT_VEC_ADD:		vec_add() ; break ;
	   case V3_XCT_VEC_ADD_CONSTANT: vec_add_constant() ; break ;
	   case V3_XCT_VEC_COPY:	vec_copy() ; break ;
	   case V3_XCT_VEC_FIND_CONSTANT: vec_find_constant() ; break ;
	   case V3_XCT_VEC_SET:		vec_set() ; break ;
	   case V3_XCT_VEC_SPAN:	vec_span() ; break ;
	   case V3_XCT_VEC_SUM:		vec_sum() ; break ;
	   case V3_XCT_VEC_SWAP:	vec_swap() ; break ;

/*	M O R E   V 3   O P E R A T I O N S			*/

	   case V3_XCT_SEG_CREATE:	iou_seg_create() ; break ;
	   case V3_XCT_SEG_LOCK:	iou_seg_lock() ; break ;
	   case V3_XCT_SEG_SHARE:	iou_seg_share() ; break ;
	   case V3_XCT_SEG_UNLOCK:	iou_seg_unlock() ; break ;
	   case V3_XCT_SEG_UPDATE:	iou_seg_update() ; break ;
	   case V3_XCT_STR_APPEND:	stru_append() ; break ;
	   case V3_XCT_STR_BE:		stru_be() ; break ;
	   case	V3_XCT_STR_BE2:		stru_be2() ; break ;
	   case V3_XCT_STR_BL:		stru_bl() ; break ;
	   case V3_XCT_STR_BREAK:	stru_break() ; break ;
	   case V3_XCT_STR_BREAK_RIGHT:	stru_break_right() ; break ;
	   case V3_XCT_STR_COMPARE:	stru_compare() ; break ;
	   case V3_XCT_STR_COMPRESS:	stru_compress() ; break ;
	   case V3_XCT_STR_CONCAT:	stru_concat() ; break ;
	   case V3_XCT_STR_EXPAND:	stru_expand() ; break ;
	   case V3_XCT_STR_HASH:
		stru_popstr(&strref) ; i = prsu_str_hash(strref.ptr,stru_sslen(&strref)) ;
		PUSHINT(i) ; break ;
	   case V3_XCT_STR_ITOS:	stru_itos() ; break ;
	   case V3_XCT_STR_LEN:		stru_len() ; break ;
	   case V3_XCT_STR_LIST:	stru_list() ; break ;
	   case V3_XCT_STR_LOGICAL:	stru_logical() ; break ;
	   case V3_XCT_STR_MATCH:	stru_match() ; break ;
	   case V3_XCT_STR_NULLS: stru_popstr(&strref) ; memset(strref.ptr,0,strref.format.fld.length) ; break ;
	   case V3_XCT_STR_PRIOR:	stru_prior() ; break ;
	   case V3_XCT_STR_SPACES: stru_popstr(&strref) ; memset(strref.ptr,' ',strref.format.fld.length) ; break ;
	   case V3_XCT_STR_STOI:	stru_popstr(&strref) ; PUSHINT(*strref.ptr) ; break ;
	   case V3_XCT_STR_TYPE:	stru_type() ; break ;
	   case V3_XCT_STR_TRIM:	stru_trim() ; break ;
	   case V3_XCT_STR_VALUE:
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type != VFT_INDIRECT)
		 v3_error(V3E_NOTSR,"STR","VALUE","NOTSR","Argument to str_value must be \"dcl sr\"",(void *)tmp_val.fld.type) ;
		POPVP(fw,(struct db__formatptr *)) ;
		PUSHMP(fw->ptr) ; PUSHF(fw->format.all) ; break ;
	   case V3_XCT_TTY_SCR_ANSI:
		bp1 = (char *)iou_scr_ansi() ; PUSHMP(bp1) ; PUSHF(V3_FORMAT_VARSTR) ; break ;
	   case V3_XCT_TTY_SCR_GET:
/*		See if we have optional mask */
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_BININT || tmp_val.fld.type == VFT_BINWORD)
		 { PUSHF(tmp_val.all) ; base_ptr = 0 ; }
		 else { PUSHF(tmp_val.all) ; base_ptr = (int *)stru_popcstr(tmpstr) ; } ;
/*		Get time, flags, and length */
		i = xctu_popint() ; j = xctu_popint() ; k = xctu_popint() ;
/*		Now call routine */
#ifdef VMSOS
		if ((i = vax_scr_get(ttystr,k,j,i,(char *)base_ptr)) < 0) { ttystr[i=0] = -1 ; ttystr[1] = NULLV ; } ;
#else
		if ((i = iou_scr_get(ttystr,k,j,i,(char *)base_ptr)) < 0) { ttystr[i=0] = -1 ; ttystr[1] = NULLV ; } ;
#endif
/*		Save terminator */
		strncpy(ttytrm,&ttystr[i],V3_TTY_TERMINATOR_MAX) ; ttystr[i] = NULLV ;
/*		And push string back onto stack */
		PUSHMP(&ttystr) ; PUSHF(V3_FORMAT_VARSTR) ; break ;
	   case	V3_XCT_TTY_TERMINATOR:
		PUSHMP(&ttytrm) ; PUSHF(V3_FORMAT_VARSTR) ; break ;
	   case V3_XCT_SCR_DISPLAY_ROWS:
		iou_scr_display_rows() ; break ;
	   case V3_XCT_SCR_SET_FIELD:
		i = xctu_popint() ;	/* Get /row.col,xxx/ or column ? */
		if (i < 500) { col = i ; row = xctu_popint() ; attr = xctu_popint() ; }
		 else { row = (i >> 24) ; col = 0xFF & (i >> 16) ; attr = (0xFFFF & i) ; } ;
		 stru_popstr(&strref) ; bp1 = strref.ptr ; j = stru_sslen(&strref) ;	/* bp1/j = ptr/len of text */
		 stru_popstr(&strref) ; bp2 = strref.ptr ;				/* bp2 = ptr to image */
		 stru_popstr(&strref) ; wp1 = (short int *)strref.ptr ;			/* wp1 = ptr to parameters */
		 bp2 += (row-1)*135 + col-1 ; for(i=j;(i--) > 0;) { *(bp2++) = *(bp1++) ; } ; /* Copy image */
		 wp1 += (row-1)*135 + col-1 ; *(wp1++) = 0X8000+attr ; for(i=j-1;(i--)>0;) { *(wp1++) = -1 ; } ;
		 break ;
	   case V3_XCT_SUBSCRIPT:
/*		First pop off the symbol to be subscripted */
		POPF(sssym_format.all) ; POPVP(bp1,(char *)) ;
		sslp = 0 ;
ss_entry:
/*		Now pop off all subscripts */
		for (ac=0;ac<V3_PRS_SUBSCRIPT_DIM_MAX;ac++)
		 { POPF(tmp_val.all) ;
		   if (tmp_val.all == V3_FORMAT_EOA) { POPVI(i) ; break ; } ;
/*		   Append next subscript to sslist */
		   PUSHF(tmp_val.all) ;
		   if ((sslist[ac] = xctu_popint()) <= 0)
		    v3_error(V3E_SUBLEZERO,"XCT","OPERATIONS","SUBLEZERO","Subscript cannot be less than 1",(void *)sslist[ac]) ;
		 } ;
/*		Now calculate offset */
		if (psi->pure_base != NULLV)
		 { if (sslp == 0) sslp = (struct db__dim_info *)(psi->pure_base + *(psi->code_ptr++)) ; }
		 else { if (sslp == 0) sslp = (struct db__dim_info *)(process->package_ptrs[psi->package_index]->pure_ptr + *(psi->code_ptr++)) ; } ;
		offset = 0 ;
		for (i=0;i<sslp->count;i++)
		 { offset += sslp->dimlen[i]*(sslist[--ac]-1) ;
		   if (ac <= 0) break ;
		 } ;
/*		Finally push adjusted element onto stack */
		bp1 += offset ; PUSHMP(bp1) ; PUSHF(sssym_format.all) ;
		break ;
	   case V3_XCT_SUBSCRIPT_OBJ:	/* Here for subscripting of objects */
		POPF(sssym_format.all) ; POPVP(bp1,(char *)) ;
/*		Next symbol on stack points to subscripting info */
		POPF(i) ; POPVP(sslp,(struct db__dim_info *)) ; goto ss_entry ;
	   case V3_XCT_V3_CLEAR:
		POPF(tmp_val.all) ;
		switch (tmp_val.fld.type)
		 { case VFT_BININT:
			POPVP(base_ptr,(int *)) ; *base_ptr = 0 ; break ;
		   case VFT_BINLONG:
			POPVP(long_ptr,(LONGINT *)) ; *long_ptr = 0 ; break ;
		   case VFT_BINWORD:
			POPVP(word_ptr,(short int *)) ; *word_ptr = 0 ; break ;
		   case VFT_FLOAT:
			POPVP(dbl_ptr,(double *)) ; *dbl_ptr = 0.0 ; break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			POPVP(str_ptr,(char *)) ;
			for(i=0;i<tmp_val.fld.length;i++) { *(str_ptr++) = ' ' ; } ;
			break ;
		   case VFT_VARSTR:
			POPVP(str_ptr,(char *)) ; *str_ptr = NULLV ; break ;
		   case VFT_STRINT:
			POPVP(str_ptr,(char *)) ;
			mthu_cvtis((LONGINT)0,0,str_ptr,tmp_val.fld.length,0) ; break ;
		   case VFT_PACDEC:
			POPVP(str_ptr,(char *)) ;
			mthu_cvtip(0,0,str_ptr,tmp_val.fld.length,0) ; break ;
		 } ;
		REPUSH ; break ;
	   case V3_XCT_V3_CLONE:
		POPF(j) ; POPVP(bp1,(char *)) ;  /* pointer to slave */
		POPF(tmp_val.all) ; tmp_val.fld.mode = VFM_PTR ; POPVI(k) ;	/* Get & tweak format */
		PUSHMP(bp1) ; PUSHF(tmp_val.all) ;			/* Push slave location & master format */
		break ;
	   case V3_XCT_V3_COPY:
		POPF(tmp_val.all) ; j = tmp_val.fld.length ; POPVP(bp1,(char *)) ;
		POPF(tmp_val.all) ; j = (j <= tmp_val.fld.length ? j : tmp_val.fld.length) ; POPVP(bp2,(char *)) ;
		memcpy(bp2,bp1,j) ;				/* Just copy from second to first */
		break ;
	   case V3_XCT_V3_COPYSWAP:
		POPF(tmp_val.all) ; j = tmp_val.fld.length ; POPVP(up1,(char *)) ; i = tmp_val.fld.type ;
		POPF(tmp_val.all) ; j = (j <= tmp_val.fld.length ? j : tmp_val.fld.length) ; POPVP(up2,(char *)) ;
		if (i != tmp_val.fld.type)
		 v3_error(V3E_BADINFOARG,"V3","INFO","BADINFOARG","Both arguments to v3_copyswap must be same type",NULLV) ;
		switch (tmp_val.fld.type)
		 { default:		v3_error(V3E_BADINFOARG,"V3","INFO","BADINFOARG","Invalid V3 datatype for swap",NULLV) ;
		   case VFT_FIXSTR:
		   case VFT_VARSTR:
		   case VFT_STRINT:	memcpy(up2,up1,j) ; break ;
		   case VFT_BININT:	{ int *li1=(int *)up1, *li2=(int *)up2 ; FLIPLONG((*li2),(*li1)) ; } break ;
		   case VFT_BINWORD:	{ short int *si1=(short *)up1, *si2=(short *)up2 ; FLIPSHORT((*si2),(*si1)) ; } break ;

		 }
		break ;
	   case V3_XCT_V3_INFO:
/*		Pop off args until EOA				*/
		for (i=0;;i++)
		 { POPF(tmp_val.all) ; POPVI(j) ;
		   if (tmp_val.all == V3_FORMAT_EOA) break ;
		 } ; bsp = psi->stack_ptr ; REPUSH ; REPUSH ;
		switch (xctu_popint())
		 { default: v3_error(V3E_BADINFOARG,"V3","INFO","BADINFOARG","Invalid argument to V3_INFO",NULLV) ;
		   case 50:
			PUSHMP(&V3_OPCODE_USAGE) ; PUSHF(V3_FORMAT_POINTER) ; goto info_end ;
		   case V3_FLAGS_V3_ASSERTION_CHANGES:
			i = process->assertion_changes ; break ;
		   case V3_FLAGS_V3_ASTERISK_COUNT:
			i = process->asterisk_count ; break ;
		   case V3_FLAGS_V3_CHECKSUM_TABLE:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; i = xctu_popint() ;
			if ((pckp = process->package_ptrs[i]) == NULLV)
			 v3_error(V3E_PCKNOTLOADED,"V3","INFO","PCKNOTLOADED","Package not loaded",NULLV) ;
			psi->stack_ptr = bsp ; PUSHMP(pckp->checksum_ptr) ; PUSHF(V3_FORMAT_POINTER) ; goto info_end ;
		   case V3_FLAGS_V3_COUNTRY_INFO:
		   	psi->stack_ptr = bsp ;
		   	PUSHMP(&process->ci) ; PUSHF(V3_FORMAT_POINTER) ; goto info_end ;
		   case V3_FLAGS_V3_CURRENT_PACKAGE:
			i = psi->package_index*1000+V3_PACKAGE_MAX ; break ;
		   case V3_FLAGS_V3_DATA_TYPE:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; POPF(tmp_val.all) ;
			i = tmp_val.fld.type ; break ;
		   case V3_FLAGS_V3_DECIMAL_PLACES:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; POPF(tmp_val.all) ;
			i = tmp_val.fld.decimals ; break ;
		   case V3_FLAGS_V3_ERROR:
			i = process->last_errnum ; break ;
		   case V3_FLAGS_V3_MAX_LENGTH:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; POPF(tmp_val.all) ;
			i = tmp_val.fld.length ; break ;
		   case V3_FLAGS_V3_OBJECT_PACKAGE:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; POPF(tmp_val.all) ;
			if (tmp_val.fld.type == VFT_OBJECT)
			 { POPVI(i) ; sob.all = tmp_val.all ; PUSHINT(sob.fld.package_id) ; break ; } ;
			if (tmp_val.fld.type != VFT_OBJREF)
			 v3_error(V3E_NOTOBJ,"OBJ","PACKAGE","NOTOBJ","Argument to OBJ_xxx not object/objref",NULLV) ;
			POPVP(fw,(struct db__formatptr *)) ; sob.all = fw->format.all ;
			i = sob.fld.package_id ; break ;
		   case	V3_FLAGS_V3_OPEN_FILES:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; bp1 = xctu_popptr() ;
/*			If arg is zero then search for first, otherwise for next after arg */
			if (bp1 == NULLV)
			 { for(j=0;j<ounits.count;j++) { if (ounits.unit[j].unitp != NULLV) goto got_unit ; } ;
			   j = -1 ; goto got_unit ;	/* No more open units */
			 } ;
/*			Here to search for "next" one */
			for(j=0;j<ounits.count;j++) { if (ounits.unit[j].unitp == (struct iou__unit *)bp1) break ; } ;
			for(j++;j<ounits.count;j++) { if (ounits.unit[j].unitp != NULLV) goto got_unit ; } ;
			j = -1 ; goto got_unit ;
got_unit:		psi->stack_ptr = bsp ;
			if (j < 0) { PUSHMP(NULLV) ; } else { PUSHMP(ounits.unit[j].unitp) ; } ;
			PUSHF(V3_FORMAT_POINTER) ; goto info_end ;
		   case V3_FLAGS_V3_PACKAGE_FILENAME:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; i = xctu_popint() ;
			if ((pckp = process->package_ptrs[i]) == NULLV)
			 v3_error(V3E_PCKNOTLOADED,"V3","INFO","PCKNOTLOADED","Package not loaded",NULLV) ;
			bp1 = pckp->file_name ; goto info_str ;
		   case V3_FLAGS_V3_PACKAGE_ID:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; i = xctu_popint() ;
			if ((pckp = process->package_ptrs[i]) == NULLV)
			 v3_error(V3E_PCKNOTLOADED,"V3","INFO","PCKNOTLOADED","Package not loaded",NULLV) ;
			if (pckp->id_obj.all == NULLV)
			 v3_error(V3E_NOIDOBJ,"V3","INFO","NOIDOBJ","This package has no DCL PACKAGE_ID object",NULLV) ;
			psi->stack_ptr = bsp ; PUSHVI(NULLV) ; PUSHF(pckp->id_obj.all) ; goto info_end ;
		   case V3_FLAGS_V3_PACKAGE_LOADED:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; i = xctu_popint() ;
			if (i < 0 || i >= V3_PACKAGE_MAX) { i = 0 ; break ; } ;
			psi->stack_ptr = bsp ; PUSHMP(process->package_ptrs[i]) ; PUSHF(V3_FORMAT_POINTER) ; goto info_end ;
		   case V3_FLAGS_V3_PACKAGE_NAME:
			psi->stack_ptr = bsp-(3*SIZEOFSTACKENTRY) ; i = xctu_popint() ;
			if ((pckp = process->package_ptrs[i]) == NULLV)
			 v3_error(V3E_PCKNOTLOADED,"V3","INFO","PCKNOTLOADED","Package not loaded",NULLV) ;
			bp1 = pckp->package_name ; goto info_str ;
		   case V3_FLAGS_V3_PARSE_ERRORS:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; i = xctu_popint() ;
/*			If argument is 0 then return total process parser error count */
			if (i == 0) { i = process->parser_errors ; break ; } ;
			if ((pckp = process->package_ptrs[i]) == NULLV)
			 v3_error(V3E_PCKNOTLOADED,"V3","INFO","PCKNOTLOADED","Package not loaded",NULLV) ;
			i = (pckp->parse_ptr == NULLV ? -1 : pckp->parse_ptr->error_cnt) ; break ;
		   case V3_FLAGS_V3_STARTUP_OPTIONS:
			bp1 = process->v3_startup_option_list ; goto info_str ;
		   case V3_FLAGS_V3_VERSION:
			i = V3_VERSION_MAJOR*1000+V3_VERSION_MINOR ; break ;
		 } ;
		psi->stack_ptr = bsp ; PUSHINT(i) ; break ;
/*		Here to return string pointed to by i */
info_str:
		psi->stack_ptr = bsp ; PUSHMP(bp1) ; PUSHF(V3_FORMAT_VARSTR) ;
info_end:	break ;
	   case V3_XCT_V3_MATCH:
		bsp = psi->stack_ptr ; POPF(i) ; POPVI(i) ;
		POPF(tmp_val.all) ; /* Pop off intern type of base string/number */
		switch (tmp_val.fld.type)
		 { case VFT_BINWORD:
		   case VFT_BINLONG:
		   case VFT_BININT:
		   case VFT_FLOAT:
		   case VFT_STRINT:
		   case VFT_PACDEC:
			PUSHF(tmp_val.all) ; v3num_popv3n(&v3n2) ; v3num_popv3n(&v3n1) ;
			switch (v3num_cmp(&v3n1,&v3n2))
			 { case -1:	PUSHINT(-1) ; break ;
			   case 0:	PUSHTRUE ; break ;
			   case 1:	PUSHINT(-2) ; break ;
			 } ;
/*			if (v3num_cmp(&v3n1,&v3n2) == 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
*/
			break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
		   case VFT_VARSTR:
			psi->stack_ptr = bsp ; stru_match() ; break ;
		 } ;
		break ;
	   case V3_XCT_V3_ARG_NUM:
		PUSHINT(psi->arg_cnt) ; break ;
	   case V3_XCT_V3_ARG_VALUE:
		j = xctu_popint() ;
/*		Allow for single value or range of the form: first::last */
		if ((j & 0xFFFF0000) != 0) { k = (j & 0xFFFF) ; j >>= 16 ; }
		 else { k = j ; } ;
		if (j < 1 || k > psi->arg_cnt)
		 v3_error(V3E_BADMODARG,"MOD","ARG_VALUE","BADMODARG","Argument to MOD_ARG_VALUE is out of range",(void *)psi->arg_cnt) ;
		for(i=j;i<=k;i++)
		 { PUSHVP(*(psi->arg_ptr-2*i+3)) ; PUSHF(*(psi->arg_ptr-(2*(i-1)))) ; } ;
		break ;
	   case	V3_XCT_V3_ERROR:
/*		Pop off results */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_BINWORD || tmp_val.fld.type == VFT_BINLONG || tmp_val.fld.type == VFT_BININT)
		 { i = xctu_popint() ; } else { i = 0 ; } ;
		stru_popcstr(tmpstr) ; stru_popcstr(tmpstr1) ; stru_popcstr(errstr2) ; stru_popcstr(errstr1) ;
/*		Now call v3_error */
		v3_error(i,errstr1,errstr2,tmpstr1,tmpstr,0) ;
	   case V3_XCT_V3_SET_PARAMETER:
		for (i=0;;i++) { POPF(tmp_val.all) ; POPVI(j) ; if (tmp_val.all == V3_FORMAT_EOA) break ; } ;
		bsp = psi->stack_ptr ; REPUSH ; REPUSH ;
		switch (xctu_popint())
		 { default: v3_error(V3E_BADSETPARAMARG,"V3","SET_PARAMETER","BADSETPARAMARG","Invalid argument to V3_SET_PARAMETER",NULLV) ;
		   case V3_FLAGS_V3_SET_DATE:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; v3_param_date = xctu_popint() ; break ;
		   case V3_FLAGS_V3_SET_JOBID:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; v4mm_SetUserJobId(xctu_popint(),NULL) ; break ;
		   case V3_FLAGS_V3_SET_TIME_OF_DAY:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; v3_param_time = xctu_popint() ; break ;
		   case V3_FLAGS_V3_SET_UNDEF_MODULE:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ; stru_popcstr(tmpstr) ;
			strcpy(process->v4_eval_on_undefmod,tmpstr) ; break ;
		   case V3_FLAGS_V3_SET_WATCH_LOCATION:
			psi->stack_ptr = bsp - (3*SIZEOFSTACKENTRY) ;
			watch_location = (int *)xctu_popptr() ; watch_value = *watch_location ; break ;
		 } ;
		psi->stack_ptr = bsp ; break ;
	   case V3_XCT_STRUCTPTR:
		POPF(i) ; POPVI(j) ; POPF(k) ; POPVP(mp_ptr,(int *(*))) ;
		if (*mp_ptr == NULL)
		 v3_error(V3E_PTRZERO,"XCT","STRUCTPTR","PTRZERO","Pointer cannot be equal to 0",(void *)0) ;
		PUSHMP((char *)(*mp_ptr) + j) ; PUSHF(i) ;
		break ;
	   case V3_XCT_HEAP_ALLOC:
		if ((i = xctu_popint()) <= 0)
		 v3_error(V3E_INVHEAPSIZE,"HEAP","ALLOC","INVHEAPSIZE","Argument to HEAP_ALLOC less than 1",(void *)i) ;
/*		Maybe increase allocation by "fudge" factor */
		if (i >= ALLOCATE_MIN) i += ALLOCATE_EXTRA ;
		if ((base_ptr = (int *)v4mm_AllocChunk(i,TRUE)) == 0)
		 v3_error(V3E_NOMOREMEM,"HEAP","ALLOC","NOMOREMEM","Virtual memory mapping or paging quota exceeded",(void *)0) ;
		PUSHMP(base_ptr) ; PUSHF(V3_FORMAT_POINTER) ; break ;
	   case V3_XCT_HEAP_FREE:
		bp1 = xctu_popptr() ;
		v4mm_FreeChunk(bp1) ;
		break ;
	   case V3_XCT_ADDRESSSKEL:
	   case V3_XCT_ADDRESS:
		POPF(tmp_val.all) ;
		switch (tmp_val.fld.type)
		 { case VFT_STRUCTPTR: POPVP(mp_ptr,(int *(*))) ; PUSHMP(*mp_ptr) ; break ;
		   case VFT_FIXSTRSK:
			if (code == V3_XCT_ADDRESS) v3_error(V3E_INVSYSADDRESSDT,"SYS","ADDRESS","INVSYSADDRESSDT","Cannot sys_address() a skeleton!",0) ;
		   default: POPVP(bp1,(char *)) ; PUSHMP(bp1) ; break ;
		   case VFT_OBJREF:
		   case VFT_INDIRECT:
			POPVP(fw,(struct db__formatptr *)) ; PUSHMP(fw->ptr) ; break ;
		 } ;
		PUSHF(V3_FORMAT_POINTER) ; break ;
	   case	V3_XCT_PARTIAL:
/*		Get address of top-of-stack & assign as value of second */
		POPF(tmp_val.all) ; POPVP(base_ptr,(int *)) ;
		POPF(tmp_val.all) ; POPVP(mp_ptr,(int *(*))) ;
		*mp_ptr = base_ptr ; break ;
	   case V3_XCT_PARTIAL_SKELETON:
/*		Same as above except have to get base skeleton offset to make adjustment */
		POPF(tmp_val.all) ; POPVP(bp1,(char *)) ; POPF(tmp_val.all) ; POPVP(mp_ptr,(int *(*))) ;
		pckp = process->package_ptrs[psi->package_index] ;
		if (psi->pure_base != NULLV)
		 { skelref = (struct db__skeleton_ref *)(psi->pure_base + *(psi->code_ptr++)) ; }
		 else { skelref = (struct db__skeleton_ref *)(pckp->pure_ptr + *(psi->code_ptr++)) ; } ;
/*		If referenced top level of skeleton then no adjustment, otherwise have to subtract out offset */
		if (skelref->el_index == -1) { *mp_ptr = (int *)bp1 ; break ; } ;
		pckskel = pckp->skeleton_ptr ; skel = (struct db__prs_skeleton *)&(pckskel->buf[pckskel->index[skelref->skel_index]]) ;
		*mp_ptr = (int *)(bp1 - skel->el[skelref->el_index].offset) ; break ;
	   case V3_XCT_RPT_COLWIDTH:		i = rptu_colwidth() ; PUSHINT(i) ; break ;
	   case V3_XCT_OBJ_BIND_SKELETON:	sobu_bind_skeleton() ; break ;
	   case V3_XCT_OBJ_FIND:
/*		See if second argument is name literal (i.e. BININT) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_BININT)
		 { if ((i = sobu_object_find(xctu_popint())) == NULLV)
		    { POPF(i) ; POPVI(i) ; PUSHFALSE ; break ; } ;
		   goto obj_find_found ;
		 } ;
		stru_popcstr(tmpstr) ;
/*		Convert to upper case */
		for (i=0;tmpstr[i] != 0;i++)
		 { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ; } ;
		if ((i = sobu_object_dereference(NULLV,tmpstr)) == NULLV)
		 { POPF(i) ; POPVI(i) ; PUSHFALSE ; break ; } ;
/*		Found the object - update first argument */
obj_find_found:
		POPF(tmp_val.all) ; POPVP(fw,(struct db__formatptr *)) ;
		if (tmp_val.fld.type != VFT_OBJREF)
		 v3_error(V3E_NOTOBJREF,"OBJ","FIND","NOTOBJREF","First argument to OBJ_FIND/OBJ_HAS_FIND must be OBJREF",NULLV) ;
		fw->format.all = i ;		/* Update with object id */
		fw->ptr = NULLV ; 	/* Zap object pointer to avoid any complications */
		PUSHTRUE ;
		break ;
	   case V3_XCT_OBJ_HAS_FIND:
/*		See if third argument is name literal (i.e. BININT) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_BININT)
		 { j = xctu_popint() ; }
		 else
		 { j = 0 ; stru_popcstr(tmpstr) ;
/*		   Convert to upper case */
		   for (i=0;tmpstr[i] != 0;i++)
		    { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ; } ;
		 } ;
/*		Now pop off the object reference */
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_OBJREF)
		 { POPVP(fw,(struct db__formatptr *)) ; i = fw->format.all ; }
		 else
		 { if (tmp_val.fld.type != VFT_OBJECT)
		    v3_error(V3E_NOTOBJ,"OBJ","HAS_FIND","NOTOBJ","Argument to OBJ_xxx not object/objref",NULLV) ;
		   POPVI(i) ; i = tmp_val.all ;
		 } ;
		if ((i = ( j == 0 ? sobu_object_dereference(i,tmpstr) : sobu_has_find(i,j))) == NULLV)
		 { POPF(i) ; POPVI(i) ; PUSHFALSE ; }
		 else
/*		   Found the object - update first argument */
		 { POPF(tmp_val.all) ; POPVP(fw,(struct db__formatptr *)) ;
		   if (tmp_val.fld.type != VFT_OBJREF)
		    v3_error(V3E_NOTOBJREF,"OBJ","HAS_FIND","NOTOBJREF","First argument to OBJ_FIND/OBJ_HAS_FIND must be OBJREF",NULLV) ;
		   fw->format.all= i ; fw->ptr = NULLV ; 	/* Update with object id */
		   PUSHTRUE ;
		 } ;
		break ;
	   case V3_XCT_OBJ_ID:
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_OBJECT)
		 { POPVI(i) ; PUSHINT(tmp_val.all) ; break ; } ;
		if (tmp_val.fld.type != VFT_OBJREF)
		 v3_error(V3E_NOTOBJ,"OBJ","ID","NOTOBJ","Argument to OBJ_xxx not object/objref",NULLV) ;
		POPVP(base_ptr,(int *)) ; PUSHINT(*base_ptr) ; break ;
	   case V3_XCT_OBJ_NAME:
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_OBJREF)
		 { POPVP(fw,(struct db__formatptr *)) ; tmp_val.all = fw->format.all ; }
		 else { POPVI(i) ; } ;
		base_ptr = (int *)sobu_name_str(tmp_val.all) ;	/* Find the name */
		PUSHMP(base_ptr) ; PUSHF(V3_FORMAT_VARSTR) ; break ;
	   case V3_XCT_OBJ_VALUE:
		POPF(tmp_val.all) ;
/*		See if given optional value index */
		if (tmp_val.fld.type == VFT_BININT || tmp_val.fld.type == VFT_BINWORD)
		 { PUSHF(tmp_val.all) ; ac = xctu_popint() ; POPF(tmp_val.all) ; }
		 else ac = 0 ;
		if (tmp_val.fld.type == VFT_OBJECT)
		 { POPVP(base_ptr,(int *)) ; POPFV ;
		   sobu_push_val(tmp_val.all,ac,(char *)base_ptr,NULLV,FALSE,FALSE) ; break ; } ;
/*		Best have an OBJREF */
		if (tmp_val.fld.type != VFT_OBJREF)
		 v3_error(V3E_NOTOBJ,"OBJ","VALUE","NOTOBJ","Argument to OBJ_xxx not object/objref",NULLV) ;
		POPVP(fw,(struct db__formatptr *)) ;
		POPFV ;			 /* Pop EOA off of stack */
		sobu_push_val(fw->format.all,ac,fw->ptr,NULLV,FALSE,FALSE) ; break ;
	   case V3_XCT_OBJ_VALUE_NUM:
		POPF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_OBJECT)
		 { POPVI(i) ; PUSHINT(sobu_val_num(tmp_val.all)) ; }
		 else
		 { if (tmp_val.fld.type != VFT_OBJREF)
		    v3_error(V3E_NOTOBJ,"OBJ","VALUE_NUM","NOTOBJ","Argument to OBJ_xxx not object/objref",NULLV) ;
		   POPVP(fw,(struct db__formatptr *)) ; PUSHINT(sobu_val_num(fw->format.all)) ;
		 } ;
		break ;
	   case V3_XCT_XTI_ALPHA:	i = xti_alpha(TRUE) ; PUSHINT(i) ; break ;
	   case V3_XCT_XTI_DATE:	i = xti_date() ; PUSHINT(i) ; break ;
	   case V3_XCT_XTI_DATE_TIME:	i = xti_date_time() ; PUSHINT(i) ; break ;
	   case V3_XCT_XTI_GEN:		xti_gen() ; break ;
	   case V3_XCT_XTI_INT:		i = xti_int() ; PUSHINT(i) ; break ;
	   case V3_XCT_XTI_LOGICAL:	i = xti_logical() ; PUSHINT(i) ; break ;
	   case V3_XCT_XTI_MONTH:	i = xti_month() ; PUSHINT(i) ; break ;
	   case V3_XCT_XTI_POINTER:	i = xti_pointer() ; PUSHINT(i) ; break ;
	   case V3_XCT_XTI_TIME:	i = xti_time() ; PUSHINT(i) ; break ;
	 } ;
	return(TRUE) ;
}
