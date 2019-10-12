/*
 *  vbUGens.cpp
 *  Plugins
 *
 *  Created by vboehm on 12.04.15.
 *  
 *
 */

#include "SC_PlugIn.h"


// InterfaceTable contains pointers to function in the host (server)
static InterfaceTable *ft;

struct VBSlide : public Unit {
	float input_up, input_down;
	float lastout, slideup, slidedown;
};

struct Lores : public Unit {
	float freq, r;
	double a1, a2, a1p, a2p, ym1, ym2;
	double fqterm, resterm, twopidsr, rcon, fcon;
};

// function declarations, exposed to C
static void VBSlide_Ctor(VBSlide *unit);
static void VBSlide_next(VBSlide *unit, int inNumSamples);

static void Lores_Ctor(Lores *unit);
static void Lores_next(Lores *unit, int inNumSamples);
static void Lores_next_unroll(Lores *unit, int inNumSamples);


//////////////////////////////////////////////////////////////////////
// VBSlide

void VBSlide_Ctor( VBSlide *unit )
{
    
	float in1 = IN0(1);
	float in2 = IN0(2);

	unit->input_up = in1;
	unit->input_down = in2;
	unit->slideup = (in1 > 1 ? (1 / in1) : 1.) ;
	unit->slidedown = (in2 > 1 ? (1 / in2) : 1.) ;
	unit->lastout = 0;
	
	SETCALC(VBSlide_next);		// tells sc synth the name of the calculation function
	VBSlide_next(unit, 1);
	
}


void VBSlide_next( VBSlide *unit, int inNumSamples)
{
	float *in = IN(0);
	float *out = OUT(0);
	
	float in_up = unit->input_up;
	float in_dwn = unit->input_down;
	float up =  unit->slideup;
	float down = unit->slidedown;
	float ym1 = unit->lastout;
	float curup, curdwn, val, env;		// sq, absval;
	
	curup = IN0(1);
	curdwn = IN0(2);
	
	
	if(in_up != curup) {
		// slide up input needs updating
		in_up = curup;
		up = (curup > 1 ? (1 / curup) : 1.) ;
	}
	if(in_dwn != curdwn) {
		in_dwn = curdwn;
		down = (curdwn > 1 ? (1 / curdwn) : 1.) ;
	}
	
	
	for(int i=0; i<inNumSamples; i++) {
		val = in[i];
		//absval = (val >= 0 ? val : -val);
		//sq = val*val;
		
		env = (ym1 < val) ? (ym1+(val-ym1)*up) : (ym1+(val-ym1)*down);
		ym1 = env;
		
		out[i] = env;
	}
	
	unit->slideup = up;
	unit->slidedown = down;
	unit->input_up = in_up;
	unit->input_down = in_dwn;
	unit->lastout = ym1;
	
}


//////////////////////////////////////////////////////////////////////
// Lores


void lores_calc(Lores* unit) {
	double resterm;
	
	// calculate filter coefficients from frequency and resonance
	resterm = exp(unit->r * 0.125) * 0.882497;
	unit->fqterm = cos(unit->twopidsr * unit->freq);
	unit->a1 = -2. * resterm * unit->fqterm;
	unit->a2 = resterm * resterm;
	unit->resterm = resterm;
}

void Lores_Ctor(Lores *unit)
{
	float in1 = IN0(1);
	float in2 = IN0(2);

	unit->freq = in1;
	unit->r = in2;
	unit->twopidsr = twopi / SAMPLERATE ;

	lores_calc(unit);
	
	unit->a1p = unit->a1;
	unit->a2p = unit->a2;
	
	SETCALC(Lores_next);		// tells sc synth the name of the calculation function
	//Lores_next(unit, 1);
	Lores_next_unroll(unit, 1);
}

void Lores_next(Lores *unit, int inNumSamples)
{
	float *in = IN(0);
	float *out = OUT(0);
	
	float freq = IN0(1);
	float resonance = IN0(2);
	
	double a1 = unit->a1,a2 = unit->a2, ym1 = unit->ym1, ym2 = unit->ym2;
    double val, scale, temp, resterm;
    
	
    // constrain resonance value
    
	if (resonance >= 1.)
		resonance = 1. - 1E-20;
	else if (resonance < 0.)
		resonance = 0.;
    
    // do we need to recompute coefficients?
    
    if (freq != unit->freq || resonance != unit->r) {
    	if (resonance != unit->r)
    		resterm = unit->resterm = exp(resonance * 0.125) * .882497;
    	else
    		resterm = unit->resterm;
    	if (freq != unit->freq)
    		unit->fqterm = cos(unit->twopidsr * freq);
    	unit->a1 = a1 = -2. * resterm * unit->fqterm;
    	unit->a2 = a2 = resterm * resterm;
    	unit->r = resonance;
    	unit->freq = freq;
    }
    
    scale = 1. + a1 + a2;
    
    // DSP loop
    
    while (inNumSamples--) {
    	val = *in++;
    	temp = ym1;
    	ym1 = scale * val - a1 * ym1 - a2 * ym2;
		
		zapgremlins(temp);
		zapgremlins(ym1);
		/*
#ifdef DENORM_WANT_FIX
		if (IS_DENORM_NAN_FLOAT(ym1)) ym1 = temp = 0;
#endif
		 */
    	ym2 = temp;
    	*out++ = ym1;
    }
    unit->ym1 = ym1;
    unit->ym2 = ym2;

}

void Lores_next_unroll(Lores *unit, int inNumSamples)
{
	float *in = IN(0);
	float *out = OUT(0);
	
	float freq = IN0(1);
	float resonance = IN0(2);
	
	double a1 = unit->a1,a2 = unit->a2, yna = unit->ym2, ynb = unit->ym1;
    double val, scale, resterm;
    
	int n = inNumSamples / 4;		// !
	
    // constrain resonance value
	if (resonance >= 1.)
		resonance = 1. - 1E-20;
	else if (resonance < 0.)
		resonance = 0.;
    
    // do we need to recompute coefficients?
    
    if (freq != unit->freq || resonance != unit->r) {
    	if (resonance != unit->r)
    		resterm = unit->resterm = exp(resonance * 0.125) * .882497;
    	else
    		resterm = unit->resterm;
    	if (freq != unit->freq)
    		unit->fqterm = cos(unit->twopidsr * freq);
    	unit->a1 = a1 = -2. * resterm * unit->fqterm;
    	unit->a2 = a2 = resterm * resterm;
    	unit->r = resonance;
    	unit->freq = freq;
    }
    
    scale = 1. + a1 + a2;
    
    // DSP loop
    
    while (inNumSamples--) {
		*out++ = yna = scale * (val= *in++) - a1 * ynb - a2 * yna;
		zapgremlins(yna);
		zapgremlins(ynb);
		
		//
		*out++ = ynb = scale * (val = *in++) - a1 * yna - a2 * ynb;
		zapgremlins(yna);
		zapgremlins(ynb);
		
		//
		*out++ = yna = scale * (val = *in++) - a1 * ynb - a2 * yna;
		zapgremlins(yna);
		zapgremlins(ynb);
		
		//
		*out++ = ynb = scale * (val = *in++) - a1 * yna - a2 * ynb;
		zapgremlins(yna);
		zapgremlins(ynb);
		
    }
    unit->ym1 = ynb;
    unit->ym2 = yna;
	
}



//////////////////////////////////////////////////////////////////////

PluginLoad(VBSlide) {
	ft = inTable;
	DefineSimpleUnit(VBSlide);
	DefineSimpleUnit(Lores);
}
