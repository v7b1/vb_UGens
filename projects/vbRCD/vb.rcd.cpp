/**
 
 The MIT License (MIT)
 
 Copyright (c) 2012 Dan Green (danngreen1@gmail.com)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 _______         */



/**
    RCD
 
    A rotation clock divider
    based on RCD eurorack module by 4MS
 
    volker böhm, 2021
 
 **/

#include "SC_PlugIn.h"
#include <cstdio>

static InterfaceTable *ft;

const uint8_t rotate_table[8]= {0,1,2,3,5,7,11,15};

const uint8_t kNumGates = 8;



struct RCD : public Unit
{
    uint8_t clock_up, clock_down, reset2_up, count;

    uint8_t o[kNumGates];
    uint8_t div[kNumGates];
    
    uint8_t d, t;
    
    uint8_t div_mode;
    uint8_t spreadmode_switch;
    
    bool    last_trigger;
    bool    reset_bang;
    
    bool    gate_mode;
    uint8_t loop_len;
    uint8_t rotate_in;
    
    float   state[kNumGates];       // gate states
    short   rotate_connected;
    
};



static void RCD_next(RCD *unit, int inNumSamples);
static void RCD_Ctor(RCD *unit);


static void RCD_Ctor(RCD *unit)
{
    
    SETCALC(RCD_next);

    
    unit->clock_up=0; unit->clock_down=0; unit->reset2_up=0; unit->count=0;
    
    for(int i=0; i<kNumGates; i++) {
        unit->div[i] = i;
        unit->o[i] = 0;
        unit->state[i] = 0.0;
    }
    

    unit->d = 0;
    unit->t = 7;
    
    unit->div_mode = 0;
    unit->spreadmode_switch=0;
    
    unit->last_trigger = false;        // do we still need this?
    unit->reset_bang = false;
    unit->loop_len = 0;        // off
    unit->rotate_in = 0;
    
    unit->gate_mode = IN0(8) != 0.f;
    
    RCD_next(unit, 1);
}




void info(RCD *x) {

    std::printf( "-----------------\n" );
    for(int k=0; k<kNumGates; ++k) {
        std::printf("div[%d]: %d -- o[%d]: %d\n", k, x->div[k], k, x->o[k]);
    }

    std::printf("----\n");

}



uint8_t divby(uint8_t rot_amt, uint8_t max_divby, uint8_t jack, uint8_t spread) {
    if (spread) {
        if (max_divby==15)          // /2 /4 /6 /8 /10 /12 /14 /16 rotating within 1-16
            return ((((jack<<1)+1)+rot_amt) & 15);
        else if (max_divby==31)     // /4 /8 /12... /32 rotating within 1-32
            return ((((jack<<2)+3)+rot_amt) & 31);
        else if (max_divby==63)     // /8 /16 /20.../64 rotating within 1-64
            return ((((jack<<3)+7)+rot_amt) & 63);
        else
            return rotate_table[((jack+rot_amt) & 7)];
        
    } else {        //no spread, so jacks are 1...8 (or 1..16, etc depending on maxdivby)
        return ((jack+rot_amt) & max_divby);
    }
}



void recalc_divisions(RCD *unit)
{
    uint8_t d, t;
    uint8_t *div = unit->div;

    t = (1 << (unit->div_mode+3)) - 1;         // 7, 15, 31, 63
    d = unit->rotate_in;
    
    if (!unit->spreadmode_switch) {
        uint8_t offset = ((1 << unit->div_mode) >> 1) << 3;       // 0, 8, 16, 32
        d += offset;
    }
    
    for(int i=0; i<kNumGates; ++i) {
        div[i] = divby(d, t, i, unit->spreadmode_switch);
    }
    
    unit->t = t;
    
//    for(int k=0; k<kNumGates; ++k) {
//        std::printf("div[%d]: %d\t-- o[%d]: %d\n", k, unit->div[k], k, unit->o[k]);
//    }
//    std::printf("---\n");
}



void RCD_next(RCD *unit, int inNumSamples)
{
	float       *in1 = IN(0);          // clock input (trigger/gate)
    uint8_t     rotate = IN0(1);
	float       *reset_in = IN(2);     // reset input
    uint8_t     div_mode = IN0(3);
    bool        spread = IN0(4) != 0.f;
    bool        auto_reset = IN0(5) != 0.f;
    uint8_t     loop_len = IN0(6);
    bool        down_beat = IN0(7) != 0.f;
    
    uint16_t    vs = inNumSamples;
    
    bool        reset_bang = unit->reset_bang;
    bool        last_trigger = unit->last_trigger;
    uint8_t     *div = unit->div;
    uint8_t     *o = unit->o;
    uint8_t     count = unit->count;
    float       *state = unit->state;
    bool        reset_trig = false;
    
    unit->div_mode = div_mode;
    
    
    if(rotate != unit->rotate_in || spread != unit->spreadmode_switch) {
        unit->rotate_in = rotate;
        unit->spreadmode_switch = spread;
        recalc_divisions(unit);
    }
    
    if (INRATE(2) == calc_FullRate) {
        float sum = 0.f;
        for(int i=0; i<vs; ++i)
            sum += reset_in[i];
        reset_trig = ( sum > 0.f );
    }
    else
        reset_trig = (reset_in[0] > 0.f);
    
    if ( reset_trig || reset_bang ) {
        if (unit->reset2_up == 0) {
            unit->reset2_up = 1;
            memset(o, 0, kNumGates);
            
            reset_bang = false;
            count = 0;      // good ?
        }
        
    } else  unit->reset2_up = 0;
    
    
    
    for(int i=0; i<vs; i++)
    {
        bool trigger_in = in1[i] > 0.f;
        
        if(trigger_in)          // && !last_trigger)
        {
            unit->clock_down = 0;
            
            if (!unit->clock_up)
            {
                unit->clock_up = 1;     //rising edge only

                if (down_beat) {
                    for(int k=0; k<kNumGates; ++k)
                        if(o[k] == 0) state[k] = 1.0;
                    
                    if (unit->gate_mode) {
                        for(int k=0; k<kNumGates; ++k)
                            if(o[k] == ((div[k] >> 1) + 1)) state[k] = 0.0f;
                        
                    } else {
                        for(int k=0; k<kNumGates; ++k) {
                            if(++o[k] > div[k]) o[k] = 0;
                        }

                    }
                } else { //DOWNBEAT is off (upbeat)
                    if (unit->gate_mode)
                    {
                        for(int k=0; k<kNumGates; ++k)
                            if(o[k] == ((div[k] >> 1) + 1)) state[k] = 1.0f;
                        
                        for(int k=0; k<kNumGates; ++k)
                            if(o[k] == 0) state[k] = 0.0f;
                        
                    } else {
                        for(int k=0; k<kNumGates; ++k) {
                            if(++o[k] > div[k]) {
                                o[k] = 0;
                                state[k] = 1.0f;
                            }
                        }
                    }
                }
                
                if (auto_reset) {
                    //(t+1)*2: 16, 32, 64, 128
                    uint8_t limit = (loop_len > 0 ? loop_len : ((unit->t+1)<<1));
                    if (++count >= limit) {
                        count = 0;
                        memset(o, 0, kNumGates);        // reset all counters
                        std::printf("auto-reset!\n");
                    }
                }
                else {
                    count = 0;
                }
                
            }
            
        } else {
            
            unit->clock_up = 0;
            
            if (!unit->clock_down)
            {
                unit->clock_down = 1;      //rising edge only
                
                if (unit->gate_mode) {
                    if (down_beat) {
                        for(int k=0; k<kNumGates; ++k)
                            if (o[k] == ((div[k]+1)>>1)) state[k] = 0.0;
                        
                    } else {
                        for(int k=0; k<kNumGates; ++k)
                            if (o[k] == ((div[k]+1)>>1)) state[k] = 1.0;
                        
                    }
                    for(int k=0; k<kNumGates; ++k)
                        if (++o[k] > div[k]) o[k] = 0;
                    
                } else
                    memset(state, 0, kNumGates*sizeof(float));
            }
        }
        
        last_trigger = trigger_in;
        
        for(int k=0; k<kNumGates; ++k) {
            OUT(k)[i] = state[k];
        }
        
    }

    unit->reset_bang = reset_bang;
    unit->last_trigger = last_trigger;
    unit->count = count;

}



PluginLoad(RCD)
{
    ft = inTable;
    DefineSimpleUnit(RCD);
}
