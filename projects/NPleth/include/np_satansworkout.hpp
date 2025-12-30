#pragma once


#include "synth_pinknoise.hpp"
#include "synth_pwm.hpp"
#include "effect_freeverb.hpp"


class SatansWorkout : public Generator {

public:

    SatansWorkout() {}

	virtual ~SatansWorkout() {}

    SatansWorkout(const SatansWorkout&) = delete;
    SatansWorkout& operator=(const SatansWorkout&) = delete;


    virtual void init(int16_t *mem)
    {
        pink.amplitude(1);
        pwm.amplitude(1);
        freeverb.damping(-5);
        
        // clear memory
        uint16_t refcount = 0;
        for (int i=0; i<2; i++) {
            audio_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            refcount++;
            audio_block[i].zeroAudioBlock();
        }

	}
    
    virtual void reset()
    {
//        for (int i=0; i<2; i++)
//            audio_block[i].zeroAudioBlock();
    }
    
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch1 = k1 * k1;
        
        pwm.frequency(8.0f + (pitch1 * 6000.0f));
//        self->pink1->amplitude(pitch1);     // vb: this is in the manual but not in the original code!

        freeverb.roomsize(0.001f + k2 * 4.0f);
        
        
        pink.update(&audio_block[0]);
        pwm.update(&audio_block[0], &audio_block[1]);
        freeverb.update(&audio_block[1], out_block);

	}



private:
    
    AudioSynthWaveformPWM       pwm;
    AudioSynthNoisePink         pink;
    AudioEffectFreeverb         freeverb;
    
    audio_block_t               audio_block[2];

};

