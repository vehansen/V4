/*	V3EDTU.C - XTI_XXX() AND ITX_XXX() MODULES FOR V3

	LAST EDITED 6/24/86 BY VICTOR E. HANSEN		*/

#include <time.h>
#include "v3defs.c"

/*	External linkages				*/
extern struct db__parse_info *parse ;
extern struct db__process *process ;
extern struct db__psi *psi ;


/*	I N T E G E R   E D I T I N G   R O U T I N E S		*/

/*	xti_int - Converts integer: External -> Internal	*/

xti_int()
{ struct str__ref inpstr ;	/* String ref to be edited */
   union flag__ref flags ;	/* Editing flags */
   struct num__ref anum ;		/* Describes destination number */
   union val__v3num *v3n ;
   char digits[50],dx=0 ;		/* Track digits for big decimal numbers */
   int i,minus=FALSE,plus=FALSE,radix=10,dps=0,havedp=FALSE,len ;
   B64INT result=0 ; char c ; char *errmne,*errmsg ; int errnum ;

#define INTERR(NUM,MNE,MSG) errnum = NUM ; errmne = MNE ; errmsg = MSG ; goto bad_character
/*	Get the arguments		*/
	flags.all = xctu_popint() ; xctu_popnum(&anum) ;stru_popstr(&inpstr) ;
/*	Figure out total length */
	len = (flags.fld.byte3 == 0 ? stru_sslen(&inpstr) : flags.fld.byte3) ;
/*	Non decimal radix ? */
	if (flags.fld.mask & V3_FLAGS_EDT_HEX) radix = 16 ;
	if (flags.fld.mask & V3_FLAGS_EDT_OCTAL) radix = 8 ;
/*	Loop thru all bytes & parse */
	for (i=0;i<len;i++)
	 { c = *(inpstr.ptr+i) ;
	   if (c == process->ci.DecimalPlaceDelim) c = '.' ;
	   if (c == process->ci.NegNumPrefix) c = '-' ;
	   if (c == process->ci.NumericFldDelim) c = ',' ;
           switch (c)
	    { default:	/* Here on bad byte */
		/* v3_error(V3E_INVNUMCHAR,"XTI","NUM","INVNUMCHAR","Invalid character in number",0) ; */
		INTERR(V3E_INVNUMCHAR,"INVNUMCHAR","Invalid character in number") ;
	      case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
		/* v3_error(V3E_INVCHRRADIX,"XTI","NUM","INVCHRRADIX","Invalid digit for specified radix",0) ; */
		if ((c -= '0') >= radix) { INTERR(V3E_INVCHRRADIX,"INVCHRRADIX","Invalid digit for specified radix") ; } ;
		if (havedp) dps += 1 ;
		digits[dx++] = c ;
		result *= radix ; result += c ; break ;
	      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		/* v3_error(V3E_INVNUMCHAR,"XTI","NUM","INVNUMCHAR","Invalid character in number",0) ; */
		if (radix != 16) { INTERR(V3E_INVNUMCHAR,"INVNUMCHAR","Invalid character in number") ; } ;
		result <<= 4 ; result |= c-'A'+10 ; break ;
	      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		/* v3_error(V3E_INVNUMCHAR,"XTI","NUM","INVNUMCHAR","Invalid character in number",0) ; */
		if (radix != 16) { INTERR(V3E_INVNUMCHAR,"INVNUMCHAR","Invalid character in number") ; } ;
		result <<= 4 ; result |= c-'a'+10 ; break ;
	      case ' ': case '\t': case ',': case '$':
		/* v3_error(V3E_INVNUMCHAR,"XTI","NUM","INVNUMCHAR","Invalid character in number",0) ; */
		if ((flags.fld.mask & V3_FLAGS_EDT_PUNCTUATION) == 0)
		 { INTERR(V3E_INVNUMCHAR,"INVNUMCHAR","Invalid character in number") ; } ;
		break ;
	      case '.':
		/* v3_error(V3E_NODP,"XTI","NUM","NODP","Single decimal point allowed on decimal numbers",0) ; */
		if (radix != 10 || havedp > 0)
		 { INTERR(V3E_NODP,"NODP","Single decimal point allowed on decimal numbers") ; } ;
		havedp = TRUE ; break ;
	      case '-':
		/* v3_error(V3E_ONEMINUS,"XTI","NUM","ONEMINUS","Only one sign allowed",0) ; */
		if (result != 0 || minus || plus) { INTERR(V3E_ONEMINUS,"ONEMINUS","Only one sign allowed") ; } ;
		/* v3_error(V3E_NOTPOS,"XTI","NUM","NOTPOS","Numeric must be positive",0) ; */
		if ((flags.fld.mask & V3_FLAGS_EDT_POSITIVE) != 0)
		 { INTERR(V3E_NOTPOS,"NOTPOS","Numeric must be positive") ; } ;
		minus = TRUE ; break ;
	      case '+':
		/* v3_error(V3E_ONEPLUS,"XTI","NUM","ONEPLUS","Only one sign allowed",0) ; */
		if (result != 0 || minus || plus) { INTERR(V3E_ONEPLUS,"ONEPLUS","Only one sign allowed") ; } ;
		plus = TRUE ; break ;
	    } ;
	 } ;
/*	Here then all done - see if we have to scale result */
all_done:
/*	Update result & return with length */
	if (anum.format.fld.type == VFT_V3NUM)
	 { v3n = (union val__v3num *)anum.ptr ;		/* Link up to the v3 number */
	   memset(&digits[dx],0,9) ;			/* Make sure we got plenty of padding */
	   v3n->fd.big = 0 ; v3n->fd.num = 0 ; v3n->fd.dps = 0 ;
	   for(i=dx-dps;i<=dx-dps+8;i++) { v3n->fd.dps *= 10 ; v3n->fd.dps += digits[i] ; } ;
	   if (dx-dps-10 >= 0)
	    { i = dx-dps-18 ; if (i<0) i = 0 ;
	      for(;i<=dx-dps-10;i++) { v3n->fd.big *= 10 ; v3n->fd.big += digits[i] ; } ;
	    } ;
	   if (dx-dps > 0)
	    { i = dx-dps-9 ; if (i<0) i = 0 ;
	      for(;i<dx-dps;i++) { v3n->fd.num *= 10 ; v3n->fd.num += digits[i] ; } ;
	    } ;
	   if (minus) { v3n->fd.big = -v3n->fd.big ; v3n->fd.num = -v3n->fd.num ; v3n->fd.dps = -v3n->fd.dps ; } ;
	 } else if (anum.format.all != V3_FORMAT_NULL) xctu_num_upd((minus ? -result : result),dps,&anum) ;
	return(len) ;
/*	Here on bad character */
bad_character:
	if ((flags.fld.mask & V3_FLAGS_EDT_ZERO_IF_ERROR) != 0)
	 { xctu_num_upd((B64INT)0,0,&anum) ; return(len) ; } ;
	if ((flags.fld.mask & V3_FLAGS_EDT_UNTIL_DELIMITER) != 0)
	 { len = i ; goto all_done ; } ;
/*	If got "NO_ERROR" flag then return - length else generate V3 error */
	if (flags.fld.mask & V3_FLAGS_EDT_NOERROR) return(-i) ;
	v3_error(errnum,"XTI","NUM",errmne,errmsg,0) ;
	return(0) ;
}

/*	itx_int - Converts integer: Internal -> External	*/

itx_int()
{ union flag__ref flags ;
   struct db__command_search_list clist ;	/* Command search list in case of field overflow */
   struct db__xct_cmd_info cinfo ;
   struct str__ref xstr ; int xlen ;
   union val__format format ;
   struct num__ref anum ;
   union val__v3num v3n ;
   int sign,commas,dps ;
   char str[200],dbuf[5],*cp,*sptr,*lineptr,*ivp ; int strex,str_len ;
   int signflag ; int i ; char fill ;

/*	Pick up arguments */
	flags.all = xctu_popint() ; stru_popstr(&xstr) ;
/*	Need value & format of first argument */
	POPF(format.all) ; PUSHF(format.all) ;
/*	See if we got a report line */
	if (((*xstr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
	 { lineptr = xstr.ptr ;
	   switch (rptu_colpos(&xstr,&xlen,flags.fld.byte3,(format.fld.type == VFT_V3NUM || format.fld.type == VFT_FLOAT ? V3_COLTYP_REAL : V3_COLTYP_INT)))
	    { case 0:	break ;	/* Fall thru for normal report */
	      case 1:	flags.all &= ~(V3_FLAGS_EDT_COMMAS | V3_FLAGS_EDT_FLOATING_DOLLAR | V3_FLAGS_EDT_RIGHT_HAND_SIGN
					| V3_FLAGS_EDT_ASTERISK_FILL | V3_FLAGS_EDT_PAREN_IF_NEG | V3_FLAGS_EDT_CENTER) ;
			break ;
	    } ;
	 }
	 else { if ( xstr.format.fld.type == VFT_VARSTR)
		 { flags.all |= V3_FLAGS_EDT_CONCAT ; xlen = (flags.fld.byte3 > 0 ? flags.fld.byte3 : 0) ; }
		 else { xlen = stru_sslen(&xstr) ; if (flags.fld.byte3 > 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
		lineptr = NULLV ;
	      } ;
	if (flags.fld.byte2 == 0)
	 { if (format.fld.type == VFT_V3NUM || format.fld.type == VFT_FLOAT)
	    { flags.fld.byte2 = (format.fld.decimals == 0 ? 8 : format.fld.decimals) ;
	    } else flags.fld.byte2 = format.fld.decimals ;
	 } ;
	v3num_popv3n(&v3n) ; ivp = (char *)v3num_cvtcb(&v3n,flags.fld.byte2,&signflag) ;
/*	Maybe try & get rid of trailing 0's */
	if (flags.fld.byte2 == 8)
	 { cp = ivp ;
	   for(;flags.fld.byte2>1 && *cp == '0';) { cp-- ; flags.fld.byte2-- ; } ;
	   ivp = cp ;
	 } ;
/*	Set up some pointers, etc. */
	sign = FALSE ; sptr = &str[strex=180] ; str[180] = (str[181] = 0) ;
/*	Check for special edits (hex & octal) */
	if (flags.fld.mask & (V3_FLAGS_EDT_OCTAL | V3_FLAGS_EDT_HEX))
	 { REPUSH ;
	   sprintf(sptr,(flags.fld.mask & V3_FLAGS_EDT_OCTAL ? "%o" : "%X"),xctu_popint()) ;
	   goto got_number ;
	 } ;
/*	See if ivp is zero */
	if (ivp == NULLV)
	 {
	   dbuf[0] = 0 ; ivp = &dbuf[0] ;
	   if (flags.fld.mask & V3_FLAGS_EDT_DASH_IF_ZERO)
	    { *(--sptr) = process->ci.DashIfZero ; flags.fld.mask &= ~V3_FLAGS_EDT_FLOATING_DOLLAR ; goto got_number ; } ;
	   if (flags.fld.mask & V3_FLAGS_EDT_BLANK_IF_ZERO)
	    { *(--sptr) = ' ' ; flags.fld.mask &= ~V3_FLAGS_EDT_FLOATING_DOLLAR ; goto got_number ; } ;
	 } ;
/*	Test for negative number */
	if (signflag < 0) sign = TRUE ;
/*	Now convert number to character string */
	commas = (flags.fld.mask & V3_FLAGS_EDT_COMMAS ? 3+flags.fld.byte2 : 0) ;
	dps = flags.fld.byte2 ;
	for (cp=ivp;*cp != 0;)
	 { *(--sptr) = *(cp--) ;
	   if (--dps == 0) *(--sptr) = process->ci.DecimalPlaceDelim ;
	   if (--commas == 0) { if (*cp == 0) break ; commas = 3 ; *(--sptr) = process->ci.NumericFldDelim ; } ;
	 } ;
/*	Fill in mandatory zeros */
	for(;dps>=0;)
	 { *(--sptr) = '0' ;
	   if (--dps == 0) *(--sptr) = process->ci.DecimalPlaceDelim ;
	 } ;
/*	When here then have number as string - fill in sign */
got_number:
	if (flags.fld.mask & V3_FLAGS_EDT_FLOATING_DOLLAR && process->ci.LeadingDollar[0] == ' ')
	 { for(i=1;i<strlen(process->ci.LeadingDollar);i++)
	    { str[strex++] = process->ci.LeadingDollar[i] ; } ;
	 } ;
	if (sign)
	 { if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_HAND_SIGN) { str[strex++] = process->ci.NegNumSuffix ; }
	    else { if (flags.fld.mask & V3_FLAGS_EDT_PAREN_IF_NEG)  str[strex++] = process->ci.NegNumRight ; } ;
	 } else
	 { if (flags.fld.mask & (V3_FLAGS_EDT_RIGHT_HAND_SIGN | V3_FLAGS_EDT_PAREN_IF_NEG))
	    { str[strex++] = ' ' ; } ;
	 } ;
/*	Now try for leading fill */
	if ((flags.fld.mask & (V3_FLAGS_EDT_CENTER | V3_FLAGS_EDT_LEFT_JUSTIFY)) == 0)
	 { i = xlen - strlen(sptr) ;	/* i = number of leading places */
	   if (flags.fld.mask & V3_FLAGS_EDT_FLOATING_DOLLAR) i-- ;
	   if (sign && ( (flags.fld.mask & V3_FLAGS_EDT_RIGHT_HAND_SIGN) == 0)) i-- ;
	   if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL) { fill = '0' ; }
	    else { if (flags.fld.mask & V3_FLAGS_EDT_ASTERISK_FILL) { fill = process->ci.NumFill ; }
		    else { goto no_fill_yet ; } ;
		 } ;
	   for (;i>0;i--) *(--sptr) = fill ;
	 } ;
no_fill_yet:
/*	Now try for sign & floating dollar sign */
	if (sign)
	 { if (flags.fld.mask & V3_FLAGS_EDT_PAREN_IF_NEG) { *(--sptr) = process->ci.NegNumLeft ; }
	    else { if ((flags.fld.mask & V3_FLAGS_EDT_RIGHT_HAND_SIGN) == 0) *(--sptr) = process->ci.NegNumSuffix ; } ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_FLOATING_DOLLAR && process->ci.LeadingDollar[0] != ' ')
	 { for(i=strlen(process->ci.LeadingDollar);i>0;i--)
	    { *(--sptr) = process->ci.LeadingDollar[i-1] ; } ;
	 } ;
/*	If xlen is zero then update with actual length & update rpt stuff */
	str_len = strlen(sptr) ;
	if (xlen == 0)
	 { xlen = str_len ; if (lineptr != NULLV) rptu_lineupd((struct rpt__line *)lineptr,xlen) ; } ;
/*	Is the result too big ? */
	if (strlen(sptr) > xlen)
	 { sptr = str ; for (i=0;i<xlen;i++) str[i] = process->ci.NumOverflow ; str[xlen] = NULLV ; str_len = strlen(sptr) ;
	   process->asterisk_count ++ ; /* Just increment count of times this happened */
	 } ;
/*	Whew, place result in xstr */
prx:
	if ((flags.fld.mask & (V3_FLAGS_EDT_LEFT_JUSTIFY | V3_FLAGS_EDT_CENTER)) == 0)
	 flags.all |= V3_FLAGS_EDT_RIGHT_JUSTIFY ;
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (xstr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","ALPHA","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)xstr.format.fld.type) ;
	   xlen = strlen(xstr.ptr) ;
	   if (str_len + xlen >= xstr.format.fld.length)
	    v3_error(V3E_TOOLONG,"XTI","ALPHA","TOOLONG","String to long to \'/concat/\'",(void *)str_len) ;
	   strncat(xstr.ptr,sptr,str_len) ; *(xstr.ptr+xlen+str_len) = NULLV ;
	   return(xlen+str_len) ;
	 } ;
/*	See if we are to right justify */
	if (str_len > xlen) str_len = xlen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { xstr.ptr += (xlen-str_len) ; }
	    else
	    { for (i=0;i<xlen-str_len;i++) { *(xstr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(xstr.ptr,sptr,str_len) ;
	   return(xlen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { xstr.ptr += (xlen-str_len)/2 ; }
	    else
	    { for (i=0;i<(xlen-str_len)/2;i++) { *(xstr.ptr++) = ' ' ; } ; } ;
	   memcpy(xstr.ptr,sptr,str_len) ; xstr.ptr += str_len ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<xlen-((xlen-str_len)/2+str_len);i++) *(xstr.ptr++) = ' ' ;
	    } ;
	   return(xlen) ;
	 } ;
/*	If here then left justify */
	memcpy(xstr.ptr,sptr,str_len) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += str_len ;
	   for (i=0;i<xlen-str_len;i++) *(xstr.ptr++) = ' ' ;
	 } ;
	return(xlen) ;
}

/*	A L P H A   E D I T I N G   R O U T I N E S		*/

/*	xti_alpha - Edits an alpha string			*/

xti_alpha(itx_xti_flag)
  int itx_xti_flag ;		/* TRUE if xti_alpha, FALSE if itx_alpha */
{ struct str__ref xstr,istr ;	/* External/Internal string refs */
   union flag__ref flags ;
   int i,nq,xlen,ilen,spreadsheet ; char *tptr,*lineptr ;
   char tbuf[V3_LITERAL_STRING_MAX] ; /* Temp buffer for upper case */

/*	Pick up arguments */
	flags.all = xctu_popint() ; stru_popstr(&istr) ; stru_popstr(&xstr) ; spreadsheet = FALSE ;
	if (istr.ptr == 0) { istr.ptr = tbuf ; tbuf[0] = NULLV ; } ;
	if (itx_xti_flag) *istr.ptr = NULLV ; /* If xti_alpha then zap first byte (variable length string and -1 for reports) */
/*	Check for a varible length string */
	if (istr.format.fld.type == VFT_VARSTR)
	 { flags.all |= V3_FLAGS_EDT_CONCAT ;
	 } ;
/*	Check for report line */
	if (((*istr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
/*	if ((*istr.ptr == -1) && (flags.fld.byte3 > 0)) */
	 { lineptr = istr.ptr ;
	   if (rptu_colpos(&istr,&ilen,flags.fld.byte3,V3_COLTYP_ALPHA) > 0) spreadsheet = TRUE ;
/*	   If we got /EXTEND/ then extend report column for length of the input string */
	   if (flags.fld.mask & V3_FLAGS_EDT_EXTEND) ilen = stru_sslen(&xstr) ;
	 }
	 else { lineptr = NULLV ; ilen = stru_sslen(&istr) ;
		if (flags.fld.byte3 != 0 && flags.fld.byte3 < ilen) ilen = flags.fld.byte3 ; } ;
	xlen = stru_sslen(&xstr) ;
/*	Do we have to do any trimming ? */
	if (flags.fld.mask & V3_FLAGS_EDT_TRIM_LEFT)
	 { while (xlen > 0 && (*xstr.ptr == ' ' || *xstr.ptr == '\t')) { xstr.ptr++ ; xlen-- ; } ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_TRIM_RIGHT)
	 { tptr = xstr.ptr + xlen - 1 ;
	   while (xlen > 0 && (*tptr == ' ' || *tptr == '\t')) { tptr-- ; xlen-- ; } ;
	 } ;
/*	How about convert to upper case */
	if (flags.fld.mask & V3_FLAGS_EDT_UPPER_CASE)
	 { tptr = xstr.ptr ; xstr.ptr = tbuf ;
	   for (i=0;i<xlen;(i++,tptr++,xstr.ptr++))
	    { *xstr.ptr = (*tptr >= 'a' && *tptr <= 'z') ? *tptr-32 : (*tptr == '-' ? '_' : *tptr) ; } ;
	   xstr.ptr = tbuf ;	/* Use temp buf from now on */
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_QUOTED)
	 { tptr = xstr.ptr ; xstr.ptr = &tbuf[1] ;
	   for(i=0,nq=FALSE;i<xlen;i++,tptr++)
	    { switch (*tptr)
	       { default:
			*(xstr.ptr++) = *tptr ;
			if (*tptr >= 'A' && *tptr <= 'Z') break ;
			if (*tptr == '_') break ;
			nq = TRUE ; break ;
		 case 10:	*(xstr.ptr++) = '\\' ; *(xstr.ptr++) = 'l' ; nq = TRUE ; break ;
		 case 13:	*(xstr.ptr++) = '\\' ; *(xstr.ptr++) = 'r' ; nq = TRUE ; break ;
		 case '\'': case '\\':
		 case '"':	*(xstr.ptr++) = '\\' ; *(xstr.ptr++) = *tptr ; nq = TRUE ; break ;
	       } ;
	    } ;
	   if (nq) { *(xstr.ptr++) = '"' ; *(xstr.ptr++) = 0 ; tbuf[0] = '"' ; xstr.ptr = tbuf ; }
	    else { *(xstr.ptr++) = 0 ; xstr.ptr = &tbuf[1] ; } ;
	   xlen = strlen(xstr.ptr) ;		/* Get new length */
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_UPPER_CASE_ALT)
	 { tptr = xstr.ptr ; xstr.ptr = tbuf ;
	   for (i=0;i<xlen;(i++,tptr++,xstr.ptr++))
	    { *xstr.ptr = (*tptr >= 'a' && *tptr <= 'z') ? *tptr-32 : *tptr ; } ;
	   xstr.ptr = tbuf ;	/* Use temp buf from now on */
	 } ;
/*	How about convert to lower case */
	if (flags.fld.mask & V3_FLAGS_EDT_LOWER_CASE)
	 { tptr = xstr.ptr ; xstr.ptr = tbuf ;
	   for (i=0;i<xlen;(i++,tptr++,xstr.ptr++))
	    { *xstr.ptr = (*tptr >= 'A' && *tptr <= 'Z') ? *tptr+32 : *tptr ; } ;
	   xstr.ptr = tbuf ;	/* Use temp buf from now on */
	 } ;
/*	How about to a spreadsheet ? */
	if (spreadsheet)
	 { tptr = xstr.ptr ; xstr.ptr = tbuf ;
	   for (i=0;i<xlen;(i++,tptr++,xstr.ptr++)) { *xstr.ptr = ((*tptr == '"') ? '\'' : *tptr) ; } ;
	   xstr.ptr = tbuf ;	/* Use temp buffer from now on */
	 } ;
/*	How about symbol normalize */
	if (flags.fld.mask & V3_FLAGS_EDT_NORMALIZE)
	 { tptr = xstr.ptr ; xstr.ptr = tbuf ;
/*	   Scan "symbol" and convert to upper case, "-" to "_", otherwise error */
	   for (i=0;i<xlen;(i++,tptr++,xstr.ptr++))
	    { if (*tptr >= 'A' && *tptr <= 'Z') { *xstr.ptr = *tptr ; continue ; } ;
	      if ((*tptr >= '0' && *tptr <= '9') || *tptr == '_') { *xstr.ptr = *tptr ; continue ; } ;
	      if (*tptr == '*' || *tptr == '$') { *xstr.ptr = *tptr ; continue ; } ;
	      if (*tptr >= 'a' && *tptr <= 'z') { *xstr.ptr = *tptr-32 ; continue ; } ;
	      if (*tptr == '-' || *tptr == ' ') { *xstr.ptr = '_' ; continue ; } ;
	      v3_error(V3E_INVSYM,"XTI","ALPHA","INVSYM","Invalid character in symbol",(void *)*tptr) ;
	    } ;
	   xstr.ptr = tbuf ;	/* Use temp buf from now on */
	 } ;
/*	If result is "nil" then quit now */
	if (istr.ptr == tbuf) return(xlen) ;
/*	Now see if results are to be concatenated to istr */
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (istr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","ALPHA","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)istr.format.fld.type) ;
	   if (xlen + ilen >= istr.format.fld.length)
	    v3_error(V3E_TOOLONG,"XTI","ALPHA","TOOLONG","String to long to \'/concat/\'",(void *)xlen) ;
	   strncat(istr.ptr,xstr.ptr,xlen) ; *(istr.ptr+ilen+xlen) = NULLV ;
	   return(ilen+xlen) ;
	 } ;
/*	Check for report line */
	if (ilen == 0)
	 { ilen = xlen ; if (lineptr != NULLV) rptu_lineupd((struct rpt__line *)lineptr,ilen) ; } ;
/*	See if we are to right justify */
	if (xlen > ilen) xlen = ilen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { istr.ptr += (ilen-xlen) ; }
	    else
	    { for (i=0;i<ilen-xlen;i++) { *(istr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(istr.ptr,xstr.ptr,xlen) ;
	   return(ilen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { istr.ptr += (ilen-xlen)/2 ; }
	    else
	    { for (i=0;i<(ilen-xlen)/2;i++) { *(istr.ptr++) = ' ' ; } ; } ;
	   memcpy(istr.ptr,xstr.ptr,xlen) ; istr.ptr += xlen ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<ilen-((ilen-xlen)/2+xlen);i++) *(istr.ptr++) = ' ' ;
	    } ;
	   return(ilen) ;
	 } ;
/*	If here then left justify */
	memcpy(istr.ptr,xstr.ptr,xlen) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { istr.ptr += xlen ;
	   for (i=0;i<ilen-xlen;i++) *(istr.ptr++) = ' ' ;
	 } ;
	return(ilen) ;
}

/*	D A T E   E D I T I N G   R O U T I N E S		*/

/*	xti_date - Converts string to internal date format	*/

static struct str__ref xstr ;	/* External string reference */
static int xlen ;		/* External string length */

char days_in_month[12] =
 { 31,28,31,30,31,30,31,31,30,31,30,31 } ;

extern short int julian[] ;
extern short int julian_leap[] ;

xti_date()
{ union flag__ref flags ;		/* Edit flags */
   struct num__ref anum ;	/* Describes dst variable */
   int century_and_year,year,month,day ; char tmonth[4] ;
   B64INT i ;
   int reslen ;
   time_t tt ;
   char *errmne,*errmsg ; int errnum ;

#define DATERR(NUM,MNE,MSG) errnum = NUM ; errmne = MNE ; errmsg = MSG ; goto bad_date

/*	Pick up the arguments			*/
	flags.all = xctu_popint() ; xctu_popnum(&anum) ; stru_popstr(&xstr) ; xlen = stru_sslen(&xstr) ;
	if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ;
	reslen = xlen ;		/* Save as result length */
/*	If /punctuation/ then strip off leading spaces */
	if (flags.fld.mask & V3_FLAGS_EDT_PUNCTUATION)
	 { while (*xstr.ptr == ' ' || *xstr.ptr == '\t')
	    { xstr.ptr++ ; if (--xlen <= 0) break ; } ;
	 } ;
/*	Do we have enough for valid date ? */
	if (strncmp(process->ci.yesterday,xstr.ptr,xlen)*strncmp(process->ci.YESTERDAY,xstr.ptr,xlen) == 0)
	 { if (anum.format.all != V3_FORMAT_NULL)
	    { tt = time(NULL)-(60*mscu_minutes_west()) ; i = TIMEOFFSETDAY + tt/(60*60*24) ;
	      xctu_num_upd((B64INT)(i-1),0,&anum) ; return(xlen) ;
	    } ;
	 } ;
	if (strncmp(process->ci.tomorrow,xstr.ptr,xlen)*strncmp(process->ci.TOMORROW,xstr.ptr,xlen) == 0)
	 { if (anum.format.all != V3_FORMAT_NULL)
	    { tt = time(NULL)-(60*mscu_minutes_west()) ; i = TIMEOFFSETDAY + tt/(60*60*24) ;
	      xctu_num_upd((B64INT)(i+1),0,&anum) ; return(xlen) ;
	    } ;
	 } ;
	if (xlen < 6)
/*	   Look for special keywords "none" & "today" */
	 { if (strncmp(process->ci.none,xstr.ptr,xlen)*strncmp(process->ci.NONE,xstr.ptr,xlen) == 0)
	    { if (anum.format.all != V3_FORMAT_NULL) xctu_num_upd((B64INT)0,0,&anum) ; return(xlen) ; } ;
	   if (strncmp(process->ci.today,xstr.ptr,xlen)*strncmp(process->ci.TODAY,xstr.ptr,xlen) == 0)
	    { if (anum.format.all != V3_FORMAT_NULL)
	       { tt = time(NULL)-(60*mscu_minutes_west()) ; i = TIMEOFFSETDAY + tt/(60*60*24) ;
	         xctu_num_upd((B64INT)(i),0,&anum) ;
	       } ; return(xlen) ;
	    } ;
	   /* v3_error(V3E_DATETOOSHORT,"XTI","DATE","DATETOOSHORT","Invalid date format") ; */
	   DATERR(V3E_DATETOOSHORT,"DATETOOSHORT","Invalid date format") ;
	 } ;
/*	See if "mm/dd/yy" */
	if (*(xstr.ptr+1) == '/' || *(xstr.ptr+2) == '/')
	 { month = xti_date_num() ;
	   /* v3_error(V3E_BADMONTH,"XTI","DATE","BADMONTH","Invalid month",0) ; */
	   if (*(xstr.ptr++) != '/') { DATERR(V3E_BADMONTH,"BADMONTH","Invalid month") ; } ;
	   day = xti_date_num() ;
	   /* v3_error(V3E_BADDAY,"XTI","DATE","BADDAY","Invalid day",0) ; */
	   if (*(xstr.ptr++) != '/') { DATERR(V3E_BADDAY,"BADDAY","Invalid day") ; } ;
	   year = xti_date_num() ; goto end_date_parse ;
	 } ;
/*	See if "dd-mmm-yy" */
	if (*(xstr.ptr+1) == '-' || *(xstr.ptr+2) == '-')
	 { day = xti_date_num() ; xstr.ptr++ ;
/*	   Convert month to upper case */
	   for (i=0;i<3;i++) { tmonth[i] = toupper(*(xstr.ptr++)) ; } ;
/*	   Search for month in list */
	   xlen -= 4 ;
	   if (*(xstr.ptr++) != '-') { DATERR(V3E_BADDAY,"BADDAY","Invalid day") ; } ;
	   for (month=0;month<12;month++)
	    { if (strncmp(tmonth,process->ci.ucMonths[month],3) == 0) goto found_month ; } ;
	   DATERR(V3E_BADMONTH,"BADMONTH","Invalid month") ;
found_month:
	   month += 1 ; year = xti_date_num() ;
	   goto end_date_parse ;
	 } ;
/*	If here then must be date of form: "mmddyy" */
	month = xti_date_num() ; year = month % 100 ; day = (month/100) % 100 ; month /= 10000 ;
/*	Got year, month & day as numbers - check out */
end_date_parse:
	if (month < 1 || month > 12) { DATERR(V3E_INVMONTH,"INVMONTH","Month must be between 1 and 12") ; } ;
	if (day < 1 || day > days_in_month[month-1])
	 { if (!(month == 2 && day == 29 && (year%4) == 0))
	    { DATERR(V3E_INVDAYOFMTH,"INVDAYOFMTH","Day of month is not valid for specified month") ; } ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_HISTORY)
	 { int udate, uToday ;
	   uToday = TIMEOFFSETDAY + (time(NULL)-(60*mscu_minutes_west()))/(60*60*24) ;
	   century_and_year = (year < 100 ? year + 2000 : year) ;
	   if (century_and_year < 1850 || century_and_year > 2130) { DATERR(V3E_BADYEAR,"BADYEAR","Year must be between 1850 & 2130") ; } ;
	   udate = mscu_ymd_to_ud(century_and_year,month,day) ;
	   if (udate > uToday)
	    { century_and_year = (year < 100 ? year + 1900 : year) ;
	      udate = mscu_ymd_to_ud(century_and_year,month,day) ;
	      if (udate > uToday) { DATERR(V3E_BADYEAR,"BADYEAR","Date cannot be in the future") ; } ;
	    } ;
	 } else
	 { century_and_year = (year < 100 ? year + 2000 : year) ;
	 } ;
	if (century_and_year < 1850 || century_and_year > 2130) { DATERR(V3E_BADYEAR,"BADYEAR","Year must be between 1850 & 2130") ; } ;
//	if (year < 100) { if (year < 20) { year += 2000 ; } else { year += 1900 ; } ; } ;
/*	If result is "nil" then quit now */
	if (anum.format.all == V3_FORMAT_NULL) return(reslen) ;
/*	Looks like valid date - see what kind of result to return */
	if ((flags.fld.mask & V3_FLAGS_EDT_CENTURY) == 0) year = year % 100 ;
/*	Return "YYMMDD" ? */
	if (flags.fld.mask & V3_FLAGS_EDT_YYMMDD)
	 { if ((flags.fld.mask & V3_FLAGS_EDT_CENTURY) == 0) century_and_year = century_and_year % 100 ;
	   xctu_num_upd((B64INT)(century_and_year*10000 + month*100 + day),0,&anum) ; return(reslen) ;
	 } ;
/*	Return julian ? */
	if (flags.fld.mask & V3_FLAGS_EDT_JULIAN)
	 { if ((flags.fld.mask & V3_FLAGS_EDT_CENTURY) == 0) century_and_year = century_and_year % 100 ;
	   if ((year % 4) == 0 && month > 2) day++ ;
	   xctu_num_upd((B64INT)(century_and_year*1000 + julian[month-1] + day),0,&anum) ;
	   return(reslen) ;
	 } ;
/*	Must want universal */
	xctu_num_upd((B64INT)mscu_ymd_to_ud(century_and_year,month,day),0,&anum) ;
	return(reslen) ;
/*	Here to handle bad date */
bad_date:
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_IF_ERROR)
	 { xctu_num_upd((B64INT)0,0,&anum) ; return(reslen) ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_NOERROR) return(0) ;
/*	Generate a V3 error */
	v3_error(errnum,"XTI","DATE",errmne,errmsg,0) ;
	return(0) ;				/* Won't get here - just to make C compiler happy */
}

/*	Here to parse numbers in dates	*/
xti_date_num()
{ int res=0,digit ;
	for (;(xlen--) > 0;xstr.ptr++)
	 { digit = *xstr.ptr - '0' ;
	   if (digit < 0 || digit > 9) break ;
	   res *= 10 ; res += digit ;
	 } ;
	return(res) ;
}

/*	itx_date - Converts internal date to string form	*/

itx_date()
{ struct str__ref xstr ;
   union flag__ref flags ;
   int i,xlen,datestr_len,month,day,year,century_and_year ;
   int id,ud ;
   char datestr[100],*lineptr ;

/*	Pick up the arguments					*/
	flags.all = xctu_popint() ; stru_popstr(&xstr) ; id = xctu_popint() ;
	flags.fld.mask |= process->ci.DefaultDateMask ;
/*	Check for report line */
	if (((*xstr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
	 { lineptr = xstr.ptr ; rptu_colpos(&xstr,&xlen,flags.fld.byte3,V3_COLTYP_DATE) ; }
	 else { lineptr = NULLV ;
		if (xstr.format.fld.type == VFT_VARSTR)
		 { if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
		    { xlen = xstr.format.fld.length - stru_sslen(&xstr) - 1 ; }
		    else { xlen = xstr.format.fld.length - 1 ; *xstr.ptr = NULLV ; flags.fld.mask |= V3_FLAGS_EDT_CONCAT ; } ;
		 } else xlen = xstr.format.fld.length ;
		if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
	if (id == 0) { datestr[0] = NULLV ; goto got_string ; } ;
/*	Convert all dates to month, day, and year */
	if (flags.fld.mask & V3_FLAGS_EDT_YYMMDD)
	 { year = id/10000 ; month = (id/100) % 100 ; day = id % 100 ; goto got_mdy ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_JULIAN)
	 { julian_entry:
	   year = id/1000 ; id %= 1000 ;
	   if ((year % 4) == 0)
	    { for (month=1;month<12;month++)
	       { if (id <= julian_leap[month]) break ; } ;
	      day = id-julian_leap[month-1] ; goto got_mdy ;
	    } else
	    { for (month=1;month<12;month++)
	       { if (id <= julian[month]) break ; } ;
	      day = id-julian[month-1] ; goto got_mdy ;
	    } ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_DATE_TIME) id = id / 86400 + 44240 ;
/* 	Must be universal - convert it */
	if (id < 0 || id > 70200)
	 v3_error(V3E_INVUD,"ITX","DATE","INVUD","Invalid universal date",(void *)id) ;
	year = ((id+678955)*4)/1460 ;
	for (;;)
	 { ud = mscu_ymd_to_ud(year,1,1) ;
	   if (ud > id) { year-- ; continue ; } ;
	   if (id - ud < ((year % 4) == 0 ? 366 : 365)) break ;
	   year-- ;
	 } ;
/*	Fake it as julian */
	id = year*1000 + id-ud + 1 ; goto julian_entry ;

/*	We got year, month & day - see if we need century */
got_mdy:
	if (year < 100) { year += 2000 ; } ;
	century_and_year = year ; if ((flags.fld.mask & V3_FLAGS_EDT_CENTURY) == 0) year %= 100 ;
/*	Now convert to string (datestr)	*/
	if (flags.fld.mask & V3_FLAGS_EDT_DDMMYY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	    { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(datestr,"%02d/%02d",day,month) ; }
		else sprintf(datestr,"%02d/%02d/%02d",day,month,year) ; }
	    else { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(datestr,"%d/%d",day,month) ; }
		     else sprintf(datestr,"%d/%d/%02d",day,month,year) ; } ;
	   goto got_string ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_MDY)
	 { sprintf(datestr,"%02d%02d%02d",month,day,year) ; goto got_string ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_MMDDYY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	    { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(datestr,"%02d/%02d",month,day) ; }
		else sprintf(datestr,"%02d/%02d/%02d",month,day,year) ; }
	    else { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(datestr,"%d/%d",month,day) ; }
		     else sprintf(datestr,"%d/%d/%02d",month,day,year) ; } ;
	   goto got_string ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_LONG_DATE)
	 { sprintf(datestr,"%s %d, %d",&process->ci.LongMonths[month-1],day,century_and_year) ;
	   goto got_string ;
	 } ;
/*	By default, convert to dd-mmm-yy */
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(datestr,"%02d-%s",day,process->ci.Months[month-1]) ; }
	    else sprintf(datestr,"%02d-%s-%02d",day,process->ci.Months[month-1],year) ; }
	 else { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(datestr,"%d-%s",day,process->ci.Months[month-1]) ; }
		 else sprintf(datestr,"%d-%s-%02d",day,process->ci.Months[month-1],year) ; } ;
	goto got_string ;
/*	Here with resulting string - update second argument */
got_string:
	datestr_len = strlen(datestr) ;
/*	Check for a report line */
	if (xlen == 0)
	 { xlen = datestr_len ; if (lineptr != NULLV) rptu_lineupd((struct rpt__line *)lineptr,xlen) ; } ;
/*	Now see if results are to be concatenated to istr */
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (xstr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","ALPHA","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)xstr.format.fld.type) ;
	   if (datestr_len > xlen)
	    v3_error(V3E_TOOLONG,"XTI","ALPHA","TOOLONG","String to long to \'/concat/\'",(void *)datestr_len) ;
	   strncat(xstr.ptr,datestr,datestr_len) ; *(xstr.ptr+(i = strlen(xstr.ptr))) = NULLV ;
	   return(i) ;
	 } ;
/*	See if we are to right justify */
	if (datestr_len > xlen) datestr_len = xlen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { xstr.ptr += (xlen-datestr_len) ; }
	    else
	    { for (i=0;i<xlen-datestr_len;i++) { *(xstr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(xstr.ptr,datestr,datestr_len) ;
	   return(xlen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { xstr.ptr += (xlen-datestr_len)/2 ; }
	    else
	    { for (i=0;i<(xlen-datestr_len)/2;i++) { *(xstr.ptr++) = ' ' ; } ; } ;
	   memcpy(xstr.ptr,datestr,datestr_len) ; xstr.ptr += datestr_len ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<xlen-((xlen-datestr_len)/2+datestr_len);i++) *(xstr.ptr++) = ' ' ;
	    } ;
	   return(xlen) ;
	 } ;
/*	If here then left justify */
	memcpy(xstr.ptr,datestr,datestr_len) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += datestr_len ;
	   for (i=0;i<xlen-datestr_len;i++) *(xstr.ptr++) = ' ' ;
	 } ;
	return(xlen) ;
}

/*	D A T E - T I M E   E D I T I N G   R O U T I N E S		*/

/*	xti_date_time - Converts string to internal date/time format	*/

xti_date_time()
{ union flag__ref flags ;		/* Edit flags */
   struct num__ref anum ;	/* Describes dst variable */
   int year,month,day ; char tmonth[4] ;
   int hours,mins,secs ;
   int i,reslen ;
   char *errmne,*errmsg ;

#define DTERR(MNE,MSG) errmne = MNE ; errmsg = MSG ; goto bad_date_time

/*	Pick up the arguments			*/
	flags.all = xctu_popint() ; xctu_popnum(&anum) ; stru_popstr(&xstr) ; xlen = stru_sslen(&xstr) ;
	if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ;
	reslen = xlen ;		/* Save as result length */
/*	If /punctuation/ then strip off leading spaces */
	if (flags.fld.mask & V3_FLAGS_EDT_PUNCTUATION)
	 { while (*xstr.ptr == ' ' || *xstr.ptr == '\t')
	    { xstr.ptr++ ; if (--xlen <= 0) break ; } ;
	 } ;
/*	Do we have enough for valid date ? */
	if (strncmp(process->ci.yesterday,xstr.ptr,xlen)*strncmp(process->ci.YESTERDAY,xstr.ptr,xlen) == 0)
	 { if (anum.format.all != V3_FORMAT_NULL)
	    xctu_num_upd((B64INT)(TIMEOFFSETDAY-44240-1+(time(NULL)-(60*mscu_minutes_west()))/(60*60*24))*3600*24,0,&anum) ;
	    return(xlen) ;
	 } ;
	if (strncmp(process->ci.tomorrow,xstr.ptr,xlen)*strncmp(process->ci.TOMORROW,xstr.ptr,xlen) == 0)
	 { if (anum.format.all != V3_FORMAT_NULL)
	    xctu_num_upd((B64INT)(TIMEOFFSETDAY-44240+1+(time(NULL)-(60*mscu_minutes_west()))/(60*60*24))*3600*24,0,&anum) ;
	    return(xlen) ;
	 } ;
 	if (strncmp(process->ci.now,xstr.ptr,xlen)*strncmp(process->ci.NOW,xstr.ptr,xlen) == 0)
	 { i = time(NULL) - (60*mscu_minutes_west()) ; i -= TIMEOFFSETSEC ;
	   if (anum.format.all != V3_FORMAT_NULL) xctu_num_upd((B64INT)i,0,&anum) ;
	   return(xlen) ;
	 } ;

	if (xlen < 6)
/*	   Look for special keyword "none" */
	 { if (strncmp(process->ci.none,xstr.ptr,xlen)*strncmp(process->ci.NONE,xstr.ptr,xlen) == 0)
	    { if (anum.format.all != V3_FORMAT_NULL) xctu_num_upd((B64INT)0,0,&anum) ; return(xlen) ; } ;
	   if (strncmp(process->ci.today,xstr.ptr,xlen)*strncmp(process->ci.TODAY,xstr.ptr,xlen) == 0)
	    { if (anum.format.all != V3_FORMAT_NULL)
	       xctu_num_upd((B64INT)(TIMEOFFSETDAY-44240+(time(NULL)-(60*mscu_minutes_west()))/(60*60*24))*3600*24,0,&anum) ;
	       return(xlen) ;
	    } ;
/*	    v3_error(V3E_DTTOOSHORT,"XTI","DATE_TIME","DTTOOSHORT","Invalid date format- too short") ; */
	    DTERR("DTTOOSHORT","Invalid date-time format- too short") ;
	 } ;
/*	See if "mm/dd/yy" */
	if (*(xstr.ptr+1) == '/' || *(xstr.ptr+2) == '/')
	 { month = xti_date_num() ;
	   /* v3_error(V3E_BADMONTH,"XTI","DATE_TIME","BADMONTH","Invalid month",0) ; */
	   if (*(xstr.ptr++) != '/') { DTERR("BADMONTH","Invalid month") ; } ;
	   day = xti_date_num() ;
	   /* v3_error(V3E_BADDAY,"XTI","DATE_TIME","BADDAY","Invalid day",0) ; */
	   if (*(xstr.ptr++) != '/') { DTERR("BADDAY","Invalid day") ; } ;
	   year = xti_date_num() ; goto end_date_parse ;
	 } ;
/*	See if "dd-mmm-yy" */
	if (*(xstr.ptr+1) == '-' || *(xstr.ptr+2) == '-')
	 { day = xti_date_num() ; xstr.ptr++ ;
/*	   Convert month to upper case */
	   for (i=0;i<3;i++) { tmonth[i] = toupper(*(xstr.ptr++)) ; } ;
/*	   Search for month in list */
	   xlen -= 4 ;
	   if (*(xstr.ptr++) != '-') { DTERR("BADDAY","Invalid day") ; } ;
	   for (month=0;month<12;month++)
	    { if (strncmp(tmonth,process->ci.ucMonths[month],3) == 0) goto found_month ; } ;
	   DTERR("BADMONTH","Invalid month") ;
found_month:
	   month += 1 ; year = xti_date_num() ;
	   goto end_date_parse ;
	 } ;
/*	If here then must be date of form: "mmddyy" */
	month = xti_date_num() ; year = month % 100 ; day = (month/100) % 100 ; month /= 10000 ;
/*	Got year, month & day as numbers - check out */
end_date_parse:
	if (year < 100) { year += 2000 ; } ;
	/* v3_error(V3E_DTBADYEAR,"XTI","DATE_TIME","DTBADYEAR","Year must be between 1850 & 2050",0) ; */
	if (year < 1980 || year > 2047) { DTERR("DTBADYEAR","Year must be between 1980 & 2047") ; } ;
	/* v3_error(V3E_INVMONTH,"XTI","DATE_TIME","INVMONTH","Month must be between 1 and 12",0) ; */
	if (month < 1 || month > 12) { DTERR("INVMONTH","Month must be between 1 and 12") ; } ;
	/* v3_error(V3E_INVDAYOFMTH,"XTI","DATE_TIME","INVDAYOFMTH","Day of month is not valid for specified month",0) ; */
	if (day < 1 || day > days_in_month[month-1])
	 { if (!(month == 2 && day == 29 && (year%4) == 0))
	    { DTERR("INVDAYOFMTH","Day of month is not valid for specified month") ; } ;
	 } ;

/*	End of date, see if spaces or whatever until the time */
	if (xlen > 0)
	 { if ( !(*xstr.ptr == ' ' || *xstr.ptr == process->ci.TimeDelim))
	      /* v3_error(V3E_DTINVSEP,"XTI","DATE_TIME","DTINVSEP","Date portion must be followed by space of colon...") */
	    { DTERR("INVSEP","Date portion must be followed by space or colon before time portion") ; }
	   xstr.ptr++ ;
	 } else { hours = 0 ; mins = 0 ; secs = 0 ; goto end_time ; } ;

/*	Parse time spec of form "hh:mm:ss" */
	hours = xti_date_num() ;
	/* v3_error(V3E_BADHOUR,"XTI","DATE_TIME","BADHOUR","Invalid hour specification",0) ; */
	if (hours > 23) { DTERR("BADHOUR","Invalid hour specification") ; } ;
	/* v3_error(V3E_BADHOUR,"XTI","DATE_TIME","BADHOUR","Invalid hour specification",0) ; */
	if (*(xstr.ptr++) != process->ci.TimeDelim) { DTERR("BADHOUR","Invalid hour specification") ; } ;
	mins = xti_date_num() ;
	/* v3_error(V3E_BADMIN,"XTI","DATE_TIME","BADMIN","Invalid minute specification",0) ; */
	if (mins > 59) { DTERR("BADMIN","Invalid minute specification") ; } ;
/*	Are we done or do we have seconds ? */
	secs = 0 ; if (xlen <= 0) goto end_time ;
	if (*(xstr.ptr++) == process->ci.TimeDelim)
	 { secs = xti_date_num() ; xstr.ptr++ ;
	   /* v3_error(V3E_BADSEC,"XTI","DATE_TIME","BADSEC","Invalid seconds specification",0) ; */
	   if (secs > 59) { DTERR("BADSEC","Invalid seconds specification") ; } ;
	 } ;
/*	Look for AM/PM */
	for (;xlen>0 && *xstr.ptr==' ';(xlen--,xstr.ptr++)) ;
	if (xlen <= 0) goto end_time ;
	if (*xstr.ptr == 'P' || *xstr.ptr == 'p')
	 { if (hours < 12) hours += 12 ; goto end_time ; } ;
	/* v3_error(V3E_BADAMHOUR,"XTI","DATE_TIME","BADAMHOUR","Invalid AM hour specification",0) ; */
	if (*xstr.ptr == 'A' || *xstr.ptr == 'a')
	 { if (hours > 12) { DTERR("BADAMHOUR","Invalid AM hour specification") ; } ;
	   if (hours == 12) hours = 0 ;
	   goto end_time ;
	 } ;
	/* v3_error(V3E_BADAMPM,"XTI","DATE_TIME","BADAMPM","Invalid seconds or AM/PM specificat",0) ; */
	DTERR("BADAMPM","Invalid seconds or AM/PM specification") ;
/*	Here on end of time parse */
end_time:
	if (anum.format.all != V3_FORMAT_NULL)
	 xctu_num_upd((B64INT)(mscu_ymd_to_ud(year,month,day)-44240)*86400+hours*3600+mins*60+secs,0,&anum) ;
	return(reslen) ;
/*	Here on bad date/time format */
bad_date_time:
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_IF_ERROR)
	 { xctu_num_upd((B64INT)0,0,&anum) ; return(reslen) ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_NOERROR) return(0) ;
	v3_error(V3E_INVUD,"XTI","DATE_TIME",errmne,errmsg,0) ;
	return(0) ;
}

/*	itx_date_time - Converts internal date/time to string form	*/

itx_date_time()
{ struct str__ref xstr ;
   union flag__ref flags ;
   int i,xlen,datestr_len,month,day,year,century_and_year ;
   int id,ud ;
   int hours,mins,secs ;
   int ival ;
   char str[150],*lineptr ; int tstr_len ;

/*	Pick up the arguments					*/
	flags.all = xctu_popint() ; stru_popstr(&xstr) ; id = xctu_popint() ;
	flags.fld.mask |= process->ci.DefaultDTMask ;
	ival = id % 86400 ;
/*	Check for report line */
	if (((*xstr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
	 { lineptr = xstr.ptr ; rptu_colpos(&xstr,&xlen,flags.fld.byte3,V3_COLTYP_DATE_TIME) ; }
	 else { lineptr = NULLV ;
		if (xstr.format.fld.type == VFT_VARSTR)
		 { if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
		    { xlen = xstr.format.fld.length - stru_sslen(&xstr) - 1 ; }
		    else { xlen = xstr.format.fld.length - 1 ; *xstr.ptr = NULLV ; flags.fld.mask |= V3_FLAGS_EDT_CONCAT ; } ;
		 } else xlen = xstr.format.fld.length ;
		if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
	if (id == 0) { str[0] = NULLV ; goto got_string ; } ;
	id = id / 86400 + 44240 ;
	if (id < 0 || id > 70200)
	 v3_error(V3E_INVUD,"ITX","DATE","INVUD","Invalid universal date",(void *)id) ;
	year = ((id+678955)*4)/1460 ;
	for (;;)
	 { ud = mscu_ymd_to_ud(year,1,1) ;
	   if (ud > id) { year-- ; continue ; } ;
	   if (id - ud < ((year % 4) == 0 ? 366 : 365)) break ;
	   year-- ;
	 } ;
	id = year*1000 + id-ud + 1 ;
	year = id/1000 ; id %= 1000 ;
	if ((year % 4) == 0)
	 { for (month=1;month<12;month++)
	    { if (id <= julian_leap[month]) break ; } ;
	   day = id-julian_leap[month-1] ; goto got_mdy ;
	 } else
	 { for (month=1;month<12;month++)
	    { if (id <= julian[month]) break ; } ;
	   day = id-julian[month-1] ; goto got_mdy ;
	 } ;

/*	We got year, month & day - see if we need century */
got_mdy:
	if (year < 100) { year += 2000 ; } ;
	century_and_year = year ; if ((flags.fld.mask & V3_FLAGS_EDT_CENTURY) == 0) year %= 100 ;
/*	Now convert to string (str)	*/
	if (flags.fld.mask & V3_FLAGS_EDT_DDMMYY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	    { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(str,"%02d/%02d",day,month) ; }
		else sprintf(str,"%02d/%02d/%02d",day,month,year) ; }
	    else { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(str,"%d/%d",day,month) ; }
		     else sprintf(str,"%d/%d/%02d",day,month,year) ; } ;
	   goto got_date ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_MMDDYY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	    { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(str,"%02d/%02d",month,day) ; }
		else sprintf(str,"%02d/%02d/%02d",month,day,year) ; }
	    else { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(str,"%d/%d",month,day) ; }
		     else sprintf(str,"%d/%d/%02d",month,day,year) ; } ;
	   goto got_date ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_LONG_DATE)
	 { sprintf(str,"%s %d, %d",process->ci.LongMonths[month-1],day,century_and_year) ;
	   goto got_date ;
	 } ;
/*	By default, convert to dd-mmm-yy */
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(str,"%02d-%s",day,process->ci.Months[month-1]) ; }
	    else sprintf(str,"%02d-%s-%02d",day,process->ci.Months[month-1],year) ; }
	 else { if (flags.fld.mask & V3_FLAGS_EDT_NOYEAR) { sprintf(str,"%d-%s",day,process->ci.Months[month-1]) ; }
		 else sprintf(str,"%d-%s-%02d",day,process->ci.Months[month-1],year) ; } ;
got_date:
	strcat(str," ") ;	/* Separate time from date with space */

/*	Convert time to hours/mins/seconds */
	hours = ival/3600 ; mins = (ival % 3600)/60 ; secs = ival % 60 ;
	if (flags.fld.mask & V3_FLAGS_EDT_AM_PM)
	 { if (hours > 12) { hours -= 12 ; }
	    else { if (hours == 0) hours = 12 ; } ;
	 } ;
/*	Convert hours:mins to string */
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	 { sprintf(&str[strlen(str)],"%02d%c%02d",hours,process->ci.TimeDelim,mins) ; } else { sprintf(&str[strlen(str)],"%d:%02d",hours,mins) ; } ;
/*	Does user want seconds ? */
	if (flags.fld.mask & V3_FLAGS_EDT_SECONDS)
	 { sprintf(&str[strlen(str)],"%c%02d",process->ci.TimeDelim,secs) ; } ;
/*	Append trailing AM/PM ? */
	if (flags.fld.mask & V3_FLAGS_EDT_AM_PM)
	 { strcat(str,(ival >= 12*3600 ? " PM" : " AM")) ; } ;

got_string:
	tstr_len = strlen(str) ;
/*	Check for a report line */
	if (xlen == 0)
	 { xlen = tstr_len ; if (lineptr != NULLV) rptu_lineupd((struct rpt__line *)lineptr,xlen) ; } ;
/*	Now see if results are to be concatenated to istr */
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (xstr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","DATE_TIME","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)xstr.format.fld.type) ;
	   strncat(xstr.ptr,str,(tstr_len>xlen ? xlen : tstr_len)) ; *(xstr.ptr+(i = strlen(xstr.ptr))) = NULLV ;
	   if (tstr_len > xlen)
	    v3_error(V3E_TOOLONG,"XTI","DATE_TIME","TOOLONG","String to long to \'/concat/\'",(void *)tstr_len) ;
	   return(i) ;
	 } ;
/*	See if we are to right justify */
	if (tstr_len > xlen) tstr_len = xlen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { xstr.ptr += (xlen-tstr_len) ; }
	    else
	    { for (i=0;i<xlen-tstr_len;i++) { *(xstr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(xstr.ptr,str,tstr_len) ;
	   return(xlen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { xstr.ptr += (xlen-tstr_len)/2 ; }
	    else
	    { for (i=0;i<(xlen-tstr_len)/2;i++) { *(xstr.ptr++) = ' ' ; } ; } ;
	   memcpy(xstr.ptr,str,tstr_len) ; xstr.ptr += tstr_len ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<xlen-((xlen-tstr_len)/2+tstr_len);i++) *(xstr.ptr++) = ' ' ;
	    } ;
	   return(xlen) ;
	 } ;
/*	If here then left justify */
	memcpy(xstr.ptr,str,tstr_len) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += tstr_len ;
	   for (i=0;i<xlen-tstr_len;i++) *(xstr.ptr++) = ' ' ;
	 } ;
	return(xlen) ;
}

/*	G E N E R A L   F O R M A T T I N G			*/

/*	xti_gen - General External to Internal Formatting	*/

void xti_gen()
{ static union val__sob xti_object_name,xti_flags_object_name ;
   extern union val__sob obj_skel_objlr ;
   union val__format format ;
   struct db__formatptr mod ;
   union val__sob mobj,fobj ;
   int i,flags ;

/*	If first call then get object name for "ACCEPTOR" & "ACCEPTOR_FLAGS" */
	if (xti_object_name.all == NULLV)
	 { xti_object_name.all = sobu_name_lookup("ACCEPTOR",NULLV) ;
	   xti_flags_object_name.all = sobu_name_lookup("ACCEPTOR_FLAGS",NULLV) ;
	 } ;
/*	Do we have a valid xti object ? */
	if (obj_skel_objlr.all != NULLV)
/*	   Does this object have ACCEPTOR module attribute */
	 { if ((mobj.all = sobu_has_find(obj_skel_objlr.all,xti_object_name.all)) != NULLV)
/*	      Does this object have ACCEPTOR_FLAGS ? */
	    { if ((fobj.all = sobu_has_find(obj_skel_objlr.all,xti_flags_object_name.all)) != NULLV)
	       { flags = xctu_popint() ;
		 sobu_push_val(fobj.all,0,NULLV,NULLV,FALSE,FALSE) ;
		 flags |= xctu_popint() ; PUSHINT(flags) ;
	       } ;
	    } ;
	   sobu_push_val(mobj.all,0,NULLV,NULLV,FALSE,FALSE) ;
/*	   Now invoke the module */
	   POPF(mod.format.all) ; POPVP(mod.ptr,(char *)) ;
	   xctu_call_defer(&mod) ; return ;
	 } ;
/*	We don't have an object or module, try our best */
	format.all = NEXTSF ;			/* Get format of 2nd argument */
	switch (format.fld.type)
	 { default:
		v3_error(V3E_INVXTIGENDT,"XTI","GEN","INVXTIGENDT","Cannot use XTI_GEN on this value type",(void *)format.fld.type) ;
	   case VFT_BININT:
	   case VFT_BINLONG:
	   case VFT_FLOAT:
	   case VFT_PACDEC:
	   case VFT_STRINT:
	   case VFT_BINWORD:	i = xti_int() ; break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
	   case VFT_VARSTR:	i = xti_alpha(FALSE) ; break ;
	 } ;
/*	Pop off EOA and return "i" */
	POPF(flags) ; POPVI(flags) ; PUSHINT(i) ;
}

/*	itx_gen - General Internal to External Formatting	*/

void itx_gen()
{ static union val__sob itx_object_name,itx_flags_object_name ;
   extern union val__sob obj_skel_objlr ;
   union val__format format ;
   struct db__formatptr mod ;
   union val__sob mobj,fobj ;
   int i,flags ;

/*	If first call then get object name for "DISPLAYER" & "DISPLAYER_FLAGS" */
	if (itx_object_name.all == NULLV)
	 { itx_object_name.all = sobu_name_lookup("DISPLAYER",NULLV) ;
	   itx_flags_object_name.all = sobu_name_lookup("DISPLAYER_FLAGS",NULLV) ;
	 } ;
/*	Do we have a valid itx object ? */
	if (obj_skel_objlr.all != NULLV)
/*	   Does this object have DISPLAYER module attribute */
	 { if ((mobj.all = sobu_has_find(obj_skel_objlr.all,itx_object_name.all)) != NULLV)
/*	      Does this object have DISPLAYER_FLAGS ? */
	    { if ((fobj.all = sobu_has_find(obj_skel_objlr.all,itx_flags_object_name.all)) != NULLV)
	       { flags = xctu_popint() ;
		 sobu_push_val(fobj.all,0,NULLV,NULLV,FALSE,FALSE) ;
		 flags |= xctu_popint() ; PUSHINT(flags) ;
	       } ;
	    } ;
	   sobu_push_val(mobj.all,0,NULLV,NULLV,FALSE,FALSE) ;
/*	   Now invoke the module */
	   POPF(mod.format.all) ; POPVP(mod.ptr,(char *)) ;
	   xctu_call_defer(&mod) ; return ;
	 } ;
/*	We don't have an object or module, try our best */
	format.all = NNEXTSF ;	/* Get format of 1st argument */
	switch (format.fld.type)
	 { default:
		v3_error(V3E_INVITXGENDT,"ITX","GEN","INVITXGENDT","Cannot use ITX_GEN on this value type",(void *)format.fld.type) ;
	   case VFT_BININT:
	   case VFT_BINLONG:
	   case VFT_FLOAT:
	   case VFT_PACDEC:
	   case VFT_STRINT:
	   case VFT_BINWORD:	i = itx_int() ; break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
	   case VFT_VARSTR:	i = xti_alpha(FALSE) ; break ;
	 } ;
/*	Pop off EOA and return "i" */
	POPF(flags) ; POPVI(flags) ; PUSHINT(i) ;
}

/*	L O G I C A L   E D I T I N G   R O U T I N E S		*/

/*	xti_logical - Parses a logical value			*/

xti_logical()
{ struct str__ref xstr ; int xlen ;
   struct str__ref resstr ;
   struct num__ref anum ;
   union flag__ref flags ;
   union val__format res_format ;
   char res_type ; B64INT result ;

/*	Pick up the arguments			*/
	flags.all = xctu_popint() ;
	POPF(res_format.all) ; PUSHF(res_format.all) ;
	switch (res_type = res_format.fld.type)
	 { default: xctu_popnum(&anum) ; break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
	   case VFT_VARSTR:
		stru_popstr(&resstr) ; break ;
	 } ;
	stru_popstr(&xstr) ; xlen = stru_sslen(&xstr) ;
/*	Get length */
	if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ;
/*	Strip off leading junk ? */
	if (flags.fld.mask & V3_FLAGS_EDT_PUNCTUATION)
	 { while (*xstr.ptr == ' ' || *xstr.ptr == '\t')
	    { xstr.ptr++ ; if (--xlen <= 0) break ; } ;
	 } ;
/*	Now test for Y/N, T/F, 1/0	*/
	if (strchr("YyTt1",*xstr.ptr) > 0 || process->ci.yes[0] == *xstr.ptr || process->ci.YES[0] == *xstr.ptr) { result = 1 ; }
	 else { if (strchr("NnFf0",*xstr.ptr) > 0 || process->ci.no[0] == *xstr.ptr || process->ci.NO[0] == *xstr.ptr) { result = 0 ; }
		 else { if (flags.fld.mask & V3_FLAGS_EDT_NOERROR) return(0) ;
			v3_error(V3E_BADLOG,"XTI","LOGICAL","BADLOG","Expecting Y/N, 1/0, or T/F",0) ;
		      } ;
	      } ;
	switch (res_format.fld.type)
	 { default: xctu_num_upd(result,0,&anum) ; break ;
	   case VFT_V3INT: break ;
	   case VFT_VARSTR: *resstr.ptr = (result > 0 ? process->ci.YES[0] : process->ci.NO[0]) ; *(resstr.ptr+1) = NULLV ; break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
		*resstr.ptr = (result > 0 ? process->ci.YES[0] : process->ci.NO[0]) ;
		break ;
	 } ;
	return(xlen) ;
}

/*	itx_logical - Converts internal logical to string	*/

itx_logical()
{ union flag__ref flags ;
   union val__format format ;
   char ival ; struct str__ref istr ;
   struct str__ref xstr ; int xlen ;
   int i ; char str[20],*lineptr ; int tstr_len ;

/*	Pick up args, maybe convert internal to integer 1/0 */
	flags.all = xctu_popint() ; stru_popstr(&xstr) ;
/*	Check for a report line */
	if (((*xstr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
	 { lineptr = xstr.ptr ; rptu_colpos(&xstr,&xlen,flags.fld.byte3,V3_COLTYP_LOGICAL) ; }
	 else { lineptr = NULLV ; xlen = stru_sslen(&xstr) ;
		if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
	POPF(format.all) ; PUSHF(format.all) ;
	if (format.fld.type == VFT_FIXSTR || format.fld.type == VFT_FIXSTRSK || format.fld.type == VFT_VARSTR)
	 { stru_popstr(&istr) ; ival = (*istr.ptr == 'Y' ? 1 : 0) ; }
	 else { ival = xctu_popint() ; } ;

/*	Now figure out what to convert to, YES/NO, TRUE/FALSE, ... */
	if (flags.fld.mask & V3_FLAGS_EDT_TRUE_FALSE)
	 { if (xlen >= 4)
	    { strcpy(str,(ival > 0 ? process->ci.lTRUE : process->ci.lFALSE)) ; }
	    else { strcpy(str,(ival > 0 ? "T" : "F")) ; } ;
	   goto log_done ;
	 }
	if (flags.fld.mask & V3_FLAGS_EDT_X_TRUE)
	 { strcpy(str,(ival > 0 ? "X" : " ")) ; goto log_done ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_X_FALSE)
	 { strcpy(str,(ival > 0 ? " " : "X")) ; goto log_done ; } ;
/*	Assume YES/NO */
	if (xlen >= 3)
	 { strcpy(str,(ival > 0 ? process->ci.YES : process->ci.NO)) ; }
	 else { strcpy(str,(ival > 0 ? "Y" : "N")) ; } ;
/*	Here to place result in external string */
log_done:
/*	Now take str & shove into xstr */
	tstr_len = strlen(str) ;
/*	Check for a report line */
	if (xlen == 0)
	 { xlen = tstr_len ; if (lineptr != NULLV) rptu_lineupd((struct rpt__line *)lineptr,xlen) ; } ;
/*	Now see if results are to be concatenated to istr */
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (xstr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","ALPHA","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)xstr.format.fld.type) ;
	   if (tstr_len + xlen >= xstr.format.fld.length)
	    v3_error(V3E_TOOLONG,"XTI","ALPHA","TOOLONG","String to long to \'/concat/\'",(void *)tstr_len) ;
	   strncat(xstr.ptr,str,tstr_len) ; *(xstr.ptr+xlen+tstr_len) = NULLV ;
	   return(xlen+tstr_len) ;
	 } ;
/*	See if we are to right justify */
	if (tstr_len > xlen) tstr_len = xlen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { xstr.ptr += (xlen-tstr_len) ; }
	    else
	    { for (i=0;i<xlen-tstr_len;i++) { *(xstr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(xstr.ptr,str,tstr_len) ;
	   return(xlen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { xstr.ptr += (xlen-tstr_len)/2 ; }
	    else
	    { for (i=0;i<(xlen-tstr_len)/2;i++) { *(xstr.ptr++) = ' ' ; } ; } ;
	   memcpy(xstr.ptr,str,tstr_len) ; xstr.ptr += tstr_len ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<xlen-((xlen-tstr_len)/2+tstr_len);i++) *(xstr.ptr++) = ' ' ;
	    } ;
	   return(xlen) ;
	 } ;
/*	If here then left justify */
	memcpy(xstr.ptr,str,tstr_len) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += tstr_len ;
	   for (i=0;i<xlen-tstr_len;i++) *(xstr.ptr++) = ' ' ;
	 } ;
	return(xlen) ;
}

/*	M O N T H   E D I T I N G   R O U T I N E S		*/

/*	xti_month - Converts a string to internal month format	*/

xti_month()
{ union flag__ref flags ;		/* Edit flags */
   struct num__ref anum ;	/* Describes dst variable */
   int century_and_year,year,month ; char tmonth[4] ;
   int i,reslen ;
   char *errmne,*errmsg ; int errnum ;

#define MONERR(NUM,MNE,MSG) errnum = NUM ; errmne = MNE ; errmsg = MSG ; goto bad_month

/*	Pick up the arguments			*/
	flags.all = xctu_popint() ; xctu_popnum(&anum) ; stru_popstr(&xstr) ; xlen = stru_sslen(&xstr) ;
	if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ;
	reslen = xlen ;		/* Save as result length */
/*	If /punctuation/ then strip off leading spaces */
	if (flags.fld.mask & V3_FLAGS_EDT_PUNCTUATION)
	 { while (*xstr.ptr == ' ' || *xstr.ptr == '\t')
	    { xstr.ptr++ ; if (--xlen <= 0) break ; } ;
	 } ;
/*	Do we have enough for valid month ? */
	if (xlen < 4) v3_error(V3E_MONTHTOOSHORT,"XTI","MONTH","MONTHTOOSHORT","Invalid month format",0) ;
/*	Look for special keyword "none" */
	if (strncmp(process->ci.none,xstr.ptr,xlen)*strncmp(process->ci.NONE,xstr.ptr,xlen) == 0)
	 { if (anum.format.all != V3_FORMAT_EOA) xctu_num_upd((B64INT)0,0,&anum) ; return(xlen) ; } ;
/*	See if "mm/yy" */
	if (*(xstr.ptr+1) == '/' || *(xstr.ptr+2) == '/')
	 { month = xti_date_num() ;
	   /* v3_error(V3E_BADMONTH,"XTI","MONTH","BADMONTH","Invalid month",0) ; */
	   if (*(xstr.ptr++) != '/') { MONERR(V3E_BADMONTH,"BADMONTH","Invalid month") ; } ;
	   year = xti_date_num() ; goto end_month_parse ;
	 } ;
/*	See if "mmm-yy" */
	if (*(xstr.ptr+3) == '-')
/*	   Convert month to upper case */
	 { for (i=0;i<3;i++) { tmonth[i] = toupper(*(xstr.ptr++)) ; } ;
/*	   Search for month in list */
	   xlen -= 4 ;
	   if (*(xstr.ptr++) != '-') { MONERR(V3E_BADDAY,"BADDAY","Invalid day") ; } ;
	   for (month=0;month<12;month++)
	    { if (strncmp(tmonth,process->ci.ucMonths[month],3) == 0) goto found_month ; } ;
	   MONERR(V3E_BADMONTH,"BADMONTH","Invalid month") ;
found_month:
	   month += 1 ; year = xti_date_num() ;
	   goto end_month_parse ;
	 } ;
/*	If here then must be of form: "mmyy" */
	month = xti_date_num() ; year = month % 100 ; month /= 100 ;
/*	Got year & month as numbers - check out */
end_month_parse:
	if (year < 100) { year += 2000 ; } ;
	/* v3_error(V3E_BADYEAR,"XTI","MONTH","BADYEAR","Year must be between 1850 & 2050",0) ; */
	if (year < 1850 || year > 2130) { MONERR(V3E_BADYEAR,"BADYEAR","Year must be between 1850 & 2130") ; } ;
	/* v3_error(V3E_INVMONTH,"XTI","MONTH","INVMONTH","Month must be between 1 and 12",0) ; */
	if (month < 1 || month > 12) { MONERR(V3E_INVMONTH,"INVMONTH","Month must be between 1 and 12") ; } ;
/*	If result is "nil" then quit now */
	if (anum.format.all == V3_FORMAT_NULL) return(reslen) ;
/*	All looks well, return internal format */
	xctu_num_upd((B64INT)(year-1850)*12+(month-1),0,&anum) ; return(reslen) ;
/*	Here to handle bad month */
bad_month:
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_IF_ERROR)
	 { xctu_num_upd((B64INT)0,0,&anum) ; return(reslen) ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_NOERROR) return(0) ;
/*	Generate a V3 error */
	v3_error(errnum,"XTI","MONTH",errmne,errmsg,0) ;
	return(0) ;
}

/*	itx_month - Converts internal month to string form	*/

itx_month()
{ struct str__ref xstr ;
   union flag__ref flags ;
   int i,xlen,monstr_len,month,year,century_and_year ;
   int id,ud ;
   char monstr[100],*lineptr ;

/*	Pick up the arguments					*/
	flags.all = xctu_popint() ; stru_popstr(&xstr) ; id = xctu_popint() ;
	flags.fld.mask |= process->ci.DefaultMonthMask ;
/*	Check for report line */
	if (((*xstr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
	 { lineptr = xstr.ptr ; rptu_colpos(&xstr,&xlen,flags.fld.byte3,V3_COLTYP_MONTH) ; }
	 else { lineptr = NULLV ; xlen = stru_sslen(&xstr) ;
		if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
	if (id == 0) { monstr[0] = NULLV ; goto got_string ; } ;
/*	Convert internal form to month & year */
	year = id/12 + 1850 ; month = (id % 12) + 1 ;
/*	We got year & month - see if we need century */
	if (year < 100) { year += 2000 ; } ;
	century_and_year = year ; if ((flags.fld.mask & V3_FLAGS_EDT_CENTURY) == 0) year %= 100 ;
/*	Now convert to string (monstr)	*/
	if (flags.fld.mask & V3_FLAGS_EDT_MMDDYY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	    { sprintf(monstr,"%02d/%02d",month,year) ; }
	    else { sprintf(monstr,"%d/%02d",month,year) ; } ;
	   goto got_string ;
	 } ;
	if (flags.fld.mask & V3_FLAGS_EDT_LONG_DATE)
	 { sprintf(monstr,"%s %d",process->ci.LongMonths[month-1],century_and_year) ;
	   goto got_string ;
	 } ;
/*	By default, convert to mmm-yy */
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	 { sprintf(monstr,"%s-%02d",process->ci.Months[month-1],year) ; }
	 else { sprintf(monstr,"%s-%02d",process->ci.Months[month-1],year) ; } ;
	goto got_string ;
/*	Here with resulting string - update second argument */
got_string:
	monstr_len = strlen(monstr) ;
/*	Check for a report line */
	if (xlen == 0)
	 { xlen = monstr_len ; if (lineptr != NULLV) rptu_lineupd((struct rpt__line *)lineptr,xlen) ; } ;
/*	Now see if results are to be concatenated to istr */
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (xstr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","ALPHA","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)xstr.format.fld.type) ;
	   if (monstr_len + xlen >= xstr.format.fld.length)
	    v3_error(V3E_TOOLONG,"XTI","ALPHA","TOOLONG","String to long to \'/concat/\'",(void *)monstr_len) ;
	   strncat(xstr.ptr,monstr,monstr_len) ; *(xstr.ptr+xlen+monstr_len) = NULLV ;
	   return(xlen+monstr_len) ;
	 } ;
/*	See if we are to right justify */
	if (monstr_len > xlen) monstr_len = xlen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { xstr.ptr += (xlen-monstr_len) ; }
	    else
	    { for (i=0;i<xlen-monstr_len;i++) { *(xstr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(xstr.ptr,monstr,monstr_len) ;
	   return(xlen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { xstr.ptr += (xlen-monstr_len)/2 ; }
	    else
	    { for (i=0;i<(xlen-monstr_len)/2;i++) { *(xstr.ptr++) = ' ' ; } ; } ;
	   memcpy(xstr.ptr,monstr,monstr_len) ; xstr.ptr += monstr_len ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<xlen-((xlen-monstr_len)/2+monstr_len);i++) *(xstr.ptr++) = ' ' ;
	    } ;
	   return(xlen) ;
	 } ;
/*	If here then left justify */
	memcpy(xstr.ptr,monstr,monstr_len) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += monstr_len ;
	   for (i=0;i<xlen-monstr_len;i++) *(xstr.ptr++) = ' ' ;
	 } ;
	return(xlen) ;
}

/*	P O I N T E R   E D I T I N G   R O U T I N E S		*/

/*	xti_pointer - Converts pointer: External -> Internal	*/

xti_pointer()
{ struct str__ref inpstr ;	/* String ref to be edited */
  union val__format vf ;
  union flag__ref flags ;	/* Editing flags */
  char *result ; char tbuf[100] ;
  int len,i ; char c ; char *(*ptr) ;

/*	Get the arguments		*/
	flags.all = xctu_popint() ; POPF(vf.all) ;
	if (vf.fld.type != VFT_POINTER) v3_error(V3E_NOTPTR,"XTI","POINTER","NOTPTR","Value not a POINTER",(void *)vf.fld.type) ;
	POPVP(ptr,(char *(*))) ; stru_popstr(&inpstr) ;
	result = NULLV ; len = (flags.fld.byte3 == 0 ? stru_sslen(&inpstr) : flags.fld.byte3) ;
	if (len-1 >= sizeof tbuf) v3_error(V3E_INVLEN,"XTI","POINTER","INVLEN","Invalid length of pointer",(void *)len) ;
/*	Maybe advance over leading "^X" */
	if (*inpstr.ptr == '^' && (*(inpstr.ptr+1) == 'x' || *(inpstr.ptr+1) == 'X'))
	 { len -= 2 ; inpstr.ptr = inpstr.ptr + 2 ; } ;
	strncpy(tbuf,inpstr.ptr,len) ; tbuf[len] = 0 ;
/*	Loop thru all bytes & parse */
	if (sscanf(inpstr.ptr,"%p",&result) != 1) v3_error(V3E_INVPOINTER,"XTI","POINTER","INVPOINTER","Invalid format for pointer",(void *)0) ;
/*	Update result & return with length */
	*ptr = (char *)result ; return(len) ;
}

/*	itx_pointer - Converts pointer: Internal -> External	*/

itx_pointer()
{ union flag__ref flags ;
   struct str__ref xstr ; int xlen ;
   char *ptr ;
   char *lineptr ; int str_len ;
   int i ; char buf[50] ;

/*	Pick up arguments */
	flags.all = xctu_popint() ; stru_popstr(&xstr) ;
/*	See if we got a report line */
	if (((*xstr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
	 { lineptr = xstr.ptr ;
	 }
	 else { if ( xstr.format.fld.type == VFT_VARSTR)
		 { flags.all |= V3_FLAGS_EDT_CONCAT ; xlen = (flags.fld.byte3 > 0 ? flags.fld.byte3 : 0) ; }
		 else { xlen = stru_sslen(&xstr) ; if (flags.fld.byte3 > 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
		lineptr = NULLV ;
	      } ;
	ptr = (char *)xctu_popptr() ; sprintf(buf,"^X%p",ptr) ; str_len = strlen(buf) ;
/*	If here then left justify */
	if (str_len > xlen) v3_error(V3E_FLDTOOSMALL,"ITX","POINTER","FLDTOOSMALL","Field too small to hold pointer",(void *)str_len) ;
	memcpy(xstr.ptr,buf,str_len) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += str_len ; for (i=0;i<xlen-str_len;i++) *(xstr.ptr++) = ' ' ; } ;
	return(xlen) ;
}

/*	T I M E   E D I T I N G   R O U T I N E S		*/

/*	xti_time - Converts string to internal time format	*/

xti_time()
{ union flag__ref flags ;
   struct num__ref anum ;
   int hours,mins,secs,i,reslen ;
   char *errmne,*errmsg ; int errnum ;

#define TIMERR(NUM,MNE,MSG) errnum = NUM ; errmne = MNE ; errmsg = MSG ; goto bad_time

/*	Pick up the arguments */
	flags.all = xctu_popint() ; xctu_popnum(&anum) ;
	stru_popstr(&xstr) ; xlen = stru_sslen(&xstr) ;
	if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ;
	reslen = xlen ;		/* Save as result length */
/*	If /punctuation/ then strip off leading spaces */
	if (flags.fld.mask & V3_FLAGS_EDT_PUNCTUATION)
	 { while (*xstr.ptr == ' ' || *xstr.ptr == '\t')
	    { xstr.ptr++ ; if (--xlen <= 0) break ; } ;
	 } ;
	if (strncmp(xstr.ptr,process->ci.none,xlen) == 0 || strncmp(xstr.ptr,process->ci.NONE,xlen) == 0)
	 { hours = ( mins = 0) ; secs = -1 ; goto end_time ; } ;
/*	Parse time spec of form "hh:mm:ss" */
	hours = xti_date_num() ;
	/* v3_error(V3E_BADHOUR,"XTI","TIME","BADHOUR","Invalid hour specification",0) ; */
	if (hours > 23) { TIMERR(V3E_BADHOUR,"BADHOUR","Invalid hour specification") ; } ;
	/* v3_error(V3E_BADHOUR,"XTI","TIME","BADHOUR","Invalid hour specification",0) ; */
	if (*(xstr.ptr++) != process->ci.TimeDelim) { TIMERR(V3E_BADHOUR,"BADHOUR","Invalid hour specification") ; } ;
	mins = xti_date_num() ;
	/* v3_error(V3E_BADMIN,"XTI","TIME","BADMIN","Invalid minute specification",0) ; */
	if (mins > 59) { TIMERR(V3E_BADMIN,"BADMIN","Invalid minute specification") ; } ;
/*	Are we done or do we have seconds ? */
	secs = 0 ; if (xlen <= 0) goto end_time ;
	if (*(xstr.ptr++) == process->ci.TimeDelim)
	 { secs = xti_date_num() ; xstr.ptr++ ;
	   /* v3_error(V3E_BADSEC,"XTI","TIME","BADSEC","Invalid seconds specification",0) ; */
	   if (secs > 59) { TIMERR(V3E_BADSEC,"BADSEC","Invalid seconds specification") ; } ;
	 } ;
/*	Look for AM/PM */
	for (;xlen>0 && *xstr.ptr==' ';(xlen--,xstr.ptr++)) ;
	if (xlen <= 0) goto end_time ;
	if (*xstr.ptr == 'P' || *xstr.ptr == 'p')
	 { if (hours < 12) hours += 12 ; goto end_time ; } ;
	/* v3_error(V3E_BADAMHOUR,"XTI","TIME","BADAMHOUR","Invalid AM hour specification",0) ; */
	if (*xstr.ptr == 'A' || *xstr.ptr == 'a')
	 { if (hours > 12) { TIMERR(V3E_BADAMHOUR,"BADAMHOUR","Invalid AM hour specification") ; } ;
	   if (hours == 12) hours = 0 ;
	   goto end_time ;
	 } ;
	/* v3_error(V3E_BADAMPM,"XTI","TIME","BADAMPM","Invalid seconds or AM/PM specificat",0) ; */
	TIMERR(V3E_BADAMPM,"BADAMPM","Invalid seconds or AM/PM specification") ;
/*	Here on end of time parse */
end_time:
	if (anum.format.all != V3_FORMAT_NULL)
	 xctu_num_upd((B64INT)hours*3600+mins*60+secs,0,&anum) ;
	return(reslen) ;
/*	Here on bad time */
bad_time:
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_IF_ERROR)
	 { xctu_num_upd((B64INT)0,0,&anum) ; return(reslen) ; } ;
	if (flags.fld.mask & V3_FLAGS_EDT_NOERROR) return(0) ;
	v3_error(errnum,"XTI","TIME",errmne,errmsg,0) ;
	return(0) ;
}

/*	itx_time - Converts internal time to print format	*/
	
itx_time()
{ union flag__ref flags ;
   struct str__ref xstr ; int xlen ;
   int hours,mins,secs,i ;
   int ival ;
   char str[50],*lineptr ; int tstr_len ;

/*	Pick up the arguments					*/
	flags.all = xctu_popint() ; stru_popstr(&xstr) ; ival = xctu_popint() ;
/*	Check for a report line */
	if (((*xstr.ptr & 0xff) == 0xff) && (flags.fld.byte3 > 0))
	 { lineptr = xstr.ptr ; rptu_colpos(&xstr,&xlen,flags.fld.byte3,V3_COLTYP_TIME) ; }
	 else { lineptr = NULLV ; xlen = stru_sslen(&xstr) ;
		if (flags.fld.byte3 != 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
/*	If time value less than 0 then blank it */
	if (ival < 0) { strcpy(str,"     ") ; goto time_format_done ; } ;
/*	If a date_time the pick out just the time */
	if (flags.fld.mask & V3_FLAGS_EDT_DATE_TIME) ival = ival % 86400 ;
/*	Convert time to hours/mins/seconds */
	hours = ival/3600 ; mins = (ival % 3600)/60 ; secs = ival % 60 ;
	if (flags.fld.mask & V3_FLAGS_EDT_AM_PM)
	 { if (hours > 12) { hours -= 12 ; }
	    else { if (hours == 0) hours = 12 ; } ;
	 } ;
/*	Convert hours:mins to string */
	if (flags.fld.mask & V3_FLAGS_EDT_ZERO_FILL)
	 { sprintf(str,"%02d%c%02d",hours,process->ci.TimeDelim,mins) ; } else { sprintf(str,"%d:%02d",hours,mins) ; } ;
/*	Does user want seconds ? */
	if (flags.fld.mask & V3_FLAGS_EDT_SECONDS)
	 { sprintf(&str[strlen(str)],"%c%02d",process->ci.TimeDelim,secs) ; } ;
/*	Append trailing AM/PM ? */
	if (flags.fld.mask & V3_FLAGS_EDT_AM_PM)
	 { strcat(str,(ival >= 12*3600 ? " PM" : " AM")) ; } ;
/*	Now take str & shove into xstr */
	tstr_len = strlen(str) ;
/*	Check for a report line */
	if (xlen == 0)
	 { xlen = tstr_len ; if (lineptr != NULLV) rptu_lineupd((struct rpt__line *)lineptr,xlen) ; } ;
/*	Now see if results are to be concatenated to istr */
time_format_done:
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (xstr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","ALPHA","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)xstr.format.fld.type) ;
	   if (tstr_len + xlen >= xstr.format.fld.length)
	    v3_error(V3E_TOOLONG,"XTI","ALPHA","TOOLONG","String to long to \'/concat/\'",(void *)tstr_len) ;
	   strncat(xstr.ptr,str,tstr_len) ; *(xstr.ptr+xlen+tstr_len) = NULLV ;
	   return(xlen+tstr_len) ;
	 } ;
/*	See if we are to right justify */
	if (tstr_len > xlen) tstr_len = xlen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { xstr.ptr += (xlen-tstr_len) ; }
	    else
	    { for (i=0;i<xlen-tstr_len;i++) { *(xstr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(xstr.ptr,str,tstr_len) ;
	   return(xlen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { xstr.ptr += (xlen-tstr_len)/2 ; }
	    else
	    { for (i=0;i<(xlen-tstr_len)/2;i++) { *(xstr.ptr++) = ' ' ; } ; } ;
	   memcpy(xstr.ptr,str,tstr_len) ; xstr.ptr += tstr_len ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<xlen-((xlen-tstr_len)/2+tstr_len);i++) *(xstr.ptr++) = ' ' ;
	    } ;
	   return(xlen) ;
	 } ;
/*	If here then left justify */
	memcpy(xstr.ptr,str,tstr_len) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += tstr_len ;
	   for (i=0;i<xlen-tstr_len;i++) *(xstr.ptr++) = ' ' ;
	 } ;
	return(xlen) ;
}
