#pragma once


#include "effect_multiply.h"
#include "synth_sine.hpp"


class XmodRingSine : public Generator {

public:

    XmodRingSine() {}

	virtual ~XmodRingSine() {}

    XmodRingSine(const XmodRingSine&) = delete;
    XmodRingSine& operator=(const XmodRingSine&) = delete;


    virtual void init(int16_t *mem)
    {
        sine_fm1.frequency(1100);
        sine_fm2.frequency(1467);
        sine_fm1.amplitude(1);
        sine_fm2.amplitude(1);
        
        uint16_t refcount = 0;
        // clear memory
        for (int i=0; i<2; i++) {
            audio_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            audio_block[i].zeroAudioBlock();
        }

	}
    
    virtual void reset()
    {
        // clear memory
//        for (int i=0; i<2; i++) {
//            audio_block[i].zeroAudioBlock();
//        }
        
        audio_block[1].zeroAudioBlock();
    }
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch1 = k1 * k1;
		float pitch2 = k2 * k2;
        
        sine_fm1.frequency(100.0f + (pitch1 * 8000.0f));
        sine_fm2.frequency(60.0f + (pitch2 * 3000.0f));

        // xmod oscillators
        sine_fm1.update(&audio_block[1], &audio_block[0]);
        sine_fm2.update(&audio_block[0], &audio_block[1]);
        
        multiply.update(&audio_block[0], &audio_block[1], out_block);

	}



private:
    
    AudioSynthWaveformSineModulated sine_fm1;
    AudioSynthWaveformSineModulated sine_fm2;
    AudioEffectMultiply      multiply;
    
    audio_block_t   audio_block[2];

};

//REGISTER_PLUGIN(XmodRingSine);
