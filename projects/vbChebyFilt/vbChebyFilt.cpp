/*
	SuperCollider real time audio synthesis system
 Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */


/*
 A chebyshev filter, adapted from S.Smith's DSP-guide
 author: Volker Böhm https://vboehm.net
 June 2018
 
 */


#include "SC_PlugIn.h"
#include <cstdio>

#if defined(__APPLE__) && !defined(SC_IPHONE)
#include <Accelerate/Accelerate.h>
#endif

#define MAXPOLES 20
#define TWOTANHALF (2 * tan(0.5))

// InterfaceTable contains pointers to functions in the host (server).
static InterfaceTable *ft;

// declare struct to hold unit generator state
struct VBChebyFilt : public Unit
{
    double		a[MAXPOLES*3/2];
    double		b[MAXPOLES*3/2];
    int			poles, mode;			// # of poles, mode: lowpass (0), hipass (1)
    double		cfreq;			// cutoff freq in Hz
    double		cf;				// normalized cutoff frequency
    double		ripple;			// amount of passband ripple in % (0 - 29%)
    double		r_sr;				// reciprocal sr
    double		*infilt, *outfilt;
    double		coeffs[MAXPOLES*5/2];
    double		xms[MAXPOLES];
    double		yms[MAXPOLES];
    int         stages[MAXPOLES*5/2];
    
};

// declare unit generator functions
static void VBChebyFilt_next(VBChebyFilt *unit, int inNumSamples);
static void VBChebyFilt_Ctor(VBChebyFilt* unit);
static void VBChebyFilt_Dtor(VBChebyFilt *unit);
static int  memalloc(VBChebyFilt* unit, int n);
void calcCoeffs(VBChebyFilt *unit);
void calc(VBChebyFilt *unit, int j, double *returns);
void set_cf(VBChebyFilt *unit, double input);
void num_poles(VBChebyFilt *unit, int input);
void clearFilt(VBChebyFilt *unit);

//////////////////////////////////////////////////////////////////

// Ctor
void VBChebyFilt_Ctor(VBChebyFilt* unit)
{
    int i;
    int blocksize = unit->mWorld->mFullRate.mBufLength;
    
    int err = memalloc(unit, blocksize);
    if(err)
        return;
    
    
    SETCALC(VBChebyFilt_next);
    
    
    // initialize the unit generator state variables.
    
    unit->cfreq = IN0(1);
    unit->mode = IN0(2);			// 0: lowpass, 1: highpass
    int poles = IN0(3);
    
    for(i=0; i<MAXPOLES; i++) {
        unit->a[i] = 0.0;
        unit->b[i] = 0.0;
        unit->xms[i] = 0.0;
        unit->yms[i] = 0.0;
    }
    
    unit->ripple = 0.5;
    unit->r_sr = 1.0 / SAMPLERATE;
    
    
    num_poles(unit, poles);
    set_cf(unit, unit->cfreq);
    

	VBChebyFilt_next(unit, 1);
    
}


int memalloc(VBChebyFilt* unit, int n) {

    unit->infilt = (double*)RTAlloc(unit->mWorld, (n+2)*sizeof(double));
    unit->outfilt = (double *)RTAlloc(unit->mWorld, (n+2)*sizeof(double));
    
    if(!unit->infilt || !unit->outfilt) {
        std::printf("VBChebyFilt: memory allocation failed!\n");
        return 1;
    }
    
    return 0;
}



static void VBChebyFilt_Dtor(VBChebyFilt *unit) {
    
    if(unit->infilt)
        RTFree(unit->mWorld, unit->infilt);
    
    if(unit->outfilt)
        RTFree(unit->mWorld, unit->outfilt);
    
    SCWorld_Allocator alloc(ft, unit->mWorld);
    
}


//////////////////////////////////////////////////////////////////


void calcCoeffs(VBChebyFilt *unit)
{
    int i, j;
    double results[5];
    double sa, sb, rgain;
    int poles2 = unit->poles/2;			// poles must be even
    
    for(j=0; j<poles2; j++) {
        calc(unit, j, results);
        
        // normalize gain
        sa = 0;
        sb = 0;
        for(i=0; i<3; i++) {
            if(unit->mode == 0) {
                sa += results[i];
            } else {
                sa += results[i]*pow(-1, i);
            }
        }
        if(unit->mode == 0)
            sb = results[3]+results[4];
        else
            sb = -results[3]+results[4];
        
        rgain = (1-sb) / sa;
        
        double val;
        for(i=0; i<3; i++) {
            val = results[i]*rgain;
            unit->coeffs[j*5+i] = val;
        }
        for(i=3; i<5; i++) {
            unit->coeffs[j*5+i] = -results[i];
        }
        
    }
    
}


void calc(VBChebyFilt *unit, int j, double *returns)
{
    double rp, ip, es, vx, kx, t, w, m, d, k, x0, x1, x2, y1, y2, tt, kk;
    rp = ip = es = vx = kx = t = w = m = d = k = x0 = x1 = x2 = y1 = y2 = 0.0;
    
    // calculate the pole location on the unit circle
    rp = -cos( pi /(unit->poles*2) + j * pi / unit->poles );
    ip = sin( pi /(unit->poles*2) + j * pi / unit->poles );
    
    // wrap from a circle to an ellipse
    if(unit->ripple > 0) {
        es = sqrt( (100.0 / (100.0 - unit->ripple)) *  (100.0 / (100.0 - unit->ripple)) -1);
        vx = (1.0/unit->poles) * log( (1.0/es) + sqrt( (1.0/(es*es))+1) );
        kx = (1.0/unit->poles) * log( (1.0/es) + sqrt( (1.0/(es*es))-1) );
        kx = (exp(kx) + exp(-kx))*0.5;
        rp = rp * ( (exp(vx) - exp(-vx)) *0.5 ) / kx;
        ip = ip * ( (exp(vx) + exp(-vx)) *0.5 ) / kx;
    }
    
    // s-domain to z-domain conversion
    t = TWOTANHALF;	//2 * tan(0.5);
    tt = t*t;
    w = twopi * unit->cf;
    m = rp*rp + ip*ip;
    d = 4 - 4*rp*t + m*tt;
    x0 = tt / d;
    x1 = 2 * x0;
    x2 = x0;
    y1 = (8 - 2*m*tt) / d;
    y2 = (-4 - 4*rp*t - m*tt) / d;
    
    
    // LP to LP, or LP to HP Transform
    if(unit->mode ==0)
        k = sin(0.5-w*0.5) / sin(0.5+w*0.5);
    else
        k = -cos(w*0.5 + 0.5) / cos(w*0.5-0.5);
    kk = k*k;
    d = 1+ y1*k - y2*kk;
    returns[0] = (x0 - x1*k + x2*kk) / d;
    returns[1] = (-2*x0*k + x1 + x1*kk - 2*x2*k) / d;
    returns[2] = (x0*kk - x1*k + x2) / d;
    returns[3] = (2*k + y1 + y1*kk - 2*y2*k) / d;
    returns[4] = ( -(kk) - y1*k + y2) / d;
    if(unit->mode ==1) {
        returns[1] = -returns[1];
        returns[3] = -returns[3];
    }
}



void set_cf(VBChebyFilt *unit, double input) {
    
    if(input>=33) {
        unit->cfreq = input;
        unit->cf = input * unit->r_sr;			// cf: normalized frequency
    }
    else {
        unit->cfreq = 33;
        unit->cf = 33 * unit->r_sr;
    }
    calcCoeffs(unit);
}


void num_poles(VBChebyFilt *unit, int input) {
    
    if(input>=2 && input<=20) {
        if(input%2==0) {		// #poles must be even
            unit->poles = input;
        } else {
            unit->poles = --input;
            std::printf("VBChebyFilt: number of poles must be even\n\tsetting poles to %d\n", unit->poles);
        }
        clearFilt(unit);
        calcCoeffs(unit);
    } else
        std::printf("VBChebyFilt: # poles out of range! setting to %d\n", MAXPOLES);
}


void clearFilt(VBChebyFilt *unit)
{
    int i;
    for(i=0; i<MAXPOLES; i++) {
        unit->xms[i] = unit->yms[i] = 0.0;
    }
}

//////////////////////////////////////////////////////////////////


void VBChebyFilt_next(VBChebyFilt *unit, int inNumSamples)
{
    float *in = IN(0);
	float *out = OUT(0);

	// freq is control rate, so calculate it once.
    float freq = IN0(1);


    double	*outfilt = unit->outfilt;
    double	*infilt = unit->infilt;
    double  *coeffs = unit->coeffs;
    double	*xms = unit->xms;
    double	*yms = unit->yms;
    int		poles2 = unit->poles >> 1; 	//each biquad can calc 2 poles (and 2 zeros)
    int     vs = inNumSamples;
    int     k;
    
    if(freq != unit->cfreq)
        set_cf(unit, freq);
    
    // convert input to double precision and copy to outfilt
#if defined(__APPLE__)
    vDSP_vspdp(in, 1, outfilt+2, 1, vs);
#else
    for(int i=0; i<vs; ++i) {
        outfilt[i+2] = in[i];
    }
#endif
    
    
    for(k=0; k<poles2; k++) {
        
        // restore last two input samps
        infilt[0] = xms[k*2];
        infilt[1] = xms[k*2+1];
        
        // copy from outfilt to infilt (recursion)
        memcpy(infilt+2, outfilt+2, vs*sizeof(double));
        
        // restore last two output samps
        outfilt[0] = yms[k*2];
        outfilt[1] = yms[k*2+1];
        
        // do the biquad!
#if defined(__APPLE__)
        vDSP_deq22D(infilt, 1, coeffs+(k*5), outfilt, 1, vs);
#else
        double a0 = coeffs[k*5];
        double a1 = coeffs[k*5+1];
        double a2 = coeffs[k*5+2];
        double b1 = coeffs[k*5+3];
        double b2 = coeffs[k*5+4];
        
        for(int i=2; i<vs+2; ++i) {
            outfilt[i] = a0 * infilt[i] + a1 * infilt[i-1] + a2 * infilt[i-2]
            - b1 * outfilt[i-1] - b2 * outfilt[i-2];
        }
        
#endif
        // save last two input & output samples for next vector
        xms[k*2] = infilt[vs];
        xms[k*2+1] = infilt[vs+1];
        yms[k*2] = outfilt[vs];
        yms[k*2+1] = outfilt[vs+1];
    }
    
    // TODO: check for denormals
    
    // copy values to out and convert to single precision
#if defined(__APPLE__)
    vDSP_vdpsp(outfilt+2, 1, out, 1, vs);
    
#else
     for(int i=0; i<vs; i++) {
         out[i] = outfilt[i+2];
     }
#endif
}


PluginLoad(VBChebyFilt)
{
	ft = inTable; // store pointer to InterfaceTable

	DefineDtorUnit(VBChebyFilt);
}

