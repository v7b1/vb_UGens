// https://vboehm.net

NPleth : UGen {
	*ar {
		arg algo=0, knob1=0.5, knob2=0.5, freq = 0.75, res = 0.5, mul=1.0, add=0.0;
		^this.multiNew('audio', algo, knob1, knob2, freq, res).madd(mul, add);
	}
	*kr {
		arg algo=0, knob1=0.5, knob2=0.5, freq = 0.75, res = 0.5, mul=1.0, add=0.0;
		^this.multiNew('control', algo, knob1, knob2, freq, res).madd(mul, add);
	}
}