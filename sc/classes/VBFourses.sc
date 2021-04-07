
// by volker boehm, 2017, https://vboehm.net

VBFourses : MultiOutUGen {
	*ar { arg freqarray, smoother = 0.5;
		^this.multiNewList( ['audio', smoother] ++ freqarray.asArray );
	}
	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(4, rate);
	}
}