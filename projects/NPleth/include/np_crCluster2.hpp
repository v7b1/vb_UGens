// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once


#include "synth_waveform.hpp"
#include "mixer.hpp"



class CrCluster2 : public Generator {

public:
    
    CrCluster2() {}
    virtual ~CrCluster2() {}
    
    // delete copy constructors
    CrCluster2(const CrCluster2&) = delete;
    CrCluster2& operator=(const CrCluster2&) = delete;
    
    
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        for (int i=0; i<6; i++) {
            waveformMod[i].begin(0.2f, 794.0f, WAVEFORM_SINE);
        }
        
        modulator.begin(1, 1000, WAVEFORM_SINE);
        
        // init mixer gains        
        for (int k=0; k<4; k++) {
            mixer.gain(k, 1.0f);
        }
        
        // clear memory
        for (int i=0; i<6; i++) {
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            waveform_block[i].zeroAudioBlock();
            refcount++;
            
        }
        
        
        mixer_block[0].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mixer_block[0].zeroAudioBlock();
        refcount++;
        mixer_block[1].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mixer_block[1].zeroAudioBlock();
        refcount++;
        mod_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mod_block.zeroAudioBlock();
        
    }
    
    
    virtual void reset()
    {
        
    }
    
    
    
    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float pitch1 = k1 * k1;
        float f1 = 40 + pitch1 * 8000.0f;
        
        float freq = f1;
        waveformMod[0].frequency(freq);
        freq *= 1.227f;
        waveformMod[1].frequency(freq);
        freq *= 1.24f;
        waveformMod[2].frequency(freq);
        freq *= 1.17f;
        waveformMod[3].frequency(freq);
        freq *= 1.2f;
        waveformMod[4].frequency(freq);
        freq *= 1.3f;
        waveformMod[5].frequency(freq);
        
        modulator.amplitude(k2);
        modulator.frequency(f1 * 2.7f);
        
        
        modulator.update(&mod_block);

        for (int i=0; i<6; i++) {
            waveformMod[i].update(&mod_block, nullptr, &waveform_block[i]);
        }
            
        mixer.update(&waveform_block[0], &waveform_block[1], &waveform_block[2], &waveform_block[3], &mixer_block[0]);
        mixer.update(&waveform_block[4], &waveform_block[5], nullptr, nullptr, &mixer_block[1]);
        
        // sum output of all mixers
        mixer.update(&mixer_block[0], &mixer_block[1], nullptr, nullptr, out_block);
    }

    
private:
    
    AudioSynthWaveformModulated waveformMod[6];
    AudioSynthWaveform       modulator;
    AudioMixer4                 mixer;

    audio_block_t   waveform_block[6];
    audio_block_t   mod_block;
    audio_block_t   mixer_block[2];

};

