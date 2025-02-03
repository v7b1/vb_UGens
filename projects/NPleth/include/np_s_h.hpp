#pragma once


#include "synth_waveform.hpp"
#include "effect_freeverb.hpp"


class S_H : public Generator {

public:

    S_H() {}

	virtual ~S_H() {}

    S_H(const S_H&) = delete;
    S_H& operator=(const S_H&) = delete;


    virtual void init(int16_t *mem)
    {
        waveformMod.begin(1, 200, WAVEFORM_SAMPLE_HOLD);
        waveformMod.frequencyModulation(10);

        freeverb.damping(1);
        freeverb.roomsize(0.5);

        mixer.gain(0, 1);
        mixer.gain(1, 0);
        
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

    }
    
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        waveformMod.frequency(15 + (k1 * 6000.0f));
        mixer.gain(0, 1.0f - k2);
        mixer.gain(1, k2 * 4.0f);
        
        freeverb.roomsize(0.001f + k2 * 4.0f);
        
        
        waveformMod.update(nullptr, nullptr, &audio_block[0]);
        freeverb.update(&audio_block[0], &audio_block[1]);
        mixer.update(&audio_block[0], &audio_block[1], nullptr, nullptr, out_block);

	}



private:
    
    AudioSynthWaveformModulated       waveformMod;
    AudioEffectFreeverb         freeverb;
    AudioMixer4                 mixer;
    
    audio_block_t               audio_block[2];

};

