/*	V3STRU.C - String Utilities for VICTIM III

	Last edited 7/30/84 by Victor E. Hansen		*/

#include <time.h>
#include "v3defs.c"

/*	Set up some stuff for global linkages		*/
extern struct db__process *process ;	/* Master process pointer */
extern struct db__psi *psi ;		/* Master psi		*/

char *stru_popcstr() ;
char *stru_alloc_bytes() ;

/*	Set up Circular buffer for string routines requiring character buffers */
static struct {
  unsigned short start_index ;	/* Index into below to starting string */
  unsigned short index ;	/* Index into below to next free byte */
  char buf[V3_STR_CIRCULAR_BUF_SIZE+500] ;
 } cbuf ;

/*	V 3   S T R I N G   F U N C T I O N S		*/

/*	stru_append - Appends one or more strings to target	*/

void stru_append()
{ struct str__ref dstr,sstr ;
   union val__format format ;
   int slen,dlen ;

/*	Pop off the src & dst strings	*/
	stru_popstr(&sstr) ; slen = stru_sslen(&sstr) ;
	stru_popstr(&dstr) ; dlen = stru_sslen(&dstr) ;
/*	Make sure dest is variable length */
	if (dstr.format.fld.type != VFT_VARSTR)
	 v3_error(V3E_ARG1NOTSTRING,"STR","APPEND","ARG1NOTSTRING","First argument must be SV/STRING",(void *)dstr.format.fld.type) ;
/*	Now append - but don't overflow */
	strncat(dstr.ptr,sstr.ptr,(dlen+slen >= dstr.format.fld.length ? dstr.format.fld.length-dlen-1 : slen)) ;
/*	Finally generate error if overflow */
	if (dlen+slen >= dstr.format.fld.length)
	 v3_error(V3E_TOOSHORT,"STR","APPEND","TOOSHORT","First argument not long enough",(void *)(dlen+slen)) ;
/*	Return VARSTR pointing to end */
	PUSHMP(dstr.ptr+dlen+slen) ;
	format.all = V3_FORMAT_VARSTR ;	format.fld.length = dstr.format.fld.length - slen ; PUSHF(format.all) ;
	return ;
}

/*	stru_be - Returns substring based on begin/end indexes	*/

void stru_be()
{ int si,ei,len ;
   struct str__ref str ;
   union val__format format ;

/*	Pop off the indexes & check them out		*/
	if ((ei = xctu_popint()) * (si = xctu_popint()) == 0)
	 v3_error(V3E_NOTZERO,"STR","BE","NOTZERO","Indexes cannot be zero",NULLV) ;
/*	Get the string */
	stru_popstr(&str) ;
/*	Are we indexing from right ? */
	if (ei < 0 || si < 0)
	 { len = stru_sslen(&str)+1 ;
	   if (ei < 0) ei += len ; if (si < 0) si += len ;
	 } ;
	if (si > ei) v3_error(V3E_STRINDEXES,"STR","BE","STRINDEXES","Start index cannot be greater than end index",NULLV) ;
/*	Return substring */
	PUSHMP(str.ptr+si-1) ;
	format.all = V3_FORMAT_FIXSTR ;	format.fld.length = ei-si+1 ; PUSHF(format.all) ;
	return ;
}

/*	stru_be2 - Returns substring based on begin/end indexes	*/

void stru_be2()
{ int si,ei,len ; int i ;
   struct str__ref str ;
   union val__format format ;

/*	Pop off the indexes & check them out		*/
	i = xctu_popint() ; si = i >> 16 ; ei = i & 0xFFFF ;
	if (si * ei == 0)
	 v3_error(V3E_NOTZERO,"STR","BE2","NOTZERO","Indexes cannot be zero",NULLV) ;
/*	Get the string */
	stru_popstr(&str) ;
/*	Are we indexing from right ? */
	if (ei < 0 || si < 0)
	 { len = stru_sslen(&str)+1 ;
	   if (ei < 0) ei += len ; if (si < 0) si += len ;
	 } ;
	if (si > ei) v3_error(V3E_STRINDEXES,"STR","BE2","STRINDEXES","Start index cannot be greater than end index",NULLV) ;
/*	Return substring */
	PUSHMP(str.ptr+si-1) ;
	format.all = V3_FORMAT_FIXSTR ;	format.fld.length = ei-si+1 ; PUSHF(format.all) ;
	return ;
}

/*	stru_bl - Returns substring based on start index & length */

void stru_bl()
{ int si,len ;
   struct str__ref str ;
   union val__format format ;

/*	Pop off index & length				*/
	if ((len = xctu_popint()) < 0)
	 v3_error(V3E_NEGLEN,"STR","BL","NEGLEN","Length cannot be negative",(void *)len) ;
	if ((si = xctu_popint()) == 0)
	 v3_error(V3E_ZEROSTART,"STR","BL","ZEROSTART","Start index cannot be zero",(void *)0) ;
/*	Get the string */
	stru_popstr(&str) ;
/*	Are we indexing from right ? */
	if (si < 0) si += stru_sslen(&str)+1 ;
/*	Return substring */
	PUSHMP(str.ptr+si-1) ;
	format.all = V3_FORMAT_FIXSTR ; format.fld.length = len ; PUSHF(format.all) ;
	return ;
}

/*	stru_break - Breaks string based on set of delimiters	*/

void stru_break()
{ union val__format format ;
   struct str__ref bstr,dstr ; int dlen,blen ;
   struct db__formatptr *se ;
   int i,j ;

/*	Get arguments */
	POPF(format.all) ;
	if (format.fld.type != VFT_INDIRECT)
	 v3_error(V3E_NOTREF,"STR","BREAK","NOTREF","Second argument to STR_BREAK(_RIGHT) must be STRING_REF",(void *)format.fld.type) ;
	POPVP(se,(struct db__formatptr *)) ; REPUSH ; stru_popstr(&bstr) ; blen = stru_sslen(&bstr) ;
	stru_popstr(&dstr) ; dlen = stru_sslen(&dstr) ;
/*	Going to return string starting with break string */
	PUSHMP(bstr.ptr) ; format.all = V3_FORMAT_FIXSTR ;
/*	Start searching for a delimiter */
	for (i=0;i<blen;i++)
	 { for (j=0;j<dlen;j++)
	    { if (*(dstr.ptr+j) == *(bstr.ptr+i)) goto found_delim ; } ;
	 } ;
/*	If here then hit end of break string with no delimiters */
	format.fld.length = blen ; PUSHF(format.all) ;
	se->ptr = bstr.ptr + blen ; se->format.fld.length = 0 ;
	return ;
/*	Here when got a delimiter - update appropriately */
found_delim:
	format.fld.length = i ; PUSHF(format.all) ;
	se->ptr = bstr.ptr + i + 1 ; se->format.fld.length -= (i+1) ;
	return ;
}

/*	stru_break_right - Breaks string (from right) based on set of delimiters	*/

void stru_break_right()
{ union val__format format ;
   struct str__ref bstr,dstr ; int dlen,blen ;
   struct db__formatptr *se ;
   int i,j ;

/*	Get arguments */
	POPF(format.all) ;
	if (format.fld.type != VFT_INDIRECT)
	 v3_error(V3E_NOTREF,"STR","BREAK_RIGHT","NOTREF","Second argument to STR_BREAK(_RIGHT) must be STRING_REF",(void *)format.fld.type) ;
	POPVP(se,(struct db__formatptr *)) ; REPUSH ; stru_popstr(&bstr) ; blen = stru_sslen(&bstr) ;
	stru_popstr(&dstr) ; dlen = stru_sslen(&dstr) ;
	format.all = V3_FORMAT_FIXSTR ;
/*	Make sure that the second argument is a fixed length string */
	se->format.all = V3_FORMAT_FIXSTR ;
/*	Start searching for a delimiter */
	for (i=blen-1;i>=0;i--)
	 { for (j=0;j<dlen;j++)
	    { if (*(dstr.ptr+j) == *(bstr.ptr+i)) goto found_delim ; } ;
	 } ;
/*	If here then hit begin of break string with no delimiters */
	PUSHMP(bstr.ptr) ;
	format.fld.length = blen ; PUSHF(format.all) ;
	se->ptr-- ; se->format.fld.length = 0 ;
	return ;
/*	Here when got a delimiter - update appropriately */
found_delim:
	PUSHMP(bstr.ptr+i+1) ;
	format.fld.length = blen-i-1 ; PUSHF(format.all) ;
	se->ptr = (char *)(bstr.ptr) ; se->format.fld.length = i ;
	return ;
}

/*	stru_compare - Compares two strings			*/

void stru_compare()
{ struct str__ref str1,str2 ;
   int len1,len2,res ;

/*	Get the strings */
	stru_popstr(&str2) ; stru_popstr(&str1) ;
/*	If either args are nil then return FALSE */
	if (str1.ptr == NULLV || str2.ptr == NULLV) { PUSHINT(-9999) ; return ; } ;
/*	Figure out how many bytes to compare */
	len1 = stru_sslen(&str1) ; len2 = stru_sslen(&str2) ;
	res = strncmp(str1.ptr,str2.ptr,(len1 > len2 ? len1 : len2)) ;
	if (res < 0) { res = -1 ; }
	 else { if (res > 0) res = 1 ; } ;
	PUSHINT(res) ;
	return ;
}

/*	stru_compress - Compresses one string into another	*/

void stru_compress()
{ struct str__ref outstr,instr ;

	stru_popstr(&instr) ; stru_popstr(&outstr) ;
	PUSHINT(data_compress(outstr.ptr,instr.ptr,stru_sslen(&instr),0,outstr.format.fld.length)) ;
}

/*	stru_concat - Concatenates all arguments to single string */

void stru_concat()
{ char *tstr,*tstr_cpy,*bp ; double *dbl_ptr ;
   union val__v3num v3n ;
   union val__format format ;
   struct num__ref anum ;
   V3STACK *base_ptr ;
   struct db__formatptr *fp ;
   int *(*mp_ptr) ;
   int ac,i,j,*int_ptr ; char *v3_tstr ;

/*	Allocate a string from temp storage */
	tstr_cpy = (tstr = (v3_tstr = stru_alloc_bytes(500))) ; *tstr = NULLV ;
/*	Count number of arguments */
	for (ac=0;;ac++)
	 { POPF(format.all) ; POPVI(i) ;
	   if (format.all == V3_FORMAT_EOA) break ;
	 } ;
/*	Know how many args, now print out in proper order */
	base_ptr = psi->stack_ptr-SIZEOFSTACKENTRY ;
	for (j=1;j<=ac;j++)
	 { psi->stack_ptr = base_ptr-(SIZEOFSTACKENTRY*j) ;
	   POPF(format.all) ;
/*	   What do we got ? */
	   switch (format.fld.type)
	    { default: POPVI(i) ; strcat(tstr,"*??*") ; break ;
	      case VFT_FIXSTRSK:
	      case VFT_FIXSTR: 	POPVP(bp,(char *)) ; sprintf(tstr,"%0.*s",format.fld.length,bp) ; break ;
	      case VFT_VARSTR:	POPVP(bp,(char *)) ; sprintf(tstr,"%s",bp) ; break ;
	      case VFT_BINWORD:
	      case VFT_BININT:
	      case VFT_BINLONG:
	      case VFT_STRINT:
	      case VFT_PACDEC:
	      case VFT_V3NUM:
	      case VFT_FLOAT:
		PUSHF(format.all) ; i = v3num_popv3n(&v3n) ; v3num_itx(&v3n,tstr,i) ; break ;
	      case VFT_INDIRECT:
		POPVP(fp,(struct db__formatptr *)) ;
		if (fp->format.fld.type == VFT_VARSTR) { sprintf(tstr,"%s",fp->ptr) ; }
		 else sprintf(tstr,"%0.*s",fp->format.fld.length,fp->ptr) ;
		 break ;
	      case VFT_OBJREF:
		POPVP(int_ptr,(int *)) ; sprintf(tstr,"(obj_%s)",sobu_name_str(*int_ptr)) ; break ;
	      case VFT_OBJECT:
		POPVI(i) ; sprintf(tstr,"(obj_%s)",sobu_name_str(format.all)) ; break ;
	      case VFT_POINTER:
		POPVP(mp_ptr,(int *(*))) ;
		if (format.fld.mode == VFM_IMMEDIATE) { sprintf(tstr,"^X%p",mp_ptr) ; break ; }
		 else { sprintf(tstr,"^X%p",*mp_ptr) ; break ; } ;
	    } ;
/*	   All done with argument, update tstr past it */
	   tstr += strlen(tstr) ;
	 } ;
/*	All done - return temp string */
	psi->stack_ptr = base_ptr+SIZEOFSTACKENTRY ;
	PUSHMP(v3_tstr) ;
	format.all = V3_FORMAT_FIXSTR ; format.fld.length = strlen(tstr_cpy) ; PUSHF(format.all) ;
	return ;
}

/*	stru_expand - Expands a compressed string	*/

void stru_expand()
{ struct str__ref outstr,instr ;

	stru_popstr(&instr) ; stru_popstr(&outstr) ;
	PUSHINT(data_expand(outstr.ptr,instr.ptr,stru_sslen(&instr))) ;
}

/*	stru_itos - Converts integer to single character string */

void stru_itos()
{ int i ; char *j ;
   union val__format format ;

/*	Pop integer off of V3 stack & just plug into cbuf	*/
	i = xctu_popint() ;
	j = (char *)stru_alloc_bytes(1) ; PUSHMP(j) ; cbuf.buf[cbuf.start_index] = i ;
	format.all = V3_FORMAT_FIXSTR ; format.fld.length = 1 ; PUSHF(format.all) ;
	return ;
}

/*	stru_len - Returns length of a string			*/

void stru_len()
{ struct str__ref str ;

	stru_popstr(&str) ;
	PUSHINT(stru_sslen(&str)) ;
	return ;
}

/*	stru_list - Returns element of string "list"		*/

void stru_list()
{ struct str__ref str ;
   union val__format format ;
   int index ; char delim,*last_byte,*sb ;

/*	First pop off arguments */
	if ((index = xctu_popint()) <= 0)
	 v3_error(V3E_BADLISTINDEX,"STR","LIST","BADLISTINDEX","Index to str_list() cannot be less than one",(void *)index) ;
	stru_popstr(&str) ; last_byte = str.ptr + stru_sslen(&str) ;
/*	What is out delimiter ? */
	if (*str.ptr >= 'A' && *str.ptr <= 'Z') { delim = ',' ; }
	 else { if (*str.ptr >= 'a' && *str.ptr <= 'z') { delim = ',' ; }
		 else { if (*str.ptr >= '0' && *str.ptr <= '9') { delim = ',' ; } else delim = *(str.ptr++) ; } ;
	      } ;
/*	Now position to proper element */
	for(;index>1;index--)
	 { for(;;)
	    { if (*(str.ptr++) == delim) break ;
	      if (str.ptr >= last_byte)
	       v3_error(V3E_INDEXTOOBIG,"STR","LIST","INDEXTOOBIG","Index to str_list() is too large",(void *)index) ;
	    } ;
	 } ;
/*	Got the element, now position to the next */
	PUSHMP(str.ptr) ; sb = str.ptr ;
	for(;;)
	 { if (*(str.ptr++) == delim) { str.ptr-- ; break ; } ;
	   if (str.ptr >= last_byte) break ;
	 } ;
/*	Got the element, return it */
	format.all = V3_FORMAT_FIXSTR ; format.fld.length = (char *)str.ptr - sb ; PUSHF(format.all) ;
}

/*	stru_logical - Returns TRUE or FALSE based on first char = Y or N */

void stru_logical()
{ struct str__ref str ;

	stru_popstr(&str) ;
	if (*str.ptr == 'Y') { PUSHTRUE ; }
	 else { if (*str.ptr == 'N') { PUSHFALSE ; }
		 else v3_error(V3E_NOTYN,"STR","LOGICAL","NOTYN","Argument to STR_LOGICAL not \'Y\' or \'N\'",(void *)*str.ptr) ;
	      } ;
	return ;
}

/*	stru_match - Performs various matching functions on 2 strings */

void stru_match()
{ char dstr[300000],sstr[5000] ;
   int flags ; char *dptr,*sptr,*xptr,*breakptr ;
   int i,j,k,slen,dlen,xlen ;

/*	First get the 3 arguments */
	flags = xctu_popint() ;
	if ((flags & V3_FLAGS_STR_COMPARE) != 0) { stru_compare() ; return ; } ;
	stru_popcstr(dstr) ; stru_popcstr(sstr) ;
/*	Want everything done upper case ? */
	if ((flags & V3_FLAGS_STR_NOCASE) != 0)
	 { stru_csuc(dstr) ; stru_csuc(sstr) ; } ;
/*	Doing a regular search ? */
	if ((flags & (V3_FLAGS_STR_PATTERN | V3_FLAGS_STR_KEYWORD)) == 0)
	 { if (flags & V3_FLAGS_STR_EXACT)
	    { if (strcmp(dstr,sstr) == 0)
	       { PUSHTRUE ; } else { PUSHFALSE ; } ;
	      return ;
	    } ;
	   slen = strlen(sstr) ;				/* sslen = length of src */
	   if (slen == 0) { PUSHFALSE ; return ; } ;		/* Src is 0 length - no match */
	   if (flags & V3_FLAGS_STR_BEGINS)
	    { if (strncmp(sstr,dstr,slen) == 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
	      return ;
	    } ;
	   k = 1 + strlen(dstr) - slen ;			/* k = number of times to iterate */
	   dptr = dstr ;
	   for (i=1;i<=k;(i++,dptr++))
	    { if (strncmp(sstr,dptr,slen) == 0) { PUSHINT(i) ; return ; } ; } ;
/*	   No match - return 0 */
	   PUSHINT(0) ; return ;
	 }
/*	How about a keyword search ? */
	if ((flags & V3_FLAGS_STR_KEYWORD) != 0)
	 { sptr = sstr ; slen = strlen(sstr) ;
	   breakptr = ((flags & V3_FLAGS_STR_NOSYN) ? "," : ",=") ;
try_kw_again:
	   dptr = dstr ; j = 0 ;
/*	   Loop thru all "keywords" in dstr (delimited with ",") */
	   for (i=1;(i == 1) || (*(dptr-1) != 0);(i++,dptr+=j+1))
	    { if ((j = strcspn(dptr,breakptr)) == 0)
/*		 Ran out of keywords - no match to be had */
	       { PUSHINT(0) ; return ; } ;
/*	      If delimited on "=" then scan for extra string */
	      if (*(dptr+j) == '=') { xptr = dptr+j+1 ; xlen = strcspn(dptr+j+1,",") ; }
	       else xptr = NULLV ;
/*	      If this keyword is less than search string then skip */
	      if (j < slen) continue ;
/*	      See if we have a match */
	      if (strncmp(sptr,dptr,slen) != 0) continue ;
/*	      Yes ! - do we need an exact match or can we accept partial */
	      if (j == slen) goto have_kw_match ; /* Got an exact match */
	      if ((flags & V3_FLAGS_STR_EXACT) != 0) continue ;
/*	      Partial match ok, check next keyword for ambiguous */
	      if (*(dptr+j) == NULLV) goto have_kw_match ;
	      if ((k = strcspn(dptr+j+1,",=")) == 0) goto have_kw_match ;
	      if (slen > k) goto have_kw_match ;
	      if (strncmp(sptr,dptr+j+1,slen) !=0) { goto have_kw_match ; }
/*		 Next keyword also matches, sptr is ambiguous */
	       { PUSHINT(-i) ; return ; } ;
	    } ;
	   PUSHINT(0) ; return ;
have_kw_match:
/*	   If we got a "xxx=yyy" match then have to search again */
	   if (xptr != NULLV)
	    { sptr = xptr ; slen = xlen ; goto try_kw_again ; } ;
	   PUSHINT(i) ; return ;
	 } ;
/*	Unimplemented operations */
	v3_error(V3E_NYI,"STR","MATCH","NYI","Feature not yet implemented",NULLV) ;
}

/*	stru_nulls - Sets all bytes of string to NULLs		*/

void stru_nulls()
{ struct str__ref str ;

	stru_popstr(&str) ; memset(str.ptr,0,str.format.fld.length) ;
}

/*	stru_prior - Returns prior character as string		*/

void stru_prior()
{ struct str__ref str ;
   union val__format format ;

	stru_popstr(&str) ;	/* Get argument */
	PUSHMP(str.ptr-1) ;
	format.all = V3_FORMAT_FIXSTR ; format.fld.length = 1 ; PUSHF(format.all) ;
	return ;
}

/*	stru_share - Does "==" (sharing) update for VFT_INDIRECT symbols */

void stru_share()
{ int *val,*(*ind) ;
  struct db__formatptr *fw ;

   union val__format format,tmp_format ;

/*	Pop off source format & value */
	POPF(format.all) ;
/*	Is src an indirect string ? */
	switch (format.fld.type)
	 { default:
		POPVP(val,(int *)) ; break ;
	   case VFT_INDIRECT:
		POPVP(fw,(struct db__formatptr *)) ; format.all = fw->format.all ; val = (int *)fw->ptr ; break ;
	   case VFT_STRUCTPTR:
		format.fld.type = VFT_FIXSTR ; POPVP(ind,(int *(*))) ;
		val = *ind ; break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
	   case VFT_VARSTR:
		POPVP(val,(int *)) ; break ;
	 } ;

/*	if (format.fld.type == VFT_INDIRECT)
	 { POPVP(fw,(struct db__formatptr *)) ; format.all = fw->format.all ; val = fw->ptr ; }
	 else
	 { if (format.fld.type == VFT_STRUCTPTR)
	    { format.fld.type = VFT_FIXSTR ; POPVP(ind,(int *(*))) ; val = *ind ;
	    } else
	    { POPVP(val,(int *)) ; } ;
	 } ;
*/
/*	Now make sure destination is indirect & get a pointer */
	POPF(tmp_format.all) ;
	if (tmp_format.fld.type != VFT_INDIRECT)
	 v3_error(V3E_ONLYSR,"STR","SHARE","ONLYSR","The \'==\' operator only allowed on SR/STRING_REF variables",(void *)tmp_format.fld.type) ;
/*	Update the indirect value doubleword */
	POPVP(fw,(struct db__formatptr *)) ; fw->format.all = format.all ; fw->ptr = (char *)val ;
/*	Now return with the value on the stack */
	REPUSH ; return ;
}

/*	stru_spaces - Sets all bytes of string to spaces		*/

void stru_spaces()
{ struct str__ref str ;
  int len ;

	stru_popstr(&str) ; len = stru_sslen(&str) ;
	memset(str.ptr,' ',len) ;
}

/*	stru_trim - Returns left/right trimmed value of its argument */

void stru_trim()
{ struct str__ref str ;
   union val__format format ;
   char *start,*end,*last_sig,*skip ;

	stru_popstr(&str) ; end = str.ptr + str.format.fld.length -1 ;
/*	First get rid of leading spaces & tabs */
	for(start=str.ptr;start<=end;start++)
	 { if (*start == 0) { last_sig = start-1 ; goto trim_done ; } ;
	   if (*start > ' ') break ;
	 } ;
	if (start > end) { last_sig = start-1 ; goto trim_done ; } ;
/*	Now look for trailing */
	for(skip=start;skip<=end;skip++)
	 { if (*skip == 0) goto trim_done ;
	   if (*skip > ' ') last_sig = skip ;
	 } ;
/*	Here at end, start points to begin, last_sig to the end */
trim_done:
	format.all = V3_FORMAT_FIXSTR ; format.fld.length = last_sig - start + 1 ;
	if (format.fld.length == 0) start = "" ;	/* VEH110923 - If zero-length then set string pointer to null-terminated string */
	PUSHMP(start) ; PUSHF(format.all) ;
	return ;
}

/*	stru_type - Returns "type" of string */

void stru_type()
{ struct str__ref str ;
  int i,slen,b ;
  int uc=0,lc=0,nc=0,oc=0,bc=0 ;


	stru_popstr(&str) ; slen = stru_sslen(&str) ;
	for(i=0;i<slen;i++)
	 { b = *str.ptr++ ;			/* Get next byte ; */
	   if (b >= 'A' && b <= 'Z') { uc++ ; }
	    else if (b >= 'a' && b <= 'z') { lc++ ; }
	    else if (b >= '0' && b <= '9') { nc++ ; }
	    else if (b == ' ' || b == '\t' || b == 0) { bc++ ; }
	    else oc++ ;
	 } ;
/*	Return code indicating type of string - which count is largest? */
	if (uc == 0 && lc == 0 && nc == 0 && oc == 0) { i = 5 ; }	/* String is blank */
	 else if (uc >= lc && uc >= nc && uc >= oc) { i = 1 ; }		/* String (mostly) uppercase */
	 else if (lc >= nc && lc >= oc) { i = 2 ; }			/* Lower case */
	 else if (nc >= oc) { i = 3 ; }					/* Numeric */
	 else i = 4 ;							/* Other */
	PUSHINT(i) ; return ;
}

/*	stru_updfix - Update to a fixed length string */

void stru_updfix()
{ struct str__ref sstr,dstr ;
   int slen,dlen,clen ;

/*	Pop off source string & get its length */
	stru_popstr(&sstr) ; slen = stru_sslen(&sstr) ;
/*	Now repeat for destination */
	stru_popstr(&dstr) ; dlen = dstr.format.fld.length ;
/*	How much to copy ? */
	clen = (dlen < slen ? dlen : slen) ; dlen -= clen ;
	memcpy(dstr.ptr,sstr.ptr,clen) ;
	memset(dstr.ptr+clen,' ',dlen) ;		/* Pad with spaces ? */
	return ;
}

/*	stru_updind - Updates an indirect string reference */

void stru_updind()
{
/*	For now, make same as stru_updfix */
	stru_updfix() ;
	return ;
}

/*	stru_updvar - Updates a variable length string */

void stru_updvar()
{ struct str__ref dstr,sstr ;
   int dlen,slen,clen ;

/*	Pop off the two strings	*/
	stru_popstr(&sstr) ; slen = stru_sslen(&sstr) ;
	stru_popstr(&dstr) ; dlen = dstr.format.fld.length-1 ;
/*	Figure out how much to copy - generate error if src too long */
	clen = (dlen < slen ? dlen : slen) ;
/*	Copy the string */
	if (dstr.ptr != sstr.ptr) strncpy(dstr.ptr,sstr.ptr,clen) ;
	*(dstr.ptr + clen) = NULLV ;
/*	Now maybe an error ? */
	if (slen > dlen)
	 v3_error(V3E_SRCLONGER,"STR","UPDVAR","SRCLONGER","Source string longer than destination",(void *)(slen-dlen)) ;
	return ;
}

/*	S T R I N G   F U N C T I O N   U T I L I T I E S	*/

/*	stru_alloc_bytes - Allocates bytes from circular buffer	*/
/*	  returns machine-pointer to begin of string			*/
char *stru_alloc_bytes(number)
  int number ;			/* Number of bytes to allocate */
{ 
 
	if (cbuf.index + number >= V3_STR_CIRCULAR_BUF_SIZE)
	 { cbuf.index = 0 ; } ;	/* Have to start around again */
	cbuf.start_index = cbuf.index ; cbuf.index += number ;
	return(&cbuf.buf[cbuf.start_index]) ;
}

/*	stru_csuc - Converts a C-format string to upper case */

void stru_csuc(sp)
 char *sp ;
{
	for (;*sp != NULLV;sp++)
	 { if (*sp >= 'a' && *sp <= 'z') { *sp -= 32 ; } else { if (*sp == '-') *sp = '_' ; } ; } ;
}

/*	stru_sslen - Returns length of string based on standard string descriptor (str__ref) */

int stru_sslen(sstr)
  struct str__ref *sstr ;
{
	if (sstr->format.fld.type == VFT_VARSTR) return(strlen(sstr->ptr)) ;
/*	Not variable length - return format length */
	return(sstr->format.fld.length) ;
}

/*	stru_cvtcstr - Converts standard string descriptor to C (null terminated) string */

char *stru_cvtcstr(cbuf,sstr)
  char *cbuf ;		/* Pointer to get C string */
  struct str__ref *sstr ;
{
	if (sstr->format.fld.type == VFT_VARSTR) { strcpy(cbuf,sstr->ptr) ; }
	 else { strncpy(cbuf,sstr->ptr,sstr->format.fld.length) ; *(cbuf + sstr->format.fld.length) = 0 ; } ;
	return(cbuf) ;
}

/*	stru_popcstr - Pops off string from value stack & converts to "C" language format  */
/*	  Returns: Pointer to updated string			*/

char *stru_popcstr(cstr)
  char *cstr ;
{ struct str__ref strref ;

	stru_popstr(&strref) ;
	switch (strref.format.fld.type)
	 { default: v3_error(V3E_NOTSTRING,"STR","POPCSTR","NOTSTRING","Expecting a string argument",(void *)strref.format.fld.type) ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		strncpy(cstr,strref.ptr,strref.format.fld.length) ;
		*(cstr+strref.format.fld.length) = NULLV ; break ;
	   case VFT_VARSTR:
		strcpy(cstr,strref.ptr) ; break ;
	 } ;
	return(cstr) ;
}

/*	stru_popstr - Pops off string from value stack & converts to standard string format	*/

void stru_popstr(sstr)
  struct str__ref *sstr ;		/* Updated with standard string format */
{ char *(*mp) ;
  struct db__formatptr *fw ;

	POPF(sstr->format.all) ;
/*	Is this an indirect string ? */
	if (sstr->format.fld.type != VFT_INDIRECT)
	 { POPVP(sstr->ptr,(char *)) ;
	   if (sstr->format.fld.type != VFT_STRUCTPTR) return ;
	   mp = (char *(*))sstr->ptr ; sstr->ptr = *mp ; return ;
	 } ;
/*	It is - then loc points to another string descriptor */
	POPVP(fw,(struct db__formatptr *)) ; sstr->format.all = fw->format.all ; sstr->ptr = fw->ptr ;
	return ;
}
