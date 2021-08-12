// https://vboehm.net

SCM : MultiOutUGen {
	*ar {
		arg clock=0, bpm = 120, rotate = 0, slip = 0, shuffle = 0,
		skip = 0, pw = 0;
		^this.multiNew('audio', clock, bpm, rotate, slip, shuffle, skip, pw);
	}
	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(8, rate);
	}
}
