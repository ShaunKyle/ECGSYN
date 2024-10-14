// "ran1" is a routine available from Numerical Recipes in C.
// random deviate, minimal standard plus shuffle
//
// http://numerical.recipes/routines/instc.html
// C routines in Numerical Recipes Second Edition, by chapter and section.

/*---------------------------------------------------------------------------*/
/*      DEFINITIONS FOR CONSTANTS                                            */
/*---------------------------------------------------------------------------*/

// Choices for the multiplier a (IA) and modulus m (IM) in Park and Miller's 
// "Minimal Standard" generator.
#define IA 16807
#define IM 2147483647


#define AM (1.0/IM)

#define IQ 127773
#define IR 2836

#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)

// Random value maxiumum (RNMX), based on interval machine epsilon for binary32
// floating-point number (EPS).
//
// https://en.wikipedia.org/wiki/Machine_epsilon
#define EPS 1.2e-7
#define RNMX (1.0-EPS)


/*---------------------------------------------------------------------------*/
/*      RANDOM NUMBER GENERATOR                                              */
/*---------------------------------------------------------------------------*/

//! @brief Generates a uniform deviate (random number) within range of 0 to 1.
//!
//! "Minimal Standard" generator proposed by Park and Miller (1969), with 
//! Bays-Durham shuffle and added safeguards.
//!
//! @param idum
//!
//! @return a uniform deviate between 0.0 and 1.0
float ran1(long *idum){
	int j;
	long k;
	static long iy=0;
	static long iv[NTAB];
	float temp;

	if (*idum <= 0 || !iy) {
		if (-(*idum) < 1) *idum=1;
		else *idum = -(*idum);
		for (j=NTAB+7;j>=0;j--) {
			k=(*idum)/IQ;
			*idum=IA*(*idum-k*IQ)-IR*k;
			if (*idum < 0) *idum += IM;
			if (j < NTAB) iv[j] = *idum;
		}
		iy=iv[0];
	}
	k=(*idum)/IQ;
	*idum=IA*(*idum-k*IQ)-IR*k;
	if (*idum < 0) *idum += IM;
	j=iy/NDIV;
	iy=iv[j];
	iv[j] = *idum;
	if ((temp=AM*iy) > RNMX) return RNMX;
	else return temp;
}
