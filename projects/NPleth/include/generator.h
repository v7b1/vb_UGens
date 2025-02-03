// Copyright 2024 vb
//
// Base class for all generators.

#ifndef NOISE_GENERATOR_H_
#define NOISE_GENERATOR_H_

#include <random>

std::random_device rd;
const uint32_t seed = rd();
std::mt19937 rng( seed );
std::uniform_int_distribution<uint32_t> uint_dist;


class Generator {

public:
  
    Generator() { }
    virtual ~Generator() { }


    virtual void reset() = 0;
    virtual void init(int16_t *mem) = 0;
    virtual void process (
                          float kn1,
                          float kn2,
                          audio_block_t *out_block) = 0;
    
    virtual float get_rand() {
        // return random floats from 0.0 .. 1.0
        return (float)uint_dist(rng) / UINT32_MAX;
    }


};


#endif  // NOISE_GENERATOR_H_
