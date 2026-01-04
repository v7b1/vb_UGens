// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once


#include "synth_waveform.hpp"


class Atari : public Generator {

public:

    Atari() {}

	virtual ~Atari() {}

    Atari(const Atari&) = delete;
    Atari& operator=(const Atari&) = delete;


    virtual void init(int16_t *mem)
    {
        waveFormMod[0].begin(WAVEFORM_SQUARE);
        waveFormMod[1].begin(WAVEFORM_PULSE);
        waveFormMod[0].amplitude(1.0f);
        waveFormMod[1].amplitude(1.0f);
        waveFormMod[0].offset(1.0f);
        

        waveform_block.data = mem;
        waveform_block.zeroAudioBlock();

	}
    
    virtual void reset()
    {

    }
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch1 = k1 * k1;
        
        waveFormMod[0].frequency(10.0f + (pitch1 * 50.0f));
        waveFormMod[1].frequency(10.0f + (k2 * 200.0f));
        waveFormMod[0].frequencyModulation(k2 * 8.0f + 3.0f);

        // xmod oscillators
        waveFormMod[0].update(out_block, nullptr, &waveform_block);
        waveFormMod[1].update(nullptr, &waveform_block, out_block);

	}



private:
    
    AudioSynthWaveformModulated waveFormMod[2];
    audio_block_t   waveform_block;

};

