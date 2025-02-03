


#include "synth_waveform.hpp"
#include "mixer.hpp"
#include "synth_dc.hpp"



class PwCluster : public Generator {

public:
    
    PwCluster() {}
    virtual ~PwCluster() {}
    
    // delete copy constructors
    PwCluster(const PwCluster&) = delete;
    PwCluster& operator=(const PwCluster&) = delete;
    
    
    virtual void init(int16_t *mem)
    {
        uint16_t refcount = 0;
        
        for (int i=0; i<6; i++) {
            waveformMod[i].begin(0.7f, 794.0f, WAVEFORM_PULSE);
        }
        
        // init mixer gains        
        for (int k=0; k<4; k++) {
            mixer.gain(k, 1.0f);
        }
        
        // clear memory
        for (int i=0; i<6; i++) {
            waveform_block[i].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
            waveform_block[i].zeroAudioBlock();
            refcount++;
            
        }
        
        
        mixer_block[0].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mixer_block[0].zeroAudioBlock();
        refcount++;
        mixer_block[1].data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        mixer_block[1].zeroAudioBlock();
        refcount++;
        dc_block.data = mem+(refcount*AUDIO_BLOCK_SAMPLES);
        dc_block.zeroAudioBlock();
        
    }
    
    
    virtual void reset()
    {
        
    }
    
    
    
    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float pitch1 = k1 * k1;
        float freq = 40 + pitch1 * 8000.0f;
        waveformMod[0].frequency(freq);
        freq *= 1.227f;
        waveformMod[1].frequency(freq);
        freq *= 1.24f;
        waveformMod[2].frequency(freq);
        freq *= 1.17f;
        waveformMod[3].frequency(freq);
        freq *= 1.2f;
        waveformMod[4].frequency(freq);
        freq *= 1.3f;
        waveformMod[5].frequency(freq);
        
        dc.amplitude(1 - k2 * 0.97);
        dc.update(&dc_block);
        

        for (int i=0; i<6; i++)
        {
            waveformMod[i].update(nullptr, &dc_block, &waveform_block[i]);
        }
            
        mixer.update(&waveform_block[0], &waveform_block[1], &waveform_block[2], &waveform_block[3], &mixer_block[0]);
        mixer.update(&waveform_block[4], &waveform_block[5], nullptr, nullptr, &mixer_block[1]);
        
        // sum output of all mixers
        mixer.update(&mixer_block[0], &mixer_block[1], nullptr, nullptr, out_block);
    }

    
private:
    
    AudioSynthWaveformModulated waveformMod[6];
    AudioMixer4                 mixer;
    AudioSynthWaveformDc        dc;

    audio_block_t   waveform_block[6];
    audio_block_t   mixer_block[2];
    audio_block_t   dc_block;
};

