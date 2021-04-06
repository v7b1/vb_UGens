/*
 *  VBPVoc.cpp
 *  
 *
 *  Created by vboehm on 25.05.15.
 *  Copyright 2015 vb_audio. All rights reserved.
 *
 */

#include "SC_PlugIn.h"
#include "FFT_UGens.h"
#if defined(__APPLE__) && !defined(SC_IPHONE)
	#include <Accelerate/Accelerate.h>
#endif

#define ONEOVERPI	(1.0 / pi)


#define CHECK_BUF \
if (!bufData) { \
unit->mDone = true; \
ClearUnitOutputs(unit, inNumSamples); \
return; \
}


// InterfaceTable contains pointers to function in the host (server)
//static InterfaceTable *ft;

struct VBPVoc : public Unit {
	uint32	m_fftsize, m_frames, m_hopsize, m_pos, m_idx;
	float	*m_buffer, *m_fftbuf;
	float	**m_lastphase, **m_outbuf;
	scfft	*m_scfft;
    uint8   m_numTracksToOutput;
	
	float	m_fbufnum;
	SndBuf	*m_buf;

};

// function declarations, exposed to C
static void VBPVoc_Ctor(VBPVoc *unit);
static void VBPVoc_Dtor(VBPVoc *unit);
static void VBPVoc_next(VBPVoc *unit, int numSamples);
inline float interpol(float a, float b, float frac );
inline float phasewrap( float input );


static void VBPVoc_Ctor(VBPVoc *unit) 
{
	unit->m_fbufnum = -1.f;
	unit->m_buf = unit->mWorld->mSndBufs;
	unit->m_pos = (int)IN0(1);
	unit->m_idx = 0;
    
    int fftsize = fabsf(IN0(2));
    if (fftsize == 0) {
        printf("fft size can't be zero!\n");
        fftsize = 2048;
    }
    // make sure fftsize is a power of 2
    int log2n = log2(fftsize) + 0.5;
    unit->m_fftsize = 1 << log2n;

	int overlap = 4;	//(int)IN0(3);  fixed to 4x
	int numOutputs = unit->mNumOutputs;
	
	unit->m_hopsize = unit->m_fftsize / overlap ;
	printf("fftsize: %d -- hopsize: %d\n", unit->m_fftsize, unit->m_hopsize);
    printf("overlap fixed at 4\n");
	
	
#if SC_FFT_FFTW
	printf("using FFTW\n");
#elif SC_FFT_VDSP 
	printf("using vDSP\n");
#elif SC_FFT_GREEN
	printf("using FFTGreen...\n");
#endif
	
	
	GET_BUF
	if (!bufData) {
		printf("can't get buffer!\n");
		SETCALC(*ClearUnitOutputs);
		unit->mDone = true;
		return;
	}
	
    printf("input frames: %d\n", bufFrames);
    printf("input channles: %d\n", bufChannels);
    
	// how many pvoc tracks are in analysis file?
	// pvoc file stores mag on first, phasdiff on second channel.
    // so two channels in file make one pvoc track
	int pvocTracks = bufChannels>>1;
    printf("pvoc tracks: %d\n", pvocTracks);
    printf("num of outputs: %d\n", numOutputs);
    
    unit->m_numTracksToOutput = pvocTracks;
    if( numOutputs > pvocTracks) {
        printf("warning: more UGen outputs than pvocTracks in file!\n");
    }
    else if( pvocTracks > numOutputs) {
        printf("warning: more pvocTracks in file than UGen outputs!\n");
        unit->m_numTracksToOutput = numOutputs;
    }

	
	// allocate memory
	int size = unit->m_fftsize * sizeof(float);
	unit->m_lastphase = (float**)RTAlloc(unit->mWorld, numOutputs*sizeof(float*));
	unit->m_outbuf = (float**)RTAlloc(unit->mWorld, numOutputs*sizeof(float*));
	for(int i=0; i<unit->m_numTracksToOutput; i++) {
		unit->m_lastphase[i] = (float*)RTAlloc(unit->mWorld, size);
		unit->m_outbuf[i] = (float*)RTAlloc(unit->mWorld, size);
		
		memset(unit->m_lastphase[i], 0, size);
		memset(unit->m_outbuf[i], 0, size);
	}
	unit->m_buffer = (float*)RTAlloc(unit->mWorld, size);
	unit->m_fftbuf = (float*)RTAlloc(unit->mWorld, size);
	
    memset(unit->m_buffer, 0, size);        // just for safety
	memset(unit->m_fftbuf, 0, size);
		
	
	SCWorld_Allocator alloc(ft, unit->mWorld);
	unit->m_scfft = scfft_create(unit->m_fftsize, unit->m_fftsize, kHannWindow, 
								 unit->m_fftbuf, unit->m_buffer, kBackward, alloc);

	
	SETCALC(VBPVoc_next);
}


static void VBPVoc_Dtor(VBPVoc *unit) 
{
	printf("try to free %d tracks\n", unit->m_numTracksToOutput);
	
	for(int i=0; i<unit->m_numTracksToOutput; ++i) {
		if(unit->m_lastphase[i])
			RTFree(unit->mWorld, unit->m_lastphase[i]);
		if(unit->m_outbuf[i])
			RTFree(unit->mWorld, unit->m_outbuf[i]);
	}
	if(unit->m_lastphase) RTFree(unit->mWorld, unit->m_lastphase);
	if(unit->m_outbuf) RTFree(unit->mWorld, unit->m_outbuf);
	if( unit->m_buffer) RTFree(unit->mWorld, unit->m_buffer);
	if(unit->m_fftbuf) RTFree(unit->mWorld, unit->m_fftbuf);
	
	SCWorld_Allocator alloc(ft, unit->mWorld);
	if(unit->m_scfft) scfft_destroy(unit->m_scfft, alloc);
	printf("done freeing memory!\n");
}


static void VBPVoc_next(VBPVoc *unit, int numSamples)
{
	uint32	fftsize = unit->m_fftsize;
	uint32	fftsize2 = fftsize>>1;
	uint32	hopsize = unit->m_hopsize;
	float	pos = fabs(IN0(1));
	uint32	idx = unit->m_idx;

	float	*fftbuf = unit->m_fftbuf;
	float	**outbuf = unit->m_outbuf;
	float	*buffer = unit->m_buffer;
	float	**lastphase = unit->m_lastphase;
	float	mag, phas, phasDelta;
	uint8	numOutputs = unit->mNumOutputs;
    uint8   numTracksToOutput = unit->m_numTracksToOutput;
    uint8   numSilentOutputs = unit->mNumOutputs - numTracksToOutput;
	
	if (idx & hopsize) {

		idx = 0;
		
		GET_BUF
		if (!bufData) {
			ClearUnitOutputs(unit, numSamples);
			return;
		}
        
		float framepos = pos * ((bufFrames/fftsize2)-1);
		int readpos = (int)framepos;		
		float frac = framepos - readpos;
		readpos *= fftsize2;
		
        //printf("pos: %f -- framepos: %f -- readpos: %d -- bufFrames: %u\n", pos, framepos, readpos, bufFrames);
       
		 if( readpos >= (bufFrames - fftsize2)) {
             printf("oops!\n");
			 ClearUnitOutputs(unit, numSamples);
			 return;
		 }

		
		readpos *= bufChannels;				// interleaved data in buffer

		// loop over number of output channels

		for(int k=0; k<numTracksToOutput; k++) {
			int offset = (k<<1) + readpos;
			
            
//#if defined(__APPLE__) && !defined(SC_IPHONE)
#if SC_FFT_VDSP
			// interpolate -----------------------------------------------
			int nextFrameIndex = offset+fftsize2*bufChannels;
			vDSP_vintb(bufData+offset, bufChannels, bufData+(nextFrameIndex), bufChannels,
					   &frac, fftbuf, 2, fftsize2);	// interpolate magnitudes
			vDSP_vintb(bufData+(offset+1), bufChannels, bufData+(nextFrameIndex+1), bufChannels,
					   &frac, fftbuf+1, 2, fftsize2); // interpolate phaseDiffs

             
			// accumulate phase differences -------------------------------
			vDSP_vadd(fftbuf+1, 2, lastphase[k], 1, fftbuf+1, 2, fftsize2);
			
			for(int j=0; j<fftsize2; j++) {
				uint32 index = (j<<1)+1;
				
				// TODO make vectorized version of phasewrap
				phas = phasewrap(fftbuf[index]);
				fftbuf[index] = lastphase[k][j] = phas;

			}
			vDSP_rect(fftbuf, 2, fftbuf, 2, fftsize2);

#else
            // interpolate ------------------------------------------------
            int nextFrameOffset = fftsize2*bufChannels;
			for(int j=0; j<fftsize2; j++) {
                // bufChannels: real number of channels in analysis file
				uint32 index = j*bufChannels+offset;
                uint32 index2 = index + nextFrameOffset;
                mag = bufData[index] + frac * (bufData[index2] - bufData[index]);
                phasDelta = bufData[index+1] + frac * (bufData[index2+1] - bufData[index+1]);
                
                // accumulate phase differences and wrap ------------------
                phas = phasewrap(phasDelta + lastphase[k][j]);	
				// poltocar...
				fftbuf[j<<1] = mag * cosf( phas );
				fftbuf[(j<<1)+1] = mag * sinf( phas );
				
				lastphase[k][j] = phas;
			}
#endif
			
			scfft_doifft(unit->m_scfft);
			
			for(int j=0; j<(fftsize-hopsize); j++)
				buffer[j] += outbuf[k][j+hopsize];
			memcpy(outbuf[k], buffer, fftsize*sizeof(float));
			
		}
	}
	
	for(int k=0; k<numTracksToOutput; k++)
		memcpy(OUT(k), outbuf[k]+idx, numSamples*sizeof(float));
    
    // if we demanded more UGen outputs than pvoc tracks in file,
    // make sure, that the rest of the outputs are zeroed out
    for(int k=0; k<numSilentOutputs; k++)
        memset(OUT(k+numTracksToOutput), 0, numSamples*sizeof(float));
	
	idx += numSamples;
	unit->m_idx = idx;
}



inline float interpol(float a, float b, float frac )
{
	return a+ frac*(b-a);
}


inline float phasewrap( float input ) {
	long qpd = input * ONEOVERPI;
	if(qpd >= 0) qpd += qpd&1;
	else qpd -= qpd&1;
	return ( input - pi * (float)qpd );
}



void PVocAnalysis(World *world, struct SndBuf *analbuf, struct sc_msg_iter *msg) 
{
	int		start, writepos;
	uint32	inputbufnum = msg->geti();
	int     fftsize = msg->geti();
    int     overlap = 4;    //msg->geti();

	// make sure fftsize is a power of 2
    int     log2n = log2(fftsize) + 0.5;
    fftsize = 1 << log2n;
	int		n2 = fftsize >> 1;
	int		hopsize = fftsize / overlap;
	
	printf("---> start analysing!\nbufnum: %d -- fftsize: %d -- overlap: %d\n", inputbufnum, fftsize, overlap);
	
	
	/* 
	 NOTE: have to implement FFT ourselves... 
	 we can't use vDSP routines from SC_fftlib.cpp as they are not thread-safe!
	 */
	
	
	if (inputbufnum >= world->mNumSndBufs) inputbufnum = 0;	// good idea???
	SndBuf *inputbuf = world->mSndBufs + inputbufnum;
	float *samps = inputbuf->data;		    // input audio samples
	float *pvocdata = analbuf->data;		// outgoing pvoc samples

	int inputFrames = inputbuf->frames;
    int fftFrames = (inputFrames - fftsize) / hopsize + 1;
	int nchnls = inputbuf->channels;
    int rest = inputFrames - fftFrames * hopsize;
	//int fftframes = (analbuf->frames - (fftsize-1)) / n2;
    int fftOuputFrames = analbuf->frames / n2;
    float scale = 0.5f;     // input scaling
	
	printf("inputframes: %d\n", inputFrames);
	printf("inputchannels: %d\n", nchnls);
    printf("fftFrames: %d\n", fftFrames);
    printf("rest: %d\n", rest);
	printf("fftOuputFrames: %d\n", fftOuputFrames);
	
	// allocate memory ------------------------
	float *inframe = (float*)RTAlloc(world, fftsize * sizeof(float));
	float *spectrum = (float*)RTAlloc(world, fftsize * sizeof(float));
	float *lastphas = (float*)RTAlloc(world, n2 * sizeof(float));
	
#if SC_FFT_FFTW
	printf("using FFTW\n");
#elif SC_FFT_VDSP 
	printf("using vDSP\n");
#elif SC_FFT_GREEN
	printf("using FFTGreen...\n");
#endif

	
#if SC_FFT_VDSP
	//printf("using our own vDSP calls...\n");
	
	COMPLEX_SPLIT	A;
	A.realp = (float*)RTAlloc(world, n2 * sizeof(float));
	A.imagp = (float*)RTAlloc(world, n2 * sizeof(float));
	float *wind = (float*)RTAlloc(world, fftsize * sizeof(float));
	vDSP_hann_window(wind, fftsize, 0);
    vDSP_vsmul(wind, 1, &scale, wind, 1, fftsize);  // scale input by 0.5
	FFTSetup setupReal = vDSP_create_fftsetup(log2n, FFT_RADIX2);
	
#else
	SCWorld_Allocator alloc(ft, world);
	scfft *m_scfft = scfft_create(fftsize, fftsize, kHannWindow, inframe, spectrum, kForward, alloc);
#endif
	
	

//#if defined(__APPLE__) && !defined(SC_IPHONE)
	
	
	// loop over number of channels in input buffer
	for(int k=0; k<nchnls; k++) {
		
        // reset stored phases from last frame
		memset(lastphas, 0, n2*sizeof(float));
		
		// loop over number of fft frames ----------------------
		for(int i=0; i<fftFrames; i++) {
			
			start = i * hopsize * nchnls + k;
			
			
#if defined(SC_FFT_VDSP)
            // apply windowing
            vDSP_vmul(samps+start, nchnls, wind, 1, inframe, 1, fftsize);
			
			//------------ do FFT ------------------------------
			vDSP_ctoz( (COMPLEX *) inframe, 2, &A, 1, n2);
            // do forward transform
			vDSP_fft_zrip( setupReal, &A, 1, log2n, FFT_FORWARD);
			vDSP_ztoc( &A, 1, (COMPLEX *)spectrum, 2, n2);
			
			// ----------- calc magnitudes & phases ------------
            // cartesian to polar conversion (in place)
			vDSP_polar(spectrum, 2, spectrum, 2, n2);
			
#else
			// ----------- do windowing & FFT ------------------
            for(int j=0; j<fftsize; j++)
                inframe[j] = samps[j*nchnls+start];
			scfft_dofft(m_scfft);
			
			// ---------- calc magnitudes & phases -------------
			for(int j=0; j<n2; j++) {
				int index = j<<1;
				float real = spectrum[index];
				float imag = spectrum[index+1];
				spectrum[index] = sqrtf(real*real + imag*imag);		//magnitudes 
				spectrum[index+1] = atan2f(imag, real);				// phases
			}
			
#endif
            // zero out DC/Nyquist?
            spectrum[0] = spectrum[1] = 0.f;
			
			// calc phase differences and wrap ----------------
			for(int j=0; j<n2; j++) {
				int index = (j<<1)+1;
				float phase = spectrum[index];
				spectrum[index] = phasewrap( phase - lastphas[j] );
				lastphas[j] = phase;
			}
            // spectrum now contains magnitudes and phase differences
            // in interleaved format
			
			
			// store pvoc data --------------------------------
			writepos = (i+1)*n2;    // start with an empty frame
			
			if(nchnls==1)
				memcpy(pvocdata+(writepos*2), spectrum, fftsize*sizeof(float));
			else {
                // k is track to write,
                // *2 because mag and phasdiffs go to different channels
                int offset = writepos*(nchnls<<1)+(k<<1);       // TODO: check this for multichannel!!!
				for(int j=0; j<n2; j++) {
					//int index = offset+(j<<nchnls);
                    int index = offset+(j*nchnls*2);
					memcpy(pvocdata+index, spectrum+(j<<1), 2*sizeof(float));
				}
			}
			
		}
	}
	
	
	// ------------ clean up -------------------------
	
	 RTFree(world, inframe);
	 RTFree(world, spectrum);
	 RTFree(world, lastphas);

	
#if SC_FFT_VDSP
	 RTFree(world, A.realp);
	 RTFree(world, A.imagp);
	 RTFree(world, wind);
	vDSP_destroy_fftsetup(setupReal);
	
#else
	if(m_scfft)
		scfft_destroy(m_scfft, alloc);
#endif
	
	printf("---> i'm done!\n");
	
}


#define MAXFFTSIZE        2097152        // about 47.5 seconds @ sr: 44.1 kHz
#define RAND_SCALE    1.0/RAND_MAX

float getRand() {
    float out = ((float)rand() * RAND_SCALE)*2-1;
    return out;
}


#ifdef SC_FFT_FFTW
#include "fftw3.h"
#endif


void BufferFreeze(World *world, struct SndBuf *inputbuf, struct sc_msg_iter *msg)
{
    int        i, j, k;
    float    magthresh = msg->getf();
    float    *samps = inputbuf->data;        // input audio samples
    int        n = inputbuf->frames;
    int        n2 = n>>1;
    float    scale = 1.0/n;
    int        nchnls = inputbuf->channels;
    
    if(n>MAXFFTSIZE) {
        printf("sorry, buffer too large!\n");
        return;
    }
    
#ifdef SC_FFT_FFTW
    
    // allocate memory
    float *in = (float *) fftwf_malloc(sizeof(float) * n);
    float *out = (float *) fftwf_malloc(sizeof(float) * n);
    fftwf_complex *spec = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * (n/2+1));
    fftwf_plan pIN = fftwf_plan_dft_r2c_1d( n, in, spec, FFTW_ESTIMATE );
    fftwf_plan pOUT = fftwf_plan_dft_c2r_1d( n, spec, out, FFTW_ESTIMATE );
    
    // initialize
    memset(in, 0, sizeof(float)*n);
    
    printf("---> start freezing with magthresh: %f!\n", magthresh);
    
    
    for(k=0; k<nchnls; k++)
    {
        for(i=0; i<n; i++)
            in[i] = samps[i*nchnls+k];
        
        fftwf_execute(pIN);
        
        
        for(i=1; i<n2; i++) {
            float mag = sqrtf(spec[i][0]*spec[i][0] + spec[i][1]*spec[i][1]);        // calc magnitude
            if(mag>=magthresh) {
                float rnd = getRand() * M_PI ;        // randomize phases
                spec[i][0] = mag * cosf(rnd);
                spec[i][1] =  mag * sinf(rnd);
            }
            else {
                spec[i][0] = 0.0;
                spec[i][1] = 0.0;
            }
        }
        
        // zero out DC and Nyquist
        spec[0][0] = 0;
        spec[0][1] = 0;
        spec[n2][0] = 0;
        spec[n2][1] = 0;
        
        fftwf_execute(pOUT);            //---- do inverse FFT
        
        for(i=0; i<n; i++)
            samps[i*nchnls+k] = out[i] / n;
    }
    
    
    
    // clean up
    fftwf_free(in);
    fftwf_free(out);
    fftwf_free(spec);
    fftwf_destroy_plan(pIN);
    fftwf_destroy_plan(pOUT);
#else
    printf("sorry, buffer freezing is only available with FFTW");
#endif
}


void initVBPVoc(InterfaceTable *it)
{
	DefineDtorUnit(VBPVoc);
	DefineBufGen("PVocAnal", PVocAnalysis);
#ifdef SC_FFT_FFTW
	DefineBufGen("FreezeBuf", BufferFreeze);
#endif
}
