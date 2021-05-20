
// RCD, by volker boehm, 2021, https://vboehm.net
// based on 4ms RCD by Dan Green

RCD : MultiOutUGen {
	*ar {
		arg clock=0, rotate=0, reset=0, div=0, spread=0, auto=0, len=0, down=0, gate=0;
		^this.multiNew( 'audio', clock, rotate, reset, div, spread, auto, len, down, gate );
	}
	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(8, rate);
	}
}