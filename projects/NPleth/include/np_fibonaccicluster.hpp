// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once


#include "synth_waveform.hpp"
#include "synth_whitenoise.hpp"
#include "mixer.hpp"



class FibonacciCluster : public Generator {

public:
    
    FibonacciCluster() {}
    virtual ~FibonacciCluster() {}
    
    // delete copy constructors
    FibonacciCluster(const FibonacciCluster&) = delete;
    FibonacciCluster& operator=(const FibonacciCluster&) = delete;
    
    
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        for (int i=0; i<16; i++) {
            waveformMod[i].begin(0.2f, 794.0f, WAVEFORM_SAWTOOTH);
        }
        
//        noise.amplitude(0.0f);
        
        for (int i=0; i<5; i++) {
            for (int k=0; k<4; k++) {
                mixer[i].gain(k, 1.0f);
            }
        }
        
        // clear memory
        for (int i=0; i<4; i++) {
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            waveform_block[i].zeroAudioBlock();
            refcount++;
            
            mixer_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            mixer_block[i].zeroAudioBlock();
            refcount++;
        }
        
//        noise_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
//        noise_block.zeroAudioBlock();
        
    }
    
    
    virtual void reset()
    {
//        noise_block.zeroAudioBlock();
    }
    
    
    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float pitch1 = k1 * k1;
        float pitch2 = k2 * k2;
        float multFactor = 0.1f + (pitch2 * 0.5);
        float freq[16] = {};
        
        
        freq[0] = 40.0f + pitch1 * 5000.0f;
        freq[1] = freq[0] * (2.0f * multFactor + 1.0f);
        
       
        
        for (int i=2; i<16; i++)
        {
            freq[i] = freq[i-2] + freq[i-1] * multFactor;
        }
        
//        noise.update(&noise_block);
        
        for (int i=0, j=0; i<4; i++, j+=4)
        {
            for (int k=0; k<4; k++)
            {
                uint8_t indx = k + j;
                waveformMod[indx].frequency(freq[indx]);
//                waveformMod[indx].update(&noise_block, nullptr, &waveform_block[k]);
                waveformMod[indx].update(nullptr, nullptr, &waveform_block[k]);
            }
            
            // sum up
            mixer[i].update(&waveform_block[0], &waveform_block[1], &waveform_block[2], &waveform_block[3], &mixer_block[i]);

        }
        
        // sum output of all mixers
        mixer[4].update(&mixer_block[0], &mixer_block[1], &mixer_block[2], &mixer_block[3], out_block);
        
    }

    
private:
    
    AudioSynthWaveformModulated waveformMod[16];
    AudioSynthNoiseWhite    noise;
    AudioMixer4             mixer[5];

    audio_block_t           waveform_block[4];
    audio_block_t           mixer_block[4];
//    audio_block_t           noise_block;
};

