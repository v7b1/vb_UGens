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
        setParameters(0.0, M_SQRT1_2);
    }

    void setParameters(double fc, double q) {
        
        const double g = std::tan(M_PI * fc);
        const double R = 1.0 / (2.0 * q);

        alpha0 = 1.0 / (1.0 + 2.0 * R * g + g * g);
        alpha = g;
        rho = 2.0 * R + g;
    }

    void process(double input) {
        hp = (input - rho * s1 - s2) * alpha0;
        bp = alpha * hp + s1;
        lp = alpha * bp + s2;
        s1 = alpha * hp + bp;
        s2 = alpha * bp + lp;
    }

    double output(FilterMode mode) {
        switch (mode) {
            case LOWPASS: return lp;
            case BANDPASS: return bp;
            case HIGHPASS: return hp;
            default: return 0.0;
        }
    }

private:
    double alpha, alpha0, rho;
    double hp = 0.0, bp = 0.0, lp = 0.0, s1 = 0.0, s2 = 0.0;
};



class StateVariableFilter4thOrder
{
public:
    StateVariableFilter4thOrder() {

    }

    void setParameters(double fc, double q) {
        double rootQ = std::sqrt(q);
        stage1.setParameters(fc, rootQ);
        stage2.setParameters(fc, rootQ);
    }

    double process(double input, FilterMode mode) {
        stage1.process(input);
        double s1 = stage1.output(mode);

        stage2.process(s1);
        double s2 = stage2.output(mode);

        return s2;
    }

private:
    StateVariableFilter2ndOrder stage1, stage2;
};




class Biquad
{
public:
    void reset() {
        xm1 = xm2 = ym1 = ym2 = 0.0;
    }
    
    void setParams(double cf, double Q)
    {
        double K = std::tan(M_PI * cf);
        
        // highpass
        double norm = 1.0 / (1.0 + K / Q + K * K);
        a0 = norm;
        a1 = -2.0 * norm;
        a2 = norm;
        b1 = 2.0 * (K * K - 1.0) * norm;
        b2 = (1.0 - K / Q + K * K) * norm;
        
    }
    
    double process(double input)
    {
        double y = a0 * input + a1 * xm1 + a2 * xm2 - b1 * ym1 - b2 * ym2;
        ym2 = ym1;
        ym1 = y;
        xm2 = xm1;
        xm1 = input;
        return y;
    }
    
    
private:
    double a0 = 1.0;
    double a1 = 0.0;
    double a2 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    
    double xm1 = 0.0;
    double xm2 = 0.0;
    double ym1 = 0.0;
    double ym2 = 0.0;
    
};




class DCBlocker
{
public:
    void init(double sr)
    {
        fc = 22.05 / sr;
        
        double poleInc = M_PI / 4;
        double firstAngle = poleInc / 2;
        double q1 = 1.0 / (2 * cos(firstAngle));
        double q2 = 1.0 / (2 * cos(firstAngle+poleInc));
        
        b1.setParams(fc, q1);
        b2.setParams(fc, q2);
        
    }
    
    double process(double input)
    {
        double m = b1.process(input);
        return b2.process(m);
    }
    
    
private:
    
    Biquad b1;
    Biquad b2;
    double fc = 0.001;
    
    
};


class DCBlockerSimple
{
public:
    void init(double c)
    {
        if (c < 0.0) coef = 0.0;
        else if (c > 1.0) coef = 1.0;
        else coef = c;
        
        a0 = (1.0 + coef) * 0.5;
        a1 = -a0;
        
    }
    
    double process(double input)
    {
        double output = input * a0 + xm1 * a1 + ym1 * coef;
        ym1 = output;
        xm1 = input;
        return output;
    }
    
private:
    double coef = 0.999;
    double a0, a1;
    double xm1, ym1;
    
};



class Saturator {
    
public:
    double process(double input)
    {
        return input < 0.0 ? -satur(-input) : satur(input);
    }
    
private:
    
    double satur(double x)
    {
        x /= limit;
        double x1 = (x + 1.0) * 0.5;
        return limit * (offset + x1 - sqrt(x1 * x1 - y1 * x) / y1);
    }
    
    const double limit = 1.05;
    const double y1 = 0.96886;
    const double offset = 0.016071;
    
};
