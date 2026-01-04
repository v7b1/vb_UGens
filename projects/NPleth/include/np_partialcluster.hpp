// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once

#include "synth_waveform.hpp"
#include "mixer.hpp"



class PartialCluster : public Generator {

public:
    
    PartialCluster() {}
    virtual ~PartialCluster() {}
    
    // delete copy constructors
    PartialCluster(const PartialCluster&) = delete;
    PartialCluster& operator=(const PartialCluster&) = delete;
    
    
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        

        int masterWaveform = WAVEFORM_SAWTOOTH;
        float masterVolume = 0.25f;

        
        for (int i=0; i<16; i++) {
            waveform[i].begin(masterVolume, 794.0f, masterWaveform);
        }
        
        // init memory
        for (int i=0; i<4; i++) {
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            waveform_block[i].zeroAudioBlock();
            refcount++;
            
            mixer_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            mixer_block[i].zeroAudioBlock();
            refcount++;
            
            // init mixer gains
            mixer.gain(i, 1.0f);
        }
        
//        noise_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        
    }
    
    
    virtual void reset()
    {

    }
    
    
    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float knob_1 = k1;
        float knob_2 = k2;
        float pitch = knob_1 * knob_1;
        float fundamental = 50.f + pitch * 1000.0f;
        float spread = knob_2 * 1.1f + 1.01f;
        
        
//        noise.update(&noise_block);  //vb, doesn't do anything, with amplitude at zero...
        
        float f = 1.0f;
        
        for (int i=0, j=0; i<4; i++, j+=4) {
            for (int k=0; k<4; k++) {
                int indx = j + k;
                waveform[indx].frequency(f * fundamental);
                waveform[indx].update(nullptr, nullptr, &waveform_block[k]);
                f *= spread;
            }
            mixer.update(&waveform_block[0], &waveform_block[1], &waveform_block[2], &waveform_block[3], &mixer_block[i]);
            
        }
        

        // sum up
        mixer.update(&mixer_block[0], &mixer_block[1], &mixer_block[2], &mixer_block[3], out_block);
        
    }

    
private:
    
    AudioSynthWaveformModulated waveform[16];
    AudioMixer4     mixer;
    AudioSynthNoiseWhite noise;
    
    audio_block_t   waveform_block[4];
    audio_block_t   mixer_block[4];
//    audio_block_t   noise_block;
    
};

