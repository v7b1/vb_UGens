

enum FilterMode {
    LOWPASS,
    BANDPASS,
    HIGHPASS,
    NUM_TYPES
};

// based on VCV Rack's code
// based on Chapter 4 of THE ART OF VA FILTER DESIGN and
// Chap 12.4 of "Designing Audio Effect Plugins in C++" Will Pirkle
class StateVariableFilter2ndOrder {
public:

    StateVariableFilter2ndOrder() {
        setParameters(0.01f, M_SQRT1_2);
    }

    void setParameters(float fc, float q) {
        
        const double g = std::tan(M_PI * fc);
        const double R = 1.0f / (2.0f * q);

        alpha0 = 1.0f / (1.0f + 2.0f * R * g + g * g);
        alpha = g;
        rho = 2.0f * R + g;
    }

    void process(float input) {
        hp = (input - rho * s1 - s2) * alpha0;
        bp = alpha * hp + s1;
        lp = alpha * bp + s2;
        s1 = alpha * hp + bp;
        s2 = alpha * bp + lp;
    }

    float output(FilterMode mode) {
        switch (mode) {
            case LOWPASS: return lp;
            case BANDPASS: return bp;
            case HIGHPASS: return hp;
            default: return 0.0f;
        }
    }

private:
    float alpha, alpha0, rho;
    float hp = 0.0f, bp = 0.0f, lp = 0.0f, s1 = 0.0f, s2 = 0.0f;
};



class StateVariableFilter4thOrder
{
public:
    StateVariableFilter4thOrder() {

    }

    void setParameters(float fc, float q) {
        float rootQ = std::sqrt(q);
        stage1.setParameters(fc, rootQ);
        stage2.setParameters(fc, rootQ);
    }

    float process(float input, FilterMode mode) {
        stage1.process(input);
        float s1 = stage1.output(mode);

        stage2.process(s1);
        float s2 = stage2.output(mode);

        return s2;
    }

private:
    StateVariableFilter2ndOrder stage1, stage2;
};




class Biquad
{
public:
    Biquad() {
        reset();
        setParams(0.001, 1.0);
    }
    
    void reset() {
        xm1 = xm2 = ym1 = ym2 = 0.0f;
    }
    
    void setParams(double cf, double Q)
    {
        const double K = std::tan(M_PI * cf);
        
        // highpass
        double norm = 1.0 / (1.0 + K / Q + K * K);
        a0 = norm;
        a1 = -2.0 * norm;
        a2 = norm;
        b1 = 2.0 * (K * K - 1.0) * norm;
        b2 = (1.0 - K / Q + K * K) * norm;
        
    }
    
    float process(float input)
    {
        float y = a0 * input + a1 * xm1 + a2 * xm2 - b1 * ym1 - b2 * ym2;
        ym2 = ym1;
        ym1 = y;
        xm2 = xm1;
        xm1 = input;
        return y;
    }
    
    
private:
    float a0, a1, a2, b1, b2;
    float xm1, xm2, ym1, ym2;
    
};




class DCBlocker
{
public:
    DCBlocker() {}
    
    void init(float sr)
    {
        const double fc = 22.05 / sr;
        
        const double poleInc = M_PI / 4.0;
        const double firstAngle = poleInc / 2.0;
        float q1 = 1.0 / (2.0 * cos(firstAngle));
        float q2 = 1.0 / (2.0 * cos(firstAngle+poleInc));
        
        b1.setParams(fc, q1);
        b2.setParams(fc, q2);
    }
    
    float process(float input)
    {
        float m = b1.process(input);
        return b2.process(m);
    }
    
    
private:
    
    Biquad b1;
    Biquad b2;
    
};


class Saturator {
    
public:
    float process(float input)
    {
        return input < 0.0f ? -satur(-input) : satur(input);
    }
    
private:
    
    float satur(float x)
    {
        float x1 = (x + 1.0f) * 0.5f;
        return offset + x1 - std::sqrt(x1 * x1 - y1 * x) / y1;
    }

    const float y1 = 0.96886f;
    const float offset = 0.016071f;
    
};
