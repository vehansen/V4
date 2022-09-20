/*	v4stat.c - Statistical Functions		*/

/* Approximation to ln(N!) for N > 1000 (see http://mathworld.wolfram.com/StirlingsApproximation.html)  */
#define LNFACTORIAL(FACT,N) \
 if ((N) > 1000) { FACT = (((N) + 0.5)*log(N) - (N) + 0.5*log(2*PI)) ; } \
  else { double _i ; for(_i=2,FACT=0.0;_i<=(N);_i++) FACT += log(_i) ; } ;

#define MAXTRIES 10000				/* Max number of iterations */
#define NRANSI

/*	From Numerical Recipies				*/
#ifndef _NR_UTILS_H_
#define _NR_UTILS_H_

static double sqrarg;
#define SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)

static double dsqrarg;
#define DSQR(a) ((dsqrarg=(a)) == 0.0 ? 0.0 : dsqrarg*dsqrarg)

static double dmaxarg1,dmaxarg2;
#define DMAX(a,b) (dmaxarg1=(a),dmaxarg2=(b),(dmaxarg1) > (dmaxarg2) ?\
        (dmaxarg1) : (dmaxarg2))

static double dminarg1,dminarg2;
#define DMIN(a,b) (dminarg1=(a),dminarg2=(b),(dminarg1) < (dminarg2) ?\
        (dminarg1) : (dminarg2))

static double maxarg1,maxarg2;
#define FMAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
        (maxarg1) : (maxarg2))

static double minarg1,minarg2;
#define FMIN(a,b) (minarg1=(a),minarg2=(b),(minarg1) < (minarg2) ?\
        (minarg1) : (minarg2))

static long lmaxarg1,lmaxarg2;
#define LMAX(a,b) (lmaxarg1=(a),lmaxarg2=(b),(lmaxarg1) > (lmaxarg2) ?\
        (lmaxarg1) : (lmaxarg2))

static long lminarg1,lminarg2;
#define LMIN(a,b) (lminarg1=(a),lminarg2=(b),(lminarg1) < (lminarg2) ?\
        (lminarg1) : (lminarg2))

static int imaxarg1,imaxarg2;
#define IMAX(a,b) (imaxarg1=(a),imaxarg2=(b),(imaxarg1) > (imaxarg2) ?\
        (imaxarg1) : (imaxarg2))

static int iminarg1,iminarg2;
#define IMIN(a,b) (iminarg1=(a),iminarg2=(b),(iminarg1) < (iminarg2) ?\
        (iminarg1) : (iminarg2))

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

#if defined(__STDC__) || defined(ANSI) || defined(NRANSI) /* ANSI */


#else /* ANSI */

#endif /* ANSI */

#endif /* _NR_UTILS_H_ */

void nrerror(txt)
  char *txt ;
{	v4_error(0,0,"V4Stat","NR","Err",txt) ;
}

#define MAXIT 100
#define EPS 3.0e-7
#define FPMIN 1.0e-30
void avevar(data,n,ave,var)
  double data[] ; unsigned long n ; double *ave ; double *var ;
{
	unsigned long j;
	double s,ep;

	for (*ave=0.0,j=0;j<n;j++) *ave += data[j];
	*ave /= n;
	*var=ep=0.0;
	for (j=0;j<n;j++) {
		s=data[j]-(*ave);
		ep += s;
		*var += s*s;
	}
	*var=(*var-ep*ep/n)/(n-1);
}

double betacf(a, b, x)
  double a,b,x ;
{
	int m,m2;
	double aa,c,d,del,h,qab,qam,qap;

	qab=a+b;
	qap=a+1.0;
	qam=a-1.0;
	c=1.0;
	d=1.0-qab*x/qap;
	if (fabs(d) < FPMIN) d=FPMIN;
	d=1.0/d;
	h=d;
	for (m=1;m<=MAXIT;m++) {
		m2=2*m;
		aa=m*(b-m)*x/((qam+m2)*(a+m2));
		d=1.0+aa*d;
		if (fabs(d) < FPMIN) d=FPMIN;
		c=1.0+aa/c;
		if (fabs(c) < FPMIN) c=FPMIN;
		d=1.0/d;
		h *= d*c;
		aa = -(a+m)*(qab+m)*x/((a+m2)*(qap+m2));
		d=1.0+aa*d;
		if (fabs(d) < FPMIN) d=FPMIN;
		c=1.0+aa/c;
		if (fabs(c) < FPMIN) c=FPMIN;
		d=1.0/d;
		del=d*c;
		h *= del;
		if (fabs(del-1.0) < EPS) break;
	}
	if (m > MAXIT) nrerror("a or b too big, or MAXIT too small in betacf");
	return h;
}
#undef MAXIT
#undef EPS
#undef FPMIN

double betai(a,b,x)
  double a,b,x ;
{
	double bt;

	if (x < 0.0 || x > 1.0) nrerror("Bad x in routine betai");
	if (x == 0.0 || x == 1.0) bt=0.0;
	else
		bt=exp(gammln(a+b)-gammln(a)-gammln(b)+a*log(x)+b*log(1.0-x));
	if (x < (a+1.0)/(a+b+2.0))
		return bt*betacf(a,b,x)/a;
	else
		return 1.0-bt*betacf(b,a,1.0-x)/b;
}

void chsone( bins,  ebins,  nbins,  knstrn,  df, chsq, prob)
  double bins[],  ebins[], *df, *chsq, *prob ;
  int nbins, knstrn ;
{
	int j;
	double temp;

	*df=nbins-knstrn;
	*chsq=0.0;
	for (j=0;j<nbins;j++) {
		if (ebins[j] <= 0.0) nrerror("Bad expected number in chsone");
		temp=bins[j]-ebins[j];
		*chsq += temp*temp/ebins[j];
	}
	*prob=gammq(0.5*(*df),0.5*(*chsq));
}

double v4errff(x)
  double x ;
{
	return x < 0.0 ? -gammp(0.5,x*x) : gammp(0.5,x*x);
}

double erffc( x)
  double x ;
{
	return x < 0.0 ? 1.0+gammp(0.5,x*x) : gammq(0.5,x*x);
}

void ftest( data1,   n1,  data2,   n2, f,  prob)
  double data1[], data2[], *f, *prob ;
  unsigned long n1, n2 ;
{
	double var1,var2,ave1,ave2,df1,df2;

	avevar(data1,n1,&ave1,&var1);
	avevar(data2,n2,&ave2,&var2);
	if (var1 > var2) {
		*f=var1/var2;
		df1=n1-1;
		df2=n2-1;
	} else {
		*f=var2/var1;
		df1=n2-1;
		df2=n1-1;
	}
	*prob = 2.0*betai(0.5*df2,0.5*df1,df2/(df2+df1*(*f)));
	if (*prob > 1.0) *prob=2.0-*prob;
}

void fit( x,  y,  ndata,  sig,  mwt, a,b, siga, sigb, chi2, q)
  double x[], y[], sig[], *a, *b, *siga, *sigb, *chi2, *q ;
  int ndata, mwt ;
{
	int i;
	double wt,t,sxoss,sx=0.0,sy=0.0,st2=0.0,ss,sigdat;

	*b=0.0;
	if (mwt) {
		ss=0.0;
		for (i=0;i<ndata;i++) {
			wt=1.0/SQR(sig[i]);
			ss += wt;
			sx += x[i]*wt;
			sy += y[i]*wt;
		}
	} else {
		for (i=0;i<ndata;i++) {
			sx += x[i];
			sy += y[i];
		}
		ss=ndata;
	}
	sxoss=sx/ss;
	if (mwt) {
		for (i=0;i<ndata;i++) {
			t=(x[i]-sxoss)/sig[i];
			st2 += t*t;
			*b += t*y[i]/sig[i];
		}
	} else {
		for (i=0;i<ndata;i++) {
			t=x[i]-sxoss;
			st2 += t*t;
			*b += t*y[i];
		}
	}
	*b /= st2;
	*a=(sy-sx*(*b))/ss;
	*siga=sqrt((1.0+sx*sx/(ss*st2))/ss);
	*sigb=sqrt(1.0/st2);
	*chi2=0.0;
	if (mwt == 0) {
		for (i=0;i<ndata;i++)
			*chi2 += SQR(y[i]-(*a)-(*b)*x[i]);
		*q=1.0;
		sigdat=sqrt((*chi2)/(ndata-2));
		*siga *= sigdat;
		*sigb *= sigdat;
	} else {
		for (i=0;i<ndata;i++)
			*chi2 += SQR((y[i]-(*a)-(*b)*x[i])/sig[i]);
		*q=gammq(0.5*(ndata-2),0.5*(*chi2));
	}
}

double gammln(xx)
  double xx ;
{
  double x,y,tmp,ser;
  static double cof[6]={76.18009172947146,-86.50532032941677,24.01409824083091,-1.231739572450155,0.1208650973866179e-2,-0.5395239384953e-5};
  int j;

	y=x=xx;
	tmp=x+5.5;
	tmp -= (x+0.5)*log(tmp);
	ser=1.000000000190015;
	for (j=0;j<=5;j++) ser += cof[j]/++y;
	return -tmp+log(2.5066282746310005*ser/x);
}

double gammp( a,  x)
  double a, x ;
{
	double gamser,gammcf,gln;

	if (x < 0.0 || a <= 0.0) nrerror("Invalid arguments in routine gammp");
	if (x < (a+1.0)) {
		gser(&gamser,a,x,&gln);
		return gamser;
	} else {
		gcf(&gammcf,a,x,&gln);
		return 1.0-gammcf;
	}
}


double gammq( a,  x)
  double a,x ;
{
	double gamser,gammcf,gln;

	if (x < 0.0 || a <= 0.0) nrerror("Invalid arguments in routine gammq");
	if (x < (a+1.0)) {
		gser(&gamser,a,x,&gln);
		return 1.0-gamser;
	} else {
		gcf(&gammcf,a,x,&gln);
		return gammcf;
	}
}

#define ITMAX 100
#define EPS 3.0e-7
#define FPMIN 1.0e-30

void gcf(gammcf, a, x, gln)
  double *gammcf, a, x, *gln ;
{
	int i;
	double an,b,c,d,del,h;

	*gln=gammln(a);
	b=x+1.0-a;
	c=1.0/FPMIN;
	d=1.0/b;
	h=d;
	for (i=1;i<=ITMAX;i++) {
		an = -i*(i-a);
		b += 2.0;
		d=an*d+b;
		if (fabs(d) < FPMIN) d=FPMIN;
		c=b+an/c;
		if (fabs(c) < FPMIN) c=FPMIN;
		d=1.0/d;
		del=d*c;
		h *= del;
		if (fabs(del-1.0) < EPS) break;
	}
	if (i > ITMAX) nrerror("a too large, ITMAX too small in gcf");
	*gammcf=exp(-x+a*log(x)-(*gln))*h;
}
#undef ITMAX
#undef EPS
#undef FPMIN

#define ITMAX 100
#define EPS 3.0e-7

void gser(gamser, a, x, gln)
  double *gamser, a, x, *gln ;
{
	int n;
	double sum,del,ap;

	*gln=gammln(a);
	if (x <= 0.0) {
		if (x < 0.0) nrerror("x less than 0 in routine gser");
		*gamser=0.0;
		return;
	} else {
		ap=a;
		del=sum=1.0/a;
		for (n=1;n<=ITMAX;n++) {
			++ap;
			del *= x/ap;
			sum += del;
			if (fabs(del) < fabs(sum)*EPS) {
				*gamser=sum*exp(-x+a*log(x)-(*gln));
				return;
			}
		}
		nrerror("a too large, ITMAX too small in routine gser");
		return;
	}
}
#undef ITMAX
#undef EPS

int ndatat;
double *xt,*yt,aa,abdevt;

void medfit(x, y, ndata, a, b, abdev)
  double x[], y[], *a, *b, *abdev ;
  int ndata ;
{
	int j;
	double bb,b1,b2,del,f,f1,f2,sigb,temp;
	double sx=0.0,sy=0.0,sxy=0.0,sxx=0.0,chisq=0.0;

	ndatat=ndata;
	xt=x;
	yt=y;
	for (j=0;j<ndata;j++) {
		sx += x[j];
		sy += y[j];
		sxy += x[j]*y[j];
		sxx += x[j]*x[j];
	}
	del=ndata*sxx-sx*sx;
	aa=(sxx*sy-sx*sxy)/del;
	bb=(ndata*sxy-sx*sy)/del;
	for (j=0;j<ndata;j++)
		chisq += (temp=y[j]-(aa+bb*x[j]),temp*temp);
	sigb=sqrt(chisq/del);
	b1=bb;
	f1=rofunc(b1);
	b2=bb+SIGN(3.0*sigb,f1);
	f2=rofunc(b2);
	if (b2 == b1) {
		*a=aa;
		*b=bb;
		*abdev=abdevt/ndata;
		return;
	}
	while (f1*f2 > 0.0) {
		bb=b2+1.6*(b2-b1);
		b1=b2;
		f1=f2;
		b2=bb;
		f2=rofunc(b2);
	}
	sigb=0.01*sigb;
	while (fabs(b2-b1) > sigb) {
		bb=b1+0.5*(b2-b1);
		if (bb == b1 || bb == b2) break;
		f=rofunc(bb);
		if (f*f1 >= 0.0) {
			f1=f;
			b1=bb;
		} else {
			f2=f;
			b2=bb;
		}
	}
	*a=aa;
	*b=bb;
	*abdev=abdevt/ndata;
}
#define EPS 1.0e-7

extern int ndatat;
extern double *xt,*yt,aa,abdevt;

double rofunc(b)
  double b ;
{
	int j;
	double *arr,d,sum=0.0;

	arr=vector(1,ndatat);
	for (j=0;j<ndatat;j++) arr[j]=yt[j]-b*xt[j];
	if (ndatat & 1) {
		aa=nrselect((ndatat+1)>>1,ndatat,arr);
	}
	else {
		j=ndatat >> 1;
		aa=0.5*(nrselect(j,ndatat,arr)+nrselect(j+1,ndatat,arr));
	}
	abdevt=0.0;
	for (j=0;j<ndatat;j++) {
		d=yt[j]-(b*xt[j]+aa);
		abdevt += fabs(d);
		if (yt[j] != 0.0) d /= fabs(yt[j]);
		if (fabs(d) > EPS) sum += (d >= 0.0 ? xt[j] : -xt[j]);
	}
	free_vector(arr,1,ndatat);
	return sum;
}
#undef EPS

#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;

double nrselect(k, n, arr)
  unsigned long k, n ;
  double arr[] ;
{
	unsigned long i,ir,j,l,mid;
	double a,temp;

	l=1;
	ir=n;
	for (;;) {
		if (ir <= l+1) {
			if (ir == l+1 && arr[ir] < arr[l]) {
				SWAP(arr[l],arr[ir])
			}
			return arr[k];
		} else {
			mid=(l+ir) >> 1;
			SWAP(arr[mid],arr[l+1])
			if (arr[l] > arr[ir]) {
				SWAP(arr[l],arr[ir])
			}
			if (arr[l+1] > arr[ir]) {
				SWAP(arr[l+1],arr[ir])
			}
			if (arr[l] > arr[l+1]) {
				SWAP(arr[l],arr[l+1])
			}
			i=l+1;
			j=ir;
			a=arr[l+1];
			for (;;) {
				do i++; while (arr[i] < a);
				do j--; while (arr[j] > a);
				if (j < i) break;
				SWAP(arr[i],arr[j])
			}
			arr[l+1]=arr[j];
			arr[j]=a;
			if (j >= k) ir=j-1;
			if (j <= k) l=i;
		}
	}
}
#undef SWAP

void ttest(data1, n1,  data2,  n2, t, prob)
  double data1[], data2[], *t, *prob ;
  unsigned long n1, n2 ;
{
	double var1,var2,svar,df,ave1,ave2;

	avevar(data1,n1,&ave1,&var1);
	avevar(data2,n2,&ave2,&var2);
	df=n1+n2-2;
	svar=((n1-1)*var1+(n2-1)*var2)/df;
	*t=(ave1-ave2)/sqrt(svar*(1.0/n1+1.0/n2));
	*prob=betai(0.5*df,0.5,df/(df+(*t)*(*t)));
}

void tptest( data1, data2, n, t, prob)
 double data1[], data2[], *t, *prob ;
 unsigned long n ;
{
	unsigned long j;
	double var1,var2,ave1,ave2,sd,df,cov=0.0;

	avevar(data1,n,&ave1,&var1);
	avevar(data2,n,&ave2,&var2);
	for (j=0;j<n;j++)
		cov += (data1[j]-ave1)*(data2[j]-ave2);
	cov /= df=n-1;
	sd=sqrt((var1+var2-2.0*cov)/n);
	*t=(ave1-ave2)/sd;
	*prob=betai(0.5*df,0.5,df/(df+(*t)*(*t)));
}

void tutest(data1, n1, data2, n2, t, prob)
 double data1[], data2[], *t, *prob ;
 unsigned long n1, n2 ;
{
	double var1,var2,df,ave1,ave2;

	avevar(data1,n1,&ave1,&var1);
	avevar(data2,n2,&ave2,&var2);
	*t=(ave1-ave2)/sqrt(var1/n1+var2/n2);
	df=SQR(var1/n1+var2/n2)/(SQR(var1/n1)/(n1-1)+SQR(var2/n2)/(n2-1));
	*prob=betai(0.5*df,0.5,df/(df+SQR(*t)));
}

/* ************************************************************************************************* */
/* CAUTION: This is the ANSI C (only) version of the Numerical Recipes
   utility file nrutil.c.  Do not confuse this file with the same-named
   file nrutil.c that is supplied in the same subdirectory or archive
   as the header file nrutil.h.  *That* file contains both ANSI and
   traditional K&R versions, along with #ifdef macros to select the
   correct version.  *This* file contains only ANSI C.               */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#define NR_END 1
#define FREE_ARG char*

double *vector( nl,  nh)
  long nl, nh ;
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;

	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	if (!v) nrerror("allocation failure in vector()");
	return v-nl+NR_END;
}

int *ivector( nl,  nh)
  long nl, nh ;
/* allocate an int vector with subscript range v[nl..nh] */
{
	int *v;

	v=(int *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(int)));
	if (!v) nrerror("allocation failure in ivector()");
	return v-nl+NR_END;
}

unsigned char *cvector( nl,  nh)
  long nl, nh ;
/* allocate an unsigned char vector with subscript range v[nl..nh] */
{
	unsigned char *v;

	v=(unsigned char *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(unsigned char)));
	if (!v) nrerror("allocation failure in cvector()");
	return v-nl+NR_END;
}

unsigned long *lvector( nl,  nh)
  long nl, nh ;
/* allocate an unsigned long vector with subscript range v[nl..nh] */
{
	unsigned long *v;

	v=(unsigned long *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(long)));
	if (!v) nrerror("allocation failure in lvector()");
	return v-nl+NR_END;
}

double *dvector( nl,  nh)
  long nl, nh ;
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;

	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	if (!v) nrerror("allocation failure in dvector()");
	return v-nl+NR_END;
}

double **matrix( nrl,  nrh,  ncl,  nch)
  long nrl, nrh, ncl, nch ;
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;

	/* allocate pointers to rows */
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	if (!m) nrerror("allocation failure 1 in matrix()");
	m += NR_END;
	m -= nrl;

	/* allocate rows and set pointers to them */
	m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	if (!m[nrl]) nrerror("allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

double **dmatrix( nrl,  nrh,  ncl,  nch)
  long nrl, nrh, ncl, nch ;
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;

	/* allocate pointers to rows */
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	if (!m) nrerror("allocation failure 1 in matrix()");
	m += NR_END;
	m -= nrl;

	/* allocate rows and set pointers to them */
	m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	if (!m[nrl]) nrerror("allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

int **imatrix( nrl,  nrh,  ncl,  nch)
  long nrl, nrh, ncl, nch ;
/* allocate a int matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
	int **m;

	/* allocate pointers to rows */
	m=(int **) malloc((size_t)((nrow+NR_END)*sizeof(int*)));
	if (!m) nrerror("allocation failure 1 in matrix()");
	m += NR_END;
	m -= nrl;


	/* allocate rows and set pointers to them */
	m[nrl]=(int *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(int)));
	if (!m[nrl]) nrerror("allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

double **submatrix(a,  oldrl,  oldrh,  oldcl,  oldch, newrl,  newcl)
  double **a ;
  long oldrl, oldrh, oldcl, oldch, newrl, newcl ;
/* point a submatrix [newrl..][newcl..] to a[oldrl..oldrh][oldcl..oldch] */
{
	long i,j,nrow=oldrh-oldrl+1,ncol=oldcl-newcl;
	double **m;

	/* allocate array of pointers to rows */
	m=(double **) malloc((size_t) ((nrow+NR_END)*sizeof(double*)));
	if (!m) nrerror("allocation failure in submatrix()");
	m += NR_END;
	m -= newrl;

	/* set pointers to rows */
	for(i=oldrl,j=newrl;i<=oldrh;i++,j++) m[j]=a[i]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

double **convert_matrix(a,  nrl,  nrh,  ncl,  nch)
  double *a ;
  long nrl, nrh, ncl, nch ;
/* allocate a double matrix m[nrl..nrh][ncl..nch] that points to the matrix
declared in the standard C manner as a[nrow][ncol], where nrow=nrh-nrl+1
and ncol=nch-ncl+1. The routine should be called with the address
&a[0][0] as the first argument. */
{
	long i,j,nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;

	/* allocate pointers to rows */
	m=(double **) malloc((size_t) ((nrow+NR_END)*sizeof(double*)));
	if (!m) nrerror("allocation failure in convert_matrix()");
	m += NR_END;
	m -= nrl;

	/* set pointers to rows */
	m[nrl]=a-ncl;
	for(i=1,j=nrl+1;i<nrow;i++,j++) m[j]=m[j-1]+ncol;
	/* return pointer to array of pointers to rows */
	return m;
}

double ***f3tensor( nrl,  nrh,  ncl,  nch,  ndl,  ndh)
  long nrl, nrh, ncl, nch, ndl, ndh ;
/* allocate a double 3tensor with range t[nrl..nrh][ncl..nch][ndl..ndh] */
{
	long i,j,nrow=nrh-nrl+1,ncol=nch-ncl+1,ndep=ndh-ndl+1;
	double ***t;

	/* allocate pointers to pointers to rows */
	t=(double ***) malloc((size_t)((nrow+NR_END)*sizeof(double**)));
	if (!t) nrerror("allocation failure 1 in f3tensor()");
	t += NR_END;
	t -= nrl;

	/* allocate pointers to rows and set pointers to them */
	t[nrl]=(double **) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double*)));
	if (!t[nrl]) nrerror("allocation failure 2 in f3tensor()");
	t[nrl] += NR_END;
	t[nrl] -= ncl;

	/* allocate rows and set pointers to them */
	t[nrl][ncl]=(double *) malloc((size_t)((nrow*ncol*ndep+NR_END)*sizeof(double)));
	if (!t[nrl][ncl]) nrerror("allocation failure 3 in f3tensor()");
	t[nrl][ncl] += NR_END;
	t[nrl][ncl] -= ndl;

	for(j=ncl+1;j<=nch;j++) t[nrl][j]=t[nrl][j-1]+ndep;
	for(i=nrl+1;i<=nrh;i++) {
		t[i]=t[i-1]+ncol;
		t[i][ncl]=t[i-1][ncl]+ncol*ndep;
		for(j=ncl+1;j<=nch;j++) t[i][j]=t[i][j-1]+ndep;
	}

	/* return pointer to array of pointers to rows */
	return t;
}

void free_vector(v,  nl,  nh)
  double *v ; long nl, nh ;
/* free a double vector allocated with vector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_ivector(v,  nl, nh)
 int *v ; long nl, nh ;
/* free an int vector allocated with ivector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_cvector(v,  nl, nh)
 unsigned char *v ; long nl, nh ;
/* free an unsigned char vector allocated with cvector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_lvector(v,  nl,  nh)
  unsigned long *v ; long nl, nh ;
/* free an unsigned long vector allocated with lvector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_dvector(v, nl, nh)
  double *v ; long nl, nh ;
/* free a double vector allocated with dvector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_matrix(m, nrl, nrh, ncl, nch)
  double **m ; long nrl, nrh, ncl, nch ;
/* free a double matrix allocated by matrix() */
{
	free((FREE_ARG) (m[nrl]+ncl-NR_END));
	free((FREE_ARG) (m+nrl-NR_END));
}

void free_dmatrix(m, nrl, nrh, ncl, nch)
  double **m ; long nrl, nrh, ncl, nch ;
/* free a double matrix allocated by dmatrix() */
{
	free((FREE_ARG) (m[nrl]+ncl-NR_END));
	free((FREE_ARG) (m+nrl-NR_END));
}

void free_imatrix(m, nrl, nrh, ncl, nch)
  double **m ; long nrl, nrh, ncl, nch ;
/* free an int matrix allocated by imatrix() */
{
	free((FREE_ARG) (m[nrl]+ncl-NR_END));
	free((FREE_ARG) (m+nrl-NR_END));
}

void free_submatrix(b, nrl, nrh, ncl, nch)
  double **b ; long nrl, nrh, ncl, nch ;
/* free a submatrix allocated by submatrix() */
{
	free((FREE_ARG) (b+nrl-NR_END));
}

void free_convert_matrix(b, nrl, nrh, ncl, nch)
  double **b ; long nrl, nrh, ncl, nch ;
/* free a matrix allocated by convert_matrix() */
{
	free((FREE_ARG) (b+nrl-NR_END));
}

void free_f3tensor(t, nrl, nrh, ncl, nch, ndl, ndh)
  double ***t ; long nrl, nrh, ncl, nch, ndl, ndh ;
/* free a double f3tensor allocated by f3tensor() */
{
	free((FREE_ARG) (t[nrl][ncl]+ndl-NR_END));
	free((FREE_ARG) (t[nrl]+ncl-NR_END));
	free((FREE_ARG) (t+nrl-NR_END));
}
#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define MASK 123459876

static long idum0=MASK+1,idum1= -MASK+1 ;
static long ran2seed=-MASK ;

void v4stat_RanSeed(type,seed)
  INDEX type ;
  UB64INT seed ;
{ int i ;

	switch(type)
	 { case 0: srand((int)seed) ; break ;	/* Seed system generator */
	   case 1: idum0 = (seed == MASK ? MASK+1 : (int)seed) ;
		   i = idum0 % 100 ; for(;i-- > 0;) v4stat_ran0() ;
		   break ;
	   case 2: idum1 = (seed < 0 ? (int)seed : -((int)seed+1)) ; break ;
	   case 3: ran2seed = (seed < 0 ? (int)seed : -((int)seed+1)) ; break ;
	   case 4: vRan64_Seed(seed) ; break ;
	 } ;
}

double v4stat_ran0()
{ 

	long k;
	double ans;

	idum0 ^= MASK;
	k=(idum0)/IQ;
	idum0=IA*(idum0-k*IQ)-IR*k;
	if (idum0 < 0) idum0 += IM;
	ans=AM*(idum0);
	idum0 ^= MASK;
	return ans;
}
#undef IA
#undef IM
#undef AM
#undef IQ
#undef IR
#undef MASK

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1E-300
#define RNMX (1.0-EPS)

double v4stat_ran1()
{

	int j;
	long k;
	static long iy=0;
	static long iv[NTAB];
	double temp;

	if (idum1 <= 0 || !iy) {
		if (-(idum1) < 1) idum1=1;
		else idum1 = -(idum1);
		for (j=NTAB+7;j>=0;j--) {
			k=(idum1)/IQ;
			idum1=IA*(idum1-k*IQ)-IR*k;
			if (idum1 < 0) idum1 += IM;
			if (j < NTAB) iv[j] = idum1;
		}
		iy=iv[0];
	}
	k=(idum1)/IQ;
	idum1=IA*(idum1-k*IQ)-IR*k;
	if (idum1 < 0) idum1 += IM;
	j=iy/NDIV;
	iy=iv[j];
	iv[j] = idum1;
	if ((temp=AM*iy) > RNMX) return RNMX;
	else return temp;
}
#undef IA
#undef IM
#undef AM
#undef IQ
#undef IR
#undef NTAB
#undef NDIV
#undef EPS
#undef RNMX


#define IM1 2147483563
#define IM2 2147483399
#define AM (1.0/IM1)
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 32
#define NDIV (1+IMM1/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

double v4stat_ran2()
{
	int j;
	long k;
	static long idum2=123456789;
	static long iy=0;
	static long iv[NTAB];
	double temp;

	if (ran2seed <= 0) {
		if (-(ran2seed) < 1) ran2seed=1;
		else ran2seed = -(ran2seed);
		idum2=(ran2seed);
		for (j=NTAB+7;j>=0;j--) {
			k=(ran2seed)/IQ1;
			ran2seed=IA1*(ran2seed-k*IQ1)-k*IR1;
			if (ran2seed < 0) ran2seed += IM1;
			if (j < NTAB) iv[j] = ran2seed;
		}
		iy=iv[0];
	}
	k=(ran2seed)/IQ1;
	ran2seed=IA1*(ran2seed-k*IQ1)-k*IR1;
	if (ran2seed < 0) ran2seed += IM1;
	k=idum2/IQ2;
	idum2=IA2*(idum2-k*IQ2)-k*IR2;
	if (idum2 < 0) idum2 += IM2;
	j=iy/NDIV;
	iy=iv[j]-idum2;
	iv[j] = ran2seed;
	if (iy < 1) iy += IMM1;
	if ((temp=AM*iy) > RNMX) return RNMX;
	else return temp;
}
#undef IM1
#undef IM2
#undef AM
#undef IMM1
#undef IA1
#undef IA2
#undef IQ1
#undef IQ2
#undef IR1
#undef IR2
#undef NTAB
#undef NDIV
#undef EPS
#undef RNMX
/* (C) Copr. 1986-92 Numerical Recipes Software 6)#41jQ.n.. */









/* ************************************************************************************************* */

/*	end Numerical Recipies				*/

/*	v4stat_StatAdjCosSim - Adjusted Cosine Similarity	*/

double v4stat_StatAdjCosSim(countx,numxs,county,numys,countz,numzs,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  int countz ;
  double numzs[] ;		/* Z values */
  LOGICAL *ok ;
{ double iNum,yNum,top,botl,botr,res ; INDEX ix ;

	top = 0 ; botl = 0 ; botr = 0 ;
	for(ix=0;ix<countx;ix++)
	 { iNum = numxs[ix] - numzs[ix] ; yNum = numys[ix] - numzs[ix] ;
	   top += (iNum * yNum) ;
	   botl += (iNum * iNum) ; botr += (yNum * yNum) ;
	 } ;
	res = top / (sqrt(botl) * sqrt(botr)) ;
	return(res) ;
}

/*	v4stat_AvgDev - Average Deviation		*/

double v4stat_AvgDev(count,nums,ok)
  int count ; LOGICAL *ok ;			/* Number of nums */
  double nums[] ;			/* Array of numbers */
{ double avg,sum ; int i ;

	if (count == 0.0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatAvgDev) ; return(0.0) ; } ;
	for(i=0,avg=0.0;i<count;i++) { avg += nums[i] ; } ;
	avg = avg / count ;
	for(i=0,sum=0.0;i<count;i++) { sum += fabs(nums[i]-avg) ; } ;
	*ok = TRUE ; return(sum/count) ;
}

/*	v4stat_Avg - Average		*/

double v4stat_Avg(count,nums,ok)
  int count,*ok ;			/* Number of nums */
  double nums[] ;			/* Array of numbers */
{ double avg ; int i ;

	if (count == 0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatAvg) ; return(0.0) ; } ;
	for(i=0,avg=0.0;i<count;i++) { avg += nums[i] ; } ;
	*ok = TRUE ; return(avg / count) ;
}

/*	v4stat_BetaDist - Cumualative beta Probability Density Function */

double v4stat_BetaDist(x,alpha,beta,A,B,cum)
  double x ;			/* Value at which to evaluate */
  double alpha,beta ;		/* Parameters to the distribution */
  double A,B ;			/* Optional lower/upper bound to interval of x */
  int cum ;			/* True for cumulative */
{ double t,b ;

	x = (x-A)/(B-A) ;	/* Make sure x over interval 0<=x<=1 */
	if (!cum)
	 { t = pow(x,alpha-1.0) * pow(1.0-x,beta-1.0) ;
	   b = v4stat_Gamma(alpha)*v4stat_Gamma(beta)/v4stat_Gamma(alpha+beta) ;
	   return(t/b) ;
	 } ;
	return(betai(alpha,beta,x)) ;
}

/*	v4stat_BetaInv - Inverse of BetaDist		*/

double v4stat_BetaInv(prob,alpha,beta,A,B,ok)
  double prob ;			/* Probability to shoot for */
  double alpha,beta ;		/* Parameters to the distribution */
  double A,B ;			/* Optional lower/upper bound to interval of x */
  LOGICAL *ok ;
{ double up,low,tx, fx ;
  struct V4C__Context *ctx ;		/* Current context */
  int max ;

	*ok = TRUE ; low = A ; up = B ;
	for(max=0;max<MAXTRIES;max++)
	 { 
	   tx = low + (up - low)/2 ;
	   fx = v4stat_BetaDist(tx,alpha,beta,A,B,TRUE) ;
	   if (up-low < 0.0000001) return(tx) ;
	   if (fabs(fx-prob) < 0.000001) return(tx) ;	/* Quit already */
	   if (fx < prob)
	    { up = tx ; }
	    else { low = tx ; } ;
	 } ;
	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	*ok = FALSE ; return(1.0) ;
}

/*	v4stat_BinomDist - Individual Binomial Distribution Probability	*/

double v4stat_BinomDist(numok,trials,probok,cum,ok)
  double numok ;		/* Number of successes */
  double trials ;		/* Number of trials */
  double probok ;		/* Probability of success on each trial (0..1) */
  LOGICAL cum,*ok ;
{ int y ; double sum,d1 ;

	if (cum)
	 { for(y=0,sum=0.0;y<=numok;y++)
	    { sum += v4stat_BinomDist((double)y,trials,probok,FALSE,ok) ; if (!ok) return(1.0) ;
	    } ;
	   return(sum) ;
	 } else
	 { 
//	   d1 = v4stat_NCombR(DtoI(trials),DtoI(numok),ok) ;
//	   if (!*ok) return(1.0) ;
//	   *ok = TRUE ;
//	   return(d1*pow(probok,numok)*pow(1.0-probok,trials-numok)) ;
	   double v4stat_logNCombR(int,int,LOGICAL *) ;
	   d1 = v4stat_logNCombR(DtoI(trials),DtoI(numok),ok) ;
	   if (!*ok) return(1.0) ;
	   *ok = TRUE ;
	   sum = d1 + (log(probok)*numok) + (log(1.0-probok)*(trials-numok)) ;
	   return(exp(sum)) ;
	   
	   
	 } ;
}

/*	v4stat_ChiDist - One-Tailed Probability of the chi-squared distribution	*/

double v4stat_ChiDist(x,degfree)
  double x ;			/* Value at which to evaluate */
  double degfree ;		/* Degrees of freedom */
{
	return(gammq(degfree/2.0,x/2.0)) ;
}

/*	v4stat_ChiInv - Inverse of ChiTest		*/

double v4stat_ChiInv(prob,degfree,ok)
  double prob ;			/* Probability to match */
  double degfree ;		/* Degrees of Freedom */
  LOGICAL *ok ;
{ double up,low,tx, fx ;
  struct V4C__Context *ctx ;		/* Current context */
  int max ;

	*ok = TRUE ; low = 0.0 ; up = 1000000.0 ;
	for(max=0;max<MAXTRIES;max++)
	 { 
	   tx = low + (up - low)/2 ;
	   fx = v4stat_ChiDist(tx,degfree) ;
	   if (up-low < 0.0000001) return(tx) ;
	   if (fabs(fx-prob) < 0.000001) return(tx) ;	/* Quit already */
	   if (fx < prob)
	    { up = tx ; }
	    else { low = tx ; } ;
	 } ;
	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	*ok = FALSE ; return(1.0) ;
}

/*	v4stat_ChiTest - Chi-squared test of independence	*/

double v4stat_ChiTest(constraints,countx,numxs,county,numys)
  double constraints ;		/* Number of constraints */
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
{ double df,chsq,prob ;

	chsone(numxs,numys,countx,DtoI(constraints),&df,&chsq,&prob) ;
	return(prob) ;
}

/*	v4stat_Confidence - Confidence Interval for a Population	*/

double v4stat_Confidence(alpha,stddev,size,ok)
  double alpha ;		/* Significance level */
  double stddev ;
  double size ;			/* Sample size */
  LOGICAL *ok ;
{ double area ;

	*ok = TRUE ;
	area = v4stat_NormStdInv(1.0 - alpha/2,ok) ;	/* Get area from standard normal curve */
	return(area*stddev/sqrt(size)) ;
}

/*	v4stat_Correl - Correlation Coefficient between two arrays	*/

double v4stat_Correl(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double d1,d2,d3 ;

	d1 = v4stat_Covar(countx,numxs,county,numys,ok) ;
	if (!ok) return(1.0) ;
	d2 = v4stat_StdDevP(countx,numxs,ok) ; if (!ok) return(1.0) ;
	d3 = v4stat_StdDevP(county,numys,ok) ; if (!ok) return(1.0) ;
	d2 *= d3 ;
	if (d2 == 0.0)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatSDZero",V4IM_OpCode_StatCovar) ; return(0.0) ; } ;
	return(d1/d2) ;
}

/*	v4stat_Covar - Covariance - Avg of the products of deviations */

double v4stat_Covar(countx,numxs,county,numys,ok)
  int countx,*ok ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
{ double sumx=0.0,sumy=0.0,avgx=0.0,avgy=0.0,sum=0.0 ;
  int i ;

	if (countx != county) { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	if (countx == 0.0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatCovar) ; return(0.0) ; } ;
	for(i=0;i<countx;i++) { sumx += numxs[i] ; sumy += numys[i] ; } ;
	avgx = sumx / (double)countx ; avgy = sumy / (double)county ;
	for(i=0;i<countx;i++) { sum += (numxs[i] - avgx)*(numys[i] - avgy) ; } ;
	*ok = TRUE ; return(sum/(double)countx) ;
}

/*	v4stat_CritBinom - Smallest integer s.t. Cum BinDist is >= alpha	*/

double v4stat_CritBinom(trials,probs,alpha,ok)
  double trials ;		/* Number of Bernoulli trials */
  double probs ;		/* Probability of success */
  double alpha ;		/* Criterion value */
  LOGICAL *ok ;
{ double k,r ;

	for(k=0.0;;k++)
	 { r = v4stat_BinomDist(k,trials,probs,TRUE,ok) ;
	   if (!*ok) return(1.0) ;
	   if (r >= alpha) return(k) ;
	 } ;
}

/*	v4stat_DevSq - Sum of Squares Deviation	*/

double v4stat_DevSq(count,nums,ok)
  int count ;				/* Number of nums */
  double nums[] ;			/* Array of numbers */
  LOGICAL *ok ;
{ double avg,sum ; int i ;

	for(i=0,avg=0.0;i<count;i++) { avg += nums[i] ; } ;
	if (count == 0.0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatDevSq) ; return(0.0) ; } ;
	avg = avg / count ;
	for(i=0,sum=0.0;i<count;i++) { sum += (nums[i]-avg)*(nums[i]-avg) ; } ;
	return(sum) ;
}

/*	v4stat_ErrF - Error Function			*/

double v4stat_ErrF(upper)
  double upper ;		/* Upper bounds */
{
	return(v4errff(upper)) ;
}

/*	v4stat_ErrFC - Complementary Error Function		*/

double v4stat_ErrFC(upper)
  double upper ;		/* Upper bounds */
{
	return(erffc(upper)) ;
}

/*	v4stat_ExponDist - Exponential Distribution	*/

double v4stat_ExponDist(x,lambda,cum)
  double x ;		/* Value of the function */
  double lambda ;	/* The parameter value */
  LOGICAL cum ;		/* TRUE for cumulative, FALSE for p.d.f. */
{
	if (cum)
	 { return(1.0 - exp(-lambda*x)) ;
	 } else
	 { return(lambda*exp(-lambda*x)) ;
	 } ;
}

/*	v4stat_FDist - F Probability Distribution	*/

double v4stat_FDist(x,dof1,dof2)
  double x ;		/* Value at which to evaluate */
  double dof1 ;		/* Degrees of freedom- numerator */
  double dof2 ;		/* ditto- denominator */
{
	return(betai(dof2/2.0,dof1/2.0,dof2/(dof2+dof1*x))) ;
}

/*	v4stat_FInv - Inverse of FDist			*/

double v4stat_FInv(prob,dof1,dof2,ok)
  double prob ;			/* Probability to shoot for */
  double dof1,dof2 ;		/* Degrees-of-Freedom */
  LOGICAL *ok ;
{ double up,low,tx, fx ;
  struct V4C__Context *ctx ;		/* Current context */
  int max ;

	*ok = TRUE ; low = 0.0 ; up = 1000000.0 ;
	for(max=0;max<MAXTRIES;max++)
	 { 
	   tx = low + (up - low)/2 ;
	   fx = v4stat_FDist(tx,dof1,dof2) ;
	   if (up-low < 0.0000001) return(tx) ;
	   if (fabs(fx-prob) < 0.000001) return(tx) ;	/* Quit already */
	   if (fx < prob)
	    { up = tx ; }
	    else { low = tx ; } ;
	 } ;
	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	*ok = FALSE ; return(1.0) ;
}

/*	v4stat_Forecast - Forecast a Y given list of x's/y's and new X	*/

double v4stat_Forecast(x,countx,numxs,county,numys)
  double x ;			/* Value at which to calculate */
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
{ double a,b,siga,sigb,chi2,q ;

	fit(numxs,numys,countx,NULL,0,&a,&b,&siga,&sigb,&chi2,&q) ;
	return(a + b*x) ;
}

/*	v4stat_Frequency - Frequency of a number or range in a list */

double v4stat_Frequency(dnum,countx,numxs)
  double dnum ;
  int countx ;
  double numxs[] ;		/* X values */
{ 

	return(0.0) ;
}
/*	v4stat_FTest - Results of an F-Test		*/

double v4stat_FTest(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double f,prob ;

	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	ftest(numxs,countx,numys,county,&f,&prob) ;
	*ok = TRUE ; return(prob) ;
}

/*	v4stat_Fisher - Fisher Transformation		*/

double v4stat_Fisher(x)
  double x ;		/* Value to transform */
{
	return(0.5 * log((1.0+x)/(1.0-x))) ;
}

/*	v4stat_FisherInv - Inverse			*/

double v4stat_FisherInv(y)
  double y ;		/* Fisher number to be inversed */
{
	return( (exp(2.0*y) - 1.0) / (exp(2.0*y) + 1.0) ) ;
}

/*	v4stat_GammaDist - Gamma Distribution	*/

double v4stat_GammaDist(x,alpha,beta,cum)
  double x ;		/* Value to evaluate distribution */
  double alpha ;	/* Parameter to the distribution */
  double beta ;		/* ditto, if beta=1 return standard gamma distribution */
  LOGICAL cum ;		/* TRUE for cumulative, FALSE probabiltity mass fucntion */
{ int i,ialpha ; double alphaf,p1,y1,y2,z,sum,inc ;

	ialpha = (int)alpha ;
	for(i=ialpha-1,alphaf=1.0;i>1;i--) { alphaf *= i ; } ;
	if (!cum) return(pow(x,alpha-1.0)*exp(-x/beta)/(pow(beta,alpha)*alphaf)) ;
	inc = 0.1 ;
	z = 0.0 ;				/* Starting point for integral */
	p1 = pow(beta,alpha)*alphaf ;
	y1 = pow(z,alpha-1.0)*exp(-z/beta)/p1 ;
	for(sum=0.0;z<=x;)
	 { 
	   y2 = pow(z+inc,alpha-1.0)*exp(-(z+inc)/beta)/p1 ;
	   if (fabs(y1-y2) > 0.00001) { if (inc > 0.0001) { inc = inc / 2.0 ; continue ; } ; } ;
	   z += inc ; sum += inc * y2 ;
	   sum += ((y1-y2)*inc)/2.0 ;
	   if (fabs(y1-y2) < 0.00000001 && inc < 2.0) { inc = inc * 2.0 ; } ;
	   y1 = y2 ;
	 } ;
	return(sum) ;
}

/*	v4stat_GammaInv - Inverse */

double v4stat_GammaInv(x,alpha,beta,ok)
  double x ;		/* Probability */
  double alpha ;	/* Parameter to the distribution */
  double beta ;		/* ditto, if beta=1 return standard gamma distribution */
  LOGICAL *ok ;
{ double up,low,tx, fup,flow,fx ;
  struct V4C__Context *ctx ;		/* Current context */
  int max ;

	*ok = TRUE ; low = 0.0 ; up = 10.0*alpha ;
	fup = v4stat_GammaDist(up,alpha,beta,TRUE) ;
	flow = v4stat_GammaDist(low,alpha,beta,TRUE) ;
	for(max=0;max<MAXTRIES;max++)
	 { 
	   tx = low + (up - low)/2 ;
	   fx = v4stat_GammaDist(tx,alpha,beta,TRUE) ;
	   if (up-low < 0.0000001) return(tx) ;
	   if (fabs(fx-x) < 0.000001) return(tx) ;	/* Quit already */
	   if (fx > x)
	    { up = tx ; }
	    else { low = tx ; } ;
	 } ;
	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	*ok = FALSE ; return(1.0) ;
}

/*	v4stat_Gamma - Gamma function	*/

double v4stat_Gamma(x)
  double x ;
{
	if (x <= 1.0) return(1.0) ;
	return(exp(gammln(x))) ;
}

/*	v4stat_GammaLn - Natural Log of gamma function */

double v4stat_GammaLn(x)
  double x ;		/* Value */
{ 

	return(gammln(x)) ;
}

/*	v4stat_GeoMean - Geometric Mean of List		*/

double v4stat_GeoMean(count,nums,ok)
  int count ; LOGICAL *ok ;		/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ double p ; int i ;

	for(i=0,p=1.0;i<count;i++) { p *= nums[i] ; } ;
	*ok = TRUE ; return(pow(p,1.0/(double)count)) ;
}

/*	v4stat_HarMean - Harmonic Mean of List		*/

double v4stat_HarMean(count,nums,ok)
  int count ; LOGICAL *ok ;		/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ double sum ; int i ;

	if (count == 0.0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatHarMean) ; return(0.0) ; } ;
	for(i=0,sum=0.0;i<count;i++) { sum += 1.0/nums[i] ; } ;
	*ok = TRUE ; return(1.0 / ((1.0/count) * sum) ) ;
}

/*	v4stat_Kurtosis - Kurtosis of data set		*/

double v4stat_Kurtosis(count,nums,ok)
  int count ; LOGICAL *ok ;		/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ double p1,p2,p3,sd,avg,t ; int i ;

	if (count < 4) { v_Msg(NULL,NULL,"StatMinPoints",4) ; *ok = FALSE ; return(0.0) ; } ;
	p1 = (count)*(count+1.0) / (double)((count-1)*(count-2)*(count-3)) ;
	sd = v4stat_StdDev(count,nums,ok) ; if (!ok) return(0.0) ;
	if (sd == 0.0)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatSDZero",V4IM_OpCode_StatKurtosis) ; return(0.0) ; } ;
	for(i=0,avg=0.0;i<count;i++) { avg += nums[i] ; } ;
	avg = avg / count ;
	for(i=0,p2=0.0;i<count;i++) { t = (nums[i] - avg) / sd ; p2 += t*t*t*t ; } ;
	p3 = 3.0 * (count-1) * (count-1) / ( (count-2)*(count-3) ) ;
	*ok = TRUE ; return((p1 * p2) - p3) ;
}

/*	v4stat_LinFit - Linear Regression Fitting Module */

void v4stat_LinFit(slf,ok)
  struct V4IM__StatLinFit *slf ; /* Structure with all x/y info */
  LOGICAL *ok ;
{ double a,b,siga,sigb,chi2,q,abdev ;
  struct V4C__Context *ctx ;		/* Current context */

	if (slf->Y->Count <= 2)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution3Arg") ;
	   *ok = FALSE ; return ;
	 } ;
	*ok = TRUE ;
	switch (slf->Method)
	 {
	   case V4IM_StatLF_MethodSLLS:
	     if (slf->XCount == 1)	/* Doing simple single-x regression? */
	      { fit(slf->Xs[0].X->Pay,slf->Y->Pay,slf->Y->Count,NULL,0,&a,&b,&siga,&sigb,&chi2,&q) ;
	        slf->Xs[0].Coefficient = b ; slf->Intercept = a ; slf->ChiSq = chi2 ;
	        return ;
	      } ;
	     break ;
	   case V4IM_StatLF_MethodSLLAD:
	     if (slf->XCount == 1)	/* Doing simple single-x regression? */
	      { medfit(slf->Xs[0].X->Pay,slf->Y->Pay,slf->Y->Count,&a,&b,&abdev) ;
	        slf->Xs[0].Coefficient = b ; slf->Intercept = a ; slf->AbsDev = abdev ;
	        return ;
	      } ;
	     break ;
	 } ;
}

/*	v4stat_LogInv - Inverse of the LogNormal Distribution */

double v4stat_LogInv(prob,mean,stddev,ok)
  double prob ;			/* Probability */
  double mean ;			/* Mean of x */
  double stddev ;		/* Standard deviation of x */
  LOGICAL *ok ;
{ double t ;

	t = v4stat_NormStdInv(prob,ok) ;
	return(exp(mean + stddev*t)) ;
}

/*	v4stat_LogNormDist - LogNormal Cumulative Distribution	*/

double v4stat_LogNormDist(x,mean,stddev)
  double x ;
  double mean ;			/* Mean of x */
  double stddev ;		/* Standard deviation of x */
{

	return(v4stat_NormStdDist((log(x)-mean)/stddev)) ;
}

/*	v4stat_HyperGeo - HyperGeometric Distribution */

double v4stat_HyperGeo(Samples,Trials,NumS,NumP,ok)
 double Samples, Trials, NumS, NumP ;
 LOGICAL *ok ;
{
  double r1, r2=0, r3=0 ;

	r1 = v4stat_logNCombR(DtoI(NumS),DtoI(Samples),ok) ;
	if (*ok) r2 = v4stat_logNCombR(DtoI(NumP-NumS),DtoI(Trials-Samples),ok) ;
	if (*ok) r3 = v4stat_logNCombR(DtoI(NumP),DtoI(Trials),ok) ;
	return(exp((r1+r2)-r3)) ;
}

/*	v4stat_Max - Returns largest number in list	*/

double v4stat_Max(count,nums,ok)
  int count ; LOGICAL *ok ;		/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ double x ; int i ;

	if (count <= 0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatMax) ; return(0.0) ; } ;
	x = -DBL_MAX ;
	for(i=0;i<count;i++) { if (nums[i]>x) x = nums[i] ; } ;
	*ok = TRUE ; return(x) ;

}

/*	v4stat_Median - Returns median in a list of numbers	*/

int v4stat_DoubleCompare(p1,p2)
  const void *p1,*p2 ;
{
	return((int)((*(double *)p1)-(*(double *)p2))) ;
}

double v4stat_Median(count,nums,ok)
  int count ; LOGICAL *ok ;		/* Number of nums */
  double nums[] ;		/* Array of numbers */
{
	*ok = TRUE ;
	if (count <= 0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatMedian) ; return(0.0) ; } ;
	if (count == 1) return(nums[0]) ;
	qsort(nums,count,sizeof(double),v4stat_DoubleCompare) ;
	if ((count % 2) == 0)		/* Do we have an even number? */
	 { return((nums[count/2]+nums[count/2-1])/2) ;
	 } else				/*  no have odd - take middle number */
	 { return(nums[count/2]) ;
	 } ;
}

/*	v4stat_Min - Returns smallest number in list	*/

double v4stat_Min(count,nums,ok)
  int count ; LOGICAL *ok ;		/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ double x ; int i ;

	if (count <= 0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatMin) ; return(0.0) ; } ;
	x = DBL_MAX ;
	for(i=0;i<count;i++) { if (nums[i]<x) x = nums[i] ; } ;
	*ok = TRUE ; return(x) ;

}

/*	v4stat_Mode - Returns most common number in list	*/

double v4stat_Mode(count,nums,ok)
  int count ; LOGICAL *ok ;			/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ double mode ; int i,j,cnt ;

	if (count <= 0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatMode) ; return(0.0) ; } ;
	qsort(nums,count,sizeof(double),v4stat_DoubleCompare) ;
	for(i=0,mode=nums[1],cnt=0;i<count;)
	 { for(j=i+1;j<count;j++)
	    { if (nums[j] != nums[i]) break ;		/* Have more of the same? */
	    } ;
	   if (j-i > cnt) { cnt = j-i ; mode = nums[i] ; } ;
	   i = j ;
	 } ;
	if (cnt <= 1)
	 { v_Msg(NULL,NULL,"StatNoMode",V4IM_OpCode_StatMode) ;
	   *ok = FALSE ; return(DBL_MIN) ;
	 } ;
	*ok = TRUE ; return(mode) ;
}

///*	v4stat_NCombR - Combinations of N things r at a time	*/
//
//double v4stat_NCombR(N,r,ok)
//  int N,r ;
//  LOGICAL *ok ;
//{ double Nfact,rfact,Nrfact ; int i ;
//  struct V4C__Context *ctx ;		/* Current context */
//  
//	if (N == 0 && r != 0)
//	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
//	   *ok = FALSE ; return(1.0) ;
//	 } ;
//	*ok = TRUE ;
//	if (r == 0) return(1.0) ;
//	for(i=N,Nfact=1;i>1;i--) { Nfact *= i ; } ;
//	for(i=r,rfact=1;i>1;i--) { rfact *= i ; } ;
//	for(i=N-r,Nrfact=1;i>1;i--) { Nrfact *= i ; } ;
//	return(Nfact/(Nrfact*rfact)) ;
//}

/*	v4stat_logNCombR - Log of combinations of N things r at a time	*/
double v4stat_logNCombR(N,r,ok)
  int N,r ;
  LOGICAL *ok ;
{ double Nfact,rfact,Nrfact ;
  struct V4C__Context *ctx ;		/* Current context */
  double tres ;


	if (N == 0 && r != 0)
	 { ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	   *ok = FALSE ; return(1.0) ;
	 } ;
	*ok = TRUE ;
	if (r == 0) return(0.0) ;

//	for(i=2,Nfact=0.0;i<=N;i++) Nfact += log((double)i) ;
//	for(i=2,rfact=0.0;i<=r;i++) rfact += log((double)i) ;
//	for(i=2,Nrfact=0.0;i<=N-r;i++) Nrfact += log((double)i) ;
//	tres = Nfact - (Nrfact+rfact) ;
//	return(tres) ;

	LNFACTORIAL(rfact,r) ;
	LNFACTORIAL(Nrfact,N-r) ;
	LNFACTORIAL(Nfact,N) ;
//	if (abs(tres - (Nfact - (Nrfact+rfact))) > 0.0)
//	 printf("%d %g, %d %g, %d %g, %g - %g = %g\n",r,rfact,N-r,Nrfact,N,Nfact,tres,(Nfact - (Nrfact+rfact)),abs(tres - (Nfact - (Nrfact+rfact)))) ;
	tres = Nfact - (Nrfact+rfact) ;
	return(tres) ;
}

/*	v4stat_NegBinomDist - Negative Binomial Distribution	*/

double v4stat_NegBinomDist(numf,nums,probs,ok)
  double numf ;		/* Number of failures */
  double nums ;		/* Number of successes */
  double probs ;	/* Probability of success */
  LOGICAL *ok ;
{ int inumf,inums ; double d1 ;

	inumf = (int)numf ; inums = (int)nums ;		/* Truncate to integers */
	d1 = v4stat_logNCombR(inumf+inums-1,inums-1,ok) ;
	if (!ok) return(1.0) ;
	*ok = TRUE ; 
//	return(d1 * pow(probs,inums) * pow(1.0-probs,inumf)) ;
	return(exp(d1 + log(probs)*inums + log(1.0-probs)*inumf)) ;
}

/*	v4stat_NormDist - Normal Distribution Function		*/

double v4stat_NormDist(x,mean,stddev,cum)
  double x ;
  double mean ;			/* Mean of x */
  double stddev ;		/* Standard deviation of x */
  LOGICAL cum ;			/* TRUE = function, FALSE = probability mass function */
{ double p1,p2 ;

	p1 = sqrt(2.0 * PI) * stddev ;
	if (cum)
	 { p1 = (x-mean)/stddev ;		/* errf(x) = 2*P(x*sqrt(2))-1 where P(x) = integral from -inf to x of std normal dist. */
	   p1 = p1 / sqrt(2.0) ;
	   p2 = v4errff(p1) ;
	   return((p2 + 1.0)/2) ;
	 } ;
	p2 = (x - mean)/stddev ;
	return(exp(-0.5*(p2*p2))/p1) ;
}

/*	v4stat_NormInv - Inverse of Normal Cumulative Distribution */

double v4stat_NormInv(prob,mean,stddev,ok)
  double prob ;			/* Probability corresponding to Normal Distribution */
  double mean ;			/* The mean of the distribution */
  double stddev ;		/*  & the std deviation */
  LOGICAL *ok ;
{ double up,low,x, fup,flow,fx ;
  struct V4C__Context *ctx ;		/* Current context */
  int max ;

	*ok = TRUE ;
	if (prob > 0.5)					/* Which side of curve are we on? */
	 { low = mean ; up = mean + 100.0*stddev ; }
	 else { up = mean ; low = mean - 100.0*stddev ; } ;
	fup = v4stat_NormDist(up,mean,stddev,TRUE) ;
	flow = v4stat_NormDist(low,mean,stddev,TRUE) ;
	for(max=0;max<MAXTRIES;max++)
	 { 
	   x = low + (up - low)/2 ;
	   fx = v4stat_NormDist(x,mean,stddev,TRUE) ;
	   if (up - low < 0.0000001) return(x) ;
	   if (fabs(fx-prob) < 0.000001) return(x) ;	/* Quit already */
	   if (fx > prob)
	    { up = x ; }
	    else { low = x ; } ;
	 } ;
	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	*ok = FALSE ; return(1.0) ;
}


/*	v4stat_NormStdDist - Standard Normal Cumulative Distribution	*/

double v4stat_NormStdDist(z)
  double z ;		/* Value to calculate on */
{
	return(v4stat_NormDist(z,0.0,1.0,TRUE)) ;
}

/*	v4stat_NormStdInv - Inverse of above		*/

double v4stat_NormStdInv(prob,ok)
  double prob ;		/* Probability corresponding to the normal distribution */
  LOGICAL *ok ;
{ double up,low,x, fx ;
  struct V4C__Context *ctx ;		/* Current context */
  int max ; double mean, stddev ;

	*ok = TRUE ; mean = 0.0 ; stddev = 1.0 ;	/* Pretty much same as NormInv with fixed mean/stddev */
	if (prob > 0.5)					/* Which side of curve are we on? */
	 { low = mean ; up = mean + 100.0*stddev ; }
	 else { up = mean ; low = mean - 100.0*stddev ; } ;
	for(max=0;max<MAXTRIES;max++)
	 { 
	   x = low + (up - low)/2 ;
	   fx = v4stat_NormDist(x,mean,stddev,TRUE) ;
	   if (up-low < .0000001) return(x) ;
	   if (fabs(fx-prob) < 0.000001) return(x) ;	/* Quit already */
	   if (fx > prob)
	    { up = x ; }
	    else { low = x ; } ;
	 } ;
	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	*ok = FALSE ; return(1.0) ;
}

/*	v4stat_Pearson - ReturnsPearson Product Moment		*/

double v4stat_Pearson(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double sumx=0.0,sumy=0.0,sumxy=0.0,sumx2=0.0,sumy2=0.0,top,bot ;
  int i ;
	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	for(i=0;i<countx;i++)
	 { sumxy += numxs[i]*numys[i] ;
	   sumx += numxs[i] ; sumy += numys[i] ;
	   sumx2 += numxs[i]*numxs[i] ; sumy2 += numys[i]*numys[i] ;
	 } ;
	top = countx*sumxy - sumx*sumy ;
	bot = (countx*sumx2 - sumx*sumx) * (countx*sumy2 - sumy*sumy) ;
	if (bot == 0.0) { v_Msg(NULL,NULL,"StatNoResult",V4IM_OpCode_StatPearson) ; *ok = FALSE ; return(0.0) ; } ;
	*ok = TRUE ; return(top / sqrt(bot)) ;
}

/*	v4stat_Percentile - Returns value at kth percentile	*/

double v4stat_Percentile(kth,count,nums)
  double kth ;			/* The kth percentile (0..1) to pick */
  int count ;			/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ double x ; int xf,xc ;

	qsort(nums,count,sizeof(double),v4stat_DoubleCompare) ;
	x = (count-1) * kth ;
	xf = (int)floor(x) ; xc = (int)ceil(x) ;
	if (x == xf) return(nums[xf]) ;
/*	Have to interpolate between xf (floor) & xc (ceiling) */
	return(nums[xf] + (x-xf)*(nums[xf+1]-nums[xf])) ;
}

/*	v4stat_PercentRank - Returns percentage ranking of x amongst list of values	*/

double v4stat_PercentRank(x,count,nums)
  double x ;			/* The value to rank */
  int count ;			/* Number of nums */
  double nums[] ;		/* Array of numbers */
{ int i ; double y ;

	qsort(nums,count,sizeof(double),v4stat_DoubleCompare) ;
	for(i=0;i<count;i++) { if (nums[i] >= x) break ; } ;
	if (x == nums[i]) return((double)i/(double)(count-1)) ;
	if (x > nums[count-1]) return(100.0) ;
	if (x < nums[0]) return(0.0) ;
	y = (x - nums[i-1])/(nums[i]-nums[i-1]) ;
	return((i+y)/(double)(count-1)) ;

}

/*	v4stat_Permute - Number of permutations		*/

double v4stat_Permute(number,chosen)
  double number ;		/* Number of objects */
  double chosen ;		/* Number in each permutation */
{ int inum,icho,i ;
  double fnum,fcho ;

	inum = DtoI(number) ; icho = DtoI(chosen) ;
	for(i=inum,fnum=1.0;i>1;i--) { fnum *= i ; } ;
	for(i=inum-icho,fcho=1.0;i>1;i--) { fcho *= i ; } ;
	return(fnum/fcho) ;
}

/*	v4stat_Poisson - Poisson Probability Distribution	*/

double v4stat_Poisson(x,mean,cum)
  double x ;		/* Number of events */
  double mean ;		/* Expected mean value */
  LOGICAL cum ;
{ double xfact,sum,r ; int i ;

	if (cum)
	 { for(i=0,sum=0.0;i<=x;i++) { sum += v4stat_Poisson((double)i,mean,FALSE) ; } ;
	   return(sum) ;
	 } else
	 { 
//	   for(i=DtoI(x),xfact=1.0;i>1;i--)
//	    { if (!ISFINITE(xfact))
//	       return(0) ;
//	      xfact *= i ;
//	    } ;
//	   r = pow(mean,x)*exp(-mean)/xfact ;

	   LNFACTORIAL(xfact,x) ;
	   r = exp(log(mean) * x - mean - xfact) ;

	   return(ISFINITE(r) ? r : 0.0) ;

	 } ;
}

	/* Inverse of Poisson */

double v4stat_PoissonInv(prob,mean)
  double prob,mean ;
{ double x,limit ;

	limit = (mean < 1000.0 ? 1000.0 * mean : mean * mean) ;
	for(x=0;;x++)
	 { if (v4stat_Poisson(x,mean,TRUE) >= prob) return(x) ;
	 } ;
}

/*	v4stat_Prob - Calculates probability of a list against list of probs	*/

double v4stat_Prob()
{
	return(0.0) ;
}

/*	v4stat_Product - Calculates product of a list */

double v4stat_Product(count,nums,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;
{ double prod ; int i ;

	prod = 1.0 ;
	for(i=0;i<count;i++) { prod *= nums[i] ; } ;
	*ok = TRUE ; return(prod) ;
}

/*	v4stat_Quartile - Returns quartile of a list	*/

double v4stat_Quartile(quartile,count,nums,ok)
  int quartile ;
  int count ; LOGICAL *ok ;
  double nums[] ;
{ double x ;

	*ok = TRUE ;
	if (quartile < 0 || quartile > 4) { *ok = FALSE ; v_Msg(NULL,NULL,"StatQuartErr",quartile) ; return(1.0) ; } ;
	x = v4stat_PercentRank((double)quartile,count,nums) ;
	return(x) ;
}

P *v4stat_DoStatRan(ctx,respnt,argpnts,argcnt,intmodx)
  struct V4C__Context *ctx ;
  P *respnt ;
  int intmodx ;
  P *argpnts[] ;
  int argcnt ;

{ P *ipt,*cpt,*selpt,ptbuf,lpnt ;
  struct V4L__ListPoint *lp,*sellp ;
#define PROBARRAYMAX 1000
#define MAXSTATRANLISTSIZE 10000
#define MAXLOCALUSED 500
  double probarray[PROBARRAYMAX],cum ; int probc ;
  int i,ix,ok,to,type,haveseed,seed,num,listsize,offset,sellistsize ; double dnum ;

	to = UNUSED ; type = 1 ; haveseed = FALSE ; num = UNUSED ; ok = TRUE ; probc = UNUSED ; listsize = UNUSED ; offset = 0 ;
	sellp = NULL ;
	for(ix=1;ok&&ix<=argcnt;ix++)		/* Step thru the remaining arguments */
	 { ipt = argpnts[ix] ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&ptbuf))
	    { default:		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto statran_fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto statran_fail ;
	      case V4IM_Tag_To:		to = v4im_GetPointInt(&ok,cpt,ctx) ;
					if (to <= 1) { v_Msg(ctx,NULL,"ModArgRange",intmodx,ix,cpt,2,V4LIM_BiggestPositiveInt) ; goto statran_fail ; } ;
					if (to != V4LIM_BiggestPositiveInt) to++ ;
					break ;
	      case V4IM_Tag_Type:	type = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_ListOf:	listsize = v4im_GetPointInt(&ok,cpt,ctx) ;
					if (listsize < 1 || listsize > MAXSTATRANLISTSIZE) { v_Msg(ctx,NULL,"ModArgRange",intmodx,ix,cpt,1,MAXSTATRANLISTSIZE) ; goto statran_fail ; } ;
					break ;
	      case V4IM_Tag_Offset:	offset = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Seed:	haveseed = TRUE ; seed = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Num:	num = v4im_GetPointInt(&ok,cpt,ctx) ; to = num ; break ;
	      case V4IM_Tag_Prob:	lp = v4im_VerifyList(NULL,ctx,cpt,intmodx) ;
					for(cum=0,probc=0;probc<PROBARRAYMAX;probc++)
					 { if(v4l_ListPoint_Value(ctx,lp,probc+1,&lpnt) <= 0) break ;
					   cum += (probarray[probc] = v4im_GetPointDbl(&ok,&lpnt,ctx)) ; if (!ok) break ;
//					   if (probc > 0) probarray[probc] += probarray[probc-1] ;	/* Make it cumulative */
					 } ;
					if (fabs(1.0 - cum) < 0.001)		/* If cumulative is close to one then make cumulative */
					 { for(i=1;i<probc;i++) probarray[i] += probarray[i-1] ;
					 } else
					 { if (fabs(1.0 - probarray[probc-1]) > 0.001)
					    { v_Msg(ctx,NULL,"StatProbArray",intmodx,cum,probarray[probc-1]) ; goto statran_fail ; } ;
					 } ;
					break ;
	      case V4IM_Tag_Select:	selpt = cpt ; sellp = v4im_VerifyList(NULL,ctx,selpt,intmodx) ;
					sellistsize = (sellp == NULL ? -1 : v4l_ListPoint_Value(ctx,sellp,-1,&lpnt)) ;
					if (sellistsize <= 0) { v_Msg(ctx,NULL,"ListSizeErr",intmodx,argpnts[ix]) ; goto statran_fail ; } ;
					break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto statran_fail ; } ;
	if (haveseed) { v4stat_RanSeed(type,seed) ; if (to == UNUSED) { ipt = (P *)&Log_True ; return(ipt) ; } ; } ;
	if (listsize != UNUSED)
	 { int i ; char usedbuf[MAXLOCALUSED], *used ;
	   INITLP(respnt,lp,Dim_List) intPNT(&ptbuf) ;
	   if (listsize <= MAXLOCALUSED) { used = usedbuf ; memset(used,0,listsize) ; }
	    else { used = v4mm_AllocChunk(listsize,TRUE) ; } ;		/* Allocate one byte for each element we want & zero out */
	   for(i=0;i<listsize;i++)
	    { 
	      switch(type)
	       { default:	num = (int)floor(v4stat_ran0() * listsize) ; break ;
	         case 0:	{ double d = rand() % listsize ; num = DtoI(d) ; break ; }
	         case 2:	num = (int)floor(v4stat_ran1() * listsize) ; break ;
	         case 3:	num = (int)floor(v4stat_ran2() * listsize) ; break ;
	         case 4:	num = (int)floor(vRan64_RandomDbl() * listsize) ; break ;
	       } ;
	      for(;;num++)	/* Look for next unused number in sequence starting with num */
	       { if (num >= listsize) num = 0 ; if (used[num] == 0) break ;
	       } ;
	      ptbuf.Value.IntVal = num+1+offset ; used[num] = 1 ;
	      if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&ptbuf,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto statran_fail ; } ;
	    } ;
	   if (listsize > MAXLOCALUSED) { v4mm_FreeChunk(used) ; } ;
	   ENDLP(respnt,lp) ; return(respnt) ;
	 };

	if (sellp != NULL)
	 { to = (num = sellistsize) ;
	   switch(type)
	    { default:	num = (int)floor(v4stat_ran0() * to) ; break ;
	      case 0:	{ double d = rand() % to ; num = DtoI(d) ; break ; }
	      case 2:	num = (int)floor(v4stat_ran1() * to) ; break ;
	      case 3:	num = (int)floor(v4stat_ran2() * to) ; break ;
	      case 4:	num = (int)floor(vRan64_RandomDbl() * to) ; break ;
	    } ; num++ ;
	   if (v4l_ListPoint_Value(ctx,sellp,num,respnt) <= 0) { v_Msg(ctx,NULL,"ListNth",num,selpt) ; goto statran_fail ; } ;
	   return(respnt) ;
	 } ;

	if (probc != UNUSED)
	 {
	   switch(type)
	    { default:	dnum = v4stat_ran0() ; break ;
	      case 0:	dnum = rand()/(RAND_MAX+1.0) ; break ;
	      case 2:	dnum = v4stat_ran1() ; break ;
	      case 3:	dnum = v4stat_ran2() ; break ;
	      case 4:	dnum = vRan64_RandomDbl() ; break ;
	    } ;
	   for(ix=0;ix<probc;ix++)
	    { if (dnum <= probarray[ix]) break ; } ;
//	   ZPH(respnt) ; respnt->Dim = Dim_Int ; respnt->PntType = V4DPI_PntType_Int ; respnt->Bytes = V4PS_Int ;
//	   respnt->Value.IntVal = ix + 1 ;
	   intPNTv(respnt,ix+1) ; return(respnt) ;
	 } ;
	if (to == UNUSED && num == UNUSED)				/* Return random as floating point */
	 { 
//	   ZPH(respnt) ; respnt->PntType = V4DPI_PntType_Foreign ;	/* Force to funky type so SetPointValue forces to Real */
	   switch(type)
	    { default:	dnum = v4stat_ran0() ; break ;
	      case 0:	dnum = rand()/(RAND_MAX+1.0) ; break ;
	      case 2:	dnum = v4stat_ran1() ; break ;
	      case 3:	dnum = v4stat_ran2() ; break ;
	      case 4:	dnum = vRan64_RandomDbl() ; break ;
	    } ;
//	   v4im_SetPointValue(ctx,respnt,dnum) ;
	   dblPNTv(respnt,dnum) ;
	 } else			/* Return as bounded integer */
	 { intPNT(respnt) ;
//	   ZPH(respnt) ; respnt->PntType = V4DPI_PntType_Int ; respnt->Dim = Dim_Int ;
//	   respnt->Bytes = V4PS_Int ;
	   switch(type)
	    { default:	respnt->Value.IntVal = (int)floor(v4stat_ran0() * to) ; break ;
	      case 0:	{ double d = rand() % to ; respnt->Value.IntVal = DtoI(d) ; break ; }
	      case 2:	respnt->Value.IntVal = (int)floor(v4stat_ran1() * to) ; break ;
	      case 3:	respnt->Value.IntVal = (int)floor(v4stat_ran2() * to) ; break ;
	      case 4:	respnt->Value.IntVal = (int)floor(vRan64_RandomDbl() * to) ; break ;
	    } ; if (num != UNUSED) respnt->Value.IntVal++ ;
	 } ;
	return(respnt) ;
statran_fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}

/*	v4stat_Rank - Returns rank of a number within a list	*/

double v4stat_Rank(count,nums,x,start)
  int count ;			/* Count of nums */
  double nums[] ;		/* List of numbers to rank */
  int x ;			/* Number to find */
  int start ;			/* TRUE to rank lowest to highest, FALSE for highest to lowest */
{ int i ;

	qsort(nums,count,sizeof(double),v4stat_DoubleCompare) ;
	if (start)
	 { for(i=0;i<count;i++) { if (x <= nums[i]) break ; } ;
	   if (x == nums[i]) return((double)(i+1)) ;
	   if (i == 0) return(0.0) ;
	   if (i >= count) return(0.0) ;
	   return(i + (x - nums[i])/(nums[i+1]-nums[i])) ;
	 } ;
	for(i=count-1;i>=0;i--) { if (x >= nums[i]) break ; } ;
	if (x == nums[i]) return((double)(count-i)) ;
	if (i == 0) return(0.0) ;
	if (i >= count) return(0.0) ;
	return(count-i + (x - nums[i-1])/(nums[i]-nums[i-1])) ;
}

/*	v4stat_RSQ - Returns r-squared value of linear regression	*/

double v4stat_RSQ(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double sumx=0.0,sumy=0.0,sumxy=0.0,sumx2=0.0,sumy2=0.0,top,bot,r ;
  int i ;
	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	for(i=0;i<countx;i++)
	 { sumxy += numxs[i]*numys[i] ;
	   sumx += numxs[i] ; sumy += numys[i] ;
	   sumx2 += numxs[i]*numxs[i] ; sumy2 += numys[i]*numys[i] ;
	 } ;
	top = countx*sumxy - sumx*sumy ;
	bot = (countx*sumx2 - sumx*sumx) * (countx*sumy2 - sumy*sumy) ;
	if (bot == 0.0) { v_Msg(NULL,NULL,"StatNoResult",V4IM_OpCode_StatRSQ) ; *ok = FALSE ; return(0.0) ; } ;
	r = top / sqrt(bot) ;
	*ok = TRUE ; return(r*r) ;
}

/*	v4im_DoStatScale - Handles StatScale() Module		*/

P *v4im_DoStatScale(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  int intmodx ;
  P *respnt,*argpnts[] ;
  int argcnt,trace ;
{ P *cpt,ptbuf,lptbuf ;
  struct V4L__ListPoint *lp ;
  struct V4DPI__DimInfo *di ;
  struct V4IM_DblArray *npv ;
  int ok,argx,i,resdim,respnttype ; double min,max,dnum ;
  double offset,minimum,maximum,scale ;

	lp = v4im_VerifyList(&lptbuf,ctx,argpnts[1],intmodx) ;
/*	Build up list of values, find min/max while we are at it */
	npv = v_InitDblArray(0) ; min = DBL_MAX ; max = -DBL_MAX ;
	for(i=1;;i++)
	 { if (v4l_ListPoint_Value(ctx,lp,i,&ptbuf) <= 0) break ;
	   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
	   if (i == 1) { resdim = ptbuf.Dim ; respnttype = ptbuf.PntType ; }
	    else { if (resdim != UNUSED) { if (resdim != ptbuf.Dim) resdim = UNUSED ; } ;
		 } ;
	   dnum = v4im_GetPointDbl(&ok,&ptbuf,ctx) ;
	   if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,1) ; goto fail ; } ;
	   npv->Pay[npv->Count++] = dnum ;
	   if (dnum > max) max = dnum ; if (dnum < min) min = dnum ;
	 } ;

	offset = DBL_MAX ; minimum = DBL_MAX ; maximum = DBL_MAX ;
	for(argx=2,ok=TRUE;ok && argx<=argcnt;argx++)
	 { 
	   switch (v4im_CheckPtArgNew(ctx,argpnts[argx],&cpt,&ptbuf))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Dim:	resdim = cpt->Value.IntVal ; DIMINFO(di,ctx,resdim) ; respnttype = di->PointType ; break ;
	      case V4IM_Tag_Linear:
	      case V4IM_Tag_Log:	break ;
	      case V4IM_Tag_Maximum:	maximum = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Minimum:	minimum = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Offset:	offset = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,argx-1) ; goto fail ; } ;

/*	Now scale list of numbers in npv */
	if (offset != DBL_MAX) { for(i=0;i<npv->Count;i++) { npv->Pay[i] += offset ; } ; } ;
	if (maximum != DBL_MAX && minimum != DBL_MAX)
	 { if (maximum <= minimum) { v_Msg(ctx,NULL,"StatMinMax",intmodx,V4IM_Tag_Minimum,minimum,V4IM_Tag_Maximum,maximum) ; goto fail ; } ;
	   offset = minimum - min ; scale = (maximum - minimum) / (max - min) ;
	   for(i=0;i<npv->Count;i++) { npv->Pay[i] = ((npv->Pay[i] - min) * scale) + minimum ; } ;
	 } else
	 { if (maximum != DBL_MAX)
	    { offset = maximum - max ; for(i=0;i<npv->Count;i++) { npv->Pay[i] += offset ; } ; } ;
	   if (minimum != DBL_MAX)
	    { offset = minimum - min ; for(i=0;i<npv->Count;i++) { npv->Pay[i] += offset ; } ; } ;
	 } ;


/*	Create resulting list with dimension & pointtype of original list */
	if (resdim == UNUSED) { resdim = Dim_Num ; respnttype = V4DPI_PntType_Real ; } ;
	ZPH(&ptbuf) ; ptbuf.Dim = resdim ; ptbuf.PntType = respnttype ; ptbuf.Bytes = ctx->pi->PointBytes[respnttype] ;
	INITLP(respnt,lp,Dim_List) ;
	switch (respnttype)
	 { default:
		for(i=0;i<npv->Count;i++)
		 { PUTREAL(&ptbuf,npv->Pay[i]) ;
		   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&ptbuf,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;
		 } ; break ;
	   CASEofINT
		for(i=0;i<npv->Count;i++)
		 { ptbuf.Value.IntVal =  DtoI(npv->Pay[i]) ;
		   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&ptbuf,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;
		 } ; break ;
	 } ;
	ENDLP(respnt,lp)
	return(respnt) ;

fail:	REGISTER_ERROR(0) ; return(NULL) ;
}

/*	v4stat_Skew - Returns skewness of a distribution	*/

double v4stat_Skew(count,nums,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;
{ double p1,p2,sd,avg,t ; int i ;

	if (count < 4) { v_Msg(NULL,NULL,"StatMinPoints",3) ; *ok = FALSE ; return(0.0) ; } ;
	p1 = (double)count / (double)((count-1)*(count-2)) ;
	sd = v4stat_StdDev(count,nums,ok) ; if (!ok) return(0.0) ;
	if (sd == 0.0) { v_Msg(NULL,NULL,"StatSDZero",V4IM_OpCode_StatSkew) ; *ok = FALSE ; return(0.0) ; } ;
	for(i=0,avg=0.0;i<count;i++) { avg += nums[i] ; } ;
	avg = avg / count ;
	for(i=0,p2=0.0;i<count;i++) { t = (nums[i] - avg) / sd ; p2 += t*t*t ; } ;
	*ok = TRUE ; return(p1 * p2) ;
}

/*	v4stat_Slope - Slope of the linear regression line */

double v4stat_Slope(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double sumx=0.0,sumy=0.0,sumxy=0.0,sumx2=0.0,top,bot ;
  int i ;
	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	for(i=0;i<countx;i++)
	 { sumxy += numxs[i]*numys[i] ;
	   sumx += numxs[i] ; sumy += numys[i] ;
	   sumx2 += numxs[i]*numxs[i] ;
	 } ;
	top = countx*sumxy - sumx*sumy ;
	bot = countx*sumx2 - sumx*sumx ;
	if (bot == 0.0) { v_Msg(NULL,NULL,"StatNoResult",V4IM_OpCode_StatSlope) ; *ok = FALSE ; return(0.0) ; } ;
	*ok = TRUE ; return(top / bot) ;
}

/*	v4stat_Standardize - Returns the normalized value from distribution	*/

double v4stat_Standardize(x,mean,stddev)
  double x ;		/* Value to be normalized */
  double mean ;		/* Mean of distribution */
  double stddev ;	/* Std Dev of distribution */
{
	return((x-mean)/stddev) ;
}


/*	v4stat_StdDev - Standard Deviation	*/

double v4stat_StdDev(count,nums,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;
{ int i ; double sum=0.0,sum2=0.0 ;

	if (count <= 1) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatStdDev) ; return(0.0) ; } ;
	for(i=0;i<count;i++)
	 { sum += nums[i] ;
	   sum2 += nums[i]*nums[i] ;
	 } ;
	*ok = TRUE ; return(sqrt( (count*sum2 - sum*sum) / (count*(count-1.0)) )) ;
}

/*	v4stat_StdDevP - Standard Deviation (given entire population)	*/

double v4stat_StdDevP(count,nums,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;
{ int i ; double sum=0.0,sum2=0.0 ;

	if (count <= 1) { *ok = FALSE ; v_Msg(NULL,NULL,"StatEmpty",V4IM_OpCode_StatStdDevP) ; return(0.0) ; } ;
	for(i=0;i<count;i++)
	 { sum += nums[i] ;
	   sum2 += nums[i]*nums[i] ;
	 } ;
	*ok = TRUE ; return(sqrt( (count*sum2 - sum*sum) / (count*count) )) ;
}

/*	v4stat_StdErrYX - Standard error of the regression	*/

double v4stat_StdErrYX(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double sumx=0.0,sumy=0.0,sumxy=0.0,sumx2=0.0,sumy2=0.0,top,bot ;
  int i ;

	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	for(i=0;i<countx;i++)
	 { sumxy += numxs[i]*numys[i] ;
	   sumx += numxs[i] ; sumy += numys[i] ;
	   sumx2 += numxs[i]*numxs[i] ; sumy2 += numys[i]*numys[i] ;
	 } ;
	top = (countx*sumxy - sumx*sumy)*(countx*sumxy - sumx*sumy) ;
	bot = countx*sumx2 - sumx*sumx ;
	*ok = TRUE ; return(sqrt( (1.0/(countx*(countx-2.0))) * ( countx*sumy2 - sumy*sumy - top/bot ) )) ;
}

/*	v4stat_SumSq - Sum of Squares		*/

double v4stat_SumSq(count,nums,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;
{ double sumx2=0.0 ; int i ;

	for(i=0;i<count;i++) { sumx2 += nums[i]*nums[i] ; } ;
	*ok = TRUE ; return(sumx2) ;
}

/*	v4stat_SumX2mY2 - Sum of difference of squares	*/

double v4stat_SumX2mY2(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double sumxmy2=0.0 ;
  int i ;

	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	for(i=0;i<countx;i++)
	 { sumxmy2 += numxs[i]*numxs[i] - numys[i]*numys[i] ;
	 } ;
	*ok = TRUE ; return(sumxmy2) ;
}

/*	v4stat_SumX2pY2 - Sum of sum of squares	*/

double v4stat_SumX2pY2(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double sumxpy2=0.0 ;
  int i ;

	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	for(i=0;i<countx;i++)
	 { sumxpy2 += numxs[i]*numxs[i] + numys[i]*numys[i] ;
	 } ;
	*ok = TRUE ; return(sumxpy2) ;
}


/*	v4stat_SumXmY2 - Sum of differences squared	*/

double v4stat_SumXmY2(countx,numxs,county,numys,ok)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  LOGICAL *ok ;
{ double sumxmy2=0.0 ;
  int i ;

	if (countx != county)
	 { *ok = FALSE ; v_Msg(NULL,NULL,"StatXYNoMatch",countx,county) ; return(1.0) ; } ;
	for(i=0;i<countx;i++)
	 { sumxmy2 += (numxs[i] - numys[i])*(numxs[i] - numys[i]) ;
	 } ;
	*ok = TRUE ; return(sumxmy2) ;
}

/*	v4stat_TDist - Student's T-distribution		*/

double v4stat_TDist(x,dof,tails)
  double x ;			/* Value at which to evaluate */
  double dof ;			/* Degrees of freedom */
  int tails ;			/* 1/2 tails */
{ double p1 ;

	p1 = betai(dof/2.0,0.5,dof/(dof+x*x)) ;
	return(p1) ;
}

double v4stat_TInv(prob,dof,ok)
  double prob ;			/* Probability to hunt for */
  double dof ;			/* Degrees of Freedom */
  LOGICAL *ok ;
{ double up,low,tx, fx ;
  int max ;
  struct V4C__Context *ctx ;		/* Current context */

	low = 0.0 ; up = 1000000.0 ;
	for(max=0;max<MAXTRIES;max++)
	 { 
	   tx = low + (up - low)/2 ;
	   fx = v4stat_TDist(tx,dof,TRUE) ;
	   if (up-low < 0.0000001) return(tx) ;
	   if (fabs(fx-prob) < 0.000001) return(tx) ;	/* Quit already */
	   if (fx < prob)
	    { up = tx ; }
	    else { low = tx ; } ;
	 } ;
	ctx = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ctx ; v_Msg(ctx,ctx->ErrorMsgAux,"NoSolution") ;
	*ok = FALSE ; return(1.0) ;
}


/*	v4stat_Trend - Fits a straight line		*/

int v4stat_Trend()
{
	return(0) ;
}

/*	v4stat_TrimMean - Returns Mean of a list excluding top/bottom numbers	*/

double v4stat_TrimMean(count,nums,percent,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;	/* The list of numbers */
  double percent ;	/* The percent (0..1) to trim equally from top & bottom */
{ int cnt,i ; double sum=0.0 ;

	if (percent < 0.0 || percent >= 1.0) { v_Msg(NULL,NULL,"StatPercent",percent) ; *ok = FALSE ; return(1.0) ; } ;
	qsort(nums,count,sizeof(double),v4stat_DoubleCompare) ;
	cnt = DtoI(count * percent) ; cnt = cnt / 2 ; if (cnt < 1) cnt = 1 ;
	for(i=cnt;i<count-cnt;i++) { sum += nums[i] ; } ;
	*ok = TRUE ; return(sum/(count-cnt-cnt)) ;
}

/*	v4stat_TTest - Returns Probability Associated with Student's T Test	*/

double v4stat_TTest(countx,numxs,county,numys,tails,type)
  int countx ;
  double numxs[] ;		/* X values */
  int county ;
  double numys[] ;		/* Y values */
  int tails ;			/* Number of tails to use (1/2) */
  int type ;			/* Type of test (1=paired, 2=homoscedastic, 3=heteroscedastic) */
{ double t,prob ;

	switch (type)
	 { case 1:	tptest(numxs,numys,county,&t,&prob) ; break ;
	   case 2:	ttest(numxs,countx,numys,county,&t,&prob) ; break ;
	   case 3:	tutest(numxs,countx,numys,county,&t,&prob) ; break ;
	 } ;
	return(prob) ;
}

/*	v4stat_Var - Estimates variance of list of numbers	*/

double v4stat_Var(count,nums,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;
{ int i ; double sum=0.0,sum2=0.0 ;

	for(i=0;i<count;i++)
	 { sum += nums[i] ;
	   sum2 += nums[i]*nums[i] ;
	 } ;
	if (count < 3) { v_Msg(NULL,NULL,"StatMinPoints",2) ; *ok = FALSE ; return(0.0) ; } ;
	*ok = TRUE ; return((count*sum2 - sum*sum) / (count*(count-1.0))) ;
}

/*	v4stat_VarP - Estimates variance of list of numbers, given entire population	*/

double v4stat_VarP(count,nums,ok)
  int count ; LOGICAL *ok ;
  double nums[] ;
{ int i ; double sum=0.0,sum2=0.0 ;

	if (count < 2) { v_Msg(NULL,NULL,"StatMinPoints",1) ; *ok = FALSE ; return(0.0) ; } ;
	for(i=0;i<count;i++)
	 { sum += nums[i] ;
	   sum2 += nums[i]*nums[i] ;
	 } ;
	*ok = TRUE ; return((count*sum2 - sum*sum) / (count*count)) ;
}

/*	v4stat_Weibull - Returns Weibull distribution		*/

double v4stat_Weibull(x,alpha,beta,cum)
  double x ;		/* Value at which to eval */
  double alpha ;	/* Parameter to the distribution */
  double beta ;		/* Another param */
  LOGICAL cum ;		/* TRUE = cumulative distribution, FALSE = prob. density function */
{
	if (cum)
	 { return(1.0 - exp(-pow(x/beta,alpha))) ;
	 } else
	 { return( (alpha/pow(beta,alpha)) * pow(x,alpha-1.0) * exp(-pow(x/beta,alpha)) ) ;
	 } ;
}

/*	v4stat_ZTest - Returns 2-tailed P-value of a z-test	*/

double v4stat_ZTest(count,nums,x,sigma,ok)
  int count ;		/* Count of nums */
  double nums[] ;
  double x ;		/* The value to test */
  double sigma ;	/* Std Dev (if < 0 then calculate from nums) */
  LOGICAL *ok ;
{ double avg ; int i ;

	for(i=0,avg=0.0;i<count;i++) { avg += nums[i] ; } ; avg /= (double)count ;
	if (sigma < 0) sigma = v4stat_StdDev(count,nums,ok) ; if (!*ok) return(0.0) ;
	if (sigma == 0) { *ok = FALSE ; v_Msg(NULL,NULL,"StatSDZero",V4IM_OpCode_StatZTest) ; return(0.0) ; } ;
	return(1.0 - v4stat_NormStdDist((avg - x)/(sigma/sqrt((double)count)))) ;
}

P *v4stat_Arg1List(ctx,respnt,proc,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;		/* Current context */
  P *respnt ;				/* Ptr to result buffer point */
  double proc(int,double[],LOGICAL *) ;			/* Handler C module */
  int intmodx ;				/* IntMod index */
  P *argpnts[] ;			/* Argument to the intmod */
  int argcnt ;				/* Number of arguments */
  int trace ;				/* Trace flag */
{ int i,j,count,lx,frameid,ok,distribute ;
  struct V4L__ListPoint *lp ;
  DIMID resDim ; struct V4DPI__DimInfo *di ;
  struct V4IM_DblArray *npv,*dpv ;
  P *ipt,*cpt,*lpt,*ept,*vpt ; P pnt ; double result ;

	lpt = NULL ; ept = NULL ; vpt = NULL ; npv = v_InitDblArray(0) ; distribute = FALSE ; resDim = UNUSED ;
	for(i=1;i<=argcnt;i++)
	 { ipt = argpnts[i] ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Dim:	resDim = cpt->Value.IntVal ; DIMINFO(di,ctx,resDim) ; break ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case -V4IM_Tag_Distribution: distribute = TRUE ; break ;
	      case V4IM_Tag_Enum:	ept = cpt ; lx = i ; break ;
	      case V4IM_Tag_List:	lpt = cpt ; lx = i ; break ;
	      case V4IM_Tag_Values:	vpt = cpt ; break ;
	    } ;
	 } ;
	if (lpt != NULL)			/* Have list - no tags */
	 { if (ept != NULL || vpt != NULL) { v_Msg(ctx,NULL,"StatListAndTag",intmodx) ; goto fail ; } ;
	   lp = v4im_VerifyList(NULL,ctx,lpt,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,lpt) ; goto fail ; } ;
	   for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&pnt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&pnt) ; goto fail ; } ;
	    } ;
	 } else					/* Have to enumerate & get list */
	 { if (vpt == NULL || ept == NULL) { v_Msg(ctx,NULL,"StatNoList",intmodx) ; goto fail ; } ;
	   lp = v4im_VerifyList(NULL,ctx,ept,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ept) ; goto fail ; } ;
	   frameid = v4ctx_FramePush(ctx,NULL) ;		/* Start new context frame */
	   for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
	      if (!v4ctx_FrameAddDim(ctx,0,&pnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
//	      v4ctx_FrameAddDim(ctx,0,&pnt,0,0) ;
	      CLEARCACHE
	      ipt = v4dpi_IsctEval(&pnt,vpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval2",intmodx,vpt) ; goto fail ; } ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&pnt) ; goto fail ; } ;
	    } ;
	   if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; goto fail ; } ;
	 } ;
	if (distribute)
	 { for(i=0,count=0;i<npv->Count;i++)			/* Go through distribute & determine actual number of sample points */
	    { if (npv->Pay[i] < 0) { v_Msg(ctx,NULL,"StatNegDist",intmodx,i+1,npv->Pay[i]) ; goto fail ; } ;
	      count += DtoI(npv->Pay[i]) ;
	    } ;
	   dpv = v_InitDblArray(count) ;
/*	   Now go thru and convert distribution to sample points */
	   for(i=0;i<npv->Count;i++)
	    { for(j=1;j<=nint(npv->Pay[i]);j++) { dpv->Pay[dpv->Count++] = i ; } ;
	    } ;
	   v4mm_FreeChunk(npv) ;				/* All done with npv */
	   npv = dpv ;						/* Make this the list */
	 } ;
//	ipt = respnt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Foreign ; ok = TRUE ;
	ipt = respnt ; ok = TRUE ;
//	v4im_SetPointValue(ctx,ipt,proc(npv->Count,npv->Pay,&ok)) ;		/* Call required procedure & force into double */
//	dblPNTi(ipt,proc(npv->Count,npv->Pay,&ok)) ;				/* Call required procedure & force into double */
	ZUS(ctx->ErrorMsg) ;
	result = proc(npv->Count,npv->Pay,&ok) ;
	if (!ok)
	 { if (UCempty(ctx->ErrorMsg)) v_Msg(ctx,NULL,"ModFailed2",intmodx) ;
	   goto fail ;
	 } ;
	v4mm_FreeChunk(npv) ;
	if (resDim == UNUSED)			/* If no Dim specified return as double */
	 { dblPNTi(ipt,result) ; return(ipt) ; } ;
	switch (di->PointType)
	 { default:
	   CASEofINT		intPNTv(respnt,(int)result) ; break ;
	   CASEofDBL		dblPNTv(respnt,result) ; break ;
	 } ;
	respnt->Dim = di->DimId ; respnt->PntType = di->PointType ;
	return(ipt) ;
fail:
	v4mm_FreeChunk(npv) ;
	REGISTER_ERROR(0) ; return(NULL) ;
}


P *v4stat_Arg2List(ctx,respnt,proc,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;		/* Current context */
  P *respnt ;				/* Ptr to results buffer */
  double proc(int,double[],int,double [],LOGICAL *) ;	/* Handler C module */
  int intmodx ;				/* IntMod index */
  P *argpnts[] ;			/* Argument to the intmod */
  int argcnt ;				/* Number of arguments */
  int trace ;				/* Trace flag */
{ int i,lx,lx2,frameid,ok ;
  struct V4L__ListPoint *lp ;
  struct V4IM_DblArray *npv,*npv2 ;
  P *ipt,*cpt,*lpt,*lpt2,*ept,*xpt,*ypt ; P pnt ;

	lpt = NULL ; lpt2 = NULL ; ept = NULL ; xpt = NULL ; ypt = NULL ; npv = NULL ; npv2 = NULL ;
	for(i=1;i<=argcnt;i++)
	 { ipt = argpnts[i] ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Enum:	ept = cpt ; lx = i ; break ;
	      case V4IM_Tag_List:
		if (lpt == NULL) { lpt = cpt ; lx = i ; } else { lpt2 = cpt ; lx2 = i ; } ;
		break ;
	      case V4IM_Tag_X:		xpt = cpt ; break ;
	      case V4IM_Tag_Y:		ypt = cpt ; break ;
	    } ;
	 } ;
	if (lpt != NULL)			/* Have list - no tags */
	 { if (ept != NULL || xpt != NULL || ypt != NULL)
	    { v_Msg(ctx,NULL,"StatListAndTag",intmodx) ; goto fail ; } ;
	   if (lpt2 == NULL) { v_Msg(ctx,NULL,"ListNumWrong",intmodx,2) ; goto fail ; } ;
	   lp = v4im_VerifyList(NULL,ctx,lpt,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; goto fail ; } ;
	   npv = v_InitDblArray(0) ; npv2 = v_InitDblArray(0) ;
	   for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&pnt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&pnt) ; goto fail ; } ;
	    } ;
	   lp = v4im_VerifyList(NULL,ctx,lpt2,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,lpt2) ; goto fail ; } ;
	   for(npv2->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv2->Count >= npv2->Max) npv2 = v_EnlargeDblArray(npv2,0) ;
	      npv2->Pay[npv2->Count++] = v4im_GetPointDbl(&ok,&pnt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&pnt) ; goto fail ; } ;
	    } ;
	   if (npv->Count != npv2->Count) { v_Msg(ctx,NULL,"StatXYNoMatch",intmodx,npv->Count,npv2->Count) ;  goto fail ; } ;
	 } else					/* Have to enumerate & get list */
	 { if (xpt == NULL || ypt == NULL || ept == NULL)
	    { v_Msg(ctx,NULL,"StatNoList",intmodx) ; goto fail ; } ;
	   lp = v4im_VerifyList(NULL,ctx,ept,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ept) ; goto fail ; } ;
	   frameid = v4ctx_FramePush(ctx,NULL) ;		/* Start new context frame */
	   npv = v_InitDblArray(0) ; npv2 = v_InitDblArray(0) ;
	   for(npv->Count=0,npv2->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) { npv = v_EnlargeDblArray(npv,0) ; npv2 = v_EnlargeDblArray(npv2,0) ; } ;
	      if (!v4ctx_FrameAddDim(ctx,0,&pnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
//	      v4ctx_FrameAddDim(ctx,0,&pnt,0,0) ;
	      CLEARCACHE
	      ipt = v4dpi_IsctEval(&pnt,xpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { v_Msg(ctx,NULL,"StatXYFail",V4IM_Tag_X,intmodx,i,xpt) ; goto fail ; } ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto fail ; } ;
	      ipt = v4dpi_IsctEval(&pnt,ypt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { v_Msg(ctx,NULL,"StatXYFail",V4IM_Tag_X,intmodx,i,ypt) ; goto fail ; } ;
	      npv2->Pay[npv2->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto fail ; } ;
	    } ;
	   if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; goto fail ; } ;
	 } ;
//	ipt = respnt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Foreign ; ok = TRUE ;
	ipt = respnt ; ok = TRUE ;
//	v4im_SetPointValue(ctx,ipt,proc(npv->Count,npv->Pay,npv2->Count,npv2->Pay,&ok)) ;	/* Call required procedure & force into double */
	ZUS(ctx->ErrorMsg) ;
	dblPNTi(ipt,proc(npv->Count,npv->Pay,npv2->Count,npv2->Pay,&ok)) ;			/* Call required procedure & force into double */
	if (!ok)
	 { if (UCempty(ctx->ErrorMsg)) v_Msg(ctx,NULL,"ModFailed2",intmodx) ;
	   goto fail ;
	 } ;
	v4mm_FreeChunk(npv) ; v4mm_FreeChunk(npv2) ;
	return(ipt) ;
fail:
	if (npv != NULL) v4mm_FreeChunk(npv) ; if (npv2 != NULL) v4mm_FreeChunk(npv2) ;
	REGISTER_ERROR(0) ; return(NULL) ;
}

P *v4stat_Arg3List(ctx,respnt,proc,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;		/* Current context */
  P *respnt ;				/* Ptr to results buffer */
  double proc(int,double[],int,double [],int,double [],LOGICAL *) ;	/* Handler C module */
  int intmodx ;				/* IntMod index */
  P *argpnts[] ;			/* Argument to the intmod */
  int argcnt ;				/* Number of arguments */
  int trace ;				/* Trace flag */
{ int i,lx,lx2,lx3,frameid,ok ;
  struct V4L__ListPoint *lp ;
  struct V4IM_DblArray *npv,*npv2,*npv3 ;
  P *ipt,*cpt,*lpt,*lpt2,*lpt3=NULL,*ept,*xpt,*ypt, *zpt ; P pnt ;

	lpt = NULL ; lpt2 = NULL ; ept = NULL ; xpt = NULL ; ypt = NULL ; zpt = NULL ; npv = NULL ; npv2 = NULL ; npv3 = NULL ;
	for(i=1;i<=argcnt;i++)
	 { ipt = argpnts[i] ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Enum:	ept = cpt ; lx = i ; break ;
	      case V4IM_Tag_List:
		if (lpt == NULL) { lpt = cpt ; lx = i ; }
		 else if (lpt2 == NULL) { lpt2 = cpt ; lx2 = i ; }
		 else { lpt3 = cpt ; lx3 = i ; } ;
		break ;
	      case V4IM_Tag_X:		xpt = cpt ; break ;
	      case V4IM_Tag_Y:		ypt = cpt ; break ;
	      case V4IM_Tag_Z:		zpt = cpt ; break ;
	    } ;
	 } ;
	if (lpt != NULL)			/* Have list - no tags */
	 { if (ept != NULL || xpt != NULL || ypt != NULL || zpt != NULL)
	    { v_Msg(ctx,NULL,"StatListAndTag",intmodx) ; goto fail ; } ;
	   if (lpt2 == NULL) { v_Msg(ctx,NULL,"ListNumWrong",intmodx,2) ; goto fail ; } ;
	   lp = v4im_VerifyList(NULL,ctx,lpt,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; goto fail ; } ;
	   npv = v_InitDblArray(0) ; npv2 = v_InitDblArray(0) ; npv3 = v_InitDblArray(0) ;
	   for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&pnt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&pnt) ; goto fail ; } ;
	    } ;
	   lp = v4im_VerifyList(NULL,ctx,lpt2,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,lpt2) ; goto fail ; } ;
	   for(npv2->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv2->Count >= npv2->Max) npv2 = v_EnlargeDblArray(npv2,0) ;
	      npv2->Pay[npv2->Count++] = v4im_GetPointDbl(&ok,&pnt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&pnt) ; goto fail ; } ;
	    } ;
	   if (npv->Count != npv2->Count) { v_Msg(ctx,NULL,"StatXYNoMatch",intmodx,npv->Count,npv2->Count) ;  goto fail ; } ;
	   lp = (lpt3 == NULL ? NULL : v4im_VerifyList(NULL,ctx,lpt3,intmodx)) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,lpt3) ; goto fail ; } ;
	   for(npv3->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv3->Count >= npv3->Max) npv3 = v_EnlargeDblArray(npv3,0) ;
	      npv3->Pay[npv3->Count++] = v4im_GetPointDbl(&ok,&pnt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&pnt) ; goto fail ; } ;
	    } ;
	   if (npv->Count != npv3->Count) { v_Msg(ctx,NULL,"StatXYNoMatch",intmodx,npv->Count,npv3->Count) ;  goto fail ; } ;
	 } else					/* Have to enumerate & get list */
	 { if (xpt == NULL || ypt == NULL || zpt == NULL || ept == NULL)
	    { v_Msg(ctx,NULL,"StatNoList",intmodx) ; goto fail ; } ;
	   lp = v4im_VerifyList(NULL,ctx,ept,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ept) ; goto fail ; } ;
	   frameid = v4ctx_FramePush(ctx,NULL) ;		/* Start new context frame */
	   npv = v_InitDblArray(0) ; npv2 = v_InitDblArray(0) ; npv3 = v_InitDblArray(0) ;
	   for(npv->Count=0,npv2->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&pnt) > 0;i++)
	    { if (npv->Count >= npv->Max) { npv = v_EnlargeDblArray(npv,0) ; npv2 = v_EnlargeDblArray(npv2,0) ; npv3 = v_EnlargeDblArray(npv3,0) ; } ;
	      if (!v4ctx_FrameAddDim(ctx,0,&pnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
//	      v4ctx_FrameAddDim(ctx,0,&pnt,0,0) ;
	      CLEARCACHE
	      ipt = v4dpi_IsctEval(&pnt,xpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { v_Msg(ctx,NULL,"StatXYFail",V4IM_Tag_X,intmodx,i,xpt) ; goto fail ; } ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto fail ; } ;
	      ipt = v4dpi_IsctEval(&pnt,ypt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { v_Msg(ctx,NULL,"StatXYFail",V4IM_Tag_Y,intmodx,i,ypt) ; goto fail ; } ;
	      npv2->Pay[npv2->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto fail ; } ;
	      ipt = v4dpi_IsctEval(&pnt,zpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { v_Msg(ctx,NULL,"StatXYFail",V4IM_Tag_Z,intmodx,i,zpt) ; goto fail ; } ;
	      npv3->Pay[npv3->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto fail ; } ;
	    } ;
	   if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; goto fail ; } ;
	 } ;
//	ipt = respnt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Foreign ; ok = TRUE ;
	ipt = respnt ; ok = TRUE ;
//	v4im_SetPointValue(ctx,ipt,proc(npv->Count,npv->Pay,npv2->Count,npv2->Pay,&ok)) ;	/* Call required procedure & force into double */
	ZUS(ctx->ErrorMsg) ;
	dblPNTi(ipt,proc(npv->Count,npv->Pay,npv2->Count,npv2->Pay,npv3->Count,npv3->Pay,&ok)) ;	/* Call required procedure & force into double */
	if (!ok)
	 { if (UCempty(ctx->ErrorMsg)) v_Msg(ctx,NULL,"ModFailed2",intmodx) ;
	   goto fail ;
	 } ;
	v4mm_FreeChunk(npv) ; v4mm_FreeChunk(npv2) ; v4mm_FreeChunk(npv3) ;
	return(ipt) ;
fail:
	if (npv != NULL) v4mm_FreeChunk(npv) ; if (npv2 != NULL) v4mm_FreeChunk(npv2) ; if (npv3 != NULL) v4mm_FreeChunk(npv3) ;
	REGISTER_ERROR(0) ; return(NULL) ;
}

/*	v4stat_Dist - Top Level Handler for StatDist() module */
P *v4stat_Dist(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;
  int intmodx ;
  P *respnt,*argpnts[] ;
  int argcnt,trace ;
{ P *cpt,*ipt,pnt ;
  int i,j,ok ; int res=0,tres,Cumulative=TRUE ; double dres ;
  double A,B,Alpha,Beta,DoF2,Lambda,Mean,Num,NumF,NumP,NumS,ProbS,Samples,StdDev,Trials,X,Z ;
  double DoF ; int gotAlpha, gotBeta, gotX, gotLambda, gotMean, gotZ ;
  	
	A = 0 ; B = 1 ; DoF = -1.0 ; DoF2 = -1.0 ;		/* Set up defaults */
	Samples = -1.0 ; Trials = -1.0 ; StdDev = -1.0 ; Num = -1 ; NumS = -1 ; NumP = -1 ; NumF = -1 ;
	gotAlpha = FALSE ; gotBeta = FALSE ; gotX = FALSE ; gotLambda = FALSE ; gotMean = FALSE ; gotZ = FALSE ;
	for(i=1,ok=TRUE;ok&&i<=argcnt;i++)
	 { tres = 0 ;
	   switch (j=v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case -V4IM_Tag_BetaPDF:		tres = j ; break ;
	      case -V4IM_Tag_Binomial:		tres = j ; break ;
	      case -V4IM_Tag_ChiSq:		tres = j ; break ;
	      case -V4IM_Tag_Exponential:	tres = j ; break ;
	      case -V4IM_Tag_F:			tres = j ; break ;
	      case -V4IM_Tag_Gamma:		tres = j ; break ;
	      case -V4IM_Tag_HyperGeo:		tres = j ; break ;
	      case -V4IM_Tag_LogNorm:		tres = j ; break ;
	      case -V4IM_Tag_NegBinom:		tres = j ; break ;
	      case -V4IM_Tag_Normal:		tres = j ; break ;
	      case -V4IM_Tag_NormalStd:		tres = j ; break ;
	      case -V4IM_Tag_Poisson:		tres = j ; break ;
	      case -V4IM_Tag_StudentT:		tres = j ; break ;
	      case -V4IM_Tag_Weibull:		tres = j ; break ;
	      case V4IM_Tag_A:			A = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_B:			B = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Alpha:		gotAlpha = TRUE ; Alpha = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Beta:		gotBeta = TRUE ; Beta = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Cumulative:		Cumulative = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_DoF:		*(DoF == -1 ? &DoF : &DoF2) = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Lambda:		gotLambda = TRUE ; Lambda = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Mean:		gotMean = TRUE ; Mean = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Num:		Num = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_NumF:		NumF = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_NumP:		NumP = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_NumS:		NumS = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_ProbS:		ProbS = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Samples:		Samples = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_StdDev:		StdDev = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Trials:		Trials = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_X:			gotX = TRUE ; X = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Z:			gotZ = TRUE ; Z = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	    } ;
	   if (tres != 0)
	    { if (res != 0) { v_Msg(ctx,NULL,"TagOnlyOneRes",intmodx) ; goto fail ; } ;
	      res = tres ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto fail ; } ;
	switch(res)
	 { default:			v_Msg(ctx,NULL,"TagNoRes",intmodx) ; goto fail ;
	   case -V4IM_Tag_BetaPDF:	if (X < A || X > B) { v_Msg(ctx,NULL,"StatABXVals",intmodx) ;  goto fail ; } ;
					if (!gotX || !gotAlpha || !gotBeta) goto missing_args ;
					dres = v4stat_BetaDist(X,Alpha,Beta,A,B,TRUE) ; break ;
	   case -V4IM_Tag_Binomial:	if (NumS < 0 || Trials < 0 || ProbS < 0) goto missing_args ;
					dres = v4stat_BinomDist(NumS,Trials,ProbS,Cumulative,&ok) ; break ;
	   case -V4IM_Tag_ChiSq:	if (!gotX || DoF < 0) goto missing_args ;
					dres = v4stat_ChiDist(X,DoF) ; break ;
	   case -V4IM_Tag_Exponential:	if (!gotX || !gotLambda) goto missing_args ;
					dres = v4stat_ExponDist(X,Lambda,Cumulative) ; break ;
	   case -V4IM_Tag_F:		if (!gotX || DoF < 0 || DoF2 < 0) goto missing_args ;
					dres = v4stat_FDist(X,DoF,DoF2) ; break ;
	   case -V4IM_Tag_Gamma:	if (!gotX || !gotAlpha || !gotBeta) goto missing_args ;
					dres = v4stat_GammaDist(X,Alpha,Beta,Cumulative) ; break ;
	   case -V4IM_Tag_HyperGeo:	if (Samples < 0.0 || Trials < 0 || NumS < 0 || NumP < 0) goto missing_args ;
					dres = v4stat_HyperGeo(Samples,Trials,NumS,NumP,&ok) ; break ;
	   case -V4IM_Tag_LogNorm:	if (!gotX || !gotMean || StdDev < 0) goto missing_args ;
					dres = v4stat_LogNormDist(X,Mean,StdDev) ; break ;
	   case -V4IM_Tag_NegBinom:	if (NumF < 0 || NumS < 0 || ProbS < 0) goto missing_args ;
					dres = v4stat_NegBinomDist(NumF,NumS,ProbS,&ok) ; break ;
	   case -V4IM_Tag_Normal:	if (!gotX || !gotMean || StdDev < 0) goto missing_args ;
					dres = v4stat_NormDist(X,Mean,StdDev,Cumulative) ; break ;
	   case -V4IM_Tag_NormalStd:	if (!gotZ) goto missing_args ;
					dres = v4stat_NormStdDist(Z) ; break ;
	   case -V4IM_Tag_Poisson:	if (!gotX || !gotMean) goto missing_args ;
					dres = v4stat_Poisson(X,Mean,Cumulative) ; break ;
	   case -V4IM_Tag_StudentT:	if (!gotX || !gotMean || DoF < 0) goto missing_args ;
					dres = v4stat_TDist(X,DoF,FALSE) ; break ;
	   case -V4IM_Tag_Weibull:	if (!gotX || !gotAlpha || !gotBeta) goto missing_args ;
					dres = v4stat_Weibull(X,Alpha,Beta,Cumulative) ; break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto fail ; } ;
//	ipt = respnt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Foreign ;
	ipt = respnt ; dblPNTv(ipt,dres) ;
	return(ipt) ;
missing_args:
	v_Msg(ctx,NULL,"ModArgMand",intmodx) ; goto fail ;

fail:	REGISTER_ERROR(0) ; return(NULL) ;
}


/*	v4stat_DistInv - Top Level Handler for StatDistInv() module */

P *v4stat_DistInv(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;
  int intmodx ;
  P *respnt,*argpnts[] ;
  int argcnt,trace ;
{ P *cpt,*ipt,pnt ;
  int i,j,ok ; int res=0,tres ; double dres ;
  double DoF=-1, DoF2=-1 ;
  double Prob=-1, A=0.0, B=1.0, Alpha, Beta, Mean, StdDev=-1 ; int gotAlpha=FALSE, gotBeta=FALSE, gotMean=FALSE ;
	
	ok = TRUE ;
	for(i=1,ok;ok&&i<=argcnt;i++)
	 { tres = 0 ;
	   switch (j=v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case -V4IM_Tag_BetaPDF:		tres = j ; break ;
	      case -V4IM_Tag_ChiSq:		tres = j ; break ;
	      case -V4IM_Tag_F:			tres = j ; break ;
	      case -V4IM_Tag_Gamma:		tres = j ; break ;
	      case -V4IM_Tag_LogNorm:		tres = j ; break ;
	      case -V4IM_Tag_Normal:		tres = j ; break ;
	      case -V4IM_Tag_NormalStd:		tres = j ; break ;
	      case -V4IM_Tag_StudentT:		tres = j ; break ;
	      case -V4IM_Tag_Poisson:		tres = j ; break ;
	      case V4IM_Tag_A:			A = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_B:			B = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Alpha:		gotAlpha = TRUE ; Alpha = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Beta:		gotBeta = TRUE ; Beta = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_DoF:		*(DoF == -1 ? &DoF : &DoF2) = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Mean:		gotMean = TRUE ; Mean = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Prob:		Prob = v4im_GetPointDbl(&ok,cpt,ctx) ;
						if (Prob <= 0.0 || Prob >= 1.0)
						 { v_Msg(ctx,NULL,"StatProbSpec",intmodx,V4IM_Tag_Prob,Prob) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
						break ;
	      case V4IM_Tag_StdDev:		StdDev = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	    } ;
	   if (tres != 0)
	    { if (res != 0) { v_Msg(ctx,NULL,"TagOnlyOneRes",intmodx) ;  REGISTER_ERROR(0) ; return(NULL) ; } ;
	      res = tres ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	switch(res)
	 { default:			v_Msg(ctx,NULL,"TagNoRes",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	   case -V4IM_Tag_BetaPDF:	if (Prob < 0 || !gotAlpha || !gotBeta) goto missing_args ;
					dres = v4stat_BetaInv(Prob,Alpha,Beta,A,B,&ok) ; break ;
	   case -V4IM_Tag_ChiSq:	if (DoF < 0 || DoF2 < 0 || Prob < 0) goto missing_args ;
					dres = v4stat_ChiInv(Prob,DoF,&ok) ; break ;
	   case -V4IM_Tag_F:		if (DoF < 0 || DoF2 < 0 || Prob < 0) goto missing_args ;
					dres = v4stat_FInv(Prob,DoF,DoF2,&ok) ; break ;
	   case -V4IM_Tag_Gamma:	if (Prob < 0 || !gotAlpha || !gotBeta) goto missing_args ;
					dres = v4stat_GammaInv(Prob,Alpha,Beta,&ok) ; break ;
	   case -V4IM_Tag_LogNorm:	if (Prob < 0 || !gotMean || StdDev < 0) goto missing_args ;
					dres = v4stat_LogInv(Prob,Mean,StdDev,&ok) ; break ;
	   case -V4IM_Tag_Normal:	if (Prob < 0 || !gotMean || StdDev < 0) goto missing_args ;
					dres = v4stat_NormInv(Prob,Mean,StdDev,&ok) ; break ;
	   case -V4IM_Tag_NormalStd:	if (Prob < 0) goto missing_args ;
					dres = v4stat_NormStdInv(Prob,&ok) ; break ;
	   case -V4IM_Tag_StudentT:	if (Prob < 0 || DoF < 0) goto missing_args ;
					dres = v4stat_TInv(Prob,DoF,&ok) ; break ;
	   case -V4IM_Tag_Poisson:	if (Prob < 0 || !gotMean) goto missing_args ;
					dres = v4stat_PoissonInv(Prob,Mean) ; break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	ipt = respnt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Foreign ;
	ipt = respnt ; dblPNTv(ipt,dres) ;
	return(ipt) ;
missing_args:
	v_Msg(ctx,NULL,"ModArgMand",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
}


P *v4stat_Fit(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;		/* Current context */
  int intmodx ;				/* IntMod index */
  P *respnt,*argpnts[] ;			/* Argument to the intmod */
  int argcnt ;				/* Number of arguments */
  int trace ;				/* Trace flag */
{
  struct V4IM__StatLinFit *slf ;
  struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp,*lp1 ;
  struct V4IM_DblArray *npv ;		/* Temp structure to hold cash payments for n periods */
  struct V4DPI__Binding *bindpt ;
  P *ipt,*tpt,isctbuf,spnt,atpnt ;
  int i,j,frameid,ok ; double dnum ;

	slf = (struct V4IM__StatLinFit *)v4mm_AllocChunk(sizeof *slf,TRUE) ;
	for(ok=TRUE,i=1;ok&&i<=argcnt;i++)
	 { switch (v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,NULL))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto failure ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto failure ;
	      case V4IM_Tag_AbsDev:
		ISVALIDDIM(ipt->Value.IntVal,i,"LinFit()") ;
		DIMINFO(di,ctx,ipt->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,i,ipt) ; goto failure ; } ;
		slf->DimAbsDev = ipt->Value.IntVal ; break ;
	      case V4IM_Tag_At:
		tpt = v4dpi_IsctEval(&atpnt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		if (tpt == NULL)  { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_At,ipt) ; goto failure ; } ;
		slf->PAt = tpt ;
		break ;
	      case V4IM_Tag_Bind:
		if (ipt->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,i,ipt->PntType,V4DPI_PntType_Isct) ; goto failure ; } ;
		slf->PBind = ipt ; break ;
	      case V4IM_Tag_ChiSq:
		ISVALIDDIM(ipt->Value.IntVal,i,"LinFit()") ;
		DIMINFO(di,ctx,ipt->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,i,ipt) ; goto failure ; } ;
		slf->DimChiSq = ipt->Value.IntVal ; break ;
	      case V4IM_Tag_Context:
		ISVALIDDIM(ipt->Value.IntVal,i,"LinFit()") ;
		DIMINFO(di,ctx,ipt->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,i,ipt) ; goto failure ; } ;
		slf->DimContext = ipt->Value.IntVal ; break ;
	      case V4IM_Tag_Do:		slf->PDo = ipt ; break ;
	      case V4IM_Tag_Enum:	slf->PEnum = ipt ; break ;
	      case V4IM_Tag_Intercept:
		ISVALIDDIM(ipt->Value.IntVal,i,"LinFit()") ;
		DIMINFO(di,ctx,ipt->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,i,ipt) ; goto failure ; } ;
		slf->DimIntercept = ipt->Value.IntVal ; break ;
	      case V4IM_Tag_Method:
		switch (v4im_GetDictToEnumVal(ctx,ipt))
		 { default:	v_Msg(ctx,NULL,"TagValue",intmodx,V4IM_Tag_Method,ipt) ; goto failure ;
		   case _SLLS:	slf->Method = V4IM_StatLF_MethodSLLS ; break ;
		   case _SLLAD:	slf->Method = V4IM_StatLF_MethodSLLAD ; break ;
		 } ;
//		if (ipt->Value.IntVal == v4dpi_DictEntryGet(ctx,0,"SLLS",NULL,NULL)) { slf->Method = V4IM_StatLF_MethodSLLS ; }
//		 else if (ipt->Value.IntVal == v4dpi_DictEntryGet(ctx,0,"SLLAD",NULL,NULL)) { slf->Method = V4IM_StatLF_MethodSLLAD ; }
//		 else { v_Msg(ctx,NULL,"TagValue",intmodx,V4IM_Tag_Method,ipt) ; goto failure ; } ;
		break ;
	      case V4IM_Tag_Next:	slf->Next = v4im_GetPointInt(&ok,ipt,ctx) ; break ;
	      case V4IM_Tag_Slope:
		ISVALIDDIM(ipt->Value.IntVal,i,"LinFit()") ;
		DIMINFO(di,ctx,ipt->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,i,ipt) ; goto failure ; } ;
		slf->DimSlope = ipt->Value.IntVal ; break ;
	      case V4IM_Tag_StdDev:	slf->PStdDev = ipt ; break ;	
	      case V4IM_Tag_X:
		if (slf->XCount >= V4IM_StatLF_XMax) { v_Msg(ctx,NULL,"StatMaxVals",intmodx,V4IM_StatLF_XMax,V4IM_Tag_X) ; goto failure ; } ;
		slf->Xs[slf->XCount].PX = ipt ; slf->XCount++ ; break ;
	      case V4IM_Tag_Y:		slf->PY = ipt ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto failure ; } ;
	if (slf->PY == NULL) { v_Msg(ctx,NULL,"TagMissing",intmodx,V4IM_Tag_Y) ; goto failure ; } ;
	slf->Y = v_InitDblArray(0) ;
	for(j=0;j<slf->XCount;j++) { slf->Xs[j].X = v_InitDblArray(0) ; } ;
	if (slf->PEnum == NULL)				/* Got lists or iscts to eval to lists? */
	 { lp = v4im_VerifyList(NULL,ctx,slf->PY,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,slf->PY) ; goto failure ; } ;
	   npv = slf->Y ;
	   for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
	    { if (npv->Count >= npv->Max) slf->Y = (npv = v_EnlargeDblArray(npv,0)) ;
	      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&isctbuf) ; goto failure ; } ;
	    } ;
	   for(j=0;j<slf->XCount;j++)
	    { lp = v4im_VerifyList(NULL,ctx,slf->Xs[j].PX,intmodx) ;
	      if (lp == NULL)
	       { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,slf->Xs[j].PX) ; goto failure ; } ;
	      npv = slf->Xs[j].X ;
	      for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
	       { if (npv->Count >= npv->Max) slf->Xs[j].X = (npv = v_EnlargeDblArray(npv,0)) ;
	         npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ;
	         if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&isctbuf) ; goto failure ; } ;
	       } ;
	      if (npv->Count != slf->Y->Count) { v_Msg(ctx,NULL,"StatXYNoMatch",intmodx,npv->Count,slf->Y->Count) ; goto failure ; } ;
	    } ;
	 } else
	 { lp = v4im_VerifyList(NULL,ctx,slf->PEnum,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,slf->PEnum) ; goto failure ; } ;
	   frameid = v4ctx_FramePush(ctx,NULL) ;	/* Start new context frame */
	   for(i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
	    { 
	      if (isctbuf.PntType == V4DPI_PntType_Isct)		/* If an ISCT then evaluate it */
	       { ipt = v4dpi_IsctEval(&spnt,&isctbuf,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL) { v_Msg(ctx,NULL,"StrEvalIsctFail",intmodx,&isctbuf) ; goto failure ; } ;
	       } else { ipt = &isctbuf ; } ;
	      if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//	      v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;			/* Add point to current context */
	      CLEARCACHE
	      ipt = v4dpi_IsctEval(&spnt,slf->PY,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { v_Msg(ctx,NULL,"StrEvalIsctFail",intmodx,slf->PY) ; goto failure ; } ;
	      if (slf->Y->Count >= slf->Y->Max) slf->Y = v_EnlargeDblArray(slf->Y,0) ;
	      slf->Y->Pay[slf->Y->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto failure ; } ;
	      for(j=0;j<slf->XCount;j++)
	       { ipt = v4dpi_IsctEval(&spnt,slf->Xs[j].PX,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL) { v_Msg(ctx,NULL,"StrEvalIsctFail",intmodx,slf->Xs[j].PX) ; goto failure ; } ;
	         npv = slf->Xs[j].X ;
		 if (npv->Count >= npv->Max) slf->Xs[j].X = (npv = v_EnlargeDblArray(npv,0)) ;
	         npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
	         if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto failure ; } ;
	       } ;
	    } ;
	   if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; goto failure ; } ;
	 } ;
	if (slf->XCount == 0)			/* If no Xs given then supply 1..n */
	 { npv = v_InitDblArray(slf->Y->Count) ;
	   for(i=0;i<slf->Y->Count;i++) { npv->Pay[i] = (double)i ; } ;
	   npv->Count = slf->Y->Count ; slf->Xs[slf->XCount++].X = npv ;
	 } ;
	if (slf->PBind != NULL && slf->PAt == NULL)
	 { v_Msg(ctx,NULL,"TagMissing2",intmodx,V4IM_Tag_At,V4IM_Tag_Bind) ; goto failure ; } ;
	v4stat_LinFit(slf,&ok) ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto failure ; } ;
	if (slf->DimAbsDev != 0)
	 { 
//	   ZPH(&spnt) ; spnt.PntType = V4DPI_PntType_Foreign ; v4im_SetPointValue(ctx,&spnt,slf->AbsDev) ;
	   dblPNTv(&spnt,slf->AbsDev) ; spnt.Dim = slf->DimAbsDev ;
	   if (!v4ctx_FrameAddDim(ctx,0,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//	   v4ctx_FrameAddDim(ctx,0,&spnt,0,0) ;
	 } ;
	if (slf->DimSlope != 0)
	 { 
//	   ZPH(&spnt) ; spnt.PntType = V4DPI_PntType_Foreign ; v4im_SetPointValue(ctx,&spnt,slf->Xs[0].Coefficient) ;
	   dblPNTv(&spnt,slf->Xs[0].Coefficient) ; spnt.Dim = slf->DimSlope ;
	   if (!v4ctx_FrameAddDim(ctx,0,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//	   v4ctx_FrameAddDim(ctx,0,&spnt,0,0) ;
	 } ;
	if (slf->DimIntercept != 0)
	 { 
//	   ZPH(&spnt) ; spnt.PntType = V4DPI_PntType_Foreign ; v4im_SetPointValue(ctx,&spnt,slf->Intercept) ;
	   dblPNTv(&spnt,slf->Intercept) ; spnt.Dim = slf->DimIntercept ;
	   if (!v4ctx_FrameAddDim(ctx,0,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//	   v4ctx_FrameAddDim(ctx,0,&spnt,0,0) ;
	 } ;
	ipt = NULL ;				/* Set up result to return */
	if (slf->PAt == NULL) goto statlinfit_end ;
	if (slf->PDo != NULL)					/* Eval isct for each result? */
	 { if (!v4im_CouldBeList(ctx,slf->PAt)) { lp = NULL ; }
	    else { lp = v4im_VerifyList(NULL,ctx,slf->PAt,intmodx) ; } ;
	   if (slf->DimContext != 0)
	    di = (struct V4DPI__DimInfo *)v4dpi_DimInfoGet(ctx,slf->DimContext) ;
	   frameid = v4ctx_FramePush(ctx,NULL) ;	/* Start new context frame */
	   for(i=1;;i++)
	    { if (lp == NULL) { ipt = slf->PAt ; }
	       else { if (!v4l_ListPoint_Value(ctx,lp,i,&spnt)) break ; ipt = &spnt ; } ;
	      if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//	      v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;		/* Add the At:xxx point to the context */
	      CLEARCACHE
	      dnum = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto failure ; } ;
	      if (slf->DimContext)
	       { ZPH(&spnt) ; spnt.PntType = di->PointType ; spnt.Dim = slf->DimContext ;
	         spnt.Bytes = (spnt.PntType == V4DPI_PntType_Real || spnt.PntType == V4DPI_PntType_Calendar ? V4PS_Real : V4PS_Int) ;
		 v4im_SetPointValue(ctx,&spnt,slf->Xs[0].Coefficient*dnum+slf->Intercept) ;
		 if (!v4ctx_FrameAddDim(ctx,0,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//		 v4ctx_FrameAddDim(ctx,0,&spnt,0,0) ;
	       } ;
	      if (v4dpi_IsctEval(&spnt,slf->PDo,ctx,V4DPI_EM_EvalQuote,NULL,NULL) == NULL)
	       { v_Msg(ctx,NULL,"StrEvalIsctFail",intmodx,slf->PDo) ; goto failure ; } ;
	      if (lp == NULL) break ;			/* If single point then only got thru once */
	    } ;
	   if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; goto failure ; } ;
	   goto statlinfit_end ;
	 } ;
	if (slf->PBind != NULL)				/* Want to bind result(s) ? */
	 { 
	   if (!v4im_CouldBeList(ctx,slf->PAt)) { lp = NULL ; }
	    else { lp = v4im_VerifyList(NULL,ctx,slf->PAt,intmodx) ; } ;
	   bindpt = (struct V4DPI__Binding *)v4mm_AllocChunk(sizeof *bindpt,FALSE) ;
	   for(i=1;;i++)
	    { if (lp == NULL) { ipt = slf->PAt ; }
	       else { if (!v4l_ListPoint_Value(ctx,lp,i,&spnt)) break ; ipt = &spnt ; } ;
	      dnum = v4im_GetPointDbl(&ok,ipt,ctx) ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,ipt) ; goto failure ; } ;
	      memcpy(&isctbuf,slf->PBind,slf->PBind->Bytes) ;			/* Start new bind point */
	      memcpy((char *)&isctbuf + isctbuf.Bytes,ipt,ipt->Bytes) ;		/* Append At point */
	      isctbuf.Bytes += spnt.Bytes ; isctbuf.Grouping ++ ;
//	      ZPH(&spnt) ; spnt.PntType = V4DPI_PntType_Foreign ;		/* spnt = result */
//	      v4im_SetPointValue(ctx,&spnt,slf->Xs[0].Coefficient*dnum+slf->Intercept) ;
	      dblPNTi(&spnt,slf->Xs[0].Coefficient*dnum+slf->Intercept) ;
	      if (!v4dpi_BindListMake(bindpt,&spnt,&isctbuf,ctx,NULL,NOWGTADJUST,0,DFLTRELH))
	       { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto failure ; } ;
	      if (trace & V4TRACE_TallyBind)
	       { 
//	         v4dpi_PointToString(tb,&spnt,ctx,-1) ; v4dpi_PointToString(v4tbuf,&isctbuf,ctx,-1) ;
	         v_Msg(ctx,UCTBUF1,"*TraceTallyBind",&isctbuf,&spnt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	       } ;
	      if (lp == NULL) break ;			/* If single point then only got thru once */
	    } ;
	   v4mm_FreeChunk(bindpt) ;
	   goto statlinfit_end ;
	 } ;
	if (!v4im_CouldBeList(ctx,slf->PAt))			/* Wants a single point evaluated as result? */
	 { ipt = respnt ;
	   dnum = v4im_GetPointDbl(&ok,slf->PAt,ctx) ;
	   if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,slf->PAt) ; goto failure ; } ;
//	   ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Foreign ;
	   dblPNTi(ipt,slf->Xs[0].Coefficient*dnum+slf->Intercept) ;
	   goto statlinfit_end ;
	 } ;
	INITLP(respnt,lp1,Dim_List)
//	ipt = respnt ;
//	lp1 = ALIGNLP(&ipt->Value) ;
//	lp1->Bytes = 0 ; lp1->ListType = V4L_ListType_Point ; lp1->Entries = 0 ; lp1->BytesPerEntry = 0 ;
//	lp1->Dim = 0 ; lp1->PntType = 0 ; lp1->LPIOffset = 0 ; lp1->FFBufx = 0 ;
	lp = v4im_VerifyList(NULL,ctx,slf->PAt,intmodx) ;
	if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,slf->PAt) ; goto failure ; } ;
	for(i=1;v4l_ListPoint_Value(ctx,lp,i,&spnt);i++)
	 { dnum = v4im_GetPointDbl(&ok,&spnt,ctx) ;
	   if (!ok) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,&spnt) ; goto failure ; } ;
//	   ZPH(&spnt) ; spnt.PntType = V4DPI_PntType_Foreign ;		/* spnt = result */
	   dblPNTi(&spnt,slf->Xs[0].Coefficient*dnum+slf->Intercept) ;
	   if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&spnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto failure ; } ;
	 } ;
	DIMINFO(di,ctx,Dim_List) ;
//	ipt->Dim = di->DimId ; ipt->PntType = di->PointType ;
//	ipt->Bytes = ALIGN((char *)&ipt->Value.AlphaVal[lp1->Bytes] - (char *)ipt) ;
//	if (ipt->Bytes < V4DPI_PointHdr_Bytes + (&lp1->Buffer[0] - (char *)lp1))
//	 ipt->Bytes = V4DPI_PointHdr_Bytes + (&lp1->Buffer[0] - (char *)lp1) ;
	ENDLP(respnt,lp1) ipt = respnt ;
statlinfit_end:
	v4mm_FreeChunk(slf->Y) ; for(i=0;i<slf->XCount;i++) { v4mm_FreeChunk(slf->Xs[i].X) ; } ;
	v4mm_FreeChunk(slf) ;
	return(ipt) ;
failure:
	if (slf->Y != NULL) v4mm_FreeChunk(slf->Y) ;
	for(i=0;i<slf->XCount;i++) { v4mm_FreeChunk(slf->Xs[i].X) ; } ;
	v4mm_FreeChunk(slf) ;
	REGISTER_ERROR(0) ; return(NULL) ;
}

#define ONCE(param) if (param != NULL) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"ModArgOnce",intmodx) ; break ; } ;
#define ONCEU(param) if (param >= 0) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"ModArgOnce",intmodx) ; break ; } ;

/*	D A T A   M I N I N G   R O U T I N E S

/*	v4im_DoDMCluster - Clusters Points via Nearness Algorithm	*/

#define V4DM_Cluster_InitialMax 1000
#define V4DM_ClusterDtl_InitialEntryMax 1000

struct V4DM__ClusterMaster
{ int MaxEntries ;		/* Max entries below */
  int Count ;			/* Number of clusters defined below */
  struct {			/* Structure will expand as necessary */
    struct V4DM__ClusterDtl *vdcd ;
   } Clus[V4DM_Cluster_InitialMax] ;
} ;

struct V4DM__ClusterDtl
{ double Distance ;		/* Distance current point is from this cluster */
  int MaxEntries ;		/* Max entries below */
  int Count ;			/* Current number */
  struct {			/* Structure will expand as necessary */
    struct V4DPI__LittlePoint clpt ;
   } Entry[V4DM_ClusterDtl_InitialEntryMax] ;
} ;

struct V4DPI__Point *v4im_DoDMCluster(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  int intmodx ;
  P *argpnts[] ;
  int argcnt,trace ;
{ struct V4L__ListPoint *lp,*lp1 ;
  struct V4DPI__Binding binding ;
  P *cpt,*ipt,*lpt,*distpt,*wpt,*bindpt,*indexpt,spnt,shellpt,tpnt ;
  struct V4DPI__LittlePoint idxpnt ;
  struct V4DM__ClusterMaster *vdcm ;
  struct V4DM__ClusterDtl *vdcd ;
  int i,j,k,ok,ic,frameid,startindex,nearness,savej,maxcount ;
  int shelldim ; double threshold,dbl ;

	lpt = NULL ; distpt = NULL ; bindpt = NULL ; indexpt = NULL ; nearness = 3 ; maxcount = V4LIM_BiggestPositiveInt ;
	shelldim = UNUSED ; threshold = UNUSED ; wpt = NULL ;
	ipt = argpnts[1] ;
	for(ok=TRUE,i=1;ok&&i<=argcnt;i++)
	 { 
	   if (i == 1 && argpnts[i]->PntType != V4DPI_PntType_TagVal) { lpt = argpnts[i] ; continue ; } ;
	   switch (k=v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Shell:	ONCEU(shelldim) ; shelldim = v4dpi_DimShellDimId(&ok,ctx,cpt) ; break ;
	      case -V4IM_Tag_Minimum:	nearness = 1 ; break ;
	      case -V4IM_Tag_Maximum:	nearness = 2 ; break ;
	      case -V4IM_Tag_Average:	maxcount = V4LIM_BiggestPositiveInt ; nearness = 3 ; break ;
	      case V4IM_Tag_Average:	maxcount = v4im_GetPointInt(&ok,cpt,ctx) ; nearness = 3 ; break ;
	      case V4IM_Tag_While:	ONCE(wpt) ; wpt = cpt ; break ;
	      case V4IM_Tag_Distance:	ONCE(distpt) ; distpt = cpt ; break ;
	      case V4IM_Tag_Threshold:	threshold = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Bind:	ONCE(bindpt) ; bindpt = cpt ; break ;
	      case V4IM_Tag_Index:	ONCE(indexpt) ;	indexpt = v4dpi_IsctEval(&tpnt,cpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
					if (indexpt == NULL ? TRUE : indexpt->PntType != V4DPI_PntType_Int)
					 { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"@Index::point not type INTEGER") ; }
					 else { memcpy(&idxpnt,indexpt,indexpt->Bytes) ; startindex = indexpt->Value.IntVal ; } ;
					break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	if (indexpt == NULL || bindpt == NULL || distpt == NULL || threshold == UNUSED)
	 { v_Msg(ctx,NULL,"ModArgMand",intmodx) ;  REGISTER_ERROR(0) ; return(NULL) ;
	 } ;

	if (lpt != NULL)
	 { lp = v4im_VerifyList(NULL,ctx,lpt,intmodx) ;
	   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList2",intmodx,lpt) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	 } else { v_Msg(ctx,NULL,"StatNoList",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

	frameid = v4ctx_FramePush(ctx,NULL) ;
	vdcm = v4mm_AllocChunk(sizeof *vdcm,FALSE) ; vdcm->MaxEntries = V4DM_Cluster_InitialMax ; vdcm->Count = 0 ;
/*	First point goes, by definition, into first cluster */
	if (v4l_ListPoint_Value(ctx,lp,1,&spnt) <= 0)
	 { v_Msg(ctx,NULL,"DMClusterNoPt",intmodx) ; ipt = NULL ; goto cluster_finish ; } ;
	vdcm->Clus[vdcm->Count++].vdcd = (vdcd = v4mm_AllocChunk(sizeof *vdcd,FALSE)) ;
	vdcd->MaxEntries = V4DM_ClusterDtl_InitialEntryMax ; vdcd->Count = 0 ;
	if (spnt.Bytes > sizeof vdcd->Entry[0].clpt)
	 { v_Msg(ctx,NULL,"DMClusterTooBig",intmodx) ; ipt = NULL ; goto cluster_finish ; } ;
	lpt = (P *)&vdcd->Entry[vdcd->Count].clpt ; memcpy(lpt,&spnt,spnt.Bytes) ; vdcd->Count++ ;
/*	Create shell shell */
	ZPH(&shellpt) ; shellpt.Dim = shelldim ; shellpt.PntType = V4DPI_PntType_Shell ;
	for(i=2;;i++)
	 { if (ic=v4l_ListPoint_Value(ctx,lp,i,&spnt) <= 0) break ;
	   if (ic != 2)
	    {
	      if (!v4ctx_FrameAddDim(ctx,0,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//	      v4ctx_FrameAddDim(ctx,0,&spnt,0,0) ;
	      CLEARCACHE
	    } ;
/*	   Now find which point this is closest to */
	   for (j=0;j<vdcm->Count;j++)					/* Loop thru each cluster */
	    { vdcd = vdcm->Clus[j].vdcd ;
	      switch(nearness)
	       { case 1: vdcd->Distance = DBL_MAX ;
	         case 2: vdcd->Distance = -DBL_MAX ;
	         case 3: vdcd->Distance = 0.0 ;
	       } ;
	      for (k=0;k<vdcd->Count && k<maxcount;k++)			/*  and each point in current cluster */
	       { memcpy(&shellpt.Value,&vdcd->Entry[k].clpt,vdcd->Entry[k].clpt.Bytes) ;
	         shellpt.Bytes = V4DPI_PointHdr_Bytes + vdcd->Entry[k].clpt.Bytes ;
		 if (!v4ctx_FrameAddDim(ctx,0,&shellpt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
//		 v4ctx_FrameAddDim(ctx,0,&shellpt,0,0) ;
		 CLEARCACHE	/* Add shell point to context */
		 ipt = v4dpi_IsctEval(&tpnt,distpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		 if (ipt == NULL)					/* Could not calcluate distance? */
		  { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_Distance,distpt) ; goto cluster_finish ;} ;
		 dbl = v4im_GetPointDbl(&ok,ipt,ctx) ;
		 if (!ok) { v_Msg(ctx,NULL,"TagValue2",intmodx,V4IM_Tag_Distance,ipt) ; goto cluster_finish ; } ;
		 switch(nearness)
		  { case 1: if (dbl < vdcd->Distance) vdcd->Distance = dbl ;
		    case 2: if (dbl > vdcd->Distance) vdcd->Distance = dbl ;
		    case 3: vdcd->Distance += dbl ;
		  } ;
	       } ;
	    } ;
/*	   Now see which cluster current point is closest to */
	   dbl = DBL_MAX ; vdcd = NULL ;
if ((i % 5000) == 0) printf("i = %d\n",i) ;
	   for(j=0;j<vdcm->Count;j++)
	    { if (nearness == 3)
	       vdcm->Clus[j].vdcd->Distance /= vdcm->Clus[j].vdcd->Count ; /* Convert to average distance */
	      if (vdcm->Clus[j].vdcd->Distance > threshold) continue ;
	      if (vdcm->Clus[j].vdcd->Distance < dbl)			/* Got cluster that is currently closest */
	       { savej = j ; vdcd = vdcm->Clus[j].vdcd ; dbl = vdcm->Clus[j].vdcd->Distance ; } ;
	    } ;
	   if (vdcd == NULL)						/* If no cluster then have to make new one */
	    { if (vdcm->Count >= vdcm->MaxEntries)			/* Have to increase size of Master */
	       { int bytes ;
	         vdcm->MaxEntries = (int)(vdcm->MaxEntries * 1.5) ;
		 bytes = (char *)&vdcm->Clus[vdcm->MaxEntries] - (char *)vdcm ;
		 vdcm = realloc(vdcm,bytes) ;	         
	       } ;
	      vdcm->Clus[vdcm->Count++].vdcd = (vdcd = v4mm_AllocChunk(sizeof *vdcd,FALSE)) ;
	      vdcd->MaxEntries = V4DM_ClusterDtl_InitialEntryMax ; vdcd->Count = 0 ;
	    } ;
/*	   Add current point to currently defined cluster */
	   if (spnt.Bytes > sizeof vdcd->Entry[0].clpt)
	    { v_Msg(ctx,NULL,"DMClusterTooBig",intmodx) ; ipt = NULL ; goto cluster_finish ; } ;
	   if (vdcd->Count >= vdcd->MaxEntries)				/* Have to increase size for more points */
	    { int bytes ;
	      vdcd->MaxEntries = (int)(vdcd->MaxEntries * 1.5) ;
	      bytes = (char *)&vdcd->Entry[vdcd->MaxEntries] - (char *)vdcd ;
	      vdcd = realloc(vdcd,bytes) ; vdcm->Clus[savej].vdcd = vdcd ;
	    } ;
	   lpt = (P *)&vdcd->Entry[vdcd->Count].clpt ; memcpy(lpt,&spnt,spnt.Bytes) ; vdcd->Count++ ;
	 } ;
/*	All done assigning clusters - generate output lists */
	INITLP(respnt,lp,Dim_List) ;			/* Going to return list of Indexes */
	for(i=0;i<vdcm->Count;i++)
	 { idxpnt.Value.IntVal = startindex + i ;
	   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&idxpnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; ipt = NULL ; goto cluster_finish ; } ;
/*	   Create new list of points in this cluster */
	   ipt = &tpnt ; INITLP(ipt,lp1,Dim_List) ;
	   vdcd = vdcm->Clus[i].vdcd ;
	   for(j=0;j<vdcd->Count;j++)					/* Append each point in cluster to list */
	    { if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,(P *)&vdcd->Entry[j].clpt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; ipt = NULL ; goto cluster_finish ; } ;
	    } ; ENDLP(ipt,lp1) ;
/*	   Now create binding linking to this list */
	   ipt = &spnt ;						/* ipt = Build up intersection for binding */
	   if (bindpt->PntType != V4DPI_PntType_Isct)			/* If bind point not intersection then make one */
	    { INITISCT(ipt) ; NOISCTVCD(ipt) ; ipt->Grouping = 1 ;
	      memcpy(ISCT1STPNT(ipt),bindpt,bindpt->Bytes) ; ipt->Bytes += bindpt->Bytes ;
	    } else { memcpy(ipt,bindpt,bindpt->Bytes) ; } ;
	   for(k=0,ipt=ISCT1STPNT(&spnt);k<spnt.Grouping;k++) { ADVPNT(ipt) ; } ;
	   memcpy(ipt,&idxpnt,idxpnt.Bytes) ; spnt.Bytes += idxpnt.Bytes ; spnt.Grouping ++ ;
/*	   Now create binding to list */
	   if (!v4dpi_BindListMake(&binding,&tpnt,&spnt,ctx,NULL,NOWGTADJUST,0,DFLTRELH))
	    { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; ipt = NULL ; goto cluster_finish ; } ;
	 } ;
	ENDLP(respnt,lp) ;
	ipt = respnt ;
cluster_finish:
	if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; ipt = NULL ; } ;
	for(i=0;i<vdcm->Count;i++) { v4mm_FreeChunk(vdcm->Clus[i].vdcd) ; } ;
	v4mm_FreeChunk(vdcm) ;
	if (ipt == NULL) { REGISTER_ERROR(0) ; return(NULL) ; } ;
	return(ipt) ;
}

/*	v4im_DoReAllocate - Handles ReAllocate Module	*/

struct V4DPI__Point *v4im_DoReAllocate(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  int intmodx ;
  P *argpnts[] ;
  int argcnt,trace ;
{ struct V4L__ListPoint *lp1,*lp2 ; P lppt1, lppt2 ;
  struct lcl__Pair {
    int Count ;		/* Number of elements below */
    struct {
      double Interval, Value ;
     } Entry[1] ;	/* Allocated below to correct size */
   } *src,*tar ;
  P *pt,*cpt,*bpt,*ipt,*tpt,*opt,*vpt ; P ctxpt,valpt,bindpt ;
  struct V4DPI__Binding binding ;
  int i,j,ok,ax,method,frameid,num,tag ; double amt ;

	lp1 = NULL ; lp2 = NULL ;
	bpt = NULL ; ipt = NULL ; method = 1 ; tpt = NULL ; opt = NULL ; vpt = NULL ;
	for(ok=TRUE,ax=1;ok&&ax<=argcnt;ax++)
	 { 
	   if (argpnts[ax]->PntType != V4DPI_PntType_TagVal)
	    { if (lp1 == NULL) { lp1 = v4im_VerifyList(&lppt1,ctx,argpnts[ax],intmodx) ; }
	       else if (lp2 == NULL) { lp2 = v4im_VerifyList(&lppt2,ctx,argpnts[ax],intmodx) ; }
	       else { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"ListTooMany",2) ; break ; } ;
	      continue ;
	    } ;
	   switch (i=v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,NULL))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ;
	      case V4IM_Tag_Bind:	bpt = cpt ; break ;
	      case V4IM_Tag_Interval:	ipt = cpt ; break ;
	      case V4IM_Tag_Method:
		if (cpt->PntType != V4DPI_PntType_Dict)
		 { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"TagValue",V4IM_Tag_Method,cpt) ; break ; } ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"TagValue",V4IM_Tag_Method,cpt) ; break ;
		   case _Linear:	method = 1 ; break ;
		   case _First:		method = 2 ; break ;
		   case _Closest:	method = 3 ; break ;
		 } ;
//		if (cpt->Value.IntVal == v4dpi_DictEntryGet(ctx,UNUSED,"Linear",NULL,NULL)) { method = 1 ; }
//		 else if (cpt->Value.IntVal == v4dpi_DictEntryGet(ctx,UNUSED,"First",NULL,NULL)) { method = 2 ; }
//		 else if (cpt->Value.IntVal == v4dpi_DictEntryGet(ctx,UNUSED,"Closest",NULL,NULL)) { method = 3 ; }
//		 else { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"TagValue",V4IM_Tag_Method,cpt) ; break ; } ;
		break ;
	      case V4IM_Tag_Overflow:	opt = cpt ; break ;
	      case V4IM_Tag_Target:	tpt = cpt ; break ;
	      case V4IM_Tag_Value:	vpt = cpt ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	if (lp1 == NULL || lp2 == NULL) { v_Msg(ctx,NULL,"ListNumWrong",intmodx,2) ; REGISTER_ERROR(0) ; return(NULL) ; } ;
	if (bpt == NULL || ipt == NULL || tpt == NULL || vpt == NULL)
	 { v_Msg(ctx,NULL,"ModArgMand",intmodx) ; REGISTER_ERROR(0) ; return(NULL) ; } ;

	frameid = v4ctx_FramePush(ctx,NULL) ;		/* Start new context frame */
/*	Get size of source list & fill up with Interval & Value */
	num = v4l_ListPoint_Value(ctx,lp1,-1,&valpt) ;
	src = v4mm_AllocChunk(sizeof *src + (num * sizeof(src->Entry[0])),FALSE) ; src->Count = num ;
	for(i=0;;i++)					/* Loop thru each point in list */
	 { if (v4l_ListPoint_Value(ctx,lp1,i+1,&ctxpt) <= 0) break ;
	   if (!v4ctx_FrameAddDim(ctx,0,&ctxpt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//	   v4ctx_FrameAddDim(ctx,0,&ctxpt,0,0) ;
	   pt = v4dpi_IsctEval(&valpt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (pt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,(tag=V4IM_Tag_Interval),ipt) ; goto failure ; } ;
	   src->Entry[i].Interval = v4im_GetPointDbl(&ok,pt,ctx) ; if (!ok) break ;
	   pt = v4dpi_IsctEval(&valpt,vpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (pt == NULL) {  v_Msg(ctx,NULL,"TagEvalFail",intmodx,(tag=V4IM_Tag_Value),vpt) ; goto failure ; } ;
	   src->Entry[i].Value = v4im_GetPointDbl(&ok,pt,ctx) ; if (!ok) break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"TagValue2",intmodx,1,pt) ; goto failure ; } ;
/*	Repeat for just the Target (interval) */
	num = v4l_ListPoint_Value(ctx,lp2,-1,&valpt) ;
	tar = v4mm_AllocChunk(sizeof *tar + (num * sizeof(tar->Entry[0])),FALSE) ; tar->Count = num ;
	for(j=0;;j++)					/* Loop thru each point in list */
	 { if (v4l_ListPoint_Value(ctx,lp2,j+1,&ctxpt) <= 0) break ;
	   if (!v4ctx_FrameAddDim(ctx,0,&ctxpt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto failure ; } ;
//	   v4ctx_FrameAddDim(ctx,0,&ctxpt,0,0) ;
	   pt = v4dpi_IsctEval(&valpt,tpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (pt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,(tag=V4IM_Tag_Target),tpt) ; goto failure ; } ;
	   tar->Entry[j].Interval = v4im_GetPointDbl(&ok,pt,ctx) ; if (!ok) break ;
	   tar->Entry[j].Value = 0.0 ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"TagValue2",intmodx,1,pt) ; goto failure ; } ;
/*	Now go through source and allocate into target */
	for(i=0,j=0;i<src->Count && j<tar->Count;)
	 { switch(method)
	    { case 1:	/* Linear */
		if (src->Entry[i].Interval <= tar->Entry[j].Interval)	/* Source "fits" entirely in target */
		 { tar->Entry[j].Value += src->Entry[i].Value ; tar->Entry[j].Interval -= src->Entry[i].Interval ;
		   src->Entry[i++].Interval = 0 ; continue ;
		 } ;
/*		Source "fits" only partially - allocate & continue */
		if (tar->Entry[j].Interval <= 0) { j++ ; continue ; } ;	/* Just in case minor floating point issues */
		amt = src->Entry[i].Value * (tar->Entry[j].Interval / src->Entry[i].Interval) ;
		tar->Entry[j].Value += amt ; src->Entry[i].Value -= amt ;
		src->Entry[i].Interval -= tar->Entry[j].Interval ; tar->Entry[j++].Interval = 0 ;
		continue ;
	      case 2:	/* First */
		tar->Entry[j].Value += src->Entry[i].Value ;
		tar->Entry[j].Interval -= src->Entry[i++].Interval ;
		if (tar->Entry[j].Interval <= 0) j++ ;
		continue ;
	      case 3:	/* Closest */
/*		Should it go in current target slot or next */
		if (tar->Entry[j].Interval <= 0) { j++ ; continue ; } ;	/* Just in case minor floating point issues */
		if ((tar->Entry[j].Interval / src->Entry[i].Interval) < 0.50) j++ ;
		tar->Entry[j].Value += src->Entry[i].Value ; tar->Entry[j].Interval -= src->Entry[i++].Interval ;
		continue ;
	    } ;
	 } ;
/*	All done (at least end of target) - Is there any overflow to track? */
	if (opt != NULL)
	 { for(amt=0.0;i<src->Count;) { amt += src->Entry[i++].Value ; } ;
//	   ZPH(&valpt) ; valpt.Dim = Dim_Num ; valpt.PntType = V4DPI_PntType_Real ; valpt.Bytes = V4PS_Real ;
//	   memcpy(&valpt.Value.RealVal,&amt,sizeof amt) ;
	   dblPNTv(&valpt,amt) ;
	   if (opt->PntType != V4DPI_PntType_Isct)			/* If bind point not intersection then make one */
	    { INITISCT(&bindpt) ; NOISCTVCD(&bindpt) ; bindpt.Grouping = 1 ;
	      memcpy(ISCT1STPNT(&bindpt),opt,opt->Bytes) ; bindpt.Bytes += opt->Bytes ;
	    } else { memcpy(&bindpt,opt,opt->Bytes) ; } ;
	   if (!v4dpi_BindListMake(&binding,&valpt,&bindpt,ctx,NULL,NOWGTADJUST,0,DFLTRELH))
	    { v_Msg(ctx,NULL,"BindMakeErr",intmodx,&bindpt,&valpt) ; goto failure ; } ;
	 } ;
/*	Create bindings for Target */
	for(j=0;j<tar->Count;j++)
	 { dblPNTv(&valpt,tar->Entry[j].Value) ;
//	   ZPH(&valpt) ; valpt.Dim = Dim_Num ; valpt.PntType = V4DPI_PntType_Real ; valpt.Bytes = V4PS_Real ;
//	   memcpy(&valpt.Value.RealVal,&tar->Entry[j].Value,sizeof tar->Entry[j].Value) ;
	   if (bpt->PntType != V4DPI_PntType_Isct)			/* If bind point not intersection then make one */
	    { INITISCT(&bindpt) ; NOISCTVCD(&bindpt) ; bindpt.Grouping = 1 ;
	      memcpy(ISCT1STPNT(&bindpt),bpt,bpt->Bytes) ; bindpt.Bytes += bpt->Bytes ;
	    } else { memcpy(&bindpt,bpt,bpt->Bytes) ; } ;
	   pt = (P *)((char *)&bindpt + bindpt.Bytes) ;			/* pt = spot for target */
	   if (v4l_ListPoint_Value(ctx,lp2,j+1,&ctxpt) <= 0)
	    { v_Msg(ctx,NULL,"ListGetErr",intmodx,j+1,&lppt2) ; goto failure ; } ;
	   memcpy(pt,&ctxpt,ctxpt.Bytes) ; bindpt.Bytes += ctxpt.Bytes ; bindpt.Grouping ++ ;
	   if (!v4dpi_BindListMake(&binding,&valpt,&bindpt,ctx,NULL,NOWGTADJUST,0,DFLTRELH))
	    { v_Msg(ctx,NULL,"BindMakeErr",intmodx,&bindpt,&valpt) ; goto failure ; } ;
	 } ;
/*	Deallocate everything & return happy */
	v4mm_FreeChunk(src) ; v4mm_FreeChunk(tar) ;
	if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameid) ; goto failure ; } ;
	return((P *)&Log_True) ;
failure:
	REGISTER_ERROR(0) ; v4ctx_FramePop(ctx,frameid,NULL) ; return(NULL) ;
}


