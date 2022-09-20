/*	V3PRSD.C - PARSING UTILITIES FOR VICTIM III

	LAST EDITED 6/29/84 BY VICTOR E. HANSEN	*/

#include <time.h>
#include "v3defs.c"

/*	Global Definitions for Parser		*/
extern struct db__parse_info *parse ;
extern struct db__tknres tkn ;
extern jmp_buf parse_exit ;

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

/*	prsu_dcl_subs - Checks DCL element for subscripts	*/
/*	Returns 0 or total number of dimension elements		*/

int prsu_dcl_subs(dsd)
  struct db__dsd *dsd ;
{ int dimels = 0 ;
/*	Get next token to check for "["	*/
	dsd->sscnte = 0 ;
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_PREDEFINED || tkn.sym_ptr != (struct db__opr_info *)&notbl_lbracket)
	 { prsu_nxt_token(TRUE) ;	/* No subscripting here ! */
	   return(dimels) ;
	 } ;
/*	Got something - may be multiple dimensions	*/
	dimels = 1 ;
	for (;;)
	 { if (! prsu_eval_int_exp(FALSE,&tkn.value.int_lit))
	    prsu_error(ERRP,"BADDIMVAL","Subscript dimension must be integer expression or value") ;
	    if (tkn.value.int_lit <= 0)
	     prsu_error(ERRP,"INVDIMVAL","Subscript dimension must be greater than zero") ;
	    if (dsd->sscnte >= V3_PRS_SUBSCRIPT_DIM_MAX)
	     prsu_error(ERRP,"TOOMANYDIMS","Exceeded maximum number of subscript dimensions") ;
	    dsd->ssnum[dsd->sscnte++] = tkn.value.int_lit ; dimels *= tkn.value.int_lit ;
	    prsu_nxt_token(FALSE) ;
	    if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma) continue ;
	    if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rbracket)
	     prsu_error(ERRP,"INVDIMTERM","Expecting a comma or right bracket") ;
/*	    Got end of dimension - just check for another "[" */
	    prsu_nxt_token(FALSE) ;
	    if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lbracket) continue ;
/*	    Nope - end of dimension */
	    prsu_nxt_token(TRUE) ; return(dimels) ;
	 } ;
}

/*	prsu_dcl_constant - Parses #constant of form:
		dcl value symbol "value" ;
		dcl macro symbol "macro expansion" ;
		dcl value_prime symbol integer ;		*/

void prsu_dcl_constant(is_macro,is_prime)
  int is_macro ;			/* TRUE if via DCL MACRO */
  int is_prime ;			/* TRUE if via DCL VALUE_PRIME */
{ int i,prime ;

/*	Get the symbol name	*/
	prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	if (tkn.type != TKN_SYMBOL)
	 prsu_error(ERRP,"INVVALUE","Expecting a symbolic constant here") ;
	if (parse->constants.count >= V3_PRS_CONSTANT_MAX-1)
	 prsu_error(ERRP,"TOOMANYVALS","Too many parser values have been defined") ;
/*	Copy the symbol name & hash */
	strcpy(parse->constants.sym[parse->constants.count].name,tkn.symbol) ;
	parse->constants.sym[parse->constants.count].hash = prsu_str_hash(tkn.symbol,-1) ;
	parse->constants.sym[parse->constants.count].is_macro = is_macro ;
/*	If parsing a "dcl value_prime" then look for integer [expression] */
	if (is_prime)
	 { if (! prsu_eval_int_exp(FALSE,&prime))
	    prsu_error(ERRP,"VALUE_PRIME","Expecting an integer value or expression here") ;
	   prime = v_CvtToPrime(prime) ;
	   sprintf(tkn.value.char_lit,"%d",prime) ; tkn.literal_len = strlen(tkn.value.char_lit) ;
	 } else
	 { prsu_nxt_token(FALSE) ;
/*	   Now get its value (best be a string literal) */
	   if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	    prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
	 } ;
	if (tkn.literal_len + 1 + parse->constants.free_offset >= V3_PRS_CONSTANT_BUF_SIZE-5)
	 prsu_error(ERRP,"VALUEOVERFLOW","Buffer overflow for all DCL VALUE xxx values") ;
/*	Looks like the value will fit - save it */
	parse->constants.sym[parse->constants.count].offset = parse->constants.free_offset ;
	strncpy(&parse->constants.buf[parse->constants.free_offset],tkn.value.char_lit,tkn.literal_len) ;
	parse->constants.free_offset += (1 + tkn.literal_len) ;
/*	Now make sure we got a ";" */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"INVVALUETERM","Expecting a DCL VALUE xxx statement to end with a semicolon") ;
	parse->constants.count++ ;
	return ;
}

/*	P A R S E   A N D   E V A L U A T E   A N   I N T E G E R   E X P R E S S I O N	*/

/*	The routines on this page make use of the following

	Operator	Opcode		Precedence Value
	   -(u)		  80			89
	   (		  30			--
	   *		  60			79
	   /		  70			79
	  //		  71			79
	   +		  40			59
	   -		  50			59
	  boe		  10			--
	  eoe		  20			39
*/

static char osx ;		/* Index into operator stack */
static char vsx ;		/* Index into value stack */
static char opstack[20] ;
static int valstack[20] ;

/*	prsu_eval_int_exp - Parses and evaluates an integer V3 expression  */
/*	(Returns TRUE and updates via result_ptr, or FALSE) */

int prsu_eval_int_exp(in_path,result_ptr)
 int in_path ;				/* TRUE if parsing a "path(xxx)" */
 int *result_ptr ;			/* Updated with the result (if any) */
{ char have_val = FALSE ;		/* TRUE when just seen value */
   char cnt ;

	osx = 0 ; vsx = 0 ; opstack[osx++] = 10 ;
/*	If we have already seen the left paren (from "path(...") then push it */
	if (in_path) opstack[osx++] = 30 ;
/*	Now loop until we have something that ain't what we want */
	for(cnt=0;;cnt++)
	 { prsu_nxt_token(FALSE) ;
/*	   If an integer literal then push on value stack */
	   if (tkn.type == TKN_LITERAL)
	    { if (tkn.lit_format.fld.type == VFT_BININT && tkn.lit_format.fld.decimals == 0)
	       { valstack[vsx++] = tkn.value.int_lit ; have_val = TRUE ; continue ; } ;
/*	      Not an integer - either quit or generate an error */
	      if (cnt == 0 || tkn.lit_format.fld.type == VFT_FIXSTR) goto end_of_intexp ;
	      prsu_error(ERRP,"INVINTLIT","Expecting expression to consist of integer literals") ;
	    } ;
/*	   See if a symbol */
	   if (tkn.type == TKN_SYMBOL) goto end_of_intexp ;
/*	   Nope, then see if one of the operators we expect */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen) { opstack[osx++] = 30 ; goto end_have_operator ; } ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rparen)
	    { prsu_eval_intexp_ops(39) ;
	      if (opstack[--osx] != 30)
	       prsu_error(ERRP,"INVINTEXP","Unbalanced parentheses (\'(...)\') in this expression") ;
/*	      If in a path and this rparen finishes the expression, then quit */
	      if (in_path && opstack[osx-1] == 10) goto end_of_intexp ;
	      have_val = TRUE ; continue ;
	    } ;
	   if (in_path)
	    { if (tkn.sym_ptr == (struct db__opr_info *)&notbl_dot_dot || tkn.sym_ptr == (struct db__opr_info *)&notbl_comma)
	       { prsu_eval_intexp_ops(39) ;
		 if (opstack[--osx] != 30)
		  prsu_error(ERRP,"INVINTEXP","Unbalanced parentheses in this \'path\' expression") ;
		 goto end_of_intexp ;
	       } ;
	    } ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_star || tkn.sym_ptr == (struct db__opr_info *)&notbl_slash || tkn.sym_ptr == (struct db__opr_info *)&notbl_slash_slash)
	    { prsu_eval_intexp_ops(79) ;
	      opstack[osx++] = (tkn.sym_ptr == (struct db__opr_info *)&notbl_star ? 60 : (tkn.sym_ptr == (struct db__opr_info *)&notbl_slash ? 70 : 71)) ;
	      goto end_have_operator ;
	    } ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_plus)
	    { prsu_eval_intexp_ops(59) ; opstack[osx++] = 40 ;
	      goto end_have_operator ;
	    } ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_minus)
	    { prsu_eval_intexp_ops(have_val ? 59 : 89) ;
	      opstack[osx++] = (have_val ? 50 : 80) ;
	      goto end_have_operator ;
	    } ;
/*	   Not an operator we recognize, terminate */
	   goto end_of_intexp ;
/*	   Here when done with operator */
end_have_operator:
	   have_val = FALSE ;
	 } ;
/*	Here on end of expression, make sure all is cool */
end_of_intexp:
	prsu_nxt_token(TRUE) ;		/* Push last token back on the stack */
/*	Did we do anthing ? */
	if (cnt == 0) return(FALSE) ;
/*	Yes, then clear out remaining operators */
	prsu_eval_intexp_ops(39) ;
	if (opstack[--osx] != 10)
	 prsu_error(ERRP,"INVINTEXPSYNTAX","Unbalanced parentheses ('(...)') in integer expression") ;
	if (vsx != 1) prsu_error(ERRP,"ININTEXPSYNTAX","Invalid syntax in integer expression") ;
/*	Update the result & return true */
	*result_ptr = valstack[--vsx] ;
	return(TRUE) ;
}

/*	prsu_eval_intexp_ops - Evaluates one or more opcodes on the stack */

void prsu_eval_intexp_ops(precedence)
  int precedence ;
{ int i ;

/*	Loop thru all ops on opstack until one with lower precedence */
	for(;;)
	 { if (precedence > opstack[osx-1]) break ;
/*	   Have operator to evaluate, make sure enough values */
	   if (vsx < (opstack[osx-1] == 80 ? 1 : 2))
	    { prsu_nxt_token(TRUE) ;
	      prsu_error(ERRP,"INVINTEXPVALS","Missing values in integer expression") ;
	    } ;
	   switch (opstack[--osx])
	    { default: prsu_error(ERRP,"INVINTEXPOP","Invalid operator in integer expression") ;
	      case 40:	/* Plus */	valstack[vsx-2] += valstack[vsx-1] ; --vsx ; break ;
	      case 50:	/* Minus */	valstack[vsx-2] -= valstack[vsx-1] ; --vsx ; break ;
	      case 60:	/* Times */	valstack[vsx-2] *= valstack[vsx-1] ; --vsx ; break ;
	      case 70:	/* Divides */	valstack[vsx-2] /= valstack[vsx-1] ; --vsx ; break ;
	      case 71:	/* Divide+Rnd */ i = valstack[vsx-2] % valstack[vsx-1] ;
					valstack[vsx-2] /= valstack[vsx-1] ; --vsx ;
					if (i > 0) ++valstack[vsx-1] ; break ;
	      case 80:	/* Unary Minus */ valstack[vsx-1] = -valstack[vsx-1] ; break ;
	    } ;
	 } ;
}

/*	D C L   O B J E C T S				*/


/*	prsu_dcl_obj - Handles DCL of objects		*/

void prsu_dcl_obj()
{ struct db__dcl_object *obp,*parent_obp ; /* Points to temp object descriptor */
   struct ob__desc *newob ;
   char checksum_entry_name[V3_PRS_SYMBOL_MAX+1] ; /* If not null then name of top level object for checksum table */
   int i ;

/*	Allocate top level object descriptor */
	obp = (struct db__dcl_object *)v4mm_AllocChunk(sizeof *obp,TRUE) ; parse->dcl_nest ++ ;
	checksum_entry_name[0] = NULLV ;
/*	Now get object name, options assertions */
dcl_another_object:
	prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	if (tkn.type != TKN_SYMBOL)
	 prsu_error(ERRP,"INVOBJNAME","Expecting an OBJECT name here") ;
	strcpy(obp->obname,tkn.symbol) ;
/*	Got assertions ? */
	if (obp->nesting == 0)
/*	   Copy in default "dcl assertions" if top level */
	 { for (i=0;i<parse->dcl_assertions_cnt;i++)
	    { obp->assertion_hash[obp->num_assertions] = prsu_str_hash(parse->dcl_assertions[i],-1) ;
	      strcpy(obp->assertions[obp->num_assertions++],parse->dcl_assertions[i]) ;
	    } ;
	 } ;
/*	Have top level object for checksum table ? */
	if (obp->nesting == 0 && parse->level->interpretive)
	 { strcpy(checksum_entry_name,obp->obname) ;
	   parse->current_checksum = 0 ; parse->token_count = 0 ;
	 } ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_langle) { prsu_nxt_token(TRUE) ; }
	 else
	 { for (;;)
	    { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
/*	      Got an assertion list - parse it */
	      if (tkn.type != TKN_SYMBOL)
	       prsu_error(ERRP,"INVASSERTNAME","Expecting an assertion name here") ;
	      if (obp->num_assertions >= V3_ASSERTIONS_PER_TABLE_MAX)
	       prsu_error(ERRP,"TOOMANYASSERTS","Too many assertions") ;
	      strcpy(obp->assertions[obp->num_assertions],tkn.symbol) ;
	      obp->assertion_hash[obp->num_assertions++] = prsu_str_hash(tkn.symbol,-1) ;
/*	      Look for a "," or ">" */
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma) continue ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rangle) break ;
	      prsu_error(ERRP,"INVASSERTTERM","Expecting a comma or closing angle here") ;
	    } ;
/*	   Assertions only allowed on top level */
	   if (obp->nesting > 0)
	    prsu_error(ERRP,"INVASSERTPOS","Assertion list only allowed for top-level object") ;
	 } ;
/*	Now look for an "isa" list */
dcl_another_from_isa:
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lparen) { prsu_nxt_token(TRUE) ; }
	 else
	 { for (;;)
	    { if (obp->num_isas >= V3_SOB_ISAS_PER_OBJECT_MAX)
	       prsu_error(ERRP,"TOOMANYISAS","Too many ISAs for this object") ;
/*	      Get the object (also update name if coming from "has (xxx)" construct */
	      prsu_dcl_getob(&obp->isarefs[obp->num_isas++],(obp->obname[0] == NULLV ? obp->obname : NULLV)) ;
/*	      See if another object reference */
	      prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma) continue ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rparen) break ;
	      prsu_error(ERRP,"INVISATERM","Expecting comma, closing paren, or maybe even an asterisk here") ;
	    } ;
	 } ;
/*	Now see if a value */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr == (struct db__opr_info *)&notbl_equal || tkn.sym_ptr == (struct db__opr_info *)&notbl_colon_equal)
	 { prsu_dcl_obj_value(obp) ; } else { prsu_nxt_token(TRUE) ; } ;
/*	Finally see if a nested object */
end_object_level:
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr == (struct db__opr_info *)&notbl_semi)
	 { newob = (struct ob__desc *)sobu_object_create(obp,parse->package_id) ;
/*	   Have new object - see if we have to link to a parent */
	   if ((parent_obp = obp->parent) == NULLV)
/*	      This is the top level object - add name to package name-ob tables */
	    { sobu_insert_name_ob(obp,parse->package_id) ; } ;
	   v4mm_FreeChunk(obp) ; parse->dcl_nest-- ;
	   if ((obp = parent_obp) == NULLV)
	    { if (checksum_entry_name[0] != NULLV && parse->enable_checksums)
	       prsu_checksum_table_insert('O',checksum_entry_name) ;
	      parse->dcl_nest = 0 ; return ;
	    } ;
/*	   Yes ! - update has-list & continue on with parent */
	   obp->hass[obp->num_hass].has_name.all = newob->name_id.all ;
	   obp->hass[obp->num_hass++].has_ob.all = newob->ob_id.all ;
/*	   Do we have another nested object or are we popping up ? */
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rbrace) goto end_object_level ;
	   prsu_nxt_token(TRUE) ; goto continue_object_level ;
	 } ;
/*	Best have nested object */
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lbrace)
	 prsu_error(ERRP,"INVOBJTERM","Expecting a \';\', \'=\', \':=\', or \'{\' here") ;
continue_object_level:
/*	Create a new object level for this level */
	if (obp->nesting >= V3_SOB_OBJECT_NESTING_MAX)
	 prsu_error(ERRP,"OBJNESTING","Object nesting is too deep") ;
	if (obp->num_hass >= V3_SOB_HASS_PER_OBJECT_MAX)
	 prsu_error(ERRP,"TOOMANYHASS","Too many \'has objects\' declared") ;
	parent_obp = obp ; obp = (struct db__dcl_object *)v4mm_AllocChunk(sizeof *obp,TRUE) ; obp->parent = parent_obp ;
	obp->nesting = parent_obp->nesting+1 ; parse->dcl_nest++ ;
/*	Make sure next token is "HAS" */
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_SYMBOL || strcmp(tkn.symbol,"HAS") != 0)
	 prsu_error(ERRP,"NOTHAS","Expecting the keyword \'has\' here") ;
/*	If next token is "(" then user wants to abbreviate - let him/her */
	prsu_nxt_token(FALSE) ; prsu_nxt_token(TRUE) ;
	if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen) goto dcl_another_from_isa ;
	goto dcl_another_object ;
}

/*	prsu_dcl_obj_value - Parses an object value		*/

void prsu_dcl_obj_value(obp)
 struct db__dcl_object *obp ;
{ union val__sob tob,tobn ;		/* Temp object & object name */
   struct db__formatmempos *sep ;
   struct db__prs_sym *sp ;		/* Symbol table entry */
   struct db__dcl_object *pobp ;
   struct db__dcl_objref tobr ;
   struct db__module_objentry moe ;
   struct db__opr_info *op ;		/* In case value is V3 primitive */
   struct db__formatmemposdims symref ;
   struct db__sym_dims *ses ;		/* Pointer into symbol table info on symbol */
   int i,len,vxmax,vx,offset ;
   char tname[V3_PRS_SYMBOL_MAX+1] ;
   char *rescan_ptr ;		/* Used to back-up token pointer for object values */

/*	Set up to loop just once unless we get a "{" */
	vxmax = 1 ;
	for (vx=0;vx<vxmax;vx++)
	 { rescan_ptr = (char *)prsu_nxt_token(FALSE) ;
/*	   Just ignore commas for now */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma) continue ;
/*	   Make sure we don't overflow our val slots */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rbrace)
	    { if (vx == 0) prsu_error(ERRP,"INVRBRACEPOS","Closing brace can only end a value list") ;
	      break ;
	    } ;
	   if (obp->num_vals >= V3_SOB_VALS_PER_OBJECT_MAX)
	    prsu_error(ERRP,"TOOMANYOBJVALS","Exceeded max number of object values") ;
/*	   Decide what kind of value we got - is it an integer expression ? */
	   if ( (tkn.type == TKN_LITERAL && tkn.lit_format.fld.type == VFT_BININT && tkn.lit_format.fld.decimals == 0)
		|| (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen || tkn.sym_ptr == (struct db__opr_info *)&notbl_minus))
	    { prsu_nxt_token(TRUE) ;
	      prsu_eval_int_exp(FALSE,&tkn.value.int_lit) ;
	      tkn.type = TKN_LITERAL ; tkn.literal_len = (tkn.lit_format.fld.length = 4) ;
	      tkn.lit_format.fld.type = VFT_BININT ; tkn.lit_format.fld.decimals = 0 ;
	      tkn.lit_format.fld.mode = VFM_IMMEDIATE ;
	    } ;
/*	   Is value a literal ? */
	   if (tkn.type == TKN_LITERAL)
	    { sep = (struct db__formatmempos *)&parse->ob_bucket.buf[prsu_stash_lit(TRUE)] ;
/*	      Save info as to its where-abouts */
	      obp->vals[obp->num_vals].format.all = sep->format.all ;
	      obp->vals[obp->num_vals++].mempos.all = sep->mempos.all ;
	      continue ;
	    } ;
/*	   Look for the beginning of a value list */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lbrace)
	    { if (vx > 0) prsu_error(ERRP,"CANTNESTVALLISTS","Cannot nest object value lists") ;
	      obp->val_is_list = TRUE ;
	      vxmax = 999 ; continue ;
	    } ;
/*	   How about a has-object for this object */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_dot)
	    { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	      if (tkn.type != TKN_SYMBOL)
	       prsu_error(ERRP,"INVOBJNAME","Expecting an object name here") ;
/*	      Convert symbol to object name & lookup in this table */
	      tobn.all = sobu_name_lookup(tkn.symbol,parse->package_id) ;
/*	      Get parent object descriptor */
	      if ((pobp = obp->parent) == NULLV)
	       prsu_error(ERRP,"INVDOTOBJPOS","Cannot use \'.xxx\' with top-level object") ;
try_prior_parent:
	      for (i=0;i<pobp->num_hass;i++)
	       { if (tobn.all == pobp->hass[i].has_name.all) goto found_local_has ;
	       } ;
	      if (pobp->parent != NULLV) { pobp = pobp->parent ; goto try_prior_parent ; } ;
	      prsu_error(ERRP,"OBJNOTHAD","This name not \'had\' by parent object(s)") ;
found_local_has:
	      obp->vals[obp->num_vals].mempos.all = NULLV ;
	      obp->vals[obp->num_vals++].format.all = pobp->hass[i].has_ob.all ;
	      continue ;
	    } ;
/*	   Look for a specific object - *name* */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_star)
	    { prsu_nxt_token(TRUE) ; prsu_dcl_getob(&tobr,NULL) ;
	      obp->vals[obp->num_vals].format.all = tobr.name_ids[0].all ;
	      obp->vals[obp->num_vals++].mempos.all = 0 ;
	      continue ;
	    } ;
/*	   Look for a "/xxx/" construct */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_slash)
	    { sep = (struct db__formatmempos *)&parse->ob_bucket.buf[prsu_eval_constant(TRUE,TRUE)] ;
	      obp->vals[obp->num_vals].format.all = sep->format.all ;
	      obp->vals[obp->num_vals++].mempos.all = sep->mempos.all ;
	      continue ;
	    } ;
/*	   If here and not a symbol then we got an error */
	   if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVOBJVALUE","Invalid object value") ;
/*	   See if this is a STRUCT xxx reference */
	   if (strcmp("STRUCT",tkn.symbol) == 0)
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_SYMBOL)
	       prsu_error(ERRP,"INVSTRUCTNAME","Expecting a STRUCT name here") ;
	       strcpy(tname,tkn.symbol) ;
	       for(i=2;i;i--)
		{ if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tname,0)) != NULLV) break ;
		  if (i != 2) prsu_error(ERRP,"DCLSTRUCTNOTDEF","Symbol not defined as STRUCT !") ;
		  if (!prs_attempt_struct_load(tname)) continue ;
/*		  Next token should be start of structure dcl, pop off "dcl" */
		  prsu_nxt_token(FALSE) ;
		  if (strcmp(tkn.symbol,"DCL") != 0)
		   prsu_error(ERRP,"NOTDCL","Expecting macro structure expansion to begin with DCL") ;
		  prsu_dcl(TRUE) ;			/* Parse structure & try again */
		} ;
	      if (sp->type != SYMBOL_STRUCTURE)
	       prsu_error(ERRP,"SYMNOTSTRUCT","This symbol is not the name of a STRUCT") ;
/*	      Loop thru all/any elements to get correct offset */
	      offset = 0 ; symref.dims.count = 0 ; ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ;
	      for (;;)
	       { prsu_nxt_token(FALSE) ;
		 if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot) { prsu_nxt_token(TRUE) ; break ; } ;
/*		 Parse element & do lookup */
		 prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
		 if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVSTRUCTEL","Expecting a structure element here") ;
		 if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,sp->search_struct_id)) == NULLV)
		  prsu_error(ERRP,"SYMNOTEL","Symbol not defined as STRUCT element") ;
		 ses = (struct db__sym_dims *)xctu_pointer(sp->mempos.all) ; offset += ses->offset ;
/*		 Check for subscript info - if we have it then best save it */
		 for (i=0;i<sp->ss_count;i++)
		  { symref.dims.dimlen[symref.dims.count++] = ses->ssentry[i].dimlen ; } ;
	       } ;
/*	      If last symbol was name of structure (indicating more to go) then set offset to 0
		so that "dcl partial" will work with skeletons */
/*	      if (sp->search_struct_id != 0) offset = 0 ; */
/*	      Have offset = element offset & sp = pointer to last symbol */
/*	      If we detected any subscripting info then have to save entire mess */
	      if (symref.dims.count > 0)
	       { symref.format.all = ses->format.all ; symref.mempos.all = offset ;
flh1:
		 i = prsu_stash_ob(&symref,sizeof *sep + 2 + (2*symref.dims.count)) ;
		 obp->vals[obp->num_vals].format.all = ses->format.all ;
		 obp->vals[obp->num_vals].format.fld.mode = VFM_INDIRECT ;
		 obp->vals[obp->num_vals++].mempos.all = prsu_valref(VAL_STATOB,i) ;
	       } else
	       { obp->vals[obp->num_vals].format.all = ses->format.all ;
		 obp->vals[obp->num_vals++].mempos.all = offset ;
	       } ;
	      continue ;
	    } ;
/*	   If here then have to decide if module or object name */
	   strcpy(tname,tkn.symbol) ;
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen)
	    { if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tname,0)) == NULLV)
	       { if ((op = (struct db__opr_info *)prsu_predefined_lookup(tname,V3_FUNCTIONS,PRS_FUNCTION_COUNT)) == NULLV)
		  { sp = (struct db__prs_sym *)prsu_sym_add(tname,PRS_EXTERNAL_SYM) ;	/* Add symbol as external module */
		    moe.module_hash = prsu_str_hash(tname,-1) ; strcpy(moe.name,tname) ;
		    sp->type = SYMBOL_MODULE ; i = prsu_stash_ob(&moe,sizeof moe,ALIGN_WORD) ;
		    obp->vals[obp->num_vals].format.all = V3_FORMAT_MODULE ;
		    obp->vals[obp->num_vals++].mempos.all = prsu_valref(VAL_STATOB,i) ;/* Link object value to module-object entry */
		  } else
/*		    Have a V3 primitive - make a fake modref */
		  { obp->vals[obp->num_vals].format.fld.type = VFT_MODREF ;
		    obp->vals[obp->num_vals].format.fld.mode = VFM_IMMEDIATE ;
		    obp->vals[obp->num_vals].format.fld.decimals = op->end_of_arg_list ;
		    obp->vals[obp->num_vals++].mempos.all = op->reference ;
		 } ;
	       } else
/*		 Have a user module reference - Save ptr to 2-word descriptor */
	       { if (sp->type != SYMBOL_MODULE)
		  prsu_error(ERRP,"SYMNOTMODULE","This symbol is not defined as a module") ;
		 moe.module_hash = prsu_str_hash(tname,-1) ; strcpy(moe.name,tname) ;
		 i = prsu_stash_ob(&moe,sizeof moe,ALIGN_WORD) ;
		 obp->vals[obp->num_vals].format.fld.type = VFT_MODREF ;
		 obp->vals[obp->num_vals].format.fld.mode = VFM_PTR ;
		 obp->vals[obp->num_vals++].mempos.all = prsu_valref(VAL_STATOB,i) ;
	       } ;
	      obp->val_is_deferred = TRUE ; /* Set (DEFERRED) as default */
/*	      See if we got the "PARENT" keyword */
	      prsu_nxt_token(FALSE) ;
	      if (tkn.type == TKN_SYMBOL && strcmp("PARENT",tkn.symbol) == 0)
	       { prsu_nxt_token(FALSE) ; obp->val_needs_parent = TRUE ; obp->val_is_deferred = FALSE ; } ;
	      if (tkn.type == TKN_SYMBOL && strcmp("DEFERRED",tkn.symbol) == 0)
	       { prsu_nxt_token(FALSE) ; obp->val_is_deferred = TRUE ; } ;
	      if (tkn.type == TKN_SYMBOL && strcmp("EVAL",tkn.symbol) == 0)
	       { prsu_nxt_token(FALSE) ; obp->val_is_deferred = FALSE ; } ;
/*	      Best have a closing paren */
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen)
	       prsu_error(ERRP,"INVMODVALTERM","Expecting EVAL, DEFERRED, PARENT or \')\' here") ;
	      continue ;
	    } ;
/*	   At this point can only be an object reference */
	   parse->ilvl[parse->ilx].input_ptr = rescan_ptr ;	/* Back up pointer to begin of object */
	   prsu_dcl_getob(&tobr,NULL) ;
	   i = prsu_alloc_ob(len = (PRSLEN(&tobr.name_ids[tobr.count+1],&tobr)),ALIGN_WORD) ;
	   memcpy(&parse->ob_bucket.buf[i],&tobr,len) ;
/*	   "Value" is object name & pointer to name reference */
	   obp->vals[obp->num_vals].format.all = V3_FORMAT_OBJNAME ;
	   obp->vals[obp->num_vals++].mempos.all = prsu_valref(VAL_STATOB,i) ;
	 } ;
/*	All done with values - return  */
	return ;
}

/*	prsu_dcl_getob - Parses an object reference & evaluates if a "star" reference */

void prsu_dcl_getob(tobr,name_ptr)
  struct db__dcl_objref *tobr ;		/* Pointer to object reference list */
  char *name_ptr ;			/* Pointer to string for last name or NULLV */
{ union val__sob ob ;			/* Points to actual object if starred */
   char have_star = FALSE,i ;

/*	See if it is a "star" ref */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_star) { prsu_nxt_token(TRUE) ; }
	 else have_star = TRUE ;
/*	Now go thru name.name.name... */
	tobr->count = 0 ;
	for (;;)
	 { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ; if (name_ptr != NULLV) strcpy(name_ptr,tkn.symbol) ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"INVOBJNAME","Expecting an object name here") ;
	   if (tobr->count >= V3_SOB_TEMP_NAME_TABLE_MAX)
	    prsu_error(ERRP,"TOOMANYOBJNEST","Too many nested references here") ;
/*	   Create a name object from symbol */
	   tobr->name_ids[tobr->count++].all = sobu_name_create(tkn.symbol,parse->package_id) ;
/*	   If a "star" reference then keep track */
	   if (have_star)
	    { if (tobr->count == 1)
	       { if ((ob.all = sobu_object_find(tobr->name_ids[0].all)) == NULLV)
		  prsu_error(ERRP,"OBJNOTFOUND","This object cannot be currently located") ;
	       } else
	       { if ((ob.all = sobu_has_find(ob.all,tobr->name_ids[tobr->count-1].all)) == NULLV)
		  prsu_error(ERRP,"OBJNOTHAD","This object is not \'had\' by parent object") ;
	       } ;
	    } ;
/*	   See if object reference is continued with "." */
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_dot) continue ;
	   prsu_nxt_token(TRUE) ; break ;
	 } ;
/*	End of object - if "starred" then make sure ends with one */
	if (have_star)
	 { prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_star)
	    prsu_error(ERRP,"NOTASTERISK","Expecting a terminating asterisk here") ;
	   tobr->count = 0 ; tobr->name_ids[0].all = ob.all ;
	 } ;
	return ;
}

/*	P A R S E   "D C L   xxx   U T I L I T I E S		*/

#define IFSYM(str) if (strcmp(tkn.symbol,str) == 0)

int prsu_dcl_radix()
{	prsu_nxt_token(FALSE) ;
	IFSYM("DECIMAL") { parse->default_radix = 10 ; }
	 else { IFSYM("HEX") { parse->default_radix = 16 ; }
		 else { IFSYM("HEXADECIMAL") { parse->default_radix = 16 ; }
			 else { IFSYM("OCTAL") { parse->default_radix = 8 ; }
				 else prsu_error(ERRP,"INVRADIX","Expecting DECIMAL, HEX, or OCTAL") ;
	      } ;     } ;    } ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_checksums()
{
  int i ;

	prsu_nxt_token(FALSE) ;
	IFSYM("INCLUDE") { i = TRUE ; }
	 else { IFSYM("EXCLUDE") { i = FALSE ; }
		 else prsu_error(ERRP,"INVCHKKEYWORD","Expecting INCLUDE or EXCLUDE here") ;
	      } ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	parse->enable_checksums = i ;
	return(TRUE) ;
}

int prsu_dcl_flag()
{
	if (parse->user_flag_count >= V3_PRS_USER_FLAG_MAX)
	 prsu_error(ERRP,"TOOMNYFLAGS","Exceeded max number of user defined flags") ;
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_SYMBOL)
	 prsu_error(ERRP,"NOTSYM","Expecting a symbol/flag name here") ;
	strcpy(parse->user_flags[parse->user_flag_count].name,tkn.symbol) ;
/*	Look for optional "=" */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_equal) prsu_nxt_token(TRUE) ;
	if (! prsu_eval_int_exp(FALSE,&tkn.value.int_lit))
	 prsu_error(ERRP,"BADFLAGVAL","FLAG value must be integer constant or expression") ;
	parse->user_flags[parse->user_flag_count++].value = tkn.value.int_lit ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_include()
{ FILE *fp ;
  char namebuf[150] ;

	prsu_nxt_token(FALSE) ;
	if (tkn.type == TKN_SYMBOL)
	 { strcpy(namebuf,"v3_include:") ; strcat(namebuf,tkn.symbol) ; strcat(namebuf,".v3i") ;
	   strcpy(tkn.value.char_lit,namebuf) ;
	 } else
	 { if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	    prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
	   tkn.value.char_lit[tkn.literal_len] = NULLV ;
	 } ;
/*	Try to open the file for loading */
	if ((fp = fopen((char *)v3_logical_decoder(tkn.value.char_lit,TRUE),"r")) == NULLV)
	 { if (strchr(tkn.value.char_lit,'.') != NULLV)
	    prsu_error(ABORT,"NOFILETOINC","Could not access file to INCLUDE") ;
	   strcat(tkn.value.char_lit,".v3i") ;	/* Append V3I extension */
	   if ((fp = fopen((char *)v3_logical_decoder(tkn.value.char_lit,TRUE),"r")) == NULLV)
	    { tkn.value.char_lit[strlen(tkn.value.char_lit)-1] = NULLV ; 	/* Make it V3 extension */
	      if ((fp = fopen((char *)v3_logical_decoder(tkn.value.char_lit,TRUE),"r")) == NULLV)
	       prsu_error(ABORT,"NOFILETOINC","Could not access file to INCLUDE") ;
	    } ;
	 } ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
/*	Have file pointer in "fp", go push onto parse input stack */
	strcpy(namebuf,(char *)v3_logical_decoder(tkn.value.char_lit,TRUE)) ;
	prsu_nest_input(parse,fp,namebuf) ;
	return(TRUE) ;
}

int prsu_dcl_v4eval()
{ struct V4LEX__TknCtrlBlk tcb ;

	if (parse->ctx == NULLV)
	 { parse->ctx = (struct V4C__Context *)v4mm_AllocChunk(sizeof *parse->ctx,TRUE) ;
	   v4ctx_Initialize(parse->ctx,NULL,NULL) ;			/* Initialize context */
	   process->ctx = parse->ctx ;				/* Save just in case! */
	 } ;
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	 prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
	tkn.value.char_lit[tkn.literal_len] = NULLV ; strcat(tkn.value.char_lit,"\n") ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(tkn.value.char_lit),V4LEX_InpMode_String) ;
	v4eval_Eval(&tcb,parse->ctx,FALSE,FALSE,FALSE,FALSE,FALSE,NULL) ;	/* Evaluate the argument */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_v4modulebind()
{
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	 prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
	tkn.value.char_lit[tkn.literal_len] = NULLV ;
	strcpy(parse->dflt_module_bind,tkn.value.char_lit) ;		/* Just copy */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_v4moduleeval()
{
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	 prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
	tkn.value.char_lit[tkn.literal_len] = NULLV ;
	strcpy(parse->dflt_module_v4eval,tkn.value.char_lit) ;		/* Just copy */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_v4structeval()
{
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	 prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
	tkn.value.char_lit[tkn.literal_len] = NULLV ;
	strcpy(parse->eval_on_undefstruct,tkn.value.char_lit) ;		/* Just copy */
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_v4b()
{ FILE *fp ;
  char namebuf[150] ;

	prsu_nxt_token(FALSE) ;
	if (tkn.type == TKN_SYMBOL)
	 { strcpy(namebuf,tkn.symbol) ; strcat(namebuf,".v4b") ;
	   strcpy(tkn.value.char_lit,namebuf) ;
	 } else
	 { if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
	    prsu_error(ERRP,"NOTSTRLIT","Expecting a string literal here") ;
	   tkn.value.char_lit[tkn.literal_len] = NULLV ;
	 } ;
/*	Try to open the file for loading */
	if (strchr(tkn.value.char_lit,'.') == NULLV)
	 { strcat(tkn.value.char_lit,".v4b") ; } ;
	if ((fp = fopen((char *)v3_logical_decoder(tkn.value.char_lit,TRUE),"w")) == NULLV)
	 prsu_error(ABORT,"CREATEV4B","Could not open V4B file for writing") ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
/*	Maybe close off old V4B file & then assign this one */
	if (parse->v4b_fileptr != 0) fclose(parse->v4b_fileptr) ;
	parse->v4b_fileptr = fp ;
	return(TRUE) ;
}

int prsu_dcl_reference_checking()
{
	prsu_nxt_token(FALSE) ;
	IFSYM("ENABLED") { parse->check_initialized = TRUE ; }
	 else { IFSYM("DISABLED") { parse->check_initialized = FALSE ; }
		 else prsu_error(ERRP,"INVKW","Expecting \'ENABLED\' or \'DISABLED\' here") ;
	      } ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
 	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_startup_module()
{
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_SYMBOL)
	 prsu_error(ERRP,"INVMODSYM","Expecting a module name here") ;
	strcpy(process->package_ptrs[parse->package_id]->startup_module,tkn.symbol) ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_package_id()
{ int i ;

	prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	if (tkn.type != TKN_SYMBOL)
	 prsu_error(ERRP,"INVIDOBJ","Expecting an object name here") ;
	if ((i = sobu_object_dereference(NULLV,tkn.symbol)) == NULLV)
	 prsu_error(ERRP,"OBJNOTFOUND","Cannot locate this object") ;
	process->package_ptrs[parse->package_id]->id_obj.all = i ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
 	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_package_slot()
{

	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_BININT || tkn.lit_format.fld.decimals != 0)
	 prsu_error(ERRP,"INVDCLPCKSLOT","Expecting an integer package slot here") ;
	if (tkn.value.int_lit < 0 || tkn.value.int_lit >= V3_PACKAGE_MAX)
	 prsu_error(ERRP,"INVDCLPCKSLOTNUM","Package slot number out of allowed range") ;
	if (parse->impure_free+parse->pure_free != 1)
	 prsu_error(WARNP,"INVDCLPCKSLOTPOS","Cannot change slot number after code or variables have been allocated") ;
	if (process->package_ptrs[tkn.value.int_lit] != 0)
	 prsu_error(WARNP,"DCLPCKSLOTUSE","Cannot change slot number to package already in use") ;
/*	Looks good, change the slot number */
	psi->package_index = tkn.value.int_lit ;
	process->package_ptrs[tkn.value.int_lit] = process->package_ptrs[parse->package_id] ;
	process->package_ptrs[parse->package_id] = NULLV ;
	process->package_ptrs[tkn.value.int_lit]->package_id = tkn.value.int_lit ;
	parse->package_id = tkn.value.int_lit ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_semi)
	 prsu_error(ERRP,"NOTSEMI","Expecting end-of-statement (\';\') here") ;
	return(TRUE) ;
}

int prsu_dcl_assertions()
{

	for (parse->dcl_assertions_cnt = 0;;)
	 { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"INVASRTNAME","Expecting an ASSERTION name here") ;
	   if (parse->dcl_assertions_cnt >= V3_ASSERTIONS_PER_TABLE_MAX)
	    prsu_error(ERRP,"TOOMANYASRTS","Exceeded max number of assertions in list") ;
	   strcpy(parse->dcl_assertions[parse->dcl_assertions_cnt++],tkn.symbol) ;
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_semi) break ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma) continue ;
	   prsu_error(ERRP,"INVASRTDELIM","Expecting a comma or semicolon here") ;
	 } ;
	return(TRUE) ;
}
