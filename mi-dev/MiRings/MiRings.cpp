
#include "SC_PlugIn.h"

#include "rings/dsp/part.h"
#include "rings/dsp/strummer.h"
#include "rings/dsp/string_synth_part.h"
#include "rings/dsp/dsp.h"
#include "TriggerInput.h"


//float kSampleRate = 48000.0;
int kBlockSize = rings::kMaxBlockSize;
//float a0 = (440.0f / 8.0f) / kSampleRate;

static InterfaceTable *ft;


struct MiRings : public Unit {
    
    rings::Part             part; //*part;
    //rings::StringSynthPart  *string_synth;
    rings::StringSynthPart  string_synth;
    rings::Strummer         *strummer;
    //rings::ReadInputs       read_inputs;
    
    double                  in_level;
    
    uint16_t                *reverb_buffer;
    
    rings::PerformanceState performance_state;
    rings::Patch            patch;
    
    TriggerInp              trigger_input;
    
    double                  sr;
    int                     sigvs;
    short                   strum_connected;
    short                   fm_patched;
    
    int                     pcount;
    
};


static void MiRings_Ctor(MiRings *unit);
static void MiRings_Dtor(MiRings *unit);
static void MiRings_next(MiRings *unit, int inNumSamples);
void myObj_info(MiRings *unit);



static void MiRings_Ctor(MiRings *unit) {
    printf("MiRings constructor --!\n");
    
    printf("sr: %f\n", SAMPLERATE);
    //kSampleRate = SAMPLERATE;
    //a0 = (440.0f / 8.0f) / kSampleRate;

    // init some params
    unit->in_level = 0.0f;
    
    // allocate memory
    unit->reverb_buffer = (uint16_t*)RTAlloc(unit->mWorld, 65536*sizeof(uint16_t));

    // init with zeros?
    //memset(unit->reverb_buffer, 0, 65536*sizeof(uint16_t));

    unit->strummer = new rings::Strummer;
    //unit->part = new rings::Part;
    //unit->string_synth = new rings::StringSynthPart;
    
    //printf("sizeof strummer: %ld\n", sizeof(rings::Strummer));
    //printf("sizeof strummer2 :%ld\n", sizeof(*MiRings::strummer));
    printf("MiRings zero out --!\n");
    memset(unit->strummer, 0, sizeof(rings::Strummer));
    //memset(unit->part, 0, sizeof(rings::Part));
    //memset(unit->string_synth, 0, sizeof(rings::StringSynthPart));
    
    printf("MiRings init --!\n");
    unit->strummer->Init(0.01, rings::kSampleRate / kBlockSize);
    unit->part.Init(unit->reverb_buffer);
    unit->string_synth.Init(unit->reverb_buffer);
    
    printf("MiRings set params --!\n");
    
    unit->part.set_polyphony(2);
    unit->part.set_model(rings::RESONATOR_MODEL_MODAL);
        //RESONATOR_MODEL_STRING_AND_REVERB
        //RESONATOR_MODEL_FM_VOICE
        //RESONATOR_MODEL_MODAL
    unit->string_synth.set_polyphony(2);
    unit->string_synth.set_fx(rings::FX_REVERB);
    
    unit->performance_state.internal_exciter = false;
    unit->performance_state.internal_strum = true;
    unit->performance_state.internal_note = true;
    
    unit->pcount = 0;
    
    printf("MiRings END constructor --!\n");
    SETCALC(MiRings_next);        // tells sc synth the name of the calculation function
    //MiRings_next(unit, 64);       // do we reallly need this?
    

    
}


static void MiRings_Dtor(MiRings *unit) {
    
    SCWorld_Allocator alloc(ft, unit->mWorld);

    delete unit->strummer;
    //delete unit->part;
    //delete unit->string_synth;
    if(unit->reverb_buffer) {
        RTFree(unit->mWorld, unit->reverb_buffer);
        //free(unit->reverb_buffer);
    }
    printf("done freeing memory!\n");
}


#pragma mark ----- dsp loop -----

void MiRings_next( MiRings *unit, int inNumSamples)
{
    
    float *in = IN(0);
    float *trig_in = IN(1);
    
    float voct_in = IN0(2);
    float struct_in = IN0(3);
    float bright_in = IN0(4);
    float damp_in = IN0(5);
    float pos_in = IN0(6);
    bool  useTrigger = IN0(7);
    bool bypass = IN0(8);
    
    float *out1 = OUT(0);
    float *out2 = OUT(1);
    
    float input[64];

    size_t     size = kBlockSize;   //rings::kBlockSize;
    uint8_t    count = 0;
    rings::Patch *patch = &unit->patch;
    
    float pitch = abs(voct_in);
    CONSTRAIN(pitch, 0.f, 127.f);
    unit->performance_state.note = pitch;
    
    float freq = 0.f;
    CONSTRAIN(freq, -1.0f, 1.f);   // ??
    float transpose = freq * 60.0f;
    unit->performance_state.tonic = transpose + 12.0f;
    
    //unit->performance_state.tonic = 0.0f;
    

    CONSTRAIN(struct_in, 0.0f, 0.9995f);
    patch->structure = struct_in;
    float chord = struct_in;
    chord *= static_cast<float>(rings::kNumChords - 1);       // 11 - 1
    int32_t chord_ = static_cast<int32_t>(chord);
    CONSTRAIN(chord_, 0, rings::kNumChords - 1);
    unit->performance_state.chord = chord_;

    CONSTRAIN(bright_in, 0.0f, 1.0f);
    patch->brightness = bright_in;

    CONSTRAIN(damp_in, 0.0f, 1.0f);
    patch->damping = damp_in;
    
    CONSTRAIN(pos_in, 0.0f, 1.0f);
    patch->position = pos_in;
    
    // we have to copy input vector (why?)
    //for(int i=0; i<kBlockSize; i++)
      //  input[i] = in[i];
    std::copy(&in[0], &in[size], &input[0]);
    
    //unit->performance_state.strum = unit->trigger_input.rising_edge();
    //unit->performance_state.strum = false;
    
    
    //if(useTrigger && !unit->performance_state.internal_strum) {
    if(useTrigger) {
        unit->performance_state.internal_strum = false;
        float trig = 0.f;
        for(int i=0; i<inNumSamples; ++i) {
            if(trig_in[i] > 0.001f) {
                trig = 1.0f;
                break;
            }
        }
        unit->trigger_input.Read(trig);
    }
    else
        unit->performance_state.internal_strum = true;
    

    unit->pcount++;
    if(unit->pcount >= 2000) {
        unit->pcount = 0;
        //myObj_info(unit);
    }
    

    unit->part.set_bypass(bypass);
    
    unit->strummer->Process(in, size, &unit->performance_state);
    //if(unit->performance_state.strum)
      //  printf("STRUM!\n");
    unit->part.Process(unit->performance_state, unit->patch, input, out1, out2, size);
    
    /*
    for(count = 0; count < inNumSamples; count += size) {
        
        
        float trig = 0.f;
        for(int i=0; i<inNumSamples; ++i) {
            if(in[i] > 0.001f) {
                trig = 1.0f;
                unit->performance_state.strum = true;
                printf("trigger!\n");
                break;
            }
        }*/
        
        /*
        unit->strummer->Process(in+count, size, &unit->performance_state);
        unit->part.Process(unit->performance_state, unit->patch, in+count, out1+count, out2+count, size);
        
    }*/
    /*
    for(int i=0; i<size; i++) {
        out1[i] = in[i];
        out2[i] = in[i];
    }*/
    
}


void myObj_info(MiRings *unit)
{
    
    rings::Patch p = unit->patch;
    rings::PerformanceState *ps = &unit->performance_state;
    rings::Part *pa = &unit->part;
    //rings::State *st = self->settings->mutable_state();

    printf( "Patch ----------------------->\n");
    printf("structure: %f\n", p.structure);
    printf("brightness: %f\n", p.brightness);
    printf("damping: %f\n", p.damping);
    printf("position: %f\n", p.position);
    
    printf("Performance_state ----------->\n");
    printf("strum: %d\n", ps->strum);
    printf("internal_exciter: %d\n", ps->internal_exciter);
    printf("internal_strum: %d\n", ps->internal_strum);
    printf("internal_note: %d\n", ps->internal_note);
    printf("tonic: %f\n", ps->tonic);
    printf("note: %f\n", ps->note);
    printf("fm: %f\n", ps->fm);
    printf("chord: %d\n", ps->chord);
    
    printf("polyphony: %d\n", pa->polyphony());
    printf("model: %d\n", pa->model());
    //printf("easter_egg: %d\n", st->easter_egg);
    
    printf( "-----\n");
    
}


PluginLoad(MiRings) {
    ft = inTable;
    DefineDtorUnit(MiRings);
}



