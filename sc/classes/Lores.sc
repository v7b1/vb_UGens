Lores : UGen {
	*ar {
		arg in, freq = 880.0, res = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, res).madd(mul, add);
	}
}
