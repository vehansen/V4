/*	V3PRSA.C - PARSING UTILITIES FOR VICTIM II

	LAST EDITED 6/29/84 BY VICTOR E. HANSEN	*/

#include <time.h>
#include "v3defs.c"

/*	Global Definitions for Parser		*/
extern struct db__parse_info *parse ;
extern struct db__tknres tkn ;
extern jmp_buf parse_exit ;
extern char type_of_token[] ;

/*	Global References for Execution		*/
extern struct db__process *process ;
extern struct db__psi *psi ;
extern struct prsu__v4b v4b ;

/*	P A R S E   L I T E R A L   R O U T I N E S		*/

/*	prsu_stash_lit - stashes current literal (if necessary) in literal
	table & returns byte offset (in pure_buf) to it	*/
prsu_stash_lit(obflag)
  int obflag ;
{ int hx,i,j,align_mask ;
   struct db__lit_table *ltbl ;
   struct literal {
    int format ;		/* Format of literal value in buffer */
    union val__mempos litval ;
   } *litp ;

/*	Figure if we are looking at objects or modules/others */
	if (obflag)
	 { ltbl = &parse->oblit ;
	 } else
	 { ltbl = &parse->lit ;
	 } ;
/*	First make up a good starting hash address */
	if (tkn.lit_format.fld.type == VFT_BININT || tkn.lit_format.fld.type == VFT_BINLONG)
	 { hx = tkn.value.int_lit % V3_PRS_LITERAL_MAX ; }
	 else { hx = prsu_str_hash(tkn.value.char_lit,tkn.literal_len) % V3_PRS_LITERAL_MAX ; } ;
	if (hx < 0) hx = -hx ;
/*	Scan existing literals to avoid duplicates */
	for (;hx<V3_PRS_LITERAL_MAX-2;hx++)
	 { if ((j = ltbl->offsets[hx]) == 0) break ;	/* Don't have this literal */
	   if (obflag) { litp = (struct literal *)&parse->ob_bucket.buf[j] ; }
	    else { litp = (struct literal *)&parse->pure_buf[j] ; } ;
	   if (tkn.lit_format.all != litp->format) continue ;
/*	   If literal is immediate mode then check as integer */
	   if (tkn.lit_format.fld.mode == VFM_IMMEDIATE)
	    { if (litp->litval.all == tkn.value.int_lit) return(j) ;
	      continue ;
	    } ;
/*	    Literal is a string - find it in pure buffer space & compare */
	   if (obflag)
	    { if (strncmp(tkn.value.char_lit,(char *)&(parse->ob_bucket.buf[litp->litval.statpure.offset]),tkn.literal_len) == 0)
		return(j) ;	/* Found existing literal */
	    } else
	    { if (strncmp(tkn.value.char_lit,(char *)&(parse->pure_buf[litp->litval.statpure.offset]),tkn.literal_len) == 0)
		return(j) ;	/* Found existing literal */
	    } ;
	 } ;
/*	If here then literal not found - have to append it */
	if (ltbl->count >= V3_PRS_LITERAL_MAX)
	 prsu_error(ABORT,"TOOMANYLITS","Exceeded maximum number of literals in package") ;
	 switch (tkn.lit_format.fld.type)
	  { default: align_mask = ALIGN_BYTE ; break ;		/* Determine address alignment */
	    case VFT_BINLONG:	align_mask = ALIGN_LONG ; break ;
	    case VFT_BININT:	align_mask = ALIGN_WORD ; break ;
	    case VFT_MODREF:
	    case VFT_OBJREF:	align_mask = ALIGNofPTR ; break ;
	    case VFT_FLOAT:	align_mask = ALIGN_DOUBLE ; break ;
	  } ;
/*	If literal is immed_value then stash */
	if (tkn.lit_format.fld.mode == VFM_IMMEDIATE)
	 { i = (obflag ? prsu_alloc_ob(sizeof *litp,align_mask) : prsu_alloc_pure(sizeof *litp,align_mask)) ;
	   litp = (struct literal *)(obflag ? &parse->ob_bucket.buf[i] : &parse->pure_buf[i]) ;
	   litp->format = tkn.lit_format.all ; litp->litval.all = tkn.value.int_lit ;
	 }
	 else	/* If literal is a string the allocate in pure */
	 { if (obflag)
	    { i = prsu_alloc_ob(sizeof *litp,ALIGN_WORD) ; litp = (struct literal *)&parse->ob_bucket.buf[i] ;
	      litp->format = tkn.lit_format.all ;
/*	      Construct a statpure pointer to literal */
	      j = prsu_alloc_ob(tkn.literal_len>0 ? tkn.literal_len : 1,align_mask) ;
	      litp->litval.all = prsu_valref(VAL_STATOB,j) ;
/*	      And finally copy string into bytes following 2-word descriptor */
	      if (tkn.literal_len > 0)
	       { strncpy((char *)&(parse->ob_bucket.buf[j]),tkn.value.char_lit,tkn.literal_len) ; }
	       else parse->ob_bucket.buf[j] = 0 ;
	    } else 
	    { i = prsu_alloc_pure(sizeof *litp,ALIGN_WORD) ; litp = (struct literal *)&parse->pure_buf[i] ;
	      litp->format = tkn.lit_format.all ;
/*	      Construct a statpure pointer to literal */
	      j = prsu_alloc_pure(tkn.literal_len>0 ? tkn.literal_len : 1,align_mask) ;
	      litp->litval.all = prsu_valref(VAL_STATPURE,j) ;
/*	      And finally copy string into bytes following 2-word descriptor */
	      if (tkn.literal_len > 0)
	       { strncpy((char *)&(parse->pure_buf[j]),tkn.value.char_lit,tkn.literal_len) ; }
	       else parse->pure_buf[j] = 0 ;
	    } ;
	 } ;
/*	Update literal table so we can keep track of duplicate literals */
	ltbl->offsets[hx] = i ; (ltbl->count)++ ;
	return(i) ;
}

/*	prsu_stash_pure - Stashes string in pure buffer space 	*/
prsu_stash_pure(strptr,len)
  char *strptr ;		/* Pointer to string to save */
  int len ;			/* Length of the string */
{ int i,end ;

/*	Figure how much to search xxxx - s/b parameter */
	end = parse->pure_free-len-200 ; if (end < 0) end = 0 ;
/*	Start search backwards for this string */
	i = (parse->pure_free-((len+SIZEofINT-1)/SIZEofINT)) & 0xFFFFFFFC ;
	for (;i>=end;i--)
	 { if (memcmp(strptr,&parse->pure_buf[i],len) == 0) return(i) ; } ;
/*	Can't find it - append to pure */
skip_it:
	i = prsu_alloc_pure(len,ALIGN_WORD) ; memcpy(&parse->pure_buf[i],strptr,len) ;
	return(i) ;
}

/*	prsu_stash_ob - Stashes string in object buffer space 	*/
prsu_stash_ob(strptr,len)
  char *strptr ;		/* Pointer to string to save */
  int len ;			/* Length of the string */
{ int i,end ;
/*	Figure how much to search xxxx - s/b parameter */
	end = parse->ob_bucket.free_offset-len-200 ; if (end < 0) end = 0 ;
/*	Start search backwards for this string */
	i = (parse->ob_bucket.free_offset-((len+SIZEofINT-1)/SIZEofINT)) & 0xFFFFFFFC ;
	for (;i>=end;i--)
	 { if (memcmp(strptr,&parse->ob_bucket.buf[i],len) == 0) return(i) ; } ;
/*	Can't find it - append to object space */
skip_it:
	i = prsu_alloc_ob(len,ALIGN_WORD) ; memcpy(&parse->ob_bucket.buf[i],strptr,len) ;
	return(i) ;
}

/*	A L L O C A T I O N   R O U T I N E S   F O R   P U R E  /  I M P U R E		*/

/*	prsu_alloc_impure - Allocates bytes in impure_buf & returns offset to start */
/*	NOTE: NOW RETURNS OFFSET WITH RESPECT TO (INT *) (i.e. to longword, not byte) */
prsu_alloc_impure(bytes,alignment)
  int bytes ;			/* Number of bytes to allocate */
  int alignment ;		/* Byte address alignment */
{ int offset ;		/* Offset to begin */
   int alloc ;		/* Number of bytes to allocate */

	alloc = bytes + (bytes >= ALLOCATE_MIN ? ALLOCATE_EXTRA : 0) ;
	if (alloc + parse->impure_free*SIZEofINT >= V3_PRS_IMPURE_BUF_SIZE)
	 prsu_error(ABORT,"MAXIMP","Exceeded maximum number of bytes in impure buffer space for package") ;
/*	Allocate on proper boudary */
	alignment = alignment >>2 ;		/* Shift two because implicit int alignment */
	offset = (parse->impure_free+alignment) & ~alignment ;
	parse->impure_free = offset + (alloc+SIZEofINT)/SIZEofINT ;
	return(offset) ;
}

/*	prsu_alloc_impurelm - Allocates bytes in impurelm_buf & returns offset to start */

prsu_alloc_impurelm(bytes,alignment)
  int bytes ;			/* Number of bytes to allocate */
  int alignment ;		/* Byte address alignment */
{ int offset ;		/* Offset to begin */
   int alloc ;		/* Number of bytes to allocate */

	alloc = bytes + (bytes >= ALLOCATE_MIN ? ALLOCATE_EXTRA : 0) ;
	if (alloc + parse->impurelm_free*SIZEofINT >= V3_PRS_IMPURE_BUF_SIZE)
	 prsu_error(ABORT,"MAXIMP","Exceeded maximum number of bytes in impurelm buffer space for package") ;
/*	Allocate on proper boudary */
	alignment = alignment >>2 ;		/* Shift two because implicit int alignment */
	offset = (parse->impurelm_free+alignment) & ~alignment ;
	parse->impurelm_free = offset + (alloc+SIZEofINT)/SIZEofINT ;
	return(offset) ;
}

/*	prsu_alloc_pure - Allocates bytes in pure_buf & returns offset to start */
/*	NOTE: NOW RETURNS OFFSET WITH RESPECT TO (INT *) (i.e. to longword, not byte) */
prsu_alloc_pure(bytes,alignment)
  int bytes ;			/* Number of bytes to allocate */
  int alignment ;
{ int offset ;		/* Offset to begin */

	if ((parse->pure_free*SIZEofINT) + bytes >= V3_PRS_PURE_BUF_SIZE)
	 prsu_error(ABORT,"MAXPURE","Exceeded maximum number of bytes in pure buffer space for module") ;
	alignment = alignment >> 2 ;		/* Shift two because of implicit int alignment */
	offset = (parse->pure_free + alignment) & ~alignment ;
	parse->pure_free = offset + (bytes+(SIZEofINT-1))/SIZEofINT ;
	return(offset) ;
}

/*	prsu_alloc_ob - Allocates from object bucket */

prsu_alloc_ob(bytes,alignment)
  int bytes ;			/* Number of bytes to allocate */
  int alignment ;
{ int offset ;		/* Offset to begin */

	if ((parse->ob_bucket.free_offset*SIZEofINT) + bytes >= V3_SOB_BUCKET_BUF_MAX)
	 prsu_error(ABORT,"MAXOBJECT","Exceeded maximum number of bytes in object buffer space for package") ;
	alignment = alignment >> 2 ;		/* Shift two because of implicit int alignment */
	offset = (parse->ob_bucket.free_offset + alignment) & ~alignment ;
	parse->ob_bucket.free_offset = offset + (bytes+(SIZEofINT-1))/SIZEofINT ;
	return(offset) ;
}

/*	prsu_alloc_code - Allocates from code bucket */

prsu_alloc_code(bytes,alignment)
  int bytes ;			/* Number of bytes to allocate */
  int alignment ;
{ int offset ;		/* Offset to begin */

	if ((parse->code_offset) + bytes >= V3_PRS_CODE_BUF_MAX)
	 prsu_error(ABORT,"MAXCODE","Exceeded maximum number of bytes in code buffer space for package") ;
	alignment = alignment >> 1 ;		/* Shift one because of implicit short-int alignment */
	offset = (parse->code_offset + alignment) & ~alignment ;
	parse->code_offset = offset + (bytes+1)/2 ;
	return(offset) ;
}

/*	prsu_alloc_stack - Allocates from module temp stack */
prsu_alloc_stack(bytes,alignment)
  int bytes ;			/* Number of bytes to allocate */
{ int offset,alloc ;

	alloc = bytes + (bytes >= ALLOCATE_MIN ? ALLOCATE_EXTRA : 0) ;
	offset = (parse->module_stack_bytes + alignment) & ~alignment ; parse->module_stack_bytes = offset + alloc ;
/*
	if (parse->module_stack_bytes > V3_MODULE_DYNAMIC_MAX)
	 prsu_error(ERRP,"MAXSTACK","Exceeded maximum allowable dynamic storage for module") ;
*/
	return(offset) ;
}

/*	prsu_alloc_struct - Allocates from structure element buffer */
prsu_alloc_struct(bytes,alignment)
  int bytes ;			/* Number of bytes to allocate */
  int alignment ;
{ int offset ;

	offset = (parse->struct_bytes + alignment) & ~alignment ;
	if ((parse->struct_bytes = offset+bytes) >= V3_PRS_STRUCT_BUF_SIZE)
	 prsu_error(ABORT,"MAXSTRUCT","Exceeded maximum number of bytes in structure element buffer") ;
	return(offset) ;
}

/*	prsu_formref - Creates a format reference	*/
prsu_formref(vft,vfm,length,dps)
  int vft,vfm,length,dps ;
{ union val__format vf ;

	vf.fld.type = vft ; vf.fld.decimals = dps ; vf.fld.mode = vfm ; vf.fld.length = length ;
	return(vf.all) ;	/* Return as integer */
}

/*	prsu_valref - Creates a value reference		*/
prsu_valref(type,offset)
  int type,offset ;
{ union val__mempos vr ;

	vr.statpure.id = type ; vr.statpure.package_id = parse->package_id ;
	if (type == VAL_STACK) { vr.stack.offset = offset ; }
	 else vr.statpure.offset = offset ;
	return(vr.all) ;
}

/*	Predefined Table I - V3 Reserved Words		*/

struct db__predef_table V3_RESERVED_WORDS[] =
    {	{"BRANCH",15,PUSH_BRANCH,POP_BRANCH,NULLV,NULLV,RES_UNK,1},
	{"DCL",18,PUSH_DCL,NULLV,TO,NULLV,RES_NONE,0},
	{"ELSE",13,PUSH_ELSE,NULLV,TO,NULLV,RES_UNK,0},
	{"FALL_THRU",30,PUSH_FALL_THRU,NULLV,TO,NULLV,RES_NONE,0},
	{"GOTO",30,PUSH_GOTO,NULLV,TO,NULLV,RES_NONE,0},
	{"IF",12,PUSH_IF,POP_IFISH,NULLV,NULLV,RES_UNK,1},
	{"LOOP",30,PUSH_LOOP,POP_LOOP,NULLV,NULLV,RES_NONE,1},
	{"LOOP1",30,PUSH_LOOP,POP_LOOP,L1,NULLV,RES_NONE,1},
	{"P",14,NULLV,NULLV,EOA,V3_XCT_PRINT,RES_NONE,0},
	{"RETURN",30,NULLV,POP_RETURN,EOA,V3_XCT_RETURN,RES_NONE,0},
	{"THEN",13,PUSH_THEN,NULLV,TO,NULLV,RES_UNK,0},
    } ;
int PRS_RESERVED_WORD_COUNT=11 ;	/* ** NOTE: Must match above ** */

/*	V3 Predefined Functions				*/

struct db__predef_table V3_FUNCTIONS[] =
   {	{"AND",22,NULLV,NULLV,NULLV,V3_XCT_AND,RES_INT,2},
	{"BITMAP_AND",30,NULLV,NULLV,UPD1,V3_XCT_BITMAP_AND,RES_NONE,2},
	{"BITMAP_BYTES",30,NULLV,NULLV,NULLV,V3_XCT_BITMAP_BYTES,RES_INT,1},
	{"BITMAP_CLEAR",30,NULLV,NULLV,UPD1,V3_XCT_BITMAP_CLEAR,RES_NONE,2},
	{"BITMAP_COMP",30,NULLV,NULLV,UPD1,V3_XCT_BITMAP_COMP,RES_NONE,1},
	{"BITMAP_COUNT",30,NULLV,NULLV,NULLV,V3_XCT_BITMAP_COUNT,RES_INT,1},
	{"BITMAP_DCL",30,NULLV,NULLV,UPD1,V3_XCT_BITMAP_DCL,RES_NONE,2},
	{"BITMAP_EXTEND",30,NULLV,NULLV,UPD1,V3_XCT_BITMAP_EXTEND,RES_NONE,2},
	{"BITMAP_NEXT_CLEAR",30,NULLV,NULLV,NULLV,V3_XCT_BITMAP_NEXT_CLEAR,RES_INT,2},
	{"BITMAP_NEXT_SET",30,NULLV,NULLV,NULLV,V3_XCT_BITMAP_NEXT_SET,RES_INT,2},
	{"BITMAP_OR",30,NULLV,NULLV,UPD1,V3_XCT_BITMAP_OR,RES_NONE,2},
	{"BITMAP_SET",30,NULLV,NULLV,UPD1,V3_XCT_BITMAP_SET,RES_NONE,2},
	{"BITMAP_TEST",30,NULLV,NULLV,NULLV,V3_XCT_BITMAP_TEST,RES_INT,2},
	{"BITMAP_TESTDF",30,NULLV,NULLV,NULLV,V3_XCT_BITMAP_TESTDF,RES_INT,2},
	{"BIT_AND",30,NULLV,NULLV,NULLV,V3_XCT_BIT_AND,RES_INT,2},
	{"BIT_CLEAR",30,NULLV,NULLV,NULLV,V3_XCT_BIT_CLEAR,RES_INT,2},
	{"BIT_COMP",30,NULLV,NULLV,NULLV,V3_XCT_BIT_COMP,RES_INT,1},
	{"BIT_EQV",30,NULLV,NULLV,NULLV,V3_XCT_BIT_EQV,RES_INT,2},
	{"BIT_GET_BYTE",30,NULLV,NULLV,NULLV,V3_XCT_BIT_GET_BYTE,RES_INT,2},
	{"BIT_INVERT",30,NULLV,NULLV,NULLV,V3_XCT_BIT_INVERT,RES_INT,1},
	{"BIT_OR",30,NULLV,NULLV,NULLV,V3_XCT_BIT_OR,RES_INT,2},
	{"BIT_ROTATE",30,NULLV,NULLV,NULLV,V3_XCT_BIT_ROTATE,RES_INT,2},
	{"BIT_SET",30,NULLV,NULLV,NULLV,V3_XCT_BIT_SET,RES_INT,2},
	{"BIT_SHIFT",30,NULLV,NULLV,NULLV,V3_XCT_BIT_SHIFT,RES_INT,2},
	{"BIT_TEST_ALL",30,NULLV,NULLV,NULLV,V3_XCT_BIT_TEST_ALL,RES_INT,2},
	{"BIT_TEST_ANY",30,NULLV,NULLV,NULLV,V3_XCT_BIT_TEST_ANY,RES_INT,2},
	{"BIT_TEST_NONE",30,NULLV,NULLV,NULLV,V3_XCT_BIT_TEST_NONE,RES_INT,2},
	{"BIT_TEST_NTH",30,NULLV,NULLV,NULLV,V3_XCT_BIT_TEST_NTH,RES_INT,2},
	{"BIT_UNPACK_LH",30,NULLV,NULLV,NULLV,V3_XCT_BIT_UNPACK_LH,RES_INT,1},
	{"BIT_UNPACK_RH",30,NULLV,NULLV,NULLV,V3_XCT_BIT_UNPACK_RH,RES_INT,1},
	{"BIT_XOR",30,NULLV,NULLV,NULLV,V3_XCT_BIT_XOR,RES_INT,2},
	{"CB_PUT",30,NULLV,NULLV,NULLV,V3_XCT_CB_PUT,RES_NONE,1},
	{"CMD_ARG_NUM",30,NULLV,NULLV,NULLV,V3_XCT_CMD_ARG_NUM,RES_INT,0},
	{"CMD_ARG_VALUE",30,NULLV,NULLV,NULLV,V3_XCT_CMD_ARG_VALUE,RES_UNK,1},
	{"CMD_TEST",30,NULLV,NULLV,NULLV,V3_XCT_CMD_TEST,RES_INT,1},
	{"CMD_XCT",30,NULLV,NULLV,EOA,V3_XCT_CMD_XCT,RES_NONE,1},
	{"CONTINUE",30,PUSH_CONTINUE,NULLV,NULLV,NULLV,RES_NONE,0},
	{"DATE_INFO",30,NULLV,NULLV,NULLV,V3_XCT_DATE_INFO,RES_NONE,1},
	{"DBG_CLEARBP",30,NULLV,NULLV,NULLV,V3_XCT_DBG_CLEARBP,RES_NONE,1},
	{"DBG_SETBP",30,NULLV,NULLV,NULLV,V3_XCT_DBG_SETBP,RES_INT,1},
	{"DBG_SHOW_CALLS",30,NULLV,NULLV,EOA,V3_XCT_DBG_SHOW_CALLS,RES_UNK,0},
	{"FORK_DELETE",30,NULLV,NULLV,NULLV,V3_XCT_FORK_DELETE,RES_INT,1},
	{"FORK_NEW",30,NULLV,NULLV,NULLV,V3_XCT_FORK_NEW,RES_INT,1},
	{"FORK_SWITCH",30,NULLV,POP_FORK_SWITCH,NULLV,V3_XCT_FORK_SWITCH,RES_NONE,1},
	{"HEAP_ALLOC",30,NULLV,NULLV,NULLV,V3_XCT_HEAP_ALLOC,RES_INT,1},
	{"HEAP_FREE",30,NULLV,NULLV,NULLV,V3_XCT_HEAP_FREE,RES_NONE,1},
	{"INT_DISABLE",30,NULLV,NULLV,NULLV,V3_XCT_INT_DISABLE,RES_NONE,1},
	{"INT_ENABLE",30,NULLV,NULLV,NULLV,V3_XCT_INT_ENABLE,RES_NONE,2},
	{"INT_IGNORE",30,NULLV,NULLV,NULLV,V3_XCT_INT_IGNORE,RES_NONE,1},
	{"INT_SET_TIMER",30,NULLV,NULLV,NULLV,V3_XCT_INT_SET_TIMER,RES_NONE,1},
	{"IO_CLOSE",30,NULLV,NULLV,NULLV,V3_XCT_IO_CLOSE,RES_NONE,1},
	{"IO_GET",30,NULLV,NULLV,NULLV,V3_XCT_IO_GET,RES_NONE,1},
	{"IO_MISC",30,NULLV,NULLV,UPD2+EOA,V3_XCT_IO_MISC,RES_UNK,2},
	{"IO_OPEN",30,NULLV,NULLV,NULLV,V3_XCT_IO_OPEN,RES_NONE,1},
	{"IO_PUT",30,NULLV,NULLV,NULLV,V3_XCT_IO_PUT,RES_NONE,1},
	{"ITX_ALPHA",30,NULLV,NULLV,UPD2,V3_XCT_ITX_ALPHA,RES_INT,3},
	{"ITX_DATE",30,NULLV,NULLV,UPD2,V3_XCT_ITX_DATE,RES_INT,3},
	{"ITX_DATE_TIME",30,NULLV,NULLV,UPD2,V3_XCT_ITX_DATE_TIME,RES_INT,3},
	{"ITX_GEN",30,PUSH_ITX_GEN,NULLV,UPD3+EOA,V3_XCT_ITX_GEN,RES_INT,3},
	{"ITX_LOGICAL",30,NULLV,NULLV,UPD2,V3_XCT_ITX_LOGICAL,RES_INT,3},
	{"ITX_MONTH",30,NULLV,NULLV,UPD2,V3_XCT_ITX_MONTH,RES_INT,3},
	{"ITX_NUM",30,NULLV,NULLV,UPD2,V3_XCT_ITX_INT,RES_INT,3},
	{"ITX_POINTER",30,NULLV,NULLV,UPD2,V3_XCT_ITX_POINTER,RES_INT,3},
	{"ITX_TIME",30,NULLV,NULLV,UPD2,V3_XCT_ITX_TIME,RES_INT,3},
	{"LOAD",30,NULLV,NULLV,NULLV,V3_XCT_LOAD,RES_NONE,1},
	{"MOD_APPLY",30,NULLV,NULLV,EOA,V3_XCT_MOD_APPLY,RES_UNK,1},
	{"MOD_ARG_NUM",30,NULLV,NULLV,NULLV,V3_XCT_V3_ARG_NUM,RES_INT,0},
	{"MOD_ARG_VALUE",30,NULLV,NULLV,NULLV,V3_XCT_V3_ARG_VALUE,RES_UNK,1},
	{"MOD_CURRENT_PSI",30,NULLV,NULLV,NULLV,V3_XCT_MOD_CURRENT_PSI,RES_UNK,0},
	{"MOD_DEFINED",30,NULLV,NULLV,NULLV,V3_XCT_MOD_DEFINED,RES_INT,1},
	{"MOD_ENTRY_HANDLE",30,NULLV,NULLV,NULLV,V3_XCT_MOD_ENTRY_HANDLE,RES_UNK,1},
	{"MOD_ITEM_DELETE",30,NULLV,NULLV,NULLV,V3_XCT_MOD_ITEM_DELETE,RES_INT,1},
	{"MOD_ITEM_FIND",30,NULLV,NULLV,UPD2,V3_XCT_MOD_ITEM_FIND,RES_UNK,3},
	{"MOD_ITEM_PUSH",30,NULLV,NULLV,NULLV,V3_XCT_MOD_ITEM_PUSH,RES_NONE,3},
	{"MOD_NAME",30,NULLV,NULLV,NULLV,V3_XCT_MOD_NAME,RES_UNK,1},
	{"MOD_PRIOR_PSI",30,NULLV,NULLV,NULLV,V3_XCT_MOD_PRIOR_PSI,RES_UNK,0},
	{"MOD_UNLOAD",30,NULLV,NULLV,NULLV,V3_XCT_MOD_UNLOAD,RES_INT,1},
	{"MOD_WATCHDOG",30,NULLV,NULLV,NULLV,V3_XCT_MOD_WATCHDOG,RES_NONE,1},
	{"MTH_ABS",30,NULLV,NULLV,NULLV,V3_XCT_MTH_ABS,RES_ARGS,1},
	{"MTH_DIGITS",30,NULLV,NULLV,NULLV,V3_XCT_MTH_DIGITS,RES_INT,2},
	{"MTH_FTOI",30,NULLV,NULLV,NULLV,V3_XCT_MTH_FTOI,RES_INT,2},
	{"MTH_LOG10",30,NULLV,NULLV,NULLV,V3_XCT_MTH_LOG10,RES_UNK,1},
	{"MTH_LOGE",30,NULLV,NULLV,NULLV,V3_XCT_MTH_LOGE,RES_UNK,1},
	{"MTH_MAX",30,NULLV,NULLV,EOA+ARITH,V3_XCT_MTH_MAX,RES_ARGS,2},
	{"MTH_MIN",30,NULLV,NULLV,EOA+ARITH,V3_XCT_MTH_MIN,RES_ARGS,2},
	{"MTH_MOD",30,NULLV,NULLV,NULLV,V3_XCT_MTH_MOD,RES_INT,2},
	{"MTH_MOD1",30,NULLV,NULLV,NULLV,V3_XCT_MTH_MOD1,RES_INT,2},
	{"MTH_PERCENT",30,NULLV,NULLV,NULLV,V3_XCT_MTH_PERCENT,RES_UNK,2},
	{"MTH_POWER",30,NULLV,NULLV,NULLV,V3_XCT_MTH_POWER,RES_UNK,2},
	{"MTH_RANDOM",30,NULLV,NULLV,NULLV,V3_XCT_MTH_RANDOM,RES_UNK,1},
	{"MTH_RANDOM_SEED",30,NULLV,NULLV,NULLV,V3_XCT_MTH_RANDOM_SEED,RES_NONE,1},
	{"MTH_SQRT",30,NULLV,NULLV,NULLV,V3_XCT_MTH_SQRT,RES_UNK,1},
	{"NIL",30,NULLV,NULLV,NULLV,V3_XCT_NIL,RES_UNK,0},
	{"NOT",23,NULLV,NULLV,NULLV,V3_XCT_NOT,RES_INT,1},
	{"NULL",30,NULLV,NULLV,NULLV,V3_XCT_NULL,RES_INT,1},
	{"OBJ_ASSERTION_ADD",30,NULLV,NULLV,NULLV,V3_XCT_ASSERTION_ADD,RES_INT,1},
	{"OBJ_ASSERTION_DELETE",30,NULLV,NULLV,NULLV,V3_XCT_ASSERTION_DELETE,RES_INT,1},
	{"OBJ_ASSERTION_TEST",30,NULLV,NULLV,NULLV,V3_XCT_ASSERTION_TEST,RES_INT,1},
	{"OBJ_BIND_SKELETON",30,NULLV,NULLV,NULLV,V3_XCT_OBJ_BIND_SKELETON,2},
	{"OBJ_FIND",30,NULLV,NULLV,UPD1,V3_XCT_OBJ_FIND,RES_INT,2},
	{"OBJ_HAS_FIND",30,NULLV,NULLV,UPD1,V3_XCT_OBJ_HAS_FIND,RES_INT,3},
	{"OBJ_ID",30,NULLV,NULLV,NULLV,V3_XCT_OBJ_ID,RES_INT,1},
	{"OBJ_NAME",30,NULLV,NULLV,NULLV,V3_XCT_OBJ_NAME,RES_UNK,1},
	{"OBJ_VALUE",30,NULLV,NULLV,EOA,V3_XCT_OBJ_VALUE,RES_UNK,1},
	{"OBJ_VALUE_NUM",30,NULLV,NULLV,NULLV,V3_XCT_OBJ_VALUE_NUM,RES_INT,1},
	{"OR",21,NULLV,NULLV,NULLV,V3_XCT_OR,RES_INT,2},
	{"OUT",15,PUSH_OUT,NULLV,TO,NULLV,RES_NONE,0},
	{"PATH",15,PUSH_PATH,NULLV,TO,NULLV,RES_NONE,0},
	{"PCK_COMPILE",30,NULLV,NULLV,NULLV,V3_XCT_PCK_COMPILE,RES_NONE,2},
	{"PCK_DCL_FILENAME",30,NULLV,NULLV,NULLV,V3_XCT_PCK_DCL_FILENAME,RES_NONE,2},
	{"PCK_ENTER",30,NULLV,NULLV,NULLV,V3_XCT_PCK_ENTER,RES_NONE,2},
	{"PCK_LOAD",30,NULLV,NULLV,NULLV,V3_XCT_PCK_LOAD,RES_NONE,2},
	{"PCK_NEW",30,NULLV,NULLV,NULLV,V3_XCT_PCK_NEW,RES_NONE,2},
	{"PCK_OLD_PARSE",30,NULLV,NULLV,NULLV,V3_XCT_PCK_OLD_PARSE,RES_INT,1},
	{"PCK_OLD_SYSTEM",30,NULLV,NULLV,NULLV,V3_XCT_PCK_OLD_SYSTEM,RES_INT,1},
	{"PCK_OLD_XCT",30,NULLV,NULLV,NULLV,V3_XCT_PCK_OLD_XCT,RES_INT,2},
	{"PCK_SAVE",30,NULLV,NULLV,NULLV,V3_XCT_PCK_SAVE,RES_NONE,2},
	{"PCK_UNLOAD",30,NULLV,POP_PCK_UNLOAD,NULLV,NULLV,RES_NONE,1},
	{"RPT_COLWIDTH",30,NULLV,NULLV,NULLV,V3_XCT_RPT_COLWIDTH,RES_INT,2},
	{"SCR_DISPLAY_ROWS",30,NULLV,NULLV,NULLV,V3_XCT_SCR_DISPLAY_ROWS,RES_NONE,6},
	{"SCR_SET_FIELD",30,NULLV,NULLV,NULLV,V3_XCT_SCR_SET_FIELD,RES_NONE,4},
	{"SEG_CREATE",30,NULLV,NULLV,NULLV,V3_XCT_SEG_CREATE,RES_NONE,2},
	{"SEG_LOCK",30,NULLV,NULLV,NULLV,V3_XCT_SEG_LOCK,RES_NONE,2},
	{"SEG_SHARE",30,NULLV,NULLV,NULLV,V3_XCT_SEG_SHARE,RES_INT,2},
	{"SEG_UNLOCK",30,NULLV,NULLV,NULLV,V3_XCT_SEG_UNLOCK,RES_NONE,1},
	{"SEG_UPDATE",30,NULLV,NULLV,NULLV,V3_XCT_SEG_UPDATE,RES_NONE,1},
	{"STR_APPEND",30,NULLV,NULLV,NULLV,V3_XCT_STR_APPEND,RES_UNK,2},
	{"STR_BE",30,NULLV,NULLV,UPD1,V3_XCT_STR_BE,RES_UNK,3},
	{"STR_BE2",30,NULLV,NULLV,NULLV,V3_XCT_STR_BE2,RES_UNK,2},
	{"STR_BL",30,NULLV,NULLV,NULLV,V3_XCT_STR_BL,RES_UNK,3},
	{"STR_BREAK",30,NULLV,NULLV,UPD2,V3_XCT_STR_BREAK,RES_UNK,2},
	{"STR_BREAK_RIGHT",30,NULLV,NULLV,UPD2,V3_XCT_STR_BREAK_RIGHT,RES_UNK,2},
	{"STR_COMPARE",30,NULLV,NULLV,NULLV,V3_XCT_STR_COMPARE,RES_INT,2},
	{"STR_COMPRESS",30,NULLV,NULLV,UPD1,V3_XCT_STR_COMPRESS,RES_INT,2},
	{"STR_CONCAT",30,NULLV,NULLV,EOA,V3_XCT_STR_CONCAT,RES_UNK,1},
	{"STR_EBCTOASC",30,NULLV,NULLV,NULLV,V3_XCT_STR_EBCTOASC,RES_NONE,2},
	{"STR_EXPAND",30,NULLV,NULLV,UPD1,V3_XCT_STR_EXPAND,RES_INT,2},
	{"STR_HASH",30,NULLV,NULLV,NULLV,V3_XCT_STR_HASH,RES_INT,1},
	{"STR_ITOS",30,NULLV,NULLV,NULLV,V3_XCT_STR_ITOS,RES_UNK,1},
	{"STR_LEN",30,NULLV,NULLV,NOREF,V3_XCT_STR_LEN,RES_INT,1},
	{"STR_LIST",30,NULLV,NULLV,NULLV,V3_XCT_STR_LIST,RES_UNK,2},
	{"STR_LOGICAL",30,NULLV,NULLV,NULLV,V3_XCT_STR_LOGICAL,RES_INT,1},
	{"STR_MATCH",30,NULLV,NULLV,NULLV,V3_XCT_STR_MATCH,RES_INT,3},
	{"STR_NULLS",30,NULLV,NULLV,UPD1,V3_XCT_STR_NULLS,RES_UNK,1},
	{"STR_PRIOR",30,NULLV,NULLV,NULLV,V3_XCT_STR_PRIOR,RES_UNK,1},
	{"STR_SPACES",30,NULLV,NULLV,UPD1,V3_XCT_STR_SPACES,RES_UNK,1},
	{"STR_STOI",30,NULLV,NULLV,NULLV,V3_XCT_STR_STOI,RES_INT,1},
	{"STR_TRIM",30,NULLV,NULLV,NULLV,V3_XCT_STR_TRIM,RES_UNK,1},
	{"STR_TYPE",30,NULLV,NULLV,NULLV,V3_XCT_STR_TYPE,RES_INT,1},
	{"STR_VALUE",30,NULLV,NULLV,NULLV,V3_XCT_STR_VALUE,RES_UNK,1},
	{"SYS_ADDRESS",30,NULLV,NULLV,NOREF,V3_XCT_ADDRESS,RES_UNK,1},
	{"SYS_ADDRESSSKEL",30,NULLV,NULLV,NOREF,V3_XCT_ADDRESSSKEL,RES_UNK,1},
	{"SYS_CHAIN",30,NULLV,NULLV,NULLV,V3_XCT_SYS_CHAIN,RES_NONE,1},
	{"SYS_DELETE",30,NULLV,NULLV,NULLV,V3_XCT_SYS_DELETE,RES_INT,1},
	{"SYS_EXIT",30,NULLV,NULLV,EOA,V3_XCT_SYS_EXIT,RES_NONE,0},
	{"SYS_INFO",30,NULLV,NULLV,EOA,V3_XCT_SYS_INFO,RES_UNK,1},
	{"SYS_LOGOUT",30,NULLV,NULLV,NULLV,V3_XCT_SYS_LOGOUT,RES_NONE,0},
	{"SYS_PRINT",30,NULLV,NULLV,NULLV,V3_XCT_SYS_PRINT,RES_INT,1},
	{"SYS_RENAME",30,NULLV,NULLV,NULLV,V3_XCT_SYS_RENAME,RES_INT,1},
	{"SYS_SET_PARAMETER",30,NULLV,NULLV,NULLV,V3_XCT_SYS_SET_PARAMETER,RES_NONE,2},
	{"SYS_SLEEP",30,NULLV,NULLV,NULLV,V3_XCT_SYS_SLEEP,RES_NONE,1},
	{"SYS_SPAWN",30,NULLV,NULLV,EOA,V3_XCT_SYS_SPAWN,RES_INT,0},
	{"SYS_SUBMIT",30,NULLV,NULLV,NULLV,V3_XCT_SYS_SUBMIT,RES_INT,1},
	{"TKN_NEXT",30,NULLV,NULLV,NULLV,V3_XCT_TKN_NEXT,RES_INT,2},
	{"TKN_PARSE",30,NULLV,NULLV,UPD1,V3_XCT_TKN_PARSE,RES_INT,2},
	{"TKN_PARSEOPT",30,NULLV,NULLV,UPD1,V3_XCT_TKN_PARSE_OPT,RES_NONE,2},
	{"TKN_SET",30,NULLV,NULLV,UPD1,V3_XCT_TKN_SET,RES_NONE,2},
	{"TTY_GET",30,NULLV,NULLV,EOA,V3_XCT_TTY_GET,RES_UNK,0},
	{"TTY_GETC",30,NULLV,NULLV,EOA,V3_XCT_TTY_GETC,RES_UNK,0},
	{"TTY_MISC",30,NULLV,NULLV,EOA,V3_XCT_TTY_MISC,RES_UNK,1},
	{"TTY_PUT",30,NULLV,NULLV,EOA,V3_XCT_TTY_PUT,RES_NONE,1},
	{"TTY_SCR_ANSI",30,NULLV,NULLV,EOA,V3_XCT_TTY_SCR_ANSI,RES_UNK,2},
	{"TTY_SCR_GET",30,NULLV,NULLV,NULLV,V3_XCT_TTY_SCR_GET,RES_INT,3},
	{"TTY_TERMINATOR",30,NULLV,NULLV,NULLV,V3_XCT_TTY_TERMINATOR,RES_UNK,0},
	{"V3_CLEAR",30,NULLV,NULLV,UPD1,V3_XCT_V3_CLEAR,RES_UNK,1},
	{"V3_CLONE",30,NULLV,NULLV,NOREF,V3_XCT_V3_CLONE,RES_UNK,2},
	{"V3_COPY",30,NULLV,NULLV,UPD1,V3_XCT_V3_COPY,RES_NONE,2},
	{"V3_COPYSWAP",30,NULLV,NULLV,UPD1,V3_XCT_V3_COPYSWAP,RES_NONE,2},
	{"V3_ERROR",30,NULLV,NULLV,NULLV,V3_XCT_V3_ERROR,RES_NONE,4},
	{"V3_INFO",30,NULLV,NULLV,EOA,V3_XCT_V3_INFO,RES_UNK,1},
	{"V3_MATCH",30,NULLV,NULLV,NULLV,V3_XCT_V3_MATCH,RES_INT,3},
	{"V3_SET_PARAMETER",30,NULLV,NULLV,EOA,V3_XCT_V3_SET_PARAMETER,RES_UNK,1},
	{"V3_XCTRPPEXP",30,NULLV,NULLV,EOA,V3_XCT_V3_XCTRPPEXP,RES_UNK,1},
	{"VEC_ADD",30,NULLV,NULLV,UPD1,V3_XCT_VEC_ADD,RES_NONE,3},
	{"VEC_ADD_CONSTANT",30,NULLV,NULLV,UPD2,V3_XCT_VEC_ADD_CONSTANT,RES_NONE,3},
	{"VEC_COPY",30,NULLV,NULLV,UPD1,V3_XCT_VEC_COPY,RES_NONE,3},
	{"VEC_FIND_CONSTANT",30,NULLV,NULLV,NULLV,V3_XCT_VEC_FIND_CONSTANT,RES_INT,3},
	{"VEC_SET",30,NULLV,NULLV,UPD2,V3_XCT_VEC_SET,RES_NONE,3},
	{"VEC_SPAN",30,NULLV,NULLV,NULLV,V3_XCT_VEC_SPAN,RES_INT,3},
	{"VEC_SUM",30,NULLV,NULLV,NULLV,V3_XCT_VEC_SUM,RES_INT,2},
	{"VEC_SWAP",30,NULLV,NULLV,NULLV,V3_XCT_VEC_SWAP,RES_NONE,3},
	{"XTI_ALPHA",30,NULLV,NULLV,UPD2,V3_XCT_XTI_ALPHA,RES_INT,3},
	{"XTI_DATE",30,NULLV,NULLV,UPD2,V3_XCT_XTI_DATE,RES_INT,3},
	{"XTI_DATE_TIME",30,NULLV,NULLV,UPD2,V3_XCT_XTI_DATE_TIME,RES_INT,3},
	{"XTI_GEN",30,PUSH_ITX_GEN,NULLV,UPD3+EOA,V3_XCT_XTI_GEN,RES_INT,3},
	{"XTI_LOGICAL",30,NULLV,NULLV,UPD2,V3_XCT_XTI_LOGICAL,RES_INT,3},
	{"XTI_MONTH",30,NULLV,NULLV,UPD2,V3_XCT_XTI_MONTH,RES_INT,3},
	{"XTI_NUM",30,NULLV,NULLV,UPD2,V3_XCT_XTI_INT,RES_INT,3},
	{"XTI_POINTER",30,NULLV,NULLV,UPD2,V3_XCT_XTI_POINTER,RES_INT,3},
	{"XTI_TIME",30,NULLV,NULLV,UPD2,V3_XCT_XTI_TIME,RES_INT,3},
   } ;
int PRS_FUNCTION_COUNT=200 ;		/* ** NOTE: Must match above ** */


/*	P R E D E F I N E D   S Y M B O L S   R O U T I N E S	*/

/*	prsu_predefined_lookup - Does lookup in predefined VICTIM III
	symbol table and returns pointer or NULLV to symbol	*/
char *prsu_predefined_lookup(sym_ptr,table_ptr,sym_count)
    char *sym_ptr ; int sym_count ;
    struct db__predef_table *table_ptr ;
{ int cr,i=0,j=sym_count-1,k=0 ; char *cp ;
   struct db__predef_table *tp ;

/*	Perform binary search on table */
	for (;;)
	 { if (i > j) return(NULLV) ;
	   tp = table_ptr + (k=(i+j)/2) ;
	   cr = strcmp(sym_ptr,tp->name) ;
	   switch (cr < 0 ? 1 : (cr > 0 ? 2 : cr))
	     {case 1:	/* Symbol before current table entry */
		j = k-1 ; break ;
	      case 0:	/* Symbol equals current table entry */
/*		Play games with pointers to get proper address */
		cp = (char *)tp ; return(cp + V3_PREDEFINED_SYM_LEN_MAX) ;
	      case 2:	/* Symbol after current table entry */
		i = k+1 ; break ;
	     } ;
	 } ;
}

/*	P A C K A G E   S Y M B O L   T A B L E   R O U T I N E S */

/*	prsu_sym_add - Adds a symbol to global/local/external symbol table  */
/*	  returns: *db__prs_sym			*/

struct db__prs_sym *prsu_sym_add(name,type)
  char *name ;			/* Symbol to be added */
  unsigned char type ;		/* One of: PRS_GLOBAL_SYM, PRS_EXTERNAL_SYM or PRS_LOCAL_SYM */
{ struct db__prs_sym_table_big *table ;
   int sx ; int hash_val ;

/*	Get the hash value for this symbol */
	hash_val = prsu_str_hash(name,-1) ;
/*	Now see if global or local */
	switch (type)
	 {case PRS_GLOBAL_SYM:
		table = (struct db__prs_sym_table_big *)&(parse->globals) ;
		if (table->count >= V3_PRS_SYMBOL_TABLE_MAX_SMALL)
		  prsu_error(ABORT,"MAXGLOBALS","Too many global symbols in this package") ;
		strcpy(table->sym[sx = (table->count)++].name,name) ;
		table->sym[sx].hash = hash_val ;
		table->length = PRSLEN(&(table->sym[table->count+1]),table) ;
/*		Return pointer to global symbol entry */
		return(&table->sym[sx]) ;
	  case PRS_STRUCTDEF_SYM:
		table = (struct db__prs_sym_table_big *)&(parse->structures) ;
		if (table->count >= V3_PRS_SYMBOL_TABLE_MAX_SMALL)
		  prsu_error(ABORT,"MAXGLOBALS","Too many global structure definitions in this package") ;
		strcpy(table->sym[sx = (table->count)++].name,name) ;
		table->sym[sx].hash = hash_val ;
		table->length = PRSLEN(&(table->sym[table->count+1]),table) ;
/*		Return pointer to symbol entry */
		return(&table->sym[sx]) ;
	  case PRS_LOCAL_SYM:
		table = (struct db__prs_sym_table_big *)&(parse->locals) ;
		if (table->count >= V3_PRS_SYMBOL_TABLE_MAX_BIG)
		  prsu_error(ABORT,"MAXLOCALS","Too many local symbols in this package") ;
		strcpy(table->sym[sx = (table->count)++].name,name) ;
		table->sym[sx].hash = hash_val ;
/*		Have to link symbol for proper search of levels */
		table->sym[sx].prior_sym_index = parse->level->prior_sym_index ;
		table->sym[sx].level_id = parse->level->level_id ;
		table->sym[sx].module_id = parse->last_module_id ;
		parse->level->prior_sym_index = sx+1 ; /* Add 1 so 0 = end-of-list */
		table->length = PRSLEN(&(table->sym[table->count+1]),table) ;
/*		Return pointer to local symbol entry */
		return(&table->sym[sx]) ;
	  case PRS_EXTERNAL_SYM:
		table = (struct db__prs_sym_table_big *)&(parse->externals) ;
		if (table->count >= V3_PRS_SYMBOL_TABLE_MAX_SMALL)
		  prsu_error(ABORT,"MAXEXTERNALS","Too many external symbols in this package") ;
		strcpy(table->sym[sx = (table->count)++].name,name) ;
		table->sym[sx].hash = hash_val ;
		table->length = PRSLEN(&(table->sym[table->count+1]),table) ;
/*		Return pointer to external symbol entry */
		return(&table->sym[sx]) ;
	  case PRS_LABEL_SYM:
		table = (struct db__prs_sym_table_big *)&(parse->labels) ;
		if (table->count >= V3_PRS_SYMBOL_TABLE_MAX_SMALL)
		  prsu_error(ABORT,"MAXEXTERNALS","Too many external symbols in this package") ;
		strcpy(table->sym[sx = (table->count)++].name,name) ;
		table->sym[sx].hash = hash_val ;
/*		Remember label's module level_id */
		table->sym[sx].level_id = parse->level->level_id ;
		table->sym[sx].module_id = parse->last_module_id ;
		table->length = PRSLEN(&(table->sym[table->count+1]),table) ;
/*		Return pointer to label symbol entry */
		return(&table->sym[sx]) ;
	  case PRS_ELEMENT_SYM:
		table = (struct db__prs_sym_table_big *)&(parse->elements) ;
		if (table->count >= V3_PRS_SYMBOL_TABLE_MAX_BIG)
		  prsu_error(ABORT,"MAXEXTERNALS","Too many element symbols in this package") ;
/*		Hash into (hopefully) free slot in this table */
		sx = hash_val % V3_PRS_SYMBOL_TABLE_MAX_BIG ;
		for(;;sx++)
		 { if (table->sym[sx].hash == 0) break ;
		   if (sx >= V3_PRS_SYMBOL_TABLE_MAX_BIG-1) sx = -1 ;
		 } ;
		strcpy(table->sym[sx].name,name) ; table->sym[sx].hash = hash_val ;
		table->count++ ;
/*		Return pointer to external symbol entry */
		return(&table->sym[sx]) ;
	   default:
/*		Here if bad parameter passed to routine */
		prsu_error(INTERNAL,"BADSYMTBL","Invalid symbol table type in prsu_sym_add") ;
	 } ;
	return(NULL) ;
}

/*	prsu_sym_lookup - Looks up symbol in all active tables  */

struct db__prs_sym *prsu_sym_lookup(name,structure_id)
  char *name ;			/* Symbol name */
  int structure_id ;		/* STRUCT id to be matched */
				/* If negative then module id to find */
{ struct db__prs_sym_table_big *table ;
   int sx ; int hash_val ;

/*	Get hash value for the symbol */
	hash_val = prsu_str_hash(name,-1) ;
/*	The first symbol table to be search is the LOCAL table */
	table = (struct db__prs_sym_table_big *)&(parse->locals) ;
/*	See if explicit module reference */
	if (structure_id < 0)
	 { for (sx=table->count-1;sx>=0;sx--)
	    { if (hash_val != table->sym[sx].hash) continue ;
	      if (-structure_id != table->sym[sx].module_id) continue ;
	      if (strcmp(table->sym[sx].name,name) != 0) continue ;
/*	      Got a match ! */
	      return(&table->sym[sx]) ;
	    } ;
/*	   Can't seem to find in local table - try others */
	   goto not_local ;
	 } ;
/*	If looking for a structure element then search special table */
	if (structure_id > 0)
	 { sx = hash_val % V3_PRS_SYMBOL_TABLE_MAX_BIG ;
	   table = (struct db__prs_sym_table_big *)&parse->elements ;
/*	   Got starting hash position in table, see if proper symbol */
	   for(;;sx++)
	    { if (sx >= V3_PRS_SYMBOL_TABLE_MAX_BIG) sx = 0 ;
	      if (table->sym[sx].hash == 0) break ;	/* Found hole in table, ergo no symbol */
	      if (hash_val != table->sym[sx].hash) continue ;
	      if (strcmp(table->sym[sx].name,name) != 0) continue ;
	      if (structure_id != table->sym[sx].structure_id) continue ;
/*	      Got a hit ! */
	      return(&table->sym[sx]) ;
	    } ;
/*	   No hit, won't find the symbol */
	   return(NULLV) ;
	 } ;
	if ((sx = parse->level->prior_sym_index) == 0) goto not_local ;
/*	Scan backwards thru current level "access path" */
	do {if (hash_val != table->sym[sx-1].hash) continue ;
	    if (strcmp(table->sym[sx-1].name,name) != 0) continue ;
/*	    Looks like we got a hit - return pointer */
	    table->sym[sx-1].referenced = TRUE ;
	    return(&table->sym[sx-1]) ;
	   } while (sx = table->sym[sx-1].prior_sym_index) ;
/*	Not in local table, try the global */
not_local:
	table = (struct db__prs_sym_table_big *)&(parse->globals) ;
	for (sx = table->count-1;sx>=0;sx--)	/* Search backwards */
	 { if (hash_val != table->sym[sx].hash) continue ;
	   if (strcmp(table->sym[sx].name,name) != 0) continue ;
/*	   Got a match ! */
	   return(&table->sym[sx]) ;
	 } ;
/*	Try structure definitions */
	table = (struct db__prs_sym_table_big *)&(parse->structures) ;
	for (sx = table->count-1;sx>=0;sx--)	/* Search backwards */
	 { if (hash_val != table->sym[sx].hash) continue ;
	   if (strcmp(table->sym[sx].name,name) != 0) continue ;
/*	   Got a match ! */
	   return(&table->sym[sx]) ;
	 } ;
/*	Try externals */
	table = (struct db__prs_sym_table_big *)&(parse->externals) ;
	for (sx = table->count-1;sx>=0;sx--)	/* Search backwards */
	 { if (hash_val != table->sym[sx].hash) continue ;
	   if (strcmp(table->sym[sx].name,name) != 0) continue ;
/*	   Got a match ! */
	   return(&table->sym[sx]) ;
	 } ;
/*	Not anywhere to be found - see if maybe a label */
	table = (struct db__prs_sym_table_big *)&(parse->labels) ;
	for (sx = table->count-1;sx>=0;sx--)	/* Search backwards */
	 { if (hash_val != table->sym[sx].hash) continue ;
	   if (parse->last_module_id != table->sym[sx].module_id) continue ;
	   if (strcmp(table->sym[sx].name,name) != 0) continue ;
/*	   Got a match ! */
	   return(&table->sym[sx]) ;
	 } ;
/*	If here then can't find symbol anywhere - return NULLV */
	return(NULLV) ;
}

/*	prsu_sym_define - Defines a symbol table entry		*/
struct db__prs_sym *prsu_sym_define(name,type,table,vft,val,length,ttllen,subs,dps,strid,offset,search_id,align_mask)
  char *name ;			/* Symbol name */
  int type ;			/* SYMBOL_xxx (ex: SYMBOL_NORMAL) */
  int table ;			/* PRS_xxx_SYM where xxx = LOCAL/GLOBAL */
  int vft ;			/* VFT_xxx value (ex: VFT_BININT) */
  int val ;			/* VAL_xxx value (ex: VAL_STATIMP) */
  int length ;			/* Number of bytes for sym value (each occurance) */
  int ttllen ;			/* Total bytes to allocate (taking subscripts into account) */
  int subs ;			/* Number of subscripts */
  int dps ;			/* Number of decimal places */
  int strid ;			/* Structure id this symbol belongs to */
  int offset ;			/* Symbol offset if a structure element */
  int search_id ;		/* Search id to use for structure's elements */
  int align_mask ;		/* Address alignment mask (ex: 3 for word alignment) */
{ int i ;
   struct db__prs_sym *sp ;	/* Points to symbol table entry */
   struct db__formatmempos *se ;

	if (search_id == 0 && strid != 0)
	 { if (v4b.fileref > 0)
	    { strcpy(v4b.el[v4b.count].name,name) ; v4b.el[v4b.count].offset = offset ;
	      v4b.el[v4b.count].bytesall = ttllen ; v4b.el[v4b.count].elbytes = length ;
	      v4b.el[v4b.count].elcount = ttllen / length ; v4b.el[v4b.count].v3dt = vft ;
	      v4b.el[v4b.count].decimals = dps ; v4b.el[v4b.count].memberof = strid ;
	      v4b.el[v4b.count].ownerof = search_id ; v4b.count ++ ;
	    } ;
	 } ;
/*	Create the symbol in proper table */
	sp = prsu_sym_add(name,(strid == 0 ? table : PRS_ELEMENT_SYM)) ;
	sp->type = type ; sp->structure_id = strid ; sp->ss_count = subs ;
	sp->search_struct_id = search_id ; sp->align_mask = align_mask ;
/*	If a skeleton structure then return now */
	if(type == SYMBOL_SKELETON) return(sp) ;
/*	Check out the length of this symbol */
	if (ttllen > V3_PRS_MAX_STRING_LEN)
	 prsu_error(ERRP,"SYMTOOBIG","Symbol exceeds max V3 string length") ;
/*	If an external sym then just allocate space for 2-word desc in impure area & leave it at that */
	if (table == PRS_EXTERNAL_SYM)
	 { i = prsu_alloc_impure(sizeof *se,ALIGN_WORD) ; se = (struct db__formatmempos *)&parse->impure_buf[i] ;
	   sp->mempos.all = prsu_valref(VAL_STATIMP,i) ;
	   se->format.all = (se->mempos.all = NULLV) ;
	   return(sp) ;
	 } ;
/*	Allocate space for 2-word (plus 4 bytes/subscript) descriptor */
	if (type != SYMBOL_MODULE)
	 { if ((strid != 0) || (ttllen == 0))
/*	      Symbol is a structure element, save descriptor in struct_buf, not pure_buf */
	    { i = prsu_alloc_struct(4*subs + (sizeof *se),ALIGN_WORD) ; se = (struct db__formatmempos *)&parse->struct_buf[i] ;
	      sp->mempos.all = prsu_valref(VAL_STRUCT,i) ;
	    } else
	    { if ((val == VAL_STATIMP) && (table == PRS_GLOBAL_SYM))
	       { i = prsu_alloc_impure(4*subs + (sizeof *se),ALIGN_WORD) ; se = (struct db__formatmempos *)&parse->impure_buf[i] ;
	         sp->mempos.all = prsu_valref(VAL_STATIMP,i) ;
	       } else
	       { i = prsu_alloc_pure(4*subs + (sizeof *se),ALIGN_WORD) ; se = (struct db__formatmempos *)&parse->pure_buf[i] ;
	         sp->mempos.all = prsu_valref(VAL_STATPURE,i) ;
	       } ;
	    } ;
	 } else
	 { return(sp) ;		/* Symbol is a module - don't have to do anything now */
	 } ;
/*	Allocate storage for symbol */
	if (strid != 0 || type == SYMBOL_STRUCTURE) goto end_allocate ;	/* But not if STRUCT element */
	switch (val)
	 { case VAL_STATIMP:
		if (parse->is_module_bind)
		 prsu_error(ERRP,"V4MOD","Cannot reference package/global impure area within this (V4) module") ;
		i = prsu_alloc_impure(ttllen,align_mask) ; break ;
	   case VAL_STATIMPLM:
		i = prsu_alloc_impurelm(ttllen,align_mask) ; break ;
	   case VAL_STATPURE:
		i = prsu_alloc_pure(ttllen,align_mask) ; break ;
	   case VAL_STACK:
		i = prsu_alloc_stack(ttllen,align_mask) ; break ;
	   default: prsu_error(ERRP,"BADVALARG","Invalid val argument to prsu_sym_define") ;
	 } ;
end_allocate:
/*	Finally create 2-word descriptor */
	se->format.all = prsu_formref(vft,(strid == 0 ? VFM_PTR : VFM_OFFSET),length,dps) ;
	se->mempos.all = (strid == 0 ? prsu_valref(val,i) : offset) ;

	return(sp) ;
}

/*	T O K E N   U T I L I T I E S			*/

/*	prsu_str_hash - HASHES A STRING INTO int
call:	<int> = prsu_str_hash ( <string pointer> , <length or -1> )
*/
prsu_str_hash(str_ptr,str_len)
    unsigned char *str_ptr ;
    int str_len ;
{ int result=0 ; int i,b ;
    static unsigned char primes[] = { 1,3,5,7,11,13,17,19,23,29,31,37,41 } ;
    static unsigned char codes[] =
	{ 1,2,3,4,5,6,7,8,9,10,  0,0,0,0,0,0,0,
	  11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,
	  0,0,0,0,37,0,
	  11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36 } ;
/*	LOOP THRU EACH CHARACTER & MULTIPLY POSITION BY PRIME */
	if (str_len == -1) str_len = 999 ;
	for (i=0;i < str_len;i++)
	  { if (*str_ptr == 0) break ;	/* END OF STRING */
	    b = *(str_ptr++) ;
	    result += primes[i % sizeof primes] * (b >= '0' ? codes[b - '0'] : b) ;
	  } ;
	return(result) ;
 }

/*	prsu_array_setup - Sets up token "jump table" arrays	*/

void prsu_array_setup(dst_array,chars_to_set,set_value)
  char dst_array[],chars_to_set[],set_value ;
{ unsigned char i ;
	for (i=0;chars_to_set[i] != NULLV;i++)
	  dst_array[chars_to_set[i]] = set_value ;
	return ;
}

/*	prsu_nest_input - Initializes a new parser input level */
void prsu_nest_input(parse_ptr,file_ptr,prompt_string)
  struct db__parse_info *parse_ptr ;	/* Current parsing environment */
  FILE *file_ptr ;			/* Input file pointer (or NULLV for tty input) */
  char *prompt_string ;			/* Prompt string for terminal input */
{ char i ;

	if (parse_ptr->ilx >= V3_PARSER_INPUT_LEVEL_MAX - 1)
	 prsu_error(ABORT,"MAXLEVELS","Exceeded max number of nested parser input levels") ;
	i = ++(parse_ptr->ilx) ;
	parse_ptr->ilvl[i].is_sos_file = (parse_ptr->ilvl[i].is_not_sos_file = FALSE) ;
	parse_ptr->ilvl[i].current_line = 0 ; parse_ptr->ilvl[i].total_lines = 0 ;
	parse_ptr->ilvl[i].condcomp_nest = (parse_ptr->ilvl[i].condcomp_ignore = 0) ;
	parse_ptr->ilvl[i].free_ptr = 0 ;
/*	Save current level so we can match on eof */
	parse_ptr->ilvl[i].level_on_entry = parse_ptr->level ;
/*	Did we get a file or should we go via terminal */
	strcpy(parse_ptr->ilvl[i].prompt,prompt_string) ;
	parse_ptr->ilvl[i].file = file_ptr ; strcpy(parse_ptr->ilvl[i].file_name,prompt_string) ;
	parse_ptr->ilvl[i].file_page = 1 ; parse_ptr->ilvl[i].last_page_printed = 0 ;
	parse_ptr->need_input_line = TRUE ;		/* Force to new line */
	return ;
}

/*	P A R S E R   I N I T I A L I Z A T I O N	*/

/*	prsu_init_new - Sets up for a new parse environment	*/
struct db__parse_info *prsu_init_new()
{ struct db__parse_info *parse_ptr ;
   struct db__parse_info *save_ptr ;

/*	Allocate & link all necessary junk */
	parse_ptr = (struct db__parse_info *)v4mm_AllocChunk(sizeof *parse_ptr,TRUE) ;
/*	Set up some other stuff */
	parse_ptr->ilx = 0 ;
	parse_ptr->code_offset = 0 ; parse_ptr->pure_free = 1 ;
	parse_ptr->need_input_line = TRUE ; parse_ptr->check_initialized = TRUE ;
	strcpy(parse_ptr->ilvl[0].prompt,"V3>") ;
	parse_ptr->default_radix = 10 ;
/*	Now create a parser "top_level" */
	save_ptr = parse ; parse = parse_ptr ; prs_begin_level("top_level",TRUE,FALSE,FALSE) ; parse = save_ptr ;
	parse_ptr->generate_pic = TRUE ;		/* Generate position-independent code */
/*	And finally return pointer to environment */
	return(parse_ptr) ;
}

/*	C H E C K S U M   T A B L E   E N T R Y		*/

void prsu_checksum_table_insert(type,name)
  char type ;			/* Type of entry: 'M', 'O', 'S' */
  char *name ;			/* Name of the entry */
{
	if (parse->checksums.count >= V3_CHECKSUM_TABLE_MAX)
	 prsu_error(ABORT,"MAXCHKTBL","Exceeded max number of checksum table entries") ;
	parse->checksums.item[parse->checksums.count].type = type ;
	strcpy(parse->checksums.item[parse->checksums.count].name,name) ;
	parse->checksums.item[parse->checksums.count].checksum = parse->current_checksum ;
	parse->checksums.count++ ;
	parse->checksums.length = PRSLEN(&parse->checksums.item[parse->checksums.count],&parse->checksums) ;
}

/*	G E N E R A T E   C O D E   F O R   S Y M B O L I C   R E F E R E N C E S	*/


/*	prsu_gen_symbol_ref - Converts arg to proper V3 code for symbol referencing */

void prsu_gen_symbol_ref(srp)
  struct prs__symbol_ref_list *srp ;
{ struct db__formatmempos setmp,*se,*se1,*se2 ;		/* For symbol ref double-words */
   struct db__dcl_objref *nlp ;	/* Parse name list */
   struct db__dim_info dims ;		/* Constructed dimension info for subscripting */
   struct db__sym_dims *ses ;		/* Points to subscripted symbol def */
   struct db__skeleton_ref skelref ;
   int i,j,k,isptr ;

/*	If got subscripting then clear off val_stack */
	if (srp->got_subs) prsu_chk_oper(NULLV,EOA,RES_NONE,1) ;
/*	If first symbol is stack/arg symbol & got a module_id then convert to XSTACK/XARG */
	setmp.mempos.all = NULLV ;
	if (srp->module_id != 0)
	 { if (srp->sym[1].sp->mempos.fld.value_type == VAL_ARG)
	    { setmp.mempos.xarg.id = VAL_XARG ; setmp.mempos.xarg.frames = srp->frames ;
	      setmp.mempos.xarg.module_id = srp->module_id ; setmp.mempos.xarg.offset = srp->sym[1].sp->mempos.arg.offset ;
	      goto setup ;
	    } ;
	   se = (struct db__formatmempos *)xctu_pointer(srp->sym[1].sp->mempos.all) ;
	   if (se->mempos.fld.value_type == VAL_STACK)
	    { setmp.mempos.xstack.id = VAL_XSTACK ; setmp.mempos.xstack.frames = srp->frames ;
	      setmp.mempos.xstack.module_id = srp->module_id ;
	      if (se->mempos.stack.offset > 0x80000)
	       prsu_error(ERRP,"INVOFFSET","Stack offset too big for [xxx] symbol reference") ;
	      setmp.mempos.xstack.offset = se->mempos.stack.offset ;
	      setmp.format.all = se->format.all ;
	    } ;
	 } ;
/*	See if simple reference (i.e. no "." constructs) */
setup:
	if (srp->count == 1)
/*	   Check to see if we have XSTACK/XARG reference */
	 { if (setmp.mempos.all != NULLV)
	    { PUSHCODE(CODE_PURE,prsu_stash_pure((char *)&setmp,sizeof setmp)) ;
/*	      If XARG then call DUMMYREFX */
	      if (setmp.mempos.fld.value_type == VAL_XARG)
	       { PUSHCODE(CODE_OPERATION,V3_XCT_DUMMYREFX) ; prsu_chk_oper(NULLV,NULLV,RES_UNK,0) ; }
	       else { prsu_chk_value(NULLV) ; } ;
	      goto gen_sym_subs ;
	    } ;
/*	   If the symbol is an simple objref but we have subscripting then kick out here & handle below */
	   if (srp->type == SYMBOL_OBJREF && srp->got_subs) goto compref ;
	   if (srp->type == SYMBOL_SKELETONREF) goto compref ;
	   se = (struct db__formatmempos *)xctu_pointer(srp->sym[1].sp->mempos.all) ; isptr = (se->format.fld.mode == VFM_PTR) ;
	   if (srp->sym[1].sp->mempos.fld.value_type == VAL_STATIMP)
	    { PUSHCODE((isptr ? CODE_IMPUREPV : CODE_IMPUREIV),srp->sym[1].sp->mempos.statimp.offset) ;
	    } else { PUSHCODE((isptr ? CODE_PUREPV : CODE_PUREIV),srp->sym[1].sp->mempos.statpure.offset) ; } ;
	   prsu_chk_value(srp->sym[1].sp) ; goto gen_sym_subs ;
	 } ;
/*	Have a more complicated reference - figure out how to handle */
compref:
	switch (srp->type)
	 { default: prsu_error(INTERNAL+POINT,"BADSYMTYP","Invalid symbol type at prsu_gen_symbol_ref") ;
	   case SYMBOL_STRUCTREF:
/*		First figure out offset to final symbol element */
		j = 0 ;
		for (i=2;i<=srp->count;i++)
		 { se2 = (struct db__formatmempos *)xctu_pointer(srp->sym[i].sp->mempos.all) ; j += se2->mempos.all ; } ;
/*		Now create a reference using last element format & first element loc + offset */
/*		se1 = pointer to first sym in list, se2 = pointer to last */
		se1 = (struct db__formatmempos *)xctu_pointer(srp->sym[1].sp->mempos.all) ;
		setmp.format.all = se2->format.all ; setmp.format.fld.mode = VFM_PTR ;
		if (setmp.mempos.all == NULLV) setmp.mempos.all = se1->mempos.all ;
		if (setmp.mempos.fld.value_type == VAL_STACK)
		 { setmp.mempos.stack.offset += j ;
		   PUSHCODE(CODE_PUREPV,prsu_stash_pure((char *)&setmp,sizeof setmp)) ; prsu_chk_value(srp->sym[1].sp) ;
		 } else
		 { if (setmp.mempos.fld.value_type != VAL_STATIMP)
		    { if (setmp.mempos.fld.value_type != VAL_STATIMPLM)
		       prsu_error(ERRP,"REFNOTIMP","Expecting reference to static/stack storage") ;
		      setmp.mempos.all = setmp.mempos.statimp.offset << 2 ;	/* Convert from int to byte offset */
		      setmp.mempos.all += j ;
		      PUSHCODE(CODE_PUREPVLMO,prsu_stash_pure((char *)&setmp,sizeof setmp)) ; prsu_chk_value(srp->sym[1].sp) ;
		    } else
		    { setmp.mempos.all = setmp.mempos.statimp.offset << 2 ;	/* Convert from int to byte offset */
		      setmp.mempos.all += j ;
		      PUSHCODE(CODE_PUREPVIO,prsu_stash_pure((char *)&setmp,sizeof setmp)) ; prsu_chk_value(srp->sym[1].sp) ;
		      if (parse->is_module_bind)
		       prsu_error(ERRP,"IMPREF","Cannot reference global impure within this (V4) module") ;
		   } ;
		 } ;
		goto gen_sym_subs ;
	   case SYMBOL_STRUCTPTR:
/*		Generate code of form -
		  (pointer symbol) (pure ptr to last element format,,offset) (V3_XCT_STRUCTPTR)	*/
		if (setmp.mempos.all != NULLV)
		 { PUSHCODE(CODE_PUREPV,prsu_stash_pure((char *)&setmp,sizeof setmp)) ; }
	         else { j = (srp->sym[1].sp->mempos.fld.value_type == VAL_STATIMP ? CODE_IMPUREPV : CODE_PUREPV) ;
			PUSHCODE(j,srp->sym[1].sp->mempos.statpure.offset) ;
		      } ;
/*		Now figure out offsets from pointer we just pushed */
		j = 0 ;
		for (i=2;i<=srp->count;i++)
		 { se2 = (struct db__formatmempos *)xctu_pointer(srp->sym[i].sp->mempos.all) ; j += se2->mempos.all ; } ;
/*		Create literal value for doubleword descriptor */
		setmp.format.all = se2->format.all ; setmp.mempos.all = j ;
		isptr = (setmp.format.fld.mode == VFM_PTR) ;
		PUSHCODE((isptr ? CODE_PUREPV : CODE_PUREIV),prsu_stash_pure((char *)&setmp,sizeof setmp)) ;
		prsu_chk_value(srp->sym[1].sp) ;
		PUSHCODE(CODE_FREQOP,V3_XCT_STRUCTPTR) ; goto gen_sym_subs ;
	   case SYMBOL_OBJREF:
/*		Generate code of form -
		  (objref sym) (XCT_OBJREF[SS]) (statpure ptr to name list) */
		prsu_chk_oper(NULLV,NULLV,RES_UNK,0) ;
		j = (srp->sym[1].sp->mempos.fld.value_type == VAL_STATIMP ? CODE_IMPUREPV : CODE_PUREPV) ;
		PUSHCODE(j,srp->sym[1].sp->mempos.statpure.offset) ;
		PUSHCODE(CODE_OPERATION,(srp->got_subs ? V3_XCT_OBJREFSS : V3_XCT_OBJREF)) ;
/*		Make a list of names & append to pure area */
		i = prsu_alloc_pure(1 + 4*srp->count-1,ALIGN_WORD) ; nlp = (struct db__dcl_objref *)&parse->pure_buf[i] ;
		PUSHVAL(i) ; nlp->count = srp->count-1 ;
		for (i=2;i<=srp->count;i++)
		 { nlp->name_ids[i-2].all = srp->sym[i].obname.all ; } ;
		if (srp->got_subs) { PUSHCODE(CODE_OPERATION,V3_XCT_SUBSCRIPT_OBJ) ; } ;
		return ;
	   case SYMBOL_SKELETONREF:
/*		Generate code of the form-
		  XCT_EVAL_SKELETON (statpure to skelref) */

		skelref.skel_index = srp->skel_index ; skelref.el_index = srp->last_el ;
		se = (struct db__formatmempos *)xctu_pointer(srp->sym[1].sp->mempos.all) ; skelref.base_where.all = se->mempos.all ;
		PUSHCODE(CODE_OPERATION,(srp->got_subs ? V3_XCT_EVAL_SKELETON_SS : V3_XCT_EVAL_SKELETON)) ;
		parse->code[parse->code_offset++] = prsu_stash_pure((char *)&skelref,sizeof skelref) ;
/*		Add an unknown value onto value stack */
		parse->level->val_stack[parse->level->valx++] = RES_UNK ;
		if (srp->got_subs) { PUSHCODE(CODE_OPERATION,V3_XCT_SUBSCRIPT_OBJ) ; } ;
		return ;
	 } ;
 /*	Here to handle references involving subscripting */
gen_sym_subs:
/*	If no subscripting then return now */
	if (!srp->got_subs)
	 { if (srp->ignore_subs) return ;
/*	   Best make sure subscripting not required */
	   for((j=0,i=1);i<=srp->count;i++) { j += srp->sym[i].sp->ss_count ; } ;
	   if (j > 0) prsu_error(WARNP,"MISSINGSS","Subscripting (or \'[]\') missing on this symbol/structure") ;
	   return ;
	 } ;
/*	Got subscripting, make sure ok */
	if (srp->ignore_subs)
	 prsu_error(WARNP,"INVSS","Cannot mix and match subscripting with \'[]\' construct") ;
	dims.count = 0 ;
/*	Loop thru all symbols in list & get dimension lengths */
	for (i=1;i<=srp->count;i++)
	 { ses = (struct db__sym_dims *)xctu_pointer(srp->sym[i].sp->mempos.all) ;
	   for (j=0;j<srp->sym[i].sp->ss_count;j++)
	    { dims.dimlen[dims.count++] = ses->ssentry[j].dimlen ; } ;
	 } ;
	if (dims.count == 0)
	 prsu_error(WARNP,"NODIMSYM","Cannot subscript a non-dimensioned symbol") ;
/*	Now generate proper code */
	PUSHCODE(CODE_OPERATION,V3_XCT_SUBSCRIPT) ; PUSHVAL(prsu_stash_pure((char *)&dims,(2+(2*dims.count)))) ;
	return ;
}

/*	O P E R A T O R   /   V A L U E   C H E C K S		*/

/*	prsu_chk_oper - Called AFTER PUSHCODE to check on args & results of an operation/module */

void prsu_chk_oper(opip,flags,results,args)
  struct db__opr_info *opip ;	/* NULLV or pointer to operator info */
  int flags ;			/* Parser flags (i.e. EOA & ARITH) */
  int results ;			/* Results of operation - RES_xxx */
  int args ;			/* (Minimum) number of arguments required */
{ union db__module_code_element *mce ;
  int opres,cnt ;
  int i,oldx ; char tmp[200] ;

/*	First see if we got opip or other args */
	if (opip != NULLV)
	 { results = opip->results ; args = opip->args ;
	   flags = ( (opip->end_of_arg_list ? EOA : 0) | (opip->arithmetic ? ARITH : 0)
		    |(opip->update_arg1 ? UPD1 : (opip->update_arg2 ? UPD2 : (opip->update_arg3 ? UPD3 : 0)))
		    |(opip->noreference ? NOREF : 0)
	           ) ;
	 } ;
/*	See if we have enough arguments/operands */
	for (cnt=1;cnt <= parse->level->valx && parse->level->val_stack[parse->level->valx-cnt] != RES_EOA;cnt++) ;
	cnt-- ;
pco1:
	if (cnt < args)
	 prsu_error(WARNP,"INSUFFARGS","Insufficient number of operands/arguments") ;
/*	That's ok, see if we need arithmetic check */
	if ((flags & ARITH) != 0)
	 { if ( (parse->level->val_stack[parse->level->valx-1] != RES_INT) ||
		(args == 1 ? FALSE : parse->level->val_stack[parse->level->valx-2] != RES_INT) )
/*	     Both args are not integer, twiddle opcode to next higher to perform general arithmetic */
	      { mce = (union db__module_code_element *)&parse->code[parse->code_offset-1] ; mce->fld.offset ++ ; } ;
	 } ;
/*	Now go thru arguments (stripping them off stack) */
	opres = RES_INT ; oldx = parse->level->valx ;
	if ((flags & EOA) != 0)
	 { for (;parse->level->valx > 0 && parse->level->val_stack[parse->level->valx-1] != RES_EOA;parse->level->valx--)
	    { if (parse->level->val_stack[parse->level->valx-1] != RES_INT) opres = RES_UNK ; } ;
	   parse->level->valx-- ;
	 } else
	 { for (;args > 0;(args--,parse->level->valx--))
	    { if (parse->level->val_stack[parse->level->valx-1] != RES_INT) opres = RES_UNK ;
	      if (parse->level->val_stack[parse->level->valx-1] == RES_EOA)
		prsu_error(ERRP,"BADVALSTACK","Invalid arguments or messed up V3 compiler") ;
	     } ;
	 } ;
/*	Check out references to all arguments */
	for(i=parse->level->valx;i<oldx;i++)
	 { if (parse->level->sym_stack[i] == NULLV) continue ;
/*	   Got a real symbol pointer, see if symbol has been updated */
	   if (parse->level->sym_stack[i]->initialized) continue ;
/*	   If we are within a command then ignore since commands executed out-of-sequence */
	   if (parse->level->in_command) continue ;
/*	   Symbol has not, see if updated by this module or maybe an error */
	   if ( (flags & UPD1) && i == parse->level->valx)
	    { parse->level->sym_stack[i]->initialized = TRUE ; continue ; } ;
	   if ( (flags & UPD2) && i == parse->level->valx+1)
	    { parse->level->sym_stack[i]->initialized = TRUE ; continue ; } ;
	   if ( (flags & UPD3) && i == parse->level->valx+2)
	    { parse->level->sym_stack[i]->initialized = TRUE ; continue ; } ;
	   if (flags & NOREF) continue ;	/* No error if value not really referenced (sys_address) */
/*	   Symbol has not yet been updated, generate an error */
	   parse->level->sym_stack[i]->initialized = TRUE ;
	   sprintf(tmp,"Symbol (%s) referenced before being initialized",parse->level->sym_stack[i]->name) ;
	   prsu_error(WARN,"SYMNOTUPD",tmp) ;
	 } ;
/*	Finally push on the result */
pco2:
	switch (results)
	 { default: prsu_error(ERRP,"BADRES","Invalid results argument to prsu_chk_oper") ;
	   case RES_NONE: opres = RES_NONE ; break ;
	   case RES_UNK: opres = RES_UNK ; break ;
	   case RES_INT: opres = RES_INT ; break ;
	   case RES_ARGS: break ;
	   case RES_EOA: opres = RES_EOA ; break ;
	 } ;
	if (opres != RES_NONE)
	 { parse->level->sym_stack[parse->level->valx] = NULLV ;
	   parse->level->val_stack[parse->level->valx++] = opres ;
	 } ;
	return ;
}

/*	prsu_chk_value - Called AFTER PUSHCODE to see what type of value was pushed */

void prsu_chk_value(sym_ptr)
  struct db__prs_sym *sym_ptr ;		/* Pointer to symbol entry or NULLV */
{ union db__module_code_element ce ;
   union val__format *fp ;
   int opres ;

	ce.all = parse->code[parse->code_offset - 1] ;
	switch (ce.fld.type)
	 { default: prsu_error(ERRP,"INVOPCODE","Invalid V3 opcode detected in prsu_chk_value") ;
	   case CODE_PUREPVLMO:
	   case CODE_PUREPVIO:
	   case CODE_PUREIV:
	   case CODE_PUREPV:
	   case CODE_PURE:
		fp = (union val__format *)&parse->pure_buf[ce.fld.offset] ; break ;
	   case CODE_IMPUREIV:
	   case CODE_IMPUREPV:
	   case CODE_IMPURE:
		fp = (union val__format *)&parse->impure_buf[ce.fld.offset] ; break ;
	   case CODE_INT_LITERAL:
		opres = RES_INT ; goto pc1 ;	/* We know its an integer ! */
	 } ;
/*	Check it out to see if integer */
	opres = (((fp->fld.type == VFT_BININT || fp->fld.type == VFT_BINWORD || fp->fld.type == VFT_BINLONG)
			 && (fp->fld.decimals == 0))
		  ? RES_INT : RES_UNK) ;
pc1:
	parse->level->val_stack[parse->level->valx] = opres ;
	parse->level->sym_stack[parse->level->valx++] = sym_ptr ;
	return ;
}

/*	P A R S E R   S T A T I S T I C S   R O U T I N E S	*/

/*	prsu_show_size - Shows size of current parse tables	*/

void prsu_show_size()
{ int i ;

/*	Dump out package size information */
	printf("Buffer Sizes-\n") ;
	printf("  Pure = %d/%d\n",parse->pure_free*SIZEofINT,V3_PRS_PURE_BUF_SIZE) ;
	printf("  Impure = %d/%d\n",parse->impure_free*SIZEofINT,V3_PRS_IMPURE_BUF_SIZE) ;
	printf("  ImpureLM = %d/%d\n",parse->impurelm_free*SIZEofINT,V3_PRS_IMPURE_BUF_SIZE) ;
	printf("  Struct = %d/%d\n",parse->struct_bytes,V3_PRS_STRUCT_BUF_SIZE) ;
	printf("  dcl value/macro = %d/%d\n",parse->constants.free_offset,V3_PRS_CONSTANT_BUF_SIZE) ;
	printf("Symbol Table Sizes-\n") ;
	printf("  Globals = %d/%d\n",parse->globals.count,V3_PRS_SYMBOL_TABLE_MAX_SMALL) ;
	printf("  Structures = %d/%d\n",parse->structures.count,V3_PRS_SYMBOL_TABLE_MAX_SMALL) ;
	printf("  Locals = %d/%d\n",parse->locals.count,V3_PRS_SYMBOL_TABLE_MAX_BIG) ;
	printf("  Externals = %d\n",parse->externals.count) ;
	printf("  Labels = %d\n",parse->labels.count) ;
	printf("  Elements = %d\n",parse->elements.count) ;
	printf("  User Flags = %d/%d\n",parse->user_flag_count,V3_PRS_USER_FLAG_MAX) ;
	printf("  Checksums = %d/%d\n",parse->checksums.count,V3_CHECKSUM_TABLE_MAX) ;
	printf("Object Sizes-\n") ;
	printf("  Number = %d/%d\n",parse->ob_bucket.count,V3_SOB_OBS_PER_BUCKET_MAX) ;
	printf("  Buffer space = %d/%d\n",parse->ob_bucket.free_offset*SIZEofINT,V3_SOB_BUCKET_BUF_MAX) ;
	printf("  Name Table = %d/%d\n",parse->name_table.count,V3_SOB_NAME_TABLE_MAX) ;
	printf("  Assertions = %d/%d\n",parse->assertion_table.count,V3_PRS_SOB_NAME_MAX) ;
	for(i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++)
	 { if (parse->name_ob_pairs[i].num_names > 0)
	    printf("  Name/Ob Table #%d = %d/%d\n",i,parse->name_ob_pairs[i].num_names,V3_NAME_OB_PAIR_MAX) ;
	 } ;
	printf("Code Generation-\n") ;
	printf("  Code elements = %d/%d\n",parse->code_offset,V3_PRS_CODE_BUF_MAX) ;
	printf("  Parser Levels Assigned = %d\n",parse->last_level_id) ;
	printf("  Modules created = %d\n",parse->last_module_id) ;
	printf("  dcl values = %d/%d\n",parse->constants.count,V3_PRS_CONSTANT_MAX) ;
	printf("  Number of literals = %d/%d\n",parse->lit.count,V3_PRS_LITERAL_MAX) ;
}

/*	P A R S E R   E R R O R   H A N D L E R		*/

/*	Here to trap all parser errors			*/

void prsu_error(flags,code,message)
  int flags ;			/* See Error constants */
  char *code ;
  char *message ;
{ char tmp[V3_PRS_INPUT_LINE_MAX] ; int i,j,nx ; char *ptr ;

/*	See if we should print out the offending source file */
	if (parse->ilvl[parse->ilx].file_name[0] != NULLV)
	 { if (parse->ilvl[parse->ilx].last_page_printed != parse->ilvl[parse->ilx].file_page)
	    { parse->ilvl[parse->ilx].last_page_printed = parse->ilvl[parse->ilx].file_page ;
	      printf("%s, page %d -\n",parse->ilvl[parse->ilx].file_name,parse->ilvl[parse->ilx].file_page) ;
	    } ;
	 } ;
/*	Print current input line for user */
	if (parse->ilvl[parse->ilx].is_sos_file)
	 { printf("%5d	%s",parse->ilvl[parse->ilx].current_line,parse->ilvl[parse->ilx].input_str) ; }
	 else
	 { if (parse->ilvl[parse->ilx].is_not_sos_file)
	    { printf("Module %s+%d -\n",parse->module_name,parse->ilvl[parse->ilx].current_line) ;
	      printf("%5d	%s",parse->ilvl[parse->ilx].total_lines,parse->ilvl[parse->ilx].input_str) ;
	    } else
	    { printf("	%s\n",parse->ilvl[parse->ilx].input_str) ; } ;
	 } ;
/*	Should we point to offending whatever ? */
	if (flags & POINT)
	 { strcpy(tmp,parse->ilvl[parse->ilx].input_str) ;
/*	   Set up to make pointer */
	   if (parse->constant_bkp_count != 0)
	    { j = (char *)parse->constant_input_ptr_bkp[1] - (char *)(&(parse->ilvl[parse->ilx].input_str)) ; }
	    else j = (char *)parse->ilvl[parse->ilx].input_ptr - (char *)(&(parse->ilvl[parse->ilx].input_str)) ;
	   if (j > sizeof tmp) j = 0 ;
	   for (i=0;i<j-1;i++)
	    { if (tmp[i] > ' ') tmp[i] = ' ' ; } ;
	   tmp[i] = '^' ; tmp[++i] = NULLV ;
	   printf("	%s\n",tmp) ;
/*	   If in value/macro expansion then dump it out also */
	   for(nx=parse->constant_bkp_count;nx>0;nx--)
	    { printf("+	%s\n",parse->constant_start_ptr[nx]) ;
	      strcpy(tmp,parse->constant_start_ptr[nx]) ;
	      ptr = (nx == parse->constant_bkp_count ? parse->ilvl[parse->ilx].input_ptr : parse->constant_input_ptr_bkp[nx]) ;
	      for (i=0;i<ptr-parse->constant_start_ptr[nx]-1;i++)
	       { if (tmp[i] > ' ') tmp[i] = ' ' ; } ;
	      tmp[i] = '^' ; tmp[++i] = NULLV ; printf("	%s\n",tmp) ;
	    } ;
	 } ;
/*	Print out the error message */
	printf("%%V3PRS-%s, %s\n\n",code,message) ;
/*	See if we are at end of current line */
	if (type_of_token[*parse->ilvl[parse->ilx].input_ptr] == END_OF_LINE || type_of_token[*(parse->ilvl[parse->ilx].input_ptr+1)] == END_OF_LINE)
	 parse->need_input_line = TRUE ;
/*	Increment error count */
	parse->error_cnt++ ; parse->error_flag = TRUE ;
	process->parser_errors++ ;
	if (parse->error_cnt > V3_PARSER_ERROR_MAX)
	 { printf("\n%%V3PRS-F-Excessive errors forcing premature termination of compilation\n") ;
	   longjmp(parse_exit,2) ;
	 } ;
/*	Now handle depending on severity */
	switch (flags & 511)
	 { case WARN:	return ;
	   case ERROR:	parse->ilvl[parse->ilx].input_ptr++ ; longjmp(parse_exit,1) ;
	   case INTERNAL: parse->ilvl[parse->ilx].input_ptr++ ; longjmp(parse_exit,1) ;
	   case ABORT:	parse->ilvl[parse->ilx].input_ptr++ ; longjmp(parse_exit,2) ;
	   default:
		v3_error(V3E_BADFLAGS,"PRS","ERROR","BADFLAGS","Invalid flags argument to prsu_error",NULLV) ;
	 } ;
}
