// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once


#include "synth_waveform.hpp"
#include "mixer.hpp"
#include "synth_dc.hpp"


class RadioOhNo : public Generator {

public:

	RadioOhNo() {}

	virtual ~RadioOhNo() {}

	RadioOhNo(const RadioOhNo&) = delete;
	RadioOhNo& operator=(const RadioOhNo&) = delete;

    
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        for (int i=0; i<4; i++) {
            waveformMod[i].begin(0.8, 500, WAVEFORM_PULSE);
            ab[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            ab[i].zeroAudioBlock();
        }
        
        shape_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        shape_block.zeroAudioBlock();
        
        waveformMod[0].frequencyModulation(5);
        waveformMod[1].frequencyModulation(5);
        waveformMod[2].frequencyModulation(8);
        waveformMod[3].frequencyModulation(8);  // reflect typo!

	}
    
    virtual void reset()
    {
        ab[0].zeroAudioBlock();
        ab[3].zeroAudioBlock();
        
//        for (int i=0; i<4; i++) {
//            ab[i].zeroAudioBlock();
//        }
//        shape_block.zeroAudioBlock(); vb: nor needed?
    }

    
	virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch = k1 * k1;
		float knob_2 = k2;


		waveformMod[0].frequency(2500.0f * pitch + 20.0f);
		waveformMod[1].frequency(1120.0f - (1100.0f * pitch));
		waveformMod[2].frequency(2900.0f * pitch + 20.0f);
		waveformMod[3].frequency(8020.0f - (8000.0f * pitch + 20.0f));

		shape.amplitude(knob_2);
        
        shape.update(&shape_block);
        
        
        waveformMod[1].update(&ab[0], &shape_block, &ab[1]);
        waveformMod[0].update(&ab[1], &shape_block, &ab[0]);
        
        waveformMod[2].update(&ab[3], &shape_block, &ab[2]);
        waveformMod[3].update(&ab[2], &shape_block, &ab[3]);
        
        mixer.update(&ab[0], &ab[2], nullptr, nullptr, out_block);

	}


private:

	audio_block_t ab[4];
	audio_block_t shape_block;

	AudioSynthWaveformDc     shape;
	AudioSynthWaveformModulated waveformMod[4];
	AudioMixer4              mixer;

};

