/**
    Shuffling Clock Multiplier,
    based on 4ms SCM eurorack module by Dan Green
 
    vb, nov.2020
 */

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include "cv_skip.inc"


//MIN_PW 10 is about 1.28ms
#define MIN_PW 1 //10
#define INTERNAL 9
#define CLKIN 0



void *myObj_class;


typedef struct _myObj {
	t_pxobject	x_obj;
    
    uint32_t    tmr[10];
    uint8_t     rotation;
    bool        faster_switch;
    uint8_t     shuffle_adc, slippage_adc, pw_adc, skip_adc, rotate_adc;
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
    double      sr;
    double      bpm;
    bool        last_clk, last_resync;
	
} t_myObj;


void *myObj_new(t_symbol *s, long argc, t_atom *argv);
uint32_t div32x8(uint32_t a, uint8_t b); // ??
uint32_t div32_8(uint32_t a, uint8_t b);
void myObj_period(t_myObj *x, double f);
void myObj_rotate(t_myObj *x, long n);
void myObj_slip(t_myObj *x, long n);
void myObj_shuffle(t_myObj *x, long n);
void myObj_skip(t_myObj *x, long n);
void myObj_pw(t_myObj *x, long n);
t_max_err faster_setter(t_myObj *x, void *attr, long ac, t_atom *av);
void update_pulse_parameters(t_myObj *x);
uint32_t calc_pw(uint8_t pw_adc, uint32_t period);

void myObj_float(t_myObj *x, double f);
void myObj_int(t_myObj *x, long n);
void myObj_bang(t_myObj *x);
void myObj_dsp64(t_myObj *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void myObj_assist(t_myObj *x, void *b, long m, long a, char *s);
void myObj_free(t_myObj *x);



void *myObj_new(t_symbol *s, long argc, t_atom *argv)
{
	t_myObj *x = object_alloc(myObj_class);
	dsp_setup((t_pxobject *)x, 7);		// input, rotate, slip, shuffle, skip, pw, resync
	outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    outlet_new((t_pxobject *)x, "signal");
    //
    
    x->sr = sys_getsr();
    if(x->sr <= 0.0) x->sr = 44100.0;

    memset(x->tmr, 0, 10 * sizeof(uint32_t));
    
    x->rotation = 0;
    x->faster_switch = false;
    
    x->pw_adc = x->skip_adc = x->rotate_adc = 0;
    x->shuffle_adc = x->slippage_adc = 0;
    x->skip_pattern = 0xFF;
    x->slip_every = 2;
    x->slip_howmany = 0;
    x->clockin_irq_timestamp = 0;
    x->got_internal_clock = false;
    x->got_external_clock = false;
    x->mute = false;
    
    for(int i=0; i<7; i++) {
        x->d[i] = 0;
        x->per[i] = 0;
    }
    
    x->bpm = 120;
    myObj_period(x, x->bpm);
    
    memset(x->output, 0, 8 * sizeof(uint8_t));
    
    for(int i=0; i<5; i++) {
        x->dd[i] = 0;
        x->slipamt[i] = 0;
    }
    
    //pulse width amount for each jack
    x->pw[0] = 8400;
    x->pw[1] = 4200;
    x->pw[2] = 2800;
    x->pw[3] = 2200;
    x->pw[4] = 1680;
    x->pw[5] = 1400;
    x->pw[6] = 1050;
    
    x->now = 0;
    x->update_pulse_params = 1;

    myObj_rotate(x, 0);
    
    x->last_clk = x->last_resync = false;
    
    // process attributes
    attr_args_process(x, argc, argv);
    
	return (x);
}


void myObj_float(t_myObj *x, double f)
{
    long innum = proxy_getinlet((t_object *)x);
    
    if (innum == 0)
        myObj_period(x, f);     // bpm input sets period
    
}


void myObj_int(t_myObj *x, long n)
{
    long innum = proxy_getinlet((t_object *)x);

    switch (innum) {
        case 0:
//            x->period = (uint32_t)n;
//            myObj_period(x, n);
            // do nothing...
            break;
        case 1:         // rotate
            myObj_rotate(x, n);
            break;
        case 2:         // slip
            myObj_slip(x, n);
            break;
        case 3:         // shuffle
            myObj_shuffle(x, n);
            break;
        case 4:         // skip
            myObj_skip(x, n);
            break;
        case 5:         // pw
            myObj_pw(x, n);
            break;
    }
}

void myObj_bang(t_myObj *x) {
//    p2=0;p3=0;p4=0;p5=0;p6=0;
//    s2=0;s3=0;s4=0;s5=0;s6=0;
//    slip2=slipamt2;
//    slip3=slipamt3;
//    slip4=slipamt4;
//    slip5=slipamt5;
//    slip6=slipamt7;
    for(int i=0; i<5; ++i) {
        x->p[i] = x->s[i] = 0;
        x->slip[i] = x->slipamt[i];
    }
}


void myObj_period(t_myObj *x, double bpm) {
    if (bpm > 1.0)
        x->period = (x->sr * 60.0 / bpm) + 0.5;
    x->bpm = bpm;
    x->update_pulse_params = 1;  // TODO: good here?
}


void myObj_rotate(t_myObj *x, long n)
{
    x->rotation = n; //CLAMP(n, 0, 7);
    for(int i=0; i<6; ++i)
        x->d[i] = ((x->rotation + i) & 7) + 1;
    x->d[6] = ((x->rotation + 7) & 7) + 1;
    
    for(int i=0; i<5; ++i)
        x->dd[i] = x->d[i+2];
    
    if (x->faster_switch) { //*4
        for(int i=0; i<7; ++i) {
            x->d[i] <<= 2;
//            object_post(NULL, "d[%d]: %ld", i, x->d[i]);
        }
    }
    
    x->update_pulse_params = 1;
}


void myObj_slip(t_myObj *x, long n)
{
    x->slippage_adc = CLAMP(n, 0, 255);
    x->update_pulse_params = 1;
}


void myObj_shuffle(t_myObj *x, long n)
{
    n = CLAMP(n, 0, 255);
    uint16_t temp_u16 = n & 0x7F;       //chop to 0-127|0-127
    temp_u16 = (temp_u16 * 5) >>7;      //scale 0..127 to 0-4
    x->slip_every = temp_u16 + 2;    //shift to 2-6
    
    if (n <= 127)
        x->slip_howmany = 0;
    else
        x->slip_howmany = ((n & 0b00011000) >> 3) + 1; //1..4
    
    //1..(slip_every-1)
    if (x->slip_howmany >= x->slip_every) x->slip_howmany = x->slip_every - 1;
    
}


void myObj_skip(t_myObj *x, long n)
{
    n = CLAMP(n, 0, 255);
    if (n > 128) x->skip_pattern =~ skip[0xFF-n];
    else x->skip_pattern = skip[n];
}


void myObj_pw(t_myObj *x, long n) {
    x->pw_adc = CLAMP(n, 0, 255);
    x->update_pulse_params = 1;
    
}


void update_pulse_parameters(t_myObj *x)
{
    uint32_t    pw = calc_pw(x->pw_adc, x->period);
//    object_post(NULL, "pw: %ld", pw);

    uint8_t     min_pw = MIN_PW + 1;
    
    for(int i=0; i<7; ++i) {
        x->pw[i] = div32_8(pw, x->d[i]);
        
        x->per[i] = div32_8(x->period, x->d[i]);        // oder eher hier ??
        
        if( (x->per[i] > min_pw) && (x->pw[i] < MIN_PW)) {  // TODO: check that!
            x->pw[i] = MIN_PW;
//            object_post(NULL, "pw[%d] set to MIN_PW", i);
        }
        
//        x->per[i] = div32_8(x->period, x->d[i]);      // hier ??
    }
    
    
    if (x->slippage_adc == 0) {
        for(int i=0; i<5; ++i)
            x->slipamt[i] = 0;
    } else {
        
        for(int i=0; i<5; ++i)
            x->slipamt[i] = (x->slippage_adc * (x->per[i+2] - (x->pw[i+2] + 1))) >> 8;

    }
    
    if (x->update_pulse_params == 2) {       //clock in received
        
        for(int i=0; i<5; ++i)
            x->slip[i] = x->slipamt[i];
    }
    
    x->update_pulse_params = 0;
}






void myObj_info(t_myObj *x) {
    
    object_post((t_object*)x, "-----------------");
    
    for(int k=0; k<7; ++k) {
        object_post(NULL, "[%d] per: %d - d: %d - pw: %ld", k, x->per[k], x->d[k], x->pw[k]);
    }
    for(int k=0; k<5; ++k) {
        object_post(NULL, "[%d] dd: %d - p: %d - s: %d - slip: %d - amt: %d", k, x->dd[k], x->p[k], x->s[k], x->slip[k], x->slipamt[k]);
    }
    
    object_post(NULL, "skip_pattern: %d", x->skip_pattern);
    object_post(NULL, "slip_every: %d", x->slip_every);
    object_post(NULL, "slip_howmany: %d", x->slip_howmany);
    object_post(NULL, "faster_switch: %d", x->faster_switch);
    object_post(NULL, "mute_switch: %d", x->mute);
    object_post(NULL, "irq_timestamp: %d", x->clockin_irq_timestamp);
    
    for(int k=0; k<10; ++k)
        object_post(NULL, "tmr[%d]: %ld", k, x->tmr[k]);
    
}


#pragma mark ------------- utility functions --------------

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



#pragma mark ---------- attr setters ------------
//
t_max_err faster_setter(t_myObj *x, void *attr, long ac, t_atom *av)
{
    if (ac && av) {
        t_atom_long m = atom_getlong(av);
        bool sw = m != 0;
        if (sw != x->faster_switch) {
            x->faster_switch = sw;
            if (x->faster_switch)
                for(int i=0; i<7; ++i)
                    x->d[i] <<= 2;
            else
                for(int i=0; i<7; ++i)
                    x->d[i] >>= 2;
            
            x->update_pulse_params = 1;
        }
    }

    return MAX_ERR_NONE;
}




void myObj_perform64(t_myObj *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    double      *clock_in = ins[0];     // clock input (trigger/gate)
//    t_double *in2 = ins[1];     // rotate input

    double      *resync_in = ins[6];
    bool        last_resync = x->last_resync;
    bool        last_clk = x->last_clk;
    uint16_t    vs = (uint16_t)sampleframes;
    
    uint32_t    period = x->period;
    uint32_t    *tmr = x->tmr;
    uint32_t    *per = x->per;
    uint32_t    *pw = x->pw;
    uint8_t     *p = x->p;
    uint8_t     *dd = x->dd;
    unsigned char *s = x->s;
    uint32_t    *slipamt = x->slipamt;
    int32_t     *slip = x->slip;
    uint8_t     skip_pattern = x->skip_pattern;
    uint8_t     slip_every = x->slip_every;
    uint8_t     slip_howmany = x->slip_howmany;
    bool        got_internal_clock = x->got_internal_clock;
    bool        got_external_clock = x->got_external_clock;
    uint32_t    clockin_irq_timestamp = x->clockin_irq_timestamp;
    
    uint8_t     *output = x->output;
    
    uint8_t     update_pulse_params = x->update_pulse_params;
    uint8_t     outval = !x->mute;      // if muted, output zeros instead of ones
    
    
    for(int i=0; i<vs; ++i) {
        
        // increment timers
        for(int k=0; k<10; ++k)
            tmr[k]++;
        
        // resync input, more like a 'restart pattern' input
        bool resync = resync_in[i] > 0.0;
        if (resync && !last_resync) {
            for(int i=0; i<5; ++i) {
                p[i] = s[i] = 0;
                slip[i] = slipamt[i];
            }
        }
        last_resync = resync;
        
        
        // clock input
        bool clk = clock_in[i] > 0.0;
        if (clk && !last_clk) {
            // clock input!
            got_external_clock = true;
            clockin_irq_timestamp = tmr[CLKIN];
            tmr[CLKIN] = 0;
            tmr[INTERNAL] = 0;
        }
        last_clk = clk;
        

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
            update_pulse_parameters(x);
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
            outs[k][i] = (double)output[k];
        
    }
    
    x->last_clk = last_clk;
    x->last_resync = last_resync;
    x->clockin_irq_timestamp = clockin_irq_timestamp;
    x->got_internal_clock = got_internal_clock;
    x->got_external_clock = got_external_clock;
    x->period = period;

}



void myObj_dsp64(t_myObj *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{

    if (samplerate != x->sr) {
        x->sr = samplerate;
        myObj_period(x, x->bpm);    //recalculate period
    }
    
    object_method(dsp64, gensym("dsp_add64"), x, myObj_perform64, 0, NULL);
}


void myObj_free(t_myObj *x) {
    dsp_free((t_pxobject *)x);
}


void myObj_assist(t_myObj *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) {
        switch (a) {
            case 0:
                sprintf(s,"(Signal) clock input (float) set bpm for internal clock");
                break;
            case 1:
                sprintf(s,"(Int) rotate");
                break;
            case 2:
                sprintf(s,"(Int) slip");
                break;
            case 3:
                sprintf(s,"(Int) shuffle");
                break;
            case 4:
                sprintf(s,"(Int) skip");
                break;
            case 5:
                sprintf(s,"(Int) pw");
                break;
            case 6:
                sprintf(s,"(Signal) resync");
                break;
        }
    }
    else {
        switch (a) {
            case 0:
                sprintf(s,"(Signal) trigger/gate x 1");
                break;
            case 1:
                sprintf(s,"(Signal) trigger/gate x 2");
                break;
            case 2:
                sprintf(s,"(Signal) trigger/gate x 3");
                break;
            case 3:
                sprintf(s,"(Signal) trigger/gate x 4");
                break;
            case 4:
                sprintf(s,"(Signal) trigger/gate x 5");
                break;
            case 5:
                sprintf(s,"(Signal) trigger/gate x 6");
                break;
            case 6:
                sprintf(s,"(Signal) trigger/gate x 7");
                break;
            case 7:
                sprintf(s,"(Signal) trigger/gate x 8");
                break;
        }
    }
}



void ext_main(void *r)
{
    t_class *c = class_new("vb.scm~", (method)myObj_new, (method)myObj_free, sizeof(t_myObj), NULL, A_GIMME, 0);
    
    class_addmethod(c, (method)myObj_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c, (method)myObj_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)myObj_int, "int", A_LONG, 0);
    class_addmethod(c, (method)myObj_info,"info",0);
    class_addmethod(c, (method)myObj_bang,"bang", 0);
    class_addmethod(c, (method)myObj_assist,"assist",A_CANT,0);
    class_dspinit(c);
    
    class_register(CLASS_BOX, c);
    myObj_class = c;
    
    
    // attributes
    CLASS_ATTR_CHAR(c, "4x", 0, t_myObj, faster_switch);
    CLASS_ATTR_ENUMINDEX(c, "4x", 0, "OFF ON");
    CLASS_ATTR_STYLE_LABEL(c, "4x", 0, "onoff", "4x faster on/off");
    CLASS_ATTR_FILTER_CLIP(c, "4x", 0, 1);
    CLASS_ATTR_ACCESSORS(c, "4x", NULL, (method)faster_setter);
    CLASS_ATTR_SAVE(c, "4x", 0);
    
    // mute switch
    CLASS_ATTR_CHAR(c, "mute", 0, t_myObj, mute);
    CLASS_ATTR_ENUMINDEX(c, "mute", 0, "OFF ON");
    CLASS_ATTR_STYLE_LABEL(c, "mute", 0, "onoff", "mute on/off");
    CLASS_ATTR_FILTER_CLIP(c, "mute", 0, 1);
    CLASS_ATTR_SAVE(c, "mute", 0);
    
    
    object_post(NULL, "vb.scm~ by vboehm.net, based on 4ms SCM by Dan Green");
}
