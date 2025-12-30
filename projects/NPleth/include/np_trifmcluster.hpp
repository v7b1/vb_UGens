


#include "synth_waveform.hpp"
#include "filter_variable.hpp"
#include "mixer.hpp"



class TriFMcluster : public Generator {

public:
    
    TriFMcluster() {}
    virtual ~TriFMcluster() {}
    
    // delete copy constructors
    TriFMcluster(const TriFMcluster&) = delete;
    TriFMcluster& operator=(const TriFMcluster&) = delete;
    
    
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        // init mixer gains
        for (int i=0; i<3; i++) {
            for (int k=0; k<4; k++) {
                mixer[i].gain(k, 1.0f);
            }
        }
        
        for (int i=0; i<6; i++) {
            modulator[i].begin(1.0f, 1000.0f, WAVEFORM_SINE);
            waveformMod[i].begin(0.25f, 794.0f, WAVEFORM_TRIANGLE);
            
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            waveform_block[i].zeroAudioBlock();
            refcount++;
        }
        
        mod_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mod_block.zeroAudioBlock();
        refcount++;
        
        mixer_block[0].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mixer_block[0].zeroAudioBlock();
        refcount++;
        
        mixer_block[1].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mixer_block[1].zeroAudioBlock();
        
    }
    
    
    virtual void reset()
    {

    }
    
    
    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float f[6];
        float pitch1 = k1 * k1;
        float index = k2 * 0.9f + 0.1f;
        float indexFreq = 0.07f;
        
        f[0] = 300.0f + pitch1 * 8000.0f;
        f[1] = f[0] * 1.227;
        f[2] = f[1] * 1.24f;
        f[3] = f[2] * 1.17f;
        f[4] = f[3] * 1.2f;
        f[5] = f[4] * 1.3f;
        
        
        
       for (int i=0; i<6; i++)
       {
           modulator[i].amplitude(index);
           modulator[i].frequency(f[i] * indexFreq);
           waveformMod[i].frequency(f[i]);
           
           modulator[i].update(&mod_block);
           waveformMod[i].update(&mod_block, nullptr, &waveform_block[i]);

       }

        // sum up
        mixer[0].update(&waveform_block[0], &waveform_block[1], &waveform_block[2], &waveform_block[3], &mixer_block[0]);
        mixer[1].update(&waveform_block[4], &waveform_block[5], nullptr, nullptr, &mixer_block[1]);
        mixer[2].update(&mixer_block[0], &mixer_block[1], nullptr, nullptr, out_block);
    }

    
private:
    
    AudioSynthWaveform  modulator[6];
    AudioSynthWaveformModulated waveformMod[6];
    AudioMixer4     mixer[3];
    
    audio_block_t   waveform_block[6];
    audio_block_t   mod_block;
    audio_block_t   mixer_block[2];
    
};

