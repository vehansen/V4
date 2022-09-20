/*	V3PRSB.C - PARSING UTILITIES FOR VICTIM III

	LAST EDITED 6/29/84 BY VICTOR E. HANSEN	*/

#include <time.h>
#include "v3defs.c"

/*	Global Definitions for Parser		*/
struct db__parse_info *parse ;
struct db__tknres tkn ;
jmp_buf parse_exit ;
struct db__module_entry *pcku_module_defined() ;

extern struct db__predef_table V3_FUNCTIONS[] ;
extern int PRS_FUNCTION_COUNT ;
#ifdef windows
extern struct db__predef_table V3_WINDOW_MODULES[] ;
extern int PRS_WINDOW_COUNT ;
#endif
#ifdef v3v4
extern struct db__predef_table V3_V4_MODULES[] ;
extern int PRS_V3V4_COUNT ;
#endif
extern struct db__predef_table V3_RESERVED_WORDS[] ;

/*	Global References for Execution		*/
extern struct db__process *process ;
extern struct db__psi *psi ;

/*	P R E D E F I N E D   S Y M B O L S		*/

/*	VICTIM III Predefined symbols not in main table	*/

struct notbl {
  unsigned precedence : 6 ;		/* Operator precedence */
  unsigned on_push_do : 6 ;
  unsigned on_pop_do : 4 ;
  unsigned flags : 16 ;			/* Parser flags - see above */
  unsigned reference : 16 ;
  unsigned results : 8 ;
  unsigned args : 8 ;
 } ;

struct notbl notbl_box={5,NULLV,NULLV,NULLV,NULLV,RES_NONE,0} ;
struct notbl notbl_lparen={11,NULLV,NULLV,PTI+TO,NULLV,RES_NONE,0} ;
struct notbl notbl_rparen={12,NULLV,NULLV,CHP+TO+RVL,NULLV,RES_NONE,0} ;
struct notbl notbl_star={28,PUSH_STAR,NULLV,ARITH,V3_XCT_MTH_MULTIPLY,RES_ARGS,2} ;
struct notbl notbl_star_equal={15,NULLV,NULLV,ARITH,V3_XCT_MTH_MULTEQUAL,RES_ARGS,2} ;
struct notbl notbl_plus={27,NULLV,NULLV,ARITH,V3_XCT_MTH_PLUS,RES_ARGS,2} ;
struct notbl notbl_plus_equal={15,NULLV,NULLV,ARITH,V3_XCT_MTH_PLUSEQUAL,RES_ARGS,2} ;
struct notbl notbl_comma={15,NULLV,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_minus={27,PUSH_DASH,NULLV,ARITH,V3_XCT_MTH_MINUS,RES_ARGS,2} ;
struct notbl notbl_minus_equal={15,NULLV,NULLV,ARITH,V3_XCT_MTH_MINUSEQUAL,RES_ARGS,2} ;
struct notbl notbl_dot={18,PUSH_DOT,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_dot_dot={15,PUSH_DOT_DOT,NULLV,NULLV,NULLV,RES_NONE,2} ;
struct notbl notbl_slash={28,PUSH_SLASH,NULLV,ARITH,V3_XCT_MTH_DIV,RES_ARGS,2} ;
struct notbl notbl_slash_slash={28,NULLV,NULLV,ARITH,NULLV,RES_ARGS,2} ;
struct notbl notbl_slash_equal={15,NULLV,NULLV,ARITH,V3_XCT_DIVIDEEQUAL,RES_ARGS,2} ;
struct notbl notbl_slash_star={20,NULLV,NULLV,NULLV,NULLV,RES_NONE,0} ;
struct notbl notbl_colon={15,PUSH_PATH,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_colon_colon={29,NULLV,NULLV,NULLV,V3_XCT_BIT_PACK2,RES_INT,2} ;
struct notbl notbl_colon_equal={15,NULLV,NULLV,UPD1+ARITH,V3_XCT_UPDATE,RES_ARGS,2} ;
struct notbl notbl_semi={12,PUSH_SEMI,NULLV,NULLV,V3_XCT_EOS,RES_NONE,0} ;
struct notbl notbl_langle={25,NULLV,NULLV,ARITH,V3_XCT_MTH_LESS,RES_INT,2} ;
struct notbl notbl_langle_equal={25,NULLV,NULLV,ARITH,V3_XCT_MTH_LESSEQUAL,RES_INT,2} ;
struct notbl notbl_langle_rangle={24,NULLV,NULLV,ARITH,V3_XCT_MTH_NOTEQUAL,RES_INT,2} ;
struct notbl notbl_langle_langle={9,PUSH_2LANGLE,NULLV,NULLV,V3_XCT_NULL,RES_NONE,0} ;
struct notbl notbl_rangle={26,PUSH_RANGLE,NULLV,ARITH,V3_XCT_MTH_GREATER,RES_INT,2} ;
struct notbl notbl_rangle_equal={26,NULLV,NULLV,ARITH,V3_XCT_MTH_GREATEREQUAL,RES_INT,2} ;
struct notbl notbl_rangle_rangle={10,PUSH_2RANGLE,NULLV,NULLV,V3_XCT_NULL,RES_NONE,0} ;
struct notbl notbl_lbracket={11,PUSH_LBRACKET,NULLV,NULLV,NULLV,RES_NONE,0} ;
struct notbl notbl_lrbracket={11,PUSH_LRBRACKET,NULLV,NULLV,NULLV,RES_NONE,0} ;
struct notbl notbl_rbracket={12,PUSH_RBRACKET,NULLV,TO+RVL,NULLV,RES_NONE,0} ;
struct notbl notbl_lbrace={15,PUSH_LBRACE,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_lbrace_slash={15,PUSH_LBRACE,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_rbrace={15,PUSH_RBRACE,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_rbrace_slash={15,PUSH_RBRACE,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_module_call={30,NULLV,NULLV,EOA,V3_XCT_MODCALL,RES_UNK,0} ;
struct notbl notbl_equal={24,NULLV,NULLV,ARITH,V3_XCT_MTH_EQUAL,RES_INT,2} ;
struct notbl notbl_symref={30,NULLV,POP_SYMREF,NULLV,NULLV,RES_NONE,0} ;
struct notbl notbl_structptr={31,NULLV,NULLV,NULLV,V3_XCT_STRUCTPTR,RES_UNK,1} ;
struct notbl notbl_unaryminus={30,NULLV,NULLV,ARITH,V3_XCT_MTH_UNARYMINUS,RES_ARGS,1} ;
struct notbl notbl_equal_equal={15,NULLV,NULLV,UPD1,V3_XCT_EQUAL_EQUAL,RES_UNK,2} ;
struct notbl notbl_partial={15,NULLV,POP_PARTIAL,NULLV,V3_XCT_PARTIAL,RES_UNK,2} ;
struct notbl notbl_elseif={13,PUSH_ELSEIF,NULLV,TO,NULLV,RES_UNK,0} ;
struct notbl notbl_cross_lbrace = {15,PUSH_START_CONDCOMP,NULLV,TO,NULLV,RES_UNK,0} ;
struct notbl notbl_rbrace_cross = {15,PUSH_END_CONDCOMP,NULLV,TO,NULLV,RES_UNK,0} ;
struct notbl notbl_cross = {15,NULLV,NULLV,NULLV,NULLV,RES_NONE,0} ;
struct notbl notbl_ampersand = {22,NULLV,NULLV,NULLV,V3_XCT_AND,RES_INT,2} ;
struct notbl notbl_line = {21,NULLV,NULLV,NULLV,V3_XCT_OR,RES_INT,2} ;
struct notbl notbl_tilde = {23,NULLV,NULLV,NULLV,V3_XCT_NOT,RES_INT,1} ;
struct notbl notbl_dash_rangle = {15,PUSH_OUT,NULLV,TO,NULLV,RES_NONE,0} ;
struct notbl notbl_langle_dash = {30,PUSH_CONTINUE,NULLV,NULLV,NULLV,RES_NONE,0} ;
struct notbl notbl_question = {15,PUSH_BRANCH,POP_BRANCH,NULLV,NULLV,RES_UNK,1} ;
struct notbl notbl_quote = { 0,0,0,0,0,0,0 } ;

/*	T O K E N   G E N E R A T O R			*/

/*	type_of_token - 128 byte array describing each ASCII character for start of token */
char type_of_token[128] ;


 char *prsu_nxt_token(tg_flags)
  int tg_flags ;		/* Generator flags - If TRUE then set "lookahead" flag for next call */
{
   static char symbol_translation[128],numeric_array[128],need_type_setup=TRUE ;
   static char have_lookahead=FALSE ;
   struct V4LEX__BigText *bt ;
   unsigned char dps,have_dp,sc ; char *iptr,*optr ;
   char *tkn_sym,sym_char,delimiter,*str_ptr,have_exp ; char namebuf[V3_PRS_SYMBOL_MAX+1] ;
   static char *rescan_ptr ;
   int i,j,res,hash ; int *ip ;
   double fres ;
   extern int PRS_RESERVED_WORD_COUNT ;	/* From V3PRSA.C */

/*	If first call then have to init type_of_token array	*/
	if (need_type_setup)
	 { need_type_setup = FALSE ;
	   for (i=0;i < 128;i++) type_of_token[i] = i ;
	   type_of_token[0] = (type_of_token[10] = (type_of_token[12] = (type_of_token[13] = END_OF_LINE))) ;
	   prsu_array_setup(type_of_token,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_",SYMBOL) ;
	   prsu_array_setup(type_of_token,"\042'",STRING_LIT) ;
	   prsu_array_setup(type_of_token,"0123456789",NUMERIC_LIT) ;
	   prsu_array_setup(type_of_token,"^",RADIX_OVERRIDE) ;
/*	   Set up table for symbols */
	   for (i='A';i<='Z';i++)
	    { symbol_translation[i] = i ; symbol_translation[i-'A'+'a'] = i ; } ;
	   for (i='0';i<='9';i++)
	    { symbol_translation[i] = i ; } ;
	   symbol_translation['_'] = '_' ;
/*	   Set up for numeric literals */
	   prsu_array_setup(numeric_array,"!|@#$%^&*()-_+=`~{]}\\;:,<>/?",END_OF_NUMBER) ;
	   for (i=0;i<=' ';i++) numeric_array[i] = END_OF_NUMBER ;
	   prsu_array_setup(numeric_array,"0123456789",DIGIT) ;
	   prsu_array_setup(numeric_array,"ABCDEFabcdef",HEX_DIGIT) ;
	   numeric_array['.'] = DECIMAL_POINT ; numeric_array['_'] = IGNORE ;
	 } ;

/*	See if setting have_lookahead flag or if we got it */
	if (tg_flags & 1)
	 { parse->have_lookahead = TRUE ; parse->ilvl[parse->ilx].input_ptr = parse->prior_input_ptr ; return(rescan_ptr) ; } ;
	if (parse->have_lookahead) parse->have_lookahead = FALSE ;

top_of_module:
/*	If we need another input line then grab it	*/
	if (parse->need_input_line) prsu_nxt_line() ;
/*	Check to see if we are in a comment */
	if (parse->in_comment)
	 { if ((parse->ilvl[parse->ilx].input_ptr = (char *)strchr(parse->ilvl[parse->ilx].input_ptr,'*')) == NULLV)
	    { parse->need_input_line = TRUE ; goto top_of_module ; } ;
/*	   Found a '*', see if last or next is a '/' */
	   if (*(parse->ilvl[parse->ilx].input_ptr - 1) == '/')
	    prsu_error(ERRP,"NESTCOMMENTS","Cannot nest comments") ;
	   if (*(parse->ilvl[parse->ilx].input_ptr+1) != '/') { parse->ilvl[parse->ilx].input_ptr++ ; goto top_of_module ; } ;
/*	   End of comment - kick out of this little test */
	   parse->ilvl[parse->ilx].input_ptr += 2 ; parse->in_comment = FALSE ;
	 } ;
/*	Set up some fields in token value format */
	tkn.lit_format.all = 0 ; tkn.lit_format.fld.mode = VFM_IMMEDIATE ;
	tkn.sym_ptr = NULLV ;
/*	Skip over tabs, spaces, whatever	*/
	parse->prior_input_ptr = parse->ilvl[parse->ilx].input_ptr ;
	parse->ilvl[parse->ilx].input_ptr += strspn(parse->ilvl[parse->ilx].input_ptr," 	") ;
	rescan_ptr = parse->ilvl[parse->ilx].input_ptr ;	/* Save pointer to begin of token */
/*	Check next character to see what type of token we have */
	switch (type_of_token[*parse->ilvl[parse->ilx].input_ptr])
	 { case END_OF_LINE:
/*		Check for page marker (form-feed) */
		if (*parse->ilvl[parse->ilx].input_ptr == 12) parse->ilvl[parse->ilx].file_page++ ;
		parse->need_input_line = TRUE ; goto top_of_module ;
	   case SYMBOL:
		tkn_sym = tkn.symbol ; parse->ilvl[parse->ilx].input_ptr-- ; tkn.actual_symbol_len = 0 ;
/*		Loop thru all chars in symbol & normalize */
		for (i=0;i<V3_PRS_SYMBOL_MAX;i++)
		 { if (*(tkn_sym++) = symbol_translation[*(++(parse->ilvl[parse->ilx].input_ptr))]) {tkn.actual_symbol_len++ ;}
		    else goto end_of_symbol ;
		 } ;
/*		If here then symbol too long - scan to end but don't include in tkn.symbol */
		while (symbol_translation[*(++(parse->ilvl[parse->ilx].input_ptr))]) tkn.actual_symbol_len++ ;
end_of_symbol:
		tkn.symbol_len =
		   (tkn.actual_symbol_len > V3_PRS_SYMBOL_MAX ? V3_PRS_SYMBOL_MAX : tkn.actual_symbol_len) ;
/*		Do symbol lookup in predefined table */
		if (tg_flags & TKN_FLAGS_STREL) { tkn.type = TKN_SYMBOL ; }
		 else { if ((tkn.sym_ptr = (struct db__opr_info *)prsu_predefined_lookup(tkn.symbol,V3_RESERVED_WORDS,PRS_RESERVED_WORD_COUNT)) != NULLV)
			 { tkn.type = TKN_PREDEFINED ; } else { tkn.type = TKN_SYMBOL ; } ;
		      } ;
/*		Update the current checksum */
		if (!parse->enable_checksums)
		 { parse->current_checksum += (++parse->token_count)*prsu_str_hash(tkn.symbol,-1) ; } ;
		return(rescan_ptr) ;

	   case STRING_LIT:
/*		Set up to scan to ending delimeter */
		delimiter = *(parse->ilvl[parse->ilx].input_ptr++) ;
		tkn.literal_len = 0 ; str_ptr = tkn.value.char_lit ;
		for (;;)
		 { if (*(parse->ilvl[parse->ilx].input_ptr) == delimiter)
		    { if (*(++(parse->ilvl[parse->ilx].input_ptr)) != delimiter) break ;
/*		      Got double delimiter - append it */
		      tkn.literal_len++ ; *(str_ptr++) = delimiter ; parse->ilvl[parse->ilx].input_ptr++ ; continue ;
		    } ;
		   if (++tkn.literal_len > V3_LITERAL_STRING_MAX)
		     prsu_error(ERRP,"STRLITTOOBIG","String literal exceeds maximum length allowed by compiler") ;
/*		   Check for special "\x" characters */
		   if (*parse->ilvl[parse->ilx].input_ptr == '\\')
		    { switch (*(++(parse->ilvl[parse->ilx].input_ptr)))
			{ default:
/*				   Check for upper case letters */
				    if (*parse->ilvl[parse->ilx].input_ptr >= 'A' && *parse->ilvl[parse->ilx].input_ptr <= 'Z')
				     { *(str_ptr++) = *parse->ilvl[parse->ilx].input_ptr - 'A' + 1 ; break ; } ;
/*				    Last chance - maybe a decimal number */
				    if (*parse->ilvl[parse->ilx].input_ptr < '0' || *parse->ilvl[parse->ilx].input_ptr > '9')
				     prsu_error(WARNP,"BADBACKSLSH","Invalid \'\\x\' in string literal") ;
				    for(*str_ptr = 0;;parse->ilvl[parse->ilx].input_ptr++)
				     { sc = *(parse->ilvl[parse->ilx].input_ptr) ;
				       if (sc < '0' || sc > '9') break ; *str_ptr = *str_ptr * 10 + sc-'0' ;
				     } ;
				    parse->ilvl[parse->ilx].input_ptr -- ; str_ptr ++ ; break ;
			  case 'n': ++tkn.literal_len ; *(str_ptr++) = 13 ; *(str_ptr++) = 10 ; break ;
			  case '$': *(str_ptr++) = 27 ; break ;
			  case 'b': *(str_ptr++) = 7 ; break ;
			  case '\\': *(str_ptr++) = '\\' ; break ;
			  case '<': *(str_ptr++) = 8 ; break ;
			  case 'f': *(str_ptr++) = 12 ; break ;
			  case 't': *(str_ptr++) = 9 ; break ;
			  case '"': *(str_ptr++) = '"' ; break ;
			  case '\'': *(str_ptr++) = '\'' ; break ;
			  case '!': *(str_ptr++) = '!' ; break ;
			  case 'l': *(str_ptr++) = 10 ; break ;
			  case 'r': *(str_ptr++) = 13 ; break ;
			} ;
		      ++(parse->ilvl[parse->ilx].input_ptr) ; continue ;
		    } ;
/*		   Make sure we have not run off the end of the input line */
		   if (type_of_token[*(str_ptr++) = *(parse->ilvl[parse->ilx].input_ptr++)] == END_OF_LINE)
/*		      End of line - check to see if '&' terminated */
		    { if (*(str_ptr-2) == '&')
		       { prsu_nxt_line() ; str_ptr+=-2 ; tkn.literal_len+=-2 ; continue ; }
		       else if ( *(str_ptr-2) == ' ' && *(str_ptr-3) == '&')
			     { prsu_nxt_line() ; str_ptr+=-3 ; tkn.literal_len+=-3 ; continue ; }
		       else { parse->need_input_line = TRUE ; prsu_error(ERROR,"UNTERMLIT","Unterminated string literal") ; } ;
/*		      Got continued string literal - read in next line & continue */
		    } ;
		 } ;
/*		End of string literal - set up value format */
		tkn.lit_format.fld.type = VFT_FIXSTR ; tkn.lit_format.fld.length = tkn.literal_len ;
		tkn.type = TKN_LITERAL ; tkn.lit_format.fld.mode = VFM_PTR ;
/*		Update the current checksum */
		if (parse->enable_checksums)
		 { str_ptr = tkn.value.char_lit ; j = 0 ;
		   for(i=0;i<tkn.literal_len;i++) { j += *(str_ptr++)+i ; } ;
		   parse->current_checksum += (++parse->token_count)*j ;
		 } ;
		return(rescan_ptr) ;

	   case RADIX_OVERRIDE:
		switch (*(++(parse->ilvl[parse->ilx].input_ptr)))
		 { case 'a':
		   case 'A': parse->ilvl[parse->ilx].input_ptr++ ; prsu_nxt_token(FALSE) ;
/*			     Best have a string literal */
			     if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR || tkn.lit_format.fld.length != 1)
			      prsu_error(ERRP,"INVNUMSTRLIT","Expecting single character string literal following \'^A\'") ;
			     res = tkn.value.char_lit[0] ; dps = 0 ; have_exp = 0 ; goto numeric_literal_finish ;
		   case 'b':
		   case 'B': tkn.radix = 2 ; break ;
		   case 'd':
		   case 'D': tkn.radix = 10 ; break ;
		   case 'o':
		   case 'O': tkn.radix = 8 ; break ;
		   case 'x':
		   case 'X': tkn.radix = 16 ; break ;
		   default: prsu_error(WARNP,"BADRADIX","Radix prefix must be one of: B, D, O, or X") ;
		 } ;
		parse->ilvl[parse->ilx].input_ptr++ ; goto radix_entry ;

	   case NUMERIC_LIT:
/*		Assume default radix unless overridden (see above) */
		tkn.radix = parse->default_radix ;
radix_entry:
/*		Init all sorts of stuff */
		res = 0 ;	/* Initial result */
		dps = 0 ;	/* Number of decimal places */
		have_dp = 0 ; 	/* Set to 1 on "." */
		fres = 0.0 ; 	/* Floating point temp result */
		have_exp = 0 ;	/* Set to +/- 1 on pos/neg exponent */
/*		If literal is decimal then make 'E' for exponential */
		numeric_array['e'] = (numeric_array['E'] = (tkn.radix == 10 ? EXPONENT : HEX_DIGIT)) ;
/*		Loop thru all characters in numeric literal */
		for (;;)
		 { switch (numeric_array[sc = *parse->ilvl[parse->ilx].input_ptr++])
		    { case DIGIT:
			if ((i = sc-'0') >= tkn.radix)
			  prsu_error(WARNP,"INVDIGIT","Digit too large for current radix") ;
			if (have_exp == 0) fres = fres*10.0 + (double)i ;
			dps += have_dp ; res = (res*tkn.radix)+i ; break ;
		      case HEX_DIGIT:
			if (sc >= 'a') sc += -32 ;
			if ((i = sc+10-'A') >= tkn.radix)
			 prsu_error(WARNP,"NOTHEX","Letters \'A\' thru \'F\' allowed only in HEX literals") ;
			res = (res<<4)+i ; break ;
		      case DECIMAL_POINT:
/*			Check for ".." */
			if (*parse->ilvl[parse->ilx].input_ptr == '.')
			 { parse->ilvl[parse->ilx].input_ptr-- ; goto numeric_literal_finish ; } ;
			if (have_dp) prsu_error(WARNP,"TOOMANYDPS","Only one decimal point allowed") ;
			if (have_exp) prsu_error(WARNP,"NODPINEXP","Decimal point not allowed in exponent") ;
			if (tkn.radix != 10) prsu_error(WARNP,"INVRADIX","Decimal point allowed only in decimal radix literals") ;
			have_dp++ ; break ;
		      case EXPONENT:
			if (have_exp != 0) prsu_error(WARNP,"TOOMANYDPS","Only one decimal point allowed") ;
			if (tkn.radix != 10) prsu_error(WARNP,"NOTDECIMAL","Floating point must be radix=10") ;
/*			Look for optional "+" or "-" */
			have_exp = 1 ; have_dp = 0 ;
			if (*(parse->ilvl[parse->ilx].input_ptr) == '+') {parse->ilvl[parse->ilx].input_ptr++ ; }
			 else {if (*(parse->ilvl[parse->ilx].input_ptr) == '-')
				{ parse->ilvl[parse->ilx].input_ptr++ ; have_exp = -1 ; } ;
			      } ;
			res = 0 ; break ;
		      case END_OF_NUMBER: parse->ilvl[parse->ilx].input_ptr-- ; goto numeric_literal_finish ;
		      case IGNORE: break ;
		      default:
			prsu_error(WARNP,"BADNUMLITTERM","Invalid character in/terminating numeric literal") ;
		    } ;
		 } ;
/*		Here to finish off the literal */
numeric_literal_finish:
		if (have_exp != 0)
		 { if ((i = have_exp*(res-dps)) == 0) { tkn.value.float_lit = fres ; }
		    else { tkn.value.float_lit = fres * pow(10.0,(double)i) ; } ;
		   tkn.lit_format.fld.type = VFT_FLOAT ; tkn.lit_format.fld.mode = VFM_PTR ;
		   tkn.literal_len = (tkn.lit_format.fld.length = 8) ;
		 }
		 else
		 { tkn.value.int_lit = res ; tkn.lit_format.fld.decimals = dps ;
		   tkn.lit_format.fld.type = VFT_BININT ; tkn.lit_format.fld.mode = VFM_IMMEDIATE ;
		   tkn.literal_len = (tkn.lit_format.fld.length = 4) ;
		 } ;
		tkn.type = TKN_LITERAL ;
/*		Update the current checksum */
		if (parse->enable_checksums)
		 { parse->current_checksum += (++parse->token_count)*tkn.value.int_lit ; } ;
		return(rescan_ptr) ;

#define PUNC1(PUNC) tkn.sym_ptr = (struct db__opr_info *)&PUNC ; goto single_punc
#define PUNC2(CHAR,PUNC) if ((*(parse->ilvl[parse->ilx].input_ptr+1)) == CHAR) {tkn.sym_ptr = (struct db__opr_info *)&PUNC ; goto double_punc ; }
	   case '#':
/*		Check for "#{" construct */
		if (*(parse->ilvl[parse->ilx].input_ptr+1) == '{')
		 { tkn.sym_ptr = (struct db__opr_info *)&notbl_cross_lbrace ; goto double_punc ; } ;
/*		If flag is set then don't try to expand "#xxx" */
		if (tg_flags & TKN_FLAGS_NOVALUE || parse->ilvl[parse->ilx].condcomp_ignore > 0) { PUNC1(notbl_cross) ; } ;
/*		Get next symbol and do lookup in constant table */
		parse->ilvl[parse->ilx].input_ptr++ ; prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
		if (tkn.type != TKN_SYMBOL)
		 prsu_error(ERRP,"BADVALSYM","Expecting a symbolic value to follow \'#\'") ;
		hash = prsu_str_hash(tkn.symbol,-1) ; strcpy(namebuf,tkn.symbol) ;
		for(j=2;j>0;j--)
		 { for (i=parse->constants.count-1;i>=0;i--)
		    { if (hash != parse->constants.sym[i].hash) continue ;
		      if (strcmp(namebuf,parse->constants.sym[i].name) == 0) goto found_constant ;
		    } ;
		   if (!prs_attempt_struct_load(namebuf))
		    prsu_error(ERRP,"UNDEFVALUE","No such #value/macro has been defined") ;
		   prsu_nxt_token(FALSE) ; prsu_dcl(TRUE) ;
		 } ;
found_constant:
/*		Found it - back up current pointer & continue lexical with new string */
		if (parse->constant_bkp_count >= V3_PRS_VALUE_NEST_MAX)
		 { parse->ilvl[parse->ilx].input_ptr = parse->constant_input_ptr_bkp[1] ; parse->constant_bkp_count = 0 ;
		   prsu_error(ERRP,"NESTVALUE","Too many nested \'#xxx\' values or macros") ;
		 } ;
/*		If a macro then look for "( args... )" */
		if (parse->constants.sym[i].is_macro)
		 { prsu_nxt_token(FALSE) ;
		   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lparen)
		    prsu_error(ERRP,"NOTLPAREN","Expecting #macro to be followed by \'(\'") ;
		   for (j=0;j<10;j++)
		    { prsu_nxt_token(FALSE) ;
		      if (tkn.type == TKN_SYMBOL)
		       { strcpy(parse->constants.args[j],tkn.symbol) ; }
		       else
		       { if (!(tkn.type == TKN_LITERAL && tkn.lit_format.fld.type == VFT_FIXSTR))
			  prsu_error(ERRP,"INVMACROARG","A #macro arg must be string literal or symbol") ;
			 strncpy(parse->constants.args[j],tkn.value.char_lit,tkn.literal_len) ;
			 parse->constants.args[j][tkn.literal_len] = NULLV ;
		       } ;
		      prsu_nxt_token(FALSE) ;
		      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rparen) break ;
		      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_comma)
		       prsu_error(ERRP,"NOTCOMMA","Expecting a comma or \')\' here") ;
		    } ;
/*		   Clear out remaining argument slots */
		   for(j=j+1;j<10;j++) { parse->constants.args[j][0] = 0 ; } ;
/*		   Now substitute arg values into string */
		   iptr = &parse->constants.buf[parse->constants.sym[i].offset] ; optr = parse->constants.expansion ;
		   for (;*iptr != 0;)
		    { if (*iptr != '#') { *(optr++) = *(iptr++) ; continue ; } ;
		      if (*(iptr+2) != '#') { *(optr++) = *(iptr++) ; continue ; } ;
		      j = *(++iptr) - '1' ; iptr += 2 ;
		      if (parse->constants.args[j][0] == 0)
		       prsu_error(ERRP,"MISSINGARG","MACRO argument has not been defined") ;
		      strcpy(optr,parse->constants.args[j]) ; optr += strlen(parse->constants.args[j]) ;
		     } ;
/*		   End of expansion, terminate with null & inject */
xp1:		   *optr = NULLV ;
		   parse->constant_input_ptr_bkp[++parse->constant_bkp_count] = parse->ilvl[parse->ilx].input_ptr ;
		   parse->constant_start_ptr[parse->constant_bkp_count] = (parse->ilvl[parse->ilx].input_ptr = parse->constants.expansion) ;
		   prsu_nxt_token(FALSE) ; return(rescan_ptr) ;
		 } ;
		parse->constant_input_ptr_bkp[++parse->constant_bkp_count] = parse->ilvl[parse->ilx].input_ptr ;
		parse->constant_start_ptr[parse->constant_bkp_count] =
		 (parse->ilvl[parse->ilx].input_ptr = &(parse->constants.buf[parse->constants.sym[i].offset])) ;
		prsu_nxt_token(FALSE) ; return(rescan_ptr) ;
	   case '&':	PUNC1(notbl_ampersand) ;
	   case '|':	PUNC1(notbl_line) ;
	   case '~':	PUNC1(notbl_tilde) ;
	   case '?':	PUNC1(notbl_question) ;
	   case '(':	PUNC1(notbl_lparen) ;
	   case ')':	PUNC1(notbl_rparen) ;
	   case '*':	PUNC2('=',notbl_star_equal) ; PUNC1(notbl_star) ;
	   case '+':	PUNC2('=',notbl_plus_equal) ; PUNC1(notbl_plus) ;
	   case ',':	PUNC1(notbl_comma) ;
	   case '-':	PUNC2('=',notbl_minus_equal) ; PUNC2('>',notbl_dash_rangle) ; PUNC1(notbl_minus) ;
	   case '.':	PUNC2('.',notbl_dot_dot) ; PUNC1(notbl_dot) ;
	   case '/':	PUNC2('*',notbl_slash_star) ; PUNC2('=',notbl_slash_equal) ;
			PUNC2('/',notbl_slash_slash) ; PUNC1(notbl_slash) ;
	   case ':':	PUNC2(':',notbl_colon_colon) ; PUNC2('=',notbl_colon_equal) ; PUNC1(notbl_colon) ;
	   case ';':	PUNC1(notbl_semi) ;
	   case '<':	PUNC2('>',notbl_langle_rangle) ; PUNC2('=',notbl_langle_equal) ; PUNC2('-',notbl_langle_dash) ;
			PUNC2('<',notbl_langle_langle) ; PUNC1(notbl_langle) ;
	   case '>':	PUNC2('=',notbl_rangle_equal) ; PUNC2('>',notbl_rangle_rangle) ; PUNC1(notbl_rangle) ;
	   case '=':	PUNC2('=',notbl_equal_equal) ; PUNC1(notbl_equal) ;
	   case '[':	PUNC2(']',notbl_lrbracket) ; PUNC1(notbl_lbracket) ;
	   case ']':	PUNC1(notbl_rbracket) ;
	   case '{':	PUNC2('/',notbl_lbrace_slash) ; PUNC1(notbl_lbrace) ;
	   case '}':	PUNC2('/',notbl_rbrace_slash) ; PUNC2('#',notbl_rbrace_cross) ; PUNC1(notbl_rbrace) ;
	   case '@':	PUNC1(notbl_quote) ;
/*	   Here on unknown VICTIM III character */
	   default:	parse->ilvl[parse->ilx].input_ptr++ ;
			prsu_error(ERRP,"INVCHAR","This character allowed only within string literals") ;
	 } ;
double_punc:
	parse->ilvl[parse->ilx].input_ptr++ ;	/* Advance pointer */
single_punc:
	parse->ilvl[parse->ilx].input_ptr++ ;	/* Advance (again) */
	tkn.type = TKN_PREDEFINED ;
/*	Check for start of slash-star comment */
	if (tkn.sym_ptr == (struct db__opr_info *)&notbl_slash_star)
	 { parse->in_comment = TRUE ; goto top_of_module ; } ;
/*	Update the current checksum */
	if (parse->enable_checksums)
	 { ip = (int *)tkn.sym_ptr ;
	   parse->current_checksum += (++parse->token_count)*(*ip + *(ip+1)) ; } ;
	return(rescan_ptr) ;
}

/*	prsu_nxt_line - Read next line of input		*/
void prsu_nxt_line()
{ unsigned short *ln ; char *b ;

	parse->need_input_line = FALSE ;
/*	First check for input backup due to constant	*/
	if (parse->constant_bkp_count > 0)
	 { parse->ilvl[parse->ilx].input_ptr = parse->constant_input_ptr_bkp[parse->constant_bkp_count--] ;
	   return ;
	 } ;
get_another_line:
	for (;;)
	 { parse->ilvl[parse->ilx].current_line++ ; parse->ilvl[parse->ilx].total_lines++ ;
	   if (parse->ilvl[parse->ilx].file == NULLV)
	    { if (parse->ilx > 1)	 /* If not top level then must be injected string - pop */
	       { if (parse->ilvl[parse->ilx].free_ptr != NULLV) v4mm_FreeChunk(parse->ilvl[parse->ilx].free_ptr) ;
		 parse->ilx-- ; return ;
	       } ;
	      if (iou_input_tty(parse->ilvl[parse->ilx].prompt,
				parse->ilvl[parse->ilx].input_str,V3_PRS_INPUT_LINE_MAX,TRUE)) break ;
/*	      Hit eof on tty input - pop up level if possible */
	      if (parse->ilx <= 0)
		prsu_error(WARN,"ATTOPLEVEL","Cannot pop out of top level input") ;
	      parse->ilx-- ; return ;
	    } ;
/*	   Here to get input from file */
	   if (fgets(parse->ilvl[parse->ilx].input_str,V3_PRS_INPUT_LINE_MAX,parse->ilvl[parse->ilx].file) != NULL)
	    { if (! (parse->ilvl[parse->ilx].is_not_sos_file || parse->ilvl[parse->ilx].is_sos_file))
/*		 Have to determine the file type */
	       { if (parse->ilvl[parse->ilx].input_str[1] < 9)
		  {parse->ilvl[parse->ilx].is_sos_file = TRUE ; }
		  else parse->ilvl[parse->ilx].is_not_sos_file = TRUE ;
	       } ;
	      if (parse->ilvl[parse->ilx].is_not_sos_file) break ;
/*	      Have an SOS file - get line number */
/*	      (First check for line number matching LF */
	      if (parse->ilvl[parse->ilx].input_str[0] == 10 && parse->ilvl[parse->ilx].input_str[1] == NULLV)
	       { fgets(&parse->ilvl[parse->ilx].input_str[1],V3_PRS_INPUT_LINE_MAX,parse->ilvl[parse->ilx].file) ;
	       } ;
	      if (parse->ilvl[parse->ilx].input_str[1] == 10)
	       { fgets(&parse->ilvl[parse->ilx].input_str[2],V3_PRS_INPUT_LINE_MAX,parse->ilvl[parse->ilx].file) ;
	       } ;
	      ln = (unsigned short *)&parse->ilvl[parse->ilx].input_str ; parse->ilvl[parse->ilx].current_line = *ln ;
	      parse->ilvl[parse->ilx].input_str[0] = (parse->ilvl[parse->ilx].input_str[1] = ' ') ;
	      break ;
	    } ;
/*	    End of file on input - close off & pop up level */
	    fclose(parse->ilvl[parse->ilx].file) ; parse->ilx -= 1 ;
/*	    Check to see if comment not finished or in wrong level */
	    if (parse->in_comment)
	     { parse->in_comment = FALSE ; parse->ilvl[parse->ilx].input_ptr = parse->ilvl[parse->ilx].input_str ;
	       prsu_error(WARN,"EOFINCOMMENT","Reached end of source file while in \'/* xxx */\' comment") ;
	     } ;
	    if (parse->level != parse->ilvl[parse->ilx+1].level_on_entry)
	     prsu_error(ABORT,"EOFWRONGLEVEL","Reached end of source file at different \'{ xxx }\' level than on entry") ;
	    if (parse->ilvl[parse->ilx+1].condcomp_nest || parse->ilvl[parse->ilx].condcomp_ignore)
	     prsu_error(ABORT,"EOFCONDCOMP","Reached end of source file while still conditional compilation") ;
	    if (parse->dcl_nest > 0)
	     prsu_error(ABORT,"EOFDCLNEST","Reached end of source file while still in DCL xxx") ;
	    if (parse->ilvl[parse->ilx].file == NULLV)
	     { if (parse->v4b_fileptr != 0)				/* Maybe close off V4 binding info file */
		{ fclose(parse->v4b_fileptr) ; parse->v4b_fileptr = 0 ; } ;
	     } ;
/*	    If this is the lowest level then execute "return" to go back wherever */
	    if ((parse->ilx) == 0) xctu_return() ;
	    if (parse->ilvl[parse->ilx].input_ptr != NULL) return ;
	 } ;
/*	Update input_ptr depending on SOS file or not */
	parse->ilvl[parse->ilx].input_ptr = (parse->ilvl[parse->ilx].is_sos_file ? &parse->ilvl[parse->ilx].input_str[2] : parse->ilvl[parse->ilx].input_str) ;
	if (*parse->ilvl[parse->ilx].input_ptr == '!') goto get_another_line ;
	if (*parse->ilvl[parse->ilx].input_ptr == '#')	/* Maybe skip begin of line: #nnn.nn */
	 { for(b=parse->ilvl[parse->ilx].input_ptr+1;;b++)
	    { if (*b >= '0' && *b <= '9') continue ; if (*b == '.') continue ;
	      break ;
	    } ;
	   if (b != parse->ilvl[parse->ilx].input_ptr + 1) parse->ilvl[parse->ilx].input_ptr = b ;
	 } ;
	if (parse->ilvl[parse->ilx].statement_start_line == 0)
	 parse->ilvl[parse->ilx].statement_start_line = parse->ilvl[parse->ilx].current_line ;
	return ;
}

/*	prsu_eval_constant - Evaluates /xxx/ expression & returns index into pure space or -1 if stashed immediate */
/*	  NOTE: This routine assumes the initial "/" has been parsed */

int prsu_eval_constant(code_flag,obflag)
  int code_flag ;			/* If TRUE then don't try to generate code */
  int obflag ;				/* If TRUE then stash in object buffer, otherwise pure/code space */
{ int flag_value=0 ;			/* Start with no flags */
   int flag_tmp ;

	for (;;)
	 { prsu_nxt_token(FALSE) ;
	   if (tkn.type != TKN_SYMBOL)
	    { if (tkn.type == TKN_LITERAL && tkn.lit_format.fld.type == VFT_BININT)
	       { switch (tkn.lit_format.fld.decimals)
		  { default: prsu_error(WARNP,"MAX3DPS","Number allowed maximum of three decimal places here") ;
			    tkn.lit_format.fld.decimals = 3 ; break ;
		    case 0: tkn.value.int_lit *= 0x100 ; break ;
		    case 1: tkn.value.int_lit = 0x100*(tkn.value.int_lit/10)+tkn.value.int_lit % 10 ; break ;
		    case 2: tkn.value.int_lit = 0x100*(tkn.value.int_lit/100)+tkn.value.int_lit % 100 ; break ;
		    case 3: tkn.value.int_lit = 0x100*(tkn.value.int_lit/1000)+tkn.value.int_lit % 1000 ; break ;
		  }
		 if (tkn.value.int_lit > 0xFFFF)
		  prsu_error(WARNP,"NUMTOOBIG","Numeric value is too large here") ;
		 flag_value |= (tkn.value.int_lit*0x10000) ; goto slash_next ;
	       } ;
	      prsu_error(ERRP,"BADNUMSYM","Expecting a symbol or number here") ;
	    } ;
/*	   If here then have a symbol - do lookup & or into i */
	   flag_tmp = mscu_flag_lookup(tkn.symbol) ;
	   flag_value |= flag_tmp ;
/*	   Now see what is next */
slash_next:
	   prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma || tkn.sym_ptr == (struct db__opr_info *)&notbl_plus) continue ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_slash) break ;
	   prsu_error(ERRP,"INVFLAGTERM","Expecting a comma, plus-sign or slash here") ;
	 } ;
/*	All done scan, save result as literal */
/*	If integer literal is small enough then store as CODE_INT_LITERAL */
	if (flag_value <= 0xFFF && flag_value >= 0 && (!code_flag))
	 { PUSHCODE(CODE_INT_LITERAL,flag_value) ; return(-1) ; } ;
	tkn.value.int_lit = flag_value ; tkn.lit_format.all = 0 ;
	tkn.lit_format.fld.mode = VFM_IMMEDIATE ; tkn.lit_format.fld.type = VFT_BININT ;
	tkn.lit_format.fld.length = (tkn.literal_len = 4) ;
/*	Now stash the lit & return where it went */
	return(prsu_stash_lit(obflag)) ;
}

/*	P A R S E R   L E V E L   B E G I N  &  E N D	*/

/*	prs_begin_level - starts a new parser level	*/
/*	 (May also create module entry & dummy arguments) */

void prs_begin_level(name,interpretive_flag,iterative_flag,command_flag)
  char *name ;			/* Optional (may be NULLV) level name */
  int interpretive_flag ;	/* If TRUE then this level is parsed interpretively */
  int iterative_flag ;		/* If TRUE then this level is IF/LOOP/... level */
  int command_flag ;		/* If TRUE then this level is a command (<< ... >>) */
{ struct db__prs_level *new_level ;
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Binding bindpt ;
  struct V4DPI__Point isctpt,valpt ;
  char tbuf[300] ;
   char so ;			/* Stack offset counter for dummy args */
   struct db__prs_sym *sp ;	/* Points to symbol table entry */
   struct db__module_entry *me ;/* Points to module entry block */
   struct db__formatmempos *se ;
   int mex,pbx,i,len ;

/*	If current level is BRANCH but don't yet have "{" then don't create another level */
	if (parse->level != NULLV ?
	    ((parse->level->in_branch || parse->level->in_loop) && !parse->level->have_brace) : FALSE)
	 { parse->level->have_brace = TRUE ;
	   if (name != NULLV) strcpy(parse->level->name,name) ;
	   return ;		/* That's all for this call ! */
	 } ;
/*	Allocate a new level block */
	new_level = (struct db__prs_level *)v4mm_AllocChunk(sizeof *new_level,TRUE) ;
	new_level->interpretive = interpretive_flag ;
/*	If we got a name then copy it in */
	if (name != NULLV) strcpy(new_level->name,name) ;
/*	Save temp opr stack index */
	new_level->prior_tmp_index = parse->tmp_index ;
/*	Assign level its unique id */
	new_level->level_id = (++parse->last_level_id) ;
/*	Check for a command level - Set some stuff if so */
	if (new_level->is_command = command_flag)
	 { new_level->command_level_id = new_level->level_id ;
	   new_level->in_command = TRUE ;
	 } else
	 { if (parse->level != NULLV)
	    { new_level->in_command = parse->level->in_command ;
	      new_level->command_level_id = parse->level->command_level_id ;
	    } ;
	 } ;
/*	Save current code offset for begin of level */
	new_level->top_of_level = parse->code_offset ;
/*	Link new level up to old */
	if ((new_level->prior_level = parse->level) != NULLV)
	 new_level->prior_sym_index = parse->level->prior_sym_index ;
	parse->level = new_level ;
	if (interpretive_flag) return ;	/* Don't check for module if interpretive ! */
	if (parse->level->iterative = iterative_flag) return ;
/*	Now check to see if we got a module entry point */
	if (name == NULLV)
	 { if (parse->level->prior_level->interpretive)
	    prsu_error(WARN+POINT,"NOENTRYNAME","Starting level without any entry point") ;
	   return ;
	 } ;
/*	Have a name, see if followed with "(...)" */
	prsu_nxt_token(FALSE) ;
	if (! (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen))
	 { prsu_nxt_token(TRUE) ;	/* Level not a module - set token lookahead */
	   if (new_level->is_command)
	    prsu_error(ERRP,"INVCOMENTRY","Command entry MUST be of form \'<</name(xxx)\'") ;
	   return ;
	 } ;
	if (command_flag)
	 { mex = prsu_alloc_pure(sizeof *me,ALIGNofPTR) ; me = (struct db__module_entry *)&parse->pure_buf[mex] ;
	   me->code_offset = PRSLEN(&parse->code[parse->code_offset],me) ; me->package_id = parse->package_id ;
	   me->stack_bytes = 0 ;
	   strcpy(me->name,name) ;	/* Copy the name */
	   new_level->module_entry_offset = mex ;
	 } else
/*	   Got a module entry - Add to symbol table */
	 { if (new_level->in_command)
	    prsu_error(ERRP,"NOMODINCOM","Cannot define a module within a command") ;
	   if (parse->module_level_id != 0)
/*	      Back up to last module level & continue from there */
	    { while (!parse->level->is_module) prs_end_level(NULLV) ;
	      prsu_error(WARNP,"NESTMOD","Cannot define a module within another module") ;
	    } ;
	   new_level->is_module = TRUE ; parse->module_level_id = new_level->level_id ;
/*	   See if this module name has already been defined/referenced */
	   if ((sp = (struct db__prs_sym *)prsu_sym_lookup(name,0)) != NULLV)
	    { if (sp->type != SYMBOL_MODULE)
	       prsu_error(WARNP,"SYMDCLNOTMOD","This symbol has already been declared as something other than MODULE") ;
	    } else
	    { sp = (struct db__prs_sym *)prsu_sym_add(name,(parse->level->prior_level->interpretive ? PRS_GLOBAL_SYM : PRS_LOCAL_SYM)) ;
	      sp->type = SYMBOL_MODULE ;
	    } ;
/*	   Set up module entry at next free slot in code space */
	   mex = prsu_alloc_code(sizeof *me,ALIGNofPTR) ;
	   me = (struct db__module_entry *)&parse->code[mex] ;
/*	   Update module symbol entry only if not previously defined */
	   if (sp->mempos.all == NULLV) sp->mempos.all = mex ;
/*	   Remember where module entry is */
	   new_level->module_entry_offset = mex ;
	   me->code_offset = PRSLEN(&parse->code[parse->code_offset],me) ;		/* Where actual module code begins */
	   me->package_id = parse->package_id ;
	   if (parse->generate_pic) parse->pic_base = parse->code_offset ;
	   if (parse->last_mep != NULLV)			/* Maybe link up to prior if multiple modules in package */
	    parse->last_mep->next_module_offset = PRSLEN(me,parse->last_mep) ;
	   strcpy(me->name,name) ; strcpy(parse->module_name,name) ; 	/* Copy name */
/*	   Take care of module_id stuff */
	   me->module_id = (sp->module_id = (++parse->last_module_id)) ;
/*	   Allocate for level list & command list */
	   new_level->top_of_level = parse->code_offset ;	/* Reset top_of_level past mep block */
	   new_level->module_level_list_ptr = (struct db__level_ids *)v4mm_AllocChunk(sizeof *new_level->module_level_list_ptr,TRUE) ;
	   new_level->module_command_list_ptr = (struct db__command_list *)v4mm_AllocChunk(sizeof *new_level->module_command_list_ptr,TRUE) ;
/*	   Initialize module link list */
	   parse->mod_ll.count = 1 ; strcpy(parse->mod_ll.entry[0].name,me->name) ;
	   parse->is_module_bind = FALSE ;		/* Reset V4 module binding flag */
/*	   Reset parse checksum stuff */
	   parse->current_checksum = 0 ; parse->token_count = 0 ;
/*	   If input is not SOS file then zap current line counter */
	   if (parse->ilvl[parse->ilx].is_not_sos_file) parse->ilvl[parse->ilx].current_line = 0 ;
	 } ;
/*	Now take care of any/all dummy arguments */
	for (so=0;;so+=2)
	 { prsu_nxt_token(FALSE) ;
/*	   If hit ")" then end of arg list */
	   if (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_rparen) break ;
/*	   Not a ")" - best be a symbol */
	   if (tkn.type != TKN_SYMBOL)
	    prsu_error(ERRP,"INVDUMMY","Expecting a dummy argument name") ;
/*	   Got a symbol - add to local table */
	   sp = (struct db__prs_sym *)prsu_sym_add(tkn.symbol,PRS_LOCAL_SYM) ;
	   sp->type = (command_flag ? SYMBOL_DUMMY_CMD : SYMBOL_DUMMY) ;
	   sp->mempos.all = prsu_valref(VAL_ARG,so) ;
/*	   Set symbol table flags: Symbol has been initialized & referenced */
	   sp->initialized = TRUE ; sp->referenced = TRUE ;
/*	   Do we have another argument ? */
	   tkn.sym_ptr = NULLV ; prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rparen) break ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_comma)
	    prsu_error(ERRP,"INVDUMMYTERM","Expecting a comma (\',\') or right-paren (\')\')") ;
	 } ;
/*	Check for [v4 binding] information */
	if (strlen(parse->dflt_module_bind) > 0 && new_level->is_module)
	 { parse->is_module_bind = TRUE ;
	   for(i=0,len=0;;i++)
	    { switch (parse->dflt_module_bind[i])
	       { case 0: 	parse->module_bind[len] = 0 ; goto out_loop ;
		 case '*':	parse->module_bind[len] = 0 ; strcat(parse->module_bind,me->name) ;
				len = strlen(parse->module_bind) ; break ;
		 default:	parse->module_bind[len++] = parse->dflt_module_bind[i] ; break ;
	       } ;
	    } ;
	 } else
	 { prsu_nxt_token(FALSE) ;
	   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lbracket)
	    { prsu_nxt_token(TRUE) ; }
	    else { prsu_nxt_token(FALSE) ;			/* Look for string or closing bracket */
		   if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
		    prsu_error(ERRP,"NOTSTRLIT","Expecting V4 command string enclosed in brackets") ;
		   tkn.value.char_lit[tkn.literal_len] = NULLV ; strcpy(parse->module_bind,tkn.value.char_lit) ;
		   prsu_nxt_token(FALSE) ;
	           if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rbracket)
		    prsu_error(ERRP,"NOTRBKT","Expecting V4 command string enclosed in brackets") ;
		   if (!new_level->is_module) prsu_error(ERRP,"NOTINMOD","Can only specify command after module") ;
	           parse->is_module_bind = TRUE ;
		 } ;
	 } ;
out_loop:
	if (parse->is_module_bind && new_level->is_module)
	 { strcat(parse->module_bind,"\r") ;			/* Append newline */
	   v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(parse->module_bind),V4LEX_InpMode_String) ;
	   v4dpi_PointParse(parse->ctx,&isctpt,&tcb,0) ; v4dpi_PointParse(parse->ctx,&valpt,&tcb,0) ;
	   v4eval_ChkEOL(&tcb) ;			/* Verify EOL */
	   v4ctx_FrameAddDim(parse->ctx,V4C_FrameId_Real,&valpt,0,0) ;	/* Add point to current context */
	   if (!v4dpi_BindListMake(&bindpt,&valpt,&isctpt,parse->ctx,&parse->aid,NOWGTADJUST,0,DFLTRELH))
	    prsu_error(ERRP,"INVBIND",UCretASC(parse->ctx->ErrorMsgAux)) ;
	   parse->V4ModId = valpt.Value.IntVal ;
	   if (strlen(parse->dflt_module_v4eval) > 0)
	    {
	      for(i=0,len=0;;i++)
	       { switch (parse->dflt_module_v4eval[i])
	          { case 0: 	tbuf[len] = 0 ; goto eval_it ;
		    case '*':	tbuf[len] = 0 ; strcat(tbuf,me->name) ; len = strlen(tbuf) ; break ;
		    default:	tbuf[len++] = parse->dflt_module_v4eval[i] ; break ;
	          } ;
	       } ;
eval_it:      strcat(tbuf,"\r") ; v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(tbuf),V4LEX_InpMode_String) ;
	      v4eval_Eval(&tcb,parse->ctx,FALSE,FALSE,FALSE,FALSE,FALSE,NULL) ;	/* Evaluate the argument */
	    } ;
	 } ;
	return ;
 }

/*	prs_end_level - Ends current parser level	*/

void prs_end_level(name)
  char *name ;		/* Optional (may be NULLV) level name */
{ struct db__prs_level *cur_level = parse->level ;
   char tmp[200] ;		/* Temp string */
   int i,j ; int maxval,cnt ;
   struct db__branch_info *binfo ;
   struct db__prs_level *module_level ; /* Points to last module level */
   struct db__level_ids *level_list ;	/* Points to level info list for module */
   union db__module_code_element element ; /* Code element */
   struct db__command_list *commands ;	/* Points to command info list for module */
   struct db__module_entry *mep ;	/* Points to module entry block for module */

/*	If already at top level then can't go no higher */
	if (cur_level->prior_level == NULLV)
	 { prsu_error(WARNP,"TOOMANYRBRACES","Already at top parse level") ; return ; } ;
/*	If name given then check it out */
	if (name == NULLV) goto end_name_check ;
	if (strcmp(cur_level->name,name) != 0)
	 { sprintf(tmp,"Current level is %s, not %s",cur_level->name,name) ;
	   prsu_error(WARNP,"NOTLEVELNAME",tmp) ;
	 } ;
end_name_check:
/*	If leaving a LOOP/LOOP1 then append a JUMP back to increment/test */
	if (cur_level->in_loop)
	 { PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ;
	   parse->code[parse->code_offset++] = (cur_level->loop_inc_offset > 0 ? cur_level->loop_inc_offset - parse->pic_base : 0) ;
	   parse->code_offset+=2 ;
	 } ;
/*	Here to search back to last module level to update level list */
	module_level = cur_level ;
	while (!module_level->is_module)
	 { module_level = module_level->prior_level ;
	   if (module_level == NULLV)
	    prsu_error(INTERNAL+POINT,"NOTMODLVL","Could not module level in prs_end_level") ;
	   if (module_level->interpretive) goto end_module_level ;
	 } ;
/*	Found module level - now update its level list */
	level_list = module_level->module_level_list_ptr ;
	if (level_list->count > V3_MODULE_LEVEL_ID_MAX)
	 prsu_error(ERRP,"TOOMANYLEVELS","Exceeded maximum number of levels for this module") ;
	level_list->info[level_list->count].level_id = cur_level->level_id ;
	level_list->info[level_list->count].begin_code_offset = cur_level->top_of_level - parse->pic_base ;
	level_list->info[level_list->count++].end_code_offset = parse->code_offset - parse->pic_base ;
end_module_level:
/*	If end_level_list is nonzero then do links to end of level */
/*	(ex: internal jumps to end of a BRANCH)			   */
	j = (cur_level->end_level_list > 0 ? cur_level->end_level_list - parse->pic_base : 0) ; cnt = 500 ;
	while ((i = j) && (cnt-- > 0))
	 { j = parse->code[i+parse->pic_base] ; parse->code[i+parse->pic_base] = parse->code_offset - parse->pic_base ;
/*	   Update loc after jump index with prior level id (that is where we are jumping to !) */
	   parse->code[i+1+parse->pic_base] = cur_level->prior_level->level_id ;
	 } ; if (cnt <= 0) prsu_error(ABORT,"INTERR","Internal compiler error detected") ;
/*	If leaving a BRANCH then do some final cleanup */
	if (cur_level->in_branch)
	 { binfo = cur_level->branch_info_ptr ;
/*	   See if we got the "PATH(value)" form */
	   if (binfo->value_max > 0)
	    { binfo->min_value = 0x7FFFFFFF ; maxval = 0x80000000 ;
	      for(i=0;i<binfo->value_max;i++)
	       { if (binfo->values[i] < binfo->min_value) binfo->min_value = binfo->values[i] ;
		 if (binfo->values[i] > maxval) maxval = binfo->values[i] ;
	       } ;
/*	      Got the range, see if ok */
	      i = maxval - binfo->min_value + 1 ;
	      if (i > V3_BRANCH_PATH_MAX)
	       prsu_error(ERRP,"PATHRANGE","Range of \'PATH(value)\' statments in this BRANCH exceeds max allowed") ;
	      binfo->count = -i ;
/*	      Now insert offsets relative to minimum value which goes in slot 0 */
	      for(i=0;i<binfo->value_max;i++)
	       { if (binfo->path_offsets[binfo->values[i]-binfo->min_value] != 0)
		  prsu_error(WARNP,"DUPPATHVAL","Duplicate \'PATH(value)\' value detected at end of this BRANCH") ;
		 binfo->path_offsets[binfo->values[i]-binfo->min_value] = binfo->value_offsets[i] ;
	       } ;
	      i = PRSLEN(&binfo->path_offsets[1-binfo->count],binfo) ;
	    } else i = PRSLEN(&binfo->path_offsets[binfo->count+1],binfo) ;
/*	   Allocate from pure space for the branch info */
	   if (cur_level->branch_info_offset <= 0) prsu_error(ERRP,"INVBRANCH","Invalid BRANCH syntax") ;
	   j = prsu_alloc_pure(i,ALIGN_WORD) ; parse->code[cur_level->branch_info_offset] = j ;
	   if (j > V3_CODEVAL_MAX) prsu_error(ERRP,"CODEVALMAXPURE","Pure space reference offset too large for code entry") ;
	   memcpy(&parse->pure_buf[j],binfo,i) ;
/*	   All done with branch info - free it up */
	   v4mm_FreeChunk(binfo) ;
	 } ;
/*	Go link up any linked-lists for begin/end of level */
	/* xxx */
/*	If this is the end of a module level then go get level id & command info */
	if (cur_level->is_module)
	 {
/*	   Scan thru symbol table for undefined symbols in this level */
	   for (i=parse->labels.count-1;i>=0;i--)
	    { if (parse->labels.sym[i].module_id != parse->last_module_id) continue ;
	      if (parse->labels.sym[i].type != SYMBOL_UNDEF_LABEL) continue ;
	      sprintf(tmp,"Undefined label (%s) in module %s",parse->labels.sym[i].name,cur_level->name) ;
	      prsu_error(WARN,"UNDEFLABELS",tmp) ;
	    } ;
/*	   Scan through local symbol table for any unreferenced symbols */
	   for (i=parse->locals.count-1;i>=0 && parse->locals.sym[i].module_id == parse->last_module_id;i--)
	    { if (parse->locals.sym[i].referenced) continue ;
	      sprintf(tmp,"Unreferenced local symbol (%s) declared within module %s",parse->locals.sym[i].name,cur_level->name) ;
	      prsu_error(WARN,"UNREFSYM",tmp) ;
	    } ;
	   level_list = cur_level->module_level_list_ptr ; commands = cur_level->module_command_list_ptr ;
/*	   First go thru each command & copy some info from level list */
	   for (i=0;i<commands->count;i++)
	    { for (j=0;j<level_list->count;j++)
/*		 Want to match the containing level (level-1) for this command */
	       { if (commands->com[i].level_id == level_list->info[j].level_id) goto got_id_match ;
	       } ; prsu_error(INTERNAL+POINT,"INVLIST","Could not match command level to level list in prs_end_level") ;
got_id_match:
	      commands->com[i].begin_code_offset = level_list->info[j].begin_code_offset ;
	      commands->com[i].end_code_offset = level_list->info[j].end_code_offset ;
	    } ;
	   mep = (struct db__module_entry *)&parse->code[cur_level->module_entry_offset] ;
/*	   Append level_list after code */
	   if (level_list->count == 0) { mep->level_id_offset = 0 ; }
	    else { j = PRSLEN(&level_list->info[level_list->count+1],level_list) ;
		   i = prsu_alloc_code(j,ALIGN_WORD) ; memcpy(&parse->code[i],level_list,j) ;
		   mep->level_id_offset = PRSLEN(&parse->code[i],mep) ;
		 } ;
/*	   Repeat above for command info */
	   if (commands->count == 0) { mep->command_offset = 0 ; }
	    else { j = PRSLEN(&commands->com[commands->count+1],commands) ;
		   i = prsu_alloc_code(j,ALIGN_WORD) ; memcpy(&parse->code[i],commands,j) ;
		   mep->command_offset = PRSLEN(&parse->code[i],mep) ;
		 } ;
/*	   Repeat above for module link list */
	   if (parse->mod_ll.count == 0) { mep->module_link_offset = 0 ; }
	    else { j = PRSLEN(&parse->mod_ll.entry[parse->mod_ll.count],&parse->mod_ll) ;
		   i = prsu_alloc_code(j,ALIGN_WORD) ; memcpy(&parse->code[i],&parse->mod_ll,j) ;
		   mep->module_link_offset = PRSLEN(&parse->code[i],mep) ;
		 } ;
/*	   Now copy whatever pure space we have used */
	   j = PRSLEN(&parse->pure_buf[parse->pure_free],&parse->pure_buf) ;
	   i = prsu_alloc_code(j,ALIGN_DOUBLE) ; memcpy(&parse->code[i],&parse->pure_buf,j) ;
	   mep->pure_offset = PRSLEN(&parse->code[i],mep) ;
/*	   Record number of module-local impure bytes we need */
	   mep->impure_words = parse->impurelm_free ; parse->impurelm_free = 0 ;
/* printf("Module %s, pure=%d, next code offset=%d\n",cur_level->name,j,parse->code_offset) ; */
/*	   Now reset pure space for the next go-around */
	   parse->pure_free = 0 ; for(i=0;i<V3_PRS_LITERAL_MAX;i++) { parse->lit.offsets[i] = 0 ; } ;
/*	   And finally link to set up so we can link to next module */
	   mep->next_module_offset = 0 ;		/* (none yet!) */
	   parse->last_mep = mep ;
/*	   Add this module to checksum list */
	   if (parse->enable_checksums) prsu_checksum_table_insert('M',parse->module_name) ;
/*	   Finally update module entry with max temp stack required */
	   if (parse->generate_pic)
	    { mep->stack_bytes = (parse->module_stack_bytes == 0 ? -1 : -parse->module_stack_bytes) ; }
	    else mep->stack_bytes = parse->module_stack_bytes ;
/*	   Maybe handle V4 interface */
	   if (parse->is_module_bind)
	    {
/*	      Set up key for this V3 module */
	      mep->kp.fld.KeyType = V4IS_KeyType_V4 ; mep->kp.fld.KeyMode = V4IS_KeyMode_Int ;
	      mep->kp.fld.Bytes = V4IS_IntKey_Bytes ; mep->kp.fld.AuxVal = V4IS_SubType_V3PicMod ; mep->Key = parse->V4ModId ;
/*	      Now write it all out */
	      mep->Bytes = PRSLEN(&parse->code[parse->code_offset],mep) ;	/* Get length of module */
	      v4is_Insert(parse->aid,(struct V4IS__Key *)mep,mep,mep->Bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	    } ;
	   parse->module_stack_bytes = 0 ; parse->module_level_id = 0 ;
if (parse->generate_pic && FALSE)
 { for(i=parse->pic_base;i<=parse->code_offset;i+=10)
    {
      printf("%d:	",i-parse->pic_base) ;
      for(j=0;j<10;j++)
       { element.all = parse->code[i+j] ;
		switch (element.fld.type)
		  {case CODE_OPERATION: printf("O:%d",element.fld.offset) ; break ;
		   case CODE_FREQOP: printf("F:%d",element.fld.offset) ; break ;
		   case CODE_PUREIV: printf("Pi:%d",element.fld.offset) ; break ;
		   case CODE_PUREPV: printf("Pp:%d",element.fld.offset) ; break ;
		   case CODE_PURE: printf("P:%d",element.fld.offset) ; break ;
		   case CODE_IMPUREIV: printf("Ii:%d",element.fld.offset) ; break ;
		   case CODE_IMPUREPV: printf("Ip:%d",element.fld.offset) ; break ;
		   case CODE_IMPURE: printf("I:%d",element.fld.offset) ; break ;
		   case	CODE_INT_LITERAL: printf("L:%d",element.fld.offset) ; break ;
		  } ; printf("/%d  ",element.all) ;
       } ; printf("\n") ;
    } ;
 } ;
	 } ;
/*	Restore opr temp stack */
	if (! parse->level->iterative) parse->tmp_index = cur_level->prior_tmp_index ;
/*	Link up to prior level */
	parse->level = cur_level->prior_level ;
	if (cur_level->is_module)
	 { prsu_nxt_token(FALSE) ;
	   if (! (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_semi))
	    prsu_nxt_token(TRUE) ;			/* If ";" after closing right brace then ignore! */
	 } ;
/*	And finally free old level block */
	v4mm_FreeChunk(cur_level) ;

	return ;
}

/*	prsu_find_level - Searches for a "named" level	*/
/*	 (Used after TOP/FALL_THRU/OUT ...)		*/
/*	Returns: *db__prs_level or NULLV if no level explicitly named */
struct db__prs_level *prsu_find_level()
{ struct db__prs_level *flevel ;

/*	See if next token is "("  (ex: TOP(xxx) )	*/
	prsu_nxt_token(FALSE) ;
	if (! (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen))
	 { prsu_nxt_token(TRUE) ; return(NULLV) ; } ;
	prsu_nxt_token(FALSE) ;
	if (tkn.type != TKN_SYMBOL)
	 prsu_error(ERRP,"INVLEVELNAME","Argument MUST be name of previously declared level") ;
	prsu_nxt_token(FALSE) ;
	if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen)
	 prsu_error(ERRP,"INVNAMETERM","Expecting a right paren (\')\') here") ;
/*	Scan thru current/prior levels for name */
	flevel = parse->level ;
	do
	 { if (strcmp(flevel->name,tkn.symbol) == 0) goto found_level ;
	 } while ((flevel = flevel->prior_level) != NULLV) ;
	prsu_error(ERRP,"NOLEVELNAME","No such named level currently exists") ;
found_level:
	return(flevel) ;
}

/*	V 3   P A R S E R				*/

static struct prs__symbol_ref_list *sref ;	/* Points to current symbol reference */
static int last_search_id ;			/* Updated with last symbol search id (for use with DCL PARTIAL) */
static int last_sref_type,last_sref_skel_index,last_sref_last_el ;

void prsu_parser()
{
   int i,j,isdef ; int *tptr ; int hash ; int flag_value,flag_tmp ; char v4isct[250] ;
   int entry_ilx ; int inum,cnt ;
   int save_code_offset ;
   char sym_name[V3_PRS_SYMBOL_MAX+1] ;
   union db__module_code_element mce ;
   struct db__opr_info *cur,*top ;
   struct db__prs_sym *user_sym_ptr ;
   struct db__prs_sym *sp ;		/* Points to symbol table entry */
   int box_code_offset,int_code_start,error_count=0 ;
   struct db__prs_level *tlevel ;	/* Points to temp/work parser level */
   struct db__branch_info *binfo ;
   struct db__command_list *clp ;	/* Pointer to module command list */
   struct db__formatmempos modo ;
   struct db__formatmempos *se ;
   struct db__formatptr mod ;
   struct db__do_loop_info *do_loop ;	/* Pointer to do-loop info */
   struct db__module_entry *mep ;	/* Pointer to module entry block */
   struct db__skeleton_ref skelref ;
   unsigned short *code_ptr ;

/*	Set up environment */
	parse = process->package_ptrs[psi->package_index]->parse_ptr ;
	entry_ilx = parse->ilx ;	/* Save ilx for possible errors or returns */
/*	If first call then set up a level */
	if (parse->level == NULLV) prs_begin_level("top_level",TRUE,FALSE,FALSE) ;

/*	Here for start of (next) statement */
next_statement:
/*	Save start of interpretive code */
	if (parse->level->interpretive) int_code_start = parse->code_offset ;
	box_code_offset = parse->code_offset ; /* Save in case of error recovery */
	parse->ilvl[parse->ilx].statement_start_line = 0 ;
/*	Reset "have_value" flag for unary minus check */
	parse->have_value = FALSE ;
/*	Reset value stack for this statement */
	parse->level->valx = 0 ;
/*	Set up for possible parser error */
	switch (setjmp(parse_exit))
	 { case 0: break ;		/* First time thru */
	   case 1:
/*		Got a parser error - skip until ";" or "DCL" */
		for (;;)
		 { prsu_nxt_token(FALSE) ;
		   if (parse->dcl_nest == 0 && tkn.sym_ptr == (struct db__opr_info *)&notbl_semi) break ;
/*		   If in nested DCL then wait until another DCL or get proper number of "}s" */
		   if (strcmp(tkn.symbol,"DCL") == 0)
		    { parse->dcl_nest = 0 ; prsu_nxt_token(TRUE) ; break ; } ;
		   if (parse->dcl_nest > 0)
		    { if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rbrace) { parse->dcl_nest-- ; continue ; } ;
		      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lbrace) { parse->dcl_nest++ ; continue ; } ;
		    } ;
		 } ;
		parse->code_offset = box_code_offset ; /* Wipe off any generated code */
		error_count++ ; goto next_statement ;
	   case 2:
		printf("%V3-PRS-FATAL-Fatal parsing error detected, aborting\n") ;
		exit(EXIT_FAILURE) ;
	   case 3:
		if (process->package_ptrs[psi->package_index] == NULLV) return ;
		parse = process->package_ptrs[psi->package_index]->parse_ptr ;
		for (i=parse->ilx;i>=entry_ilx;i--)
		 { if (parse->ilvl[i].file != NULLV) fclose(parse->ilvl[i].file) ;
		 } ;
		parse->ilx = 0 ;
		tlevel = parse->level ;
		while ((tptr = (int *)tlevel) != NULLV)
		 { tlevel = tlevel->prior_level ; v4mm_FreeChunk(tptr) ; } ;
/*		Return to wherever */
		return ;
	 } ;

/*	Init the parse temp stack */
	parse->tmp_index = parse->level->prior_tmp_index ;
	PUSHTMP(&notbl_box) ;
/*	Loop thru all tokens */
	for (;;)
	 {
next_token: prsu_nxt_token(FALSE) ;
/*	See if within conditional compilation */
	if (parse->ilvl[parse->ilx].condcomp_ignore > 0)
	 { for(;;)
	    { prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_cross_lbrace) { parse->ilvl[parse->ilx].condcomp_ignore++ ; continue ; } ;
	      if (tkn.sym_ptr == (struct db__opr_info *)&notbl_rbrace_cross)
	       { if (--(parse->ilvl[parse->ilx].condcomp_ignore) <= 0) break ; } ;
	    } ;
	   prsu_nxt_token(FALSE) ;
	 } ;
/*	   What did we get ? */
	   switch (tkn.type)
	    { case TKN_LITERAL:
handle_parse_literal:
/*		Have a literal - push pointer to it onto code */
		if (parse->have_value)
		 prsu_error(WARNP,"INVSYNTAXCOMMA","Invalid V3 syntax (possible missing comma, semicolon, or operator)") ;
/*		If integer literal is small enough then store as CODE_INT_LITERAL */
		if (tkn.lit_format.fld.type == VFT_BININT && tkn.lit_format.fld.decimals == 0 &&
		    tkn.value.int_lit <= 0xFFF && tkn.value.int_lit >= 0)
		 { PUSHCODE(CODE_INT_LITERAL,tkn.value.int_lit) ; }
		 else {
			PUSHCODE((tkn.lit_format.fld.mode == VFM_PTR ? CODE_PUREPV : CODE_PUREIV),prsu_stash_lit(FALSE)) ;
		      } ;
		parse->have_value = TRUE ; prsu_chk_value(NULLV) ; continue ;

	      case TKN_SYMBOL:
/*		Have a user symbol - Find it in symbol table */
		if ((user_sym_ptr = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,0)) == NULLV)
/*		   Last chance - see if symbol is V3 function */
		 { if((cur = (struct db__opr_info *)prsu_predefined_lookup(tkn.symbol,V3_FUNCTIONS,PRS_FUNCTION_COUNT)) != NULLV)
		    break ;	/* Treat as operator */
#ifdef windows
		   if((cur = (struct db__opr_info *)prsu_predefined_lookup(tkn.symbol,V3_WINDOW_MODULES,PRS_WINDOW_COUNT)) != NULLV)
		    break ;	/* Window modules - treat as operator */
#endif
#ifdef v3v4
		   if((cur = (struct db__opr_info *)prsu_predefined_lookup(tkn.symbol,V3_V4_MODULES,PRS_V3V4_COUNT)) != NULLV)
		    break ;	/* V4 modules - treat as operator */
#endif
/*		   Symbol is undefined - see if next token is a ":" for possible label */
		   prsu_nxt_token(FALSE) ;
		   if (!(tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_colon))
/*		      Last chance - see if module reference */
		    { if (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen)
		       { prsu_nxt_token(TRUE) ;
/*			 Yes - then define it as a module */
			 user_sym_ptr = (struct db__prs_sym *)prsu_sym_add(tkn.symbol,PRS_EXTERNAL_SYM) ;
			 user_sym_ptr->type = SYMBOL_MODULE ;
			 for(i=parse->mod_ll.count-1;i>=0;i--)
			  { if (strcmp(tkn.symbol,parse->mod_ll.entry[i].name) == 0) break ; } ;
			 if (i < 0)
			  { if (parse->mod_ll.count >= V3_PRS_MODULE_LIST_MAX)
			     prsu_error(ERRP,"MAXMODREF","Exceeded max number of module references within a module") ;
			    strcpy(parse->mod_ll.entry[i=(parse->mod_ll.count++)].name,tkn.symbol) ;
			  } ;
			 user_sym_ptr->mempos.all = i ;		/* Remember which module this is */
			 goto sym_switch ;
		       } ;
		      prsu_nxt_token(TRUE) ;
		    } else
		    { if (parse->code_offset != box_code_offset)
		       prsu_error(ERRP,"LABELPOS","Labels may be declared only at the beginning of a statement") ;
		      if (parse->level->interpretive)
		       prsu_error(ERRP,"LABELINTERP","A label cannot be declared when in interpretive mode") ;
/*		      Define the label */
		      user_sym_ptr = (struct db__prs_sym *)prsu_sym_add(tkn.symbol,PRS_LABEL_SYM) ;
		      user_sym_ptr->type = SYMBOL_LABEL ;
		      user_sym_ptr->mempos.all = parse->code_offset ;
		      parse->have_value = FALSE ; goto next_token ;
		    } ;
/*		   Symbol is undefined - define it to reduce number of error messages */
		   user_sym_ptr = (struct db__prs_sym *)prsu_sym_add(tkn.symbol,PRS_LOCAL_SYM) ;
		   user_sym_ptr->type = SYMBOL_NORMAL ; user_sym_ptr->referenced = TRUE ;
		   prsu_error(ERRP,"UNDEFSYM","Undefined user symbol") ;
		 } ;
sym_switch:
		if (parse->have_value)
		 prsu_error(WARNP,"INVSYNTAXSEMI","Invalid V3 syntax (possible missing semicolon, comma or operator)") ;
		parse->have_value = TRUE ;
		switch (user_sym_ptr->type)
		 { case SYMBOL_NORMAL:
		   case SYMBOL_STRUCTREF:
		   case SYMBOL_STRUCTPTR:
		   case SYMBOL_SKELETONREF:
		   case SYMBOL_OBJREF:
			prsu_parser_symbol(user_sym_ptr,TRUE) ;
			continue ;
		   case SYMBOL_DUMMY:
			PUSHCODE(CODE_FREQOP,V3_XCT_DUMMYREF) ;
			PUSHVAL(user_sym_ptr->mempos.arg.offset) ;
			prsu_chk_oper(NULLV,NULLV,RES_UNK,0) ; continue ;
		   case SYMBOL_DUMMY_CMD:
			PUSHCODE(CODE_OPERATION,V3_XCT_DUMMYREF_CMD) ;
			PUSHVAL(user_sym_ptr->mempos.arg.offset) ;
			prsu_chk_oper(NULLV,NULLV,RES_UNK,0) ; continue ;
		   case SYMBOL_SEMANTIC:
			/* xxx */
		   case SYMBOL_SKELETON:
		   case SYMBOL_STRUCTURE:
			prsu_error(ERRP,"INVSTRUCTPOS","A STRUCT name can only be used in DCL STRUCT statement") ;
		   case SYMBOL_MODULE:
			if (parse->level->interpretive)			/* If interpretive link to module entry now! */
			 { if (parse->glbl_xtrnal_count != parse->globals.count)
			    { parse->glbl_xtrnal_count = parse->globals.count ;
			      pcku_load_package(process->package_ptrs[parse->package_id]) ;
			    } ;
			   if ((mep = pcku_module_defined(tkn.symbol)) == NULLV)
			    prsu_error(ERRP,"MODSYMNOTDEF","Module not currently defined") ;
			   mod.format.fld.type = VFT_MODREF ; mod.format.fld.mode = VFM_MEP ;
			   mod.ptr = (char *)mep ; i = prsu_alloc_pure(sizeof mod,ALIGNofPTR) ;
			   memcpy(&parse->pure_buf[i],&mod,sizeof mod) ;
			   PUSHCODE(CODE_PUREPP,i) ;		/* Push module index */
			 } else
			 { for(i=parse->mod_ll.count-1;i>=0;i--)	/* Make sure this module in current module link list */
			    { if (strcmp(tkn.symbol,parse->mod_ll.entry[i].name) == 0) break ; } ;
			   if (i < 0)
			    { if (parse->mod_ll.count >= V3_PRS_MODULE_LIST_MAX)
			       prsu_error(ERRP,"MAXMODREF","Exceeded max number of module references within a module") ;
			      strcpy(parse->mod_ll.entry[i=(parse->mod_ll.count++)].name,tkn.symbol) ;
			    } ;
			   modo.format.fld.type = VFT_MODREF ; modo.format.fld.mode = VFM_OFFSET ;
			   modo.mempos.all = i ; i = prsu_stash_pure((char *)&modo,sizeof modo) ;
			   PUSHCODE(CODE_PUREIV,i) ;		/* Push module index */
			 } ;
			cur = (struct db__opr_info *)&notbl_module_call ;
/*			Make sure module reference followed by "(" */
			prsu_nxt_token(FALSE) ; prsu_nxt_token(TRUE) ;
			if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lparen)
			 prsu_error(ERRP,"INVMODREF","A module reference MUST be followed by \'(\'") ;
			goto have_op1 ;	/* Want to drop out to handle as operator */
		   case SYMBOL_LABEL:
			prsu_nxt_token(FALSE) ;
			if (tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_colon)
			 prsu_error(WARNP,"MULTDEFLABEL","This label has already been defined within this module") ;
			prsu_nxt_token(TRUE) ;	/* Allow lookahead */
			prsu_error(WARNP,"LABELUSAGE","Statment labels only allowed after GOTOs") ;
		   case SYMBOL_UNDEF_LABEL:
			prsu_nxt_token(FALSE) ;
			if (!(tkn.type == TKN_PREDEFINED && tkn.sym_ptr == (struct db__opr_info *)&notbl_colon))
			 prsu_error(WARNP,"LABELUSAGE","Statment labels only allowed after GOTOs") ;
			if (parse->code_offset != box_code_offset)
			 prsu_error(ERRP,"LABELPOS","Labels may be declared only at the beginning of a statement") ;
			if (parse->level->level_id > user_sym_ptr->level_id)
			 prsu_error(WARNP,"LABELLVLERR","Goto\'s cannot jump into level") ;
/*			Labels appears to be defined - fill in linked list */
			i = (user_sym_ptr->mempos.all > 0 ? user_sym_ptr->mempos.all-parse->pic_base : 0) ;
ul1:
			cnt = 500 ;
			while ((j=i) && (cnt-- >= 0))
			 { i = parse->code[j+parse->pic_base] ;
			   parse->code[j+parse->pic_base] = parse->code_offset - parse->pic_base ;
/*			   Save current level id in goto jump offset +1 */
			   parse->code[j+1+parse->pic_base] = parse->level->level_id ;
			 } ;if (cnt <= 0) prsu_error(ABORT,"INTERR","Internal compiler error") ;
/*			Now make this a defined label */
			user_sym_ptr->type = SYMBOL_LABEL ; user_sym_ptr->level_id = parse->level->level_id ;
			user_sym_ptr->mempos.all = parse->code_offset ;
			parse->have_value = FALSE ; goto next_token ;
		   default: prsu_error(INTERNAL+POINT,"INVSYMTYP","Invalid symbol type detected in prsu_parser") ;
		  } ;
	      case TKN_PREDEFINED:
/*		Have a predefined symbol - */
		cur = tkn.sym_ptr ; break ;

	      case TKN_ENDINPUT:
		return ;
	    } ;

/*	P A R S E  -   H A N D L E   O P E R A T O R S 	*/
/*	   If here then have to handle an operator, module, whatever */
have_op1:
	   i = cur->on_push_do ;
	   switch (cur->on_push_do)
	    { case PUSH_COLON:
		prsu_error(WARNP,"INVCOLONUSAGE","A colon may only be used to declare statement labels") ;
	      case PUSH_BRANCH:
		prs_begin_level(NULLV,FALSE,TRUE,FALSE) ;
		parse->level->in_branch = TRUE ; parse->level->branch_top_offset = parse->code_offset ;
		parse->level->branch_info_ptr = (struct db__branch_info *)v4mm_AllocChunk(sizeof *parse->level->branch_info_ptr,TRUE) ;
		break ;
	      case PUSH_DCL:
		if (parse->code_offset != box_code_offset)
		 prsu_error(WARNP,"INVDCLPOS","DCL only allowed at beginning of statement") ;
		if (prsu_dcl(FALSE)) goto next_statement ;
/*		Returned false - should continue parsing (ex: DCL PARTIAL) */
		goto next_token ;
	      case PUSH_LOOP:
		prsu_nxt_token(FALSE) ; prsu_nxt_token(TRUE) ;
/*		Loop must be followed by "(" or "{" */
		if ((tkn.sym_ptr != (struct db__opr_info *)&notbl_lparen) && (tkn.sym_ptr != (struct db__opr_info *)&notbl_lbrace) && (tkn.sym_ptr != (struct db__opr_info *)&notbl_lbrace_slash))
		 prsu_error(ERRP,"INVLOOP","Expecting LOOP to be followed by \'(\' or \'{\'") ;
		break ;
	      case PUSH_NULL: break ;
	      case PUSH_IF:
		prs_begin_level(NULLV,FALSE,TRUE,FALSE) ;
		parse->level->in_if = TRUE ;
		break ;
	      case PUSH_THEN:
		if (! parse->level->in_if)
		 prsu_error(ERRP,"MISSINGIF","Cannot have THEN without an IF") ;
		if (parse->level->have_then)
		 prsu_error(WARNP,"TOOMANYTHENS","Can only have one THEN per IF statement") ;
		parse->level->have_then = TRUE ;
		break ;
	      case PUSH_ELSE:
		if (! parse->level->in_if)
		 prsu_error(ERRP,"MISSINGIF","Cannot have ELSE/ELSE IF without an IF") ;
		if (! parse->level->have_then)
		 prsu_error(WARNP,"MISSINGTHEN","Cannot have ELSE/ELSE IF without prior THEN in IF statement") ;
		if (parse->level->have_else)
		 prsu_error(WARNP,"TOOMANYELSES","Can only have one ELSE per IF statement") ;
		parse->level->have_else = TRUE ;
/*		See if next token is "IF", if so then convert else to "else if" */
		prsu_nxt_token(FALSE) ;
		if (strcmp("IF",tkn.symbol) == 0) { cur = (struct db__opr_info *)&notbl_elseif ; }
		 else prsu_nxt_token(TRUE) ;
		break ;
	      case PUSH_DOT:
/*		Should only get here when coming out of a nested subscript */
/*		  (as in sym[xxx] . xxxxxxx)	*/
		if (parse->tmp[parse->tmp_index] != (struct db__opr_info *)&notbl_symref)
		 prsu_error(ERRP,"INVDOTUSAGE","Invalid use of dot") ;
		prsu_nxt_token(TRUE) ;
/*		Call symbol routine with current symbol (on stack) */
		prsu_parser_symbol(NULLV,TRUE) ;
		goto next_token ;
	      case PUSH_LRBRACKET:
		prsu_error(WARNP,"INVLRBRKUSG","Invalid usage of \'[]\'") ; break ;
	      case PUSH_LBRACKET:
/*		Like the "." - should only get one in certain cases */
		if (parse->tmp[parse->tmp_index] != (struct db__opr_info *)&notbl_symref)
		 { prsu_nxt_token(FALSE) ;	/* Look for [module].xxx  construct */
		   if (tkn.type != TKN_SYMBOL)
		    { if (tkn.sym_ptr == (struct db__opr_info *)&notbl_quote)	/* Look for option @ to indicate quoted */
		       { i = TRUE ; prsu_nxt_token(FALSE) ; } else { i = FALSE ; } ;
		      if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_FIXSTR)
		       prsu_error(ERRP,"INVSUBUSAGE","Invalid use of subscripts") ;
		      tkn.value.char_lit[tkn.literal_len] = NULLV ;
		      strcpy(v4isct,"[") ; strncat(v4isct,tkn.value.char_lit,sizeof(v4isct)-1) ; strcat(v4isct,"]") ;
		      prsu_nxt_token(FALSE) ;
	              if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rbracket)
		       prsu_error(ERRP,"NOTRBKT","Expecting V4 command string enclosed in brackets") ;
		      prsu_V4IsctMacro(v4isct,i) ; goto next_token ;
		    } ;
		   if ((user_sym_ptr = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,0)) == NULLV)
		    prsu_error(ERRP,"UNDEFMODSYM","Undefined symbol in \'[xxx]\' construct") ;
		   if (user_sym_ptr->type != SYMBOL_MODULE)
		    prsu_error(ERRP,"MUSTBEMOD","Symbol within \'[ xxx ]\' must be a module") ;
/*		   See if followed by ":" denoting a program location */
		   prsu_nxt_token(FALSE) ;
		   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_colon) { prsu_nxt_token(TRUE) ; }
		    else
		    { prsu_nxt_token(FALSE) ; se = (struct db__formatmempos *)xctu_pointer(user_sym_ptr->mempos.all) ;
		      mep = (struct db__module_entry *)xctu_pointer(se->mempos.all) ;
/*		      Should have a symbol or line number */
		      if (tkn.type == TKN_SYMBOL)
		       { for (i=parse->labels.count-1;i>=0;i--)
			  { if (mep->module_id != parse->labels.sym[i].module_id) continue ;
			    if (strcmp(tkn.symbol,parse->labels.sym[i].name) != 0) continue ;
/*			    Found the label - save offset into code */
			    j = parse->labels.sym[i].mempos.all ; goto make_code_ref ;
			  } ;
			 prsu_error(ERRP,"LBLNOTINMOD","Symbol not defined as label within module") ;
		       } ;
/*		      If here then should have a line number */
		      if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_BININT)
		       prsu_error(ERRP,"INVCODEPOS","Expecting a label or linenumber here") ;
/*		      If number is 0 then offset is begin of module */
		      if (tkn.value.int_lit == 0)
		       { j = mep->code_offset ; goto make_code_ref ; } ;
/*		      Have to look for line number */
		      code_ptr = &parse->code[j = mep->code_offset] ;
		      mce.fld.type = CODE_FREQOP ; mce.fld.offset = V3_XCT_EOSLN ;
		      for (;*code_ptr!=V3_XCT_STOPXCT;code_ptr+=dbg_inst_length(*code_ptr))
		       { if (*code_ptr != mce.all) continue ;
			 if (tkn.value.int_lit == *(code_ptr+1)) goto make_code_ref ;
			 j = code_ptr - &parse->code[0] ;
		       } ;
		      prsu_error(ERRP,"NOLINENUM","Line number not found in module") ;
/*		      Have code offset in j, make reference */
make_code_ref:	      i = prsu_alloc_pure(sizeof *se,ALIGN_WORD) ; se = (struct db__formatmempos *)&parse->pure_buf[i] ;
		      se->format.fld.type = VFT_MODREF ; se->format.fld.mode = VFM_OFFSET ;
		      se->mempos.all = j ; PUSHCODE(CODE_PUREIV,i) ; prsu_chk_value(NULLV) ;
		      prsu_nxt_token(FALSE) ;
		      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rbracket)
		       prsu_error(ERRP,"NOTRBRACKET","Expecting a \']\' here") ;
		      goto next_token ;
		    } ;
/*		   Now fake a symbol reference and let another module do all the work */
		   prsu_parser_symbol(user_sym_ptr,TRUE) ; goto next_token ;
		 } ;
		prsu_nxt_token(TRUE) ;
		prsu_parser_symbol(NULLV,TRUE) ;
		goto next_token ;
	      case PUSH_DASH:
/*		Check for unary minus */
		if (!parse->have_value) cur = (struct db__opr_info *)&notbl_unaryminus ;
		break ;
	      case PUSH_SLASH:
/*		Have to decide if divide or begin of V3 flag list */
		if (parse->have_value) break ;
/*		If we can't treat as small numeric literal then have to push as regular symbol */
		if ((i = prsu_eval_constant(FALSE,FALSE)) >= 0) { PUSHCODE(CODE_PUREIV,i) ; } ;
		prsu_chk_value(NULLV) ; parse->have_value = TRUE ;
		goto next_token ;
	      case PUSH_RANGLE:
/*		Have to decide if arithmetic compare or "is initialized" flag */
		if (parse->have_value) break ;
		prsu_nxt_token(FALSE) ;
		if (tkn.type != TKN_SYMBOL)
		 prsu_error(ERRP,"INVUPDSYM","Expecting a symbol after the \'is-initialized\' flag") ;
		if ((user_sym_ptr = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,0)) == 0)
		 prsu_error(ERRP,"UNDEFUPDSYM","Symbol to be flagged as initialized is not defined") ;
		user_sym_ptr->initialized = TRUE ; prsu_nxt_token(TRUE) ;
		goto next_token ;
	      case PUSH_STAR:
/*		Have to decide if multiply or V3 object name reference */
		if (parse->have_value) break ;
		prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
		if (tkn.type != TKN_SYMBOL)
		 prsu_error(ERRP,"INVOBJNAMELIT","Expecting an object name here") ;
		if (parse->package_id == 0) prsu_error(ERRP,"INVOBJPACKSLOT","Cannot use *xxx* construct with package 0") ;
		hash = sobu_name_create(tkn.symbol,parse->package_id) ;
		prsu_nxt_token(FALSE) ;
		if (tkn.sym_ptr != (struct db__opr_info *)&notbl_star)
		 prsu_error(ERRP,"INVOBJLITTERM","Expecting object name literal to end with \'*\'") ;
/*		Set up tkn.xxx as numeric literal */
		tkn.value.int_lit = hash ; tkn.lit_format.fld.decimals = 0 ; tkn.lit_format.fld.type = VFT_BININT ;
		tkn.lit_format.fld.mode = VFM_IMMEDIATE ; tkn.literal_len = (tkn.lit_format.fld.length = 4) ;
		goto handle_parse_literal ;
	      case PUSH_ITX_GEN:
/*		Push special instruction to zap global symbol */
		PUSHCODE(CODE_OPERATION,V3_XCT_ZAP_OBJ_SKEL_OBJLR) ; break ;
	      case PUSH_END_CONDCOMP:
		if (parse->ilvl[parse->ilx].condcomp_nest <= 0)
		 { prsu_error(WARNP,"TOOMNY}#","Unexpected \'}#\' detected in source file") ; }
		 else { parse->ilvl[parse->ilx].condcomp_nest-- ; } ;
		goto next_token ;
	      case PUSH_START_CONDCOMP:
/*		Look for syntax of the form: "{# (sym)..." or "{# (NOT sym) ..." */
		prsu_nxt_token(FALSE) ;
		if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lparen)
		 prsu_error(ERRP,"INVCCSYNTAX","Expecting a \'(\' here") ;
		flag_value = TRUE ;	/* Set to FALSE if we find NOT */
		prsu_nxt_token(FALSE) ;
		if (tkn.type == TKN_SYMBOL)
		 { if (strcmp(tkn.symbol,"NOT") == 0) { prsu_nxt_token(FALSE) ; flag_value = FALSE ; } ;
		 } ;
		if (tkn.type == TKN_SYMBOL)
		 { if ((strcmp(tkn.symbol,"SYM") == 0) || (strcmp(tkn.symbol,"SYMBOL") == 0))
		   { prsu_nxt_token(FALSE) ; isdef = TRUE ;
		     if (tkn.type != TKN_SYMBOL)
		      prsu_error(ERRP,"INVSYMNAME","Expecting a symbol name here") ;
		     strcpy(sym_name,tkn.symbol) ;
		     for(i=FALSE;;)
		      { if ((sp = (struct db__prs_sym *)prsu_sym_lookup(sym_name,0)) != NULLV) break ;
		        if (i) { isdef = FALSE ; break ; } ;
		        i = TRUE ;
		        if (!prs_attempt_struct_load(sym_name)) continue ;
/*		        Next token should be start of structure dcl, pop off "dcl" */
		        prsu_nxt_token(FALSE) ;
		        if (strcmp(tkn.symbol,"DCL") != 0) prsu_error(ERRP,"NOTDCL","Expecting macro structure expansion to begin with DCL") ;
		        prsu_dcl(TRUE) ;			/* Parse structure & try again */
		      } ;
/*		     Loop thru all/any elements to get to last element */
		     for (;;)
		      { prsu_nxt_token(FALSE) ;
		        if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot)
			 { if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen)
			    prsu_error(ERRP,"NOTPAREN","Expecting a closing paren (\')\') here") ;
			   break ; ;
			 } ;
			if (!isdef) { prsu_nxt_token(FALSE) ; continue ; } ;
		        if (sp->search_struct_id == 0)
		         prsu_error(ERRP,"NOTSTRREF","Cannot use dot (\'.\') after terminal symbol") ;
/*		        Parse element & do lookup */
		        prsu_nxt_token(FALSE) ;
		        if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"INVSTRUCTEL","Expecting a structure element here") ;
		        if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,sp->search_struct_id)) == NULLV)
		         { isdef = FALSE ; } ;
		      } ;
		     if (isdef) { goto found_cc_sym ; } else { goto notfound_cc_sym ; } ;
		   } ;
		 } ;
		if (tkn.type == TKN_SYMBOL)
		 { if (strcmp(tkn.symbol,"MODULE") == 0)
		   { prsu_nxt_token(FALSE) ;
		     if (tkn.type != TKN_SYMBOL)
		      prsu_error(ERRP,"INVSYMNAME","Expecting a module name here") ;
		      strcpy(sym_name,tkn.symbol) ; prsu_nxt_token(FALSE) ;
		      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen)
		       prsu_error(ERRP,"NOTPAREN","Expecting a closing paren (\')\') here") ;
		     if (pcku_module_defined(sym_name) != NULL) goto found_cc_sym ;
/*		     Not a user symbol - see if V3 predefined */
		     if ((char *)prsu_predefined_lookup(sym_name,V3_FUNCTIONS,PRS_FUNCTION_COUNT) != NULL) goto found_cc_sym ;
		     goto notfound_cc_sym ;
		   } ;
		 } ;
		prsu_nxt_token(FALSE) ;	/* Skip to ending paren */
		if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen)
		 prsu_error(ERRP,"NOTPAREN","Expecting a closing paren (\')\') here") ;
		hash = prsu_str_hash(tkn.symbol,-1) ;
/*		Look for the symbol in value table */
		for(i=0;i<parse->constants.count;i++)
		 { if (hash != parse->constants.sym[i].hash) continue ;
		   if (strcmp(tkn.symbol,parse->constants.sym[i].name) == 0) goto found_cc_sym ;
		 } ;
/*		Make special tests for compiler/system based values */
#ifdef vms
		if (strcmp(tkn.symbol,"VMS") == 0) goto found_cc_sym ;
#else
		if (strcmp(tkn.symbol,"UNIX") == 0) goto found_cc_sym ;
#endif
/*		Did not find symbol - is this good or bad ? */
notfound_cc_sym:
		if (flag_value) { parse->ilvl[parse->ilx].condcomp_ignore ++ ; }
		 else parse->ilvl[parse->ilx].condcomp_nest++ ;
		goto next_token ;
found_cc_sym:
		if (flag_value) { parse->ilvl[parse->ilx].condcomp_nest++ ; }
		 else parse->ilvl[parse->ilx].condcomp_ignore++ ;
		goto next_token ;
	    } ;
have_op2:
/*	   Now check for "execute immediate" */
	   if (cur->xct_immediate)
	    { PUSHCODE(CODE_OPERATION,cur->reference) ; prsu_chk_oper(cur,0,0,0) ; continue ; } ;
/*	   If push_immediate then push onto temp stack */
	   if (cur->push_immediate)
	    { PUSHTMP(cur) ; continue ; } ;
/*	   Set the "have_value" flag			*/
	   parse->have_value = (cur->returns_value) ;
/*	   Compare the precedence of this with top of temp stack */
	   top = parse->tmp[parse->tmp_index] ;
	   while (top->precedence >= cur->precedence)
	    { switch (top->on_pop_do)
	       { case POP_IFISH:
		   if (parse->level->if_code_offset == 0)
		    prsu_error(WARNP,"INVIFSYNTAX","Missing THEN/ELSE on IF statement") ;
/*		   Update code with skip over then/else */
		   parse->code[parse->level->if_code_offset] = parse->code_offset - parse->pic_base ;
		   prs_end_level(NULLV) ; prsu_chk_oper(NULLV,NULLV,RES_UNK,0) ; break ;
		 case POP_LOOP:
		   parse->level->in_loop_setup = FALSE ;
/*		   First test for endless loop */
		   if (parse->level->is_loop_forever)
		    { parse->level->loop_inc_offset = parse->code_offset ; break ; } ;
/*		   See if we have "do - loop" construct: loop(a:=b..c[..d]) */
		   if (parse->level->is_loop_do)
		    { PUSHCODE(CODE_OPERATION,(parse->level->in_loop1 ? V3_XCT_DOLOOP1 : V3_XCT_DOLOOP2)) ;
/*		      Push TRUE/FALSE if we got an explicit increment */
		      PUSHVAL(parse->level->have_loop_do_inc) ;
/*		      Save position for jump at bottom of loop to test/increment */
		      parse->level->loop_inc_offset = parse->code_offset ;
		      PUSHCODE(CODE_OPERATION,V3_XCT_DOLOOP3) ;
/*		      Allocate stack space & save offset */
		      i = prsu_alloc_stack(sizeof *do_loop,ALIGNofPTR) ; PUSHVAL(i) ;
/*		      Want next instruction space to point to end of loop */
		      parse->level->end_level_list = parse->code_offset ; parse->code_offset += 2 ;
		      break ;
		    } ;
/*		   How about a LOOP(n) construct ? */
		   if (parse->level->loop_test_offset == 0)
		    { i = prsu_alloc_stack(4,ALIGN_WORD) ;	/* Allocate for temp word in stack */
		      PUSHCODE(CODE_OPERATION,(parse->level->in_loop1 ? V3_XCT_REPEAT3 : V3_XCT_REPEAT1)) ;
	   	      if (i > V3_CODEVAL_MAX)
		       prsu_error(ERRP,"CODEVALMAXPURS","Stack space reference offset too large for code entry") ;
		      parse->code[parse->code_offset++] = i ;
/*		      Save offset of begin of test code */
		      parse->level->loop_inc_offset = parse->code_offset ;
		      PUSHCODE(CODE_OPERATION,V3_XCT_REPEAT2) ;
		      parse->code[parse->code_offset++] = i ;
/*		      Next loc will be updated with end_of_level */
		      parse->level->end_level_list = parse->code_offset ; parse->code_offset += 2 ;
		      break ;
		    } ;
/*		   If here then must be full LOOP/LOOP1 construct */
		   if (parse->level->loop_inc_offset == 0)
		    prsu_error(ERRP,"INCLOOP","Missing \';increment\' in LOOP( xxx ) construct") ;
/*		   Add jump after increment back to test */
		   PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ;
		   if (parse->level->loop_test_offset == 0) prsu_error(ERRP,"INVOFFSET","Invalid LOOP/BRANCH/IF syntax") ;
		   parse->code[parse->code_offset++] =
			(parse->level->loop_test_offset > 0 ? parse->level->loop_test_offset - parse->pic_base : 0) ;
		   parse->code_offset += 2 ;
/*		   Now link up jump after test to begin of code */
		   if (parse->level->loop_test_ok_offset == 0) prsu_error(ERRP,"INVOFFSET","Invalid LOOP/BRANCH/IF syntax") ;
		   parse->code[parse->level->loop_test_ok_offset] = parse->code_offset - parse->pic_base ;
/*		   If LOOP1 then link up jump after initialization to code */
		   if (parse->level->in_loop1)
		    { if (parse->level->loop1_jump_offset == 0) prsu_error(ERRP,"INVOFFSET","Invalid LOOP/BRANCH/IF syntax") ;
		      parse->code[parse->level->loop1_jump_offset] = parse->code_offset - parse->pic_base ;
		    } ;
		   break ;
		 case POP_PCK_UNLOAD:
/*			Have to handle special so we can call PACKAGE_UNLOAD before package is gone ! */
			PUSHCODE(CODE_OPERATION,V3_XCT_PCK_UNLOADA) ; prsu_chk_oper(NULLV,NULLV,RES_NONE,1) ;
			PUSHCODE(CODE_OPERATION,V3_XCT_PCK_UNLOADB) ; break ;
		 case POP_FORK_SWITCH:
/*			Have to handle special so we can handle "/fork_leave/" module calls */
			PUSHCODE(CODE_OPERATION,V3_XCT_FORK_SWITCH1) ; break ;
		 case POP_BRANCH:
			PUSHCODE(CODE_OPERATION,V3_XCT_BRANCH) ; prsu_chk_oper(NULLV,NULLV,RES_NONE,1) ;
			parse->level->branch_info_offset = (parse->code_offset++) ;
			break ;
		 case POP_SYMREF:
/*			The next item on temp stack best be symbol reference pointer */
/*			Pop it off and call routine to generate code to reference symbol */
			prsu_gen_symbol_ref(sref = (struct prs__symbol_ref_list *)parse->tmp[--(parse->tmp_index)]) ;
			last_search_id = sref->srch_id ;	/* Save id for DCL PARTIAL */
			last_sref_type = sref->type ;
			last_sref_skel_index = sref->skel_index ; last_sref_last_el = sref->last_el ;
			top = parse->tmp[--(parse->tmp_index)] ;
			continue ;
		 case POP_RETURN:
			PUSHCODE(CODE_OPERATION,V3_XCT_RETURN1) ; break ;
		 case POP_PARTIAL:
/*			If this is partial for skeleton then push special operator & skelref */
			if (last_sref_type != SYMBOL_SKELETONREF) { PUSHCODE(CODE_FREQOP,V3_XCT_PARTIAL) ; }
			 else { PUSHCODE(CODE_OPERATION,V3_XCT_PARTIAL_SKELETON) ;
/*				Push reference to this skeleton so we can factor out offset */
				skelref.skel_index = last_sref_skel_index ; skelref.el_index = last_sref_last_el ;
				parse->code[parse->code_offset++] = prsu_stash_pure((char *)&skelref,sizeof skelref) ;
			      } ;
/*			Next item on temp stack is pointer to symbol */
			user_sym_ptr = (struct db__prs_sym *)parse->tmp[--(parse->tmp_index)] ;
/*			Update its search id with whatever the parser last used */
			user_sym_ptr->search_struct_id = last_search_id ;
/*			If this is a partial of a skeleton then do below */
			if (last_sref_type == SYMBOL_SKELETONREF)
			 { user_sym_ptr->type = SYMBOL_SKELETONREF ; user_sym_ptr->structure_id = last_sref_last_el + 1 ; } ;
			top = parse->tmp[--(parse->tmp_index)] ; continue ;
		 case POP_NULL: break ;
		 default: prsu_error(INTERNAL+POINT,"INVPOP","Invalid POP_do for this operator") ;
	       } ;
/*	      Append this operator to code buffer */
append_op:
	      if (top->reference != NULLV)
	       { PUSHCODE(CODE_OPERATION,top->reference) ; prsu_chk_oper(top,0,0,0) ; } ;
	      top = parse->tmp[--(parse->tmp_index)] ;
	    } ;
have_op3:
/*	   When here then ready to do something with new opr (in cur) */
	   switch (cur->on_push_do)
	    { case PUSH_LBRACE:
		if (cur == (struct db__opr_info *)&notbl_lbrace_slash)
		 { prsu_nxt_token(FALSE) ;	/* Get level name */
		   if (tkn.type != TKN_SYMBOL)
		    prsu_error(ERRP,"MISSINGLVLNAME","The {/ and }/ tokens require a name to follow") ;
		   prs_begin_level(tkn.symbol,FALSE,FALSE,FALSE) ;
		 } else
		 { prs_begin_level(NULLV,FALSE,FALSE,FALSE) ;
		 } ;
		/* xxx */
		goto next_statement ;
	      case PUSH_RBRACE:
		if (parse->level->is_command)
		 prsu_error(ERRP,"INVCOMTERM","A command level must be terminated with \'>>\'") ;
		if (parse->level->prior_level == NULLV)
		 prsu_error(ERRP,"TOOMANYRBRACES","Already at top parse level") ;
/*		If prior level is interpretive then append a "RETURN" to this level code */
		if (parse->level->prior_level->interpretive)
		 { PUSHCODE(CODE_FREQOP,V3_XCT_PUSHEOA) ;
		   PUSHCODE(CODE_OPERATION,V3_XCT_RETURN1) ; PUSHCODE(CODE_OPERATION,V3_XCT_RETURN) ;
		   PUSHCODE(CODE_OPERATION,V3_XCT_STOPXCT) ;
		 } ;
		if (cur == (struct db__opr_info *)&notbl_rbrace_slash)
		 { prsu_nxt_token(FALSE) ;
		   if (tkn.type != TKN_SYMBOL)
		    prsu_error(ERRP,"MISSINGLVLNAME","The {/ and }/ tokens require a name to follow") ;
		   prs_end_level(tkn.symbol) ;
		 } else
		 { prs_end_level(NULLV) ;
		 } ;
		/* xxx */
		if (parse->level->interpretive) { goto next_statement ;}
		 else goto next_token ;
	      case PUSH_RBRACKET:
		if (top != (struct db__opr_info *)&notbl_lbracket)
		 prsu_error(WARNP,"BRACKETMATCH","Mismatched brackets") ;
		--(parse->tmp_index) ; break ;
	      case PUSH_GOTO:
/*		Get next token - best be a symbol */
		prsu_nxt_token(FALSE) ;
		if (tkn.type != TKN_SYMBOL)
		 prsu_error(ERRP,"INVGOTOARG","Argument to GOTO must be statement label") ;
		if ((user_sym_ptr = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,0)) != NULLV)
		 { if (user_sym_ptr->type == SYMBOL_LABEL)
		    { PUSHCODE(CODE_FREQOP,(parse->level->in_command ? V3_XCT_JUMPC : V3_XCT_JUMP)) ;
		      PUSHVAL(user_sym_ptr->mempos.all - parse->pic_base) ;
		      PUSHVAL(user_sym_ptr->level_id) ; parse->code_offset-- ; PUSHLEVEL ;
		      if (parse->level->level_id < user_sym_ptr->level_id)
		       prsu_error(WARNP,"LABELLVLERR","Goto\'s cannot jump into level") ;
		      break ;
		    } ;
		   if (user_sym_ptr->type != SYMBOL_UNDEF_LABEL)
		    prsu_error(WARNP,"SYMNOTLABEL","Symbol is not a statement label") ;
/*		   If here then undefined label - link it up */
		   PUSHCODE(CODE_FREQOP,(parse->level->in_command ? V3_XCT_JUMPC : V3_XCT_JUMP)) ;
		   PUSHVAL(user_sym_ptr->mempos.all - parse->pic_base) ;
		   user_sym_ptr->mempos.all = parse->code_offset-1 ; PUSHLEVEL ;
		   break ;
		 } ;
/*		If here then symbol has not been defined - make it a label */
		user_sym_ptr = (struct db__prs_sym *)prsu_sym_add(tkn.symbol,PRS_LABEL_SYM) ;
		user_sym_ptr->type = SYMBOL_UNDEF_LABEL ;
		PUSHCODE(CODE_FREQOP,(parse->level->in_command ? V3_XCT_JUMPC : V3_XCT_JUMP)) ; PUSHVAL(0) ;
		user_sym_ptr->mempos.all = parse->code_offset-1 ; PUSHLEVEL ;
		break ;
	      case PUSH_THEN:
		PUSHCODE(CODE_FREQOP,V3_XCT_IF) ; prsu_chk_oper(NULLV,NULLV,RES_UNK,1) ;
/*		Save current code index for jump around THEN code */
		parse->level->if_code_offset = (parse->code_offset++) ;
		break ;
	      case PUSH_ELSE:
/*		Update location for jump around of THEN code */
		if (parse->level->if_code_offset == 0) prsu_error(ERRP,"INVOFFSET","Invalid LOOP/BRANCH/IF syntax") ;
		parse->code[parse->level->if_code_offset] = parse->code_offset+2 - parse->pic_base ;
		PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ;
/*		Save current code index for jump around ELSE code */
		parse->level->if_code_offset = (parse->code_offset++) ;
		break ;
	      case PUSH_ELSEIF:
/*		Link next code slot for end of level */
		PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ;
		parse->code[parse->code_offset] =
			(parse->level->end_level_list > 0 ? parse->level->end_level_list - parse->pic_base : 0) ;
		parse->level->end_level_list = (parse->code_offset++) ; PUSHLEVEL ;
/*		Now reset some flags to allow another THEN and ELSE/ELSE_IF */
		parse->level->have_then = (parse->level->have_else = FALSE) ;
/*		Now link up for jump around the THEN */
		parse->code[parse->level->if_code_offset] = parse->code_offset - parse->pic_base ;
		break ;
	      case PUSH_PATH:
		PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ;
/*		Link next code slot for end_of_level */
		parse->code[parse->code_offset] =
			(parse->level->end_level_list > 0 ? parse->level->end_level_list - parse->pic_base : 0) ;
		parse->level->end_level_list = (parse->code_offset++) ; PUSHLEVEL ;
/*		Create a new path slot in branch_info */
		if ((binfo = parse->level->branch_info_ptr) == NULLV)
/*		   Define a BRANCH level to try to recover */
		 { prs_begin_level(NULLV,FALSE,TRUE,FALSE) ;
		   parse->level->in_branch = TRUE ; parse->level->branch_top_offset = parse->code_offset ;
		   parse->level->branch_info_ptr = (struct db__branch_info *)v4mm_AllocChunk( sizeof *parse->level->branch_info_ptr,TRUE) ;
		   parse->level->branch_info_offset = parse->code_offset++ ;
		   prsu_error(ERRP,"MISSINGBRANCH","Cannot have PATH without BRANCH") ;
		 } ;
/*		Look for "PATH(value)" format */
		prsu_nxt_token(FALSE) ;
		if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lparen)
		 { if (binfo->count != 0)
		    prsu_error(WARNP,"PATHMIX","Cannot mix \'PATH\' and \'PATH(value)\' statements in same BRANCH") ;
another_path_value:
		   if (! prsu_eval_int_exp(TRUE,&tkn.value.int_lit))
		    prsu_error(ERRP,"BADPATHVAL","PATH value must be an integer value or expression") ;
		   prsu_nxt_token(FALSE) ;
/*		   Look for sequence of form: a..b */
		   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_dot_dot)
		    { i = tkn.value.int_lit ;
		      if (! prsu_eval_int_exp(TRUE,&tkn.value.int_lit))
		       prsu_error(ERRP,"BADPATHVAL","PATH value must be an integer value or expression") ;
		      if (tkn.value.int_lit < i)
		       prsu_error(WARNP,"BADPATHRANGE","Second integer of PATH(a..b) must be greater than first") ;
		      for(j=i;j<tkn.value.int_lit;j++)
		       { binfo->values[binfo->value_max] = j ;
			 binfo->value_offsets[binfo->value_max++] = parse->code_offset - parse->pic_base ;
		       } ;
		      prsu_nxt_token(FALSE) ;
		    } ;
		   binfo->values[binfo->value_max] = tkn.value.int_lit ;
		   binfo->value_offsets[binfo->value_max++] = parse->code_offset - parse->pic_base ;
/*		   Check for another value (via comma) or ending paren */
		   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_comma) goto another_path_value ;
		   if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rparen)
		    prsu_error(ERRP,"BADPATHVALTERM","Expecting \')\', \',\', or \'..\' here") ;
		 } else { prsu_nxt_token(TRUE) ;	/* Don't have PATH(value) format */
			  if (binfo->value_max != 0)
			   prsu_error(WARNP,"PATHMIX","Cannot mix \'PATH\' and \'PATH(value)\' statements in same BRANCH") ;
			  binfo->path_offsets[binfo->count++] = parse->code_offset - parse->pic_base ;
			} ;
/*		Do we have any FALL_THRUs from prior PATH */
		j = (parse->level->branch_fall_thru_list > 0 ? parse->level->branch_fall_thru_list - parse->pic_base : 0) ;
		cnt = 500 ;
		while ((i = j) && (cnt-- > 0))
		 { j = parse->code[i+parse->pic_base] ; parse->code[i+parse->pic_base] = parse->code_offset - parse->pic_base ;
/*		   Update next location with jump-to level id */
		   parse->code[i+1+parse->pic_base] = parse->level->level_id ;
		 } ; if (cnt <= 0) prsu_error(ABORT,"INTERR","Internal compiler error") ;
		parse->level->branch_fall_thru_list = 0 ;
		break ;
	      case PUSH_FALL_THRU:
/*		Look for explicit level */
		if ((tlevel = prsu_find_level()) == NULLV)
		 { tlevel = parse->level ;
/*		   No explicit level - search for prior BRANCH */
		   do { if (tlevel->in_branch) goto found_ft_branch ; } while (tlevel = tlevel->prior_level) ;
		   prsu_error(ERRP,"BADFALLTHRUPOS","FALL_THRU only allowed within a BRANCH") ;
		 } else
		 { if (! tlevel->in_branch)
		    prsu_error(WARNP,"LEVELNOTBRANCH","Level named in FALL_THRU is not a BRANCH !") ;
		 } ;
found_ft_branch:
		PUSHCODE(CODE_FREQOP,(parse->level->in_command ? V3_XCT_JUMPC : V3_XCT_JUMP)) ;
/*		Link up to fall_thru list */
		parse->code[parse->code_offset] =
			(tlevel->branch_fall_thru_list > 0 ? tlevel->branch_fall_thru_list - parse->pic_base : 0) ;
		tlevel->branch_fall_thru_list = (parse->code_offset++) ; PUSHLEVEL ;
		break ;
	      case PUSH_OUT:
/*		Look for explicit level */
		if ((tlevel = prsu_find_level()) == NULLV)
		 { tlevel = parse->level ;
/*		   No explicit level - search for prior BRANCH/LOOP/... */
		   do { if (tlevel->in_branch || tlevel->in_loop) goto found_po_branch ;
		      } while (tlevel = tlevel->prior_level) ;
		   prsu_error(ERRP,"INVOUTPOS","OUT only allowed within a BRANCH/LOOP") ;
		 } else
		 { if (! (tlevel->in_branch || tlevel->in_loop))
		    prsu_error(ERRP,"LEVELNOTBRANCHLOOP","Level named in OUT is not a BRANCH/LOOP !") ;
		 } ;
found_po_branch:
		if (parse->level->in_command)
		 { PUSHCODE(CODE_FREQOP,V3_XCT_JUMPC) ; } else { PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ; } ;
		parse->code[parse->code_offset] = (tlevel->end_level_list > 0 ? tlevel->end_level_list - parse->pic_base : 0) ;
		tlevel->end_level_list = (parse->code_offset++) ; PUSHLEVEL ;
		break ;
	      case PUSH_LOOP:
		prs_begin_level(NULLV,FALSE,TRUE,FALSE) ;
		parse->level->in_loop = TRUE ; parse->level->in_loop_setup = TRUE ;
/*		If got LOOP1 then remember also */
		if (cur->loop1) parse->level->in_loop1 = TRUE ;
/*		Check next token for "(" */
		prsu_nxt_token(FALSE) ;
		if (tkn.sym_ptr != (struct db__opr_info *)&notbl_lparen) parse->level->is_loop_forever = TRUE ;
		prsu_nxt_token(TRUE) ;	/* Want to scan token again */
		break ;
	      case PUSH_CONTINUE:
/*		Look for explicit level */
		if ((tlevel = prsu_find_level()) == NULLV)
/*		   Have to search back for loop level */
		 { tlevel = parse->level ;
		   do { if (tlevel->in_loop) goto found_loop_level ;
			if (tlevel->interpretive) break ;
		      } while (tlevel = tlevel->prior_level) ;
		   prsu_error(ERRP,"NOTINLOOP","CONTINUE may only be used within a LOOP/LOOP1 construct") ;
		 } else
		 { if (!tlevel->in_loop)
		    prsu_error(ERRP,"LEVELNOTLOOP","Level named in CONTINUE is not a LOOP/LOOP1 level") ;
		 } ;
found_loop_level:
		if (parse->level->in_command)
		 { PUSHCODE(CODE_FREQOP,V3_XCT_JUMPC) ; } else { PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ; } ;
/*		Jump to increment code for loop */
		if (tlevel->loop_inc_offset == 0) prsu_error(ERRP,"INFOFFSET","Invalid LOOP/BRANCH/IF syntax") ;
		parse->code[parse->code_offset++] = (tlevel->loop_inc_offset > 0 ? tlevel->loop_inc_offset - parse->pic_base : 0) ;
		parse->code[parse->code_offset] = tlevel->level_id ; PUSHLEVEL ;
		break ;
	      case PUSH_SEMI:
ps1:
/*		Have to do special test for LOOP(init;test;inc) construct */
		if (parse->level->in_loop_setup && top == (struct db__opr_info *)&notbl_lparen)
		 { if (parse->level->loop_test_offset == 0)
/*		      Got first ";" delimiting init & test */
		    { if (parse->level->in_loop1)
		       { PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ;
/*			 Have LOOP1 - Jump around test for first time */
			 parse->level->loop1_jump_offset = parse->code_offset ;
			 parse->code_offset += 3 ;
		       } ;
		      parse->level->loop_test_offset = parse->code_offset ;
		      goto next_token ;
		    } ;
ps2:
		   if (parse->level->is_loop_do)
		    prsu_error(ERRP,"INVLOOPARGS","Cannot mix LOOP(init;test;inc) with LOOP(var:=init..max..inc)") ;
/*		   Should be at semi between test & increment */
		   if (parse->level->loop_inc_offset != 0)
		    prsu_error(ERRP,"TOOMANYSEMIS","Too many semicolons in LOOP construct") ;
/*		   Set up code to test */
/*		   If no test then inject TEST_TRUE operator which always takes "true" branch */
		   if (parse->code_offset == parse->level->loop_test_offset)
		    { PUSHCODE(CODE_OPERATION,V3_XCT_LOOP_TEST_TRUE) ; }
		    else { PUSHCODE(CODE_OPERATION,V3_XCT_LOOP_TEST) ; } ;
		   parse->level->end_level_list = parse->code_offset ; parse->code_offset += 2 ;
		   parse->level->loop_test_ok_offset = parse->code_offset ; parse->code_offset += 2 ;
		   parse->level->loop_inc_offset = parse->code_offset ;
		   goto next_token ;
		 } ;
/*		Regular handling of end_of_statement */
		parse->tmp_index-- ;
		if (top != (struct db__opr_info *)&notbl_box)
		 { if (top == (struct db__opr_info *)&notbl_lparen)
		    prsu_error(ERRP,"PARENMATCH","Too many \'(\'s in this statement") ;
		   prsu_error(ERRP,"SYNTAXERROR","Invalid statement syntax") ;
		 } ;
		if (TRUE)
		 { PUSHCODE(CODE_FREQOP,V3_XCT_EOSLN) ;
		   parse->code[parse->code_offset++] = parse->ilvl[parse->ilx].current_line ;
		 } else
		 { PUSHCODE(CODE_FREQOP,V3_XCT_EOS) ; } ;
/*		If this is an interpretive parse level then go xct */
		if (parse->level->interpretive) goto xct_statement ;
		goto next_statement ;
	      case PUSH_2LANGLE:
		if (parse->level->interpretive)
		 prsu_error(ERRP,"INVCOMPOS","Cannot declare a command level from interpretive mode") ;
		if (parse->level->in_if)
		 prsu_error(ERRP,"INVCOMINIF","Cannot declare a command within IF without another {xxx} level") ;
		if (parse->code_offset != box_code_offset)
		 prsu_error(ERRP,"INVCOMPOS","Can only declare a command level at the beginning of a statment") ;
/*		Make sure we got a command name: <</name()  */
		prsu_nxt_token(FALSE) ;
		if (tkn.sym_ptr != (struct db__opr_info *)&notbl_slash)
		 prsu_error(ERRP,"INVCOMNAME","Expecting command level name of form: \'<</name\'") ;
		prsu_nxt_token(FALSE) ;
		if (tkn.type != TKN_SYMBOL)
		 prsu_error(ERRP,"INVCOMNAME","Expecting a command level name of form: \'<</name\'") ;
/*		Set up for prior level to jump over this code */
		PUSHCODE(CODE_FREQOP,V3_XCT_JUMP) ; i = ((parse->code_offset+=2)-2) ;
/*		Start up a new command level */
		strcpy(sym_name,tkn.symbol) ;
		prs_begin_level(tkn.symbol,FALSE,FALSE,TRUE) ;
/*		Finish up linkage for jump over command level */
		parse->code[i] = (parse->level->end_level_list > 0 ? parse->level->end_level_list - parse->pic_base : 0) ;
		parse->level->end_level_list = i ;
/*		Now find higher level module & update its command list */
		tlevel = parse->level->prior_level ;
		while (!tlevel->is_module)
		 { tlevel = tlevel->prior_level ;
		   if (tlevel == NULLV)
		    prsu_error(ERRP,"MISSINGMODLEVEL","Could not find prior module level in prsu_parser") ;
		 } ;
		clp = tlevel->module_command_list_ptr ;
		if (clp->count > V3_MODULE_COMMAND_MAX)
		 prsu_error(ERRP,"TOOMANYCOMS","Exceeded max number of commands per module") ;
/*		Copy useful info into next slot about this command */
		strcpy(clp->com[clp->count].name,sym_name) ;
		clp->com[clp->count].hash = prsu_str_hash(sym_name,-1) ;
		clp->com[clp->count].level_id = parse->level->prior_level->level_id ;
		clp->com[clp->count].cmd_code_offset = parse->code_offset - parse->pic_base ;
		clp->count++ ;
		goto next_statement ;
	      case PUSH_2RANGLE:
		if (!parse->level->is_command)
		 prsu_error(ERRP,"INVCOMLVLTERM","Invalid use of \'>>\' - Not currently in command level") ;
		PUSHCODE(CODE_FREQOP,V3_XCT_PUSHEOA) ;
		PUSHCODE(CODE_OPERATION,V3_XCT_RETURN1) ; PUSHCODE(CODE_OPERATION,V3_XCT_RETURN) ;
/*		Look for optional "/name" */
		prsu_nxt_token(FALSE) ;
		if (tkn.sym_ptr == (struct db__opr_info *)&notbl_slash)
		 { prsu_nxt_token(FALSE) ;
		   if (tkn.type != TKN_SYMBOL)
		    prsu_error(ERRP,"MISSINGCOMNAME","The >>/ token requires a name to follow") ;
		   prs_end_level(tkn.symbol) ;
		 } else
		 { prsu_nxt_token(TRUE) ; prs_end_level(NULLV) ;
		 } ;
		goto next_statement ;
	      case PUSH_DOT_DOT:
		if (!parse->level->in_loop_setup)
		 prsu_error(ERRP,"INVDOTDOTPOS","The \'..\' construct only allowed within LOOP( xxx )") ;
		if (parse->level->loop_test_offset != 0)
		 prsu_error(ERRP,"INVLOOPSYNTAX","Cannot mix LOOP(init;test;inc) with LOOP(var:=init..max..inc)") ;
		if (parse->level->have_loop_do_inc)
		 prsu_error(WARNP,"MAXDOTDOTS","Too many \'..\'s within LOOP") ;
		if (parse->level->is_loop_do) { parse->level->have_loop_do_inc = TRUE ; }
		 else { parse->level->is_loop_do = TRUE ; } ;
		goto next_token ;
	     } ;
	   if (cur->check_paren)
	    { parse->tmp_index-- ;
/*	      Make sure top has corresponding paren, brace, etc. */
	      if (cur == (struct db__opr_info *)&notbl_rparen && top != (struct db__opr_info *)&notbl_lparen)
		prsu_error(ERRP,"PARENMATCH","Mismatched parens, braces, ...") ;
	    } ;
	   if (cur->end_of_arg_list)
	    { PUSHCODE(CODE_FREQOP,V3_XCT_PUSHEOA) ; prsu_chk_oper(NULLV,NULLV,RES_EOA,0) ; } ;
	   if (cur->throw_out) continue ;
	   if (cur->xct_immed_end)
	    { PUSHCODE(CODE_OPERATION,cur->reference) ; prsu_chk_oper(cur,0,0,0) ; continue ; } ;
/*	   Check on some other stuff */
	   /* xxx */
have_op4:
/*	   Push operator on temp stack */
	   PUSHTMP(cur) ;
	 } ;
/*	Here to xct just compiled statement */
xct_statement:
	PUSHCODE(CODE_OPERATION,V3_XCT_STOPXCT) ;
	psi->code_ptr = (V3OPEL *)&parse->code[int_code_start] ;
/*	Before doing anything, see if we should (re)bind globals/externals */
	if (parse->glbl_xtrnal_count != parse->globals.count+parse->externals.count)
	 { parse->glbl_xtrnal_count = parse->globals.count+parse->externals.count ;
	   if (!parse->in_pck_compile) pcku_load_package(process->package_ptrs[parse->package_id]) ;
	   for(i=0;i<parse->skeletons.count;i++)
	    { sobu_load_skeleton(&parse->skeletons,(struct db__prs_skeleton *)&parse->skeletons.buf[parse->skeletons.index[i]],0) ; } ;
	 } ;
	save_code_offset = parse->code_offset ;
	xctu_main() ;	/* Do it */
/*	Reset some stuff for next statement */
	parse = process->package_ptrs[psi->package_index]->parse_ptr ;
/*	If we did not load any code then reset parse->code_offset */
	if (save_code_offset == parse->code_offset)
	 parse->code_offset = box_code_offset ;
	goto next_statement ;
 }

/*	P A R S E  -   H A N D L E   S Y M B O L I C   R E F E R E N C E	*/

/*	prsu_parser_symbol - Parses symbolic references	*/
void prsu_parser_symbol(sp,ssokflag)
  struct db__prs_sym *sp ;	/* User symbol pointer */
  int ssokflag ;		/* If TRUE then subscripting is ok */
 {
   struct db__prs_skeleton *skel ;
   union val__sob tname ;
   int i,level ;

/*	Check for first call (for this symbolic reference) */
	if (parse->tmp[parse->tmp_index] == (struct db__opr_info *)&notbl_symref)
	 { sref = (struct prs__symbol_ref_list *)parse->tmp[parse->tmp_index-1] ; }
	 else
	 { sref = (struct prs__symbol_ref_list *)v4mm_AllocChunk(sizeof *sref,TRUE) ; PUSHTMP(sref) ; PUSHTMP(&notbl_symref) ; } ;
/*	If passed a module then parse remainder of: "[module-n].xxx" */
	if (sp != NULLV)
	 { if (sp->type == SYMBOL_MODULE)
	    { sref->module_id = sp->module_id ; sref->frames = 0 ;
/*	      Look for "-n" */
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_minus) { prsu_nxt_token(TRUE) ; }
	       else { prsu_nxt_token(FALSE) ;
		      if (tkn.type != TKN_LITERAL || tkn.lit_format.fld.type != VFT_BININT)
		       prsu_error(ERRP,"INVOFFSET","Expecting a literal integer here") ;
		      sref->frames = tkn.value.int_lit ;
		    } ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_rbracket) prsu_error(ERRP,"INVMODSPEC","Expecting a \']\' or \',\' here") ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.sym_ptr != (struct db__opr_info *)&notbl_dot) prsu_error(ERRP,"MISSINGDOT","Expecting a \'.\' here") ;
	      prsu_nxt_token(FALSE) ;
	      if (tkn.type != TKN_SYMBOL) prsu_error(ERRP,"NOTUSERSYM","Expecting a user symbol here") ;
	      if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,0-(sref->module_id))) == NULLV)
	       prsu_error(ERRP,"SYMNOTINMOD","This symbol not defined within specified module") ;
	     } ;
	 } ;
	for (;;)
/*	   If we got a symbol (sp != NULLV) then append to current list */
	 { if (sp != NULLV)
	    { if (sref->count == 0)
/*		 First call, initialize some stuff, especially if skeleton ref */
	       { sref->type = sp->type ;
		 if (sref->type == SYMBOL_SKELETONREF)
		  { sref->last_el = sp->structure_id-1 ; sref->skel_index = sp->search_struct_id ; } ;
	       } ;
	      sref->sym[++(sref->count)].sp = sp ;
	      sref->srch_id = sp->search_struct_id ;
	    } ;
	   prsu_nxt_token(FALSE) ;
/*	   Did we get a "."	*/
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_dot)
/*	      Got a dot - is it an object reference ? */
	    { if (sref->type == SYMBOL_OBJREF)
	       { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ; sp = NULLV ;
		 if (tkn.type != TKN_SYMBOL)
		  prsu_error(ERRP,"NOTOBJNAME","Expecting an object name here") ;
		 sref->sym[++(sref->count)].obname.all = sobu_name_create(tkn.symbol,parse->package_id) ;
		 parse->have_value = TRUE ;
		 continue ;
	       } ;
 	      if (sref->type == SYMBOL_SKELETONREF)
	       { prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
		 if (tkn.type != TKN_SYMBOL)
		  prsu_error(ERRP,"NOTSKELNAME","Expecting a skeleton element name here") ;
/*		 Make sure this symbol is in the skeleton */
		 skel = (struct db__prs_skeleton *)&(parse->skeletons.buf[parse->skeletons.index[sref->skel_index]]) ;
		 level = (sref->last_el < 0 ? 1 : skel->el[sref->last_el].level + 1) ;
		 tname.all = sobu_name_lookup(tkn.symbol,parse->package_id) ;
		 for(i=sref->last_el+1;i<skel->count;i++)
		  { if (skel->el[i].level < level) { i = 9999 ; break ; } ;
		    if (skel->el[i].level > level) continue ;
		    if (skel->el[i].el_name.all == tname.all) break ;
		  } ;
/*		 Did we find the symbol ? */
		 if (i >=skel->count)
		  prsu_error(ERRP,"NOSKELEL","No such skeleton STRUCT element") ;
		 sref->last_el = i ;
		 parse->have_value = TRUE ;
		 continue ;
	       } ;
	      if (sp == NULLV) sp = sref->sym[sref->count].sp ;
	      if (sp->search_struct_id == 0)
	       prsu_error(ERRP,"INVDOTUSAGE","Cannot use dot construct on non STRUCT symbol") ;
	      prsu_nxt_token(FALSE+TKN_FLAGS_STREL) ;
	      if (tkn.type != TKN_SYMBOL)
	       prsu_error(ERRP,"STRUCTELNOTSYM","Must have a STRUCT symbol after dot") ;
	      if ((sp = (struct db__prs_sym *)prsu_sym_lookup(tkn.symbol,sref->srch_id)) == NULLV)
	      prsu_error(ERRP,"SYMNOTSTRUCTEL","Symbol not defined as STRUCT element") ;
	     parse->have_value = TRUE ;
	     continue ;	/* Keep plugging */
	    } ;
/*	   Did we get a "[]" ? */
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lrbracket)
	    { sref->ignore_subs = TRUE ; return ; } ;
/*	   Did we get a "[" ? 	*/
	   if (tkn.sym_ptr == (struct db__opr_info *)&notbl_lbracket)
	    { if (!ssokflag)
	       prsu_error(ERRP,"NOSUBS","Subscripting not allowed here") ;
	      parse->have_value = FALSE ;
/*	      If first time then push EOA to mark end and remember */
	      if (!sref->got_subs)
	       { PUSHCODE(CODE_FREQOP,V3_XCT_PUSHEOA) ; prsu_chk_oper(NULLV,NULLV,RES_EOA,0) ;
		  sref->got_subs = TRUE ;
	       } ;
	      PUSHTMP(&notbl_lbracket) ; return ;
	    } ;
/*	   Have just a regular symbol - append to code */
	   prsu_nxt_token(TRUE) ;
	   return ;
	 } ;
}

/*	prsu_V4IsctMacro - Expand ['v4 isct'] as if a macro	*/

void prsu_V4IsctMacro(v4isct,quoted)
  char *v4isct ; int quoted ;
{ struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point isctpt,valpt ;
  struct V4DPI__Point *v4dpi_IsctEval() ;
  char ebuf[250] ; char tb2[250] ; char *s,*d ;
  static char tb1[250] ;

	if (parse->ctx == NULL) prsu_error(ERRP,"NOCTX","Cannot evaluate [ xxx ] - no context defined") ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(v4isct),V4LEX_InpMode_String) ;
	v4dpi_PointParse(parse->ctx,&isctpt,&tcb,0) ;
	if (v4dpi_IsctEval(&valpt,&isctpt,parse->ctx,0,NULL,NULL) == NULL)
	 { sprintf(ebuf,"Evaluation of [\"%s\"] failed",v4isct) ; prsu_error(ERRP,"ISCTMACUNDEF",ebuf) ; } ;
	if (valpt.PntType != V4DPI_PntType_Char || valpt.Grouping != V4DPI_Grouping_Single)
	 { sprintf(ebuf,"Evaluation of [\"%s\"] returned value type (%d) - must be single valued Alpha",v4isct,valpt.PntType) ;
	   prsu_error(ERRP,"ISCTMACNOTCHAR",ebuf) ;
	 } ;
	if (valpt.Value.AlphaVal[0] < 255)
	 { strncpy(tb1,&valpt.Value.AlphaVal[1],valpt.Value.AlphaVal[0]) ; tb1[valpt.Value.AlphaVal[0]] = 0 ; }
	 else { strcpy(tb1,&valpt.Value.AlphaVal[1]) ; } ;
	if (quoted)
	 { tb2[0] = '"' ; s = tb1 ; d = &tb2[1] ;			/* Have to quote the result */
	   for(;;) { if (*s == 0) break ; if (*s == '"') { *(d++) = '\\' ; } ; *(d++) = *(s++) ; } ;
	   *(d++) = '"' ; *d = 0 ; strcpy(tb1,tb2) ;
	 } ;
printf("Eval of [%s] => %s\n",v4isct,tb1) ;
	parse->constant_input_ptr_bkp[++parse->constant_bkp_count] = parse->ilvl[parse->ilx].input_ptr ;
	parse->constant_start_ptr[parse->constant_bkp_count] = (parse->ilvl[parse->ilx].input_ptr = tb1) ;
}
