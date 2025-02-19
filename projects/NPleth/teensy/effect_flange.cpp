/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Pete (El Supremo)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "effect_flange.h"


int16_t sin16_C(uint16_t theta) {
	static const uint16_t base[] =
	{ 0, 6393, 12539, 18204, 23170, 27245, 30273, 32137 };
	static const uint8_t slope[] =
	{ 49, 48, 44, 38, 31, 23, 14, 4 };

	uint16_t offset = (theta & 0x3FFF) >> 3; // 0..2047
	if (theta & 0x4000)
		offset = 2047 - offset;

	uint8_t section = offset / 256; // 0..7
	uint16_t b   = base[section];
	uint8_t  m   = slope[section];

	uint8_t secoffset8 = (uint8_t)(offset) / 2;

	uint16_t mx = m * secoffset8;
	int16_t  y  = mx + b;

	if (theta & 0x8000)
		y = -y;

	return y;
}

/******************************************************************/
//                A u d i o E f f e c t F l a n g e
// Written by Pete (El Supremo) Jan 2014
// 140529 - change to handle mono stream and change modify() to voices()
// 140207 - fix calculation of delay_rate_incr which is expressed as
//			a fraction of 2*PI
// 140207 - cosmetic fix to begin()
// 140219 - correct the calculation of "frac"

// circular addressing indices for left and right channels
//short AudioEffectFlange::l_circ_idx;
//short AudioEffectFlange::r_circ_idx;

//short * AudioEffectFlange::l_delayline = NULL;
//short * AudioEffectFlange::r_delayline = NULL;

// User-supplied offset for the delayed sample
// but start with passthru
//int AudioEffectFlange::delay_offset_idx = FLANGE_DELAY_PASSTHRU;
//int AudioEffectFlange::delay_length;

//int AudioEffectFlange::delay_depth;
//int AudioEffectFlange::delay_rate_incr;
//unsigned int AudioEffectFlange::l_delay_rate_index;
//unsigned int AudioEffectFlange::r_delay_rate_index;
// fails if the user provides unreasonable values but will
// coerce them and go ahead anyway. e.g. if the delay offset
// is >= CHORUS_DELAY_LENGTH, the code will force it to
// CHORUS_DELAY_LENGTH-1 and return false.
// delay_rate is the rate (in Hz) of the sine wave modulation
// delay_depth is the maximum variation around delay_offset
// i.e. the total offset is delay_offset + delay_depth * sin(delay_rate)
bool AudioEffectFlange::begin(short* delayline, int d_length, int delay_offset, int d_depth, float delay_rate) {
	bool all_ok = true;

	delay_length = d_length / 2;
	l_delayline = delayline;

	delay_depth = d_depth;
	// initial index
	l_delay_rate_index = 0;
	l_circ_idx = 0;
//	delay_rate_incr = (delay_rate * 2147483648.0) / APP->engine->getSampleRate();
    delay_rate_incr = (delay_rate * 2147483648.0) / AUDIO_SAMPLE_RATE_EXACT;   // vb


	delay_offset_idx = delay_offset;
	// Allow the passthru code to go through
	if (delay_offset_idx < -1) {
		delay_offset_idx = 0;
		all_ok = false;
	}
	if (delay_offset_idx >= delay_length) {
		delay_offset_idx = delay_length - 1;
		all_ok = false;
	}
	return (all_ok);
}


bool AudioEffectFlange::voices(int delay_offset, int d_depth, float delay_rate) {
	bool all_ok = true;

	delay_depth = d_depth;

//	delay_rate_incr = (delay_rate * 2147483648.0) / APP->engine->getSampleRate();
    delay_rate_incr = (delay_rate * 2147483648.0) / AUDIO_SAMPLE_RATE_EXACT;    //vb

	delay_offset_idx = delay_offset;
	// Allow the passthru code to go through
	if (delay_offset_idx < -1) {
		delay_offset_idx = 0;
		all_ok = false;
	}
	if (delay_offset_idx >= delay_length) {
		delay_offset_idx = delay_length - 1;
		all_ok = false;
	}
	l_delay_rate_index = 0;
	l_circ_idx = 0;
	return (all_ok);
}

void AudioEffectFlange::update(const audio_block_t* inputBlock, audio_block_t* outputBlock) {

	int idx;
	const short* inbp;
	short* outbp;
	short frac;
	int idx1;

	if (l_delayline == NULL)
		return;

	// do passthru
	if (delay_offset_idx == FLANGE_DELAY_PASSTHRU) {
		// Just passthrough

		if (inputBlock) {
			inbp = inputBlock->data;
			// fill the delay line
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				l_circ_idx++;
				if (l_circ_idx >= delay_length) {
					l_circ_idx = 0;
				}
				l_delayline[l_circ_idx] = *inbp++;
			}
			// transmit the unmodified block
			audio_block_t::copyBlock(inputBlock, outputBlock);
		}
		return;
	}

	//          L E F T  C H A N N E L
	if (inputBlock && outputBlock) {
		inbp = inputBlock->data;
		outbp = outputBlock->data;
		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			// increment the index into the circular delay line buffer
			l_circ_idx++;
			// wrap the index around if necessary
			if (l_circ_idx >= delay_length) {
				l_circ_idx = 0;
			}
			// store the current sample in the delay line
			l_delayline[l_circ_idx] = *inbp++;
			// The argument to the arm_sin_q15 function is NOT in radians. It is
			// actually, in effect, the fraction remaining after the division
			// of radians/(2*PI) which is then expressed as a positive Q15
			// fraction in the interval [0 , +1) - this is l_delay_rate_index.
			// l_delay_rate_index should probably be called l_delay_rate_phase
			// (sorry about that!)
			// It is a Q31 positive number of which the high order 16 bits are
			// used when calculating the sine. idx will have a value in the
			// interval [-1 , +1)
			frac = sin16_C(((l_delay_rate_index >> 16) & 0x7fff));
			// multiply the sin by the delay depth
			idx = (frac * delay_depth) >> 15;
			//Serial.println(idx);
			// Calculate the offset into the buffer
			idx = l_circ_idx - (delay_offset_idx + idx);
			// and adjust idx to point into the circular buffer
			if (idx < 0) {
				idx += delay_length;
			}
			if (idx >= delay_length) {
				idx -= delay_length;
			}

			// Here we interpolate between two indices but if the sine was negative
			// then we interpolate between idx and idx-1, otherwise the
			// interpolation is between idx and idx+1
			if (frac < 0)
				idx1 = idx - 1;
			else
				idx1 = idx + 1;
			// adjust idx1 in the circular buffer
			if (idx1 < 0) {
				idx1 += delay_length;
			}
			if (idx1 >= delay_length) {
				idx1 -= delay_length;
			}
			// Do the interpolation
			frac = (l_delay_rate_index >> 1) & 0x7fff;
			frac = (((int)(l_delayline[idx1] - l_delayline[idx]) * frac) >> 15);
			*outbp++ = (l_delayline[l_circ_idx] + l_delayline[idx] + frac) / 2;

			l_delay_rate_index += delay_rate_incr;
			if (l_delay_rate_index & 0x80000000) {
				l_delay_rate_index &= 0x7fffffff;
			}
		}
	}
}



