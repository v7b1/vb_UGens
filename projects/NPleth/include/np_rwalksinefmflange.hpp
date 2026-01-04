// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once

#include "synth_waveform.hpp"
#include "mixer.hpp"
#include "synth_sine.hpp"
#include "effect_flange.h"
#include <limits>

#define FLANGE_DELAY_LENGTH (2*AUDIO_BLOCK_SAMPLES)



class RwalkSineFMFlange : public Generator {

public:

    RwalkSineFMFlange() {}

	virtual ~RwalkSineFMFlange() {}

    RwalkSineFMFlange(const RwalkSineFMFlange&) = delete;
    RwalkSineFMFlange& operator=(const RwalkSineFMFlange&) = delete;


    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        mixer.gain(0, 1.0f);
        mixer.gain(2, 1.0f);
        
        
        for (int i=0; i<4; i++) {
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            waveform_block[i].zeroAudioBlock();
            
            sinefm_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            sinefm_block[i].zeroAudioBlock();
            
            
            // init
            sine_fm[i].amplitude(1.0f);
            
            waveform[i].begin(1.0f, 333, WAVEFORM_PULSE);
            waveform[i].pulseWidth(0.2f);
            
            // random walk initial conditions
            // positions: random in [0,L] x [0, L]
            x[i] = get_rand() * kL;
            y[i] = get_rand() * kL;
        }
        
        mixer_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        refcount++;
        mixer_block.zeroAudioBlock();
        
        flange.begin(l_delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);
        

	}
    
    
    virtual void reset()
    {

        
    }
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {

        float dL = kL + 100.0f;
        float snfm = k1 * 500.0f + 10.0f;
        float mod_freq = k2 * 3.0f;
        

        // loop to "walk" randomly
        for (int i = 0; i < 4; i++) {
            
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
        
        sine_fm[0].frequency(snfm);
        sine_fm[1].frequency(snfm + 55.0f);
        sine_fm[2].frequency(snfm + 65.0f);
        sine_fm[3].frequency(snfm + 75.0f);
        
        flange.voices(s_idx, s_depth, mod_freq);
        
        for (int i=0; i<4; i++) {
            waveform[i].frequency(x[i]);
            waveform[i].update(&waveform_block[i]);
            sine_fm[i].update(&waveform_block[i], &sinefm_block[i]);
        }
        
        mixer.update(&sinefm_block[0], &sinefm_block[1], &sinefm_block[2], &sinefm_block[3], &mixer_block);
        flange.update(&mixer_block, out_block);
	}



private:
    
    AudioSynthWaveform     waveform[4];
    AudioSynthWaveformSineModulated sine_fm[4];
    
    AudioEffectFlange       flange;
    AudioMixer4             mixer;
    
    audio_block_t           waveform_block[4];
    audio_block_t           sinefm_block[4];
    audio_block_t           mixer_block;
    
    float x[4], y[4];
    
    /* memory for flange effect */
    short           l_delayline[FLANGE_DELAY_LENGTH];

    const int s_idx = 2 * FLANGE_DELAY_LENGTH / 4;
    const int s_depth = FLANGE_DELAY_LENGTH / 4;
    const float s_freq = 3.0f;
    const int kL = 600;
    const float kV_0 = 30.0f;

};
