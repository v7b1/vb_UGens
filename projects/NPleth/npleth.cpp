
/**
    NPleth
    some noise algorithms ported form Befaco's Noise Plethora (for eurorack)
 
    volker böhm, 2025
 
 **/

/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SC_PlugIn.h"
#include <cstdio>

#include "audio_core.hpp"

#include "generator.h"
#include "np_xmodringsine.hpp"
#include "np_whoknows.hpp"
#include "np_radioOhNo.hpp"
#include "np_basuratotal.hpp"
#include "np_crossmodring.hpp"
#include "np_rwalkbitcrushpw.hpp"
#include "np_rwalksinefmflange.hpp"
#include "np_satansworkout.hpp"
#include "np_grainglitch2.hpp"
#include "np_grainglitch3.hpp"
#include "np_existenceispain.hpp"
#include "np_resonoise.hpp"
#include "np_clustersaw.hpp"
#include "np_trifmcluster.hpp"
#include "np_fibonaccicluster.hpp"
#include "np_pwcluster.hpp"
#include "np_atari.hpp"
#include "np_s_h.hpp"

#include "filter.h"

#define CLAMP(a, lo, hi) ( (a)>(lo)?( (a)<(hi)?(a):(hi) ):(lo) )


#define AUDIO_BLOCK_SAMPLES 128
#define AMP_SCALE (1.0f / (1 << 15))

#define MAX_GENERATORS 18


static InterfaceTable *ft;



struct NPleth : public Unit
{
    audio_block_t   ab;
    Generator       *generator_[MAX_GENERATORS];
    
    uint16_t        count, prev_generator;
    float           sr, r_sr;
    
    int16_t         *mem_pool;
    
    StateVariableFilter2ndOrder *svf2;
    float          prev_cf;
    float          prev_res;
    
    DCBlocker       *blocker;
    Saturator       *satur;
    FilterMode      filter_mode;
    
    // feedback option
    float          prev_output;
    float          fb1, fb2, fb3;
    
};


static void NPleth_next(NPleth *unit, int inNumSamples);
static void NPleth_Ctor(NPleth *unit);
static void NPleth_Dtor(NPleth *unit);


static void NPleth_Ctor(NPleth *unit)
{
    unit->sr = SAMPLERATE;
    unit->r_sr = 1.0 / SAMPLERATE;
    
    // mem alloc
    unit->mem_pool = (int16_t *)calloc(30 * AUDIO_BLOCK_SAMPLES, sizeof(int16_t));
    unit->ab.data = (int16_t *)calloc(AUDIO_BLOCK_SAMPLES, sizeof(int16_t));

    
    unit->generator_[0] = new RadioOhNo;
    unit->generator_[1] = new RwalkSineFMFlange;
    unit->generator_[2] = new XmodRingSine;
    unit->generator_[3] = new CrossModRing;
    unit->generator_[4] = new ResoNoise;
    unit->generator_[5] = new GrainGlitch2;
    unit->generator_[6] = new GrainGlitch3;
    unit->generator_[7] = new BasuraTotal;
    unit->generator_[8] = new ExistenceIsPain;
    unit->generator_[9] = new WhoKnows;
    unit->generator_[10] = new SatansWorkout;
    unit->generator_[11] = new RwalkBitCrushPW;
    unit->generator_[12] = new ClusterSaw;
    unit->generator_[13] = new TriFMcluster;
    unit->generator_[14] = new FibonacciCluster;
    unit->generator_[15] = new PwCluster;
    unit->generator_[16] = new Atari;
    unit->generator_[17] = new S_H;
    
    
    for (int i=0; i<MAX_GENERATORS; i++) {
        unit->generator_[i]->init(unit->mem_pool);
    }
    
    unit->prev_generator = 0;
    unit->fb1 = unit->fb2 = unit->fb3 = 0.0f;
    unit->prev_output = 0.0f;
    unit->count = 0;
    unit->prev_cf = -1.f;
    unit->prev_res = -1.f;
    
    audio_block_t *ab = &unit->ab;
    ab->zeroAudioBlock();
    
    uint16_t gen = IN0(0);
    if (gen < 0) gen = 0;
    else if (gen >= MAX_GENERATORS) gen = MAX_GENERATORS-1;
    unit->generator_[gen]->reset();
    unit->generator_[gen]->process(IN0(1), IN0(2), ab);
    

    unit->filter_mode = LOWPASS;
    unit->svf2 = new StateVariableFilter2ndOrder;
    
    unit->satur = new Saturator;
    unit->blocker = new DCBlocker;
    unit->blocker->init(unit->sr);

    
    SETCALC(NPleth_next);
    
}


static void NPleth_Dtor(NPleth *unit)
{
    for (int i=0; i<MAX_GENERATORS; i++)
        free(unit->generator_[i]);
    
    free(unit->mem_pool);
    free(unit->ab.data);
    free(unit->svf2);
    free(unit->blocker);
    free(unit->satur);
}



// input range: 0..1
float calc_cf(float f)
{
    // TODO: scaling from VCV Rack, pretty extreme?
//    float cf = f * 11.0f - 5.5f;
//    cf = powf(2.0f, cf) * 261.625565f;
//    return cf;
    
    // one octave higher
    float cf = f * 10.0f - 5.0f;
    cf = powf(2.0f, cf) * 523.251131;
    return cf;
}



void NPleth_next(NPleth *unit, int inNumSamples)
{
    float       *out = OUT(0);
    
    uint16_t    my_generator = IN0(0);
	float       knob_1 = IN0(1);        // knob 1 (0..1)
    float       knob_2 = IN0(2);        // knob 2 (0..1)
	float       cf = IN0(3);            // filter cut off (0..1)
    float       res = IN0(4);           // filter resonance (0..1)
    
    uint16_t    vs = inNumSamples;
    
    uint16_t count = unit->count;
    audio_block_t *ab = &unit->ab;
    StateVariableFilter2ndOrder *svf2 = unit->svf2;
    DCBlocker *blocker = unit->blocker;
    Saturator *satur = unit->satur;
    FilterMode filter_mode = unit->filter_mode;
    float   fb1, fb2, fb3, prev_output;
    
    fb1 = unit->fb1;
    fb2 = unit->fb2;
    fb3 = unit->fb3;
    prev_output = unit->prev_output;
    
    
    
    // clipping / clamping
    // knob params and res should be between 0..1 -- change cf also?
    CLAMP(knob_1, 0.0f, 1.0f);
    CLAMP(knob_2, 0.0f, 1.0f);
    CLAMP(cf, 0.0f, 1.0f);
    CLAMP(res, 0.0f, 1.0f);
    
    
    if (cf != unit->prev_cf || res != unit->prev_res)
    {
        unit->prev_cf = cf;
        unit->prev_res = res;
        
        float fc = calc_cf(cf) * unit->r_sr; // normalized cutoff freq
//        float fc = cf * unit->r_sr; // normalized cutoff freq
        float q = res * res * 10.0f + 0.707107f;
        svf2->setParameters(fc, q);
    }
    
    
    
    if (vs <= AUDIO_BLOCK_SAMPLES)
    {
    
        for (int i=0; i<vs; i++)
        {
            float noise_out = ab->data[count + i] * AMP_SCALE;
            svf2->process(noise_out);
            float filtered = svf2->output(filter_mode);
            float dcblocked = blocker->process(filtered);
            out[i] = satur->process(dcblocked);
        }
        // safe last output for feedback
        prev_output = out[0];
        
        // TODO: check if we want this!
//        if (fb3 != 0.0f) {
//            // set filter
//            cf = CLAMP(cf + fb3*prev_output, 0.0f, 1.0f);
//            double fc = calc_cf(cf) * self->r_sr; // normalized cutoff freq
//            double q = res * res * 10. + 0.707107;
//            svf2->setParameters(fc, q);
//        }
        
        count += vs;
        if (count >= AUDIO_BLOCK_SAMPLES)
        {
            count = 0;
            
            if (my_generator < 0) my_generator = 0;
            else if (my_generator >= MAX_GENERATORS) my_generator = MAX_GENERATORS-1;

            if (my_generator != unit->prev_generator) {
                unit->prev_generator = my_generator;
                unit->generator_[my_generator]->reset();
            }
            
            // add feedback
//            knob_1 = CLAMP(knob_1 + fb1*prev_output, 0.0f, 1.0f);
//            knob_2 = CLAMP(knob_2 + fb2*prev_output, 0.0f, 1.0f);
            
            unit->generator_[my_generator]->process(knob_1, knob_2, ab);
            
        }
        
        unit->count = count;
    }
    
    else
    {
        if (my_generator < 0) my_generator = 0;
        else if (my_generator >= MAX_GENERATORS) my_generator = MAX_GENERATORS-1;
        
        if (my_generator != unit->prev_generator) {
            unit->prev_generator = my_generator;
            unit->generator_[my_generator]->reset();
        }
        
        for (int k=0; k<vs; k+=AUDIO_BLOCK_SAMPLES)
        {
            // add feedback
//            knob_1 = CLAMP(knob_1 + fb1*prev_output, 0.0f, 1.0f);
//            knob_2 = CLAMP(knob_2 + fb2*prev_output, 0.0f, 1.0f);
            
            unit->generator_[my_generator]->process(knob_1, knob_2, ab);
            
            for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++)
            {
                float noise_out = ab->data[i] * AMP_SCALE;
                svf2->process(noise_out);
                float filtered = svf2->output(filter_mode);
                float dcblocked = blocker->process(filtered);
                out[i+k] = satur->process(dcblocked);
            }
            // safe last output for feedback
            prev_output = out[k];
            
//            if (fb3 != 0.0f) {
//                cf = CLAMP(cf + fb3*prev_output, 0.0f, 1.0f);
//                float fc = calc_cf(cf) * unit->r_sr; // normalized cutoff freq
//                float q = res * res * 10.0f + 0.707107f;
//                svf2->setParameters(fc, q);
//            }
        }
        
    }


}



PluginLoad(NPleth)
{
    ft = inTable;
    DefineDtorUnit(NPleth);
}
