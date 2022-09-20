/*	v_InitDblArray - Initializes array of double value		*/
/*	Call: da = v_InitDblArray( initialmax )
	  where da is pointer to new structure,
		initialmax is initial size, 0 or UNUSED if unknown	*/

struct V4IM_DblArray *v_InitDblArray(initialmax)
  int initialmax ;
{ struct V4IM_DblArray *da ;

	da = v4mm_AllocChunk((sizeof *da) + ((initialmax > 0 ? initialmax : V4IM_DblArrayMaxInit) * (sizeof(double))),FALSE) ;
	da->Max = (initialmax > 0 ? initialmax : V4IM_DblArrayMaxInit) ; da->Count = 0 ;
	return(da) ;
}

struct V4IM_DblArray *v_EnlargeDblArray(da,newmax)
  struct V4IM_DblArray *da ;
  int newmax ;
{ int newsize ;

	if (newmax > 0) { newsize = newmax ; }
	 else { newsize = (((int)(da->Max * V4IM_DblArrayIncFactor)) > V4IM_DblArrayIncMax ? V4IM_DblArrayIncMax : (int)(da->Max * V4IM_DblArrayIncFactor)) ;
		newsize += da->Max ;
	      } ;
	da = (struct V4IM_DblArray *)realloc(da,(sizeof *da) + (newsize * (sizeof(double)))) ;
	if (da == NULL) v4_error(0,0,"V4UTIL","EnlargeArray","MEM","Could not reallocate memory") ;
	da->Max = newsize ;
	return(da) ;
}


/*	References:
	A. Capital Investment & Financial Decisions, Levy & Sarnat, Prentice Hall International, 1986
	B. Money & Capital Markets, Rose, BPI/Irwin, 1989
	C. Money Market Calculations, Sitgum, Irwin Professional Publishing, 1981
*/

/*	v4fin_CouponDateCalc - Calculates various coupon dates	*/
/*	Ref:	*/

#define V4FIN_CDCDaysBS 1	/* Days from beginning of coupon period to settlement */
#define V4FIN_CDCDays 2		/* Number of days in coupon period containing settlement date */
#define V4FIN_CDCDaysNC 3	/* Days from settlement to next coupon date */
#define V4FIN_CDCNCD 4		/* Next coupon date after settlement */
#define V4FIN_CDCNum 5		/* Number of coupons payable between settlement & maturity */
#define V4FIN_CDCPCD 6		/* Previous coupon date before settlement */
#define V4FIN_CDCDaysYr 7	/* Returns number of days in year corresponding to settlement (smdate) */

int v4fin_CouponDateCalc(type,smdate,matdate,freq,basis,ok)
  int type ;		/* Calculation type: V4FIN_CDCxxx */
  int smdate ;		/* Settlement date */
  int matdate ;		/* Maturity date */
  int freq ;		/* Frequency: 1/2/4 for annual, semi, quarterly */
  int basis ;		/* 0=30/360, 1=actual/actual, 2=actual/360, 3=actual/365 */
  LOGICAL *ok ;		/* Set to TRUE if OK, FALSE otherwise */
{ struct V4C__Context *ctx ;
  int i,sd,t ;

	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	*ok = TRUE ;				/* Assume all will work out */
	switch(freq)
	 { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"@Frequency (%1d) must be 1/2/4",freq) ; *ok = FALSE ; return(1) ;
	   case 1: case 2: case 4: break ;
	 } ;
	if (basis < 0 || basis > 3)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Basis (%1d) must be 0-3",basis) ; *ok = FALSE ; return(1) ; } ;
	if (smdate >= matdate)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Settltment date must be less than maturity date") ; *ok = FALSE ; return(1) ; } ;
	switch (type)
	 { default: v_Msg(ctx,ctx->ErrorMsgAux,"@Invalid calculation type (%1d)",type) ; *ok = FALSE ; return(1) ;
	    case V4FIN_CDCDaysBS:
		for(i=-1;;i--)
		 { sd = v4fin_CDCDate(matdate,freq,basis,i) ;	/* Get prior coupon date starting with maturity */
		   if (sd <= smdate) break ;
		 } ;
		return(smdate - sd) ;
	    case V4FIN_CDCDays:
		for(i=-1;;i--)
		 { sd = v4fin_CDCDate(matdate,freq,basis,i) ;
		   if (sd <= smdate) break ;
		 } ;
		t = v4fin_CDCDate(matdate,freq,basis,i+1) ;
		return(t - sd) ;
	    case V4FIN_CDCDaysNC:
		for(i=-1;;i--)
		 { sd = v4fin_CDCDate(matdate,freq,basis,i) ;
		   if (sd <= smdate) break ;
		 } ;
		t = v4fin_CDCDate(matdate,freq,basis,i+1) ;
		return(t - smdate) ;
	    case V4FIN_CDCNCD:
		for(i=-1;;i--)
		 { sd = v4fin_CDCDate(matdate,freq,basis,i) ;
		   if (sd <= smdate) break ;
		 } ;
		return(v4fin_CDCDate(matdate,freq,basis,i+1)) ;
	    case V4FIN_CDCNum:
		for(i=-1;;i--)
		 { sd = v4fin_CDCDate(matdate,freq,basis,i) ;
		   if (sd <= smdate) break ;
		 } ;
		return((sd == smdate ? 1 : 0) - i) ;
	    case V4FIN_CDCPCD:
		for(i=-1;;i--)
		 { sd = v4fin_CDCDate(matdate,freq,basis,i) ;
		   if (sd < smdate) break ;
		 } ;
		return(sd) ;
	    case V4FIN_CDCDaysYr:
		switch(basis)		/* 0=30/360, 1=actual/actual, 2=actual/360, 3=actual/365 */
		 { case 0:	return(360) ;
		   case 1:	sd = mscu_udate_to_yyyymmdd(smdate) ;	/* Have to look at actual year */
				i = sd / 10000 ;
				return((i%4) == 0 ? 366 : 365) ;
		   case 2:	return(360) ;
		   case 3:	return(365) ;
		 } ;
	 } ;
	return(0) ;
}

/*	v4fin_CDCDate - Determines relative (monthly) dates off of a base date */

int v4fin_CDCDate(base,freq,basis,off)
  int base,freq,basis,off ;
{ int ymd,nmon,y,m,d,o,last=FALSE ;

	if (off == 0) return(base) ;
	ymd = mscu_udate_to_yyyymmdd(base) ;	/* Convert date to yyyymmdd */
	y = ymd / 10000 ; m = (ymd / 100) % 100 ; d = ymd % 100 ;

	switch(freq)
	 { default:
	   case 1:	nmon = 12 ; break ;
	   case 2:	nmon = 6 ; break ;
	   case 4:	nmon = 3 ; break ;
	 } ;
	switch(basis)
	 { case 0:
		switch(m)
		 { default:	last = (d == 31) ; break ;
		   case 4: case 6: case 9: case 11: last = (d == 30) ; break ;
		   case 2: last = (d >= 28) ; break ;
		 } ;
	   case 1:
		if (off < 0)
		 { off = -off ;
		   for(o=1;o<=off;o++) { m -= nmon ; if (m < 1) { y-- ; m += 12 ; } ; } ;
		   if (last)
		    { switch(m)
		       { default: d = 31 ; break ;
			 case 4: case 6: case 9: case 11: d = 30 ; break ;
			 case 2: d = ((y % 4) == 0 ? 29 : 28) ; break ;
		       } ;
		    } ;
		   return(mscu_ymd_to_ud(y,m,d)) ;
		 } ;
		for(o=1;o<=off;o++) { m += nmon ; if (m > 12) { y++ ; m -= 12 ; } ; } ;
		return(mscu_ymd_to_ud(y,m,d)) ;
	   case 2:
	   case 3:
		return(0) ;
	 } ;
	return(0) ;
}

/*	v4fin_CvtDec - Converts fractional dollar to decimal	*/
/*	Ref: veh	*/

double v4fin_CvtDec(fracamt,fraction)
  double fracamt ;	/* Fractional amount */
  double fraction ;	/* Fraction (8 for 1/8, 16 for 1/16) */
{ int intamt ;
  double f,decimal ;

	if (fraction < 10) { f = 10.0 ; }
	 else if (fraction < 100) { f = 100.0 ; }
	 else f = 1000.0 ;
	intamt = (int)floor(fracamt) ;
	decimal = fracamt - (double)intamt ;
	decimal *= f ;
	return(intamt + decimal/fraction) ;
}

/*	v4fin_CvtFrac - Converts decimal dollar to fractional	*/
/*	Ref: veh	*/

double v4fin_CvtFrac(decamt,fraction)
  double decamt ;	/* Decimal amount */
  double fraction ;	/* Fraction (8 for 1/8, 16 for 1/16) */
{ int intamt ;
  double f,decimal ;

	if (fraction < 10) { f = 10.0 ; }
	 else if (fraction < 100) { f = 100.0 ; }
	 else f = 1000.0 ;
	intamt = (int)floor(decamt) ;
	decimal = decamt - (double)intamt ;
	return(intamt + (fraction*decimal)/f) ;
}

/*	v4fin_Days360 - Determines number of days between 2 dates based on 360 day year	*/

int v4fin_Days360(date1,date2)
  int date1,date2 ;
{ int ymd,y1,y2,m1,m2,d1,d2,sign,tdate,days ;

	sign = date1 > date2 ; if (sign) { tdate = date1 ; date1 = date2 ; date2 = date1 ; } ;
	ymd = mscu_udate_to_yyyymmdd(date1) ;	/* Convert date to yyyymmdd */
	y1 = ymd / 10000 ; m1 = (ymd / 100) % 100 ; d1 = ymd % 100 ;
	ymd = mscu_udate_to_yyyymmdd(date2) ;
	y2 = ymd / 10000 ; m2 = (ymd / 100) % 100 ; d2 = ymd % 100 ;
	days = (y2-y1) * 360 ;
	days += (m2-m1) * 30 ;
	days += (d2 > 30 ? 30 : d2) - (d1 > 30 ? 30 : d1) ;
	return(sign ? -days : days) ;
}

/*	v4fin_DaysDif - Difference between 2 dates */
int v4fin_DaysDif(first,last,basis)
  int first,last,basis ;
{
	switch(basis)
	 { case 0:	return(v4fin_Days360(first,last)) ;
	   case 1:	return(last-first) ;
	   case 2:	return(last-first) ;
	   case 3:	return(last-first) ;
	 } ;
	return(last-first) ;
}

/*	v4fin_DepFDB - Depreciation using Fixed Declining Balance	*/
/*	Ref: excel	*/

double v4fin_DepFDB(cost,salvage,life,period,partial,ok)
  double cost ;		/* Initial cost */
  double salvage ;	/* Salvage amount and end of life */
  double life ;		/* Number of periods in lifetime */
  double period ;	/* Period in question */
  double partial ;	/* Number of months in first year */
  LOGICAL *ok ;
{ double rate,dep,cdep,rlife ; int i ;
  struct V4C__Context *ctx ;

	if (partial == 0) partial = 12 ;
	rlife = (partial == 12 ? life : life+1) ;	/* If not 12 months in first year, add another year to life for overrun */
	if (period > rlife)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"@Period (%1g) cannot exceed lifetime (%2g)",period,life) ; *ok = FALSE ; return(1.0) ; } ;
	rate = 1.0 - pow(salvage/cost,1.0/life) ;
	dep = cost * rate * partial/12.0 ;		/* Depreciation for first period */
	cdep = 0.0 ;
	for(i=2;i<=period;i++)				/* Calculate up for other periods */
	 { cdep += dep ;				/* Track cummulative */
	   dep = (cost - cdep) * rate ;
	 } ;
	*ok = TRUE ;
	if (period < rlife) return(dep) ;		/* OK up through last period in life */
	return( ((cost-cdep)*rate * (12.0 - partial)) / 12.0 ) ;
}

/*	v4fin_DepDDB - Double Declining Balance Depreciation	*/
/*	Ref: A.133	*/

double v4fin_DepDDB(cost,salvage,life,period,factor,partial,ok)
  double cost ;		/* Initial cost */
  double salvage ;	/* Salvage amount */
  double life ;		/* Lifetime */
  double period ;	/* Period in question */
  double factor ;	/* Depreciation factor (if <= 0 then 2.0) */
  double partial ;	/* If nonzero then partial first year */
  LOGICAL *ok ;
{ double d,dsum ;
  struct V4C__Context *ctx ;
  int cnt,i ;

	if(period > life)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"@Period (%1g) cannot exceed lifetime (%2g)",period,life) ; *ok = FALSE ; return(1.0) ; } ;
	d = 0.0 ; dsum = 0.0 ;
	if (factor <= 0) factor = 2.0 ;
	if (partial == 0.0) partial = 12.0 ;
	cnt = (int)(period < life ? period : life - 1) ;
	for(i=1;i<=cnt;i++)	/* Loop thru periods */
	 { d = (factor/life)*(cost-salvage-dsum) ;
	   if (i == 1) d *= (partial/12.0) ;		/* If partial first year (if not, partial == 12!) */
	   dsum += d ;
	 } ;
	*ok = TRUE ; return(period<life ? d : cost-salvage-dsum) ;
}

/*	v4fin_DepSLN - Straight Line Depreciation	*/
/*	Ref: A.133	*/

double v4fin_DepSLN(cost,salvage,life,period,partial,ok)
  double cost ;		/* Initial cost */
  double salvage ;	/* Salvage amount */
  double life ;		/* Lifetime */
  double period ;	/* Wanted period (only used if partial not zero) */
  double partial ;	/* Partial first year (if non zero) */
  LOGICAL *ok ;
{ struct V4C__Context *ctx ;

	*ok = TRUE ;
	if (partial == 0.0) return((cost-salvage)/life) ;
	if (period == 1.0) { return(((cost-salvage)/life)*(partial/12.0)) ; }
	 else if (period > 1.0 && period <= life) { return((cost-salvage)/life) ; }
	 else if (period == life + 1.0) { return(((cost-salvage)/life)*((12-partial)/12.0)) ; }
	 else { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
		v_Msg(ctx,ctx->ErrorMsgAux,"@Period (%1g) must be 1.0 thru %2g",period,life+1.0) ; *ok = FALSE ; return(1.0) ; } ;
}

/*	v4fin_DepSYD - Sum of Years Depreciation	*/
/*	Ref: A.133	*/

double v4fin_DepSYD(cost,salvage,life,period,partial,ok)
  double cost ;		/* Initial cost */
  double salvage ;	/* Salvage amount */
  double life ;		/* Lifetime */
  double period ;	/* Period in question */
  double partial ;	/* If nonzero then partial first year */
  LOGICAL *ok ;
{ double syd,d1,f,dpn,ladj ; int w ;
  struct V4C__Context *ctx ;

	*ok = TRUE ;
	syd = life*(life+1)/2 ;		/* Calc sum of years/periods */
	if(period > life)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"@Period (%1g) cannot exceed lifetime (%2g)",period,life) ; *ok = FALSE ; return(1.0) ; } ;
	if (partial == 0.0) return((life-period+1)*(cost-salvage)/syd) ;
	if (period == 1.0) { return((life/syd)*(partial/12.0)*(cost-salvage)) ; } ;
	ladj = life - (partial/12.0) ;
	d1 = (life/syd)*(partial/12.0)*(cost-salvage) ;
	w = DtoI(ladj) ; f = ladj - (double)w ;
	syd = (w+1)*(w+2*f)/2 ;
	dpn = ((ladj-period+2)/syd) * (cost-d1-salvage) ;
	return(dpn) ;
}

/*	v4fin_Disc - Discount Rate for a Security	*/
/*	Ref: B.359-360	*/

double v4fin_Disc(smdate,matdate,price,redemption,basis,ok)
  int smdate ;		/* Settlement date */
  int matdate ;		/* Maturity date */
  double price ;	/* Price per $100 face value */
  double redemption ;	/* Redemption value per $100 */
  int basis ;		/* 0=30/360, 1=actual/actual, 2=actual/360, 3=actual/365 */
  LOGICAL *ok ;
{ 
  struct V4C__Context *ctx ;

	if (basis < 0 || basis > 3)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"@Basis (%1d) must be 0-3",basis) ; *ok = FALSE ; return(1) ; } ;
	if (smdate >= matdate)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"@Settltment date must be less than maturity date") ; *ok = FALSE ; return(1) ; } ;
	*ok = TRUE ;
	switch(basis)
	 { case 0:
	     return(((redemption - price)/100.0) * (360.0/(v4fin_Days360(smdate,matdate)))) ;
	   case 1:
	     return(((redemption - price)/100.0) * (365.0/(matdate-smdate))) ;
	   case 2:
	     return(((redemption - price)/100.0) * (360.0/(matdate-smdate))) ;
	   case 3:
	     return(((redemption - price)/100.0) * (365.0/(matdate-smdate))) ;
	 } ;
	return(0.0) ;
}

/*	v4fin_FVSched - Future Value Based on Variable Interest Rates */
/*	Ref:		*/

double v4fin_FVSched(pv,count,rates)
  double pv ;		/* Present Value */
  int count ;		/* Number of periods */
  double rates[] ;	/* Cash flow */
{ int t ;
  double fv ;

	for(fv=pv,t=0;t<count;t++)
	 { fv *= (1.0 + rates[t]) ;
	 } ;
	return(fv) ;
}

/*	v4fin_IntAccrPeriodic - Accrued Interest for Security Paying Periodic Interest	*/

double v4fin_IntAccPeriodic(issue,first,settlement,rate,par,frequency,basis,ok)
  int issue,first ;	/* Issue & first-payment dates */
  int settlement ;
  double rate ;		/* Annual coupon rate */
  double par ;		/* Par value */
  int frequency ;	/* Frequency of coupon payments */
  int basis ;		/* Calendar */
  int *ok ;
{ double factor=0.0,res ;
  int A,B,lok ;

	for(A=first;;)
	 { B = v4fin_CouponDateCalc(V4FIN_CDCPCD,A-1,A,frequency,basis,&lok) ;
	   if (!lok) { *ok = FALSE ; return(0) ; } ;
	   if (B >= settlement) { A = B ; continue ; } ;
	   if (B <= settlement && B > issue)
	    { factor += (double)v4fin_DaysDif(B,settlement,basis)/(double)v4fin_DaysDif(B,A,basis) ; settlement = B ; A = B ; continue ;} ;
	   factor += (double)v4fin_DaysDif(issue,settlement,basis)/(double)v4fin_DaysDif(B,A,basis) ;
	   break ;
	 } ;
	res = par * rate/(double)frequency * factor ;
	*ok = TRUE ; return(res) ;
}

/*	v4fin_IntAccrMat - Accrued Interest for Security Paying Interest at Maturity	*/

double v4fin_IntAccrMat(issue,settlement,rate,par,basis)
  int issue,settlement ; /* Issue & settlement dates */
  double rate ;		/* Annual coupon rate */
  double par ;		/* Par value */
  int basis ;		/* Calendar */
{ int days,ok ;

	days = v4fin_CouponDateCalc(V4FIN_CDCDaysYr,settlement,settlement+1,1,basis,&ok) ;
	return(par * rate * (double)(settlement-issue)/(double)days) ;
}

/*	v4fin_IntRateEff - Calculates Effective Interest with Compounding	*/
/*	Ref: A.51	*/

double v4fin_IntRateEff(nomrate,compounds)
  double nomrate ;	/* Nominal interest rate */
  double compounds ;	/* Number of compounds per interest rate period */
{ double efr ;

	efr = pow(1.0 + nomrate/compounds, compounds) ;
	return(efr - 1.0) ;
}

/*	v4fin_IntRateEffCont - Calculates Effective Interest, continuous compounding	*/
/*	Ref: A.51	*/

double v4fin_IntRateEffCont(nomrate)
  double nomrate ;	/* Nominal interest rate */
{
	return(exp(nomrate)) ;
}

/*	v4fin_IntRateNom - Calculates nominal interest rate from effective	*/
/*	Ref: A.51 & veh */

double v4fin_IntRateNom(effrate,compounds)
  double effrate ;	/* Effective interest rate */
  double compounds ;	/* Number of compounds per period */
{ double t1,t2,t3 ;

	t1 = log(1.0 + effrate) ;
	t2 = exp(t1/compounds) ;
	t3 = (t2 - 1) * compounds ;
	return(t3) ;
}

/*	v4fin_IntRateTaxInfl - Calculates effective interest given taxes & inflation */
/*	Ref: A.44	*/

double v4fin_IntRateTaxInfl(nominal,taxrate,inflrate)
  double nominal ;	/* Nominal interest rate */
  double taxrate ;	/* Tax rate for period */
  double inflrate ;	/* Inflation rate for period */
{ 

	return(((1.0 + nominal*(1.0-taxrate))/(1+inflrate))-1) ;
}

/*	v4fin_IntRateSec - Calculates interest rate for security	*/

double v4fin_IntRecSec(price,redeem,smdate,matdate,basis,ok)
  double price ;	/* Price of security */
  double redeem ;	/* Redemption price */
  int smdate,matdate ;	/* Settlement/Maturity dates */
  int basis ;
  LOGICAL *ok ;
{ double res ;

	res = ((redeem-price)/price) *
		((double)v4fin_CouponDateCalc(V4FIN_CDCDaysYr,smdate,matdate,1,basis,ok)/(double)(matdate-smdate)) ;
	return(res) ;
}

/*	v4fin_IRR - Internal Rate of Return from Series of Cash Flows	*/

P *v4fin_IRR(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,*ept,*lpt,*vpt,pnt ;
  struct V4L__ListPoint *lp ;
  struct V4IM_DblArray *npv ;
  int i,frameid,lx,ok ; double dnum ;

	lpt = NULL ; ept = NULL ; vpt = NULL ; ok = TRUE ; npv = v_InitDblArray(0) ;
	for(i=1;ok&&i<=argcnt;i++)
	 {
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto failure ;
	      case V4IM_Tag_Unk:	ok = FALSE ; break ;
	      case V4IM_Tag_Enum:	ept = ipt ; lx = i ; break ;
	      case V4IM_Tag_First:	npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_List:	lpt = ipt ; lx = i ; break ;
	      case V4IM_Tag_Values:	vpt = ipt ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto failure ; } ;

	if (lpt != NULL)			/* Have list - no tags */
	 { if (ept != NULL || vpt != NULL)
	    { v_Msg(ctx,NULL,"@FinIRR() fail - Cannot have list & Enum/Values tags") ; goto failure ; } ;
	   lp = v4im_VerifyList(NULL,ctx,lpt,intmodx) ;
	   if (lp == NULL)
	    { v_Msg(ctx,NULL,"@FinIRR() fail - (%1P) not a list - %0A",lpt) ; goto failure ; } ;
	   for(i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&pnt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"@FinIRR() fail - point (%1P) -  %0A",&pnt) ; goto failure ; } ;
	    } ;
	 } else					/* Have to enumerate & get list */
	 { if (vpt == NULL || ept == NULL)
	    { v_Msg(ctx,NULL,"@FinIRR() fail - Need Enum/Values tags or list") ; goto failure ; } ;
	   lp = v4im_VerifyList(NULL,ctx,ept,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"@FinIRR() fail - (%1P) not a list - %0A",ept) ; goto failure ; } ;
	   frameid = v4ctx_FramePush(ctx,NULL) ;		/* Start new context frame */
	   for(i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
	      if (!v4ctx_FrameAddDim(ctx,0,&pnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
	      CLEARCACHE
	      ipt = v4dpi_IsctEval(&pnt,vpt,ctx,V4DPI_EM_EvalQuote,0,NULL) ;
	      if (ipt == NULL)
	       { v_Msg(ctx,NULL,"@FinIRR() fail - Point (%1d) did not evaluate",i) ;  goto failure ; } ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok)
	       { v_Msg(ctx,NULL,"@FinIRR() fail - point (%1P) -  %0A",ipt) ; goto failure ; } ;
	    } ;
	   if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; goto failure ; } ;
	 } ;
//	ipt = respnt ;
//	ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;		/* Force to funky type so SetPointValue forces to Real */
//	v4im_SetPointValue(ctx,ipt,v4fin_IRRCalc(npv->Count,npv->Pay,&ok)) ;
	dnum = v4fin_IRRCalc(npv->Count,npv->Pay,&ok) ;
	if (!ok) { v_Msg(ctx,NULL,"@FinIRR() fail - %0A") ; goto failure ; } ;
	dblPNTv(respnt,dnum) ;
	v4mm_FreeChunk(npv) ;
	return(respnt) ;
failure:
	v4mm_FreeChunk(npv) ;
	REGISTER_ERROR(0) ; return(NULL) ;
}

/*	v4fin_IRRCalc - Internal Rate of Return from Series of Cash Flows	*/
/*	Ref: veh/newton's method */

double v4fin_IRRCalc(count,cash,ok)
  int count ;		/* Number of periods */
  double cash[] ;	/* Cash flow */
  LOGICAL *ok ;
{ double x,x1,x2, fx,fx1,fx2, slope ;
  struct V4C__Context *ctx ;
  int max ;

	x1 = 0.09 ; fx1 = v4fin_NPV(x1,count,cash) ;	/* Prior guess */
	x2 = 0.10 ; fx2 = v4fin_NPV(x2,count,cash) ;	/* First guess */
	for(max=0;max<10000;max++)
	 { slope = (fx2 - fx1)/(x2 - x1) ;
	   if (slope == 0) { max = V4LIM_BiggestPositiveInt ; break ; } ;
	   x = x2 - fx2 / slope ;			/* Use newton's method */
	   fx = v4fin_NPV(x,count,cash) ;
	   if (fabs(fx) < 0.0001) break ;		/* Quit already */
	   x1 = x2 ; fx1 = fx2 ;			/* Keep trying */
	   x2 = x ; fx2 = fx ;
	 } ;
	if (max >= 10000)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ; *ok = FALSE ; return(1.0) ; } ;
	*ok = TRUE ; return(x) ;
}

/*	v4fin_Macaulay - Returns Macaulay Duration	*/

double v4fin_Macaulay(settlement,maturity,issue,first,rate,yield,par,frequency,basis,ok)
  int settlement ;
  int maturity ;
  int issue ;
  int first ;		/* First coupon date */
  double rate ;		/* Coupon rate */
  double par ;		/* Par value */
  double yield ;	/* Nominal annual yield */
  int frequency ;
  int basis ;
  LOGICAL *ok ;
{ int N,k ; double sumT,sumB,CFk,T,f ;

	N = v4fin_CouponDateCalc(V4FIN_CDCNum,settlement,maturity,frequency,basis,ok) ;
	sumT = 0.0 ; sumB = 0.0 ; T = 0.0 ;
	for(k=1;k<=N;k++)
	 { if (k == 1) { CFk = v4fin_IntAccPeriodic(issue,first,first,rate,par,frequency,basis,ok) ; }
	    else if (k == N) { CFk = par + par * rate/(double)frequency ; }
	    else CFk = par * rate/(double)frequency ;
	   if (k == 1)
	    { T = 
		(double)v4fin_CouponDateCalc(V4FIN_CDCDaysNC,settlement,maturity,frequency,basis,ok) /
		(double)v4fin_CouponDateCalc(V4FIN_CDCDays,settlement,maturity,frequency,basis,ok) ;
	    }
	    else { T += 1.0 ; } ;
	   f = CFk/pow(1.0+yield/(double)frequency,T) ;
	   sumT += T * f ; sumB += f ;
	 } ;
	*ok = TRUE ; return(sumT/sumB) ;
}

/*	v4fin_NPV - Net Present Value from Cash Flow	*/
/*	Ref:		*/

double v4fin_NPV(rate,count,cash)
  double rate ;		/* Interest rate */
  int count ;		/* Number of periods */
  double cash[] ;	/* Cash flow */
{ int t ;
  double x,sum ;

	for(sum=0,t=0;t<=count;t++)
	 { x = cash[t]/pow(1.0+rate,t) ;
	   sum += x ;
	 } ;
	return(sum) ;
}

/*	v4fin_Price - Price/$100 face value of a security paying periodic interest */
/*	Ref: C.112	*/

double v4fin_Price(settlement,maturity,rate,yield,redemption,frequency,basis)
  int settlement ;	/* Settlement date */
  int maturity ;	/* Maturity date */
  double rate ;		/* Annual coupon rate */
  double yield ;	/* Securities annual yield */
  double redemption ;	/* Redemption value per $100 */
  int frequency ;	/* Coupon frequency */
  int basis ;
{ double y,c,ai,p1,sum,res ; int N,tsc,B,k,tis,ok ;

	y = yield ;
	c = rate ;
	tis = v4fin_CouponDateCalc(V4FIN_CDCDaysBS,settlement,maturity,frequency,basis,&ok) ;
	ai = c * ((double)tis/360.00) ;
	N = v4fin_CouponDateCalc(V4FIN_CDCNum,settlement,maturity,frequency,basis,&ok) ;
	tsc = v4fin_CouponDateCalc(V4FIN_CDCDaysNC,settlement,maturity,frequency,basis,&ok) ;
	B = v4fin_CouponDateCalc(V4FIN_CDCDays,settlement,maturity,frequency,basis,&ok) ;
	p1 = 1.0/pow(1.0+y/2.0,N-1+(tsc/B)) ;
	for(sum=0.0,k=1;k<=N;k++)
	 { sum += (c/2.0) / pow(1.0+y/2.0,(double)k-1.0+(tsc/B)) ;} ;
	res = p1 + sum - ai ;
	return(res) ;
}

P *v4fin_TBill(ctx,respnt,argpnts,argcnt,intmodx,trace) 
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,ok ; double dres ;
  int settlement=UNUSED, maturity=UNUSED ;
  double discount=DBL_MAX, price=DBL_MAX ;

	ok = TRUE ;
	for(i=1;ok&&i<=argcnt;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Settlement:		settlement = v4im_GetPointUD(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Maturity:		maturity = v4im_GetPointUD(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Discount:		discount = v4im_GetPointDbl(&ok,ipt,ctx) ;
						if (discount <= 0 || discount >= 1) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"@Discount tag must be between 0.0 and 1.0") ; } ;
						break ;
	      case V4IM_Tag_Price:		price = v4im_GetPointDbl(&ok,ipt,ctx) ;
						if (price <= 0 || price >= 100) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"@Price tag must be greater than 0 and less than 100") ; } ;
						break ;
	      case -V4IM_Tag_Yield:
	      case -V4IM_Tag_Price:
	      case -V4IM_Tag_Value:		if (res != 0) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"@Arg #%1d - Only one Tag? may be specified",i) ; break ; } ; res = t ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	if (settlement >= maturity)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Settlement date must be less than the Maturity date") ; ok = FALSE ; } ;
	switch(res)
	 { case -V4IM_Tag_Yield:	/* Ref: B.359-360 */
		if (settlement == UNUSED || maturity == UNUSED || price == DBL_MAX)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@Settlement, Maturity, and Price tags required") ; ok = FALSE ; } ;
		discount = ((100.0 - price)/100.0) * (360.0/(maturity-settlement)) ;
		dres = (365.0 * discount) / (360.0 - (discount * v4fin_Days360(settlement,maturity))) ;
		break ;
	   case -V4IM_Tag_Price:	/* Ref: B.359-360 */
		if (settlement == UNUSED || maturity == UNUSED || discount == DBL_MAX)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@Settlement, Maturity, and Discount tags required") ; ok = FALSE ; } ;
		dres = 100.0 * (1.0 - (discount * v4fin_Days360(settlement,maturity))/360.0) ; break ;
	   case -V4IM_Tag_Value:	/* Ref: B.359-360 */
		if (settlement == UNUSED || maturity == UNUSED || discount == DBL_MAX)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@Settlement, Maturity, and Discount tags required") ; ok = FALSE ; } ;
		dres = (365 * discount)/(360.0 - (discount*(v4fin_Days360(settlement,maturity)))) ; break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	ipt = respnt ;
//	ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ; v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
}

/*	v4fin_TVMFV - Time-Value-of-Money: Future Value */
/*	Ref:	*/

double v4fin_TVMFV(rate,pmt,pv,periods,mode)
  double rate ;		/* Interest rate per period */
  double pmt ;		/* Payment per period */
  double pv ;		/* Present value */
  double periods ;	/* Number of periods */
  int mode ;		/* Payment mode, 0=end-of-period, 1=begin-of-period */
{ double fv,uspv,sppv ;

	uspv = (1.0 - pow(1.0 + rate,-periods)) / rate ;
	sppv = pow(1.0 + rate,-periods) ;
	fv = (pv + (1.0 + rate*mode)*pmt*uspv) / sppv ;
	return(-fv) ;
}

/*	v4fin_TVMIPPMT - Time-Value-of_Money: Interest/Principal for Period	*/

double v4fin_TVMIPPMT(flag,rate,pv,fv,periods,forper,mode,ok)
  int flag ;		/* TRUE for interest, FALSE for principal */
  double rate ;		/* Interest rate per period */
  double pv ;		/* Present Value */
  double fv ;		/* Future value */
  double periods ;	/* Number of periods */
  double forper ;	/* For the period */
  int mode ;		/* Payment mode, 0=end-of-period, 1=begin-of-period */
  LOGICAL *ok ;
{ double pmt,balance,interest ;
  struct V4C__Context *ctx ;
  int i ;

	if (forper > periods)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"@Period (%1g) cannot exceed number of periods (%2g)",forper,periods) ; *ok = FALSE ; return(1.0) ; } ;
	pmt = v4fin_TVMPMT(rate,pv,fv,periods,mode) ;
	balance = pv ; interest = 0 ;
	for(i=0;i<forper;i++)
	 { interest = balance * rate ;			/* Interest on balance */
	   balance += pmt + interest ;			/* New balance */
	 } ;
	*ok = TRUE ; return(flag ? -interest : pmt+interest) ;	/* Return interest/principal paid for period */
}

/*	v4fin_TVMPMT - Time-Value-of-Money: Payment */
/*	Ref:	*/

double v4fin_TVMPMT(rate,pv,fv,periods,mode)
  double rate ;		/* Interest rate per period */
  double pv ;		/* Present Value */
  double fv ;		/* Future value */
  double periods ;	/* Number of periods */
  int mode ;		/* Payment mode, 0=end-of-period, 1=begin-of-period */
{ double pmt,uspv,sppv ;

	uspv = (1.0 - pow(1.0 + rate,-periods)) / rate ;
	sppv = pow(1.0 + rate,-periods) ;
	pmt = (pv + fv * sppv) / ((1.0 + rate*mode)*uspv) ;
	return(-pmt) ;
}

/*	v4fin_TVMPeriods - Time-Value-of-Money: Periods */
/*	Ref:	*/

double v4fin_TVMPeriods(rate,pmt,pv,fv,mode,ok)
  double rate ;		/* Interest rate per period */
  double pmt ;		/* Payment per period */
  double pv ;		/* Present value */
  double fv ;		/* Future value */
  int mode ;		/* Payment mode, 0=end-of-period, 1=begin-of-period */
  LOGICAL *ok ;
{ double uspv, sppv, x,x1,x2, fx,fx1,fx2, slope ;
  struct V4C__Context *ctx ;
  int max ;

#define ALGO(dst,periods) \
  uspv = (1.0 - pow(1.0 + rate,-periods)) / rate ; \
  sppv = pow(1.0 + rate,-periods) ; \
  dst = pv + (1.0 + rate*mode)*pmt*uspv + fv*sppv ;

	x1 = 11 ; ALGO(fx1,x1) ;			/* Prior guess */
	x2 = 12 ; ALGO(fx2,x2) ;			/* First guess */
	for(max=0;max<10000;max++)
	 { slope = (fx2 - fx1)/(x2 - x1) ;
	   if (slope == 0) { max = V4LIM_BiggestPositiveInt ; break ; } ;
	   x = x2 - fx2 / slope ;			/* Use newton's method */
	   ALGO(fx,x) ;
	   if (fabs(fx) < 0.0001) break ;		/* Quit already */
	   x1 = x2 ; fx1 = fx2 ;			/* Keep trying */
	   x2 = x ; fx2 = fx ;
	 } ;
	if (max >= 10000)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ; *ok = FALSE ; return(1.0) ; } ;
	*ok = TRUE ; return(x) ;
}

/*	v4fin_TVMPV - Time-Value-of-Money: Present Value */
/*	Ref:	*/

double v4fin_TVMPV(rate,pmt,fv,periods,mode)
  double rate ;		/* Interest rate per period */
  double pmt ;		/* Payment per period */
  double fv ;		/* Future value */
  double periods ;	/* Number of periods */
  int mode ;		/* Payment mode, 0=end-of-period, 1=begin-of-period */
{ double pv,uspv,sppv ;

	uspv = (1.0 - pow(1.0 + rate,-periods)) / rate ;
	sppv = pow(1.0 + rate,-periods) ;
	pv = (1.0 + rate*mode)*pmt*uspv + fv*sppv ;
	return(-pv) ;
}

/*	v4fin_TVMRate - Time-Value-of-Money: Rate */
/*	Ref:	*/

double v4fin_TVMRate(pmt,pv,fv,periods,mode,ok)
  double pv ;		/* Present Value */
  double pmt ;		/* Payment per period */
  double fv ;		/* Future value */
  double periods ;	/* Number of periods */
  int mode ;		/* Payment mode, 0=end-of-period, 1=begin-of-period */
  LOGICAL *ok ;
{ double uspv, sppv, x,x1,x2, fx,fx1,fx2, slope ;
  struct V4C__Context *ctx ;
  int max ;

#define ALGO1(dst,rate) \
  uspv = (1.0 - pow(1.0 + rate,-periods)) / rate ; \
  sppv = pow(1.0 + rate,-periods) ; \
  dst = pv + (1.0 + rate*mode)*pmt*uspv + fv*sppv ;

	x1 = 0.09 ; ALGO1(fx1,x1) ;			/* Prior guess */
	x2 = 0.10 ; ALGO1(fx2,x2) ;			/* First guess */
	for(max=0;max<10000;max++)
	 { slope = (fx2 - fx1)/(x2 - x1) ;
	   if (slope == 0) { max = V4LIM_BiggestPositiveInt ; break ; } ;
	   x = x2 - fx2 / slope ;			/* Use newton's method */
	   ALGO1(fx,x) ;
	   if (fabs(fx) < 0.0001) break ;		/* Quit already */
	   x1 = x2 ; fx1 = fx2 ;			/* Keep trying */
	   x2 = x ; fx2 = fx ;
	 } ;
	if (max >= 10000)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ; *ok = FALSE ; return(1.0) ; } ;
	*ok = TRUE ; return(x) ;
}

/*	v4fin_TVM - Driver Routine for TVM Modules		*/

P *v4fin_TVM(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,mode,ok ; double dres ;
  double fv=0,pmt=0,pv=0,rate=0,nth=0 ; int eperiod=0,periods=0,speriod=0 ;

	ok = TRUE ;
	for(i=1;ok&&i<=argcnt;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto tvm_fail ;
	      case V4IM_Tag_Unk:	ok = FALSE ; break ;
	      case V4IM_Tag_FV:		fv = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_EPeriod:	eperiod = v4im_GetPointInt(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Nth:	nth = v4im_GetPointInt(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Periods:	periods = v4im_GetPointInt(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Pmt:	pmt = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_PV:		pv = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Rate:	rate = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_SPeriod:	speriod = v4im_GetPointInt(&ok,ipt,ctx) ; break ;

	      case -V4IM_Tag_FV:
	      case -V4IM_Tag_IPmt:
	      case -V4IM_Tag_Periods:
	      case -V4IM_Tag_Pmt:
	      case -V4IM_Tag_PPmt:
	      case -V4IM_Tag_PV:
	      case -V4IM_Tag_Rate:
		if (res != 0)
		 { v_Msg(ctx,NULL,"@FinTVM() fail - Can only specify one result") ; goto tvm_fail ; } ;
		res = -t ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto tvm_fail ; } ;
   	if (rate == 0.0) { v_Msg(ctx,NULL,"FinZeroValue",intmodx,V4IM_Tag_Rate) ; goto tvm_fail ; } ;

	mode = 0 ;			/* Default mode to 0 for now */
	if (speriod != 0 || eperiod != 0)
	 { if (eperiod < 1 || speriod > periods || speriod > eperiod)
	    { v_Msg(ctx,NULL,"@FinTVM fail() - Start period (%1d) must be >= 1 & end period (%2d) must be <= %3d",
			speriod,eperiod,periods) ; goto tvm_fail ; } ;
	 } else { speriod = nth ; eperiod = nth ; } ;
	switch(res)
	 { default: v_Msg(ctx,NULL,"@FinTVM() fail - No result point specified") ; goto tvm_fail ;
	   case V4IM_Tag_FV:
		dres = v4fin_TVMFV(rate,pmt,pv,periods,mode) ; break ;
	   case V4IM_Tag_IPmt:
		for(dres=0,nth=speriod;ok&&nth<=eperiod;nth++) { dres += v4fin_TVMIPPMT(TRUE,rate,pv,fv,periods,nth,mode,&ok) ; } ;
		break ;
	   case V4IM_Tag_Periods:	dres = v4fin_TVMPeriods(rate,pmt,pv,fv,mode,&ok) ; break ;
	   case V4IM_Tag_Pmt:
	   	dres = v4fin_TVMPMT(rate,pv,fv,periods,mode) ; break ;
	   case V4IM_Tag_PPmt:
		for(dres=0,nth=speriod;ok&&nth<=eperiod;nth++) { dres += v4fin_TVMIPPMT(FALSE,rate,pv,fv,periods,nth,mode,&ok) ; } ;
		break ;
	   case V4IM_Tag_PV:		dres = v4fin_TVMPV(rate,pmt,fv,periods,mode) ; break ;
	   case V4IM_Tag_Rate:		dres = v4fin_TVMRate(pmt,pv,fv,periods,mode,&ok) ; break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"@FinTVM() fail - %1s",ctx->ErrorMsgAux) ; goto tvm_fail ; } ;
//	ipt = respnt ;
//	ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;
//	v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
tvm_fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}


/*	v4fin_Coupon - Driver Routine for Coupon Modules		*/

P *v4fin_Coupon(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres=0,ok ;
  int set=0,mat=0,freq=0,basis=0 ;
	
	for(i=1,ok=TRUE;ok&&i<=argcnt;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Settlement:		set = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Maturity:		mat = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Frequency:		freq = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Basis:		basis = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;

	      case -V4IM_Tag_DaysBS:	tres = V4FIN_CDCDaysBS ; break ;
	      case -V4IM_Tag_DaysS:	tres = V4FIN_CDCDays ; break ;
	      case -V4IM_Tag_DaysNC:	tres = V4FIN_CDCDaysNC ; break ;
	      case -V4IM_Tag_DateAS:	tres = V4FIN_CDCNCD ; break ;
	      case -V4IM_Tag_DateBS:	tres = V4FIN_CDCPCD ; break ;
	      case -V4IM_Tag_NumSM:	tres = V4FIN_CDCNum ; break ;
	    } ;
	   if (res != 0)
	    { v_Msg(ctx,NULL,"@FinCoupon() fail - Can only specify one result") ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	   res = tres ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

	tres = v4fin_CouponDateCalc(res,set,mat,freq,basis,&ok) ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	ipt = respnt ;
	switch(res)
	 { default:
	   case V4FIN_CDCDaysBS:
	   case V4FIN_CDCDays:
	   case V4FIN_CDCDaysNC:
	   case V4FIN_CDCNum:
		intPNTv(ipt,tres) ; break ;
//		ipt->PntType = V4DPI_PntType_Int ; ipt->Dim = Dim_Int ; break ;
	   case V4FIN_CDCNCD:
	   case V4FIN_CDCPCD:
		ZPH(ipt) ; ipt->Bytes = V4PS_Int ; ipt->Value.IntVal = tres ;
		ipt->PntType = V4DPI_PntType_UDate ; ipt->Dim = Dim_UDate ; break ;
	 } ;
	return(ipt) ;
}

/*	v4fin_Dep - Driver Routine for Depreciation Modules		*/

P *v4fin_Dep(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt, *argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres,ok ;
  double dres,Cost=0.0,Factor=0.0,Salvage=0.0,Life=0.0,Partial=0.0,Period=0.0 ;

	ok = TRUE ;	
	for(i=1;ok&&i<=argcnt;i++)
	 { tres = 0 ;
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Cost:		Cost = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Factor:		Factor = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Life:		Life = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Partial:		Partial = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Period:		Period = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Salvage:		Salvage = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case -V4IM_Tag_DDB:		tres = t ; break ;
	      case -V4IM_Tag_FDB:		tres = t ; break ;
	      case -V4IM_Tag_SLN:		tres = t ; break ;
	      case -V4IM_Tag_SYD:		tres = t ; break ;
	    } ;
	   if (tres != 0)
	    { if (res != 0) { v_Msg(ctx,NULL,"@FinDep() fail - Can only specify one result tag?") ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	      res = tres ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	switch(res)
	 { case -V4IM_Tag_DDB:			dres = v4fin_DepDDB(Cost,Salvage,Life,Period,Factor,Partial,&ok) ; break ;
	   case -V4IM_Tag_FDB:			dres = v4fin_DepFDB(Cost,Salvage,Life,Period,Partial,&ok) ; break ;
	   case -V4IM_Tag_SLN:			dres = v4fin_DepSLN(Cost,Salvage,Life,Period,Partial,&ok) ; break ;
	   case -V4IM_Tag_SYD:			dres = v4fin_DepSYD(Cost,Salvage,Life,Period,Partial,&ok) ; break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	ipt = respnt ;
//	ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;
//	v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
}

/*	v4fin_IntRate - Driver Routine for Interest Rate Modules		*/

P *v4fin_IntRate(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres,ok ;
  double dres,Compounds=0.0,Effective=0,Inflation=0.0,Nominal=0,TaxRate=0.0 ;
	
	ok = TRUE ;
	for(i=1;ok&&i<=argcnt;i++)
	 { tres = 0 ;
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Compounds:		Compounds = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Effective:		Effective = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Inflation:		Inflation = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Nominal:		Nominal = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_TaxRate:		TaxRate = v4im_GetPointDbl(&ok,ipt,ctx) ; break ;
	      case -V4IM_Tag_Effective:		tres = t ; break ;
	      case -V4IM_Tag_Nominal:		tres = t ; break ;
	    } ;
	   if (tres != 0)
	    { if (res != 0) { v_Msg(ctx,NULL,"@FinIntRate() fail - Can only specify one result tag?") ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	      res = tres ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	switch(res)
	 { case -V4IM_Tag_Effective:
		if (Compounds == 0.0) { dres = v4fin_IntRateEffCont(Nominal) ; }
		 else if (TaxRate == 0.0 && Inflation == 0.0) { dres = v4fin_IntRateEff(Nominal,Compounds) ; }
		 else { dres = v4fin_IntRateTaxInfl(Nominal,TaxRate,Inflation) ; } ;
		break ;
	   case -V4IM_Tag_Nominal:		dres = v4fin_IntRateNom(Effective,Compounds) ; break ;
	 } ;
//	ipt = respnt ; ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;
//	v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
}

/*	v4fin_FinSecDisc - Calculates Discount for Securities */

P *v4fin_FinSecDisc(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,ok ; double dres ;
  int Settlement=0,Maturity=0,Basis=0 ; double Price=0.0,Redemption=0.0 ;
	
	for(i=1,ok=TRUE;i<=argcnt&&ok;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Settlement:		Settlement = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Maturity:		Maturity = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Basis:		Basis = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Price:		Price = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Redemption:		Redemption = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	dres = v4fin_Disc(Settlement,Maturity,Price,Redemption,Basis,&ok) ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	ipt = respnt ; ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;
//	v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
}

/*	v4fin_FinSecDur - Calculates security durations */

P *v4fin_FinSecDur(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres=UNUSED,ok ; double dres ;
  int Settlement=0,Maturity=0,Issue=0,First=0,Basis=0,Frequency=0 ; double Coupon=0,Yield=0 ;
	
	for(i=1,ok=TRUE;i<=argcnt&&ok;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_First:		First = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Issue:		Issue = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Settlement:		Settlement = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Maturity:		Maturity = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Frequency:		Frequency = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Basis:		Basis = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Coupon:		Coupon = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Yield:		Yield = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;

	      case -V4IM_Tag_Macaulay:	tres = t ; break ;
	      case -V4IM_Tag_Modified:	tres = t ; break ;
	    } ;
	   if (res != 0)
	    { v_Msg(ctx,NULL,"ModOneResult",intmodx,res,t) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	      sprintf(ctx->ErrorMsg,"Can only specify one result in FinSecDur (%s)",v4im_TagName(t)) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	   res = tres ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

	ipt = respnt ;
	switch(res)
	 { case UNUSED:
	   case -V4IM_Tag_Macaulay:
		if (Issue == 0) Issue = Settlement ;
		ok = TRUE ;
		if (First == 0) First = v4fin_CouponDateCalc(V4FIN_CDCNCD,Settlement,Maturity,Frequency,Basis,&ok) ;
		if (ok) dres = v4fin_Macaulay(Settlement,Maturity,Issue,First,Coupon,Yield,100.0,Frequency,Basis,&ok) ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
		break ;
	   case -V4IM_Tag_Modified:
		break ;
	 } ;
//	ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;
//	v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
}

/*	v4fin_FinSecInt - Calculate Security Interest Rates */

P *v4fin_FinSecInt(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres=UNUSED,ok ; double dres ;
  int Basis=0,First=0,Frequency=1,Issue=0,Maturity=0,Settlement=0 ;
  double Coupon=0,Investment=0,Par=1000.0,Rate=0,Redemption=0 ;
	
	for(i=1,ok=TRUE;i<=argcnt&&ok;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Settlement:		Settlement = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Maturity:		Maturity = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Frequency:		Frequency = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Basis:		Basis = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Coupon:		Coupon = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_First:		First = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Issue:		Issue = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Investment:		Investment = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Par:		Par = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Rate:		Rate = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Redemption:		Redemption = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;

	      case -V4IM_Tag_Maturity:		tres = t ; break ;
	      case -V4IM_Tag_Periodic:		tres = t ; break ;
	      case -V4IM_Tag_Rate:		tres = t ; break ;
	    } ;
	   if (res != 0)
	    { v_Msg(ctx,NULL,"ModOneResult",intmodx,res,t) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	    { sprintf(ctx->ErrorMsg,"Can only specify one result in FinSecInt (%s)",v4im_TagName(t)) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	   res = tres ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

//	ipt = respnt ;
//	ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;
	switch(res)
	 { case UNUSED:
	   case -V4IM_Tag_Maturity:
		dres = v4fin_IntAccrMat(Issue,Settlement,Rate,Par,Basis) ; break ;
	   case -V4IM_Tag_Periodic:
		dres = v4fin_IntAccPeriodic(Issue,First,Settlement,Rate,Par,Frequency,Basis,&ok) ; break ;
	   case -V4IM_Tag_Rate:
		dres = v4fin_IntRecSec(Investment,Redemption,Settlement,Maturity,Basis,&ok) ;
		break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
}

/*	v4fin_FinSecPrice - Calculates Security Price */

P *v4fin_FinSecPrice(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres=UNUSED,ok ; double dres,f1,f2,f3 ;
  int Basis=0,First=0,Frequency=0,Issue=0,Maturity=0,Settlement=0,Last=0 ; double Rate=0,Redemption=0,Yield=0 ;
	
	for(i=1,ok=TRUE;i<=argcnt&&ok;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Basis:		Basis = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_First:		First = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Frequency:		Frequency = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Issue:		Issue = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Last:		Last = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Maturity:		Maturity = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Rate:		Rate = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Redemption:		Redemption = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Settlement:		Settlement = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Yield:		Yield = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;

	      case -V4IM_Tag_Maturity:		tres = t ; break ;
	      case -V4IM_Tag_Periodic:		tres = t ; break ;
	      case -V4IM_Tag_OddFirst:		tres = t ; break ;
	      case -V4IM_Tag_OddLast:		tres = t ; break ;
	      case -V4IM_Tag_Discount:		tres = t ; break ;
	    } ;
	   if (res != 0)
//	    { sprintf(ctx->ErrorMsg,"Can only specify one result in FinSecPrice (%s)",v4im_TagName(t)) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	    { v_Msg(ctx,NULL,"ModOneResult",intmodx,res,t) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	   res = tres ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

//	ipt = respnt ; ok = TRUE ;
//	ZPH(ipt) ; ipt->PntType = xxV4DPI_PntType_Foreign ;
	switch(res)
	 { case UNUSED:
	   case -V4IM_Tag_Maturity:
		f1 = 100.0 + Rate * 100.0 * 
			((double)v4fin_DaysDif(Issue,Maturity,Basis)/(double)v4fin_CouponDateCalc(V4FIN_CDCDaysYr,Settlement,Maturity,1,Basis,&ok)) ;
		f2 = 1.0 + ((double)v4fin_DaysDif(Settlement,Maturity,Basis)/(double)v4fin_CouponDateCalc(V4FIN_CDCDaysYr,Settlement,Maturity,1,Basis,&ok)) * Yield ;
		f3 =  100.0 * Rate *
			(double)v4fin_DaysDif(Issue,Settlement,Basis)/(double)v4fin_CouponDateCalc(V4FIN_CDCDaysYr,Settlement,Maturity,1,Basis,&ok) ;
		dres = f1/f2 - f3 ;
		break ;
	   case -V4IM_Tag_Periodic:
	   case -V4IM_Tag_OddFirst:
	   case -V4IM_Tag_OddLast:
	   case -V4IM_Tag_Discount:
		dres = (double)v4fin_DaysDif(Settlement,Maturity,Basis)
				/(double)v4fin_CouponDateCalc(V4FIN_CDCDaysYr,Settlement,Maturity,1,Basis,&ok) ;
		dres = Redemption - (Rate*Redemption*dres) ;
		break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	v4im_SetPointValue(ctx,ipt,dres) ;
	dblPNTv(respnt,dres) ;
	return(respnt) ;
}

/*	v4fin_FinSecRcvd - Calculate Amount Received for a Security */

P *v4fin_FinSecRcvd(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres=0,ok ;
  int Basis=0,Maturity,Settlement ; double Investment,Discount ;
	
	for(i=1,ok=TRUE;i<=argcnt&&ok;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Basis:		Basis = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Discount:		Discount = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Investment:		Investment = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Maturity:		Maturity = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Settlement:		Settlement = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	    } ;
	   if (res != 0)
	    { v_Msg(ctx,NULL,"ModOneResult",intmodx,res,t) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	    { sprintf(ctx->ErrorMsg,"Can only specify one result in FinSecInt (%s)",v4im_TagName(t)) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	   res = tres ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

	ipt = respnt ;
	ZPH(ipt) ; ipt->Bytes = V4PS_Int ;
	switch(res)
	 { case V4IM_Tag_Maturity:
	   case V4IM_Tag_Periodic:
	   case V4IM_Tag_Rate:
		break ;
	 } ;
	return(ipt) ;
}

/*	v4fin_FinSecYield - Calcuates Security Yield	*/

P *v4fin_FinSecYield(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *ipt,pnt ;
  int i,t,res=0,tres=UNUSED,ok ;
  int Basis=0,First,Frequency,Issue,Maturity,Settlement,Last ; double Par,Redemption,Rate ;
	
	for(i=1,ok=TRUE;i<=argcnt&&ok;i++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		ok = FALSE ; break ;
	      case V4IM_Tag_Basis:		Basis = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_First:		First = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Frequency:		Frequency = v4im_GetPointInt(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Issue:		Issue = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Last:		Last = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Maturity:		Maturity = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Par:		Par = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Redemption:		Redemption = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Settlement:		Settlement = v4im_GetPointUD(&ok,ipt,ctx) ; continue ;
	      case V4IM_Tag_Rate:		Rate = v4im_GetPointDbl(&ok,ipt,ctx) ; continue ;

	      case -V4IM_Tag_Maturity:		tres = t ; break ;
	      case -V4IM_Tag_Periodic:		tres = t ; break ;
	      case -V4IM_Tag_OddFirst:		tres = t ; break ;
	      case -V4IM_Tag_OddLast:		tres = t ; break ;
	      case -V4IM_Tag_Discount:		tres = t ; break ;
	    } ;
	   if (res != 0)
	    { v_Msg(ctx,NULL,"ModOneResult",intmodx,res,t) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	    { sprintf(ctx->ErrorMsg,"Can only specify one result in FinSecPrice (%s)",v4im_TagName(t)) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	   res = tres ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

	ipt = respnt ;
	ZPH(ipt) ; ipt->Bytes = V4PS_Int ;
	switch(res)
	 { case UNUSED:
	   case V4IM_Tag_Maturity:
	   case V4IM_Tag_Periodic:
	   case V4IM_Tag_OddFirst:
	   case V4IM_Tag_OddLast:
	   case V4IM_Tag_Discount:
		break ;
	 } ;
	return(ipt) ;
}