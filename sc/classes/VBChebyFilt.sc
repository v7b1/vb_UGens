// vboehm.net

VBChebyFilt : UGen {
	*ar {
		arg in, freq = 880.0, mode = 0.0, order = 4.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, mode, order).madd(mul, add);
	}
}
