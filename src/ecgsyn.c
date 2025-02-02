/* 09 Oct 2003 "ecgsyn.c"                                                     */
/*                                                                            */
/* Copyright (c)2003 by Patrick McSharry & Gari Clifford, All Rights Reserved */
/* See IEEE Transactions On Biomedical Engineering, 50(3),289-294, March 2003.*/
/* Contact P. McSharry (patrick AT mcsharry DOT net) or                       */
/* G. Clifford (gari AT mit DOT edu)                                          */
/*                                                                            */      
/*   This program is free software; you can redistribute it and/or modify     */ 
/*   it under the terms of the GNU General Public License as published by     */ 
/*   the Free Software Foundation; either version 2 of the License, or        */ 
/*   (at your option) any later version.                                      */ 
/*                                                                            */ 
/*   This program is distributed in the hope that it will be useful,          */ 
/*   but WITHOUT ANY WARRANTY; without even the implied warranty of           */ 
/*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */ 
/*   GNU General Public License for more details.                             */
/*                                                                            */      
/*   You should have received a copy of the GNU General Public License        */
/*   along with this program; if not, write to the Free Software Foundation   */
/*   Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA            */ 
/*                                                                            */ 
/*  ecgsyn.m and its dependents are freely availble from Physionet -          */ 
/*  http://www.physionet.org/ - please report any bugs to the authors above.  */

#include <stdio.h>   
#include <math.h>  
#include <stdlib.h> 
#include "opt.h"
#define PI (2.0*asin(1.0))
#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr
#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)
#define PI (2.0*asin(1.0))
#define OFFSET 1
#define ARG1 char*

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

/*--------------------------------------------------------------------------*/
/*    DEFINE PARAMETERS AS GLOBAL VARIABLES                                 */
/*--------------------------------------------------------------------------*/

char outfile[100]="ecgsyn.dat";/*  Output data file                   */ 
int N = 256;                   /*  Number of heart beats              */
int sfecg = 256;               /*  ECG sampling frequency             */
int sf = 256;                  /*  Internal sampling frequency        */
double Anoise = 0.0;           /*  Amplitude of additive uniform noise*/
double hrmean = 60.0;          /*  Heart rate mean                    */
double hrstd = 1.0;            /*  Heart rate std                     */
double flo = 0.1;              /*  Low frequency                      */
double fhi = 0.25;             /*  High frequency                     */
double flostd = 0.01;          /*  Low frequency std                  */
double fhistd = 0.01;          /*  High frequency std                 */
double lfhfratio = 0.5;        /*  LF/HF ratio                        */

int Necg = 0;                  /*  Number of ECG outputs              */
int mstate = 3;                /*  System state space dimension       */
double xinitial = 1.0;         /*  Initial x co-ordinate value        */
double yinitial = 0.0;         /*  Initial y co-ordinate value        */
double zinitial = 0.04;        /*  Initial z co-ordinate value        */
int seed = 1;                  /*  Seed                               */
long rseed; 
double h; 
double *rr,*rrpc;
double *ti,*ai,*bi;

/* prototypes for externally defined functions */
void dfour1(double data[], unsigned long nn, int isign);
float ran1(long *idum);

/*---------------------------------------------------------------------------*/
/*      ALLOCATE MEMORY FOR VECTOR                                           */
/*---------------------------------------------------------------------------*/

double *mallocVect(long n0, long nx)
{
        double *vect;
 
        vect=(double *)malloc((size_t) ((nx-n0+1+OFFSET)*sizeof(double)));
        if (!vect){
	  printf("Memory allocation failure in mallocVect");
	}
        return vect-n0+OFFSET;
}

/*---------------------------------------------------------------------------*/
/*      FREE MEMORY FOR MALLOCVECT                                           */
/*---------------------------------------------------------------------------*/
 
void freeVect(double *vect, long n0, long nx)
{
        free((ARG1) (vect+n0-OFFSET));
}

/*---------------------------------------------------------------------------*/
/*      MEAN CALCULATOR                                                      */
/*---------------------------------------------------------------------------*/
 
double mean(double *x, int n)
/* n-by-1 vector, calculate mean */
{
        int j;
        double add;
 
        add = 0.0;
        for(j=1;j<=n;j++)  add += x[j];
 
        return (add/n);
}


/*---------------------------------------------------------------------------*/
/*      STANDARD DEVIATION CALCULATOR                                        */
/*---------------------------------------------------------------------------*/

double stdev(double *x, int n)
/* n-by-1 vector, calculate standard deviation */
{
        int j;
        double add,mean,diff,total;

        add = 0.0;
        for(j=1;j<=n;j++)  add += x[j];
        mean = add/n;

        total = 0.0;
        for(j=1;j<=n;j++)  
        {
           diff = x[j] - mean;
           total += diff*diff;
        } 
  
        return (sqrt(total/(n-1)));
}

/*--------------------------------------------------------------------------*/
/*    WRITE VECTOR IN A FILE                                                */
/*--------------------------------------------------------------------------*/

void vecfile(char filename[], double *x, int n)
{
   int i;
   FILE *fp;
  
   fp = fopen(filename,"w");
   for(i=1;i<=n;i++)  fprintf(fp,"%e\n",x[i]);
   fclose(fp);
}

/*--------------------------------------------------------------------------*/
/*    INTERP                                                                */
/*--------------------------------------------------------------------------*/

void interp(double *y, double *x, int n, int r)
{
   int i,j;
   double a;

   for(i=1;i<=n-1;i++)
   {
      for(j=1;j<=r;j++) 
      {
         a = (j-1)*1.0/r;
         y[(i-1)*r+j] = (1.0-a)*x[i] + a*x[i+1];
      }
   }
}


/*--------------------------------------------------------------------------*/
/*    GENERATE RR PROCESS                                                   */
/*--------------------------------------------------------------------------*/

void rrprocess(double *rr, double flo, double fhi, 
double flostd, double fhistd, double lfhfratio,  
double hrmean, double hrstd, double sf, int n)
{
   int i,j;
   double c1,c2,w1,w2,sig1,sig2,rrmean,rrstd,xstd,ratio;
   double df,dw1,dw2,*w,*Hw,*Sw,*ph0,*ph,*SwC;

   w = mallocVect(1,n);
   Hw = mallocVect(1,n);
   Sw = mallocVect(1,n);
   ph0 = mallocVect(1,n/2-1);
   ph = mallocVect(1,n);
   SwC = mallocVect(1,2*n);


   w1 = 2.0*PI*flo;
   w2 = 2.0*PI*fhi;
   c1 = 2.0*PI*flostd;
   c2 = 2.0*PI*fhistd;
   sig2 = 1.0;
   sig1 = lfhfratio;
   rrmean = 60.0/hrmean;
   rrstd = 60.0*hrstd/(hrmean*hrmean);

   df = sf/n;
   for(i=1;i<=n;i++) w[i] = (i-1)*2.0*PI*df;
   for(i=1;i<=n;i++) 
   {
      dw1 = w[i]-w1;
      dw2 = w[i]-w2;
      Hw[i] = sig1*exp(-dw1*dw1/(2.0*c1*c1))/sqrt(2*PI*c1*c1) 
            + sig2*exp(-dw2*dw2/(2.0*c2*c2))/sqrt(2*PI*c2*c2); 
   }
   for(i=1;i<=n/2;i++) Sw[i] = (sf/2.0)*sqrt(Hw[i]);
   for(i=n/2+1;i<=n;i++) Sw[i] = (sf/2.0)*sqrt(Hw[n-i+1]);


   /* randomise the phases */
   for(i=1;i<=n/2-1;i++) ph0[i] = 2.0*PI*ran1(&rseed);
   ph[1] = 0.0;
   for(i=1;i<=n/2-1;i++) ph[i+1] = ph0[i];
   ph[n/2+1] = 0.0;
   for(i=1;i<=n/2-1;i++) ph[n-i+1] = - ph0[i]; 


   /* make complex spectrum */
   for(i=1;i<=n;i++) SwC[2*i-1] = Sw[i]*cos(ph[i]);
   for(i=1;i<=n;i++) SwC[2*i] = Sw[i]*sin(ph[i]);

   /* calculate inverse fft */
   dfour1(SwC,n,-1);

   /* extract real part */
   for(i=1;i<=n;i++) rr[i] = (1.0/n)*SwC[2*i-1];

   xstd = stdev(rr,n);
   ratio = rrstd/xstd; 

   for(i=1;i<=n;i++) rr[i] *= ratio;
   for(i=1;i<=n;i++) rr[i] += rrmean;

   freeVect(w,1,n);
   freeVect(Hw,1,n);
   freeVect(Sw,1,n);
   freeVect(ph0,1,n/2-1);
   freeVect(ph,1,n);
   freeVect(SwC,1,2*n);
}

/*--------------------------------------------------------------------------*/
/*    THE ANGULAR FREQUENCY                                                 */
/*--------------------------------------------------------------------------*/

double angfreq(double t)
{
   int i;
  
   i = 1 + (int)floor(t/h);
  
   return 2.0*PI/rrpc[i];
}

/*--------------------------------------------------------------------------*/
/*    THE EXACT NONLINEAR DERIVATIVES                                       */
/*--------------------------------------------------------------------------*/

void derivspqrst(double t0,double x[],double dxdt[])
{
   int i,k;
   double a0,w0,r0,x0,y0,z0;
   double t,dt,dt2,*xi,*yi,zbase;
 
   k = 5; 
   xi = mallocVect(1,k);
   yi = mallocVect(1,k); 
  
   w0 = angfreq(t0);
   r0 = 1.0; x0 = 0.0;  y0 = 0.0;  z0 = 0.0;
   a0 = 1.0 - sqrt((x[1]-x0)*(x[1]-x0) + (x[2]-y0)*(x[2]-y0))/r0;

   for(i=1;i<=k;i++) xi[i] = cos(ti[i]);
   for(i=1;i<=k;i++) yi[i] = sin(ti[i]);   

   zbase = 0.005*sin(2.0*PI*fhi*t0);

   t = atan2(x[2],x[1]);
   dxdt[1] = a0*(x[1] - x0) - w0*(x[2] - y0);
   dxdt[2] = a0*(x[2] - y0) + w0*(x[1] - x0); 
   dxdt[3] = 0.0;  
   for(i=1;i<=k;i++)  
   {
      dt = fmod(t-ti[i],2.0*PI);
      dt2 = dt*dt;
      dxdt[3] += -ai[i]*dt*exp(-0.5*dt2/(bi[i]*bi[i])); 
   }
   dxdt[3] += -1.0*(x[3] - zbase);
  
   freeVect(xi,1,k);
   freeVect(yi,1,k);
}

/*--------------------------------------------------------------------------*/
/*    RUNGA-KUTTA FOURTH ORDER INTEGRATION                                  */
/*--------------------------------------------------------------------------*/

void drk4(double y[], int n, double x, double h, double yout[], 
          void (*derivs)(double, double [], double []))
{
        int i;
        double xh,hh,h6,*dydx,*dym,*dyt,*yt;
 
        dydx=mallocVect(1,n);
        dym=mallocVect(1,n);
        dyt=mallocVect(1,n);
        yt=mallocVect(1,n);

        hh=h*0.5;
        h6=h/6.0;
        xh=x+hh;
        (*derivs)(x,y,dydx);
        for (i=1;i<=n;i++) yt[i]=y[i]+hh*dydx[i];
        (*derivs)(xh,yt,dyt);
        for (i=1;i<=n;i++) yt[i]=y[i]+hh*dyt[i];
        (*derivs)(xh,yt,dym);
        for (i=1;i<=n;i++) {
                yt[i]=y[i]+h*dym[i];
                dym[i] += dyt[i];
        }
        (*derivs)(x+h,yt,dyt);
        for (i=1;i<=n;i++)
                yout[i]=y[i]+h6*(dydx[i]+dyt[i]+2.0*dym[i]);

        freeVect(dydx,1,n);
        freeVect(dym,1,n);
        freeVect(dyt,1,n);
        freeVect(yt,1,n);
}

/*--------------------------------------------------------------------------*/
/*    DETECT PEAKS                                                          */
/*--------------------------------------------------------------------------*/

double detectpeaks(double *ipeak, double *x, double *y, double *z, int n)
{
   int i,j,j1,j2,jmin,jmax,d;
   double thetap1,thetap2,thetap3,thetap4,thetap5;
   double theta1,theta2,d1,d2,zmin,zmax;
   
   /* use globally defined angles for PQRST */
   thetap1 = ti[1];
   thetap2 = ti[2];
   thetap3 = ti[3];
   thetap4 = ti[4];
   thetap5 = ti[5];

   for(i=1;i<=n;i++) ipeak[i] = 0.0;
   theta1 = atan2(y[1],x[1]);
   for(i=1;i<n;i++)
   {
      theta2 = atan2(y[i+1],x[i+1]);
      if( (theta1 <= thetap1) && (thetap1 <= theta2) )  
      {
	d1 = thetap1 - theta1;
        d2 = theta2 - thetap1;
        if(d1 < d2)  ipeak[i] = 1.0;
        else         ipeak[i+1] = 1.0;
      }
      else if( (theta1 <= thetap2) && (thetap2 <= theta2) )  
      {
	d1 = thetap2 - theta1;
        d2 = theta2 - thetap2;
        if(d1 < d2)  ipeak[i] = 2.0;
        else         ipeak[i+1] = 2.0;
      }
      else if( (theta1 <= thetap3) && (thetap3 <= theta2) )  
      {
	d1 = thetap3 - theta1;
        d2 = theta2 - thetap3;
        if(d1 < d2)  ipeak[i] = 3.0;
        else         ipeak[i+1] = 3.0;
      }
      else if( (theta1 <= thetap4) && (thetap4 <= theta2) )  
      {
	d1 = thetap4 - theta1;
        d2 = theta2 - thetap4;
        if(d1 < d2)  ipeak[i] = 4.0;
        else         ipeak[i+1] = 4.0;
      }
      else if( (theta1 <= thetap5) && (thetap5 <= theta2) )  
      {
	d1 = thetap5 - theta1;
        d2 = theta2 - thetap5;
        if(d1 < d2)  ipeak[i] = 5.0;
        else         ipeak[i+1] = 5.0;
      }
      theta1 = theta2; 
   }

   /* correct the peaks */
   d = (int)ceil(sfecg/64);
   for(i=1;i<=n;i++)
   { 
     if( ipeak[i]==1 || ipeak[i]==3 || ipeak[i]==5 )
     {
        j1 = MAX(1,i-d);
        j2 = MIN(n,i+d);
        jmax = j1;
        zmax = z[j1];
        for(j=j1+1;j<=j2;j++)
	{ 
	   if(z[j] > zmax) 
           {
	      jmax = j;
              zmax = z[j];
	   }
	}
        if(jmax != i)
	{
           ipeak[jmax] = ipeak[i];
           ipeak[i] = 0;
	}
     }
     else if( ipeak[i]==2 || ipeak[i]==4 )
     {
        j1 = MAX(1,i-d);
        j2 = MIN(n,i+d);
        jmin = j1;
        zmin = z[j1];
        for(j=j1+1;j<=j2;j++)
	{ 
	   if(z[j] < zmin) 
           {
	      jmin = j;
              zmin = z[j];
	   }
	}
        if(jmin != i)
	{
           ipeak[jmin] = ipeak[i];
           ipeak[i] = 0;
	}
     }
   }

}

/*--------------------------------------------------------------------------*/
/*      MAIN PROGRAM                                                         */
/*---------------------------------------------------------------------------*/

main(argc,argv)
int     argc;
char    **argv;
{

    /* First step is to register the options */

    optregister(outfile,CSTRING,'O',"Name of output data file");  
    optregister(N,INT,'n',"Approximate number of heart beats");    
    optregister(sfecg,INT,'s',"ECG sampling frequency [Hz]");   
    optregister(sf,INT,'S',"Internal Sampling frequency [Hz]"); 
    optregister(Anoise,DOUBLE,'a',"Amplitude of additive uniform noise [mV]");
    optregister(hrmean,DOUBLE,'h',"Heart rate mean [bpm]");
    optregister(hrstd,DOUBLE,'H',"Heart rate standard deviation [bpm]");
    optregister(flo,DOUBLE,'f',"Low frequency [Hz]");
    optregister(fhi,DOUBLE,'F',"High frequency [Hz]");
    optregister(flostd,DOUBLE,'v',"Low frequency standard deviation [Hz]");
    optregister(fhistd,DOUBLE,'V',"High frequency standard deviation [Hz]");
    optregister(lfhfratio,DOUBLE,'q',"LF/HF ratio");
    optregister(seed,INT,'R',"Seed");    
    opt_title_set("ECGSYN: A program for generating a realistic synthetic ECG\n" 
     "Copyright (c) 2003 by Patrick McSharry & Gari Clifford. All rights reserved.\n");

    getopts(argc,argv);

    dorun();
}



/*--------------------------------------------------------------------------*/
/*    DORUN PART OF PROGRAM                                                 */
/*--------------------------------------------------------------------------*/

int dorun()
{
   int i,j,k,q,Nrr,Nt,Nts;
   double *x,tstep,tecg,rrmean,qd,hrfact,hrfact2;
   double *xt,*yt,*zt,*xts,*yts,*zts;
   double timev,*ipeak,zmin,zmax,zrange;
   FILE *fp;
   void (*derivs)(double, double [], double []);

   /* perform some checks on input values */
   q = (int)rint(sf/sfecg);
   qd = (double)sf/(double)sfecg;
   if(q != qd) {
     printf("Internal sampling frequency must be an integer multiple of the \n"); 
     printf("ECG sampling frequency!\n"); 
     printf("Your current choices are:\n");
     printf("ECG sampling frequency: %d Hertz\n",sfecg);
     printf("Internal sampling frequency: %d Hertz\n",sf);
     exit(1);}


   /* declare and initialise the state vector */
   x=mallocVect(1,mstate);
   x[1] = xinitial; 
   x[2] = yinitial;
   x[3] = zinitial;

   /* declare and define the ECG morphology vectors (PQRST extrema parameters) */
   ti=mallocVect(1,5);
   ai=mallocVect(1,5);
   bi=mallocVect(1,5);
   /* P            Q            R           S           T        */
   ti[1]=-60.0; ti[2]=-15.0; ti[3]=0.0;  ti[4]=15.0; ti[5]=90.0;
   ai[1]=1.2;   ai[2]=-5.0;  ai[3]=30.0; ai[4]=-7.5; ai[5]=0.75;
   bi[1]=0.25;  bi[2]=0.1;   bi[3]=0.1;  bi[4]=0.1;  bi[5]=0.4;

   /* convert angles from degrees to radians */
   for(i=1;i<=5;i++) ti[i] *= PI/180.0;

   /* adjust extrema parameters for mean heart rate */
   hrfact = sqrt(hrmean/60.0);
   hrfact2 = sqrt(hrfact);
   for(i=1;i<=5;i++) bi[i] *= hrfact;
   ti[1]*=hrfact2;  ti[2]*=hrfact; ti[3]*=1.0; ti[4]*=hrfact; ti[5]*=1.0;


   /* calculate time scales */
   h = 1.0/sf;
   tstep = 1.0/sfecg;

   printf("ECGSYN: A program for generating a realistic synthetic ECG\n" 
    "Copyright (c) 2003 by Patrick McSharry & Gari Clifford. All rights reserved.\n"
    "See IEEE Transactions On Biomedical Engineering, 50(3), 289-294, March 2003.\n"
    "Contact P. McSharry (patrick@mcsharry.net) or G. Clifford (gari@mit.edu)\n"); 

   printf("Approximate number of heart beats: %d\n",N);
   printf("ECG sampling frequency: %d Hertz\n",sfecg);
   printf("Internal sampling frequency: %d Hertz\n",sf);
   printf("Amplitude of additive uniformly distributed noise: %g mV\n",Anoise);
   printf("Heart rate mean: %g beats per minute\n",hrmean);
   printf("Heart rate std: %g beats per minute\n",hrstd);
   printf("Low frequency: %g Hertz\n",flo);
   printf("High frequency std: %g Hertz\n",fhistd);
   printf("Low frequency std: %g Hertz\n",flostd);
   printf("High frequency: %g Hertz\n",fhi);
   printf("LF/HF ratio: %g\n",lfhfratio);

   /* initialise seed */
   rseed = -seed;  


   /* select the derivs to use */
   derivs = derivspqrst;

   /* calculate length of RR time series */
   rrmean = (60/hrmean);
   Nrr = (int)pow(2.0, ceil(log10(N*rrmean*sf)/log10(2.0)));	 
   printf("Using %d = 2^%d samples for calculating RR intervals\n",
           Nrr,(int)(log10(1.0*Nrr)/log10(2.0))); 


   /* create rrprocess with required spectrum */
   rr = mallocVect(1,Nrr);
   rrprocess(rr, flo, fhi, flostd, fhistd, lfhfratio, hrmean, hrstd, sf, Nrr); 
   vecfile("rr.dat",rr,Nrr);

   /* create piecewise constant rr */
   rrpc = mallocVect(1,2*Nrr);
   tecg = 0.0;
   i = 1;
   j = 1;
   while(i <= Nrr)
   {  
      tecg += rr[j];
      j = (int)rint(tecg/h);
      for(k=i;k<=j;k++) rrpc[k] = rr[i];
      i = j+1;
   }
   Nt = j;
   vecfile("rrpc.dat",rrpc,Nt);

   printf("Printing ECG signal to file: %s\n",outfile);

   /* integrate dynamical system using fourth order Runge-Kutta*/
   xt = mallocVect(1,Nt);
   yt = mallocVect(1,Nt);
   zt = mallocVect(1,Nt);

   timev = 0.0;
   for(i=1;i<=Nt;i++)
   {
      xt[i] = x[1];
      yt[i] = x[2];
      zt[i] = x[3];
      drk4(x, mstate, timev, h, x, derivs);
      timev += h;
   }


   /* downsample to ECG sampling frequency */
   xts = mallocVect(1,Nt);
   yts = mallocVect(1,Nt);
   zts = mallocVect(1,Nt);

   j=0;
   for(i=1;i<=Nt;i+=q)
   { 
      j++;
      xts[j] = xt[i];
      yts[j] = yt[i];
      zts[j] = zt[i];
   }
   Nts = j;


   /* do peak detection using angle */
   ipeak = mallocVect(1,Nts);
   detectpeaks(ipeak, xts, yts, zts, Nts);
 
   /* scale signal to lie between -0.4 and 1.2 mV */
   zmin = zts[1];
   zmax = zts[1];
   for(i=2;i<=Nts;i++)
   {
     if(zts[i] < zmin)       zmin = zts[i];
     else if(zts[i] > zmax)  zmax = zts[i];
   }
   zrange = zmax-zmin;
   for(i=1;i<=Nts;i++) zts[i] = (zts[i]-zmin)*(1.6)/zrange - 0.4;

   /* include additive uniformly distributed measurement noise */
   for(i=1;i<=Nts;i++) zts[i] += Anoise*(2.0*ran1(&rseed) - 1.0);    

   /* output ECG file */
   fp = fopen(outfile,"w");
   for(i=1;i<=Nts;i++) fprintf(fp,"%f %f %d\n",(i-1)*tstep,zts[i],(int)ipeak[i]);
   fclose(fp);


   printf("Finished ECG output\n");

freeVect(x,1,mstate);
freeVect(rr,1,Nrr);
freeVect(rrpc,1,2*Nrr);
freeVect(ti,1,5);
freeVect(ai,1,5);
freeVect(bi,1,5);
freeVect(xt,1,Nt);
freeVect(yt,1,Nt);
freeVect(zt,1,Nt);
freeVect(xts,1,Nt);
freeVect(yts,1,Nt);
freeVect(zts,1,Nt);
freeVect(ipeak,1,Nts);

/* END OF DORUN */
}

