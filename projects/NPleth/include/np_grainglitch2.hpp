// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once


#include "effect_granular.hpp"
#include "synth_waveform.hpp"
#include "mixer.hpp"

#define GRANULAR_MEMORY_SIZE 12800  // enough for 290 ms at 44.1 kHz


class GrainGlitch2 : public Generator {

public:

    GrainGlitch2() {}

	virtual ~GrainGlitch2() {}

    GrainGlitch2(const GrainGlitch2&) = delete;
    GrainGlitch2& operator=(const GrainGlitch2&) = delete;


    virtual void init(int16_t *mem)
    {
        audio_block.data = mem;
        audio_block.zeroAudioBlock();
        
        // init
        float msec = 150.0;
        waveformMod.begin(1.0f, 1000, WAVEFORM_SQUARE);
        amp.gain(32000);
        // the Granular effect requires memory to operate
        granular.begin(granularMemory, GRANULAR_MEMORY_SIZE);
        granular.beginFreeze(msec);

	}
    
    virtual void reset()
    {
        // clear memory
        audio_block.zeroAudioBlock();
    }
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch1 = k1 * k1;
        float msec2 = 25.0f + k2 * 75.0f;
        
        waveformMod.frequencyModulation(k2 * 2.0f);
        granular.beginPitchShift(msec2);

        waveformMod.frequency(pitch1 * 5000.0f + 500.0f);
        float ratio = powf(2.0f, k2 * 6.0f - 3.0f); // 0.125 to 8.0 -- uncomment for far too much range!
        granular.setSpeed(ratio);

        
        granular.update(&audio_block, out_block);

        waveformMod.update(out_block, nullptr, &audio_block);

        amp.update(out_block);
	}



private:
    
    AudioEffectGranular     granular;
    AudioSynthWaveformModulated waveformMod;
    AudioAmplifier      amp;
    int16_t             granularMemory[GRANULAR_MEMORY_SIZE];
    
    audio_block_t       audio_block;

};

