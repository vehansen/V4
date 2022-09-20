/*	V3MTHU.C - Conversion Routines for Unix V3

	Created 2/21/90 by Victor E. Hansen			*/

#include <time.h>
#include "v3defs.c"

extern struct db__psi *psi ;

#define ZCB memset(cb.all.buf,'0',27) ;

int pow_tens[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 } ;
int rounding[] = { 0, 5, 50, 500, 5000, 50000, 500000, 5000000, 50000000, 500000000 } ;
int dp_scale[] = { 1, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1 } ;
int dp_rounding[] = { 500000000, 50000000, 5000000, 500000, 50000, 5000, 500, 50, 5 } ;
double dbl_tens[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000.0, 100000000.0, 1000000000.0, 10000000000.0 } ;

void mthu_cvtpp() {}

void mthu_cvtps() {}

void mthu_mulpd() {}

void mthu_cvtip() {}

void mthu_addpd() {}

void mthu_cvtis(integer,src_dps,dst,dst_len,dst_dps)	/* Convert integer to DEC */
  B64INT integer ;
  int src_dps,dst_len,dst_dps ; char *dst ;
{ union val__v3num v3n ; union cnv__buf cb ;
    char *src,*b,*dstptr ; int i,n,neg ;

/*	First move integer into v3num */
	if (src_dps > dst_dps)
	 { if (integer > 0) { integer += rounding[src_dps-dst_dps-1] ; } else { integer -= rounding[src_dps-dst_dps-1] ; } ; } ;
	if (integer < 0) { neg = TRUE ; integer = -integer ; } else { neg = FALSE ; } ;
	v3n.fd.num = integer / pow_tens[src_dps] ; v3n.fd.dps = (integer % pow_tens[src_dps]) * dp_scale[src_dps] ;
	ZCB n = v3n.fd.num ; b = &cb.v3n.num[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
	n = v3n.fd.dps ; b = &cb.v3n.dps[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
/*	Now move portion to destination string */
	src = (src = (char *)&cb) + 18 + dst_dps - dst_len ;	/* src = pointer to correct spot in conversion buffer */
	dstptr = dst ; for(i=dst_len;i>0;i--) { *(dstptr++) = *(src++) ; } ;	/* Copy correct number of bytes */
	if (neg) { dstptr-- ; *dstptr = *dstptr - '0' + 'p' ; } ;
}

void mthu_negpd() {}

void mthu_cvtsp() {}

B64INT mthu_cvtii(src,srcdps,dstdps)			/* Convert integer to integer */
 B64INT src ;
 int srcdps,dstdps ;
{ int shift ;

	shift = dstdps - srcdps ;	/* Number of places to shift */
	if (shift == 0) return(src) ;
	if (shift > 0) return( src * pow_tens[shift] ) ;
	if (src > 0) { src += rounding[srcdps-dstdps-1] ; } else { src -= rounding[srcdps-dstdps-1] ; } ;
	return( src / pow_tens[-shift] ) ;
}

#define GETDIGIT(DST,SRC) { if(*SRC>='0') { *(DST++) = *(SRC++) ; continue ; } ; if(*SRC != '\0') goto badnum ; SRC++ ; *(DST++)='0' ; }

void mthu_cvtss(src_ptr,src_len,src_dps,dst_ptr,dst_len,dst_dps)
  char *src_ptr,*dst_ptr ; int src_len,src_dps,dst_len,dst_dps ;
{ union val__v3num v3n ; union cnv__buf cb ;
   int i,n,neg ; char *b,*dst,*src,*srcptr ;

/*	Convert dec to v3num	*/
	ZCB
	src = (src = (char *)&cb) + 18 + src_dps - src_len ;
//	for(srcptr=src_ptr,i=src_len;i>0;i--) { if (*srcptr < '0') goto badnum ; *(src++) = *(srcptr++) ; } ;
	for(srcptr=src_ptr,i=src_len;i>0;i--) GETDIGIT(src,srcptr) ;
/*	Check for sign */
	if (*(--src) <= '9') { neg = FALSE ; }
	 else { neg = TRUE ; *src -= ('p'-'0') ; } ;
/*	Convert the three portions */
	v3n.fd.big = 0 ; for(i=0,src=(char *)&cb.v3n.big;i<=8;i++) { v3n.fd.big *= 10 ; v3n.fd.big += *(src++) - '0' ; } ;
	v3n.fd.num = 0 ; for(i=0,src=(char *)&cb.v3n.num;i<=8;i++) { v3n.fd.num *= 10 ; v3n.fd.num += *(src++) - '0' ; } ;
	v3n.fd.dps = 0 ; for(i=0,src=(char *)&cb.v3n.dps;i<=8;i++) { v3n.fd.dps *= 10 ; v3n.fd.dps += *(src++) - '0' ; } ;
/*	Adjust for negatives */
//	if (dst_dps == 0)
//	 { if (v3n.fd.dps >= 500000000) { v3n.fd.num += 1 ; } ;
//	 } else { v3n.fd.dps += dp_rounding[dst_dps] ; } ;
	if (dst_dps == 0)
	 { if (v3n.fd.dps >= 500000000)
	    { v3n.fd.num += 1 ;
	      if (v3n.fd.num >= 1000000000) { v3n.fd.num -= 1000000000 ; v3n.fd.big += 1 ; } ;
	    } else
	    { if (v3n.fd.dps <= -500000000) v3n.fd.num -= 1 ;
	      if (v3n.fd.num <= -1000000000) { v3n.fd.num += 1000000000 ; v3n.fd.big -= 1 ; } ;
	    } ;
	 } else
	 { if (v3n.fd.dps < 0)
	    { v3n.fd.dps -= dp_rounding[dst_dps] ;
	      if (v3n.fd.dps <= -1000000000) { v3n.fd.dps += 1000000000 ; v3n.fd.num -= 1 ; } ;
	      if (v3n.fd.num <= -1000000000) { v3n.fd.num += 1000000000 ; v3n.fd.big -= 1 ; } ;
	    } else
	    { v3n.fd.dps += dp_rounding[dst_dps] ;
	      if (v3n.fd.dps >= 1000000000) { v3n.fd.dps -= 1000000000 ; v3n.fd.num += 1 ; } ;
	      if (v3n.fd.num >= 1000000000) { v3n.fd.num -= 1000000000 ; v3n.fd.big += 1 ; } ;
	    } ;
	 } ;
	ZCB
	if (v3n.fd.big != 0)
	 { n = v3n.fd.big ; b = &cb.v3n.big[8] ; for(;n>0;) { *(b--) = (n %10)+'0' ; n /= 10 ; } ; } ;
	n = v3n.fd.num ; b = &cb.v3n.num[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
	n = v3n.fd.dps ; b = &cb.v3n.dps[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
/*	Now move portion to destination string */
	src = (src = (char *)&cb) + 18 + dst_dps - dst_len ;	/* src = pointer to correct spot in conversion buffer */
	dst = dst_ptr ; for(i=dst_len;i>0;i--) { *(dst++) = *(src++) ; } ;	/* Copy correct number of bytes */
	if (neg) { dst-- ; *dst = *dst - '0' + 'p' ; } ;
	return ;
badnum:	v3_error(V3E_BADVAL,"XCT","MTH","INVDECNUM","Invalid contents within zoned-decimal number",0) ;
}

/*	Converts string to v3num			*/

void mthu_cvtsv(src_ptr,src_len,src_dps,v3n)
  char *src_ptr ; int src_len,src_dps ;
  union val__v3num *v3n ;
{ union cnv__buf cb ;
   int i,n,neg ; char *src,*srcptr ;

/*	Convert dec to v3num	*/
	ZCB
	src = (src = (char *)&cb) + 18 + src_dps - src_len ;
//	for(srcptr=src_ptr,i=src_len;i>0;i--) { if (*srcptr < '0') goto badnum ; *(src++) = *(srcptr++) ; } ;
	for(srcptr=src_ptr,i=src_len;i>0;i--) GETDIGIT(src,srcptr) ;
/*	Check for sign */
	if (*(--src) <= '9') { neg = FALSE ; }
	 else { neg = TRUE ; *src -= ('p'-'0') ; } ;
/*	Convert the three portions */
	v3n->fd.big = 0 ; for(i=0,src=(char *)&cb.v3n.big;i<=8;i++) { v3n->fd.big *= 10 ; v3n->fd.big += *(src++) - '0' ; } ;
	v3n->fd.num = 0 ; for(i=0,src=(char *)&cb.v3n.num;i<=8;i++) { v3n->fd.num *= 10 ; v3n->fd.num += *(src++) - '0' ; } ;
	v3n->fd.dps = 0 ; for(i=0,src=(char *)&cb.v3n.dps;i<=8;i++) { v3n->fd.dps *= 10 ; v3n->fd.dps += *(src++) - '0' ; } ;
/*	Adjust for negatives */
	if (neg)
	 { v3n->fd.big = -v3n->fd.big ; v3n->fd.num = -v3n->fd.num ; v3n->fd.dps = -v3n->fd.dps ;
	 } ;
	return ;
badnum:	v3_error(V3E_BADVAL,"XCT","MTH","INVDECNUM","Invalid contents within zoned-decimal number",0) ;
}

void mthu_cvtid(intnum,int_dps,dbl_ptr)		/* Convert integer to double real */
 B64INT intnum ; int int_dps ; double *dbl_ptr ;
{ 
	*dbl_ptr = intnum / dbl_tens[int_dps] ;
}

int mthu_cvtsi(dec_ptr,dec_len,dec_dps,res_dps)
  char *dec_ptr ; int dec_len,dec_dps,res_dps ;
{ union val__v3num v3n ; union cnv__buf cb ;
   int i,neg ; char *src,*decptr ;

/*	Convert dec to v3num	*/
	ZCB
	src = (src = (char *)&cb) + 18 + dec_dps - dec_len ;
//	for(decptr=dec_ptr,i=dec_len;i>0;i--) { if (*decptr < '0') goto badnum ; *(src++) = *(decptr++) ; } ;
	for(decptr=dec_ptr,i=dec_len;i>0;i--) GETDIGIT(src,decptr) ; 
/*	Check for sign */
	if (*(--src) <= '9') { neg = FALSE ; }
	 else { neg = TRUE ; *src -= ('p'-'0') ; } ;
/*	Convert the three portions */
	v3n.fd.big = 0 ; for(i=0,src=(char *)&cb.v3n.big;i<=8;i++) { v3n.fd.big *= 10 ; v3n.fd.big += *(src++) - '0' ; } ;
	v3n.fd.num = 0 ; for(i=0,src=(char *)&cb.v3n.num;i<=8;i++) { v3n.fd.num *= 10 ; v3n.fd.num += *(src++) - '0' ; } ;
	v3n.fd.dps = 0 ; for(i=0,src=(char *)&cb.v3n.dps;i<=8;i++) { v3n.fd.dps *= 10 ; v3n.fd.dps += *(src++) - '0' ; } ;
/*	Adjust for negatives */
	if (neg) { v3n.fd.big = -v3n.fd.big ; v3n.fd.num = -v3n.fd.num ; v3n.fd.dps = -v3n.fd.dps ; } ;
/*	Now return integer version */
	if (res_dps == 0)
	 { if (v3n.fd.dps >= 500000000) { v3n.fd.num += 1 ; } else { if (v3n.fd.dps < -500000000) v3n.fd.num -= 1 ; } ;
	   return(v3n.fd.num) ;
	 } ;
	if (v3n.fd.dps < 0) { v3n.fd.dps -= dp_rounding[res_dps] ; } else v3n.fd.dps += dp_rounding[res_dps] ;
	return( v3n.fd.num * pow_tens[res_dps] + v3n.fd.dps / dp_scale[res_dps] ) ;
badnum:	v3_error(V3E_BADVAL,"XCT","MTH","INVDECNUM","Invalid contents within zoned-decimal number",0) ;
	return(0) ;
}

int mthu_cvtdi(dbl_ptr,res_dps)		/* Convert double-real to integer */
    double *dbl_ptr ; int res_dps ;
{ int res ;

	return(res = *dbl_ptr * dbl_tens[res_dps]) ;
}

void mthu_cmppd() {}

void mthu_divpd() {}

void mthu_percpd() {}

void mthu_subpd() {}

int mthu_cvtpi() { return(0) ; }

/*	mthu_cvtatr - Converts anum to real			*/

void mthu_cvtatr(anum,dptr)
   struct num__ref *anum ;
   double *dptr ;
{ union val__v3num v3n,*v3np ;
  int i ; short int *si ;

	switch (anum->format.fld.type)
	 { default: v3_error(V3E_INVSQRTARG,"XCT","SQRT","INVSQRTARG","Invalid value for REAL argument",0) ;
	   case VFT_BINWORD:
		si = (short int *)anum->ptr ;
		*dptr = (double)(*si / dbl_tens[anum->format.fld.decimals]) ; break ;
	   case VFT_BINLONG:
		*dptr = *anum->ptr / dbl_tens[anum->format.fld.decimals] ; break ;
	   case VFT_BININT:
		i = *(int *)anum->ptr ;
		*dptr = i / dbl_tens[anum->format.fld.decimals] ; break ;
	   case VFT_FLOAT:
		*dptr = *anum->dbl_ptr ; break ;
	   case	VFT_V3NUM:
		v3np = (union val__v3num *)anum->ptr ;
		if (v3np->fp.marker == V3NUM_FPMARK) { *dptr = v3np->fp.dbl ; }
		 else { *dptr = v3np->fd.big*1E9 + v3np->fd.num + v3np->fd.dps/1E9 ; } ;
		break ;
	   case VFT_STRINT:
		mthu_cvtsv((char *)anum->ptr,anum->format.fld.length,anum->format.fld.decimals,&v3n) ;
		if (v3n.fp.marker == V3NUM_FPMARK) { *dptr = v3n.fp.dbl ; }
		 else { *dptr = v3n.fd.big*1E9 + v3n.fd.num + v3n.fd.dps/1E9 ; } ;
		break ;
	 } ;
}

/*	mthu_cvtvs - Converts V3 Internal Number to Zoned String */

void mthu_cvtvs(v3n,ptr,len,dps)
 union val__v3num *v3n ;
 char *ptr ;				/* Pointer to Zoned String */
 int len,dps ;			/* Length & Decimals */
{ union cnv__buf cb ;
  union val__v3num v3n1 ;
  int i,neg,n ; char *b,*src,*dst ; char buf[30] ;

/*	Check for floating point representation */
	if (v3n->fp.marker == V3NUM_FPMARK)
	 { if (v3n->fp.dbl < 0 ) { neg = TRUE ; v3n->fp.dbl = -v3n->fp.dbl ; } else neg = FALSE ;
	   sprintf(buf,"%028.9f",v3n->fp.dbl) ;
	   if (buf[19] == '.')				/* Check for possible OSF/ALPHA bug ? */
	    { memcpy(cb.all.buf,&buf[1],18) ; memcpy(&cb.all.buf[18],&buf[20],9) ;
	    } else
	    { memcpy(cb.all.buf,buf,18) ; memcpy(&cb.all.buf[18],&buf[19],9) ; } ;
	   goto float_skip ;
	 } ;
	v3n1 = *v3n ; v3n = &v3n1 ;
	if (v3n->fd.num > 0) { neg = FALSE ; }
	 else if (v3n->fd.big > 0) { neg = FALSE ; } else if (v3n->fd.dps > 0) { neg = FALSE ; }
	 else { neg = TRUE ; v3n->fd.big = -v3n->fd.big ; v3n->fd.num = -v3n->fd.num ; v3n->fd.dps = -v3n->fd.dps ; } ;
//	if (dps == 0)
//	 { if (v3n->fd.dps >= 500000000) { v3n->fd.num += 1 ; } else { if (v3n->fd.dps < -500000000) v3n->fd.num -= 1 ; } ;
//	 } else { if (v3n->fd.dps < 0) { v3n->fd.dps -= dp_rounding[dps] ; } else v3n->fd.dps += dp_rounding[dps] ; } ;

	if (dps == 0)
	 { if (v3n->fd.dps >= 500000000)
	    { v3n->fd.num += 1 ;
	      if (v3n->fd.num >= 1000000000) { v3n->fd.num -= 1000000000 ; v3n->fd.big += 1 ; } ;
	    } ;
	 } else
	 { v3n->fd.dps += dp_rounding[dps] ;
	   if (v3n->fd.dps >= 1000000000) { v3n->fd.dps -= 1000000000 ; v3n->fd.num += 1 ; } ;
	   if (v3n->fd.num >= 1000000000) { v3n->fd.num -= 1000000000 ; v3n->fd.big += 1 ; } ;
	 } ;


	ZCB
	if (v3n->fd.big != 0)
	 { n = v3n->fd.big ; b = &cb.v3n.big[8] ; for(;n>0;) { *(b--) = (n %10)+'0' ; n /= 10 ; } ; } ;
	n = v3n->fd.num ; b = &cb.v3n.num[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
	n = v3n->fd.dps ; b = &cb.v3n.dps[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
float_skip:
/*	Now copy proper portion of cb back into zoned string */
	src = (src = (char *)&cb) + 18 + dps - len ; dst = ptr ;
	for(i=len;i>0;i--) { *(dst++) = *(src++) ; } ;
	if (neg) { dst-- ; *dst = *dst - '0' + 'p' ; } ;
}

/*	v3num_cvtcb - Converts V3 Internal Number to Internal Conversion buffer (cnv__buf) */

char *v3num_cvtcb(v3n,dps,retsign)
 union val__v3num *v3n ;
 int dps ;				/* Number of decimal places */
 int *retsign ;				/* Updated with sign */
{ static union cnv__buf cb ;
  union val__v3num v3n1 ;
  int i,sign,n ; char *b,*src ; char buf[30] ;

/*	Check the sign */
	if (v3n->fd.big == 0 && v3n->fd.num == 0 && v3n->fd.dps == 0) { *retsign = 1 ; return(0) ; } ;
/*	Check for floating point representation */
	if (v3n->fp.marker == V3NUM_FPMARK)
	 { if (v3n->fp.dbl < 0 ) { sign = -1 ; v3n->fp.dbl = -v3n->fp.dbl ; } else sign = 1 ;
	   sprintf(buf,"%028.9f",v3n->fp.dbl) ;
	   if (buf[19] == '.')				/* Check for possible OSF/ALPHA bug ? */
	    { memcpy(cb.all.buf,&buf[1],18) ; memcpy(&cb.all.buf[18],&buf[20],9) ;
	    } else
	    { memcpy(cb.all.buf,buf,18) ; memcpy(&cb.all.buf[18],&buf[19],9) ; } ;
	   for(i=0;i<18 && cb.all.buf[i]=='0';i++) { cb.all.buf[i] = NULLV ; } ;
	   goto float_skip ;
	 } ;
	v3n1 = *v3n ; v3n = &v3n1 ;
	if (v3n->fd.num > 0) { sign = 1 ; }
	 else if (v3n->fd.big > 0) { sign = 1 ; } else if (v3n->fd.dps > 0) { sign = 1 ; }
	 else { sign = -1 ; v3n->fd.big = -v3n->fd.big ; v3n->fd.num = -v3n->fd.num ; v3n->fd.dps = -v3n->fd.dps ; } ;
	if (dps == 0)
	 { if (v3n->fd.dps >= 500000000)
	    { v3n->fd.num += 1 ;
	      if (v3n->fd.num >= 1000000000) { v3n->fd.num -= 1000000000 ; v3n->fd.big += 1 ; } ;
	    } else
	    { if (v3n->fd.dps <= -500000000) v3n->fd.num -= 1 ;
	      if (v3n->fd.num <= -1000000000) { v3n->fd.num += 1000000000 ; v3n->fd.big -= 1 ; } ;
	    } ;
	 } else
	 { if (v3n->fd.dps < 0)
	    { v3n->fd.dps -= dp_rounding[dps] ;
	      if (v3n->fd.dps <= -1000000000) { v3n->fd.dps += 1000000000 ; v3n->fd.num -= 1 ; } ;
	      if (v3n->fd.num <= -1000000000) { v3n->fd.num += 1000000000 ; v3n->fd.big -= 1 ; } ;
	    } else
	    { v3n->fd.dps += dp_rounding[dps] ;
	      if (v3n->fd.dps >= 1000000000) { v3n->fd.dps -= 1000000000 ; v3n->fd.num += 1 ; } ;
	      if (v3n->fd.num >= 1000000000) { v3n->fd.num -= 1000000000 ; v3n->fd.big += 1 ; } ;
	    } ;
	 } ;
	ZCB
	if (v3n->fd.big != 0)
	 { n = v3n->fd.big ; b = &cb.v3n.big[8] ; for(;n>0;) { *(b--) = (n %10)+'0' ; n /= 10 ; } ; } ;
	n = v3n->fd.num ; b = &cb.v3n.num[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
	n = v3n->fd.dps ; b = &cb.v3n.dps[8] ; for(;n>0;) { *(b--) = (n%10)+'0' ; n /= 10 ; } ;
/*	Get rid of leading 0's */
	for(i=0;i<18 && cb.all.buf[i]=='0';i++) { cb.all.buf[i] = NULLV ; } ;
/*	Return +/- ptr to last digit, 0 if result is zero */
float_skip:
	*retsign = (sign < 0 ? -1 : 1) ;	/* Update with sign of result */
	return((char *)&cb.v3n.dps + dps - 1) ;
}

/*	v3num_popv3n - Pop a V3 Internal Number & returns number of decimal places */

int v3num_popv3n(v3n)
 union val__v3num *v3n ;
{ union val__format format ;
  union val__v3num *v3n1 ; union cnv__buf cb ;
  B64INT lnum,lnum1,*lnump ; int sign ;
  int inum,*inump,neg,i,*(*mp_ptr) ; short *wnump ; double *dblp ; char *src,*dst ;

/*	See what kind of number we got */
	POPF(format.all) ;
retry_indirect:
	switch (format.fld.type)
	 { case VFT_INDIRECT:
		POPVP(inump,(int *)) ; format.all = *inump ; PUSHVI(*(inump+1)) ; goto retry_indirect ;
	   case VFT_POINTER:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVP(inump,(int *)) ; }
		 else { POPVP(mp_ptr,(int *(*))) ; inump = *mp_ptr ; } ;
		v3n->fd.big = 0 ; v3n->fd.num = *inump ; v3n->fd.dps = 0 ;
		break ;
	   case VFT_BINLONG:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVI(lnum) ; }
		 else { POPVP(lnump,(B64INT *)) ; lnum = *lnump ; } ;
		if (lnum < 0) { lnum = -lnum ; sign = TRUE ; } else { sign = FALSE ; } ;
		v3n->fd.big = 0 ;
		if (format.fld.decimals == 0)
		 { v3n->fd.big = lnum / 1000000000 ; v3n->fd.num = lnum % 1000000000 ; v3n->fd.dps = 0 ; }
		 else { lnum1 = lnum / pow_tens[format.fld.decimals] ;
			v3n->fd.big = lnum1 / 1000000000 ; v3n->fd.num = lnum1 % 1000000000 ;
			v3n->fd.dps = (lnum % pow_tens[format.fld.decimals]) * dp_scale[format.fld.decimals] ;
		      } ;
		if (sign) { v3n->fd.big = -v3n->fd.big ; v3n->fd.num = -v3n->fd.num ; v3n->fd.dps = -v3n->fd.dps ; } ;
		break ;
	   case VFT_BININT:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVI(inum) ; }
		 else { POPVP(inump,(int *)) ; inum = *inump ; } ;
		v3n->fd.big = 0 ;
		if (format.fld.decimals == 0)
		 { v3n->fd.big = inum / 1000000000 ; v3n->fd.num = inum % 1000000000 ; v3n->fd.dps = 0 ; }
		 else { v3n->fd.num = inum / pow_tens[format.fld.decimals] ;
			v3n->fd.dps = (inum % pow_tens[format.fld.decimals]) * dp_scale[format.fld.decimals] ;
		      } ;
		break ;
	   case VFT_BINWORD:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVI(inum) ;}
		 else { POPVP(wnump,(short int *)) ; inum = *wnump ; } ;
		v3n->fd.big = 0 ;
		if (format.fld.decimals == 0)
		 { v3n->fd.num = inum ; v3n->fd.dps = 0 ; }
		 else { v3n->fd.num = inum / pow_tens[format.fld.decimals] ;
			v3n->fd.dps = (inum % pow_tens[format.fld.decimals]) * dp_scale[format.fld.decimals] ;
		      } ;
		break ;
	   case VFT_V3NUM:
		POPVP(v3n1,(union val__v3num *)) ; *v3n = *v3n1 ; break ;
	   case VFT_FLOAT:
		POPVP(dblp,(double *)) ; v3n->fp.dbl = *dblp ; v3n->fp.marker = V3NUM_FPMARK ;
		return(4) ;				/* Return default of 4 decimal places */
	   case VFT_STRINT:
/*		Convert dec to v3num	*/
		neg = FALSE ; ZCB
		POPVP(dst,(char *)) ; src = (src = (char *)&cb) + 18 + format.fld.decimals - format.fld.length ;
//		for(i=format.fld.length;i>0;i--) { if (*dst < '0') goto badnum ; *(src++) = *(dst++) ; } ;
		for(i=format.fld.length;i>0;i--) GETDIGIT(src,dst) ;
/*		Check for sign */
		if (*(--src) <= '9') { neg = FALSE ; }
		 else { neg = TRUE ; *src -= ('p'-'0') ; } ;
/*		Convert the three portions */
		v3n->fd.big = 0 ; for(i=0,src=(char *)&cb.v3n.big;i<=8;i++) { v3n->fd.big *= 10 ; v3n->fd.big += *(src++) - '0' ; } ;
		v3n->fd.num = 0 ; for(i=0,src=(char *)&cb.v3n.num;i<=8;i++) { v3n->fd.num *= 10 ; v3n->fd.num += *(src++) - '0' ; } ;
		v3n->fd.dps = 0 ; for(i=0,src=(char *)&cb.v3n.dps;i<=8;i++) { v3n->fd.dps *= 10 ; v3n->fd.dps += *(src++) - '0' ; } ;
/*		Adjust for negatives */
		if (neg) { v3n->fd.big = -v3n->fd.big ; v3n->fd.num = -v3n->fd.num ; v3n->fd.dps = -v3n->fd.dps ; } ;
		break ;
	   case VFT_PACDEC:
	   default:
		v3_error(V3E_INVNUMTYPE,"XCT","POPV3NUM","INVNUMTYPE","Invalid datatype for pop_v3_num",(void *)format.fld.type) ;
	 } ;
/*	Return number of decimal places */
	return(format.fld.decimals) ;
badnum:	v3_error(V3E_BADVAL,"XCT","MTH","INVDECNUM","Invalid contents within zoned-decimal number",(void *)0) ;
	return(0) ;
}

/*	v3num_cmp - Compares 2 V3NUMs & returns -1/0/+1	*/

int v3num_cmp(v3n1,v3n2)
 union val__v3num *v3n1,*v3n2 ;
{ double dbl ;

/*	Are we comparing fixed decimal or floating ? */
	if (v3n1->fp.marker == V3NUM_FPMARK)
	 { if (v3n2->fp.marker == V3NUM_FPMARK) { dbl = v3n2->fp.dbl ; }
	    else dbl = v3n2->fd.big*1E9+v3n2->fd.num+v3n2->fd.dps/1E9 ;
	   return(v3n1->fp.dbl>dbl ? 1 : (v3n1->fp.dbl<dbl ? -1 : 0)) ;
	 } else
	 { if (v3n2->fp.marker == V3NUM_FPMARK)
	    { if (v3n1->fp.marker == V3NUM_FPMARK) { dbl = v3n1->fp.dbl ; }
		else dbl = v3n1->fd.big*1E9+v3n1->fd.num+v3n1->fd.dps/1E9 ;
	      return(dbl>v3n2->fp.dbl ? 1 : (dbl<v3n2->fp.dbl ? -1 : 0)) ;
	    } ;
	 } ;
/*	Comparing fixed decimal numbers */
	if (v3n1->fd.big > v3n2->fd.big) return(1) ;
	if (v3n1->fd.big < v3n2->fd.big) return(-1) ;
	if (v3n1->fd.num > v3n2->fd.num) return(1) ;
	if (v3n1->fd.num < v3n2->fd.num) return(-1) ;
	if (v3n1->fd.dps > v3n2->fd.dps) return(1) ;
	if (v3n1->fd.dps < v3n2->fd.dps) return(-1) ;
	return(0) ;
}

/*	xctu_popint - Pops off integer value */

B64INT xctu_popint()
{ union val__format format ;
  union val__v3num *v3n,v3n1 ; union cnv__buf cb ;
  B64INT lnum,*lnump ;
  short *wnump ; int i,inum,*inump,neg ; double *dnump,dbl ; char *src,*dst ; char *(*mp_ptr) ;
  extern double dpowers[]; 

	POPF(format.all) ;
	switch (format.fld.type)
	 { case VFT_BININT:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVI(inum) ; }
		 else { POPVP(inump,(int *)) ; inum = *inump ; } ;
		if (format.fld.decimals == 0) return(inum) ;
		return((inum+rounding[format.fld.decimals])/pow_tens[format.fld.decimals]) ;
	   case VFT_BINLONG:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVI(lnum) ; }
		 else { POPVP(lnump,(B64INT *)) ; lnum = *lnump ; } ;
		if (format.fld.decimals == 0) return(lnum) ;
		return((lnum+rounding[format.fld.decimals])/pow_tens[format.fld.decimals]) ;
	   case VFT_BINWORD:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVI(inum) ; }
		 else { POPVP(wnump,(short int *)) ; inum = *wnump ; } ;
		if (format.fld.decimals == 0) return(inum) ;
		return((inum+rounding[format.fld.decimals])/pow_tens[format.fld.decimals]) ;
	   case VFT_POINTER:
		if (format.fld.mode == VFM_IMMEDIATE) { POPVP(src,(char *)) ; }
		 else { POPVP(mp_ptr,(char *(*))) ; src = *mp_ptr ; } ;
		return(src == NULLV ? 0 : 1) ;
	   case VFT_FLOAT:
		POPVP(dnump,(double *)) ; inum = *dnump ; return(inum) ;
	   case VFT_V3NUM:
		POPVP(v3n,(union val__v3num *)) ;
		if (v3n->fp.marker == V3NUM_FPMARK)
		 { inum = v3n->fp.dbl ; return(inum) ; } ;
		if (v3n->fd.big != 0)
		 v3_error(V3E_TOOBIGFORINT,"MTHU","POPINT","TOOBIGFORINT","Number too big to convert to integer",0) ;
		return(v3n->fd.num) ;
	   case VFT_PACDEC:
		return(0) ;
	   case VFT_STRINT:
/*		Convert dec to v3num	*/
		neg = FALSE ; ZCB
//		src = (src = (char *)&cb) + 18 + format.fld.decimals - format.fld.length ;
		POPVP(dst,(char *)) ;
		if (dst[format.fld.length-1] > '9')
		 { neg = TRUE ; dst[format.fld.length-1] -= ('p'-'0') ; } ;
		dbl = 0 ;
		for(i=0;i<format.fld.length;i++) { dbl = dbl * 10 + (dst[i]-'0') ; } ;
		if (format.fld.decimals > 0) { dbl /= dpowers[format.fld.decimals] ; } ;
		if (dbl > V4LIM_BiggestPositiveInt)
		 v3_error(V3E_TOOBIGFORINT,"MTHU","POPINT","TOOBIGFORINT","Number too big to convert to integer",0) ;
		i = nint(dbl) ;
		return(neg ? -i : i) ;
//		for(i=format.fld.length;i>0;i--) { *(src++) = *(dst++) ; } ;
///*		Check for sign */
//		if (*(--src) <= '9') { neg = FALSE ; }
//		 else { neg = TRUE ; *src -= ('p'-'0') ; } ;
///*		Convert the three portions */
//		v3n1.fd.big = 0 ; for(i=0,src=(char *)&cb.v3n.big;i<=8;i++) { v3n1.fd.big *= 10 ; v3n1.fd.big += *(src++) - '0' ; } ;
//		v3n1.fd.num = 0 ; for(i=0,src=(char *)&cb.v3n.num;i<=8;i++) { v3n1.fd.num *= 10 ; v3n1.fd.num += *(src++) - '0' ; } ;
//		v3n1.fd.dps = 0 ; for(i=0,src=(char *)&cb.v3n.dps;i<=8;i++) { v3n1.fd.dps *= 10 ; v3n1.fd.dps += *(src++) - '0' ; } ;
///*		Adjust for negatives */
//		if (neg) { v3n1.fd.big = -v3n1.fd.big ; v3n1.fd.num = -v3n1.fd.num ; v3n1.fd.dps = -v3n1.fd.dps ; } ;
//		if (v3n1.fd.dps > 500000000) { v3n1.fd.num += 1 ; } else { if (v3n1.fd.dps < -500000000) v3n1.fd.num -= 1 ; } ;
//		return(v3n1.fd.num) ;
	 } ;
	return(0) ;
}

/*	v3num_mdas - Performs Basic Operations on V3NUMs */
/*	Note: v3n1 is updated to result */

void v3num_mdas(v3n1,v3n2,oper)
  union val__v3num *v3n1,*v3n2 ;
  int oper ;
{ int sgn1,sgn2 ; double dbl ;

	switch (oper)
	 { case 4:	/* Subtraction */
		if (v3n2->fp.marker == V3NUM_FPMARK) { v3n2->fp.dbl = -v3n2->fp.dbl ; }
		 else { v3n2->fd.big = -v3n2->fd.big ; v3n2->fd.num = -v3n2->fd.num ; v3n2->fd.dps = -v3n2->fd.dps ; } ;
/*		Just fall thru to addition */
	   case 3:	/* Addition */
		if (v3n1->fp.marker == V3NUM_FPMARK)
		 { if (v3n2->fp.marker == V3NUM_FPMARK)
		    { v3n1->fp.dbl += v3n2->fp.dbl ; }
		    else { v3n1->fp.dbl += v3n2->fd.big*1E9+v3n2->fd.num+v3n2->fd.dps/1E9 ; } ;
		   return ;
		 } else
		 { if (v3n2->fp.marker == V3NUM_FPMARK)
		    { if (v3n1->fp.marker == V3NUM_FPMARK)
		       { v3n1->fp.dbl += v3n2->fp.dbl ; }
		       else { v3n1->fp.dbl = v3n1->fd.big*1E9+v3n1->fd.num+v3n1->fd.dps/1E9 + v3n2->fp.dbl ;
			      v3n1->fp.marker = V3NUM_FPMARK ;
			    } ;
		      return ;
		    } ;
		 } ;
/*		Not floating point - Handle fixed point Addition/Subtraction */
		sgn1 = v3n1->fd.big | v3n1->fd.num | v3n1->fd.dps ; sgn1 = (sgn1 < 0 ? -1 : 1) ;
		sgn2 = v3n2->fd.big | v3n2->fd.num | v3n2->fd.dps ; sgn2 = (sgn2 < 0 ? -1 : 1) ;
		v3n1->fd.big += v3n2->fd.big ; v3n1->fd.num += v3n2->fd.num ; v3n1->fd.dps += v3n2->fd.dps ;
		if (sgn1 == sgn2)
		 { if (sgn1 < 0)
		    { if (v3n1->fd.dps <= -V3NUM_MAX) { v3n1->fd.dps += V3NUM_MAX ; v3n1->fd.num -= 1 ; }
		      if (v3n1->fd.num <= -V3NUM_MAX) { v3n1->fd.num += V3NUM_MAX ; v3n1->fd.big -= 1 ; } ;
		    } else
		    { if (v3n1->fd.dps >= V3NUM_MAX) { v3n1->fd.dps -= V3NUM_MAX ; v3n1->fd.num += 1 ; } ;
		      if (v3n1->fd.num >= V3NUM_MAX) { v3n1->fd.num -= V3NUM_MAX ; v3n1->fd.big += 1 ; } ;
		    } ;
		 } else
		 { sgn1 = (v3n1->fd.big != 0 ? v3n1->fd.big : (v3n1->fd.num != 0 ? v3n1->fd.num : v3n1->fd.dps)) ;
		   if (sgn1 >= 0)
		    { if (v3n1->fd.dps < 0) { v3n1->fd.dps += V3NUM_MAX ; v3n1->fd.num -= 1 ; } ;
		      if (v3n1->fd.num < 0) { v3n1->fd.num += V3NUM_MAX ; v3n1->fd.big -= 1 ; } ;
		    } else
		    { if (v3n1->fd.dps > 0) { v3n1->fd.dps -= V3NUM_MAX ; v3n1->fd.num += 1 ; } ;
		      if (v3n1->fd.num > 0) { v3n1->fd.num -= V3NUM_MAX ; v3n1->fd.big += 1 ; } ;
		    } ;
		 } ;
		return ;
	   case 2:	/* Division */
		if (v3n1->fp.marker == V3NUM_FPMARK)
		 { if (v3n2->fp.marker == V3NUM_FPMARK)
		    { v3n1->fp.dbl /= v3n2->fp.dbl ; return ; } ;
		   v3n1->fp.dbl /= (v3n2->fd.big*1E9+v3n2->fd.num+v3n2->fd.dps/1E9) ;
		   return ;
		 } ;
		dbl = v3n1->fd.big*1E9+v3n1->fd.num+v3n1->fd.dps/1E9 ;
		v3n1->fp.marker = V3NUM_FPMARK ;
		if (v3n2->fp.marker == V3NUM_FPMARK)
		 { v3n1->fp.dbl = dbl / v3n2->fp.dbl ; return ; } ;
		v3n1->fp.dbl = dbl / (v3n2->fd.big*1E9+v3n2->fd.num+v3n2->fd.dps/1E9) ;
		return ;
	   case 1:	/* Multiplication */
		if (v3n1->fp.marker == V3NUM_FPMARK)
		 { if (v3n2->fp.marker == V3NUM_FPMARK)
		    { v3n1->fp.dbl *= v3n2->fp.dbl ; return ; } ;
		   v3n1->fp.dbl *= (v3n2->fd.big*1E9+v3n2->fd.num+v3n2->fd.dps/1E9) ;
		   return ;
		 } ;
		dbl = v3n1->fd.big*1E9+v3n1->fd.num+v3n1->fd.dps/1E9 ;
		v3n1->fp.marker = V3NUM_FPMARK ;
		if (v3n2->fp.marker == V3NUM_FPMARK)
		 { v3n1->fp.dbl = dbl * v3n2->fp.dbl ; return ; } ;
		v3n1->fp.dbl = dbl * (v3n2->fd.big*1E9+v3n2->fd.num+v3n2->fd.dps/1E9) ;
		return ;
	 } ;
}

/*	v3num_itx - Formats for general print */

void v3num_itx(v3n,str,dps)
  union val__v3num *v3n ;		/* Number to be converted */
  char *str ;				/* Pointer to C string */
  int dps ;				/* Number of decimal places */
{ char *b,sign ; int i,signflag ;

/*	Call v3num_cvtcb to do all the hard work */
	b = v3num_cvtcb(v3n,dps,&signflag) ;
	if (b == NULLV)
	 { strncpy(str,"0.00000000000",(dps == 0 ? 1 : dps+2)) ; str[dps==0 ? 1 : dps+2] = NULLV ;
	   return ;
	 } ;
	if (signflag < 0) { sign = '-' ; } else { sign = NULLV ; } ;
	if (dps > 0)
	 { b += 1 ; for(i=1;i<=dps;i++) { *b = *(b-1) ; b-- ; } ; *b = '.' ; b += dps ; } ;
	*(b+1) = NULLV ;			/* Zap next byte to make it a valid string */
	for(;*b != NULLV;b--) { } ;	/* Scan back to begin of number */
	if (sign != NULLV) { *b = sign ; } else { b++ ; } ;
	strcpy(str,b) ;			/* Copy result into destination string */
	return ;
}
