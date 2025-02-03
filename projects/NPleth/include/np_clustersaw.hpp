


#include "synth_waveform.hpp"
#include "mixer.hpp"



class ClusterSaw : public Generator {

public:
    
    ClusterSaw() {}
    virtual ~ClusterSaw() {}
    
    // delete copy constructors
    ClusterSaw(const ClusterSaw&) = delete;
    ClusterSaw& operator=(const ClusterSaw&) = delete;
    
    
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        for (int i=0; i<16; i++) {
            waveformMod[i].begin(0.25f, 0.0f, WAVEFORM_SAWTOOTH);
        }
        
        // init mixer gains
        for (int i=0; i<5; i++) {
            for (int k=0; k<4; k++) {
                mixer[i].gain(k, 1.0f);
            }
        }
        
        // clear memory
        for (int i=0; i<4; i++) {
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            waveform_block[i].zeroAudioBlock();
            refcount++;
            
            mixer_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            mixer_block[i].zeroAudioBlock();
            refcount++;
        }
        
//        printf("---------> refcount: %d\n", refcount);
        
    }
    
    
    virtual void reset()
    {
        
    }
    
    
    
    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float pitch1 = k1 * k1;
        float pitch2 = k2 * k2;
        float multFactor = 1.01f + (pitch2 * 0.9);
        float freq = 20.0f + pitch1 * 1000.0f;
        

        for (int i=0, j=0; i<4; i++, j+=4)
        {
            for (int k=0; k<4; k++)
            {
                uint8_t indx = k + j;
                waveformMod[indx].frequency(freq);
                waveformMod[indx].update(nullptr, nullptr, &waveform_block[k]);
            
                freq *= multFactor;
            }
            
            // sum up
            mixer[i].update(&waveform_block[0], &waveform_block[1], &waveform_block[2], &waveform_block[3], &mixer_block[i]);

        }
        
        // sum output of all mixers
        mixer[4].update(&mixer_block[0], &mixer_block[1], &mixer_block[2], &mixer_block[3], out_block);
    }

    
private:
    
    AudioSynthWaveformModulated waveformMod[16];
    AudioMixer4     mixer[5];

    audio_block_t   waveform_block[4];
    audio_block_t   mixer_block[4];
};

