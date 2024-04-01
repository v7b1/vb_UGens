/*
 *  vbFourses.cpp
 *  Plugins
 *
 *  Created by vboehm on 26.07.17.
 *  
 *
 */


#include "SC_PlugIn.h"
#include <math.h>
#include <cstdio>


// InterfaceTable contains pointers to function in the host (server)
static InterfaceTable *ft;


#define NUMFOURSES 4
#define MAXINPUTS (NUMFOURSES * 2 + 1)

#define ONE_POLE(out, in, coeff) out += coeff * (in - out);


typedef struct {
    // this is a horse... basically a ramp generator
    double      freq;
    double		val;
    double		inc, incy;
    double		dec, decy;
    double		adder;
} t_horse;


struct VBFourses : public Unit {
	double      input_up, input_down;
	double      lastout, slideup, slidedown;
    
    double      r2_sr;
    t_horse		fourses[NUMFOURSES+2];	// four horses make a fourse...
    double		smoother;
    int         numHorses;      // actual number of 'horses' - can be smaller than 4
};


// function declarations, exposed to C
static void VBFourses_Ctor(VBFourses *unit);
static void VBFourses_next(VBFourses *unit, int inNumSamples);


//////////////////////////////////////////////////////////////////////
// VBFourses

void VBFourses_Ctor( VBFourses *unit )
{
    
    unit->r2_sr = 4.0 / SAMPLERATE;
    
    for(int i=0; i<NUMFOURSES+2; i++) {
        memset(&unit->fourses[i], 0, sizeof(t_horse));
    }

    
    double smooth = IN0(0);         // first input
    unit->smoother = 0.01 - pow(smooth,0.2)*0.01;
    

    
    int numInputs = unit->mNumInputs;
    if (numInputs > MAXINPUTS)
        numInputs = MAXINPUTS;
    
//    std::printf("\nnumInputs: %d\n", unit->mNumInputs);
    
    
    // formatting as up/dwn pairs
    for(int i=1, k=1; i<numInputs; i+=2, k++) {
        unit->fourses[k].inc = fabsf(IN0(i)) * unit->r2_sr;
        unit->fourses[k].dec = -fabsf(IN0(i+1)) * unit->r2_sr;
        
        unit->fourses[k].val = 0.0;
        
        unit->fourses[k].adder = unit->fourses[k].inc;
        
//        std::printf("up %d: %f // dwn %d: %f Hz | inc: %f | dec: %f\n", k, IN0(i), k, IN0(i+1), unit->fourses[k].inc, unit->fourses[k].dec);
    }
    
    
    unit->numHorses = (numInputs-1) / 2;
//    std::printf("numHorses: %d\n", unit->numHorses);
    
    unit->fourses[0].val = 1.;    // dummy 'horse' only used as high limit for fourses[1]
    unit->fourses[unit->numHorses+1].val = -1.; // dummy 'horse' only used as low limit
    
    
    SETCALC(VBFourses_next);
	VBFourses_next(unit, 1);
	
}


void VBFourses_next( VBFourses *unit, int inNumSamples)
{
    t_horse *fourses = unit->fourses;
    
    double  c = unit->smoother;
    double  r2_sr = unit->r2_sr;
    double  hilim = 1.0;
    double  lolim = -1.0;

    int numHorses = unit->numHorses;
    int numInputs = unit->mNumInputs;


    for(int i=1, k=1; i<numInputs; i+=2, k++) {
        fourses[k].inc = fabsf(IN0(i)) * r2_sr;
        fourses[k].dec = -fabsf(IN0(i+1)) * r2_sr;
    }
    
    
	for(int i=0; i<inNumSamples; i++) {
        
        for(int k=1; k<=numHorses; k++) {
            // smoother
            ONE_POLE(fourses[k].incy, fourses[k].inc, c);
            ONE_POLE(fourses[k].decy, fourses[k].dec, c);
            
            double val = fourses[k].val;
            val += fourses[k].adder;
            
            if(val <= fourses[k+1].val || val <= lolim ) {
                fourses[k].adder = fourses[k].incy;
            }
            else if( val >= fourses[k-1].val || val >= hilim ) {
                fourses[k].adder = fourses[k].decy;
            }
            
            OUT(k-1)[i] = val;
            
            fourses[k].val = val;
        }

	}
    
    for(int i=numHorses+1; i<=NUMFOURSES; i++) {
        memset(OUT(i-1), 0, sizeof(float)*inNumSamples);
    }
	
}




//////////////////////////////////////////////////////////////////////

PluginLoad(VBFourses) {
	ft = inTable;
	DefineSimpleUnit(VBFourses);
}
