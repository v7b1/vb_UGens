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

typedef struct {
    // this is a horse... basically a ramp generator
    double      freq;
    double		val;
    double		inc;
    double		dec;
    double		adder;
    double		incy, incym1;		// used for smoothing
    double		decy, decym1;		// used for smoothing
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
    
    std::printf("size_of_horse: %ld\n", sizeof(unit->fourses[0]));
    
    for(int i=0; i<NUMFOURSES+2; i++) {
        memset(&unit->fourses[i], 0, sizeof(unit->fourses[0]));
    }

    
    double smooth = IN0(0); // first input
    unit->smoother = 0.01 - pow(smooth,0.2)*0.01;
    std::printf("smoother: %f\n", unit->smoother);
    std::printf("\nnumInputs: %d\n", unit->mNumInputs);
    
    // noch nicht ganz gut
    int numPairs = unit->mNumInputs / 2;
    if(numPairs > NUMFOURSES) numPairs = NUMFOURSES;
    
//    for(int i=1; i<=numPairs; i++) {
//
//        unit->fourses[i].val = 0.0;
//        unit->fourses[i].inc = fabsf(IN0(i)) * unit->r2_sr;
//        unit->fourses[i].dec = -fabsf(IN0(i+4)) * unit->r2_sr;
//        unit->fourses[i].incym1 = 0.0;
//        unit->fourses[i].decym1 = 0.0;
//        std::printf("upfreq %d: %f // downfreq %d: %f\n", i, IN0(i), i, IN0(i+4));
//
//        unit->fourses[i].adder = unit->fourses[i].inc;
//    }
    
    
    
    int numInputs = unit->mNumInputs;
    int maxInputs = NUMFOURSES*2 + 1;
    if (numInputs > maxInputs)
        numInputs = maxInputs;
    
    // formatting as up/dwn pairs
    for(int i=1, k=1; i<numInputs; i+=2, k++) {
        unit->fourses[k].inc = fabsf(IN0(i)) * unit->r2_sr;
        unit->fourses[k].dec = -fabsf(IN0(i+1)) * unit->r2_sr;
        
        unit->fourses[k].val = 0.0;
        unit->fourses[k].incym1 = 0.0;
        unit->fourses[k].decym1 = 0.0;
        
        unit->fourses[k].adder = unit->fourses[k].inc;
        
        std::printf("up %d: %f // dwn %d: %f Hz | inc: %f | dec: %f\n", k, IN0(i), k, IN0(i+1), unit->fourses[k].inc, unit->fourses[k].dec);
    }
    
    
    unit->numHorses = (numInputs-1) / 2;
    std::printf("numHorses: %d\n", unit->numHorses);
    
    unit->fourses[0].val = 1.;    // dummy 'horse' only used as high limit for fourses[1]
    unit->fourses[5].val = -1.;    // dummy 'horse' only used as low limit for fourses[4]
    unit->fourses[unit->numHorses+1].val = -1.; // in case we got less
    
    std::printf("-------- info:\n");
    for(int i=1; i<=NUMFOURSES; i++) {
        std::printf("inc %d: %f | dec %d: %f\n", i, unit->fourses[i].inc, i, unit->fourses[i].dec);
    }
    
    
    
    
	
    SETCALC(VBFourses_next);
	VBFourses_next(unit, 1);
	
}


void VBFourses_next( VBFourses *unit, int inNumSamples)
{
    t_horse     *fourses = unit->fourses;
    double      val, c, r2_sr, hilim, lolim;
    
    c = unit->smoother;
    r2_sr = unit->r2_sr;
    hilim = fourses[0].val;
    lolim = fourses[5].val;

    // old
//    for(int i=1; i<=NUMFOURSES; i++) {
//        fourses[i].inc = fabsf(IN0(i)) * unit->r2_sr;
//        fourses[i].dec = -fabsf(IN0(i+4)) * unit->r2_sr;
//    }
    
    int num = unit->mNumInputs;
    if(num > NUMFOURSES+1)
        num = NUMFOURSES;
    
    // formatting as up/dwn pairs
    for(int i=1; i<num; i+=2) {
        fourses[i].inc = fabsf(IN0(i)) * unit->r2_sr;
        fourses[i].dec = -fabsf(IN0(i+1)) * unit->r2_sr;
    }
    
    int numHorses = unit->numHorses;
    
	for(int i=0; i<inNumSamples; i++) {
        
        for(int n=1; n<=numHorses; n++) {
            // smoother
            fourses[n].incy = fourses[n].inc*c + fourses[n].incym1*(1-c);
            fourses[n].incym1 = fourses[n].incy;
            
            fourses[n].decy = fourses[n].dec*c + fourses[n].decym1*(1-c);
            fourses[n].decym1 = fourses[n].decy;
            
            val = fourses[n].val;
            val += fourses[n].adder;
            
            if(val <= fourses[n+1].val || val <= lolim ) {
                fourses[n].adder = fourses[n].incy;
            }
            else if( val >= fourses[n-1].val || val >= hilim ) {
                fourses[n].adder = fourses[n].decy;
            }
            
            OUT(n-1)[i] = val;
            
            fourses[n].val = val;
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
