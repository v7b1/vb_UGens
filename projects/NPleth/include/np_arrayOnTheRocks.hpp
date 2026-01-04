// Noise Plethora Plugins
// Copyright (c) 2021 Befaco / Jeremy Bernstein
// Open-source software
// Licensed under GPL-3.0-or-later

#pragma once

#include "synth_waveform.hpp"



class ArrayOnTheRocks: public Generator {
public:

    ArrayOnTheRocks() {}
    virtual ~ArrayOnTheRocks() {}

    // delete copy constructors
    ArrayOnTheRocks(const ArrayOnTheRocks&) = delete;
    ArrayOnTheRocks& operator=(const ArrayOnTheRocks&) = delete;


    virtual void init(int16_t *mem)
    {
        printf("test: %d\n", test);
        //noise1.amplitude(1);

        waveformMod.arbitraryWaveform(myWaveform, 172.0);

        waveformMod.begin(1.0f, 250, WAVEFORM_ARBITRARY);
        waveform.begin(1.0f, 500, WAVEFORM_SINE);
        
        mod_block.data = mem;
        mod_block.zeroAudioBlock();

    }


    virtual void reset()
    {

    }


    virtual void process(float k1, float k2, audio_block_t *out_block)
    {
        float knob_1 = k1;
        float knob_2 = k2;

        float pitch1 = knob_1 * knob_1;

        waveformMod.frequency(10.0f + (pitch1 * 10000.0f));
        waveform.frequency(100.0f + (pitch1 * 500.0f));
        waveform.amplitude(knob_2);
        
        waveform.update(&mod_block);
        waveformMod.update(&mod_block, nullptr, out_block);

    }

private:

    AudioSynthWaveform      waveform;
    AudioSynthWaveformModulated waveformMod;
    audio_block_t           mod_block;

//    const int16_t test = (int16_t)myRand(-28000);
    const int16_t test = (int16_t)(get_rand() * 28000.0f * 2.0f - 28000.0f);
    const int16_t myWaveform[256] = {0,  1895,  3748,  5545,  7278,  8934, 10506, 11984, 13362, 14634,
    test, 16840, 17769, 18580, 19274, 19853, 20319, 20678, 20933, 21093,
    21163, 21153, 21072, 20927, 20731, 20492, 20221, test, 19625, 19320,
    19022, 18741, 18486, 18263, 18080, 17942, 17853, 17819, 17841, 17920,
    18058, 18254, 18507, 18813, 19170, 19573, 20017, 20497, 21006, test,
    test, test, test, 23753, 24294, 24816, 25314, 25781, 26212, 26604,
    26953, test, test, 27718, 27876, 27986, test, test, test, 27989,
    27899, 27782, 27644, 27490, test, test, test, test, test, 26582,
    26487, 26423, test, test, test, test, test, 26812, 27012, 27248,
    27514, 27808, 28122, test, 28787, test, 29451, 29762, 30045, 30293,
    test, 30643, 30727, 30738, 30667, test, 30254, 29897, test, 28858,
    28169, 27363, 26441, 25403, 24251, 22988, 21620, 20150, 18587, 16939,
    15214, 13423, 11577,  9686,  7763,  5820,  3870,  1926,     0, -1895,
    -3748, -5545, -7278, -8934,-10506, test,-13362,-14634,-15794,-16840,
    -17769,-18580,-19274,-19853,-20319,-20678,-20933,-21093,-21163,-21153,
    -21072,-20927,(int16_t)-test,-20492,-20221,-19929,-19625,-19320,-19022,-18741,
    (int16_t)-test,-18263,-18080,-17942,-17853,(int16_t)-test,-17841,-17920,-18058,-18254,
    -18507,-18813,-19170,-19573,(int16_t)-test,-20497,-21006,-21538,-22085,-22642,
    -23200,-23753,(int16_t)-test,(int16_t)-test,-25314,-25781,-26212,-26604,-26953,-27256,
    -27511,-27718,(int16_t)-test,(int16_t)-test,(int16_t)-test,-28068,-28047,(int16_t)-test,-27899,-27782,
    -27644,-27490,-27326,(int16_t)-test,-26996,-26841,-26701,-26582,-26487,(int16_t)-test,
    -26392,-26397,-26441,-26525,(int16_t)-test,-26812,(int16_t)-test,-27248,-27514,-27808,
    -28122,-28451,(int16_t)-test,(int16_t)-test,(int16_t)-test,-29762,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,
    (int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,test,-28169,-27363,
    (int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,(int16_t)-test,test,
    (int16_t)-test, (int16_t)-test, (int16_t)-test, (int16_t)-test, (int16_t)-test, (int16_t)-test};


};



