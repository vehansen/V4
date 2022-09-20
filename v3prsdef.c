/*	V3PRSDEF.C - Definitions for VICTIM III Parser

	Last edited 8/04/84 by Victor E. Hansen		*/


/*	Error Constants				*/
#define WARN 1
#undef ERROR
#define ERROR 2
#define ABORT 3
#define INTERNAL 4
#define POINT 1*512		/* Point to current token position */

#define ERRP ERROR+POINT
#define WARNP WARN+POINT

/*	Junk to calculate sizes of objects based on pointer differences */
#define PRSLEN(L1,L2) ((char *)L1-(char *)L2)

#define PUSHVAL(VAL) parse->code[parse->code_offset++] = VAL
#define PUSHLEVEL parse->code[(parse->code_offset+=2)-1] = parse->level->command_level_id
#define PUSHTMP(OP) parse->tmp[++(parse->tmp_index)] = (struct db__opr_info *)OP

/*	prs__symbol_ref_list - Used to build up symbolic references */
struct prs__symbol_ref_list {
  unsigned char type ;		/* First symbol type - SYMBOL_xxx */
  unsigned char got_subs ;	/* TRUE if subscripting detected */
  unsigned char ignore_subs ;	/* TRUE if got "[]" to ignore subscripting check */
  unsigned short srch_id ;	/* Structure search id for next symbol */
  short skel_index ;		/* Skeleton index to be used */
  short last_el ;		/* Last referenced skeleton element */
  unsigned char module_id ;	/* Module id for "XSTACK" reference */
  unsigned char frames ;	/* Frame count for "XSTACK" reference */
  unsigned char count ;		/* Number below (0'th not used) */
  union {
     struct db__prs_sym *sp ;	/* Pointer to symbol */
     union val__sob obname ;	/* Object name for object references */
   } sym[V3_PRS_SYM_REF_MAX] ;
 } ;

/*	P R E D E F I N E D   S Y M B O L S		*/

/*	Flags for symbols defined below			*/
#ifdef ISLITTLEENDIAN
#define XIM 1			/* Append to xct code immediate */
#define XEN 2			/* Append to xct after a few checks */
#define TO  4			/* Throw out */
#define RTL 8			/* Right-to-left */
#define EOA 16			/* Push end-of-arg */
#define PTI 32			/* Push temp immediate */
#define RVL 64			/* Returns a value */
#define CHP 128			/* Check as paren */
#define L1 256			/* Got LOOP1 */
#define ARITH 512		/* Test for (non)integer arithmetic */
#define UPD1 1024		/* Module updates 1st argument */
#define UPD2 2048		/* Module updates 2nd argument */
#define NOREF 4096		/* Module does not "reference" argument (ex: sys_address()) */
#define UPD3 8192		/* Module updates 3rd argument */
#else
#define UPD3 4
#define NOREF 8
#define UPD2 16
#define UPD1 32
#define ARITH 64
#define L1 128
#define CHP 256
#define RVL 512
#define PTI 1024
#define EOA 2048
#define RTL 4096
#define TO 8192
#define XEN 16384
#define XIM 32768
#endif

/*	Things to do before pushing an operator onto temp parse stack */
#define PUSH_NULL 0
#define PUSH_COLON 1
#define PUSH_LBRACE 2
#define PUSH_RBRACE 3
#define PUSH_PATH 4
#define PUSH_LPAREN 5
#define PUSH_2LANGLE 6
#define PUSH_2RANGLE 7
#define PUSH_IF 8
#define PUSH_THEN 9
#define PUSH_ELSE 10
#define PUSH_BRANCH 11
#define PUSH_FALL_THRU 12
#define PUSH_OUT 13
#define PUSH_TOP 14
#define PUSH_DCL 15
#define PUSH_GOTO 16
#define PUSH_SEMI 17
#define PUSH_CONTINUE 18
#define PUSH_LOOP 19
#define PUSH_LBRACKET 20
#define PUSH_RBRACKET 21
#define PUSH_DOT 22
#define PUSH_DASH 23
#define PUSH_CROSSHATCH 24
#define PUSH_SLASH 25
#define PUSH_DOT_DOT 26
#define PUSH_ELSEIF 27
#define PUSH_START_CONDCOMP 28
#define PUSH_END_CONDCOMP 29
#define PUSH_STAR 30
#define PUSH_RANGLE 31
#define PUSH_LRBRACKET 32
#define PUSH_ITX_GEN 33

/*	Things to do after popping operator from temp stack	*/
#define POP_NULL 0
#define POP_IFISH 1
#define POP_PCK_UNLOAD 2
#define POP_LOOP 3
#define POP_PARTIAL 4
#define POP_BRANCH 8
#define POP_SYMREF 9
#define POP_RETURN 10
#define POP_FORK_SWITCH 11

/*	VICTIM III Predefined symbol table		*/
struct db__predef_table {
  char name[V3_PREDEFINED_SYM_LEN_MAX] ;
  unsigned precedence : 6 ;		/* Operator precedence */
  unsigned on_push_do : 6 ;
  unsigned on_pop_do : 4 ;
  unsigned flags : 16 ;			/* Parser flags - see above */
  unsigned reference : 16 ;
  unsigned results : 8 ;
  unsigned args : 8 ;
 } ;


/*	T O K E N   G E N E R A T O R			*/

/*	Type of Token Constants			*/
#define END_OF_LINE 0			/* End of line */
#define SYMBOL 1			/* A predefined/user symbol */
#define STRING_LIT 2			/* A string literal */
#define NUMERIC_LIT 3			/* A numeric literal */
#define RADIX_OVERRIDE 4		/* Override for numeric radix */

/*	Character sets for numeric literals		*/
#define INVALID_NUMERIC 0		/* For invalid characters */
#define END_OF_NUMBER 1
#define DIGIT 2
#define HEX_DIGIT 3
#define EXPONENT 4
#define DECIMAL_POINT 5
#undef IGNORE
#define IGNORE 6

/*	Flags for token generator			*/

#define TKN_FLAGS_NOVALUE 16	/* Don't handle "#xxx", return "#" */
#define TKN_FLAGS_STREL 32	/* Have structure element, don't search reserved word symbol table */

/*	D C L   O F   S Y M B O L S				*/

/*	db__var_desc - Describes a variable		*/

struct db__var_desc {
  int	type,			/* SYMBOL_xxx (ex: SYMBOL_NORMAL) */
	table,			/* PRS_xxx_SYM (ex: PRS_LOCAL_SYM) */
	vft,			/* VFT_xxx (ex: VFT_BININT) */
	val,			/* VAL_xxx (ex: VAL_STATPURE) */
	bytes,			/* Number of bytes for value */
	subs,			/* Number of subscripts */
	align_mask,		/* Alignment mask (ex: 3 for word alignment) */
	dps ;			/* Number of decimal places */
  unsigned short strid ;	/* Structure id associated with symbol */
  unsigned short srchid ;	/* Structure id to be searched for elements of "this" symbol */
 } ;

struct db__dsd {
  unsigned char clx ;	/* Current structure index (0 denotes no STRUCT !) */
  struct {
   struct db__prs_sym *sym_ptr ; /* Points to top-of-level symbol */
   unsigned short strid ;	/* Structure id for elements at this level */
   unsigned short numels ;	/* Number of subscript dim elements (or zero if none) */
   int length ;		/* Current length of this level */
   int max_align_mask ;	/* Largest alignment mask seen within structure (floats upwards) */
   char dim[V3_PRS_SYMBOL_MAX+1] ;
  } level[V3_PRS_STRUCT_LEVEL_MAX] ;
  unsigned short ssnum[V3_PRS_SUBSCRIPT_DIM_MAX] ; /* Subscript dim number for each level */
  unsigned char sscnte ;	/* Number of subscript dims for current element */
  char no_alignment ;	/* TRUE if no alignment necessary within structure (primarily for porting purposes) */
 } ;

/*	V 4   I N T E R F A C E			*/

#define prsu_v4b_elMax 300

struct prsu__v4b {
  int fileref ;
  char prelude[200] ;
  char eldim[V3_PRS_SYMBOL_MAX+1] ;
  short count ;
  struct {
    char name[V3_PRS_SYMBOL_MAX+1], dim[V3_PRS_SYMBOL_MAX+1], comment[100] ;
    int v3dt,offset,memberof,ownerof,bytesall,elcount,elbytes,decimals,bind,keynum,iscount ;
   } el[prsu_v4b_elMax] ;
 } ;
