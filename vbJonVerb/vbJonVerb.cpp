
#include "SC_PlugIn.h"
#include "vb.jonverb.h"
#include <float.h>

#define DIFFORDER 4
#define NOISEINJECT 32            // noise injection period in samples


// InterfaceTable contains pointers to functions in the host (server).
static InterfaceTable *ft;

// declare struct to hold unit generator state
struct VBJonVerb : public Unit
{
	double mPhase; // phase of the oscillator, from -1 to 1.
	float mFreqMul; // a constant for multiplying frequency
    double        rate;
    double        inputbandwidth;
    g_damper      *inputDamper;
    g_diffuser    **inputDiffusers;
    g_diffuser    **decayDiffusers1;
    g_diffuser    **decayDiffusers2;
    g_tapdelay    **tapdelays;
    double        fbL, fbR, decay, erfl_gain, tail_gain;
};

// declare unit generator functions
static void VBJonVerb_next_a(VBJonVerb *unit, int inNumSamples);
//static void VBJonVerb_next_k(VBJonVerb *unit, int inNumSamples);
static void VBJonVerb_Ctor(VBJonVerb *unit);
static void VBJonVerb_Dtor(VBJonVerb *unit);


//////////////////////////////////////////////////////////////////


// A Ctor usually does 3 things.
// 1. set the calculation function.
// 2. initialize the unit generator state variables.
// 3. calculate one sample of output.
static void VBJonVerb_Ctor(VBJonVerb *unit)
{
    /*
	// 1. set the calculation function.
	if (INRATE(0) == calc_FullRate) {
		// if the frequency argument is audio rate
		SETCALC(VBJonVerb_next_a);
	} else {
		// if the frequency argument is control rate (or a scalar).
		SETCALC(VBJonVerb_next_k);
	}*/
    
    SETCALC(VBJonVerb_next_a);

	// 2. initialize the unit generator state variables.
	// initialize a constant for multiplying the frequency
	unit->mFreqMul = 2.0 * SAMPLEDUR;
	// get initial phase of oscillator
	unit->mPhase = IN0(1);
    
    //fixed delay taps
    int taps0[4] = {6598, 2949, 523, 5375};
    int taps1[4] = {6249, 394, 4407, 3128};
    int taps2[4] = {5512, 1580, 3961, 0};
    int taps3[4] = {4687, 2958, 179, 0};
    
    // allpass delay taps
    int difftaps0[3] = {210, 0, 0};
    int difftaps1[3] = {159, 0, 0};
    int difftaps2[3] = {562, 0, 0};
    int difftaps3[3] = {410, 0, 0};
    int difftaps4[3] = {996, 0, 0};
    int difftaps5[3] = {1345, 0, 0};
    int difftaps6[3] = {2667, 277, 1820};
    int difftaps7[3] = {3936, 2835, 496};
    
    unit->erfl_gain = 0.5;
    unit->tail_gain = 0.2;
    unit->decay = 0.75;
    unit->inputbandwidth = 0.5;
    unit->inputDamper = damper_make(1.0-unit->inputbandwidth);
    unit->inputDiffusers = (g_diffuser **)calloc(DIFFORDER, sizeof(g_diffuser *));
    unit->inputDiffusers[0] = diffuser_make(1024, difftaps0, 0.75);
    unit->inputDiffusers[1] = diffuser_make(1024, difftaps1, 0.75);
    unit->inputDiffusers[2] = diffuser_make(1024, difftaps2, 0.625);
    unit->inputDiffusers[3] = diffuser_make(1024, difftaps3, 0.625);
    
    // first left and right diffusers in tail
    unit->decayDiffusers1 = (g_diffuser **)calloc(2, sizeof(g_diffuser *));
    unit->decayDiffusers1[0] = diffuser_make(4096, difftaps4, -0.7);
    unit->decayDiffusers1[1] = diffuser_make(4096, difftaps5, -0.7);
    
    // tapdelays in tail
    unit->tapdelays = (g_tapdelay **)calloc(4, sizeof(g_tapdelay *));
    unit->tapdelays[0] = tapdelay_make(8192, taps0);
    unit->tapdelays[1] = tapdelay_make(8192, taps1);
    unit->tapdelays[2] = tapdelay_make(8192, taps2);
    unit->tapdelays[3] = tapdelay_make(8192, taps3);
    
    // second left and right diffusers in tail
    unit->decayDiffusers2 = (g_diffuser **)calloc(2, sizeof(g_diffuser *));
    unit->decayDiffusers2[0] = diffuser_make(4096, difftaps6, -0.5);
    unit->decayDiffusers2[1] = diffuser_make(4096, difftaps7, -0.5);
    
    unit->fbL = unit->fbR = 0.;

	// 3. calculate one sample of output.
	VBJonVerb_next_a(unit, 1);
}

static void VBJonVerb_Dtor(VBJonVerb *unit)
{
    damper_free(unit->inputDamper);
    for(int i = 0; i < DIFFORDER; i++) {
        tapdelay_free(unit->tapdelays[i]);
        //damper_free(p->fdndamps[i]);
        diffuser_free(unit->inputDiffusers[i]);
    }
    diffuser_free(unit->decayDiffusers1[0] );
    diffuser_free(unit->decayDiffusers1[1] );
    diffuser_free(unit->decayDiffusers2[0] );
    diffuser_free(unit->decayDiffusers2[1] );
}


//////////////////////////////////////////////////////////////////

// The calculation function executes once per control period
// which is typically 64 samples.

// calculation function for an audio rate frequency argument
void VBJonVerb_next_a(VBJonVerb *unit, int inNumSamples)
{
	// get the pointer to the output buffer
	float *outL = OUT(0);
    float *outR = OUT(1);

	// get the pointer to the input buffer
	float *in = IN(0);
    
    int vs = inNumSamples;
    double input, sumL, sumR, fbL, fbR, fbholdL;
    double decay, erfl_gain, tail_gain, damping;
    
    
    
    decay = IN0(1);
    damping = IN0(2);
    
    erfl_gain = IN0(4);
    tail_gain = IN0(5);
    
    fbL = unit->fbL;
    fbR = unit->fbR;
    //erfl_gain = unit->erfl_gain;
    //tail_gain = unit->tail_gain;
    //tail_gain *= tail_gain;    // make it a little softer...
    //decay = unit->decay;
    sumL = sumR = 0.;
    g_damper *inputDamper = unit->inputDamper;
    g_diffuser **inputDiffs = unit->inputDiffusers;
    g_diffuser **decayDiffs1 = unit->decayDiffusers1;
    g_diffuser **decayDiffs2 = unit->decayDiffusers2;
    g_tapdelay **tapDelays = unit->tapdelays;
    
    
    
    // first two tapdelays in reverb tail have a onepole filter at the feedback output
    tapDelays[0]->damping = damping;    //CLAMP(damping, 0., 1.);
    tapDelays[1]->damping = damping;    //CLAMP(damping, 0., 1.);
    
    // kick in some noise to keep denormals away
    int i=0;
    while(i<vs) {
        in[i] += FLT_EPSILON;
        i += NOISEINJECT;
    }
    
    damper_do_block32(inputDamper, in, vs);              // input damping
    
    for(int k=0; k<DIFFORDER; k++) {                     // diffusors (allpass filters)
        diffuser_do_block32(inputDiffs[k], in, vs);
    }
    
    // tail
    for(int i=0; i<vs; i++)
    {
        sumL = sumR = 0.;
        input = in[i];
        fbL += input;        // cross channel feedback
        fbR += input;
        
        // first decay defuser (should be modulated in the end)
        fbL = diffuser_do(decayDiffs1[0], fbL);
        fbR = diffuser_do(decayDiffs1[1], fbR);
        
        // first fixed delay with tap outputs (+ freq dependent damping)
        fbL = tapdelay1_do_left(tapDelays[0], fbL, &sumL, &sumR);
        fbR = tapdelay1_do_right(tapDelays[1], fbR, &sumL, &sumR);
        
        // freq independent feedback control
        fbL *= decay;
        fbR *= decay;
        
        // second decay diffusers + tap outputs
        fbL = diffuser_do_decay(decayDiffs2[0], fbL, &sumL, &sumR);
        fbR = diffuser_do_decay(decayDiffs2[1], fbR, &sumL, &sumR);
        
        // second fixed delay with tap outputs
        fbL = tapdelay2_do_left(tapDelays[2], fbL, &sumL, &sumR);
        fbR = tapdelay2_do_right(tapDelays[3], fbR, &sumL, &sumR);
        
        
        // levels and output
        input *= erfl_gain;
        *(outL++) = sumL*tail_gain + input;
        *(outR++) = sumR*tail_gain + input;
        
        // cross channel feedback
        fbholdL = fbL;
        fbL = fbR;
        fbR = fbholdL;
    }

    unit->fbL = fbL;
    unit->fbR = fbR;
}

//////////////////////////////////////////////////////////////////

// calculation function for a control rate frequency argument
/*
void VBJonVerb_next_k(VBJonVerb *unit, int inNumSamples)
{
	// get the pointer to the output buffer
	float *out = OUT(0);

	// freq is control rate, so calculate it once.
	float freq = IN0(0) * unit->mFreqMul;

	// get phase from struct and store it in a local variable.
	// The optimizer will cause it to be loaded it into a register.
	double phase = unit->mPhase;

	// since the frequency is not changing then we can simplify the loops
	// by separating the cases of positive or negative frequencies.
	// This will make them run faster because there is less code inside the loop.
	if (freq >= 0.f) {
		// positive frequencies
		for (int i=0; i < inNumSamples; ++i)
		{
			out[i] = phase;
			phase += freq;
			if (phase >= 1.f) phase -= 2.f;
		}
	} else {
		// negative frequencies
		for (int i=0; i < inNumSamples; ++i)
		{
			out[i] = phase;
			phase += freq;
			if (phase <= -1.f) phase += 2.f;
		}
	}

	// store the phase back to the struct
	unit->mPhase = phase;
}*/


// the entry point is called by the host when the plug-in is loaded
PluginLoad(VBJonVerb)
{
    // InterfaceTable *inTable implicitly given as argument to the load function
	ft = inTable; // store pointer to InterfaceTable

	DefineSimpleUnit(VBJonVerb);
}
