// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once

#include "effect_granular.hpp"
#include "synth_waveform.hpp"
#include "effect_combine.hpp"

#define GRANULAR_MEMORY_SIZE 12800  // enough for 290 ms at 44.1 kHz


class GrainGlitch : public Generator {

public:

	GrainGlitch() { }

	virtual ~GrainGlitch() { }

	GrainGlitch(const GrainGlitch&) = delete;
	GrainGlitch& operator=(const GrainGlitch&) = delete;

    
    virtual void init(int16_t *mem)
    {

		waveformMod.begin(1.0f, 1000.0f, WAVEFORM_SQUARE);

		// the Granular effect requires memory to operate
		granular.begin(granularMemory, GRANULAR_MEMORY_SIZE);

		float msec = 150.0f;
		granular.beginFreeze(msec);
        
        
        gran_block.data = mem;
        gran_block.zeroAudioBlock();
        mod_block.data = mem+(AUDIO_BLOCK_SAMPLES);
        mod_block.zeroAudioBlock();

	}
    
    virtual void reset()
    {

    }
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float pitch = k1 * k1;

		waveformMod.frequencyModulation(k2 * 2.0f);

		float msec2 = 25.0f + (k2 * 75.0f);

		granular.beginPitchShift(msec2);
		waveformMod.frequency(pitch * 5000.0f + 500.0f);

		float ratio;

		ratio = powf(2.0f, k2 * 6.0f - 3.0f);
		granular.setSpeed(ratio);
        
        
        granular.update(&mod_block, &gran_block);
        waveformMod.update(&gran_block, nullptr, &mod_block);
        combine.update(&gran_block, &mod_block, out_block);
	}




private:
	AudioEffectGranular      granular;
	AudioSynthWaveformModulated waveformMod;
	AudioEffectDigitalCombine combine;

	int16_t granularMemory[GRANULAR_MEMORY_SIZE];


	audio_block_t gran_block;
	audio_block_t mod_block;
};

