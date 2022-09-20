/*	V3PRSB.C - PARSING UTILITIES FOR VICTIM III

	LAST EDITED 6/29/84 BY VICTOR E. HANSEN	*/

#include <time.h>
#include "v3defs.c"

/*	Global Definitions for Parser		*/
extern struct db__parse_info *parse ;
extern struct db__tknres tkn ;
extern jmp_buf parse_exit ;
struct V4DPI__Point *v4dpi_IsctEval() ;

extern struct db__predef_table V3_FUNCTIONS[] ;
extern int PRS_FUNCTION_COUNT ;
#ifdef windows
extern struct db__predef_table V3_WINDOW_MODULES[] ;
extern int PRS_WINDOW_COUNT ;
#endif
extern struct db__predef_table V3_RESERVED_WORDS[] ;

/*	Global References for Execution		*/
extern struct db__process *process ;
extern struct db__psi *psi ;

extern LONGINT notbl_lparen,notbl_rparen,notbl_lbrace,notbl_rbrace,notbl_star,notbl_slash,notbl_equal,notbl_dot_dot,notbl_dot,
	notbl_comma,notbl_plus,notbl_minus,notbl_langle,notbl_rangle,notbl_colon_equal,notbl_semi,notbl_slash_slash,
	notbl_partial,notbl_lbracket,notbl_rbracket,notbl_lrbracket,notbl_cross ;

struct prsu__v4b v4b ;		/* Binding elements for V4 */

/*	H A N D L E   D C L   O F   S Y M B O L S		*/

/*	Declare info for different VICTIM III Data types  */

#define ALL_IND 99		/* Special code for "dcl all" */
#define BITMAP_IND 100		/* Special code for "dcl bitmap" */
#define SKEL_IND 101		/* Special code for skeleton entry */
#define SKELSTRUCT_IND 102	/* Special code for defining skeleton */
#define NIL_IND 103		/* Ditto for defining nil element */

#define AI_BYTE 0
#define AI_SHORT 1
#define AI_WORD 2
#define AI_LONG 3
#define AI_DOUBLE 4
#define AI_POINTER 5
#define AI_MAX 6

struct {
  char name[20] ;		/* Data type name */
  unsigned char vft ;		/* Data type VFT_xxx (ex: VFT_BININT) */
  short int bytes ;		/* Number of bytes (or zero is variable) */
  unsigned dps_ok : 1 ;		/* If TRUE then can specify decimal places */
  unsigned char align_mask ;	/* Address alignment mask */
  unsigned char align_index ;	/* Index to proper alignment (if aligning struct for foreign system) */
 } prs_dcl_type[] =
    {	{"ALL",ALL_IND,99,FALSE,0,AI_BYTE},
	{"ALPHA",VFT_FIXSTR,0,FALSE,ALIGN_BYTE,AI_BYTE},
	{"ALPHAIA",VFT_FIXSTR,0,FALSE,ALIGN_WORD,AI_WORD},
	{"BITMAP",BITMAP_IND,0,FALSE,ALIGN_WORD,AI_WORD},
	{"DEC",VFT_STRINT,0,TRUE,ALIGN_BYTE,AI_BYTE},
	{"DECIMAL",VFT_STRINT,0,TRUE,ALIGN_BYTE,AI_BYTE},
	{"FD",VFT_FLOAT,8,FALSE,ALIGN_DOUBLE,AI_DOUBLE},
	{"IB",VFT_BININT,SIZEofINT,TRUE,ALIGN_WORD,AI_WORD},
	{"IC",VFT_STRINT,0,TRUE,ALIGN_BYTE,AI_BYTE},
	{"IL",VFT_BINLONG,(sizeof (long int)),TRUE,ALIGN_LONG,AI_LONG},
	{"INT",VFT_BININT,SIZEofINT,FALSE,ALIGN_WORD,AI_WORD},
#ifdef LONG64
	{"IP",VFT_BINLONG,(sizeof (LONGINT)),TRUE,ALIGN_DOUBLE,AI_DOUBLE},
#else
	{"IP",VFT_V3NUM,sizeof (union val__v3num),TRUE,ALIGN_DOUBLE,AI_DOUBLE},
#endif
/*	{"IP",VFT_PACDEC,0,TRUE,ALIGN_BYTE,AI_BYTE},		Treat IP as V3NUM for now */
	{"IW",VFT_BINWORD,2,TRUE,ALIGN_SHORT,AI_SHORT},
	{"NIL",NIL_IND,1,FALSE,ALIGN_BYTE,AI_BYTE},		/* Length of 1 is temporary! */
	{"OBJREF",VFT_OBJREF,sizeof (struct db__formatptr),FALSE,ALIGNofPTR,AI_POINTER},
	{"POINTER",VFT_POINTER,SIZEofPTR,FALSE,ALIGNofPTR,AI_POINTER},
	{"REAL",VFT_FLOAT,8,FALSE,ALIGN_DOUBLE,AI_DOUBLE},
	{"SC",VFT_FIXSTR,0,FALSE,ALIGN_BYTE,AI_BYTE},
	{"SD",VFT_FIXSTR,0,FALSE,ALIGN_BYTE,AI_BYTE},
	{"SF",VFT_FIXSTR,0,FALSE,ALIGN_BYTE,AI_BYTE},
	{"SHORT",VFT_BINWORD,2,FALSE,ALIGN_SHORT,AI_SHORT},
	{"SKEL",SKEL_IND,99,FALSE,0,AI_BYTE},
	{"SN",VFT_VARSTR,0,FALSE,ALIGN_BYTE,AI_BYTE},
	{"SR",VFT_INDIRECT,sizeof (struct db__formatptr),FALSE,ALIGNofPTR,AI_POINTER},
	{"STRING",VFT_VARSTR,0,FALSE,ALIGN_BYTE,AI_BYTE},
	{"STRING_REF",VFT_INDIRECT,sizeof (struct db__formatptr),FALSE,ALIGNofPTR,AI_POINTER},
	{"V3NUM",VFT_V3NUM,sizeof (union val__v3num),TRUE,ALIGNofPTR,AI_POINTER},
	{"V4POINT",VFT_FIXSTR,sizeof (struct V4DPI__Point),FALSE,ALIGNofPTR,AI_POINTER},
    } ;
#define PRS_DCL_TYPE_COUNT 29	/* Must match number of above */

/*	prsu_dcl - Handles VICTIM III DCL Statements */
int prsu_dcl(globalflag)
  int globalflag ;		/* If TRUE then force defined symbols to be global (from auto-V4 load) */
{ struct db__var_desc vd ;	/* Updated with descriptive info */
   struct db__prs_skeleton skel,*skelp ;
   struct db__dsd dsd ;		/* Hold parsing element info */
   int aivals[AI_MAX] ;
   char lbuf[300],rbuf[300] ; char mbuf[50] ;
   FILE *fp ;
   int i,j,k,m,*ptr ; int dimels,struct_bytes ; int ex,sx ; char namebuf[150],keyparts[10] ;
   int redef ;				/* If nonnegative then element redefines another within structure */
   int have_all ;			/* Set to indicate "ALL" */
   int have_skel ;			/* Flag for "got SKEL elements" */
   int skel_index ;
   int set_init ;			/* Set to TRUE if symbol dcl'ed with ">" */
   char structname[V3_PRS_SYMBOL_MAX+1] ;	/* Temp symbol name buffer */
   char symbol_name[V3_PRS_SYMBOL_MAX+1] ; /* Ditto */
   char checksum_entry_name[V3_PRS_SYMBOL_MAX+1] ; /* If not null then name of top level structure name */
   int checksum ;
   struct db__prs_sym *sp,*spr ;	/* Points to symbol table entry */
   int elbytes ;			/* Number of bytes for element */
   struct db__sym_dims *ses,*ser ;

	aivals[AI_BYTE] = ALIGN_BYTE ; aivals[AI_SHORT] = ALIGN_SHORT ; aivals[AI_WORD] = ALIGN_WORD ;
	aivals[AI_LONG] = ALIGN_LONG ; aivals[AI_DOUBLE] = ALIGN_DOUBLE ; aivals[AI_POINTER] = ALIGNofPTR ;
/*	Get the next token & check for special keywords */
#define IFSYM(str) if (strcmp(tkn.symbol,str) == 0)
	prsu_nxt_token(FALSE) ;
	IFSYM("OBJ")
	 { prsu_dcl_obj() ; return(TRUE) ; } ;
	IFSYM("VALUE") { prsu_dcl_constant(FALSE,FALSE) ; return(TRUE) ; } ;
	IFSYM("VALUE_PRIME") { prsu_dcl_constant(FALSE,TRUE) ; return(TRUE) ; } ;
	IFSYM("MACRO") { prsu_dcl_constant(TRUE,FALSE) ; return(TRUE) ; } ;
	IFSYM("RADIX") return(prsu_dcl_radix()) ;
	IFSYM("CHECKSUMS") return(prsu_dcl_checksums()) ;
	IFSYM("STARTUP_MODULE") return(prsu_dcl_startup_module()) ;
	IFSYM("PACKAGE_ID") return(prsu_dcl_package_id()) ;
	IFSYM("PACKAGE_SLOT") return(prsu_dcl_package_slot()) ;
/*	Initialize vd.xxx	*/
	if (parse->level->interpretive || globalflag)
	 { vd.table = PRS_GLOBAL_SYM ; vd.val = VAL_STATIMP ; }
	 else { vd.table = PRS_LOCAL_SYM ; vd.val = VAL_STACK ; } ;
	vd.type = SYMBOL_NORMAL ; vd.vft = (vd.srchid = (vd.strid = (vd.subs = (vd.dps = 0)))) ;
/*	Init dsd.xxx	*/
	parse->dcl_nest = (dsd.level[0].length = 0) ; dsd.no_alignment = FALSE ;
	checksum_entry_name[0] = NULLV ;
/*	Zap skeleton structure */
	skel.count = 0 ; skel.skel_type = 0 ;
	IFSYM("FLAG") return(prsu_dcl_flag()) ;
	IFSYM("INCLUDE") return(prsu_dcl_include()) ;
	IFSYM("V4B") return(prsu_dcl_v4b()) ;
	IFSYM("V4EVAL") return(prsu_dcl_v4eval()) ;
	IFSYM("V4MODULEEVAL") return(prsu_dcl_v4moduleeval()) ;
	IFSYM("V4MODULEBIND") return(prsu_dcl_v4modulebind()) ;
	IFSYM("V4STRUCTEVAL") return(prsu_dcl_v4structeval()) ;
	IFSYM("NOPIC")
	 { parse->generate_pic = FALSE ;
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
 	    prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	   return(TRUE) ;
	 } ;
	IFSYM("GLOBAL") { vd.table = PRS_GLOBAL_SYM ; vd.val = VAL_STATIMP ; goto dcl_var_type ; } ;
	IFSYM("SHOW_SIZES")
	 { prsu_show_size() ;
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
 	    prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	   return(TRUE) ;
	 } ;
	IFSYM("REFERENCE_CHECKING") return(prsu_dcl_reference_checking()) ;
	IFSYM("CLONE")
	 { prsu_nxt_token(FALSE) ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"INVSYMNAME","Expecting a symbol name here") ;
	   strcpy(structname,tkn.symbol) ;
	   for(i=FALSE;;)
	    { if ((sp = (struct db__prs_sym *)prsu_sym_lookup(structname,0)) != NULLV) break ;
	      if (i) prsu_error(ERRP,"UNDEFSYM","This symbol is undefined") ;
	      i = TRUE ;
	      if (!prs_attempt_struct_load(structname)) continue ;
/*	      Next token should be start of structure dcl, pop off "dcl" */
	      prsu_nxt_token(FALSE) ;
	      if (strcmp(tkn.symbol,"DCL") != 0) prsu_error(ERRP,"NOTDCL","Expecting macro structure expansion to begin with DCL") ;
	      prsu_dcl(TRUE) ;			/* Parse structure & try again */
	    } ;
/*	   Loop thru all/any elements to get to last element */
	   ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ;
	   for (;;)
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot) { prsu_nxt_token(TRUE) ; break ; } ;
	      if (sp->search_struct_id == 0)
	       prsu_error(ERRP,"NOTSTRREF","Cannot use dot (\'.\') after terminal symbol") ;
/*	      Parse element & do lookup */
	      prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVSTRUCTEL","Expecting a structure element here") ;
	      if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,sp->search_struct_id)) == NULLV)
	       prsu_error(ERRP,"SYMNOTEL","Symbol not defined as STRUCT element") ;
	      ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ;
	    } ;
/*	   Now just pull vd.vft, vd.dps, and vd.bytes from this symbol */
	   vd.vft = ses->format.fld.type ; vd.dps = ses->format.fld.decimals ; vd.bytes = ses->format.fld.length ;
	   vd.align_mask = (dsd.no_alignment ? 0 : sp->align_mask) ;
	   goto dcl_sym_list ;
	 } ;
	IFSYM("PARTIAL")
	 { prsu_nxt_token(FALSE) ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"NOTUSERSYM","Expecting a user symbol here") ;
/*	   Define the symbol as local structure pointer */
	   sp = (struct db__prs_sym *)prsu_sym_define(tkn.symbol,SYMBOL_STRUCTPTR,PRS_LOCAL_SYM,VFT_FIXSTR,VAL_STACK,SIZEofPTR,SIZEofPTR,0,0,0,0,0,ALIGNofPTR) ;
/*	   Push pointer to this sym on parse temp stack & notbl_partial on top of it */
	   PUSHTMP(sp) ; PUSHTMP(&notbl_partial) ;
/*	   Now generate code for symbol */
	   PUSHCODE(CODE_PUREPV,sp->mempos.statpure.offset) ; prsu_chk_value(NULLV) ;
/*	   Flag this symbol as initialized */
	   sp->initialized = TRUE ;
	   prsu_nxt_token(FALSE) ; if (tkn.sym_ptr != (struct db__opr_info *)&notbl_equal) prsu_nxt_token(TRUE) ;
/*	   And finally return to generate code for partial reference */
	   return(FALSE) ;
	 } ;
	IFSYM("STATIC") { vd.val = (vd.table == PRS_GLOBAL_SYM ? VAL_STATIMP : VAL_STATIMPLM) ; goto dcl_var_type ; } ;
	IFSYM("MODULE")
	 { vd.bytes = 8 ; vd.val = VAL_STATIMP ; vd.table = PRS_GLOBAL_SYM ; vd.type = SYMBOL_MODULE ; goto dcl_sym_list ; } ;
	IFSYM("ASSERTIONS") return(prsu_dcl_assertions()) ;
	IFSYM("STRUCT")
	 { prsu_nxt_token(FALSE) ;
/*	   Should we align within the structure ? */
	   IFSYM("NOALIGNMENT") { dsd.no_alignment = TRUE ; prsu_nxt_token(FALSE) ; } ;
	   IFSYM("ALPHAOSF")
	    { aivals[AI_BYTE] = 0 ; aivals[AI_SHORT] = 1 ; aivals[AI_WORD] = 3 ;
	      aivals[AI_LONG] = 7 ; aivals[AI_DOUBLE] = 7 ; aivals[AI_POINTER] = 7 ;
	      prsu_nxt_token(FALSE) ;
	    } ;
	   IFSYM("VAXALIGNMENT")
	    { aivals[AI_BYTE] = ALIGN_BYTE ; aivals[AI_SHORT] = ALIGN_BYTE ; aivals[AI_WORD] = ALIGN_BYTE ;
	      aivals[AI_LONG] = ALIGN_BYTE ; aivals[AI_DOUBLE] = ALIGN_BYTE ; aivals[AI_POINTER] = ALIGN_BYTE ;
	      prsu_nxt_token(FALSE) ;
	    } ;
/*	   Look for "GLOBAL" or "STATIC"	*/
	   IFSYM("GLOBAL")
	    { vd.table = PRS_GLOBAL_SYM ; vd.val = VAL_STATIMP ; }
	    else
	    { IFSYM("STATIC")
	       { vd.val = (vd.table == PRS_GLOBAL_SYM ? VAL_STATIMP : VAL_STATIMPLM); }
	       else { prsu_nxt_token(TRUE) ; } ;
	    } ;
/*	   Maybe get a dimension name */
	   sprintf(dsd.level[parse->dcl_nest].dim,"STRUCT%d",parse->dcl_nest) ;
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen)		/* Got (xxx) after data type = dimension */
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVDIMNAME","Expecting a dimension name here") ;
	      if (strlen(tkn.symbol) > V3_PRS_SYMBOL_MAX-1)
	       prsu_error(ERRP,"INVDIMLEN","Dimension name is too long") ;
	      strcpy(dsd.level[parse->dcl_nest].dim,tkn.symbol) ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen) prsu_error(ERRP,"INVDIMSPEC","Expecting dimension to end with \")\"") ;
	      prsu_nxt_token(FALSE) ;
	    } ;
/*	   Now get the structure name */
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"NOTSTRUCTNAME","Expecting a STRUCT name here") ;
	   strcpy(structname,tkn.symbol) ; prsu_nxt_token(FALSE) ;
/*	   Check for "=nnn" indicating an FileRef linkage */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_equal)
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_BININT || tkn.lit_format.fld.decimals != 0)
	       prsu_error(ERRP,"INVDCLPCKSLOT","Expecting an AUXVAL id number here") ;
	      if (tkn.value.int_lit < 0 || tkn.value.int_lit >= 0xFFFF)
	       prsu_error(ERRP,"INVAUXVAL","Invalid AUXVAL value") ;
	      memset(&v4b,0,sizeof(v4b)) ; v4b.fileref = tkn.value.int_lit ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma)		/* Maybe got ",dimname" for elements */
	       { prsu_nxt_token(FALSE) ;
		 if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVAUXDIM","Expecting dimension name here") ;
		 strcpy(v4b.eldim,tkn.symbol) ; prsu_nxt_token(FALSE) ;
	         if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma)		/* Maybe got ",prelude=xxx" */
	          { prsu_nxt_token(FALSE) ;
	            if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	             prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
		    strncpy(v4b.prelude,tkn.value.char_lit,tkn.literal_len) ; v4b.prelude[tkn.literal_len] = 0 ;
		    prsu_nxt_token(FALSE) ;
	          } ;
	       } ;
	    } else v4b.fileref = 0 ;
/*	   Check for "(" indicating a skeleton definition
	     Syntax is: dcl struct name [(object)]	*/
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen)
	    { for (ptr = (int *)&skel,i=0;i<(sizeof skel)/SIZEofINT;i++) { *(ptr++) = 0 ; } ;
	      strcpy(skel.skel_name,structname) ; skel.master_object.all = NULLV ; skel.skel_type = SKELETON_FULL ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.type == TKN_SYMBOL)
	       { skel.master_object.all = sobu_name_create(tkn.symbol,parse->package_id) ;
		 prsu_nxt_token(FALSE) ; skel.skel_type = SKELETON_PARTIAL ;
	       } ;
/*	      If we got "(*)" then make partial but no object name */
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_star)
	       { prsu_nxt_token(FALSE) ;
		 skel.skel_type = SKELETON_PARTIAL ; skel.master_object.all = NULLV ;
	       } ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen)
	       prsu_error(ERRP,"INVSKELSYN","Expecting a closing parenthesis or comma here") ;
	      goto dcl_start_struct ;
	    } ;
/*	   Not a skeleton, do we have another name or a "{" ? */
	   prsu_nxt_token(TRUE) ;
	   if ((tkn.type != TKN_SYMBOL) && (tkn.sym_ptr != (struct db__opr_info *)&notbl_star) && (tkn.sym_ptr != (struct db__opr_info *)&notbl_rangle))
	    goto dcl_start_struct ;
/*	   Have another name, means last one better be a structure */
	   for(i=2;i;i--)
	    { if ((sp = (struct db__prs_sym *)prsu_sym_lookup(structname,0)) != NULLV) break ;
	      if (i != 2) prsu_error(ERRP,"DCLSTRUCTNOTDEF","Symbol not defined as STRUCT !") ;
	      if (!prs_attempt_struct_load(structname)) continue ;
/*	      Next token should be start of structure dcl, pop off "dcl" */
	      prsu_nxt_token(FALSE) ;
	      if (strcmp(tkn.symbol,"DCL") != 0) prsu_error(ERRP,"NOTDCL","Expecting macro structure expansion to begin with DCL") ;
	      prsu_dcl(TRUE) ;			/* Parse structure & try again */
	    } ;
/*	   If structure is a skeleton then jump out now */
	   if (sp->type == SYMBOL_SKELETON)
	    { vd.vft = SKELSTRUCT_IND ; skel_index = sp->search_struct_id ; goto dcl_sym_list ; } ;
	   if (sp->type != SYMBOL_STRUCTURE)
	    prsu_error(ERRP,"SYMNOTSTRUCT","DCL STRUCT xxx - Symbol previously defined to be other than STRUCT") ;
	   vd.strid = sp->structure_id ; vd.vft = VFT_FIXSTR ; vd.align_mask = (dsd.no_alignment ? 0 : sp->align_mask) ;
/*	   Get the length of this new element from struct length */
	   ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ; vd.bytes = ses->format.fld.length ;
/*	   And finally the search id for its elements */
	   vd.srchid = sp->search_struct_id ;
	   goto dcl_sym_list ;
	 } ;
/*	Not one of the above - push back */
	prsu_nxt_token(TRUE) ; goto dcl_var_type ;

/*	Here to define a new STRUCT	*/
dcl_start_struct:
	if (parse->dcl_nest >= V3_PRS_STRUCT_LEVEL_MAX)
	 prsu_error(ERRP,"MAXNESTEDSTRUCTS","Exceeded max number of nested STRUCTs") ;
/*	If this is top level structure declared from interpretive then save name for checksum */
	if (parse->level->interpretive && parse->dcl_nest == 0)
	 { strcpy(checksum_entry_name,structname) ;
	   parse->token_count = 0 ; parse->current_checksum = 0 ;
	   if (v4b.fileref > 0 && parse->v4b_fileptr != 0)
	    fprintf(parse->v4b_fileptr,"STRUCT( %d, %s )\n",v4b.fileref,structname) ;
	 } ;
	parse->dcl_nest++ ;	/* Bump up nesting level */
	redef = 0 ;		/* Starting offset for this structure */
/*	Are we in the midsts of a skeleton ? */
	if (skel.skel_type != 0)
	 { if (tkn.sym_ptr == (struct db__opr_info *)&notbl_equal)
	    prsu_error(ERRP,"INVREDEF","Cannot redefine (\'=\') within skeleton STRUCT") ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lbracket)
	    prsu_error(ERRP,"INVDIM","Cannot dimension within skeleton STRUCT") ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lrbracket) prsu_nxt_token(FALSE) ;
/*	   If not top level then copy this element */
	   if (parse->dcl_nest > 1)
	    { if (skel.count >= V3_PRS_SKELETON_ELEMENT_MAX)
	       prsu_error(ERRP,"TOOMNYELS","Exceeded max number of skeleton structure elements") ;
	      skel.el[skel.count].el_name.all = sobu_name_create(structname,parse->package_id) ;
	      skel.el[skel.count].object = TRUE ;
	      skel.el[skel.count++].level = parse->dcl_nest - 1 ;
	    } ;
/*	   Now make sure we got a "{" */
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lbrace)
	    prsu_error(ERRP,"NOTLBRACE","Expecting a left brace (\'{\') here") ;
	   goto dcl_var_type ;
	 } ;
/*	Check for "=" followed by a structure reference for starting offset */
	if (tkn.sym_ptr == (struct db__opr_info *)&notbl_equal)
	 { prsu_nxt_token(FALSE) ;	/* Get rid of the "=" */
	   if (parse->dcl_nest > 1)
	    prsu_error(ERRP,"INVSTRUCTOFFSET","STRUCT offset only allowed on top-level structure") ;
/*	   Have to parse reference to get offset */
	   prsu_nxt_token(FALSE) ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"INVSTRUCTNAME","Expecting a STRUCT name here") ;
	   if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,0)) == NULLV)
	    prsu_error(ERRP,"UNDEFSTRUCT","This symbol is undefined") ;
	   if (sp->type != SYMBOL_STRUCTURE)
	    prsu_error(ERRP,"SYMNOTSTRUCT","This symbol is not the name of a STRUCT") ;
	   for (;;)
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot) { prsu_nxt_token(TRUE) ; break ; } ;
/*	      Parse element & do lookup */
	      prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVSTRUCTEL","Expecting a structure element here") ;
	      if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,sp->search_struct_id)) == NULLV)
	       prsu_error(ERRP,"SYMNOTEL","Symbol not defined as STRUCT element") ;
	      ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ; redef += ses->offset ;
	    } ;
	 } ;
/*	Check for subscripts */
	dimels = prsu_dcl_subs(&dsd) ;
	if (parse->dcl_nest == 1 && dimels > 0)
	 prsu_error(ERRP,"NOSUBSONTOP","Top level STRUCT cannot be subscripted") ;
	dsd.level[parse->dcl_nest].numels = dimels ;
	dsd.level[parse->dcl_nest].sym_ptr = (sp =
	  (struct db__prs_sym *)prsu_sym_define(structname,SYMBOL_STRUCTURE,(vd.table==PRS_GLOBAL_SYM ? PRS_STRUCTDEF_SYM : vd.table),VFT_FIXSTR,
	  vd.val,0,0,dsd.sscnte,NODPS,vd.strid,dsd.level[parse->dcl_nest-1].length,++(parse->last_struct_level_id),vd.align_mask)) ;
/*	Now reset some vd.xxx symbols for structure elements */
	vd.table = PRS_LOCAL_SYM ; vd.val = 0 ; vd.align_mask = 0 ;
	dsd.level[parse->dcl_nest].strid = (vd.strid = parse->last_struct_level_id) ;
/*	Set up some level info */
	dsd.level[parse->dcl_nest].length = redef ; dsd.level[parse->dcl_nest].max_align_mask = 0 ;
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_PREDEFINED || tkn.sym_ptr != (struct db__opr_info *)&notbl_lbrace)
	 prsu_error(ERRP,"NOTLBRACE","Expecting a left brace (\'{\') here") ;
	goto dcl_var_type ;
/*	Here to look for symbol type (ex: ib 4.3) */
dcl_var_type:
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_SYMBOL)
	 prsu_error(ERRP,"INVDCLTYPE","Expecting DCL type (IB, ALPHA, ...) or STRUCT here") ;
/*	Look up in dcl_type table */
	for (i=0;i<PRS_DCL_TYPE_COUNT;i++)
	 { IFSYM(prs_dcl_type[i].name) goto found_type ; } ;
/*	If none of the above then check for STRUCT again */
	IFSYM("STRUCT")
	 { prsu_nxt_token(FALSE) ;
	   sprintf(dsd.level[parse->dcl_nest].dim,"STRUCT%d",parse->dcl_nest) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen)		/* Got (xxx) after data type = dimension */
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVDIMNAME","Expecting a dimension name here") ;
	      if (strlen(tkn.symbol) > V3_PRS_SYMBOL_MAX-1)
	       prsu_error(ERRP,"INVDIMLEN","Dimension name is too long") ;
	      strcpy(dsd.level[parse->dcl_nest].dim,tkn.symbol) ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen) prsu_error(ERRP,"INVDIMSPEC","Expecting dimension to end with \")\"") ;
	    } else prsu_nxt_token(TRUE) ;
/*	   Now get the structure name */
	   prsu_nxt_token(FALSE) ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"INVSTRUCTNAME","Expecting a STRUCT name here") ;
	   strcpy(structname,tkn.symbol) ;
/*	   Now do we have another name, *name, or a "{" ? */
	   prsu_nxt_token(FALSE) ; prsu_nxt_token(TRUE) ;
	   if ((tkn.type != TKN_SYMBOL) && (tkn.sym_ptr != (struct db__opr_info *)&notbl_star)) goto dcl_start_struct ;
/*	   Have another name, means last one better be a structure */
	   for(i=2;i;i--)
	    { if ((sp = (struct db__prs_sym *)prsu_sym_lookup(structname,0)) != NULLV) break ;
	      if (i != 2) prsu_error(ERRP,"DCLSTRUCTNOTDEF","Symbol not defined as STRUCT !") ;
	      if (!prs_attempt_struct_load(structname)) continue ;
/*	      Next token should be start of structure dcl, pop off "dcl" */
	      prsu_nxt_token(FALSE) ;
	      if (strcmp(tkn.symbol,"DCL") != 0) prsu_error(ERRP,"NOTDCL","Expecting macro structure expansion to begin with DCL") ;
	      j = parse->dcl_nest ;		/* Save this bad boy */
	      prsu_dcl(TRUE) ;			/* Parse structure & try again */
	      parse->dcl_nest = j ;
	    } ;
	   if (sp->type != SYMBOL_STRUCTURE)
	    prsu_error(ERRP,"SYMNOTSTRUCT","DCL STRUCT xxx - Symbol previously defined to be other than STRUCT") ;
	   vd.vft = VFT_FIXSTR ; vd.align_mask = (dsd.no_alignment ? 0 : sp->align_mask) ;
/*	   Get the length of this new element from struct length */
	   ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ; vd.bytes = ses->format.fld.length ;
/*	   And finally the search id for its elements */
	   vd.srchid = sp->search_struct_id ;
	   goto dcl_sym_list ;
	 } ;
/*	Best to check for "CLONEs" again */
	IFSYM("CLONE")
	 { prsu_nxt_token(FALSE) ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"INVSYMNAME","Expecting a symbol name here") ;
	   strcpy(structname,tkn.symbol) ;
	   for(i=FALSE;;)
	    { if ((sp = (struct db__prs_sym *)prsu_sym_lookup(structname,0)) != NULLV) break ;
	      if (i) prsu_error(ERRP,"UNDEFSYM","This symbol is undefined") ;
	      i = TRUE ;
	      if (!prs_attempt_struct_load(structname)) continue ;
/*	      Next token should be start of structure dcl, pop off "dcl" */
	      prsu_nxt_token(FALSE) ;
	      if (strcmp(tkn.symbol,"DCL") != 0) prsu_error(ERRP,"NOTDCL","Expecting macro structure expansion to begin with DCL") ;
	      j = parse->dcl_nest ; prsu_dcl(TRUE) ; parse->dcl_nest = j ;	/* Parse structure & try again */
	    } ;
/*	   Loop thru all/any elements to get to last element */
	   ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ;
	   for (;;)
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot) { prsu_nxt_token(TRUE) ; break ; } ;
	      if (sp->search_struct_id == 0)
	       prsu_error(ERRP,"NOTSTRREF","Cannot use dot (\'.\') after terminal symbol") ;
/*	      Parse element & do lookup */
	      prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	      if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVSTRUCTEL","Expecting a structure element here") ;
	      if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,sp->search_struct_id)) == NULLV)
	       prsu_error(ERRP,"SYMNOTEL","Symbol not defined as STRUCT element") ;
	      ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ;
	    } ;
/*	   Now just pull vd.vft, vd.dps, and vd.bytes from this symbol */
	   vd.vft = ses->format.fld.type ; vd.dps = ses->format.fld.decimals ; vd.bytes = ses->format.fld.length ;
	   vd.align_mask = (dsd.no_alignment ? 0 : sp->align_mask) ;
	   goto dcl_sym_list ;
	 } ;
	prsu_error(ERRP,"INVDCLTYPE","Expecting DCL type (IB, ALPHA, ...) or STRUCT here") ;
found_type:
/*	Update vd.xxx with info for this data type */
	vd.vft = prs_dcl_type[i].vft ; vd.dps = 0 ;
/*	Check for ALL keyword */
	if (vd.vft == ALL_IND)
	 { if (parse->dcl_nest <= 0)
	    prsu_error(ERRP,"INVALLUSAGE","ALL only allowed within STRUCT definition") ;
	   if (dsd.level[parse->dcl_nest].length == 0)
	    prsu_error(ERRP,"ALLATBEGIN","\'ALL\' at begin of [sub]structure is meaningless") ;
/*	   Check for optional "#xxx" following */
	   prsu_nxt_token(FALSE+TKN_FLAGS_NOVALUE) ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_cross) { prsu_nxt_token(TRUE) ; }
	    else { prsu_nxt_token(FALSE) ;
		   if (tkn.type != TKN_SYMBOL)
		    prsu_error(ERRP,"BADVALSYM","Expecting a symbolic value to follow \'#\'") ;
		   if (parse->constants.count >= V3_PRS_CONSTANT_MAX-1)
		    prsu_error(ERRP,"TOOMANYVALS","Too many parser values have been defined") ;
		   strcpy(parse->constants.sym[parse->constants.count].name,tkn.symbol) ;
		   parse->constants.sym[parse->constants.count].hash = prsu_str_hash(tkn.symbol,-1) ;
/*		   Convert current struct length to string */
		   parse->constants.sym[parse->constants.count].offset = parse->constants.free_offset ;
		   sprintf(tkn.value.char_lit,"%d",dsd.level[parse->dcl_nest].length) ;
		   strcpy(&parse->constants.buf[parse->constants.free_offset],tkn.value.char_lit) ;
		   parse->constants.free_offset += (1+strlen(tkn.value.char_lit)) ;
		   parse->constants.count++ ;
		 } ;
	 } ;
/*	If a regular type (e.g. "IB") then can't be a structure name */
/*	vd.srchid = 0 ; vd.align_mask = (dsd.no_alignment ? 0 : prs_dcl_type[i].align_mask) ; */
	vd.srchid = 0 ; vd.align_mask = (dsd.no_alignment ? 0 : aivals[prs_dcl_type[i].align_index]) ;
/*	Should we check for "len.dps" ? */
	if (prs_dcl_type[i].bytes == 0 || prs_dcl_type[i].dps_ok)
	 { if (prsu_eval_int_exp(FALSE,&tkn.value.int_lit)) { tkn.lit_format.fld.decimals = 0 ; }
	    else { prsu_nxt_token(FALSE) ;
		   if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_BININT)
		    { if (prs_dcl_type[i].vft == VFT_V3NUM)
		       { prsu_nxt_token(TRUE) ; tkn.lit_format.fld.decimals = 1 ; tkn.value.int_lit = 152 ; }
		       else prsu_error(ERRP,"INVDCLNUM","Expecting number of form \'nn\' or \'nn.n\'") ;
		    } ;
		 } ;
	   if (tkn.value.int_lit < 0)
	    prsu_error(ERRP,"INVDCLNUMVAL","Negative numbers not allowed in DCL statements") ;
	   vd.bytes = tkn.value.int_lit ;
/*	   Go for decimal places ? */
	   if (prs_dcl_type[i].dps_ok)
	    { if (tkn.lit_format.fld.decimals > 0)
	       { if (tkn.lit_format.fld.decimals > 1)
		  prsu_error(ERRP,"INVDCLNUMDP","Number can only be to single decimal place") ;
		 vd.dps = vd.bytes % 10 ; vd.bytes /= 10 ;
	       } ;
	    } else
	    { if (tkn.lit_format.fld.decimals > 0)
	       prsu_error(ERRP,"DPSNOTOK","This data type does not accept decimal places") ;
	    } ;
	   switch (vd.vft)
	    { case VFT_BININT:
		if (vd.bytes > 9) prsu_error(ERRP,"DCLNUMVALSIZE","Max size of IB is 9 significant digits") ; break ;
	      case VFT_BINWORD:
		if (vd.bytes > 4) prsu_error(ERRP,"DCLNUMVALSIZE","Max size of IW is 4 significant digits") ; break ;
	      case VFT_BINLONG:
	      case VFT_V3NUM:
		if (vd.bytes > 18) prsu_error(ERRP,"DCLNUMVALSIZE","Max size of IP is 18 significant digits") ; break ;
	    } ;
/*	   Override vd.bytes if necessary */
	   if (prs_dcl_type[i].bytes > 0) vd.bytes = prs_dcl_type[i].bytes ;
	 } else
	 { vd.bytes = prs_dcl_type[i].bytes ; if (vd.bytes < 0) vd.bytes = 0 ; } ;
/*	Have all information about variables - now parse symbol list */
dcl_sym_list:
	if (v4b.fileref > 0)
	 { strcpy(v4b.el[v4b.count].dim,"UNKNOWN") ;		/* Default to "unknown" dimension for element */
	   v4b.el[v4b.count].bind = FALSE ; v4b.el[v4b.count].keynum = 0 ; v4b.el[v4b.count].iscount = FALSE ;
	 } ;
	prsu_nxt_token(FALSE+TKN_FLAGS_NOVALUE) ;
	if (tkn.sym_ptr == (struct db__opr_info *)&notbl_langle)		/* Got (xxx) after data type = dimension */
	 { if (v4b.fileref <= 0) prsu_error(ERRP,"INVDIMSPEC","Can only specify (dim) within structure with AUXVAL") ;
	   prsu_nxt_token(FALSE) ;
	   if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVDIMNAME","Expecting a dimension name here") ;
	   if (strlen(tkn.symbol) > V3_PRS_SYMBOL_MAX-1)
	    prsu_error(ERRP,"INVDIMLEN","Dimension name is too long") ;
	   strcpy(v4b.el[v4b.count].dim,tkn.symbol) ;
/*	   Parse optional (dim [,bind] [,keyn]) */
	   for(;;)
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rangle) break ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_comma)
	       prsu_error(ERRP,"INVDIMSPEC","Expecting dimension to continue with \",\" or end with \")\"") ;
	      prsu_nxt_token(FALSE) ;
	      IFSYM("BIND") { v4b.el[v4b.count].bind = TRUE ; continue ; } ;
	      IFSYM("KEY1") { v4b.el[v4b.count].keynum = 1 ; continue ; } ;
	      IFSYM("KEY2") { v4b.el[v4b.count].keynum = 2 ; continue ; } ;
	      IFSYM("KEY3") { v4b.el[v4b.count].keynum = 3 ; continue ; } ;
	      IFSYM("KEY4") { v4b.el[v4b.count].keynum = 4 ; continue ; } ;
	      IFSYM("COUNTER") { v4b.el[v4b.count].iscount = TRUE ; continue ; } ;
	      prsu_error(ERRP,"INVDIMSPEC","Expecting \"BIND\" or \"KEYn\" keyword here") ;
	    } ;
	 } else { prsu_nxt_token(TRUE) ; } ;
/*	If DCLing a packed decimal string then adjust length */
	if (vd.vft == VFT_PACDEC) vd.bytes = (vd.bytes+2)/2 ;
/*	If DCLing a variable string then allocate extra byte for null */
	if (vd.vft == VFT_VARSTR) vd.bytes++ ;
/*	If DCLing a bitmap then twiddle appropriately */
	if (vd.vft == BITMAP_IND)
	 { vd.vft = VFT_FIXSTR ; vd.bytes = ((((vd.bytes+7)/8)+3)/SIZEofINT)*SIZEofINT + SIZEofINT ; } ;
/*	If DCLing a nil entry then handle below */
	if (vd.vft == NIL_IND)
	 { vd.vft = VFT_V3INT ; vd.dps = VFTV3_NULL ; vd.bytes = 0 ; } ;
/*	Check for the "ALL" again */
	if (vd.vft != ALL_IND) { have_all = FALSE ; }
	 else { have_all = TRUE ;
		vd.vft = VFT_FIXSTR ; vd.bytes = dsd.level[parse->dcl_nest].length ;
	      } ;
	if (vd.vft != SKEL_IND) { have_skel = FALSE ; }
	 else { if (parse->dcl_nest <= 0)
		 prsu_error(ERRP,"INVSKEL","SKEL allowed only within STRUCT definition") ;
		have_skel = TRUE ;
	      } ;
	for(;;)
	 { prsu_nxt_token(vd.strid == 0 ? FALSE : FALSE+TKN_FLAGS_STREL) ;
/*	   Check for special "is-initialized" flags (">") */
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rangle) { set_init = FALSE ; }
	    else { set_init = TRUE ; prsu_nxt_token(vd.strid == 0 ? FALSE : FALSE+TKN_FLAGS_STREL) ; } ;
	   strcpy(symbol_name,tkn.symbol) ;
/*	   If declaring a skeleton structure instance (dcl struct skeleton xxx) */
	   if (vd.vft == SKELSTRUCT_IND)
	    { if (tkn.sym_ptr != (struct db__opr_info *)&notbl_star)
	       prsu_error(ERRP,"SKELHASSTAR","Skeleton STRUCT instances must be pointers prefaced with star (\'*\')") ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_SYMBOL)
	       prsu_error(ERRP,"INVNAME","Invalid skeleton STRUCT instance name") ;
	      prsu_sym_define(tkn.symbol,SYMBOL_SKELETONREF,vd.table,VFT_STRUCTPTR,vd.val,SIZEofPTR,SIZEofPTR,0,0,0,0,skel_index,ALIGNofPTR) ;
	      goto end_element ;
	    } ;
/*	   If defining elements within structure definition then handle here */
	   if (skel.skel_type != 0)
	    { if (skel.count >= V3_PRS_SKELETON_ELEMENT_MAX)
	       prsu_error(ERRP,"TOOMNYELS","Exceeded max number of skeleton structure elements") ;
/*	      Save element depending on type of skeleton structure */
	      if (skel.skel_type == SKELETON_PARTIAL)
	       { if (!have_skel) prsu_error(ERRP,"NOTSKEL","Only SKEL elements allowed within partial skeleton STRUCT") ;
		 skel.el[skel.count].el_name.all = sobu_name_create(tkn.symbol,parse->package_id) ;
		 skel.el[skel.count].level = parse->dcl_nest ;
		 skel.el[skel.count++].object = TRUE ;
		 goto end_element ;
	       } ;
/*	      Have Full Skeleton, may have other skeleton reference or real reference */
	      if (have_skel)
	       { for(sx=0;sx<parse->skeletons.count;sx++)
		  { skelp = (struct db__prs_skeleton *)&(parse->skeletons.buf[parse->skeletons.index[sx]]) ;
		    if (strcmp(skelp->skel_name,tkn.symbol) == 0) break ;
		   } ;
		 if (sx >= parse->skeletons.count)
		  prsu_error(ERRP,"UNDEFSKEL","Skeleton STRUCT element not name of existing skeleton STRUCT") ;
/*		 Got skeleton index (sx), now find skeleton element */
		 prsu_nxt_token(FALSE) ;
		 if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot)
		  prsu_error(ERRP,"SKELDOT","Expecting skeleton name to be followed by dot (\'.\')") ;
		 prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
		 ex = 0 ; j = 1 ;
another_element:
		 i = sobu_name_lookup(tkn.symbol,parse->package_id) ;
		 for(;ex<skelp->count;ex++)
		  { if (skelp->el[ex].el_name.all == i) break ; } ;
		 if (ex >= skelp->count)
		  prsu_error(ERRP,"NOTEL","Symbol not element of skeleton STRUCT") ;
/*		 Is this the end of the reference ? */
		 strcpy(symbol_name,tkn.symbol) ; prsu_nxt_token(FALSE) ;
		 if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot) { prsu_nxt_token(TRUE) ; }
		  else { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
			 if (tkn.type != TKN_SYMBOL)
			  prsu_error(ERRP,"NOTEL","Expecting a skeleton element here") ;
			 ex++ ; goto another_element ;
		       } ;
		 skel.el[skel.count].skel_index = sx ; skel.el[skel.count].el_index = ex ;
		 skel.el[skel.count].el_name.all = sobu_name_create(symbol_name,parse->package_id) ;
		 skel.el[skel.count].level = parse->dcl_nest ;
		 skel.el[skel.count++].skelref = TRUE ;
		 goto end_element ;
	       } ;
/*	      Must have regular reference (e.g. "int xxx") */
	      skel.el[skel.count].format.all = prsu_formref(vd.vft,VFM_OFFSET,vd.bytes,vd.dps) ;
	      skel.el[skel.count].el_name.all = sobu_name_create(symbol_name,parse->package_id) ;
	      skel.el[skel.count].level = parse->dcl_nest ;
/*	      Check for "all" and redefines */
	      if (have_all) { skel.el[skel.count].all = TRUE ; } ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_equal) { prsu_nxt_token(TRUE) ; }
	       else { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
		      if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVREDEF","Must have skeleton element after \'=\'") ;
		      i = sobu_name_lookup(tkn.symbol,parse->package_id) ;
		      for(ex=0;ex<skel.count;ex++) { if (i == skel.el[ex].el_name.all) break ; } ;
		      if (ex >= skel.count) prsu_error(ERRP,"INVREDEF","Symbol is not part of this skeleton STRUCT") ;
		      skel.el[skel.count].redefines = TRUE ; skel.el[skel.count].el_index = ex ;
		    } ;
	      skel.el[skel.count++].defined = TRUE ;
	      goto end_element ;
	    } else
	    { if (have_skel)
	       prsu_error(ERRP,"INVSKEL","SKEL allowed only within definition of skeleton STRUCTs") ;
	    } ;
	   struct_bytes = 0 ;	/* This is nonzero only if we get a STRUCTPTR */
/*	   Figure out if structure reference/pointer/normal for symbol type */
	   if (vd.srchid == 0)
/*	      Just a regular symbol (or element) */
	    { if (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_star)
	       prsu_error(ERRP,"INVSTARTPOS","Pointers only allowed for STRUCTs") ;
/*	      Look for module declaration of form "sym()"	*/
	      if (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen)
	       { prsu_nxt_token(FALSE) ;
		 if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen) prsu_error(ERRP,"MISSINGRPAREN","Expecting \')\' here") ;
		 if (vd.strid != 0)
		  prsu_error(ERRP,"INVMODDCL","Cannot DCL module within STRUCT definition") ;
		 vd.type = SYMBOL_MODULE ;
	       } ;
/*	      Make symbol "normal" unless already set to module */
	      if (vd.type != SYMBOL_MODULE) vd.type = SYMBOL_NORMAL ;
/*	      If this is an object reference then flag with special symbol type */
	      if (vd.vft == VFT_OBJREF) vd.type = SYMBOL_OBJREF ;
	    } else
	    { if (tkn.sym_ptr == (struct db__opr_info *)&notbl_star)
	       { vd.type = SYMBOL_STRUCTPTR ; prsu_nxt_token(FALSE) ; strcpy(symbol_name,tkn.symbol) ;
		 struct_bytes = vd.bytes ; vd.bytes = SIZEofPTR ; vd.align_mask = (dsd.no_alignment ? 0 : ALIGNofPTR) ;
	       }
	       else { vd.type = SYMBOL_STRUCTREF ; } ;
	    } ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"NOTUSERSYM","Expecting a user symbol here") ;
/*	   Check for any subscripts */
	   dimels = prsu_dcl_subs(&dsd) ;
	   if (dimels > 0 && have_all)
	    prsu_error(ERRP,"NOSUBSALL","Subscripting not allowed with \'ALL\'") ;
/*	   Check for the redefines symbol (sym = sym)	*/
	   redef = -1 ; prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_equal) { prsu_nxt_token(TRUE) ; }
	    else
	    { if (vd.strid == 0)
	       prsu_error(ERRP,"NOTINSTRUCT","Redefines allowed only within STRUCT definition") ;
	      if (have_all)
	       prsu_error(ERRP,"NOREDEF","Redefines not allowed with \'ALL\'") ;
	      prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
/*	      Allow redefines to be a structure element or integer number */
	      if (tkn.type != TKN_SYMBOL)
	       { prsu_nxt_token(TRUE) ;
		 if (! prsu_eval_int_exp(FALSE,&tkn.value.int_lit))
		  prsu_error(ERRP,"INVSYMBOL","Expecting a structure element name or integer expression here") ;
/*		 Have integer, if greater than current length then extend otherwise make it a redefines */
		 if (tkn.value.int_lit-1 >= dsd.level[parse->dcl_nest].length)
		  { dsd.level[parse->dcl_nest].length = tkn.value.int_lit-1 ; }
		  else { if ((redef = tkn.value.int_lit-1) < 0) redef = 0 ; } ;
	       } else
	       { if ((spr = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,vd.strid)) == 0)
		  prsu_error(ERRP,"NOTSTRUCTEL","This symbol not defined within parent STRUCT") ;
		  ser = (struct db__sym_dims *)xctu_pointer(spr->mempos.all) ; redef = ser->offset ;
	       } ;
	    } ;
/*	   If we have "ALL" then fake a redefines */
	   if (have_all) redef = 0 ; /* To begin of structure */
/*	   Maybe align offset within structure */
	   dsd.level[parse->dcl_nest].length = (dsd.level[parse->dcl_nest].length + vd.align_mask) & ~vd.align_mask ;
/*	   Track "highest" (most aligned) member within structure */
	   dsd.level[parse->dcl_nest].max_align_mask |= vd.align_mask ;
	   sp = (struct db__prs_sym *)prsu_sym_define(symbol_name,vd.type,vd.table,(struct_bytes > 0 ? VFT_STRUCTPTR : vd.vft),vd.val,vd.bytes,
		  (dimels > 0 ? vd.bytes*dimels : vd.bytes),
		  dsd.sscnte,vd.dps,vd.strid,(redef >= 0 ? redef : dsd.level[parse->dcl_nest].length),vd.srchid,vd.align_mask) ;
/*	   Have symbol entry - if subscripts then update */
	   ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ;
/*	   Set the "is-initialized" flag to TRUE if flagged with ">" or not stack value */
	   sp->initialized = set_init ;
	   if (vd.val != VAL_STACK || (!parse->check_initialized)) sp->initialized = TRUE ;
/*	   If just created a STRUCTPTR then correct its length */
	   if (struct_bytes > 0) ses->format.fld.length = struct_bytes ;
	   j = vd.bytes ;	/* If multiple dimensions then start with rightmost at element length */
	   for (i=dsd.sscnte-1;i>=0;i--)
	    { if (j > 0xffff) prsu_error(ERRP,"SSTOOBIG","Subscripting too large, try reversing dimensions") ;
	      ses->ssentry[i].dimlen = j ; j *= (ses->ssentry[i].max = dsd.ssnum[i]) ;
	    } ;
/*	   Now update level length & offset with total length of this element */
	   i = (dimels == 0 ? vd.bytes : dimels*vd.bytes) ;
/*	   If we just got a "*xxx" symbol then reset vd.bytes */
	   if (struct_bytes > 0) vd.bytes = struct_bytes ;
/*	   Make sure the redefines was OK */
	   if (redef > 0 ? (redef & vd.align_mask) != 0 : FALSE)
	    prsu_error(ERRP,"INVREDALGN","Address alignment for this datatype lost with redefinition") ;
	   if (redef < 0)
	    { dsd.level[parse->dcl_nest].length += i ; }
	    else { if (redef+i-1 > dsd.level[parse->dcl_nest].length) dsd.level[parse->dcl_nest].length = redef+1-1 ; } ;
/*	   Now best get a "," or ";" (also allow a "}") 	*/
end_element:
	   prsu_nxt_token(FALSE) ;
	   if (tkn.type == TKN_LITERAL && tkn.lit_format.fld.type == VFT_FIXSTR)
	    { if (tkn.literal_len >= sizeof(v4b.el[0].comment)) tkn.literal_len = sizeof(v4b.el[0].comment)-1 ;
	      strncpy(v4b.el[v4b.count-1].comment,tkn.value.char_lit,tkn.literal_len) ;
	      v4b.el[v4b.count-1].comment[tkn.literal_len] = 0 ;
	      prsu_nxt_token(FALSE) ;
	    } ;
	   if (tkn.type == TKN_PREDEFINED)
	    { if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma)
	       { if (v4b.fileref > 0)					/* If capturing V4 info, copy dim name to next element */
		  { strcpy(v4b.el[v4b.count].dim,v4b.el[v4b.count-1].dim) ; } ;
		 continue ;
	       } ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_semi) break ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rbrace)
	       { prsu_nxt_token(TRUE) ; break ; } ;
	    } ;
	   prsu_error(ERRP,"INVDCLSYMTERM","Expecting DCL list to continue with comma or end with semicolon") ;
	 } ;
/*	Here at end of DCL list - return unless still in STRUCT */
dcl_end_list:
/*	If a skeleton has been defined then add to parser buffer */
	if (skel.count > 0 && parse->dcl_nest == 0)
	 { if (parse->skeletons.count >= V3_PRS_SKELETON_ENTRY_MAX)
	    prsu_error(ERRP,"TOOMNYSKELS","Exceeded max number of skeleton STRUCT definitions for package") ;
	   i = PRSLEN(&skel.el[skel.count+1].el_name,&skel) ;
	   if (parse->skeletons.free_byte + i >= V3_PRS_SKELETON_BUF_MAX)
	    prsu_error(ERRP,"MAXBUF","Exceeded maximum buffer space in package skeleton structure") ;
	   parse->skeletons.free_byte = (parse->skeletons.free_byte + ALIGN_WORD) & ~ALIGN_WORD ;
	   memcpy(&parse->skeletons.buf[parse->skeletons.free_byte],&skel,i) ;
	   parse->skeletons.index[parse->skeletons.count] = parse->skeletons.free_byte ;
	   parse->skeletons.free_byte += i ;
	   parse->skeletons.length = PRSLEN(&parse->skeletons.buf[parse->skeletons.free_byte],&parse->skeletons) ;
/*	   Add this symbol to table */
	   prsu_sym_define(skel.skel_name,SYMBOL_SKELETON,PRS_STRUCTDEF_SYM,0,0,0,0,0,0,0,0,parse->skeletons.count,0) ;
/*	   If checksums are enabled then calculate & add to table */
	   if (parse->enable_checksums)
	    { for(j=i/SIZEofINT,ptr = (int *)&skel,checksum=0;j>0;j--)
	       { checksum += (j * *ptr) ; } ;
	      prsu_checksum_table_insert('K',skel.skel_name) ;
	    } ;
/*	   Finally update skeleton count */
	   parse->skeletons.count++ ;
	   return(TRUE) ;
	 } ;
/*	If end of structure then maybe update a checksum */
#define EI(V3DIM,V4DIM) else if (strcmp(v4b.el[i].dim,V3DIM) == 0) { strcpy(v4b.el[i].dim,V4DIM) ; }
	if (parse->dcl_nest == 0)
	 { if (checksum_entry_name[0] != NULLV && parse->enable_checksums)
	    prsu_checksum_table_insert('S',checksum_entry_name) ;
	   if (v4b.fileref > 0 && parse->v4b_fileptr != 0)
	    { j = v4b.el[v4b.count-1].ownerof-1 ;			/* Adjustment so primary owner is 1 */
	      ex = 0 ;							/* Will be set to Element# of last seen "iscount" */
	      memset(keyparts,0,sizeof keyparts) ;			/* Set all slots to zip */
	      for (i=0;i<v4b.count;i++)
	       { if (strcmp(v4b.el[i].dim,"EXEC_KO_DATE_TIME") == 0) { strcpy(v4b.el[i].dim,"UDT") ; }
	          EI("EXEC_KO_DATE","UDate") EI("EXEC_KO_MONTH","UMonth") EI("APO_GEN_PERIOD","UPeriod") EI("APO_IM_REF","IM")
		  EI("APO_IM_LOCATION","Loc") EI("APO_VENDOR_REF","Ven") EI("APO_CUS_REF","Cus") EI("APO_WO_REF","WO")
		  EI("APO_TERMS_REF","Term") EI("APO_BRANCH_REF","Branch") EI("APO_EMP_REF","Emp") EI("APO_GLNUM_REF","GLMas")
		  EI("APO_BO_REF","BO") EI("APO_OE_REF","OE") EI("APO_OSI_REF","OSI") EI("APO_WO_REF","WO")
		  EI("APO_PONUM_REF","PO") EI("APO_RA_REF","RA")
		  else { switch(v4b.el[i].v3dt)
			  { default:		strcpy(v4b.el[i].dim,"Alpha") ; break ;
			    case VFT_BININT:
			    case VFT_BINWORD:	if (v4b.el[i].decimals == 0) {strcpy(v4b.el[i].dim,"Int") ; break ; } ;
			    case VFT_FLOAT:
			    case VFT_V3NUM:
			    case VFT_BINLONG:
			    case VFT_STRINT:	strcpy(v4b.el[i].dim,"Num") ; break ;
			  } ;
		       } ;
		 fprintf(parse->v4b_fileptr,"FLD( %s, %d,%d,%s,%d,%d,%d,%d,%d,%d, \"%s\")\n",
			v4b.el[i].name,v4b.fileref,i+1,v4b.el[i].dim,v4b.el[i].v3dt,v4b.el[i].keynum,
			(v4b.el[i].memberof==0 ? 0 : v4b.el[i].memberof-j),
			v4b.el[i].offset,v4b.el[i].bytesall,v4b.el[i].decimals,v4b.el[i].comment) ;
/*
		 fprintf(parse->v4b_fileptr,"Bind [Field=%s FileRef=%d DataEl] DataEl=(%d,%d,%s,%s,%d,%d,%d,%d,%d,%d)\n",
			v4b.el[i].name,v4b.fileref,v4b.fileref,i+1,v4b.el[i].name,v4b.el[i].dim,v4b.el[i].v3dt,v4b.el[i].keynum,
			(v4b.el[i].memberof==0 ? 0 : v4b.el[i].memberof-j),
			v4b.el[i].offset,v4b.el[i].bytesall,v4b.el[i].decimals) ;
*/
/*
		 if (v4b.el[i].keynum)
		  fprintf(parse->v4b_fileptr,"Bind [FileRef=%d Key=%d Part=%d] Field=%s\n",
			v4b.fileref,v4b.el[i].keynum,(++keyparts[v4b.el[i].keynum]),v4b.el[i].name) ;
*/
		 if (v4b.el[i].keynum)
		  fprintf(parse->v4b_fileptr,"FLDK( %d, %d, %d, %s)\n",
			v4b.fileref,v4b.el[i].keynum,(++keyparts[v4b.el[i].keynum]),v4b.el[i].name) ;
		 if (v4b.el[i].iscount) ex = i+1 ;
/*
		 if (v4b.el[i].ownerof > 0)
		  { fprintf(parse->v4b_fileptr,"Bind [FileRef=%d Structure=%d] StructEl=(%d,%d,%d,%d,%d,%s,%d)\n",
		     v4b.fileref,v4b.el[i].ownerof-j,v4b.fileref,v4b.el[i].ownerof-j,i+1,v4b.el[i].elbytes,v4b.el[i].elcount,
		     (ex > 0 ? v4b.el[ex-1].name : ""),v4b.el[i].offset) ;
		    ex = 0 ;
		  } ;
*/
		 if (v4b.el[i].ownerof > 0)
		  { fprintf(parse->v4b_fileptr,"FLDS( %d,%d,%d,%d,%d,%s,%d)\n",
		     v4b.fileref,v4b.el[i].ownerof-j,i+1,v4b.el[i].elbytes,v4b.el[i].elcount,
		     (ex > 0 ? v4b.el[ex-1].name : "\"\""),v4b.el[i].offset) ;
		    ex = 0 ;
		  } ;
		 if (v4b.eldim[0] == 0) { sprintf(lbuf,"Bind [%s %s",v4b.prelude,v4b.el[i].name) ; }
		  else { sprintf(lbuf,"Bind [%s %s=%s",v4b.prelude,v4b.eldim,v4b.el[i].name) ; } ;
		 rbuf[0] = 0 ;
/*		 Look for all dimensions associated with this element */
		 for(m=v4b.el[i].memberof;;)
		  { for (k=0;k<v4b.count;k++)
		     { if (!v4b.el[k].bind) continue ;
		       if (v4b.el[k].memberof != m) continue ;
		       sprintf(mbuf," %s={ALL}",v4b.el[k].name) ; strcat(lbuf,mbuf) ;
		       sprintf(mbuf," %s={BINDING}",v4b.el[k].name) ; strcat(rbuf,mbuf) ;
		     } ;
		    if (m == 0) break ;
		    for (k=0;k<v4b.count;k++) { if (v4b.el[k].ownerof == m) break ; } ;
		    if (k >= v4b.count) break ;
		    m = v4b.el[k].memberof ;
		  } ;
/* Don't want this
		 fprintf(parse->v4b_fileptr,"%s] [IntMod=FileRefGet FileRef=%d Element=%d%s]\n",
			lbuf,v4b.fileref,i+1,rbuf) ;
*/
	       } ;
	    } ;
	   return(TRUE) ;
	 } ;
/*	Check for end of level ("}") */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rbrace)
	 { prsu_nxt_token(TRUE) ; goto dcl_var_type ; } ;
/*	Look for optional ";"	*/
	prsu_nxt_token(FALSE) ; if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi) prsu_nxt_token(TRUE) ;
/*	Update top symbol for this level with length */
	if (skel.count == 0)
	 { sp = dsd.level[parse->dcl_nest].sym_ptr ; ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ;
/*	   If top level then remember alignment mask */
	   if (parse->dcl_nest == 1) { sp->align_mask = dsd.level[parse->dcl_nest].max_align_mask ; } ;
/*	   Maybe adjust length of structure to account for alignment */
	   dsd.level[parse->dcl_nest].length =
	    (dsd.level[parse->dcl_nest].length + dsd.level[parse->dcl_nest].max_align_mask)
		 & ~dsd.level[parse->dcl_nest].max_align_mask ;
	   elbytes = (dsd.level[parse->dcl_nest].numels > 0 ? dsd.level[parse->dcl_nest].numels : 1)*dsd.level[parse->dcl_nest].length ;
	   if (elbytes > V3_PRS_MAX_STRING_LEN)
	    prsu_error(WARNP,"STRUCTOOBIG","STRUCT just terminated exceeds max V3 string length") ;
	   ses->format.fld.length = elbytes ;
	   if (v4b.fileref > 0)
	    { strcpy(v4b.el[v4b.count].name,sp->name) ; strcpy(v4b.el[v4b.count].dim,dsd.level[parse->dcl_nest-1].dim) ;
	      v4b.el[v4b.count].offset = ( sp->structure_id == 0 ? 0 : ses->offset) ;
	      v4b.el[v4b.count].offset = (v4b.el[v4b.count].offset + dsd.level[parse->dcl_nest].max_align_mask)
						& ~dsd.level[parse->dcl_nest].max_align_mask ;
	      v4b.el[v4b.count].bytesall = ses->format.fld.length ;
	      v4b.el[v4b.count].elbytes = (sp->ss_count > 0 ? dsd.level[parse->dcl_nest].length : ses->format.fld.length) ;
	      v4b.el[v4b.count].elcount = (sp->ss_count > 0 ? ses->format.fld.length / dsd.level[parse->dcl_nest].length : 0) ;
	      v4b.el[v4b.count].memberof = sp->structure_id ; v4b.el[v4b.count].v3dt = VFT_FIXSTR ;
	      v4b.el[v4b.count].ownerof = sp->search_struct_id ; v4b.count ++ ;
	      if (v4b.count >= prsu_v4b_elMax)
	       { v4b.count-- ;
		 prsu_error(WARNP,"V4BELMAX","dcl struct xxx contains too many element for internal v4b block") ;
	       } ;
	    } ;
/*	   If this is not the top level then do some stuff */
	   if (parse->dcl_nest > 1)
	    {
/*	      Maybe adjust alignment of "master" structure header based on alignment of all elements */
	      i = (ses->offset + dsd.level[parse->dcl_nest].max_align_mask) & ~dsd.level[parse->dcl_nest].max_align_mask ;
	      i -= ses->offset ;	/* i = change to alignment (0 if already properly aligned) */
	      ses->offset += i ; dsd.level[parse->dcl_nest-1].length += i ;
/*	      Update prior level with length of this level */
	      dsd.level[parse->dcl_nest-1].length =	/* Maybe adjust starting offset of struct we are finishing */
		(dsd.level[parse->dcl_nest-1].length + dsd.level[parse->dcl_nest].max_align_mask)
		& ~dsd.level[parse->dcl_nest].max_align_mask ;
/*	      Allow alignment to flow to higher structure */
	      dsd.level[parse->dcl_nest-1].max_align_mask |= dsd.level[parse->dcl_nest].max_align_mask ;
	      dsd.level[parse->dcl_nest-1].length += elbytes ;
/*	      Go thru any subscripts for this level & insert correct length */
	      j = dsd.level[parse->dcl_nest].length ;
	      for (i=sp->ss_count-1;i>=0;i--)
	       { if (j > 0xffff) prsu_error(ERRP,"SSTOOBIG","Subscripting too large, try reversing dimensions") ;
	         ses->ssentry[i].dimlen = j ; j *= ses->ssentry[i].max ;
	       } ;
	    } ;
	 } ;
/*	Pop up to prior level & restore element structure id */
	parse->dcl_nest-- ;
	vd.strid = dsd.level[parse->dcl_nest].strid ;
	goto dcl_end_list ;
}

/*	prs_attempt_struct_load - Attempt to load structure from V4 area	*/
int prs_attempt_struct_load(structname)
  char *structname ;
{ struct V4LEX__TknCtrlBlk tcb ;
  struct V4LEX__MacroEntry *vlme ;
  struct V4LEX__MacroBody *vlmb ;
  char *btt,*b ;
  extern struct V4C__ProcessInfo *gpi  ;	/* Global process information */

	if (gpi == NULL)
	 v3v4_GetV4ctx() ;			/* Make sure V4 is initialized OK */
	vlme = v4eval_GetMacroEntry(parse->ctx,ASCretUC(structname),V4LEX_MACTYPE_MIDAS) ;
	if (vlme == NULL)
         { char emsg[512] ; sprintf(emsg,"Could not load macro(%s) from V4 database - %s",structname,UCretASC(gpi->ctx->ErrorMsgAux)) ;
           v3_error(V3E_NOMACINV4,"V4IM","LOADV3STRUCTMAC","NOMACINV4",emsg,0) ;
         } ;
	vlmb = (struct V4LEX__MacroBody *)&vlme->macData[vlme->bodyOffset] ;
	btt = v4mm_AllocChunk(0x8000,FALSE) ; UCstrcpyToASC(btt,vlmb->macBody) ;
	for(b=btt;*b!='\0';b++) { if (*b <= 26) *b = ' ' ; } ;	/* Convert any control characters to white-space */
//printf("** %s -> **%s**\n",structname,btt) ;
	parse->ilx++ ; parse->ilvl[parse->ilx].is_sos_file = FALSE ; parse->ilvl[parse->ilx].file = NULL ;
	parse->ilvl[parse->ilx].input_ptr = btt ; parse->ilvl[parse->ilx].free_ptr = btt ;
	strcpy(parse->ilvl[parse->ilx].file_name,parse->ilvl[parse->ilx-1].file_name) ; strcat(parse->ilvl[parse->ilx].file_name,structname) ; strcat(parse->ilvl[parse->ilx].file_name,")") ;
	parse->ilvl[parse->ilx].file_page = parse->ilvl[parse->ilx-1].file_page ;
	parse->ilvl[parse->ilx].statement_start_line = parse->ilvl[parse->ilx-1].statement_start_line ;
	parse->ilvl[parse->ilx].current_line = parse->ilvl[parse->ilx-1].current_line ;
	parse->ilvl[parse->ilx].last_page_printed = 0 ;
	return(TRUE) ;
}
