// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once


#include "synth_waveform.hpp"
#include "filter_variable.hpp"
#include "mixer.hpp"



class WhoKnows : public Generator {

public:
    
    WhoKnows() {}
    virtual ~WhoKnows() {}
    
    // delete copy constructors
    WhoKnows(const WhoKnows&) = delete;
    WhoKnows& operator=(const WhoKnows&) = delete;
    
    

    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        waveformMod[0].begin(1.0f, 21.0f, WAVEFORM_TRIANGLE);
        waveformMod[1].begin(1.0f, 70.0f, WAVEFORM_TRIANGLE);
        waveformMod[2].begin(1.0f, 90.0f, WAVEFORM_TRIANGLE);
        waveformMod[3].begin(1.0f, 77.0f, WAVEFORM_TRIANGLE);
        
        
        waveform.begin(1.0f, 5.0f, WAVEFORM_PULSE);
        waveform.pulseWidth(0.1f);
        
        for (int i=0; i<4; i++) {
            filter[i].frequency(1000.0f);
            filter[i].resonance(7);
            filter[i].octaveControl(7);
            mixer.gain(i, 0.8f);
        }
        
        // clear memory
        waveform_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        refcount++;
        waveform_block.zeroAudioBlock();
        dummy_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        refcount++;
        dummy_block.zeroAudioBlock();
        
        for (int i=0; i<4; i++) {
            filter_block[i].data = mem+(refcount * AUDIO_BLOCK_SAMPLES);
            refcount++;
            filter_block[i].zeroAudioBlock();
            
            mod_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            mod_block[i].zeroAudioBlock();
        }
        
    }
    
    virtual void reset()
    {

    }
    
    
    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float pitch1 = k1 * k1;
        float octaves = k2 * 6.0f + 0.3f;
        
        waveform.frequency(15.0f + (pitch1 * 500.0f));
       

        // waveform to filter
        waveform.update(&waveform_block);
        
        
       for (int i=0; i<4; i++)
       {
           filter[i].octaveControl(octaves);

           waveformMod[i].update(nullptr, nullptr, &mod_block[i]);

           // SVF needs LP, BP and HP (even if we only use one)
           // let's use the same audio_block for LP and HP outputs. we don't need them anyway
           filter[i].update(&waveform_block, &mod_block[i], &dummy_block, &filter_block[i], &dummy_block);

       }

       // sum up
       mixer.update(&filter_block[0], &filter_block[1], &filter_block[2], &filter_block[3], out_block);
    }

    
private:
    
    AudioSynthWaveform  waveform;
    AudioSynthWaveformModulated waveformMod[4];
    AudioFilterStateVariable    filter[4];
    AudioMixer4     mixer;

    audio_block_t   waveform_block;
    audio_block_t   filter_block[4];
    audio_block_t   mod_block[4];
    audio_block_t   dummy_block;
};

