#pragma once


#include "synth_waveform.hpp"
#include "synth_sine.hpp"
#include "synth_dc.hpp"
#include "synth_whitenoise.hpp"
#include "effect_wavefolder.hpp"
#include "filter_variable.hpp"


class ResoNoise : public Generator {

public:

    ResoNoise() {}

	virtual ~ResoNoise() {}

    ResoNoise(const ResoNoise&) = delete;
    ResoNoise& operator=(const ResoNoise&) = delete;

    virtual void init(int16_t *mem)
    {
        waveformMod.begin(1.0f, 500.0f, WAVEFORM_SQUARE);
        sine_fm.frequency(1100);
        sine_fm.amplitude(1);
        
        noise.amplitude(1.0f);
        
        filter.resonance(3);
        filter.octaveControl(5);
        
        uint16_t refcount = 0;
        // clear memory
        for (int i=0; i<5; i++) {
            audio_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            audio_block[i].zeroAudioBlock();
        }
        dummy.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        dummy.zeroAudioBlock();

	}
    
    virtual void reset()
    {
        // clear memory
//        for (int i=0; i<5; i++) {
//            audio_block[i].zeroAudioBlock();
//        }
//        dummy.zeroAudioBlock();
    }
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch1 = k1 * k1;
        
        dc.amplitude(k2 * 0.2f + 0.03f, 1);
        
        sine_fm.frequency(20.0f + (pitch1 * 10000.0f));
        waveformMod.frequency(20.0f + (pitch1 * 7777.0f));
        
        // audio_blocks
//        [0]: waveformMod1Block
//        [1]: sineFMBlock
//        [2]: dcBlock
//        [3]: wavefolderBlock
//        [4]: noiseBlock
//        [5]: filterOutLP
//        [6]: filterOutBP
//        [7]: filterOutHP
        
        waveformMod.update(nullptr, nullptr, &audio_block[0]);
        sine_fm.update(&audio_block[0], &audio_block[1]);

        dc.update(&audio_block[2]);
        wavefolder.update(&audio_block[1], &audio_block[2], &audio_block[3]);

        noise.update(&audio_block[4]);
        filter.update(&audio_block[4], &audio_block[3], out_block, &dummy, &dummy);
        

	}



private:
    
    AudioSynthWaveformModulated waveformMod;
    AudioSynthNoiseWhite noise;
    AudioSynthWaveformSineModulated sine_fm;
    AudioSynthWaveformDc dc;
    AudioEffectWaveFolder wavefolder;
    AudioFilterStateVariable filter;
    
    audio_block_t   audio_block[5];
    audio_block_t   dummy;


};
