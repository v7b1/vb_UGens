#pragma once


#include "synth_waveform.hpp"
#include "effect_freeverb.hpp"


/** Accumulates a timer when process() is called. */
template <typename T = float>
struct TTimer {
    T time = 0.f;

    void reset() {
        time = 0.f;
    }

    /** Returns the time since last reset or initialization. */
    T process(T deltaTime) {
        time += deltaTime;
        return time;
    }

    T getTime() {
        return time;
    }
};

typedef TTimer<> Timer;

//


class BasuraTotal : public Generator {

public:

    BasuraTotal() {}

	virtual ~BasuraTotal() {}

    BasuraTotal(const BasuraTotal&) = delete;
    BasuraTotal& operator=(const BasuraTotal&) = delete;

    virtual void init(int16_t *mem)
    {
        waveformMod.begin(1.0f, 500.0f, WAVEFORM_SINE);
        freeverb.roomsize(0);
        timer.reset();
        r_sr = 1.0f / AUDIO_SAMPLE_RATE_EXACT;
        
        // allocate & clear memory
        audio_block.data = mem;
        audio_block.zeroAudioBlock();
        

	}
    
    virtual void reset()
    {
//        audio_block.zeroAudioBlock();
    }
    
    static unsigned int generateNoise() {
        // See https://en.wikipedia.org/wiki/Linear_feedback_shift_register#Galois_LFSRs
        /* initialize with any 32 bit non-zero  unsigned long value. */
        static unsigned long int lfsr = 0xfeddfaceUL; /* 32 bit init, nonzero */
        static unsigned long mask = ((unsigned long)(1UL << 31 | 1UL << 15 | 1UL << 2 | 1UL << 1));
        /* If the output bit is 1, apply toggle mask.
         * The value has 1 at bits corresponding
         * to taps, 0 elsewhere. */

        if (lfsr & 1) {
            lfsr = (lfsr >> 1) ^ mask;
            return (1);
        }
        else {
            lfsr >>= 1;
            return (0);
        }
    }
    

    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
		float pitch1 = k1 * k1;
		float pitch2 = k2 * k2;
        
        float timeMicros = timer.time * 1000000;
        if (timeMicros > 100000 * pitch2) { // Changing this value changes the frequency.
            timer.reset();

            waveformMod.begin(1.0, 500.0f, WAVEFORM_SQUARE);
            waveformMod.frequency(generateNoise() * (200.0f + (pitch1 * 5000.0f))) ;
            freeverb.roomsize(1.0f);
        }
        

        const float blockTime = r_sr * AUDIO_BLOCK_SAMPLES;
        timer.process(blockTime);

        waveformMod.update(nullptr, nullptr, &audio_block);
        freeverb.update(&audio_block, out_block);

	}



private:
    
    AudioSynthWaveformModulated waveformMod;
    AudioEffectFreeverb         freeverb;
    Timer      timer;
    float       r_sr;
    
    audio_block_t   audio_block;

};

