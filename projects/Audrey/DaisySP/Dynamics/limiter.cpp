//#include "dsp.h"
#include <cmath>
#include "limiter.h"

#define SLOPE(out, in, positive, negative)                \
    {                                                     \
        float error = (in)-out;                           \
        out += (error > 0 ? positive : negative) * error; \
    }

namespace daisysp
{

inline float Crossfade(float a, float b, float fade) {
  return a + (b - a) * fade;
}

inline double SoftLimit(double x) {
  return x * (27.0 + x * x) / (27.0 + 9.0 * x * x);
}

inline double SoftClip(double x) {
  if (x < -3.0) {
    return -1.0;
  } else if (x > 3.0) {
    return 1.0;
  } else {
    return SoftLimit(x);
  }
}


void Limiter::Init()
{
    peak_ = 0.5;
}

void Limiter::ProcessBlock(float *in, size_t size, float pre_gain)
{
    while(size--)
    {
        float pre  = *in * pre_gain;
        float peak = fabsf(pre);
        SLOPE(peak_, peak, 0.05, 0.00002);
        float gain = (peak_ <= 1.0 ? 1.0 : 1.0 / peak_);
        *in++      = SoftLimit(pre * gain); // * 0.7);
    }
}


} //namespace daisysp
