#pragma once

#include "synth_waveform.hpp"
#include "mixer.hpp"
#include "synth_sine.hpp"
#include "effect_freeverb.hpp"
#include "effect_bitcrusher.h"
//#include <random>
#include <limits>



class RwalkBitCrushPW : public Generator {

public:

    RwalkBitCrushPW() {}

	virtual ~RwalkBitCrushPW() {}

    RwalkBitCrushPW(const RwalkBitCrushPW&) = delete;
    RwalkBitCrushPW& operator=(const RwalkBitCrushPW&) = delete;

//	virtual void init(int16_t *mem, uint16_t &refcount)
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        for (int i=0; i<9; i++) {
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            waveform_block[i].zeroAudioBlock();
        }
        
        for (int i=0; i<3; i++) {
            mixer_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            mixer_block[i].zeroAudioBlock();
        }
        
        // clear memory
        for (int i=0; i<2; i++) {
            audio_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            audio_block[i].zeroAudioBlock();
        }
        
        
        for (int i=0; i<4; i++) {
            mixer[0].gain(i, 1.0f);
            mixer[1].gain(i, 1.0f);
        }
        
        int masterWaveform = WAVEFORM_PULSE;
        
        for (int i=0; i<9; i++) {
            waveform[i].pulseWidth(0.2f);
            waveform[i].begin(1.0f, 283.f, masterWaveform);
        }
        
        
        // random walk initial conditions
        for (int i = 0; i < 9; i++) {
            // positions: random in [0,L] x [0, L]
            x[i] = get_rand() * kL;
            y[i] = get_rand() * kL;
        }

	}
    
    
    virtual void reset()
    {
//        for (int i=0; i<9; i++) {
//            waveform_block[i].zeroAudioBlock();
//        }
//        
//        for (int i=0; i<3; i++) {
//            mixer_block[i].zeroAudioBlock();
//        }
//        
//        for (int i=0; i<2; i++) {
//            audio_block[i].zeroAudioBlock();
//        }
    }
    
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {

        float dL = kL + 100.0f;
        float bc_01 = k1 * 0.73f + 0.2f;
        float fv = k2;
        
        bitcrusher.bits(1);
        freeverb.roomsize(fv);
        
        
        // loop to "walk" randomly
        for (int i = 0; i < 9; i++) {
            
            float theta = M_PI * (get_rand() * (2.0) - 1.0);
            float posx = cosf(theta);
            float posy = sinf(theta);

            // Update new position
            float xn = x[i] + kV_0 * posx;
            float yin = y[i] + kV_0 * posy;

            // periodic boundary conditions
            if (xn < 50)
                xn += 10;
            else if (xn > dL)
                xn -= 10;

            if (yin < 0.01)
                yin += dL;
            else if (yin > dL)
                yin -= dL;
            
            x[i] = xn;
            y[i] = yin;
        }
        
        for (int i=0; i<9; i++) {
            waveform[i].frequency(x[i]);
            waveform[i].pulseWidth(bc_01);
            
            // process
            waveform[i].update(&waveform_block[i]);
        }

        
        // mixdown first 4 oscillators
        mixer[0].update(&waveform_block[0], &waveform_block[1], &waveform_block[2], &waveform_block[3], &mixer_block[0]);
        
        mixer[1].update(&waveform_block[4], &waveform_block[5], &waveform_block[6], &waveform_block[7], &mixer_block[1]);
        

        
        // sum two blocks and feed into bitcrusher
        mixer[2].update(&mixer_block[0], &mixer_block[1], nullptr, nullptr, &mixer_block[2]);
        bitcrusher.update(&mixer_block[2], &audio_block[0]);
        
        freeverb.update(&waveform_block[8], &audio_block[1]);

        // finally sum bitcrush and freeverb
        mixer[3].update(&audio_block[0], &audio_block[1], nullptr, nullptr, out_block);

	}



private:
    
    AudioSynthWaveform     waveform[9];
    
    AudioEffectBitcrusher   bitcrusher;
    AudioEffectFreeverb     freeverb;
    AudioMixer4             mixer[4];
    
    audio_block_t           waveform_block[9];
    audio_block_t           audio_block[2];
    audio_block_t           mixer_block[3];
    
    float x[9], y[9];
    
    const int kL = 600;
    const float kV_0 = 30.0f;

};

//REGISTER_PLUGIN(RwalkBitCrushPW);
