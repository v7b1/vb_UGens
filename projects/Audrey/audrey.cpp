
// audrey~ a feedback synth engine
//
// porting Audrey_II by Synthux Academy to supercollider
// vboehm, 2026


#include "SC_PlugIn.h"
#include <cstdio>
#include "FeedbackSynthEngine.h"


using namespace daisysp;


#define BLOCKSIZE 4

// original samplerate and blocksize
//static const auto kSampleRate = SaiHandle::Config::SampleRate::SAI_48KHZ;
//static const size_t kBlockSize = 4;


static InterfaceTable *ft;

struct Audrey : public Unit
{
    infrasonic::FeedbackSynth::Engine *engine;
    Limiter limiter[2];
    double  sr;

    float   string_pitch;
    float   string_freq;
//    float   detune;
    float   fb_gain;
    float   reverb_mix, reverb_mix_target;
    float   hpf_, lpf_;
    float   echo_time, echo_scalar;
    float   drive_;
    
    float   *silence;
    
};


static void Audrey_next(Audrey *unit, int inNumSamples);
static void Audrey_Ctor(Audrey *unit);
static void Audrey_Dtor(Audrey *unit);


static void Audrey_Ctor(Audrey *unit)
{
    unit->sr = SAMPLERATE;

    unit->echo_scalar = 0.5f;
    unit->echo_time = 1.0f;
    
    unit->engine = new infrasonic::FeedbackSynth::Engine;
    unit->engine->Init(unit->sr);
    
    unit->limiter[0].Init();
    unit->limiter[1].Init();
    
    unit->string_pitch = 40.f;
//    unit->detune = 0.f;
    unit->hpf_ = 100.f;
    unit->lpf_ = 8000.f;
    unit->fb_gain = -18.0f;
    unit->reverb_mix_target = 0.1f;
    unit->drive_ = 0.4f;
    
    unit->engine->SetOutputLevel(0.5f);
    
    unit->engine->SetDamping(0.8);
    unit->engine->SetDecayRate(0.8);
    unit->engine->SetDriveAmount(unit->drive_);
    unit->engine->SetFeedbackHPFCutoff(unit->hpf_);
    unit->engine->SetFeedbackLPFCutoff(unit->lpf_);
    
    unit->engine->SetEchoDelayTime(unit->echo_time * unit->echo_scalar);
    unit->engine->SetEchoDelaySendAmount(0.5f);
    unit->engine->SetEchoDelayFeedback(0.8f);
    
    // TODO: how do I find the current blocksize? --> BUFLENGTH ?
    unit->silence = (float*)RTAlloc(unit->mWorld, BUFLENGTH*sizeof(float));
    memset(unit->silence, 0, BUFLENGTH*sizeof(float));
    
    SETCALC(Audrey_next);
    
}


static void Audrey_Dtor(Audrey *unit)
{
    delete unit->engine;
    
    if(unit->silence)
        RTFree(unit->mWorld, unit->silence);
}



void Audrey_next(Audrey *unit, int inNumSamples)
{
    float       *in1 = IN(0);        // audio in left
    float       *in2 = IN(1);        // audio in right
    float       *freq_in = IN(2);    // freq input
    float       *gain_in = IN(3);
    float       *delay_time = IN(4);        // body
    float       hpf_target = IN0(5);
    float       lpf_target = IN0(6);
    float       reverb_mix_in = IN0(7);
    float       reverb_decay_in = IN0(8);
    float       drive_in = IN0(9);
    float       echo_send = IN0(10);
    float       echo_time = IN0(11);
    float       echo_fb = IN0(12);

    float       *out1 = OUT(0);
    float       *out2 = OUT(1);
    

    long vs = inNumSamples;
    infrasonic::FeedbackSynth::Engine *engine = unit->engine;
    
    float interpol_coef = 100.f * BLOCKSIZE / unit->sr;
    
    if (INRATE(0) != calc_FullRate)
        in1 = unit->silence;
    if (INRATE(1) != calc_FullRate)
        in2 = unit->silence;
    
    float string_freq_target;
    float string_freq = unit->string_freq;
    float fb_gain = unit->fb_gain;
    float reverb_mix = unit->reverb_mix;
    float hpf = unit->hpf_;
    float lpf = unit->lpf_;
//    float detune = unit->detune;
//    float detune_target = unit->detune_target;
    
    
    engine->SetFeedbackDelay(delay_time[0] * 0.25);
    
    
    if (hpf_target != hpf) {
        fonepole(hpf, hpf_target, interpol_coef);
        engine->SetFeedbackHPFCutoff(hpf);
    }
    if (lpf_target != lpf) {
        fonepole(lpf, lpf_target, interpol_coef);
        engine->SetFeedbackLPFCutoff(lpf);
    }
    
    
    if (drive_in != unit->drive_) {
        engine->SetDriveAmount(drive_in);
        unit->drive_ = drive_in;
    }
    
    // reverb ------------
    float mixf = DSY_CLAMP(reverb_mix_in, 0., 1.);
    float reverb_mix_target = mixf * mixf;
    
    // initial: 0.2f, min: 0.2f, max: 1.0f,
    float decayf = infrasonic::ftension(reverb_decay_in, -3.0f);
    decayf = fmap(decayf, 0.2f, 1.0f);
    engine->SetReverbFeedback(decayf);
    
    fonepole(reverb_mix, reverb_mix_target, interpol_coef * 0.2f);
    engine->SetReverbMix(reverb_mix);
    
    // echo --------------
    engine->SetEchoDelaySendAmount(DSY_CLAMP(echo_send, 0.f, 1.f));
    engine->SetEchoDelayTime(DSY_CLAMP(echo_time, 0.f, 1.f));
    engine->SetEchoDelayFeedback(DSY_CLAMP(echo_fb, 0.f, 1.f));
    
    
    for (size_t i=0; i<vs; i+=BLOCKSIZE)
    {
        float fb_gain_target = (gain_in[0] * 84.0f) - 72.0f;
        fb_gain_target = DSY_CLAMP(fb_gain_target, -72.f, 12.f);
        if (fb_gain_target <= -72.0f) fb_gain_target = -120.0f;
        

        // smooth params
//        fonepole(detune, detune_target, interpol_coef * 0.1f);
//        engine->SetDetune(detune);
        
        if (INRATE(2) == calc_FullRate)
            string_freq_target = freq_in[i];
        else
            string_freq_target = freq_in[0];
        
        fonepole(string_freq, string_freq_target, interpol_coef * 0.1f);
        engine->SetStringFreq(string_freq);
        
        fonepole(fb_gain, fb_gain_target, interpol_coef);
        engine->SetFeedbackGain(fb_gain);
        
        for (size_t k=0; k<BLOCKSIZE; k++)
        {
            uint8_t idx = i + k;
            float outL, outR;
            engine->Process(in1[idx], in2[idx], outL, outR);
            
            out1[idx] = outL;
            out2[idx] = outR;
        }
        
    }
    
    unit->limiter[0].ProcessBlock(out1, vs, 0.7f);
    unit->limiter[1].ProcessBlock(out2, vs, 0.7f);
    
    unit->fb_gain = fb_gain;
    unit->string_freq = string_freq;
    unit->reverb_mix = reverb_mix;
    unit->hpf_ = hpf;
    unit->lpf_ = lpf;
//    unit->detune = detune;
}



PluginLoad(Audrey)
{
    ft = inTable;
    DefineDtorUnit(Audrey);
}

