#include "rand.h"


RAND::RAND(long num=444)
:idum(num),iset(0) {
	if (idum==0) {
		exit(-1);
	}
}

float RAND::gammln(float xx)
//Returns the value ln[Γ(xx)] for xx > 0.
{
/*Internal arithmetic will be done in double precision, a nicety that 
you can omit if five-figure accuracy is good enough. */
double x,y,tmp,ser;
static double cof[6]={76.18009172947146,-86.50532032941677,
24.01409824083091,-1.231739572450155,
0.1208650973866179e-2,-0.5395239384953e-5};
int j;
y=x=xx;
tmp=x+5.5;
tmp -= (x+0.5)*log(tmp);
ser=1.000000000190015;
for (j=0;j<=5;j++) ser += cof[j]/++y;
return -tmp+log(2.5066282746310005*ser/x);
}

float RAND::rand1(){
	int j;
	long k;
	static long iy=0;
	static long iv[NTAB];
	if (idum <= 0 || !iy) { 
		if (-(idum) < 1) idum=1; //Be sure to prevent idum = 0.
		else idum = -(idum);
		for (j=NTAB+7;j>=0;j--) { //Load the shuffle table (after 8 warm-ups).
			k=(idum)/IQ;
			idum=IA*(idum-k*IQ)-IR*k;
			if (idum < 0) idum += IM;
			if (j < NTAB) iv[j] = idum;
		}
		iy=iv[0];
	}
	k=(idum)/IQ; //Start here when not initializing.
	idum=IA*(idum-k*IQ)-IR*k; //Compute idum=(IA*idum) % IM without overif
	if (idum < 0) idum += IM; //flows by Schrage’s method.
	j=iy/NDIV; //Will be in the range 0..NTAB-1.
	iy=iv[j]; //Output previously stored value and refill the
	iv[j] = idum; //shuffle table.
	//if ((temp=) > RNMX) return RNMX; //Because users don’t expect endpoint values.
	/*else*/ return (AM*iy);
}
float RAND::randq(){
	unsigned long itemp;
	long idum1=idum;
	float rand;
	static unsigned long jflone = 0x3f800000;
	static unsigned long jflmsk = 0x007fffff;
	idum1 = 1664525L*idum1 + 1013904223L;
	itemp = jflone | (jflmsk & idum1);
	rand = (*(float *)&itemp)-1.0;
	return rand;
}

float RAND::poidev(float xm)
//Returns as a floating-point number an integer value that is a random deviate drawn from a
//Poisson distribution of mean xm, using ran1(idum) as a source of uniform random deviates.
{
static float sq,alxm,g,oldm=(-1.0); //oldm is a flag for whether xm has changed
float em,t,y; //since last call.
if (xm < 12.0) { //Use direct method.
	if (xm != oldm) {
		oldm=xm;
		g=exp(-xm); //If xm is new, compute the exponential.
	}
	em = -1;
	t=1.0;
	do { 
/*Instead of adding exponential deviates it is equivalent
to multiply uniform deviates. We never
actually have to take the log, merely compare
to the pre-computed exponential.*/
		++em;
		t *= rand1();
	} while (t > g);
} else {		//Use rejection method.
	if (xm != oldm) {		//If xm has changed since the last call, then preoldm=
		oldm=xm;				//compute some functions that occur below.
		sq=sqrt(2.0*xm);
		alxm=log(xm);
		g=xm*alxm-gammln(xm+1.0);
			//The function gammln is the natural log of the gamma function, as given in §6.1.
	}
	do {
		do { //y is a deviate from a Lorentzian comparison funcytion.
			y=tan(PI*rand1()); 
			em=sq*y+xm; //em is y, shifted and scaled.
		} while (em < 0.0); //Reject if in regime of zero probability.
		em=floor(em); //The trick for integer-valued distributions.
		t=0.9*(1.0+y*y)*exp(em*alxm-gammln(em+1.0)-g);
/*The ratio of the desired distribution to the comparison function; we accept or
reject by comparing it to another uniform deviate. The factor 0.9 is chosen so
that t never exceeds 1.*/
	} while (rand1() > t);
}
return em;
}

type RAND::bindev(double pr, type nn, int opt){
	if (0){//(nn > 10000000 || opt==1){ //normal distribution- transformation
		//produces sometimes lowest possible integer
		double sigma,mu;
		mu=nn*pr;
		sigma = sqrt(nn*pr*(1-pr) );
		//printf("&");
		return (gasdev()* sigma + mu);
		}
	float erg = bind(pr, (unsigned long) nn);
	erg = (type) erg;
	//printf(" %d",erg);
	return erg;
	}

float RAND::bind(float pp, type n)
/*Returns as a floating-point number an integer value that is a random deviate drawn from
a binomial distribution of n trials each of probability pp, using ran1(idum) as a source of
uniform random deviates.*/
{
long j;
static int nold=(-1);
float am,em,g,angle,p,bnl,sq,t,y;
static float pold=(-1.0),pc,plog,pclog,en,oldg;
p=(pp <= 0.5 ? pp : 1.0-pp);
/*The binomial distribution is invariant under changing pp to 1-pp, if we also change the
answer to n minus itself; we’ll remember to do this below.*/
am=n*p;			//This is the mean of the deviate to be produced.
if (n < 45) {		//Use the direct method while n is not too large.
	bnl=0.0;		//This can require up to 45 calls to ran1.
	//printf("A");
	for (j=1;j<=n;j++)
		if (rand() < p) ++bnl;
	} else if (am < 1.0) { 
/*If fewer than one event is expected out of 25
or more trials, then the distribution is quite
accurately Poisson. Use direct Poisson method.*/
		//printf("P");
		g=exp(-am);
		t=1.0;
		for (j=0;j<=n;j++) {
			t *= rand1();
			if (t < g) break;
		}

		bnl=(j <= n ? j : n);
		//printf("Poi: %e,%d,%d : %e",g,j,n,bnl);

	} else { //Use the rejection method.
		//printf("R");
		if (n != nold) { //If n has changed, then compute useful quantienties.
			en=n; 
			oldg=gammln(en+1.0);
			nold=n;
		} if (p != pold) { //If p has changed, then compute useful quantipcties.
			pc=1.0 - p; 
			plog=log(p);
			pclog=log(pc);
			pold=p;
		}

		sq=sqrt(2.0*am*pc); 
/*The following code should by now seem familiar:
rejection method with a Lorentzian comparison
function.*/
		do {
			do {
				angle=PI*rand1();
				y=tan(angle);
				em=sq*y+am;
			} while (em < 0.0 || em >= (en+1.0)); //Reject.
				em=floor(em); //Trick for integer-valued distribution.
				t=1.2*sq*(1.0+y*y)*exp(oldg-gammln(em+1.0)
					-gammln(en-em+1.0)+em*plog+(en-em)*pclog);
		} while (rand() > t); //Reject. This happens about 1.5 times per devibnlate, on average.
	bnl=em; 
		}
if (p != pp) bnl=n-bnl; //Remember to undo the symmetry transformation.

return bnl; 
}



double RAND::gasdev()
/*Returns a normally distributed deviate with zero mean and unit variance, using ran1(idum)
as the source of uniform deviates.*/
{
double fac,rsq,v1,v2;
	if (iset == 0) { //We don’t have an extra deviate handy, so
		do {
			v1=2.0*rand1()-1.0; //pick two uniform numbers in the square extending
			v2=2.0*rand1()-1.0; //from -1 to +1 in each direction,
			rsq=v1*v1+v2*v2; //see if they are in the unit circle,
		} while (rsq >= 1.0 || rsq == 0.0); //and if they are not, try again.
		fac=sqrt(-2.0*log(rsq)/rsq);
//Now make the Box-Muller transformation to get two normal deviates. Return one and
//save the other for next time.
		gset=v1*fac;
		iset=1; //Set flag.
	return v2*fac;
} else { //We have an extra deviate handy,
	iset=0; //so unset the flag,
	return gset; //and return it.
	}
}