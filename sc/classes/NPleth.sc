// https://vboehm.net

NPleth : UGen {
	*ar {
		arg algo=0, knob1=0.5, knob2=0.5, freq = 880.0, res = 0.5;
		^this.multiNew('audio', algo, knob1, knob2, freq, res);
	}
	*kr {
		arg algo=0, knob1=0.5, knob2=0.5, freq = 880.0, res = 0.5;
		^this.multiNew('control', algo, knob1, knob2, freq, res);
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }
}