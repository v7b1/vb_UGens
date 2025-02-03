#pragma once


#include "synth_waveform.hpp"
#include "effect_multiply.h"


class CrossModRing : public Generator {

public:

    CrossModRing() {}

	virtual ~CrossModRing() {}

    CrossModRing(const CrossModRing&) = delete;
    CrossModRing& operator=(const CrossModRing&) = delete;

    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        for (int i=0; i<4; i++) {
            waveformMod[i].begin(0.8f, 1100, WAVEFORM_SQUARE);
            
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            waveform_block[i].zeroAudioBlock();
        }
        
        waveformMod[2].begin(WAVEFORM_SAWTOOTH);
        waveformMod[2].offset(1.0f);
        
        // clear memory
        for (int i=0; i<2; i++) {
            multiply_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            multiply_block[i].zeroAudioBlock();
        }

	}
    
    virtual void reset()
    {
//        for (int i=0; i<4; i++) {
//            waveform_block[i].zeroAudioBlock();
//        }
//        for (int i=0; i<2; i++) {
//            multiply_block[i].zeroAudioBlock();
//        }
        
        waveform_block[1].zeroAudioBlock();
        waveform_block[3].zeroAudioBlock();
    }


    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch1 = k1 * k1;
        
        waveformMod[0].frequency(20.0f + (pitch1 * 807.0f));
        waveformMod[1].frequency(11.0f + (k1 * 21.0f));
        waveformMod[2].frequency(1.0f + (pitch1 * 29.0f));
        waveformMod[3].frequency(1.0f - (k1 * 7.0f));
        
        float octaves = k2 * 8.0f + 2.0f;
        
        for (int i=0; i<4; i++) {
            waveformMod[i].frequencyModulation(octaves);
        }
        
        waveformMod[2].update(&waveform_block[1], nullptr, &waveform_block[2]);
        waveformMod[0].update(&waveform_block[3], nullptr, &waveform_block[0]);
        waveformMod[3].update(&waveform_block[2], nullptr, &waveform_block[3]);
        waveformMod[1].update(&waveform_block[0], nullptr, &waveform_block[1]);
        
        
        multiply.update(&waveform_block[0], &waveform_block[1], &multiply_block[0]);
        multiply.update(&waveform_block[2], &waveform_block[3], &multiply_block[1]);
        multiply.update(&multiply_block[0], &multiply_block[1], out_block);
	}



private:
    
    AudioSynthWaveformModulated waveformMod[4];
    AudioEffectMultiply multiply;
    audio_block_t   waveform_block[4];
    audio_block_t   multiply_block[2];

};
