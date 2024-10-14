// "dfour1" is a routine available from Numerical Recipes in C.
// Double-precision version of "four1", fast Fourier transform (FFT) in 1D.
//
// http://numerical.recipes/routines/instc.html
// C routines in Numerical Recipes Second Edition, by chapter and section.

#include <math.h>   // sin

#define SWAP(a,b) tempr=a;a=b;b=tempr

//! @brief Replaces data[1..2*nn] by its discrete Fourier transform if `isign` 
//! is 1, or it inverse Fourier transform if `isign` is -1.
//!
//! @param data     array of complex numbers (1st element real, 2nd imaginary)
//! @param nn       number of complex elements in data array
//! @param isign    forward transform if 1, inverse transform if -1
void dfour1(double data[], int nn, int isign){

  int n,mmax,m,j,istep,i;
  double wtemp,wr,wpr,wpi,wi,theta;
  double tempr,tempi;

	n=nn << 1;
	j=1;
	for (i=1;i<n;i+=2) {
		if (j > i) {
            SWAP(data[j],data[i]);
			SWAP(data[j+1],data[i+1]);
		}
		m=n>>1;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1;
		}
		j+=m;
	}

	mmax=2;
	while (n > mmax) {
		istep=mmax << 1;
		theta=isign*(6.28318530717959/mmax);
		wtemp=sin(0.5*theta);
		wpr = -2.0*wtemp*wtemp;
		wpi=sin(theta);
		wr=1.0;
		wi=0.0;
		for (m=1;m<mmax;m+=2) {
			for (i=m;i<=n;i+=istep) {
				j=i+mmax;
				tempr=wr*data[j]-wi*data[j+1];
				tempi=wr*data[j+1]+wi*data[j];
				data[j]=data[i]-tempr;
				data[j+1]=data[i+1]-tempi;
				data[i] += tempr;
				data[i+1] += tempi;
			}
			wr=(wtemp=wr)*wpr-wi*wpi+wr;
			wi=wi*wpr+wtemp*wpi+wi;
		}
		mmax=istep;
	}

}
