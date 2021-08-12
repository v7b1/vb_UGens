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
    SCM
 
    A Shuffling Clock Multiplier
    based on SCM eurorack module by 4MS
    original code by Dan Green
 
    volker böhm, 2021
 
 **/

#include "SC_PlugIn.h"
#include <cstdio>

#include "cv_skip.inc"

static InterfaceTable *ft;

#define MIN_PW 1
#define INTERNAL 9
#define CLKIN 0


#define CONSTRAIN(var, min, max) \
if (var < (min)) { \
var = (min); \
} else if (var > (max)) { \
var = (max); \
}


struct SCM : public Unit
{
    uint32_t    tmr[10];
    uint8_t     rotation;
    bool        faster_switch;
    uint8_t     slippage_adc, pw_adc, shuffle_adc, skip_adc;
    uint32_t    period, pww;
    // period of each output
    uint32_t    per[7];     //per0=0,per1=0,per2=0,per3=0,per4=0,per5=0,per7=0;
    // rotation amount
    uint8_t     d[7]; //d0=0,d1=0,d2=0,d3=0,d4=0,d5=0,d7=0; // 'multiply-by' amount for each jack
    // divider for each output
    uint8_t     dd[5]; //dd2=0,dd3=0,dd4=0,dd5=0,dd7=0;
    //pulse width amount for each jack
    uint32_t    pw[7];  //pw0=8400,pw1=4200,pw2=2800,pw3=2200,pw4=1680,pw5=1400,pw7=1050;
    uint8_t     skip_pattern;
    uint32_t    slipamt[5]; //slipamt2=0,slipamt3=0,slipamt4=0,slipamt5=0,slipamt7=0;
    int32_t     slip[5];    //int32_t slip2=0,slip3=0,slip4=0,slip5=0,slip6=0;
    uint8_t     p[5];   //p2=0,p3=0,p4=0,p5=0,p6=0; //current pulse # in the sequence for each jack
    unsigned char s[5];
    uint8_t     slip_every, slip_howmany;
    
    uint32_t    now;
    uint8_t     update_pulse_params;
    uint32_t    clockin_irq_timestamp;
    
    bool        got_internal_clock;
    bool        got_external_clock;
    bool        mute;
    
    uint8_t     output[8];
    float       sr;
    float       bpm;
    bool        last_clk, last_resync;
    
};



static void SCM_next(SCM *unit, int inNumSamples);
static void SCM_Ctor(SCM *unit);



// utilities

inline uint32_t div32x8(uint32_t a, uint8_t b) {
    // in the orginal this is written in assembler;
    if(b != 0)
        return a / b;
    else
        return a;
}

inline uint32_t div32_8(uint32_t a, uint8_t b) {
    
    if (b==1) return (a);
    else if (b==2) return (a>>1);
    else if (b==4) return (a>>2);
    else if (b==8) return (a>>3);
    else if (b==16) return (a>>4);
    else return div32x8(a,b);
}


uint32_t calc_pw(uint8_t pw_adc, uint32_t period) {
    
    if (pw_adc < 4) return(MIN_PW);
    else if (pw_adc < 6) return(period>>5);
    else if (pw_adc < 10) return(period>>4);
    else if (pw_adc < 16) return((period>>4)+(period>>6));
    else if (pw_adc < 22) return((period>>4)+(period>>5));
    else if (pw_adc < 28) return((period>>4)+(period>>5)+(period>>6));
    else if (pw_adc < 32) return(period>>3);
    else if (pw_adc < 64) return((period>>3)+(period>>5));
    else if (pw_adc < 80) return((period>>3)+(period>>4));
    else if (pw_adc < 96) return((period>>3)+(period>>4)+(period>>5));
    else if (pw_adc < 112) return(period>>2);
    else if (pw_adc < 128) return((period>>2)+(period>>4));
    else if (pw_adc < 142) return((period>>2)+(period>>3));
    else if (pw_adc < 160) return((period>>2)+(period>>3)+(period>>4));
    else if (pw_adc < 178) return(period>>1);
    else if (pw_adc < 180) return((period>>1) + (period>>4));
    else if (pw_adc < 188) return((period>>1) + (period>>3));
    else if (pw_adc < 196) return((period>>1) + (period>>2));
    else if (pw_adc < 202) return((period*3)>>2);
    else if (pw_adc < 206) return((period*7)>>3);
    else return((period*15) >> 4);
    
    
}



void myUnit_period(SCM *unit, float bpm) {
    if (bpm > 1.0)
        unit->period = (unit->sr * 60.0f / bpm) + 0.5f;
    unit->bpm = bpm;
    unit->update_pulse_params = 1;
    
    std::printf("set bpm: %f -- period: %u\n", unit->bpm, unit->period);
}


void myUnit_rotate(SCM *unit, uint8_t n)
{
    unit->rotation = n;
    for(int i=0; i<6; ++i)
        unit->d[i] = ((unit->rotation + i) & 7) + 1;
    unit->d[6] = ((unit->rotation + 7) & 7) + 1;
    
    for(int i=0; i<5; ++i)
        unit->dd[i] = unit->d[i+2];
    
    if (unit->faster_switch) { //*4
        for(int i=0; i<7; ++i) {
            unit->d[i] <<= 2;
            //            std::printf("d[%d]: %ld", i, unit->d[i]);
        }
    }
    std::printf("rotate: %u\n", unit->rotation);
    unit->update_pulse_params = 1;
}


void myUnit_slip(SCM *unit, uint8_t n)
{
    CONSTRAIN(n, 0, 255);
    unit->slippage_adc = n;
    unit->update_pulse_params = 1;
}


void myUnit_shuffle(SCM *unit, uint8_t n)
{
//    CONSTRAIN(n, 0, 255);
    unit->shuffle_adc = n;
    uint16_t temp_u16 = n & 0x7F;       //chop to 0-127|0-127
    temp_u16 = (temp_u16 * 5) >>7;      //scale 0..127 to 0-4
    unit->slip_every = temp_u16 + 2;    //shift to 2-6
    
    if (n <= 127)
        unit->slip_howmany = 0;
    else
        unit->slip_howmany = ((n & 0b00011000) >> 3) + 1; //1..4
    
    //1..(slip_every-1)
    if (unit->slip_howmany >= unit->slip_every) unit->slip_howmany = unit->slip_every - 1;
    
}


void myUnit_skip(SCM *unit, uint8_t n)
{
//    CONSTRAIN(n, 0, 255);
    unit->skip_adc = n;
    if (n > 128) unit->skip_pattern =~ skip[0xFF-n];
    else unit->skip_pattern = skip[n];
}


void myUnit_pw(SCM *unit, uint8_t n) {
//    CONSTRAIN(n, 0, 255);
    unit->pw_adc = n;
    unit->update_pulse_params = 1;
}


void update_pulse_parameters(SCM *unit)
{
    uint32_t    pw = calc_pw(unit->pw_adc, unit->period);
    uint8_t     min_pw = MIN_PW + 1;
    
    for(int i=0; i<7; ++i) {
        unit->pw[i] = div32_8(pw, unit->d[i]);
        
        unit->per[i] = div32_8(unit->period, unit->d[i]);        // oder eher hier ??
        
        if( (unit->per[i] > min_pw) && (unit->pw[i] < MIN_PW)) {  // TODO: check that!
            unit->pw[i] = MIN_PW;
            //            std::printf("pw[%d] set to MIN_PW", i);
        }
        
        //        unit->per[i] = div32_8(unit->period, unit->d[i]);      // hier ??
    }
    
    
    if (unit->slippage_adc == 0) {
        for(int i=0; i<5; ++i)
            unit->slipamt[i] = 0;
    } else {
        
        for(int i=0; i<5; ++i)
            unit->slipamt[i] = (unit->slippage_adc * (unit->per[i+2] - (unit->pw[i+2] + 1))) >> 8;
        
    }
    
    if (unit->update_pulse_params == 2) {       //clock in received
        
        for(int i=0; i<5; ++i)
            unit->slip[i] = unit->slipamt[i];
    }
    
    unit->update_pulse_params = 0;
}





void info(SCM *unit) {
    
    std::printf( "-----------------\n" );
    for(int k=0; k<7; ++k) {
        std::printf("[%d] per: %d - d: %d - pw: %u\n", k, unit->per[k], unit->d[k], unit->pw[k]);
    }
    
    for(int k=0; k<5; ++k) {
        std::printf("[%d] dd: %d - p: %d - s: %d - slip: %d - amt: %d\n", k, unit->dd[k], unit->p[k], unit->s[k], unit->slip[k], unit->slipamt[k]);
    }
    
    std::printf("skip_pattern: %d\n", unit->skip_pattern);
    std::printf("slip_every: %d\n", unit->slip_every);
    std::printf("slip_howmany: %d\n", unit->slip_howmany);
    std::printf("faster_switch: %d\n", unit->faster_switch);
    std::printf("mute_switch: %d\n", unit->mute);
    std::printf("external clock: %d\n", unit->got_external_clock);
    std::printf("irq_timestamp: %d\n", unit->clockin_irq_timestamp);
    
    for(int k=0; k<10; ++k)
        std::printf("tmr[%d]: %u\n", k, unit->tmr[k]);
    
    std::printf("----\n");
    
}




static void SCM_Ctor(SCM *unit)
{
    
    SETCALC(SCM_next);

    unit->sr = SAMPLERATE;
    
    memset(unit->tmr, 0, 10 * sizeof(uint32_t));
    
    unit->rotation = 0;
    unit->faster_switch = false;
    
    unit->pw_adc = 0;
    unit->slippage_adc = unit->shuffle_adc = unit->skip_adc = 0;
    unit->skip_pattern = 0xFF;
    unit->slip_every = 2;
    unit->slip_howmany = 0;
    unit->clockin_irq_timestamp = 0;
    unit->got_internal_clock = false;
    unit->got_external_clock = false;
    unit->mute = false;
    
    for(int i=0; i<7; i++) {
        unit->d[i] = 0;
        unit->per[i] = 0;
    }
    
    unit->bpm = 120;
    myUnit_period(unit, unit->bpm);
    
    memset(unit->output, 0, 8 * sizeof(uint8_t));
    
    for(int i=0; i<5; i++) {
        unit->dd[i] = 0;
        unit->slipamt[i] = 0;
    }
    
    //pulse width amount for each jack
    unit->pw[0] = 8400;
    unit->pw[1] = 4200;
    unit->pw[2] = 2800;
    unit->pw[3] = 2200;
    unit->pw[4] = 1680;
    unit->pw[5] = 1400;
    unit->pw[6] = 1050;
    
    unit->now = 0;
    unit->update_pulse_params = 1;
    update_pulse_parameters(unit);
    
    myUnit_rotate(unit, 0);
    
    unit->last_clk = unit->last_resync = false;
    
    
    SCM_next(unit, 1);
    
    info(unit);
}


void SCM_next(SCM *unit, int inNumSamples)
{
	float       *clock_in = IN(0);          // clock input (trigger/gate)
    float       bpm = IN0(1);
    uint8_t     rotate_in = IN0(2);
	uint8_t     slip_in = IN0(3);
    uint8_t     shuffle_in = IN0(4);
    uint8_t     skip_in = IN0(5);
    uint8_t     pw_in = IN0(6);
    
//    float      *resync_in = ins[6];
    
    
    bool        last_resync = unit->last_resync;
    bool        last_clk = unit->last_clk;
    
    
    uint32_t    period = unit->period;
    uint32_t    *tmr = unit->tmr;
    uint32_t    *per = unit->per;
    uint32_t    *pw = unit->pw;
    uint8_t     *p = unit->p;
    uint8_t     *dd = unit->dd;
    unsigned char *s = unit->s;
    uint32_t    *slipamt = unit->slipamt;
    int32_t     *slip = unit->slip;
    uint8_t     skip_pattern = unit->skip_pattern;
    uint8_t     slip_every = unit->slip_every;
    uint8_t     slip_howmany = unit->slip_howmany;
    bool        got_internal_clock = unit->got_internal_clock;
    bool        got_external_clock = unit->got_external_clock;
    uint32_t    clockin_irq_timestamp = unit->clockin_irq_timestamp;
    
    uint8_t     *output = unit->output;
    
    uint8_t     update_pulse_params = unit->update_pulse_params;
    uint8_t     outval = !unit->mute;      // if muted, output zeros instead of ones
   
    
    if (bpm != unit->bpm)
        myUnit_period(unit, bpm);
    
    if (rotate_in != unit->rotation)
        myUnit_rotate(unit, rotate_in);
    
    if (slip_in != unit->slippage_adc)
        myUnit_slip(unit, slip_in);
    
    if (shuffle_in != unit->shuffle_adc)
        myUnit_shuffle(unit, shuffle_in);
    
    if (skip_in != unit->skip_adc)
        myUnit_skip(unit, skip_in);
    
    if (pw_in != unit->pw_adc)
        myUnit_pw(unit, pw_in);
    
    
    for(int i=0; i<inNumSamples; i++)
    {
        // increment timers
        for(int k=0; k<10; ++k)
            tmr[k]++;
        
        // resync input, more like a 'restart pattern' input
//        bool resync = resync_in[i] > 0.0;
//        if (resync && !last_resync) {
//            for(int i=0; i<5; ++i) {
//                p[i] = s[i] = 0;
//                slip[i] = slipamt[i];
//            }
//        }
//        last_resync = resync;
        
        
        // clock input
        if (INRATE(0) == calc_FullRate) {
            bool clk = clock_in[i] > 0.0;
            if (clk && !last_clk) {
                // clock input!
                if (tmr[CLKIN] > 100) {
                    got_external_clock = true;
                    clockin_irq_timestamp = tmr[CLKIN];
                    tmr[CLKIN] = 0;
                    tmr[INTERNAL] = 0;
//                    std::printf("timestamp: %u\n", clockin_irq_timestamp);
                }
            
            }
            last_clk = clk;
        }
        
        
        uint32_t *now = &tmr[INTERNAL];
        if(*now >= period)
            got_internal_clock = true;
        
        
        if (got_internal_clock || (clockin_irq_timestamp != 0 && got_external_clock)) {
            if (!clockin_irq_timestamp) {
                if (tmr[INTERNAL] >= period)        // TODO: check! was: tmr[INTERNAL] > period
                    tmr[INTERNAL] -= period;
            } else {
                period = clockin_irq_timestamp;
                clockin_irq_timestamp = 0;
                got_external_clock = false;
                unit->period = period;
            }
            
            
            // reset timers
            memset(tmr+1, 0, 8 * sizeof(uint32_t));
            
            
            // set all outputs to ON
            for(int k=0; k<8; ++k)
                output[k] = outval;
            
            
            got_internal_clock = 0;
            
            for(int k=0; k<5; ++k) {
                p[k] = s[k] = 0;
            }
            
            update_pulse_params = 2;
            
        }
        
        if (update_pulse_params) {
            update_pulse_parameters(unit);
            update_pulse_params = 0;
        }
        
        
        uint32_t *noww = &tmr[1];
        if (*noww >= per[0]) {
            *noww = 0;
            output[0] = outval;
        }
        else if (*noww >= pw[0])
            output[0] = 0;
        
        
        noww = &tmr[2];
        if (*noww >= per[1]) {
            *noww = 0;
            output[1] = outval;
        }
        else if (*noww >= pw[1])
            output[1] = 0;
        
        
        //Jack 8: x8 no skip/shuffle
        //use d7/per7 which defaults to x8
        noww = &tmr[8];
        if (*noww >= per[6]) {
            *noww = 0;
            output[7] = outval;
        }
        else if (*noww >= pw[6])
            output[7] = 0;
        
        
        
        // the other s-outputs
        
        for(int k=0; k<5; ++k)
        {
            uint8_t ind = k+2;
            noww = &tmr[ind+1];
            if (*noww >= (per[k+2] + slip[k])) {
                *noww = 0;
                
                if ((skip_pattern >> (8 - dd[k])) & (1 << p[k]))
                    output[ind] = outval;
                
                if (++p[k] >= dd[k]) {
                    p[k] = 0;
                    s[k] = 0;
                    slip[k] = slipamt[k];
                }
                
                if (s[k] == slip_howmany) slip[k] = -slipamt[k];
                else slip[k] = 0;
                
                if (++s[k] >= slip_every) {
                    s[k] = 0;
                    slip[k] = slipamt[k];
                }
            }
            else if (*noww >= pw[ind])
                output[ind] = 0;
        }
        
        
        //Jack 3: x3, skip/shuffle
        //use d2,per2,slipamt2, which defaults to x3
        
        
        /* Turn jacks off whenever the pulsewidth has expired*/
        
        //        if (gettmr(1)>=pw0) {OFF(OUT_PORT1,0);}
        //        if (gettmr(2)>=pw1) {OFF(OUT_PORT1,1);}
        //        if (gettmr(3)>=pw2) {OFF(OUT_PORT1,2);}
        //        if (gettmr(4)>=pw3) {OFF(OUT_PORT1,3);}
        //        if (gettmr(5)>=pw4) {OFF(OUT_PORT1,4);}
        //        if (gettmr(6)>=pw5) {OFF(OUT_PORT1,5);}
        //        if (gettmr(7)>=pw7) {OFF(OUT_PORT2,7);}
        //        if (gettmr(8)>=pw7) {OFF(OUT_PORT2,6);}
        
        // output all states
        for(int k=0; k<8; ++k)
            OUT(k)[i] = (float)output[k];
    }
    
    unit->last_clk = last_clk;
    unit->last_resync = last_resync;
    unit->clockin_irq_timestamp = clockin_irq_timestamp;
    unit->got_internal_clock = got_internal_clock;
    unit->got_external_clock = got_external_clock;
}





PluginLoad(SCM)
{
    ft = inTable;
    DefineSimpleUnit(SCM);
}
